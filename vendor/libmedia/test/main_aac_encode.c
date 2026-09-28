#include <libmedia/read/alsa_audio_reader.h>
#include <libmedia/resample/ffmpeg_audio_resampler.h>
#include <libmedia/encode/ffmpeg_audio_encoder.h>
#include <libmedia/media_errno.h>
#include <libutils2/boot_time.h>

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


struct audio_resampler_param resampler_param = {
    .src_channels = 1,
    .src_format = AUDIO_s16le,
    .src_rate = 16000,
    .dst_channels = 1,
    .dst_format = AUDIO_fltp,
    .dst_rate = 16000,
};

struct ffmpeg_audio_encoder_param audio_encoder_param = {
    .encoder_name = "aac",
    .bit_rate = 64000,
    .sample_rate = 16000,
    .channels = 1,
    .sample_fmt = AV_SAMPLE_FMT_FLTP, /*ffmpeg 提供的 aac 编码必须得是 浮点 planer(多通道的音频数据分开存储)格式的*/
};

/*alsa audio reader 读取音频相关参数*/
struct alsa_audio_reader_param alsa_reader_param = {
    .alsa_capture_device = "plughw:0,0",
    .alsa_params.rate = 16000,
    .alsa_params.format = SND_PCM_FORMAT_S16_LE,
    .alsa_params.channels = 1,
    .param.resampler_param = &resampler_param, /*音频重采样*/
    .param.samples = 1024,  /*一帧包含的采样数，根据需求来，ffmpeg aac 编一帧 需要1024个采样*/
};



static struct audio_reader *audio_reader;
static struct audio_encoder *audio_encoder;
static FILE *fp;


static int wrile_file(void *data, int size)
{
    int ret;
    ret = fwrite(data, size, 1, fp);
    if (ret < 0) {
        fprintf(stderr, "failed to write file\n");
        return -1;
    }

    return 0;
}


static int adts_header_deparse(struct adts_header_data *src, char *dst) {

    int sampling_frequency_index;
    int adtsLen = src->size + 7;

    // 匹配采样率
    int frequencies_size = sizeof(sampling_frequencies) / sizeof(sampling_frequencies[0]);
    int i = 0;
    for (i = 0; i < frequencies_size; i++) {
        if (sampling_frequencies[i] == src->sample_rate) {
            sampling_frequency_index = i;
            break;
        }
    }
    if (i >= frequencies_size) {
        fprintf(stderr, "not found audio sample_rate");
        return -1;
    }

    dst[0] = 0xff;                                //前12bit固定0xfff             高8bits
    dst[1] = 0xf0;                                //前12bit的低四位               低4bits
    dst[1] |= (0 << 3);                           //0：MPEG-4, 1：MPEG-2  1bit
    dst[1] |= (0 << 1);                           //一般为0                      2bits
    dst[1] |= src->not_crc;                      //1：没有crc校验字段             1bit

    dst[2] = (src->profile) << 6;                      //aac级别，可以使用ffmpeg获取               2bits
    dst[2] |=
        (sampling_frequency_index & 0x0f) << 2;             //可以使用ffgmpeg从包中获得  4bits
    dst[2] |= (0 << 1);                           //私有位 编码时为0                   1bit
    dst[2] |= (src->channels & 0x04) >> 2;             //3bit的声道设置的最高位  高1bit

    dst[3] = (src->channels & 0x03) << 6;              //3bit的声道设置的最低两位 低2bits
    dst[3] |= (0 << 5);                           //编码设置为0                1bit
    dst[3] |= (0 << 4);                           //编码设置为0                    1bit
    dst[3] |= (0 << 3);                           //编码设置为0        1bit
    dst[3] |= (0 << 2);                           //编码设置为0      1bit
    dst[3] |= ((adtsLen & 0x1800) >> 11);         //帧长度包括ADTS头长度   高2bits

    dst[4] = (uint8_t) ((adtsLen & 0x7f8) >> 3);  //帧长度包括ADTS头长度    中间8bits
    dst[5] = (uint8_t) ((adtsLen & 0x7) << 5);    //帧长度包括ADTS头长度    低3bits
    dst[5] |= 0x1f;                               //可变码率vbr:0x7ff 高5bits
    dst[6] = 0xfc;

	return 0;
}


int main(void)
{
    fp = fopen("/usr/data/test.aac", "w");
    if(!fp) {
        fprintf(stderr, "failed to create test.acc\n");
        return -1;
    }

    ffmpeg_audio_resampler_init_param(&resampler_param);

    alsa_audio_reader_init_param(&alsa_reader_param);
    audio_reader = audio_reader_open(&alsa_reader_param.param);
    if (!audio_reader) {
        fprintf(stderr, "ingenic_media: failed to open audio_reader\n");
        return -1;
    }


    ffmpeg_audio_encoder_init_param(&audio_encoder_param);
    audio_encoder = audio_encoder_open(&audio_encoder_param.param);
    if (!audio_encoder) {
        fprintf(stderr, "ingenic_meida:failed to open audio encoder\n");
        return -1;
    }

    struct adts_header_data adts_header_data = {
        .not_crc = 1,          /*这里暂不支持crc*/
        .profile = 0,

        .channels = audio_encoder_param.channels,
        .sample_rate = audio_encoder_param.sample_rate,
    };

    int ret;
    struct audio_frame *frame = NULL;
    struct media_packet *pkt;

    unsigned char adts_head[7];
    int cnt = 100;
    while(cnt) {
        ret = audio_reader_read_frame(audio_reader, &frame);
        if (ret == -MEDIA_EAGAIN) {
            fprintf(stderr, "continue\n");
            continue;
        }

        audio_encoder_write_frame(audio_encoder, frame);

        while(1) {
            ret = audio_encoder_get_packet(audio_encoder, &pkt);
            if (ret == -MEDIA_EAGAIN)
                break;
            if (ret < 0) {
                fprintf(stderr, "failed to encode audio\n");
                return -1;
            }

            /*在网络传输的应用场景下，根据需要是否添加头部信息。本地保存必需添加头部信息才能播放*/

            /*获取头*/
            adts_header_data.size = pkt->size;
            adts_header_deparse(&adts_header_data, adts_head);
            /*写头*/
            wrile_file(adts_head, sizeof(adts_head));
            /*写音频数据*/
            wrile_file(pkt->data, pkt->size);

            media_packet_put(pkt);
        }

        audio_frame_put(frame);
        cnt--;
    }

    audio_encoder_close(audio_encoder);
    audio_reader_close(audio_reader);

    return 0;

}