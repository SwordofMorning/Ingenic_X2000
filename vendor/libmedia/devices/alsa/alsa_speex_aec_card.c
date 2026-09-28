#include <libmedia/filter/alsa_speex_aec_card.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <math.h>

#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

// 算法帮你缓多少处理好的帧
#define DEFAULT_AEC_BUFFER               10

struct buffer_queue {
    uint8_t *buffer;
    unsigned int count;
    unsigned int unit_size;
    unsigned int write;
    unsigned int read;
};

struct aec_card {
    struct alsa_pcm *alsa_player;
    struct alsa_pcm *alsa_capturer;

    struct alsa_ctl *playback_ctl;
    struct alsa_ctl *capture_ctl;

    // 用于对齐并输入到算法中的数据队列
    struct buffer_queue *playback_aec_queue;
    struct buffer_queue *capture_aec_queue;
    // 用于暂存待播放数据的队列
    struct buffer_queue *playback_queue;
    // 算法处理完的数据的队列
    struct buffer_queue *aec_queue;

    SpeexEchoState *speex_echo_state;
    SpeexPreprocessState *speex_preprocess_state;

    pthread_t aec_thread;
    volatile int run;

    // 每次给播放录制设备的帧的样本数
    int alsa_samples;
    // 每次对齐播放与录音音频的最大值
    int alsa_buffer_samples;
    // 录音延迟送入aec算法的样本数
    int delay_samples;

    pthread_mutex_t playback_mutex;
    pthread_cond_t playback_cond;
    pthread_mutex_t aec_mutex;
    pthread_cond_t aec_cond;

    int playback_pause;
    sem_t playback_start;

    struct speex_params *config;
    int rate;
    int channels;
    enum audio_frame_format format;
};

static struct buffer_queue *queue_create(int unit_size, int count)
{
    struct buffer_queue *queue = malloc(sizeof(*queue));
    assert(queue);
    // 预留1用于区分队列的满与空
    queue->count = count + 1;
    queue->unit_size = unit_size;
    queue->buffer = malloc(count * unit_size);
    assert(queue->buffer);
    queue->write = 0;
    queue->read = 0;
    return queue;
}

static void queue_destroy(struct buffer_queue *queue)
{
    free(queue->buffer);
    free(queue);
}

static int queue_count(struct buffer_queue *queue)
{
    return (queue->write - queue->read + queue->count) % queue->count;
}

static void queue_reset(struct buffer_queue *queue)
{
    queue->read = queue->write;
}

static int enqueue(struct buffer_queue *queue, uint8_t *buffer, int count)
{
    // queue->count - 1 为队列实际能缓存的个数
    if (count > (queue->count - 1) - queue_count(queue))
        return -1;

    if (queue->write + count > queue->count) {
        int left_size = (queue->count - queue->write) * queue->unit_size;
        memcpy(queue->buffer + queue->write * queue->unit_size, buffer, left_size);
        memcpy(queue->buffer, buffer + left_size, count * queue->unit_size - left_size);
    } else
        memcpy(queue->buffer + queue->write * queue->unit_size, buffer, count * queue->unit_size);

    queue->write = (queue->write + count) % queue->count;
    return 0;
}

static int enqueue_replace(struct buffer_queue *queue, uint8_t *buffer, int count)
{
    // queue->count - 1 为队列实际能缓存的个数
    int replace = count + queue_count(queue) - (queue->count - 1);
    if (replace > 0)
        queue->read = (queue->read + replace) % queue->count;

    return enqueue(queue, buffer, count);
}

static int dequeue(struct buffer_queue *queue, uint8_t *buffer, int count)
{
    if (count > queue_count(queue))
        return -1;

    if (queue->read + count > queue->count) {
        int left_size = (queue->count - queue->read) * queue->unit_size;
        memcpy(buffer, queue->buffer + queue->read * queue->unit_size, left_size);
        memcpy(buffer + left_size, queue->buffer, count * queue->unit_size - left_size);
    } else
        memcpy(buffer, queue->buffer + queue->read * queue->unit_size, count * queue->unit_size);

    queue->read = (queue->read + count) % queue->count;
    return 0;
}

static int to_sample_fmt(struct alsa_params *alsa_param)
{
    switch (alsa_param->format) {
    case SND_PCM_FORMAT_S16_LE:     return AUDIO_s16le;
    case SND_PCM_FORMAT_S32_LE:     return AUDIO_s32le;
    case SND_PCM_FORMAT_FLOAT_LE:   return AUDIO_flt;
    default:
        fprintf(stderr, "alsa_speex_aec_card: not support this alsa fmt: %d\n", alsa_param->format);
        return -1;
    }
}

static void *aec_thread_func(void *data)
{
    struct aec_card *card = data;
    prctl(PR_SET_NAME, "aec thread");

    int bpsample = audio_frame_bytes_per_sample(card->format);
    int delayed_samples = 0;

    uint8_t *capture_unit = malloc(bpsample * card->alsa_samples);
    assert(capture_unit);
    uint8_t *playback_unit = malloc(bpsample * card->alsa_samples);
    assert(playback_unit);
    uint8_t *aec_playback_unit = malloc(bpsample * card->config->aec_samples);
    assert(aec_playback_unit);
    uint8_t *aec_capture_unit = malloc(bpsample * card->config->aec_samples);
    assert(aec_capture_unit);
    uint8_t *aec_unit = malloc(bpsample * card->config->aec_samples);
    assert(aec_unit);
    uint8_t *alsa_aligned_blank_buffer = calloc(card->alsa_buffer_samples, bpsample);
    assert(alsa_aligned_blank_buffer);

    alsa_pcm_write(card->alsa_player, alsa_aligned_blank_buffer, card->alsa_buffer_samples);
    pthread_mutex_lock(&card->playback_mutex);
    pthread_cond_signal(&card->playback_cond);
    pthread_mutex_unlock(&card->playback_mutex);

    while (card->run) {
        int ret = alsa_pcm_read(card->alsa_capturer, capture_unit, card->alsa_samples);
        if (ret < 0)
            fprintf(stderr, "alsa_speex_aec_card ERR:alsa_pcm_read=%d\n", ret);
        ret = enqueue(card->capture_aec_queue, capture_unit, card->alsa_samples);
        if (ret < 0)
            fprintf(stderr, "alsa_speex_aec_card ERR: capture overrun\n");

        pthread_mutex_lock(&card->playback_mutex);
        int playback_samples = queue_count(card->playback_queue);
        if (playback_samples < card->alsa_samples) {
            memset(playback_unit, 0, bpsample * card->alsa_samples);
            dequeue(card->playback_queue, playback_unit, playback_samples);
        } else
            dequeue(card->playback_queue, playback_unit, card->alsa_samples);
        pthread_cond_signal(&card->playback_cond);
        pthread_mutex_unlock(&card->playback_mutex);

        ret = alsa_pcm_write(card->alsa_player, playback_unit, card->alsa_samples);
        if (ret < 0)
            fprintf(stderr, "alsa_speex_aec_card ERR:alsa_pcm_write=%d\n", ret);
        ret = enqueue(card->playback_aec_queue, playback_unit, card->alsa_samples);
        if (ret < 0)
            fprintf(stderr, "alsa_speex_aec_card ERR: playback overrun\n");

        if (delayed_samples != card->delay_samples) {
            int process_samples = fmin(card->delay_samples - delayed_samples, card->config->aec_samples);
            while (queue_count(card->capture_aec_queue) >= process_samples && process_samples > 0) {
                memset(aec_unit, 0, bpsample * card->config->aec_samples);
                dequeue(card->capture_aec_queue, aec_unit, process_samples);
                speex_preprocess_ctl(card->speex_preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, NULL);
                speex_preprocess_run(card->speex_preprocess_state, (void *)aec_unit);

                pthread_mutex_lock(&card->aec_mutex);
                ret = enqueue_replace(card->aec_queue, aec_unit, process_samples);
                assert(ret == 0);
                pthread_cond_signal(&card->aec_cond);
                pthread_mutex_unlock(&card->aec_mutex);

                delayed_samples += process_samples;
                process_samples = fmin(card->delay_samples - delayed_samples, card->config->aec_samples);
            }
        } else {
            while (queue_count(card->capture_aec_queue) >= card->config->aec_samples) {
                dequeue(card->capture_aec_queue, aec_capture_unit, card->config->aec_samples);
                dequeue(card->playback_aec_queue, aec_playback_unit, card->config->aec_samples);

                speex_echo_cancellation(card->speex_echo_state, (void *)aec_capture_unit, (void *)aec_playback_unit, (void *)aec_unit);
                speex_preprocess_ctl(card->speex_preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, card->speex_echo_state);
                speex_preprocess_run(card->speex_preprocess_state, (void *)aec_unit);

                pthread_mutex_lock(&card->aec_mutex);
                enqueue_replace(card->aec_queue, aec_unit, card->config->aec_samples);
                pthread_cond_signal(&card->aec_cond);
                pthread_mutex_unlock(&card->aec_mutex);
            }
        }
    }

    free(capture_unit);
    free(playback_unit);
    free(aec_playback_unit);
    free(aec_capture_unit);
    free(aec_unit);
    free(alsa_aligned_blank_buffer);

    return NULL;
}

struct aec_card *aec_card_open(struct alsa_speex_aec_card_param *p)
{
    if (p == NULL || p->alsa_params.rate == 0 || p->alsa_params.channels == 0) {
        fprintf(stderr, "alsa_speex_aec_card_param not set\n");
        return NULL;
    }
    struct aec_card *card = malloc(sizeof(*card));
    assert(card);
    memset(card, 0, sizeof(*card));

    card->config = &p->speex_params;
    card->rate = p->alsa_params.rate;
    card->channels = p->alsa_params.channels;
    card->format = to_sample_fmt(&p->alsa_params);

    // 默认aec的处理单元为10ms
    if (card->config->aec_samples == 0)
        card->config->aec_samples = p->alsa_params.rate * 10 / 1000;

    if (card->config->filter_count == 0)
        card->config->filter_count = 5;

    // 打开录音设备
    struct alsa_params alsa_capture_param;
    memcpy(&alsa_capture_param, &p->alsa_params, sizeof(alsa_capture_param));

    int sample_fmt = to_sample_fmt(&alsa_capture_param);
    if (sample_fmt == -1)
        return NULL;

    struct alsa_pcm *alsa_capturer = NULL;
    alsa_capturer = alsa_pcm_open_capture_device(p->alsa_capture_device);
    if (!alsa_capturer) {
        fprintf(stderr, "alsa_speex_aec_card: failed to open audio device\n");
        return NULL;
    }

    int ret = alsa_pcm_set_params(alsa_capturer, &alsa_capture_param);
    if (ret < 0) {
        fprintf(stderr, "alsa_speex_aec_card: failed to set audio params\n");
        goto close_alsa_capturer;
    }
    // 设置播放录音的样本数与内部的切片时间的样本数一致，确保每次获取到录音数据的时间间隔基本固定
    card->alsa_samples = alsa_capture_param.period_frames;
    // 录音设备每次最少缓多少样本真正给应用的最大值 (依据libhardware中的切片时间以及驱动调度的延时进行估算)
    card->alsa_buffer_samples = card->alsa_samples * 2 + 20 * p->alsa_params.rate / 1000;

    card->delay_samples = p->speex_params.capture_align_ms * p->alsa_params.rate / 1000;
    if (card->delay_samples <= 0)
        card->delay_samples = card->alsa_buffer_samples - p->alsa_params.rate / 1000; // -1ms的数据 避免录音数据先于播放数据导致算法失效

    // 打开播放设备
    struct alsa_params alsa_playback_param;
    memcpy(&alsa_playback_param, &p->alsa_params, sizeof(alsa_playback_param));
    sample_fmt = to_sample_fmt(&alsa_playback_param);
    if (sample_fmt == -1) {
        goto close_alsa_capturer;
    }

    struct alsa_pcm *alsa_player = NULL;
    alsa_player = alsa_pcm_open_playback_device(p->alsa_playback_device);
    if (!alsa_player) {
        fprintf(stderr, "alsa_speex_aec_card: failed to open audio device\n");
        goto close_alsa_capturer;
    }

    ret = alsa_pcm_set_params(alsa_player, &alsa_playback_param);
    if (ret < 0) {
        fprintf(stderr, "alsa_speex_aec_card: failed to set audio params\n");
        goto close_alsa_player;
    }

    card->alsa_capturer = alsa_capturer;
    card->alsa_player = alsa_player;

    // 打开录制声音控制
    if (p->alsa_capture_ctl_device != NULL && p->alsa_capture_ctl_name != NULL) {
        card->capture_ctl = alsa_ctl_open(p->alsa_capture_ctl_device, p->alsa_capture_ctl_name);
        alsa_ctl_set_value(card->capture_ctl, card->config->mic_volume_p);
    }

    // 打开播放声音控制
    if (p->alsa_playback_ctl_device != NULL && p->alsa_playback_ctl_name != NULL) {
        card->playback_ctl = alsa_ctl_open(p->alsa_playback_ctl_device, p->alsa_playback_ctl_name);
        alsa_ctl_set_value(card->playback_ctl, card->config->spk_volume);
    }

    card->speex_echo_state =
        speex_echo_state_init(card->config->aec_samples, card->config->aec_samples * card->config->filter_count);
    card->speex_preprocess_state = speex_preprocess_state_init(card->config->aec_samples, p->alsa_params.rate);
    speex_echo_ctl(card->speex_echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &p->alsa_params.rate);
    int enable = 1;
    speex_preprocess_ctl(card->speex_preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, (void *)&enable);

    int bpsample = audio_frame_bytes_per_sample(card->format);
    // 算法缓存录音音频的长度：录制2帧 + 算法1帧
    card->capture_aec_queue = queue_create(bpsample, card->alsa_samples * 2 + card->config->aec_samples);

    // 算法缓存播放音频的长度：播放延迟时间(对齐最多要多少帧) + 播放2帧　+ 算法1帧
    card->playback_aec_queue = queue_create(bpsample, card->alsa_buffer_samples + card->alsa_samples * 2 + card->config->aec_samples);

    // 待播放的帧：播放延迟时间(对齐最多要多少帧)
    card->playback_queue = queue_create(bpsample, card->alsa_buffer_samples);

    // 算法帮你缓多少帧
    card->aec_queue = queue_create(bpsample, card->config->aec_samples * DEFAULT_AEC_BUFFER);

    pthread_mutex_init(&card->playback_mutex, NULL);
    pthread_cond_init(&card->playback_cond, NULL);

    pthread_mutex_init(&card->aec_mutex, NULL);
    pthread_cond_init(&card->aec_cond, NULL);

    card->run = 1;
    ret = pthread_create(&card->aec_thread, NULL, aec_thread_func, card);
    assert(!ret);

    return card;

close_alsa_player:
    alsa_pcm_close(alsa_player);
close_alsa_capturer:
    alsa_pcm_close(alsa_capturer);
    return NULL;
}

void aec_card_close(struct aec_card *card)
{
    card->run = 0;

    card->playback_pause = 0;
    sem_post(&card->playback_start);
    sem_destroy(&card->playback_start);

    pthread_cond_signal(&card->playback_cond);
    pthread_cond_signal(&card->aec_cond);

    pthread_join(card->aec_thread, NULL);
    pthread_mutex_destroy(&card->playback_mutex);
    pthread_cond_destroy(&card->playback_cond);
    pthread_mutex_destroy(&card->aec_mutex);
    pthread_cond_destroy(&card->aec_cond);

    speex_echo_state_destroy(card->speex_echo_state);
    speex_preprocess_state_destroy(card->speex_preprocess_state);

    queue_destroy(card->playback_aec_queue);
    queue_destroy(card->playback_queue);
    queue_destroy(card->capture_aec_queue);
    queue_destroy(card->aec_queue);

    alsa_pcm_close(card->alsa_player);
    alsa_pcm_close(card->alsa_capturer);

    if (card->capture_ctl)
        alsa_ctl_close(card->capture_ctl);
    if (card->playback_ctl)
        alsa_ctl_close(card->playback_ctl);

    free(card);
}


int aec_card_display_frame(struct aec_card *card, struct audio_frame *frame)
{
    if (frame->channels != card->channels ||
        frame->sample_rate != card->rate ||
        frame->format != card->format) {
        fprintf(stderr, "alsa speex aec card: failed to playback this frame, channels = %d,sample_rate = %d,format = %d\n",
                         frame->channels, frame->sample_rate, frame->format);
        return -1;
    }

    if (card->playback_pause)
        sem_wait(&card->playback_start);

    int left_samples = frame->nb_samples;
    pthread_mutex_lock(&card->playback_mutex);

    while (left_samples > 0) {
        if (!card->run)
            break;
        int aec_pos = (frame->nb_samples - left_samples) * audio_frame_bytes_per_sample(frame->format);
        int enqueue_samples = fmin(card->alsa_samples, left_samples);
        while (enqueue(card->playback_queue, frame->data[0] + aec_pos, enqueue_samples) == -1) {
            pthread_cond_wait(&card->playback_cond, &card->playback_mutex);
            if (!card->run)
                break;
        }
        left_samples -= enqueue_samples;
    }
    pthread_mutex_unlock(&card->playback_mutex);
    return 0;
}

int aec_card_read_frame(struct aec_card *card, struct audio_frame **frame)
{
    struct audio_frame *aec_frame =
        audio_frame_alloc_with_buffer(card->rate, card->config->aec_samples, card->channels, card->format);
    int ret = 0;

    pthread_mutex_lock(&card->aec_mutex);
    do {
        ret = dequeue(card->aec_queue, aec_frame->data[0], card->config->aec_samples);
        if (!card->run) {
            ret = -1;
            break;
        }
        if (!ret)
            break;
        pthread_cond_wait(&card->aec_cond, &card->aec_mutex);
    } while (1);
    pthread_mutex_unlock(&card->aec_mutex);

    *frame = aec_frame;
    return ret;
}

int aec_card_pause_player(struct aec_card *card)
{
    if (card->capture_ctl)
        alsa_ctl_set_value(card->capture_ctl, card->config->mic_volume_m);

    card->playback_pause = 1;

    return 0;
}

int aec_card_resume_player(struct aec_card *card)
{
    if (card->capture_ctl)
        alsa_ctl_set_value(card->capture_ctl, card->config->mic_volume_p);

    card->playback_pause = 0;
    sem_post(&card->playback_start);

    return 0;
}

int aec_card_drop_capture(struct aec_card *card)
{
    pthread_mutex_lock(&card->aec_mutex);
    queue_reset(card->aec_queue);
    pthread_mutex_unlock(&card->aec_mutex);
    return 0;
}

int aec_card_avail_capture(struct aec_card *card)
{
    return queue_count(card->aec_queue) / card->config->aec_samples;
}

void alsa_aec_card_init_default_param(struct alsa_speex_aec_card_param *param, int channels, int rate)
{
    memset(param, 0, sizeof(*param));

    if (!access("/dev/snd/pcmC1D0c", F_OK))
        param->alsa_capture_device = "hw:1";
    else
        param->alsa_capture_device = "hw:0";

    if (!access("/dev/snd/pcmC1D0p", F_OK))
        param->alsa_playback_device = "hw:1";
    else
        param->alsa_playback_device = "hw:0";

    param->alsa_params.channels = channels;
    param->alsa_params.rate = rate;
    param->alsa_params.format = SND_PCM_FORMAT_S16_LE;
}
