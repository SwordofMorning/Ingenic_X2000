#include <libmedia/decode/ffmpeg_audio_decoder.h>
#include <libmedia/play/alsa_audio_player.h>
#include <libmedia/resample/ffmpeg_audio_resampler.h>


struct adts_header_data {
    int size;
    int profile;
    int sample_rate;
    int channels;
    int not_crc;
};

static int sampling_frequencies[] = {
    [0] = 96000,
    [1] = 88200,
    [2] = 64000,
    [3] = 48000,
    [4] = 44100,
    [5] = 32000,
    [6] = 24000,
    [7] = 22050,
    [8] = 16000,
    [9] = 12000,
    [10] = 11025,
    [11] = 8000,
    [12] = 7350,
};


/*音频解码, 可根据云端下发的格式设置，也可通过adts head 设置，这里等取到头部信息后再进行设置*/
struct ffmpeg_audio_decoder_param audio_decode_param = {
    .decoder_name = "aac",
};

/*alsa audio player 播放音频相关参数设置*/
struct alsa_audio_player_param alsa_player_param = {
    .alsa_playback_device = "plughw:0,0",
    .alsa_params.channels = 1,
    .alsa_params.rate = 16000,
    .alsa_params.format = SND_PCM_FORMAT_FLOAT_LE,
};

/*重采样参数，这里等接收到adts 之后再设置*/
struct audio_resampler_param resampler_param = {
    // .src_channels = 1,
    // .src_rate = 16000,
    .src_format = AUDIO_fltp,


    .dst_rate = 16000,
    .dst_format = AUDIO_flt,
    .dst_channels = 1,
};

static struct audio_decoder *audio_decoder;
static struct audio_player *audio_player;
static struct audio_resampler *audio_resampler;

static FILE *fp;

static void audio_pkt_free(void *handle, struct media_packet *pkt)
{
    free(pkt->data);
    media_packet_free(pkt);
}



static int parse_adts_head(unsigned char *src, struct adts_header_data *dst)
{
    unsigned char *adts_head = src;
    if (adts_head[0] != 0xff || ((adts_head[1] & 0xf0) != 0xf0)) {
        fprintf(stderr, "aac decode: not found aac adts head\n");
        return -1;
    }

    int low_3bit = (adts_head[5] & 0xe0) >> 5;
    int mid_8bit = adts_head[4];
    int high_2bit = adts_head[3] & 0x03;

    int sample_rate_index = (adts_head[2] & 0x3c) >> 2;

    dst->size = low_3bit | (mid_8bit << 3) | high_2bit << 11;
    dst->profile = (adts_head[2] & 0xc0) >> 6;
    dst->channels = ((adts_head[2] & 0x01) << 2) | ((adts_head[3] & 0xc0) >> 6);
    dst->sample_rate = sampling_frequencies[sample_rate_index];
    dst->not_crc =  (adts_head[1] & 0x01);

    // printf("size = %d\n", dst_data->size);
    // printf("profile = %d\n", dst_data->profile);
    // printf("channels = %d\n", dst_data->channels);
    // printf("sample_rate = %d\n", dst_data->sample_rate);
    // printf("not_crc = %d\n", dst_data->not_crc);


    return 0;
}

static int read_audio_pkt_from_file(struct media_packet **pkt, struct adts_header_data *header_data)
{
    unsigned char adts_head[7];
    int ret;

    /*读取头部信息主要获取大小*/
    ret = fread(adts_head, 1, sizeof(adts_head), fp);
    if (ret < 7) {
        return -1;
    }

    ret = parse_adts_head(adts_head, header_data);
    if (ret < 0)
        return ret;

    /*读取音频数据数据*/
    char *data = malloc(header_data->size);
    memset(data, 0, header_data->size);
    ret = fread(data, 1, header_data->size - 7, fp);
    if (ret < 0) {
        return -1;
    }

    /*打包成media_packet, 给解码器用*/
    /*网络接收到aac 数据包，可直接打包，填充相关信息就行。packet 是否有adts 头不影响解码*/
    struct media_packet *tmp_pkt = media_packet_alloc();
    tmp_pkt->data = data;
    tmp_pkt->handle = NULL;
    tmp_pkt->put_packet = audio_pkt_free;
    tmp_pkt->size = header_data->size - 7;
    tmp_pkt->type = AUDIO_pkt_aac;

    *pkt = tmp_pkt;

    media_packet_get(tmp_pkt);

    return 0;
}


static int init_audio_decoder_resampler(int channels, int sample_rate)
{
    audio_decode_param.channels = channels;
    audio_decode_param.sample_rate= sample_rate;

    ffmpeg_audio_decoder_init_param(&audio_decode_param);
    audio_decoder = audio_decoder_open(&audio_decode_param.param);
    if (!audio_decoder) {
        fprintf(stderr, "aac_decode:failed to open audio decoder\n");
        return -1;
    }

    resampler_param.src_rate = sample_rate;
    resampler_param.src_channels = channels;

    ffmpeg_audio_resampler_init_param(&resampler_param);
    audio_resampler = audio_resampler_create(&resampler_param);
    if (!audio_resampler) {
        fprintf(stderr, "aac_decode: failed to create resampler\n");
        return -1;
    }

    return 0;
}

int main(void)
{

    fp = fopen("/usr/data/test.aac", "r");
    if (!fp) {
        fprintf(stderr, "aac_decode:failed to open test.acc\n");
        return -1;
    }

    alsa_audio_player_init_param(&alsa_player_param);
    audio_player = audio_player_open(&alsa_player_param.param);
    if (!audio_player) {
        fprintf(stderr, "aac_decode:failed to open audio player\n");
        return -1;
    }

    struct adts_header_data header_data;

    struct media_packet *pkt = NULL;
    struct audio_frame *frame = NULL;
    struct audio_frame *display_frame = NULL;
    int ret;

    while(1) {

        ret = read_audio_pkt_from_file(&pkt, &header_data);
        if (ret < 0) {
            break;
        }

        if (!audio_decoder) {
            ret = init_audio_decoder_resampler(header_data.channels, header_data.sample_rate);
            if (ret < 0)
                break;
        }

        if (header_data.channels != audio_decoder->param.channels || \
            header_data.sample_rate != audio_decoder->param.rate) {
            fprintf(stderr, "aac_decode:audio param is changed \n");
            break;
        }

        ret = audio_decoder_send_pkt(audio_decoder, pkt);
        if (ret != 0) {
            fprintf(stderr, "aac_decode:failed to send pkt\n");
            break;
        }

        while(1) {
            ret = audio_decoder_get_frame(audio_decoder, &frame);
            if (ret != 0)
                break;

            audio_resampler_convert(audio_resampler, frame, &display_frame);

            audio_player_display_audio(audio_player, display_frame);

            /*释放内存*/
            audio_frame_put(frame);
            audio_frame_put(display_frame);
        }

        /*释放内存*/
        media_packet_put(pkt);
    }


    if (audio_decoder)
        audio_decoder_close(audio_decoder);

    if (audio_resampler)
        audio_resampler_delete(audio_resampler);

    audio_player_close(audio_player);

    return 0;
}