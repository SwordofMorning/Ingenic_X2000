#ifndef __TPC_DATA_H__
#define __TPC_DATA_H__

#include <stdlib.h>
#include <stdint.h>
#include <common.h>

#define MAX_MOTO_IO 8
#define MAX_REPEAT  64


/*heat fifo 深度 为 132 字节*/
/*motor fifo 深度 为 260 字节*/
/*shfit fifo 深度 为 1284 字节*/


typedef void (*tpc_irq_callback)(void *data);

struct tpc_shift_cfg {
    uint8_t idle_level;
    uint8_t big_end;   /*大小端*/
    /**
     * 极性 spi_pol :
     * 当spi_pol=0，在时钟空闲即无数据传输时,clk电平为低电平
     * 当spi_pol=1，在时钟空闲即无数据传输时,clk电平为高电平
     * 相位 spi_pha :
     * 当spi_pha=0，表示在第一个跳变沿开始传输数据，下一个跳变沿完成传输
     * 当spi_pha=1，表示在第二个跳变沿开始传输数据，下一个跳变沿完成传输
     */
    uint8_t spi_pol;
    uint8_t spi_pha;

    uint8_t channels;       /*数据输出通道数1 ~ 8*/
    uint8_t data_invert;    /*移位数据取反*/
    uint8_t line_bytes;    /*数据输出总长度 （通道数大于4时每个通道单次输出最大是128（字节），小于4通道单次输出256(字节))*/

    uint64_t out_clk_rate;
    tpc_irq_callback irq_callback;
};

/*加热的dma数据中，若下一次锁存是由上一次加热触发的话，代表当前写入的所有dma数据是一个整体传完了才上报打印头完成中断*/
struct tpc_heat_cfg {
    int channels;                          /*热信号通道1 ~ 8*/
    int idle_level;                        /*热信号无效电平*/
    uint64_t wait_clkcnt_out;                   /*触发热信号输出后，等待多少个时钟才开始输出*/
    uint64_t max_heat_clkcnt;                   /*最大加热时间(clk cnt)*/
    tpc_irq_callback irq_callback;
};

struct tpc_lat_cfg {
    int idle_level;                         /*锁存的无效电平*/
    int active_clkcnt;                      /*锁存有效持续时间*/
    uint64_t wait_clkcnt_out;                    /*触发输出锁存信号之后，等待多少个时钟才开始输出(clk cnt)*/
    uint64_t wait_clkcnt_out_shift;              /*锁存输出完后，等待等待多少个时钟触发移位数据, 仅sync mode 有效  (clk cnt)*/
    tpc_irq_callback irq_callback;
};

/*电机控制器支持1个 8路电机或者 两个4路的电机*/
struct tpc_motor_cfg {
    int channels;                                       /*id0 通道数为 1 ～ 8，当id0 大于4通道时， id1不可用*/

    uint8_t idle_level;                                 /*电机io idle 时的电平 例如：0b0101 代表4个channels， 从1通道开始依次是 1 0 1 0*/
    uint8_t start_step_io;                              /*电机io开始步时的电平*/
    uint8_t stop_step_io;                               /*电机io结束步时的电平*/

    uint64_t start_step_clkcnt;                              /*开始步步长（clk cnt）*/
    uint64_t stop_step_clkcnt;                               /*结束步步长（clk cnt）*/

    int repeat_step;                                    /*电机循环步数,最大64步*/
    unsigned long long repeat_step_io[MAX_MOTO_IO];

    int align_step;                                     /*每多少步对应一行。与打印头自动对齐时生效*/
    uint64_t max_step_clkcnt;                                /*电机步的最大步长(clk cnt)*/
    tpc_irq_callback irq_callback;
};


static inline unsigned int timeus_to_clk_cnt(unsigned int time_us, uint64_t clk_rate)
{
    return (clk_rate / (1*1000*1000)) * time_us;
}


#endif