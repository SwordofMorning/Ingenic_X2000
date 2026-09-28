#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libhardware2/adc.h>

#define ADC_MAGIC_NUMBER                'A'
#define ADC_ENABLE                      _IO(ADC_MAGIC_NUMBER, 11)
#define ADC_DISABLE                     _IO(ADC_MAGIC_NUMBER, 22)
#define ADC_SET_VREF                    _IOW(ADC_MAGIC_NUMBER, 33, unsigned int)
#define ADC_GET_VREF                    _IOWR(ADC_MAGIC_NUMBER, 44, unsigned int)
#define ADC_GET_VALUE                   _IOWR(ADC_MAGIC_NUMBER, 60, unsigned int)
#define ADC_SET_CHANNEL                 _IOW(ADC_MAGIC_NUMBER, 61, unsigned int)
#define ADC_SEQ0_ENABLE                 _IOW(ADC_MAGIC_NUMBER, 70, struct adc_seq0_config)
#define ADC_SEQ0_DISABLE                _IOW(ADC_MAGIC_NUMBER, 71, struct adc_seq0_config)
#define ADC_SEQ0_READ_TIMEOUT           _IOWR(ADC_MAGIC_NUMBER, 72, void *)
#define ADC_SEQ1_ENABLE                 _IOW(ADC_MAGIC_NUMBER, 73, struct adc_seq1_config)
#define ADC_SEQ1_DISABLE                _IOW(ADC_MAGIC_NUMBER, 74, struct adc_seq1_config)
#define ADC_SEQ1_READ_TIMEOUT           _IOWR(ADC_MAGIC_NUMBER, 75, void *)
#define ADC_SEQ2_ENABLE                 _IOW(ADC_MAGIC_NUMBER, 76, struct adc_seq2_config)
#define ADC_SEQ2_DISABLE                _IOW(ADC_MAGIC_NUMBER, 77, struct adc_seq2_config)
#define ADC_SEQ2_READ_TIMEOUT           _IOWR(ADC_MAGIC_NUMBER, 78, void *)
#define ADC_AWD_ENABLE                  _IOW(ADC_MAGIC_NUMBER, 79, void *)
#define ADC_AWD_DISABLE                 _IO(ADC_MAGIC_NUMBER, 80)
#define ADC_AWD_READ_TIMEOUT            _IOWR(ADC_MAGIC_NUMBER, 81, void *)

long adc_enable(unsigned int channle_id)
{
    int handle, ret;
    char devname[20];
    int is_common_node = 0;

    if (!access("/dev/jz_adc_seq", F_OK)) {
        sprintf(devname, "/dev/jz_adc_seq");
        is_common_node = 1;
    } else {
        sprintf(devname, "/dev/jz_adc_aux_%d", channle_id);
        is_common_node = 0;
    }

    handle = open(devname, O_RDWR);
    if (handle < 0) {
        fprintf(stderr, "ADC:adc open dev failed: %s\n", strerror(errno));
        return -1;
    }

    if (is_common_node) {
        ret = ioctl(handle, ADC_SET_CHANNEL, &channle_id);
        if (ret < 0) {
            fprintf(stderr, "ADC:adc set channel failed: %s\n", strerror(errno));
            close(handle);
            return -1;
        }
    }

    ret = ioctl(handle, ADC_ENABLE);
    if (ret < 0) {
        fprintf(stderr, "ADC:adc enable failed: %s\n", strerror(errno));
        close(handle);
        return -1;
    }

    return handle;
}

void adc_disable(long handle)
{
    int ret;

    ret = ioctl(handle, ADC_DISABLE);
    if (ret < 0) {
        fprintf(stderr, "ADC:adc disable failed: %s\n", strerror(errno));
        return;
    }

    if (handle < 0) {
        fprintf(stderr, "ADC:adc close fd failed: %s\n", strerror(errno));
        return;
    }

    close(handle);
}

int adc_set_vref(long handle, unsigned int vref)
{
    int ret = ioctl(handle, ADC_SET_VREF, &vref);
    if (ret < 0)
        fprintf(stderr, "ADC:adc set vref failed: %s\n", strerror(errno));

    return ret;
}

int adc_get_vref(long handle)
{
    int vref = ioctl(handle, ADC_GET_VREF);
    if (vref < 0)
        fprintf(stderr, "ADC:adc get vref failed: %s\n", strerror(errno));

    return vref;
}

int adc_get_value(long handle)
{
    int val = ioctl(handle, ADC_GET_VALUE);
    if (val < 0)
        fprintf(stderr, "ADC:adc get value failed: %s\n", strerror(errno));

    return val;
}

int adc_get_voltage(long handle)
{
    int val;

    int ret = read(handle, &val, sizeof(val));
    if (ret < 0)
        fprintf(stderr, "ADC:adc get voltage failed: %s\n", strerror(errno));

    return val;
}

int adc_open(void)
{
    int handle, ret;

    handle = open("/dev/jz_adc_seq", O_RDWR);
    if (handle < 0) {
        fprintf(stderr, "ADC:adc open dev failed: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(handle, ADC_ENABLE);
    if (ret < 0) {
        fprintf(stderr, "ADC:adc enable failed: %s\n", strerror(errno));
        close(handle);
        return -1;
    }

    return handle;
}

int adc_close(long handle)
{
    int ret;

    ret = ioctl(handle, ADC_DISABLE);
    if (ret < 0)
        fprintf(stderr, "ADC:adc disable failed: %s\n", strerror(errno));

    close(handle);

    return ret;
}

int adc_seq1_enable(long handle, struct adc_seq1_config adc_seq1)
{
    int ret = ioctl(handle, ADC_SEQ1_ENABLE, &adc_seq1);
    if (ret < 0)
        fprintf(stderr, "ADC:adc seq1 enable failed: %s\n", strerror(errno));

    return ret;
}

int adc_seq1_disable(long handle, struct adc_seq1_config adc_seq1)
{
    int ret = ioctl(handle, ADC_SEQ1_DISABLE, &adc_seq1);
    if (ret < 0)
        fprintf(stderr, "ADC:adc seq1 disable failed: %s\n", strerror(errno));

    return ret;
}

int adc_seq1_read_timeout(long handle, int timeout, unsigned short *data)
{
    unsigned long arg[2];
    arg[0] = timeout;
    arg[1] = (unsigned long)data;

    int val = ioctl(handle, ADC_SEQ1_READ_TIMEOUT, arg);
    if (val < 0)
        fprintf(stderr, "ADC:adc seq1 disable failed: %s\n", strerror(errno));

    return val;
}

int adc_awd_enable(long handle, unsigned int channel, int low_threshold, int high_threshold)
{
    int ret;
    int awd_threshold[2];
    awd_threshold[0] = low_threshold;
    awd_threshold[1] = high_threshold;

    ret = ioctl(handle, ADC_SET_CHANNEL, &channel);
    if (ret < 0) {
        fprintf(stderr, "ADC:adc awd set channel failed: %s\n", strerror(errno));
        return ret;
    }

    ret = ioctl(handle, ADC_AWD_ENABLE, awd_threshold);
    if (ret < 0)
        fprintf(stderr, "ADC:adc awd enable failed: %s\n", strerror(errno));

    return ret;
}

int adc_awd_disable(long handle)
{
    int ret = ioctl(handle, ADC_AWD_DISABLE);
    if (ret < 0)
        fprintf(stderr, "ADC:adc awd disable failed: %s\n", strerror(errno));

    return ret;
}

int adc_awd_read_timeout(long handle, int timeout, unsigned short *awd_low_flags, unsigned short *awd_high_flags)
{
    unsigned short awd_flags[2];
    unsigned long arg[2];
    arg[0] = timeout;
    arg[1] = (unsigned long)awd_flags;

    int val = ioctl(handle, ADC_AWD_READ_TIMEOUT, arg);
    if (val < 0)
        fprintf(stderr, "ADC:adc awd disable failed: %s\n", strerror(errno));

    *awd_low_flags = awd_flags[0];
    *awd_high_flags = awd_flags[1];

    return val;
}