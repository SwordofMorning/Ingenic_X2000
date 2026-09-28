#ifndef _AUDIO_H_
#define _AUDIO_H_

struct audio_dma_desc {
    unsigned long cmd;
    unsigned long dma_addr;
    unsigned long next_desc;
    unsigned long trans_count;
};

void audio_connect_dev(void);

void audio_disconnect_dev(void);

void audio_dma_desc_init(
    struct audio_dma_desc *desc,
    void *buf, int buf_size, int unit_size,
    struct audio_dma_desc *next);

void audio_dma_config(int channels, int data_bits, int unit_size, snd_pcm_format_t format);

void audio_dma_start(struct audio_dma_desc *desc);

void audio_dma_stop(void);

unsigned int audio_dma_get_current_addr(void);

#endif /* _AUDIO_H_ */
