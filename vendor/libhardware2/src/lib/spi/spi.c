#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <libhardware2/spi.h>

#define SPI_ADD_DEVICE                          _IOWR('s', 200, struct spidev_register_data *)
#define SPI_DEL_DEVICE                          _IOWR('s', 201, char *)


static inline void spi_err(const char *err_msg)
{
    fprintf(stderr, "SPI: failed to %s, %s\n", err_msg, strerror(errno));
}

int spi_open(char *spidev_path)
{
    int fd = open(spidev_path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "SPI: open dev(%s) failed. %s\n", spidev_path, strerror(errno));
        return fd;
    }

    return fd;
}

void spi_close(int spi_fd)
{
    close(spi_fd);
}

int spi_transfer(int spi_fd, struct spi_ioc_transfer *spi_tr, int num)
{
    int ret;

    ret = ioctl(spi_fd, SPI_IOC_MESSAGE(num), spi_tr);
    if (ret < 0) {
        spi_err("transfer");
        return ret;
    }

    return 0;
}

int spi_read(int spi_fd, char *rx_buf, int len)
{
    int ret;
    struct spi_ioc_transfer spi_msg = {
        .len = len,
        .rx_buf = (unsigned long )rx_buf,
        .tx_buf = 0,
        .speed_hz = 0,
        .bits_per_word = 0,
    };

    ret = spi_transfer(spi_fd, &spi_msg, 1);

    if (ret < 0)
        return -1;

    return 0;
}

int spi_write(int spi_fd, char *tx_buf, int len)
{
    int ret;
    struct spi_ioc_transfer spi_msg = {
        .len = len,
        .rx_buf = 0,
        .tx_buf = (unsigned long )tx_buf,
        .speed_hz = 0,
        .bits_per_word = 0,
    };

    ret = spi_transfer(spi_fd, &spi_msg, 1);

    if (ret < 0)
        return -1;

    return 0;
}

int spi_set_mode(int spi_fd, int mode)
{
    int ret;
    ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    if (ret < 0) {
        spi_err("set mode");
        return ret;
    }

    return 0;
}

int spi_set_speed(int spi_fd, int speed)
{
    int ret;
    ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if(ret < 0) {
        spi_err("set speed");
        return ret;
    }

    return 0;
}

int spi_set_bits(int spi_fd, int bits_per_word)
{
    int ret;
    ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
    if(ret < 0) {
        spi_err("set bits");
        return ret;
    }

    return 0;
}

int spi_set_lsb(int spi_fd, int lsb_first)
{
    int ret;
    ret = ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &lsb_first);
    if (ret < 0) {
        spi_err("set lsb");
        return ret;
    }

    return 0;
}

void spi_get_info(int spi_fd, struct spi_info *spi)
{
    int ret;

    assert(spi);

    ret = ioctl(spi_fd, SPI_IOC_RD_MODE, &spi->spi_mode);
    if (ret < 0) {
        spi->spi_mode = -1;
        spi_err("get mode");
    }

    ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi->spi_speed);
    if (ret < 0) {
        spi->spi_speed = -1;
        spi_err("get speed");
    }

    ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &spi->spi_bits);
    if (ret < 0) {
        spi->spi_bits = -1;
        spi_err("get bits");
    }

    ret = ioctl(spi_fd, SPI_IOC_RD_LSB_FIRST, &spi->spi_lsb);
    if (ret < 0) {
        spi->spi_lsb = -1;
        spi_err("get lsb");
    }
}

int spi_add_device(struct spidev_register_data *data)
{
    int ret;

    int fd = open("/dev/spidev_helper", O_RDWR);
    if (fd < 0) {
        spi_err("open spidev_helper");
        return fd;
    }

    ret = ioctl(fd, SPI_ADD_DEVICE, data);
    if (ret < 0)
        spi_err("add spidev");

    close(fd);
    return ret;
}

int spi_del_device(char *spidev_path)
{
    int ret = 0;

    int fd = open("/dev/spidev_helper", O_RDWR);
    if (fd < 0) {
        spi_err("open spidev_helper");
        return fd;
    }

    ret = ioctl(fd, SPI_DEL_DEVICE, spidev_path);
    if (ret < 0)
        spi_err("del spidev");

    close(fd);
    return ret;
}