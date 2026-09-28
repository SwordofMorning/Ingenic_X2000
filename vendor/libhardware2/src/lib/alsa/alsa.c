
#define _GNU_SOURCE
#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <libhardware2/alsa.h>

struct alsa_pcm {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    struct alsa_params p;
    const char *device;
    void *period_buffer;
    int period_left_frames;
    snd_pcm_stream_t stream;
};

static inline void alsa_error(const char *path, const char *tag, int err)
{
    fprintf(stderr, "alsa: [%s] %s failed, %s(%d)\n",
        path, tag, snd_strerror(err), err);
}

static struct alsa_pcm *alsa_open_device(const char *device, snd_pcm_stream_t stream)
{
    int ret;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;

    ret = snd_pcm_open(&handle, device, stream, 0);
    if (ret) {
        alsa_error(device, "open", ret);
        return NULL;
    }

    ret = snd_pcm_hw_params_malloc(&params);
    if (ret < 0) {
        alsa_error(device, "malloc params", ret);
        goto close_pcm;
    }

    ret = snd_pcm_hw_params_any(handle, params);
    if (ret < 0) {
        alsa_error(device, "params any", ret);
        goto free_pcm_params;
    }

    struct alsa_pcm *alsa = malloc(sizeof(*alsa));
    if (!alsa) {
        alsa_error(device, "malloc alsa", -ENOMEM);
        goto free_pcm_params;
    }
    memset(alsa, 0, sizeof(*alsa));

    alsa->device = device;
    alsa->handle = handle;
    alsa->params = params;
    alsa->stream = stream;

    return alsa;

free_pcm_params:
    snd_pcm_hw_params_free(params);
close_pcm:
    snd_pcm_close(handle);
    return NULL;
}

struct alsa_pcm *alsa_pcm_open_capture_device(const char *device)
{
    return alsa_open_device(device, SND_PCM_STREAM_CAPTURE);
}

struct alsa_pcm *alsa_pcm_open_playback_device(const char *device)
{
    return alsa_open_device(device, SND_PCM_STREAM_PLAYBACK);
}

int alsa_pcm_set_params(struct alsa_pcm *alsa, struct alsa_params *p)
{
    int ret;
    snd_pcm_t *handle = alsa->handle;
    snd_pcm_hw_params_t *params = alsa->params;
    const char *device = alsa->device;

    if (alsa->period_buffer) {
        fprintf(stderr, "alsa: error: pcm params not free!\n");
        return -EINVAL;
    }

    ret = snd_pcm_hw_params_any(handle, params);
    if (ret < 0) {
        alsa_error(device, "params any", ret);
        return ret;
    }

    ret = snd_pcm_hw_params_set_access(
                handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        alsa_error(device, "set access", ret);
        return ret;
    }

    ret = snd_pcm_hw_params_set_format(handle, params, p->format);
    if (ret < 0) {
        alsa_error(device, "set format", ret);
        fprintf(stderr, "value is: %d\n", (int)p->format);
        return ret;
    }

    ret = snd_pcm_hw_params_set_channels(handle, params, p->channels);
    if (ret < 0) {
        alsa_error(device, "set channels", ret);
        fprintf(stderr, "value is: %d\n", p->channels);
        return ret;
    }

    unsigned int mrate = p->rate;
    ret = snd_pcm_hw_params_set_rate_near(handle, params, &mrate, 0);
    if (ret < 0) {
        alsa_error(device, "set rate", ret);
        fprintf(stderr, "value is: %d\n", mrate);
        return ret;
    }
    if (p->rate != mrate) {
        fprintf(stderr, "rate is not support: ask:%d get:%d\n", p->rate, mrate);
        return ret;
    }

    if (p->buffer_time == 0) {
        ret = snd_pcm_hw_params_get_buffer_time_max(params, &p->buffer_time, 0);
        if (ret < 0) {
            alsa_error(device, "get buffer time max", ret);
            return ret;
        }

        if (p->buffer_time > 500*1000)
            p->buffer_time = 500*1000;
    }

    ret = snd_pcm_hw_params_set_buffer_time_near(handle, params, &p->buffer_time, 0);
    if (ret < 0) {
        alsa_error(device, "set buffer time", ret);
        fprintf(stderr, "value is: %d\n", p->buffer_time);
        return ret;
    }

    if (p->period_time == 0)
        p->period_time = p->buffer_time / 8;

    if (p->period_time > p->buffer_time / 2)
        p->period_time = p->buffer_time / 2;

    ret = snd_pcm_hw_params_set_period_time_near(handle, params, &p->period_time, 0);
    if (ret < 0) {
        alsa_error(device, "set period time", ret);
        fprintf(stderr, "value is: %d\n", p->buffer_time);
        return ret;
    }

    ret = snd_pcm_hw_params(handle, params);
    if (ret < 0) {
        alsa_error(device, "hw params", ret);
        return ret;
    }

    snd_pcm_uframes_t period_size, buffer_size;
    ret = snd_pcm_hw_params_get_period_size(params, &period_size, 0);
    if (ret < 0) {
        alsa_error(device, "get period size", ret);
        snd_pcm_hw_free(handle);
        return ret;
    }

    ret = snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (ret < 0) {
        fprintf(stderr, "%s failed: %s %d\n", "snd_pcm_hw_params_get_buffer_size",
        snd_strerror(ret), ret);
        snd_pcm_hw_free(handle);
        return ret;
    }

    if (period_size == buffer_size) {
        fprintf(stderr, "error: period size is equal to buffer size: %d\n",
        (int) period_size);
        snd_pcm_hw_free(handle);
        return ret;
    }

    p->frame_bytes = p->channels * snd_pcm_format_physical_width(p->format) / 8;
    p->period_frames = period_size;
    p->buffer_frames = buffer_size;

    unsigned int period_bytes = period_size * p->frame_bytes;
    alsa->period_buffer = malloc(period_bytes);
    if (!alsa->period_buffer) {
        fprintf(stderr, "error: malloc period buffer size: %d\n", (int) period_bytes);
        snd_pcm_hw_free(handle);
        return -ENOMEM;
    }

    alsa->period_left_frames = 0;
    alsa->p = *p;

    return 0;
}

int alsa_pcm_start(struct alsa_pcm *alsa)
{
    return snd_pcm_start(alsa->handle);
}

int alsa_pcm_avil(struct alsa_pcm *alsa)
{
    int ret = snd_pcm_avail(alsa->handle);
    if (alsa->stream == SND_PCM_STREAM_CAPTURE)
        ret += alsa->period_left_frames;
    return ret;
}

int alsa_pcm_drop(struct alsa_pcm *alsa)
{
    int ret = 0;
    if (alsa->stream  == SND_PCM_STREAM_CAPTURE) {
        int samples = 10*alsa->p.rate/1000;
        uint8_t data[alsa->p.frame_bytes * samples];
        int avail = snd_pcm_avail(alsa->handle);

        alsa->period_left_frames = 0;
        while (avail > samples) {
            ret = alsa_pcm_read(alsa, data, samples);
            if (ret < 0)
                break;

            avail -= samples;
        }
    } else {
        ret = snd_pcm_drop(alsa->handle);
        if (ret)
            alsa_error(alsa->device, "drop", ret);
    }

    alsa->period_left_frames = 0;

    return ret;
}

int alsa_pcm_free_params(struct alsa_pcm *alsa)
{
    if (!alsa->period_buffer) {
        fprintf(stderr, "alsa: error: pcm params not set!\n");
        return -EINVAL;
    }

    free(alsa->period_buffer);
    alsa->period_buffer = NULL;

    int ret = snd_pcm_drop(alsa->handle);
    if (ret)
        alsa_error(alsa->device, "drop", ret);

    ret = snd_pcm_hw_free(alsa->handle);
    if (ret)
        alsa_error(alsa->device, "hw free", ret);

    return ret;
}

void alsa_pcm_close(struct alsa_pcm *alsa)
{
    if (alsa->period_buffer)
        alsa_pcm_free_params(alsa);
    snd_pcm_hw_params_free(alsa->params);
    snd_pcm_close(alsa->handle);
    free(alsa);
}

static int read_from_pcm(snd_pcm_t *handle, void *buffer, int frames, int frame_bytes)
{
    int err = 0;
    int offset = 0;
    int remain = frames;

    while (remain > 0) {
        err = snd_pcm_readi(handle,
                buffer + offset, remain);

        if (err == -EAGAIN || (err >= 0 && err < remain)) {
            snd_pcm_wait(handle, 500);
        } else if (err == -EPIPE) {
            snd_pcm_prepare(handle);
            fprintf(stderr, "Overrun: %s(%d)\n",
                    snd_strerror(err), err);
        } else if (err == -ESTRPIPE) {
            while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                usleep(10*1000);  /* wait until resume flag is released */
            if (err < 0)
                snd_pcm_prepare(handle);
        } else if (err < 0) {
            fprintf(stderr, "Read PCM device error: %s(%d)\n",
                    snd_strerror(err), err);
            return err;
        }

        if (err > 0) {
            remain -= err;
            offset += err * frame_bytes;
        }
    }

    return 0;
}

int alsa_pcm_read(struct alsa_pcm *alsa, void *buffer, int frames)
{
    int ret;
    int frame_bytes = alsa->p.frame_bytes;
    int period_frames = alsa->p.period_frames;

    if (!alsa->period_buffer) {
        fprintf(stderr, "alsa: error: pcm params not set!\n");
        return -EINVAL;
    }

    while (frames) {
        if (alsa->period_left_frames) {
            int n = alsa->period_left_frames;
            if (n > frames)
                n = frames;
            int off = (period_frames - alsa->period_left_frames) * frame_bytes;
            memcpy(buffer, alsa->period_buffer + off, n * frame_bytes);
            alsa->period_left_frames -= n;
            frames -= n;
            buffer += n * frame_bytes;
            continue;
        }

        if (frames >= period_frames) {
            ret = read_from_pcm(alsa->handle, buffer, period_frames, frame_bytes);
            if (ret)
                return ret;

            frames -= period_frames;
            buffer += period_frames * frame_bytes;
        } else {
            ret = read_from_pcm(alsa->handle, alsa->period_buffer, period_frames, frame_bytes);
            if (ret)
                return ret;

            alsa->period_left_frames = period_frames;
        }
    }

    return 0;
}

static int write_to_pcm(snd_pcm_t *handle, void *buffer, int frames, int frame_bytes)
{
    int err;
    int offset = 0;
    int remain = frames;

    while (remain > 0) {
        err = snd_pcm_writei(handle, buffer + offset,
                remain);
        if (err == -EAGAIN || (err >= 0 && err < frames)) {
            snd_pcm_wait(handle, 500);
        } else if (err == -EPIPE) {
            fprintf(stderr, "Underrun: %s(%d)\n",
                    snd_strerror(err), err);
            snd_pcm_prepare(handle);
        } else if (err == -ESTRPIPE) {
            while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                sleep(10*1000);  /* wait until resume flag is released */
            if (err < 0)
                snd_pcm_prepare(handle);
        } else if (err < 0) {
            fprintf(stderr, "Write PCM device error: %s(%d)\n",
                    snd_strerror(err), err);
            return err;
        }

        if (err > 0) {
            remain -= err;
            offset += err * frame_bytes;
        }
    }

    return 0;
}

static int do_write_copy(struct alsa_pcm *alsa, void *buffer, int frames)
{
    int n = alsa->p.period_frames - alsa->period_left_frames;
    if (n > frames)
        n = frames;
    int off = alsa->period_left_frames * alsa->p.frame_bytes;
    memcpy(alsa->period_buffer + off, buffer, n * alsa->p.frame_bytes);
    alsa->period_left_frames += n;
    return n;
}

int alsa_pcm_write(struct alsa_pcm *alsa, void *buffer, int frames)
{
    int ret;
    int frame_bytes = alsa->p.frame_bytes;
    int period_frames = alsa->p.period_frames;

    if (!alsa->period_buffer) {
        fprintf(stderr, "alsa: error: pcm params not set!\n");
        return -EINVAL;
    }

    if (alsa->period_left_frames) {
        int n = do_write_copy(alsa, buffer, frames);
        if (alsa->period_left_frames == period_frames) {
            alsa->period_left_frames = 0;
            ret = write_to_pcm(alsa->handle, alsa->period_buffer, period_frames, frame_bytes);
            if (ret)
                return ret;
        }

        frames -= n;
        buffer += n * frame_bytes;
    }

    while (frames >= period_frames) {
        ret = write_to_pcm(alsa->handle, buffer, period_frames, frame_bytes);
        if (ret)
            return ret;

        frames -= period_frames;
        buffer += period_frames * frame_bytes;
    }

    if (frames)
        do_write_copy(alsa, buffer, frames);

    return 0;
}

int alsa_pcm_drain(struct alsa_pcm *alsa)
{
    int ret;

    if (alsa->period_left_frames) {
        write_to_pcm(alsa->handle, alsa->period_buffer,
            alsa->period_left_frames, alsa->p.frame_bytes);
        alsa->period_left_frames = 0;
    }

    ret = snd_pcm_drain(alsa->handle);
    if (ret)
        alsa_error(alsa->device, "drain", ret);

    return ret;
}

struct alsa_ctl {
    snd_hctl_t *handle;
    snd_hctl_elem_t *elem;
    snd_ctl_elem_info_t *info;
};

struct alsa_ctl *alsa_ctl_open(const char *card_name, const char *ctl_name)
{
    int ret;
    snd_hctl_t *handle;
    snd_hctl_elem_t *elem;
    snd_ctl_elem_info_t *info;

    ret = snd_hctl_open(&handle, card_name, 0);
    if (ret < 0) {
        alsa_error(card_name, "hctl open", ret);
        return NULL;
    }

    ret = snd_hctl_load(handle);
    if (ret < 0) {
        alsa_error(card_name, "hctl load", ret);
        goto close_hctl;
    }

    snd_hctl_elem_t *m_elem = NULL;
    for (elem = snd_hctl_first_elem(handle); elem; elem = snd_hctl_elem_next(elem)) {
        if (!strcmp(snd_hctl_elem_get_name(elem), ctl_name)) {
            m_elem = elem;
            break;
        }
    }

    if (!m_elem) {
        for (elem = snd_hctl_first_elem(handle); elem; elem = snd_hctl_elem_next(elem)) {
            if (strcasestr(snd_hctl_elem_get_name(elem), ctl_name)) {
                m_elem = elem;
                break;
            }
        }
    }

    if (!m_elem) {
        fprintf(stderr, "alsa: %s can't match ctl:%s\n", card_name, ctl_name);
        goto close_hctl;
    }

    ret = snd_ctl_elem_info_malloc(&info);
    if (ret < 0) {
        alsa_error(card_name, "ctl elem info malloc", ret);
        goto free_hctl;
    }

    ret = snd_hctl_elem_info(elem, info);
    if (ret < 0) {
        alsa_error(card_name, "hctl elem info", ret);
        goto free_info;
    }

    struct alsa_ctl *ctl = malloc(sizeof(*ctl));
    if (!ctl) {
        alsa_error(card_name, "malloc alsa_ctl", -ENOMEM);
        goto free_info;
    }
    memset(ctl, 0, sizeof(*ctl));

    ctl->handle = handle;
    ctl->elem = m_elem;
    ctl->info = info;

    return ctl;
free_info:
    snd_ctl_elem_info_free(info);
free_hctl:
    snd_hctl_free(handle);
close_hctl:
    snd_hctl_close(handle);
    return NULL;
}

const char *alsa_ctl_get_name(struct alsa_ctl *ctl)
{
    return snd_hctl_elem_get_name(ctl->elem);
}

int alsa_ctl_set_value(struct alsa_ctl *ctl, long value)
{
    snd_ctl_elem_value_t *evalue;
    snd_ctl_elem_value_alloca(&evalue);

    snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(ctl->info);
    if (type != SND_CTL_ELEM_TYPE_INTEGER) {
        fprintf(stderr, "%s type is not integer: %d\n",
            alsa_ctl_get_name(ctl), (int) type);
        return -EINVAL;
    }

    snd_ctl_elem_value_set_integer(evalue, 0, value);
    int ret = snd_hctl_elem_write(ctl->elem, evalue);
    if (ret)
        alsa_error(alsa_ctl_get_name(ctl), "elem write", ret);

    return ret;
}

int alsa_ctl_get_value(struct alsa_ctl *ctl, long *value)
{
    snd_ctl_elem_value_t *evalue;
    snd_ctl_elem_value_alloca(&evalue);

    snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(ctl->info);
    if (type != SND_CTL_ELEM_TYPE_INTEGER) {
        fprintf(stderr, "%s type is not integer: %d\n",
            alsa_ctl_get_name(ctl), (int) type);
        return -EINVAL;
    }

    int ret = snd_hctl_elem_read(ctl->elem, evalue);
    if (ret)
        alsa_error(alsa_ctl_get_name(ctl), "elem write", ret);

    *value = snd_ctl_elem_value_get_integer(evalue, 0);

    return ret;
}

int alsa_ctl_get_value_range(struct alsa_ctl *ctl, long *min, long *max)
{
    snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(ctl->info);
    if (type != SND_CTL_ELEM_TYPE_INTEGER) {
        fprintf(stderr, "%s type is not integer: %d\n",
            alsa_ctl_get_name(ctl), (int) type);
        return -EINVAL;
    }

    if (min)
        *min = snd_ctl_elem_info_get_min(ctl->info);

    if (max)
        *max = snd_ctl_elem_info_get_max(ctl->info);

    return 0;
}

void alsa_ctl_close(struct alsa_ctl *ctl)
{
    snd_ctl_elem_info_free(ctl->info);
    snd_hctl_free(ctl->handle);
    snd_hctl_close(ctl->handle);
    free(ctl);
}

int alsa_ctl_list_all(const char *card_name)
{
    int ret;
    snd_hctl_t *handle;
    snd_hctl_elem_t *elem;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *value;

    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&value);

    ret = snd_hctl_open(&handle, card_name, 0);
    if (ret < 0) {
        alsa_error(card_name, "hctl open", ret);
        return ret;
    }

    ret = snd_hctl_load(handle);
    if (ret < 0) {
        alsa_error(card_name, "hctl load", ret);
        snd_hctl_close(handle);
        return ret;
    }

    for (elem = snd_hctl_first_elem(handle); elem; elem = snd_hctl_elem_next(elem)) {
        ret = snd_hctl_elem_info(elem, info);
        if (ret < 0) {
            alsa_error(card_name, "hctl load", ret);
            snd_hctl_free(handle);
            snd_hctl_close(handle);
            return ret;
        }

        snd_hctl_elem_read(elem, value);

        snd_ctl_elem_type_t type = snd_ctl_elem_info_get_type(info);
        if (type != SND_CTL_ELEM_TYPE_INTEGER)
            printf("other[%d]: \"%s\"\n", (int)type, snd_hctl_elem_get_name(elem));
        else
            printf("integer: \"%s\" [%ld, %ld] %ld\n", snd_hctl_elem_get_name(elem),
                    snd_ctl_elem_info_get_min(info),
                    snd_ctl_elem_info_get_max(info),
                    snd_ctl_elem_value_get_integer(value, 0));
    }

    snd_hctl_free(handle);
    snd_hctl_close(handle);

    return 0;
}
