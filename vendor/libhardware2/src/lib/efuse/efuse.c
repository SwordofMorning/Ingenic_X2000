#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libhardware2/efuse.h>

#define SEGMENT_NAME_MAX_LEN 128

#define CMD_READ                                    _IOWR('l', 200, struct efuse_wr_info_str *)
#define CMD_WRITE                                   _IOWR('l', 201, struct efuse_wr_info_str *)
#define CMD_READ_SEG_SIZE                           _IOWR('l', 202, char *)
#define CMD_SEGMENT_INFORMATION                     _IOWR('l', 203, struct efuse_wr_info_str *)
#define CMD_SEGMENT_NUM                             _IO('l', 204)


struct efuse_wr_info_str {
    char *seg_id_name;
    uint32_t size;
    uint32_t start;
    uint8_t *buf;
};

int efuse_read_seg_size(char *name)
{
    int handle;
    int ret;
    int seg_size;

    handle = open("/dev/efuse-string-version", O_RDWR);

    if (handle < 0) {
        fprintf(stderr, "EFUSE:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    seg_size = ioctl(handle, CMD_READ_SEG_SIZE, name);
    if (seg_size < 0) {
        fprintf(stderr, "EFUSE:read seg size failed: %s\n", strerror(errno));
        ret = -1;
        goto out;
    }

    ret = seg_size;

out:
    close(handle);
    return ret;
}

int efuse_read(char *name, unsigned char *buf, int start, int size)
{
    struct efuse_wr_info_str info = {
        .seg_id_name = name,
        .size = size,
        .start = start,
        .buf = buf
    };

    int handle;
    int ret;

    handle = open("/dev/efuse-string-version", O_RDWR);
    if (handle < 0) {
        fprintf(stderr, "EFUSE:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(handle, CMD_READ, &info);
    if (ret < 0) {
        fprintf(stderr, "EFUSE:read failed: %s\n", strerror(errno));
        ret = -1;
        goto out;
    }

    ret = 0;

out:
    close(handle);
    return ret;
}

int efuse_write(char *name, unsigned char *wr_buf, int start, int size)
{
    struct efuse_wr_info_str info = {
        .seg_id_name = name,
        .size = size,
        .start = start,
        .buf = wr_buf
    };

    int ret;
    int handle;

    handle = open("/dev/efuse-string-version", O_RDWR);
    if (handle < 0) {
        fprintf(stderr, "EFUSE:open dev failed: %s\n", strerror(errno));
        return -1;
    }

    ret = ioctl(handle, CMD_WRITE, &info);
    if (ret < 0) {
        fprintf(stderr, "EFUSE:write failed: %s\n", strerror(errno));
        ret = -1;
        goto out;
    }

    ret = 0;

out:
    close(handle);
    return ret;
}

struct efuse_segment_info *efuse_get_segment_information(void)
{
    int ret, cnt;
    int handle;
    int i;
    struct efuse_segment_info *info = NULL;

    handle = open("/dev/efuse-string-version", O_RDWR);
    if (handle < 0) {
        fprintf(stderr, "EFUSE:open dev failed: %s\n", strerror(errno));
        return NULL;
    }

    cnt = ioctl(handle, CMD_SEGMENT_NUM);
    if (cnt < 0) {
        fprintf(stderr, "EFUSE:obtain segment num failed: %s\n", strerror(errno));
        goto out;
    }

    info = malloc((cnt + 1) * sizeof(struct efuse_segment_info));

    for(i = 0; i < cnt; i++) {
        char name[SEGMENT_NAME_MAX_LEN];
        struct efuse_segment_info buf = {
            .seg_start = i,
            .seg_size = SEGMENT_NAME_MAX_LEN,
            .segment_name = name
        };

        ret = ioctl(handle, CMD_SEGMENT_INFORMATION, &buf);
        if (ret < 0) {
            fprintf(stderr, "EFUSE:get segment information failed: %s\n", strerror(errno));
            info[i].segment_name = NULL;
            goto out;
        }

        info[i].segment_name = strdup(name);
        info[i].seg_start = buf.seg_start;
        info[i].seg_size = buf.seg_size;
    }

    info[i].segment_name = NULL;

    close(handle);
    return info;
out:
    close(handle);
    if(info) {
        i = 0;
        while(info[i].segment_name != NULL) {
            free(info[i].segment_name);
            i++;
        }

        free(info);
    }
    return NULL;
}

void efuse_free_information(struct efuse_segment_info *info)
{
    int i = 0;;

    while(info[i].segment_name != NULL) {
        free(info[i].segment_name);
        i++;
    }

    free(info);
    return;
}
