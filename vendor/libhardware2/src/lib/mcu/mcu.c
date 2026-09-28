#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <sys/ioctl.h>

#define MCU_MAGIC_NUMBER    'M'

#define MCU_SHUTDOWN      _IO(MCU_MAGIC_NUMBER, 112)
#define MCU_RESET         _IO(MCU_MAGIC_NUMBER, 113)
#define MCU_BOOTUP        _IO(MCU_MAGIC_NUMBER, 114)
#define MCU_WRITE_MEM     _IOW(MCU_MAGIC_NUMBER, 115, void *)
// #define MCU_WRITE_DATA    _IOW(MCU_MAGIC_NUMBER, 116, void *)
// #define MCU_READ_DATA     _IOW(MCU_MAGIC_NUMBER, 117, void *)
#define MCU_WRITE_DATA_TIMEOUT    _IOW(MCU_MAGIC_NUMBER, 118, void *)
#define MCU_READ_DATA_TIMEOUT     _IOW(MCU_MAGIC_NUMBER, 119, void *)
#define MCU_READ_STR_TIMEOUT     _IOW(MCU_MAGIC_NUMBER, 120, void *)

static inline void mcu_err(const char *err_msg)
{
    fprintf(stderr, "mcu: failed to %s, %s\n", err_msg, strerror(errno));
}

int mcu_open(void)
{
    const char *dev_path = "/dev/mcu";

    int fd = open(dev_path, O_RDWR);
    if (fd < 0)
        fprintf(stderr, "mcu: open dev(%s) failed. %s\n", dev_path, strerror(errno));

    return fd;
}

int mcu_close(int mcu_fd)
{
    return close(mcu_fd);
}

int mcu_shutdown(int mcu_fd)
{
    int ret = ioctl(mcu_fd, MCU_SHUTDOWN);
    if (ret)
        mcu_err("shutdown");

    return ret;
}

int mcu_reset(int mcu_fd)
{
    int ret = ioctl(mcu_fd, MCU_RESET);
    if (ret)
        mcu_err("reset");

    return ret;
}

int mcu_bootup(int mcu_fd)
{
    int ret = ioctl(mcu_fd, MCU_BOOTUP);
    if (ret)
        mcu_err("bootup");

    return ret;
}

int mcu_write_mem(int mcu_fd, unsigned int offset, void *src, int len)
{
    unsigned long array[3] = {
        offset, (unsigned long) src, len
    };

    int ret = ioctl(mcu_fd, MCU_WRITE_MEM, array);
    if (ret)
        mcu_err("write_mem");

    return ret;
}

int mcu_write_data(int mcu_fd, void *src, int len)
{
    unsigned long array[3] = {
        (unsigned long) src, len, 0
    };

    int ret = ioctl(mcu_fd, MCU_WRITE_DATA_TIMEOUT, array);
    if (ret < 0)
        mcu_err("write_data");

    return ret;
}

int mcu_read_data(int mcu_fd, void *dst, int len)
{
    unsigned long array[3] = {
        (unsigned long) dst, len, 0
    };

    int ret = ioctl(mcu_fd, MCU_READ_DATA_TIMEOUT, array);
    if (ret < 0)
        mcu_err("read_data");

    return ret;
}


int mcu_read_str(int mcu_fd, void *dst, int len)
{
    unsigned long array[3] = {
        (unsigned long) dst, len, 0
    };

    int ret = ioctl(mcu_fd, MCU_READ_STR_TIMEOUT, array);
    if (ret < 0)
        mcu_err("read_str");

    return ret;
}

int mcu_read_str_timeout(int mcu_fd, void *dst, int len, unsigned long us)
{
    unsigned long array[3] = {
        (unsigned long) dst, len, us
    };

    int ret = ioctl(mcu_fd, MCU_READ_STR_TIMEOUT, array);
    if (ret < 0)
        mcu_err("read_str");

    return ret;
}

int mcu_write_data_timeout(int mcu_fd, void *src, int len, unsigned long us)
{
    unsigned long array[3] = {
        (unsigned long) src, len, us
    };

    int ret = ioctl(mcu_fd, MCU_WRITE_DATA_TIMEOUT, array);
    if (ret < 0)
        mcu_err("write_data_timeout");

    return ret;
}

int mcu_read_data_timeout(int mcu_fd, void *dst, int len, unsigned long us)
{
    unsigned long array[3] = {
        (unsigned long) dst, len, us
    };

    int ret = ioctl(mcu_fd, MCU_READ_DATA_TIMEOUT, array);
    if (ret < 0)
        mcu_err("read_data_timeout");

    return ret;
}

int mcu_write_firmware2(int mcu_fd, FILE *file)
{
    char buf[4096];
    int offset = 0;

    while (1) {
        int ret = fread(buf, 1, sizeof(buf), file);

        if (ret == 0)
            break;

        if (ret < 0) {
            if (feof(file))
                break;
            return ret;
        }

        int len = ret;
        ret = mcu_write_mem(mcu_fd, offset, buf, ret);
        if (ret)
            return ret;
        offset += len;
    }

    return 0;
}

int mcu_write_firmware(int mcu_fd, const char *firmware_path)
{
    FILE *file = fopen(firmware_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "mcu: open file(%s) failed. %s\n", firmware_path, strerror(errno));
        return -1;
    }
    int ret = mcu_write_firmware2(mcu_fd, file);
    if (ret != 0)
        fprintf(stderr, "mcu: fail to write firmware2 : %d", ret);

    fclose(file);

    return ret;
}
