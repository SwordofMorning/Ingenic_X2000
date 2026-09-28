#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <linux/types.h>
#include <libhardware2/mcu.h>
#include <libhardware2/rmem.h>

enum nemc_operation {
    READ_DATA,
    WRITE_DATA,
};

enum data_mode {
    ADDR_MODE,
    DATA_MODE,
};

struct data_header {
    enum nemc_operation operation;
    enum data_mode mode;
    unsigned long src_addr;
    unsigned long dst_addr;
    unsigned long data_size;
};

/* 以下地址范围适用于 x2600，其他 soc 请查看手册修改*/
#define NEMC0_ADDR_START 0x1b000000
#define NEMC0_ADDR_END 0x1b00ffff
#define NEMC1_ADDR_START 0x1a000000
#define NEMC1_ADDR_END 0x1a00ffff

struct nemc_resource {
    int rmem_fd;
    int mcu_fd;
    unsigned char *data_buf_maddr;
    unsigned long data_buf_paddr;
    unsigned long nemc_addr;
};

static void usage(const char *command, int status)
{
    printf("Usage1:%s read mode=<> nemc_addr=<> size=<>\n", command);
    printf("  Example:\n");
    printf("    %s read mode=2 nemc_addr=0x1b000000 size=80\n", command);
    printf("Usage2:%s write mode=<> nemc_addr=<>  size=<>\n", command);
    printf("  Example:\n");
    printf("    %s write mode=2 nemc_addr=0x1b000000 size=80\n", command);
    printf("  arg explain\n");
    printf("    mode=1: Provide the memory address to the mcu\n");
    printf("    mode=2: Provide data in memory to mcu\n");
    printf("    nemc_addr: The address of nemc. Different soc addresses may be different\n");
    printf("               The example address is x2600\n");
    printf("    size : Data size per transfer, condition: size %% data_width = 0\n\n");
    exit(status);
}

static int parse_uint(const char *str, const char *prefix, unsigned long *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    unsigned long v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        exit(-1);
    }

    *value = v;
    return 1;
}

static void wait_mcu_is_ready(int mcu_fd)
{
    char read_buf[256] = {0};
    unsigned int times = 0xff;
    int ret;
    while (times != 0) {
        ret = mcu_read_str_timeout(mcu_fd, read_buf, sizeof(read_buf), -1);
        if (ret <= 0) {
            times--;
            continue;
        }
        if (strcmp(read_buf, "mcu_nemc_ready") == 0)
            return;
    }
    if (times == 0) {
        printf("wait mcu ready timeout");
        exit(-1);
    }
}

static void wait_mcu_transfer_finish(int mcu_fd)
{
    char read_buf[256] = {0};
    unsigned int times = 0xff;
    int ret;
    while (times != 0) {
        ret = mcu_read_str_timeout(mcu_fd, read_buf, sizeof(read_buf), -1);
        if (ret <= 0) {
            times--;
            continue;
        }
        if (strcmp(read_buf, "mcu_nemc_finish") == 0)
            return;
    }
    if (times == 0) {
        printf("wait mcu transfer timeout");
        exit(-1);
    }
}

static void read_data_from_mcu(struct nemc_resource nemc_res, struct data_header header)
{
    /* 初始化 buf */
    memset(nemc_res.data_buf_maddr, 0x55, header.data_size);

    /* 刷 cache，确保内存的数据是正确的 */
    if (header.mode == ADDR_MODE)
        rmem_cache_sync(nemc_res.rmem_fd, nemc_res.data_buf_maddr, header.data_size, rmem_cache_mem_to_dev);

    header.src_addr = nemc_res.nemc_addr;
    header.dst_addr = nemc_res.data_buf_paddr;

    /* 等待 mcu 准备好接收数据 */
    wait_mcu_is_ready(nemc_res.mcu_fd);
    mcu_write_data(nemc_res.mcu_fd, &header, sizeof(header));

    /* 等待 mcu 搬运完成 */
    wait_mcu_transfer_finish(nemc_res.mcu_fd);

    /* 读取 mcu 读取到数据*/
    if (header.mode == ADDR_MODE)
        rmem_cache_sync(nemc_res.rmem_fd, nemc_res.data_buf_maddr, header.data_size, rmem_cache_dev_to_mem);
    else if(header.mode == DATA_MODE)
        mcu_read_data(nemc_res.mcu_fd, nemc_res.data_buf_maddr, header.data_size);

    /* 打印读取到的数据 */
    printf("host read data:");
    for (int i=0; i<header.data_size; i++) {
        if (i % 10 == 0)
            printf("\n");
        printf("%02x ", nemc_res.data_buf_maddr[i]);
    }
    printf("\n");

}

static void  write_data_to_mcu(struct nemc_resource nemc_res, struct data_header header)
{
    /* 数据源赋值 */
    for (int i=0; i<header.data_size; i++)
        nemc_res.data_buf_maddr[i] = i;
    /* 刷 cache，确保内存的数据是正确的 */
    if (header.mode == ADDR_MODE)
        rmem_cache_sync(nemc_res.rmem_fd, nemc_res.data_buf_maddr, header.data_size, rmem_cache_mem_to_dev);

    header.src_addr = nemc_res.data_buf_paddr;
    header.dst_addr = nemc_res.nemc_addr;

    /* 等待 mcu 准备好接收数据 */
    wait_mcu_is_ready(nemc_res.mcu_fd);
    mcu_write_data(nemc_res.mcu_fd, &header, sizeof(header));

    /* 将数据发送给 mcu */
    if (header.mode == DATA_MODE) {
        mcu_write_data(nemc_res.mcu_fd, nemc_res.data_buf_maddr, header.data_size);
    }

    /* 等待 mcu 搬运完成 */
    wait_mcu_transfer_finish(nemc_res.mcu_fd);
}


int main(int argc, char *argv[])
{
    char *command = argv[0];
    struct nemc_resource nemc_res;
    struct data_header header;
    if (argc < 5)
        usage(command, -1);

    if (!strcmp(argv[1], "read"))
        header.operation = READ_DATA;
    else
        header.operation = WRITE_DATA;

    if (!strcmp(argv[2], "mode=1"))
        header.mode = ADDR_MODE;
    else header.mode = DATA_MODE;

    if (parse_uint(argv[3], "nemc_addr=", &nemc_res.nemc_addr, 16) != 1)
        usage(command, -1);

    if (parse_uint(argv[4], "size=", &header.data_size, 10) != 1)
        usage(command, -1);

    if (nemc_res.nemc_addr > NEMC0_ADDR_END || nemc_res.nemc_addr < NEMC0_ADDR_START)
        if (nemc_res.nemc_addr > NEMC1_ADDR_END || nemc_res.nemc_addr < NEMC1_ADDR_START) {
            fprintf(stderr, "nemc addr error\n");
            return -1;
        }

    nemc_res.rmem_fd = rmem_open();
    if (nemc_res.rmem_fd < 0) {
        fprintf(stderr, "failed to open rmem_manager\n");
        return -1;
    }

    nemc_res.data_buf_maddr = rmem_alloc(nemc_res.rmem_fd, &nemc_res.data_buf_paddr, header.data_size);
    if (nemc_res.data_buf_maddr == NULL) {
        fprintf(stderr, "nemc : alloc rmem space fail\n");
        goto free_rmem_fd;
    }

    nemc_res.mcu_fd = mcu_open();
    if (nemc_res.mcu_fd < 0) {
        fprintf(stderr, "failed to open mcu\n");
        goto free_rmem_alloc;
    }

    if (header.operation == READ_DATA) {
        read_data_from_mcu(nemc_res, header);
    } else {
        write_data_to_mcu(nemc_res, header);
    }

    mcu_close(nemc_res.mcu_fd);
free_rmem_alloc:
    rmem_free(nemc_res.rmem_fd, nemc_res.data_buf_maddr, nemc_res.data_buf_paddr, header.data_size);
free_rmem_fd:
    rmem_close(nemc_res.rmem_fd);

    return 0;
}