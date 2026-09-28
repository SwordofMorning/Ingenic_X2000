#ifndef _ROT_H_
#define _ROT_H_

#include <soc/base.h>
#include <bit_field2.h>

#define BUFF_CFG_ADDR   0x002c
#define FRM_SIZE        0x0004
#define GLB_CFG         0x0008
#define ROT_CTRL        0x000c
#define STATUS          0x0010
#define CLR_STATUS      0x0014
#define INT_MASK        0x0018
#define ROT_RDMA_SITE   0x0020
#define ROT_BUFF_CNT    0x0040

/* BUFF_CFG_ADDR */
#define BUFF_ADDR       1, 31
#define START           0, 0

/* FRM_SIZE */
#define FRAM_HEIGHT     16, 26
#define FRAM_WIDTH      0,  10

/* GLB_CFG */
#define ROT_EN          30, 30
#define RDMA_EN         29, 29
#define RDMA_FMT        24, 27
#define FIFO_GATE       20, 20
#define MANUAL_AUTO     19, 19
#define ROT_ANGLE       4,  5
#define RDMA_BURST_LEN  0,  1

/* ROT_CTRL */
#define FLUSH_FIFO      4, 4
#define GEN_STOP        2, 2
#define QUICK_STOP      1, 1

/* STATUS */
#define FA_MASK_ST      20, 20
#define SOF_MASK_ST     19, 19
#define EOF_MASK_ST     18, 18
#define GSA_MASK_ST     17, 17
#define ST_FA           4, 4
#define ST_SOF          3, 3
#define ST_EOF          2, 2
#define ST_GEN_STOP_ACK 1, 1
#define WORKING         0, 0

/* CLR_STATUS */
#define CLR_FA          4, 4
#define CLR_SOF         3, 3
#define CLR_EOF         2, 2
#define CLR_GEN_STP_ACK 1, 1

/* INT_MASK */
#define FA_MASK         4, 4
#define SOF_MASK        3, 3
#define EOF_MASK        2, 2
#define GSA_MASK        1, 1

/* ROT_BUFF_CNT */
#define BUFF_CNT        0, 14

/***************************************/

struct rot_cfg {
    int angle;

    int is_enable;
    void *frame_mem;
    int frame_count;
    int bytes_per_frame;
    int mem_size;
};

/***************************************/

#define ROT_ADDR(reg)   ((volatile unsigned long *)(ROTATE_IOBASE + reg))

static inline void rot_write_reg(unsigned int reg, unsigned int value)
{
    *ROT_ADDR(reg) = value;
}

static inline unsigned int rot_read_reg(unsigned int reg)
{
    return *ROT_ADDR(reg);
}

static inline void rot_set_bits(unsigned int reg, int start, int end, unsigned int val)
{
    set_bit_field_v(ROT_ADDR(reg), start, end, val);
}

static inline unsigned int rot_get_bits(unsigned int reg, int start, int end)
{
    return get_bit_field_v(ROT_ADDR(reg), start, end);
}

static inline void dump_rot_reg(void)
{
    printf("BUFF_CFG_ADDR--0x%08x\n", rot_read_reg(BUFF_CFG_ADDR));
    printf("FRM_SIZE-------0x%08x\n", rot_read_reg(FRM_SIZE));
    printf("GLB_CFG--------0x%08x\n", rot_read_reg(GLB_CFG));
    printf("ROT_CTRL-------0x%08x\n", rot_read_reg(ROT_CTRL));
    printf("STATUS---------0x%08x\n", rot_read_reg(STATUS));
    printf("CLR_STATUS-----0x%08x\n", rot_read_reg(CLR_STATUS));
    printf("INT_MASK-------0x%08x\n", rot_read_reg(INT_MASK));
    printf("ROT_RDMA_SITE--0x%08x\n", rot_read_reg(ROT_RDMA_SITE));
    printf("ROT_BUFF_CNT---0x%08x\n", rot_read_reg(ROT_BUFF_CNT));
}

#endif
