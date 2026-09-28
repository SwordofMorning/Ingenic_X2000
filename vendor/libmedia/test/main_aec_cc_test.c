#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libmedia/filter/alsa_speex_aec_cc_card.h>

int main(int argc, char *argv[])
{
    struct alsa_speex_aec_cc_card_param param = {
        .alsa_echo_device = "hw:1",
        .alsa_capture_device = "hw:0",
        // .alsa_echo_ctl_device = "hw:1",
        // .alsa_capture_ctl_device = "hw:0",
        // .alsa_echo_ctl_name = "Mic Volume",
        // .alsa_capture_ctl_name = "Master Capture Volume",

        .alsa_params.channels = 1,
        .alsa_params.rate = 48000,
        .alsa_params.format = SND_PCM_FORMAT_S16_LE,

        // .speex_params.echo_volume = 246,
        // .speex_params.capture_volume = 14,
        // .speex_params.capture_align_ms = 70,
    };
    struct aec_cc_card *card = aec_cc_card_open(&param);
    assert(card);

    FILE *dst = fopen("/tmp/aec.pcm", "w");

    int run = 1500;

    struct audio_frame *dst_frame;
    while(run --) {
        aec_cc_card_read_frame(card, &dst_frame);
        fwrite(dst_frame->data[0], dst_frame->total_size, 1, dst);
        audio_frame_put(dst_frame);
    }

    aec_cc_card_close(card);

    fclose(dst);

    return 0;
}