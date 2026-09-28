#include <libmedia/filter/alsa_speex_aec_cc_card.h>
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

struct aec_cc_card {
    struct alsa_pcm *alsa_echo;
    struct alsa_pcm *alsa_capture;

    // 用于对齐并输入到算法中的数据队列
    struct buffer_queue *echo_queue;
    struct buffer_queue *capture_queue;
    // 算法处理完的数据的队列
    struct buffer_queue *aec_queue;

    SpeexEchoState *speex_echo_state;
    SpeexPreprocessState *speex_preprocess_state;

    pthread_t aec_thread;
    volatile int run;

    // 每次录制的样本数
    int samples;
    // capture录音延迟送入aec算法的样本数
    int delay_samples;

    pthread_mutex_t aec_mutex;
    pthread_cond_t aec_cond;

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
        fprintf(stderr, "alsa_speex_aec_cc_card: not support this alsa fmt: %d\n", alsa_param->format);
        return -1;
    }
}

static void *aec_thread_func(void *data)
{
    struct aec_cc_card *card = data;
    prctl(PR_SET_NAME, "aec thread");

    int bpsample = audio_frame_bytes_per_sample(card->format);
    int delayed_samples = 0;
    int ret;

    uint8_t *echo_unit = malloc(bpsample * card->samples);
    assert(echo_unit);
    uint8_t *capture_unit = malloc(bpsample * card->samples);
    assert(capture_unit);
    uint8_t *aec_unit = malloc(bpsample * card->samples);
    assert(aec_unit);

    while (card->run) {
        ret = alsa_pcm_read(card->alsa_echo, echo_unit, card->samples);
        if (ret < 0)
            fprintf(stderr, "alsa_speex_aec_cc_card ERR:echo alsa_pcm_read=%d\n", ret);

        if (delayed_samples < card->delay_samples) {
            int ignored_samples = fmin(card->delay_samples - delayed_samples, card->samples);
            ret = enqueue(card->echo_queue, echo_unit + ignored_samples * bpsample, card->samples - ignored_samples);
            delayed_samples += ignored_samples;
        } else
            ret = enqueue(card->echo_queue, echo_unit, card->samples);
        if (ret != 0)
            fprintf(stderr, "alsa_speex_aec_cc_card echo_queue overrun\n");

        ret = alsa_pcm_read(card->alsa_capture, capture_unit, card->samples);
        if (ret < 0)
            fprintf(stderr, "alsa_speex_aec_cc_card ERR:capture alsa_pcm_read=%d\n", ret);
        ret = enqueue(card->capture_queue, capture_unit, card->samples);
        if (ret != 0)
            fprintf(stderr, "alsa_speex_aec_cc_card capture_queue overrun\n");

        if (queue_count(card->echo_queue) >= card->samples) {
            dequeue(card->echo_queue, echo_unit, card->samples);
            dequeue(card->capture_queue, capture_unit, card->samples);

            speex_echo_cancellation(card->speex_echo_state, (void *)capture_unit, (void *)echo_unit, (void *)aec_unit);
            speex_preprocess_run(card->speex_preprocess_state, (void *)aec_unit);

            pthread_mutex_lock(&card->aec_mutex);
            enqueue_replace(card->aec_queue, aec_unit, card->samples);
            pthread_cond_signal(&card->aec_cond);
            pthread_mutex_unlock(&card->aec_mutex);
        }
    }

    free(echo_unit);
    free(capture_unit);
    free(aec_unit);

    return NULL;
}

struct aec_cc_card *aec_cc_card_open(struct alsa_speex_aec_cc_card_param *p)
{
    if (p == NULL || p->alsa_params.rate == 0 || p->alsa_params.channels == 0) {
        fprintf(stderr, "alsa_speex_aec_cc_card_param not set\n");
        return NULL;
    }
    struct alsa_params alsa_param;

    struct aec_cc_card *card = malloc(sizeof(*card));
    assert(card);
    memset(card, 0, sizeof(*card));

    card->rate = p->alsa_params.rate;
    card->channels = p->alsa_params.channels;
    card->format = to_sample_fmt(&p->alsa_params);
    card->delay_samples = p->speex_params.capture_align_ms * card->rate / 1000;

    // 默认aec的处理单元为10ms
    if (card->samples == 0)
        card->samples = p->alsa_params.rate * 10 / 1000;

    if (p->speex_params.filter_count == 0)
        p->speex_params.filter_count = 5;

    // 打开回采播放音频的录音设备
    memcpy(&alsa_param, &p->alsa_params, sizeof(alsa_param));
    int sample_fmt = to_sample_fmt(&alsa_param);
    if (sample_fmt == -1)
        return NULL;

    struct alsa_pcm *alsa_echo = NULL;
    alsa_echo = alsa_pcm_open_capture_device(p->alsa_echo_device);
    if (!alsa_echo) {
        fprintf(stderr, "alsa_speex_aec_cc_card: failed to open echo device\n");
        return NULL;
    }

    int ret = alsa_pcm_set_params(alsa_echo, &alsa_param);
    if (ret < 0 || alsa_param.rate != p->alsa_params.rate) {
        fprintf(stderr, "alsa_speex_aec_cc_card: failed to set echo params\n");
        goto close_alsa_echo;
    }

    // 打开录制外界声音的设备
    memcpy(&alsa_param, &p->alsa_params, sizeof(alsa_param));
    sample_fmt = to_sample_fmt(&alsa_param);
    if (sample_fmt == -1)
        goto close_alsa_echo;

    struct alsa_pcm *alsa_capture = NULL;
    alsa_capture = alsa_pcm_open_capture_device(p->alsa_capture_device);
    if (!alsa_capture) {
        fprintf(stderr, "alsa_speex_aec_cc_card: failed to open capture device\n");
        goto close_alsa_echo;
    }

    ret = alsa_pcm_set_params(alsa_capture, &alsa_param);
    if (ret < 0 || alsa_param.rate != p->alsa_params.rate) {
        fprintf(stderr, "alsa_speex_aec_cc_card: failed to set capture params\n");
        goto close_alsa_capture;
    }

    card->alsa_echo = alsa_echo;
    card->alsa_capture = alsa_capture;

    // 打开回采设备的声音控制
    if (p->alsa_echo_ctl_device != NULL && p->alsa_echo_ctl_name != NULL) {
        struct alsa_ctl *echo_ctl = alsa_ctl_open(p->alsa_echo_ctl_device, p->alsa_echo_ctl_name);
        if (echo_ctl) {
            alsa_ctl_set_value(echo_ctl, p->speex_params.echo_volume);
            alsa_ctl_close(echo_ctl);
        }
    }

    // 打开录制设备的声音控制
    if (p->alsa_capture_ctl_device != NULL && p->alsa_capture_ctl_name != NULL) {
        struct alsa_ctl *capture_ctl = alsa_ctl_open(p->alsa_capture_ctl_device, p->alsa_capture_ctl_name);
        if (capture_ctl) {
            alsa_ctl_set_value(capture_ctl, p->speex_params.capture_volume);
            alsa_ctl_close(capture_ctl);
        }
    }

    card->speex_echo_state =
        speex_echo_state_init(card->samples, card->samples * p->speex_params.filter_count);
    card->speex_preprocess_state = speex_preprocess_state_init(card->samples, card->rate);
    speex_echo_ctl(card->speex_echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &card->rate);
    int enable = 1;
    speex_preprocess_ctl(card->speex_preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, (void *)&enable);
    speex_preprocess_ctl(card->speex_preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE, card->speex_echo_state);

    int bpsample = audio_frame_bytes_per_sample(card->format);

    // 算法缓存回声音频的长度：算法２帧
    card->echo_queue = queue_create(bpsample, card->samples * 2);
    // 算法缓存录音音频的长度：延迟的样本数 + 算法２帧
    card->capture_queue = queue_create(bpsample, card->delay_samples + card->samples * 2);

    // 算法帮你缓多少帧
    card->aec_queue = queue_create(bpsample, card->samples * DEFAULT_AEC_BUFFER);

    pthread_mutex_init(&card->aec_mutex, NULL);
    pthread_cond_init(&card->aec_cond, NULL);

    card->run = 1;
    ret = pthread_create(&card->aec_thread, NULL, aec_thread_func, card);
    assert(!ret);

    return card;

close_alsa_capture:
    alsa_pcm_close(alsa_capture);
close_alsa_echo:
    alsa_pcm_close(alsa_echo);
    return NULL;
}

void aec_cc_card_close(struct aec_cc_card *card)
{
    card->run = 0;

    pthread_cond_signal(&card->aec_cond);

    pthread_join(card->aec_thread, NULL);
    pthread_mutex_destroy(&card->aec_mutex);
    pthread_cond_destroy(&card->aec_cond);

    queue_destroy(card->echo_queue);
    queue_destroy(card->capture_queue);
    queue_destroy(card->aec_queue);

    speex_echo_state_destroy(card->speex_echo_state);
    speex_preprocess_state_destroy(card->speex_preprocess_state);

    alsa_pcm_close(card->alsa_echo);
    alsa_pcm_close(card->alsa_capture);

    free(card);
}

int aec_cc_card_read_frame(struct aec_cc_card *card, struct audio_frame **frame)
{
    struct audio_frame *aec_frame =
        audio_frame_alloc_with_buffer(card->rate, card->samples, card->channels, card->format);
    int ret = 0;

    pthread_mutex_lock(&card->aec_mutex);
    do {
        ret = dequeue(card->aec_queue, aec_frame->data[0], card->samples);
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

int aec_cc_card_drop_capture(struct aec_cc_card *card)
{
    pthread_mutex_lock(&card->aec_mutex);
    queue_reset(card->aec_queue);
    pthread_mutex_unlock(&card->aec_mutex);
    return 0;
}

int aec_cc_card_avail_capture(struct aec_cc_card *card)
{
    return queue_count(card->aec_queue) / card->samples;
}

void alsa_aec_cc_card_init_default_param(struct alsa_speex_aec_cc_card_param *param, int channels, int rate)
{
    memset(param, 0, sizeof(*param));

    param->alsa_echo_device = "hw:0";
    param->alsa_capture_device = "hw:1";

    param->alsa_params.channels = channels;
    param->alsa_params.rate = rate;
    param->alsa_params.format = SND_PCM_FORMAT_S16_LE;
}
