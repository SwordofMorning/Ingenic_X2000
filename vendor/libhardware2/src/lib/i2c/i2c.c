/*
 *  Copyright (C) 2020 Ingenic Semiconductor Co., Ltd.
 *
 *  Ingenic library hardware version2
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "i2c-dev.h"

#define PATH_MAX_LENGTH                 (32)

int i2c_open(uint8_t bus_num)
{
    char i2c_dev_path[PATH_MAX_LENGTH] = { 0 };
    int fd;

    sprintf(i2c_dev_path, "/dev/i2c-%d", bus_num);
    fd = open(i2c_dev_path, O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "I2C: open dev(%s) failed. %s\n", i2c_dev_path, strerror(errno));
        return fd;
    }

    return fd;
}

void i2c_close(int i2c_fd)
{
    close (i2c_fd);
}

int i2c_detect(int i2c_fd, uint16_t device_addr)
{
    int ret = -EIO;
    unsigned long funcs;

    if (ioctl(i2c_fd, I2C_FUNCS, &funcs) < 0) {
        fprintf(stderr, "i2c: Could not get the adapter "
                "functionality matrix: %s\n", strerror(errno));
        return -errno;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, device_addr) < 0) {
        if (errno == EBUSY)
            return 1;

        fprintf(stderr, "i2c: Could not set the i2c slave addr[%02x] %s\n",
             (int) device_addr, strerror(errno));

        return -errno;
    }

    if (funcs & I2C_FUNC_SMBUS_QUICK) {
        ret = i2c_smbus_write_quick(i2c_fd, I2C_SMBUS_WRITE);
        if (ret >= 0)
            return 0;
    }

    if (funcs & I2C_FUNC_SMBUS_READ_BYTE) {
        ret = i2c_smbus_read_byte(i2c_fd);
        if (ret >= 0)
            return 0;
    }

    return ret;
}

int i2c_read(int i2c_fd, uint16_t device_addr, void *buffer, int size)
{
    int ret = 0;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[] = {
        {
            .addr   = device_addr,
            .flags  = I2C_M_RD,
            .len    = size,
            .buf    = buffer,
        },
    };

    data.msgs = msgs;
    data.nmsgs = 1;

    ret = ioctl(i2c_fd, I2C_RDWR, &data);
    if (ret < 0)
        fprintf(stderr, "i2c: failed to read device[%02x] %s\n",
             (int)device_addr, strerror(errno));

    return ret > 0 ? 0 : ret;
}

int i2c_write(int i2c_fd, uint16_t device_addr, void *buffer, int size)
{
    int ret = 0;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[] = {
        {
            .addr   = device_addr,
            .flags  = 0,
            .len    = size,
            .buf    = buffer,
        },
    };

    data.msgs = msgs;
    data.nmsgs = 1;

    ret = ioctl(i2c_fd, I2C_RDWR, &data);
    if (ret < 0)
        fprintf(stderr, "i2c: failed to write device[%02x] %s\n",
             (int)device_addr, strerror(errno));

    return ret > 0 ? 0 : ret;
}

int i2c_read_reg(int i2c_fd, uint16_t device_addr, uint8_t reg_addr,void *buffer, int size)
{
    int ret = 0;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[] = {
        {
            .addr   = device_addr,
            .flags  = 0,
            .len    = sizeof(reg_addr),
            .buf    = &reg_addr,
        },

        {
            .addr   = device_addr,
            .flags  = I2C_M_RD,
            .len    = size,
            .buf    = buffer,
        },
    };

    data.msgs = msgs;
    data.nmsgs = 2;

    ret = ioctl(i2c_fd, I2C_RDWR, &data);
    if (ret < 0)
        fprintf(stderr, "i2c: failed to read reg device[%02x] %s\n",
             (int)device_addr, strerror(errno));

    return ret > 0 ? 0 : ret;
}

int i2c_write_reg(int i2c_fd, uint16_t device_addr, uint8_t reg_addr,void *buffer, int size)
{
    int ret = 0;
    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg msgs[] = {
        {
            .addr   = device_addr,
            .flags  = 0,
            .len    = sizeof(reg_addr),
            .buf    = &reg_addr,
        },

        {
            .addr   = device_addr,
            .flags  = I2C_M_NOSTART,
            .len    = size,
            .buf    = buffer,
        },
    };

    data.msgs = msgs;
    data.nmsgs = 2;

    ret = ioctl(i2c_fd, I2C_RDWR, &data);
    if (ret < 0)
        fprintf(stderr, "i2c: failed to write reg device[%02x] %s\n",
             (int)device_addr, strerror(errno));

    return ret > 0 ? 0 : ret;
}


int i2c_read_reg_16(int i2c_fd, uint16_t device_addr, uint16_t reg_addr,void *buffer, int size)
{
    int ret = 0;
    struct i2c_rdwr_ioctl_data data;

    uint8_t reg_buf[2] = {reg_addr >> 8, reg_addr & 0xff};

    struct i2c_msg msgs[] = {
        {
            .addr   = device_addr,
            .flags  = 0,
            .len    = sizeof(reg_addr),
            .buf    = reg_buf,
        },

        {
            .addr   = device_addr,
            .flags  = I2C_M_RD,
            .len    = size,
            .buf    = buffer,
        },
    };

    data.msgs = msgs;
    data.nmsgs = 2;

    ret = ioctl(i2c_fd, I2C_RDWR, &data);
    if (ret < 0)
        fprintf(stderr, "i2c: failed to read reg16 device[%02x] %s\n",
             (int)device_addr, strerror(errno));

    return ret > 0 ? 0 : ret;
}

int i2c_write_reg_16(int i2c_fd, uint16_t device_addr, uint16_t reg_addr,void *buffer, int size)
{
    int ret = 0;
    struct i2c_rdwr_ioctl_data data;

    uint8_t reg_buf[2] = {reg_addr >> 8, reg_addr & 0xff};

    struct i2c_msg msgs[] = {
        {
            .addr   = device_addr,
            .flags  = 0,
            .len    = sizeof(reg_addr),
            .buf    = reg_buf,
        },

        {
            .addr   = device_addr,
            .flags  = I2C_M_NOSTART,
            .len    = size,
            .buf    = buffer,
        },
    };

    data.msgs = msgs;
    data.nmsgs = 2;

    ret = ioctl(i2c_fd, I2C_RDWR, &data);
    if (ret < 0)
        fprintf(stderr, "i2c: failed to write reg16 device[%02x] %s\n",
             (int)device_addr, strerror(errno));

    return ret > 0 ? 0 : ret;
}

