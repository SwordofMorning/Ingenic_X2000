#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <assert.h>
#include <delay.h>
#include <driver/clk.h>
#include <driver/dma.h>
#include <soc/base.h>
#include <driver/irq.h>
#include "soc/tpc_data.h"
#include "tpc_hal.h"
#include "tpc_regs.h"
#include "soc/tpc.h"
#include <driver/systick.h>


#define MAX_DMA_BUFFER_CNT       32

#define check_data_len(dev, len) \
    do { \
        if (len % dev->unit_size) { \
            panic("len = %d must align unit_size %d\n", len, dev->unit_size); \
        } \
    } while (0)

struct tpc_module_device {
    void *pdata;

    struct tpc_dma* dma;
    volatile int dev_is_start;
    volatile int is_init;

    unsigned int start_reg;
    volatile int is_err;
    int unit_size;

    tpc_irq_callback irq_callback;
};


static struct tpc_module_device shift_dev;
static struct tpc_module_device heat_dev;
static struct tpc_module_device lat_dev;

static struct tpc_module_device motor_dev[2];


struct {
    volatile int is_err;
    volatile int is_start;
    int motor_id;
    unsigned int start_reg;
} print_head;


static void device_callback(struct tpc_module_device *dev, int err)
{
    if (dev->irq_callback && dev->dev_is_start) {
        dev->irq_callback((void *)err);
    }

    dev->dev_is_start = 0;
}

static void device_is_err(struct tpc_module_device *dev, int err)
{
    dev->is_err = err;
    tpc_hal_dma_stop(dev->dma, 1);
    tpc_set_bit(dev->start_reg, QSTP, 1);

    device_callback(dev, err);
}


static void tpc_err(struct tpc_module_device *dev, int is_err)
{
    if (!print_head.is_start)
        device_is_err(dev, is_err);

    device_is_err(&shift_dev, is_err);
    device_is_err(&heat_dev, is_err);

    int motor_id = print_head.motor_id;

    if (motor_id >= 0)
        device_is_err(&motor_dev[motor_id], is_err);

    print_head.is_start = 0;
}

static void tpc_irq_handle(int irq, void *data)
{
    unsigned long flags = tpc_read_reg(PINTS);

    /*移位寄存器完成中断*/
    if (get_bit_field_v(&flags, PSFINT)) {
        tpc_clear_interrupt(PSFINT, 1);
        device_callback(&shift_dev, 0);
        return;
    }

    /*加热数据完成中断*/
    if (get_bit_field_v(&flags, PDDFINT)) {
        tpc_clear_interrupt(PDDFINT, 1);
        device_callback(&heat_dev, 0);
        return;
    }

    /*锁存完成中断*/
    if (get_bit_field_v(&flags,  PLFINT)) {
        tpc_clear_interrupt(PLFINT, 1);
        device_callback(&lat_dev, 0);
        return;
    }


    /*电机0每步中断。默认只开motor_cfg align_step的 中断*/
    if (get_bit_field_v(&flags, SM0RSINT)) {
        tpc_clear_interrupt(SM0RSINT, 1);
        if (motor_dev[0].irq_callback)
            motor_dev[0].irq_callback(NULL);

        return;
    }

    /*电机1每步中断。默认只开motor_cfg align_step的 中断*/
    if (get_bit_field_v(&flags, SM1RSINT)) {
        tpc_clear_interrupt(SM1RSINT, 1);
        if (motor_dev[1].irq_callback)
            motor_dev[1].irq_callback(NULL);

        return;
    }


    /*电机0 完成中断*/
    if (get_bit_field_v(&flags,  SM0FINT)) {
        tpc_clear_interrupt(SM0FINT, 1);
        motor_dev[0].dev_is_start = 0;
        return;
    }

    /*电机0 开始步完成中断*/
    if (get_bit_field_v(&flags, SM0SSINT)) {
        tpc_clear_interrupt(SM0SSINT, 1);
        return;
    }


    /*电机1 完成中断*/
    if (get_bit_field_v(&flags, SM1FINT)) {
        tpc_clear_interrupt(SM1FINT, 1);
        motor_dev[1].dev_is_start = 0;
        return;
    }

    /*电机1 开始步完成中断*/
    if (get_bit_field_v(&flags, SM1SSINT)) {
        tpc_clear_interrupt(SM1SSINT, 1);
        return;
    }


    /*打印头完成中断*/
    if (get_bit_field_v(&flags, PDFINT)) {
        tpc_clear_interrupt(PDFINT, 1);
        if (print_head.motor_id < 0)
            print_head.is_start = 0;
        return;
    }


    /*对齐模式完成中断*/
    if (get_bit_field_v(&flags, PFFINT)) {
        tpc_clear_interrupt(PFFINT, 1);
        print_head.is_start = 0;
        return;
    }

/*------------------------------异常---------------------------------*/

    /*电机0 underrun*/
    if (get_bit_field_v(&flags, SM0UINT)) {
        printf("motor 0: underrun\n");
        tpc_clear_interrupt(SM0UINT, 1);
        tpc_err(&motor_dev[0], MOTOR_DATA_UNDER);
        return;
    }

    /*电机0 步长异常，超过设置的最大步长*/
    if (get_bit_field_v(&flags,  SM0OINT)) {
        printf("motor 0: step time is too large\n");
        tpc_clear_interrupt(SM0OINT, 1);
        tpc_err(&motor_dev[0], MOTOR_MAX_TIME_ERR);
        return;
    }

    /*电机1 underrun*/
    if (get_bit_field_v(&flags,  SM1UINT)) {
        printf("motor 1: underrun\n");
        tpc_set_bit(PINTS, SM1UINT, 1);
        tpc_err(&motor_dev[1], MOTOR_DATA_UNDER);
        return;
    }

    /*电机1 步长异常，超过设置的最大步长*/
    if (get_bit_field_v(&flags, SM1OINT)) {
        printf("motor 1: step time is too large\n");
        tpc_clear_interrupt(SM1OINT, 1);
        tpc_err(&motor_dev[1], MOTOR_MAX_TIME_ERR);
        return;
    }


      /*移位数据underun*/
    if (get_bit_field_v(&flags, PSFUINT)) {
        printf("shift underrun interrput write clear\n");
        tpc_clear_interrupt(PSFUINT, 1);
        tpc_err(&shift_dev, SHIFT_DATA_UNDER);
        return;
    }

    /*加热数据underrun*/
    if (get_bit_field_v(&flags, PDFUINT)) {
        printf("heat data underrun interrput\n");
        tpc_clear_interrupt(PDFUINT, 1);
        tpc_err(&heat_dev, HEAT_DATA_UNDER);
        return;
    }

    /*加热时间超时中断，取决于加热最大值寄存器*/
    if (get_bit_field_v(&flags, PDOINT)) {
        printf("heat time to large interrput write clear\n");
        tpc_clear_interrupt(PDOINT, 1);
        tpc_err(&heat_dev, HEAT_MAX_TIME_ERR);
        return;
    }


    /*对齐模式异常，电机走完了，打印流程还没走完， 可能是电机时间太短或者电机步数与打印数据不匹配*/
    if (get_bit_field_v(&flags, PFAINT)) {
        printf("auto err\n");
        tpc_clear_interrupt(PFAINT, 1);
        tpc_err(NULL, PRINT_HEAD_ERR);
        return;
    }

    return;
}

static int calculate_shift_unit_size(struct tpc_shift_cfg *config)
{
    return config->line_bytes * sizeof(char);
}

static int calculate_heat_unit_size(struct tpc_heat_cfg *config)
{
    return (config->channels + 1) * sizeof(int);
}

static int calculate_motor_unit_size(struct tpc_motor_cfg *config)
{
    return config->repeat_step * sizeof(int);
}


static void device_init(struct tpc_module_device *dev, int dma_loop)
{
    dev->is_init = 1;
    dev->dev_is_start = 0;
    dev->is_err = 0;

    int dma_size = dev->unit_size;
    if (dma_loop)
        dma_size = dev->unit_size * MAX_DMA_BUFFER_CNT;

    if (dev->dma)
        tpc_hal_dma_init(dev->dma, dma_loop, dma_size, dev->unit_size);
}

static int device_write_data(struct tpc_module_device *dev, void *data, int len)
{
    if (!dev->is_init) {
        printf("device is not init\n");
        return -1;
    }

    check_data_len(dev, len);

    if (!len)
        return 0;

    return tpc_hal_dma_write_data(dev->dma, data, len);
}

static int shift_device_init(struct tpc_shift_cfg *config, int dma_loop, tpc_irq_callback shift_irq_cb)
{
    int ret = 0;
    struct tpc_module_device *dev = &shift_dev;

    if (dev->is_init) {
        printf("tpc: shift is init\n");
        return -1;
    }

    dev->pdata = config;
    tpc_hal_shift_config(config);
    dev->unit_size = calculate_shift_unit_size(config);
    dev->irq_callback = shift_irq_cb;

    tpc_shift_gpio_init(config->channels);

    device_init(dev,dma_loop);

    return ret;
}

static int heat_device_init(struct tpc_heat_cfg *config, int dma_loop, tpc_irq_callback heat_irq_cb)
{
    struct tpc_module_device *dev = &heat_dev;

    if (dev->is_init) {
        printf("tpc: heat is init\n");
        return -1;
    }

    dev->pdata = config;
    tpc_hal_heat_config(config);

    dev->unit_size = calculate_heat_unit_size(config);
    dev->irq_callback = heat_irq_cb;

    tpc_heat_gpio_init(config->channels);

    device_init(dev, dma_loop);

    return 0;
}

static int lat_device_init(struct tpc_lat_cfg *config, tpc_irq_callback lat_irq_cb)
{
    struct tpc_module_device *dev = &lat_dev;

    if (dev->is_init) {
        printf("tpc: lat is init\n");
        return -1;
    }

    dev->pdata = config;
    tpc_hal_lat_config(config);

    dev->irq_callback = lat_irq_cb;
    dev->unit_size = 0;

    tpc_lat_gpio_init();

    device_init(dev, 0);

    return 0;
}

static int shift_device_deinit(void)
{
    struct tpc_module_device *dev = &shift_dev;

    if (!dev->is_init) {
        printf("shift device is not init\n");
        return -1;
    }

    if (dev->dev_is_start) {
        printf("shift device is start, need stop\n");
        return -1;
    }

    tpc_shift_gpio_deinit();
    tpc_hal_dma_deinit(dev->dma);

    dev->is_init = 0;
    dev->irq_callback = NULL;


    return 0;
}

static int heat_device_deinit(void)
{
    struct tpc_module_device *dev = &heat_dev;

    if (!dev->is_init) {
        printf("heat device is not init\n");
        return -1;
    }

    if (dev->dev_is_start) {
        printf("heat device is start, need stop\n");
        return -1;
    }

    tpc_heat_gpio_deinit();
    tpc_hal_dma_deinit(dev->dma);

    dev->is_init = 0;
    dev->irq_callback = NULL;

    return 0;
}

static int lat_device_deinit(void)
{
    struct tpc_module_device *dev = &lat_dev;
    if (!dev->is_init) {
        printf("lat device is not init\n");
        return -1;
    }

    if (dev->dev_is_start) {
        printf("lat device is start, need stop\n");
        return -1;
    }

    tpc_lat_gpio_deinit();
    dev->is_init = 0;
    dev->irq_callback = NULL;


    return 0;
}

int tpc_shift_init(struct tpc_shift_cfg *config)
{
    int ret = shift_device_init(config, 0, config->irq_callback);
    if (!ret)
        tpc_set_bit(PDENS, PDMEN, 1);

    return ret;
}


int tpc_heat_init(struct tpc_heat_cfg *config)
{
    int ret = heat_device_init(config, 0, config->irq_callback);
    if (!ret)
        tpc_set_bit(PDENS, PDMEN, 1);

    return ret;
}

int tpc_latch_init(struct tpc_lat_cfg *config)
{
    int ret = lat_device_init(config, config->irq_callback);
    if (!ret)
        tpc_set_bit(PDENS, PDMEN, 1);

    return ret;
}

int tpc_shift_write_data(unsigned char *data, int len)
{
    struct tpc_module_device *dev = &shift_dev;
    return device_write_data(dev, data, len);
}


int tpc_heat_write_data(unsigned int *data, int len)
{
    struct tpc_module_device *dev = &heat_dev;
    return device_write_data(dev, data, len);
}


int tpc_shift_start(void)
{
    struct tpc_module_device *dev = &shift_dev;

    tpc_hal_dma_start(dev->dma);

    /*开始之前等待fifo 有数据*/
    tpc_hal_wait_shift_fifo_not_empty();

    dev->dev_is_start = 1;
    dev->is_err = 0;
    tpc_set_bit(dev->start_reg, SAT, 1);

    return dev->is_err;
}


int tpc_heat_start(void)
{
    struct tpc_module_device *dev = &heat_dev;

    tpc_hal_dma_start(dev->dma);

    /*开始之前等待fifo 有数据*/
    tpc_hal_wait_heat_fifo_not_empty();

    dev->dev_is_start = 1;
    dev->is_err = 0;

    tpc_set_bit(dev->start_reg, SAT, 1);

    return dev->is_err;
}


int tpc_latch_start(void)
{
    struct tpc_module_device *dev = &lat_dev;

    dev->dev_is_start = 1;
    dev->is_err = 0;

    tpc_set_bit(dev->start_reg, SAT, 1);

    return dev->is_err;
}


void tpc_shift_quick_stop(void)
{
    struct tpc_module_device *dev = &shift_dev;

    tpc_hal_dma_stop(dev->dma, 1);

    tpc_set_bit(dev->start_reg, QSTP, 1);

    tpc_hal_shift_flush_fifo();

    mdelay(10);
}

void tpc_heat_quick_stop(void)
{
    struct tpc_module_device *dev = &shift_dev;

    tpc_hal_dma_stop(dev->dma, 1);

    tpc_set_bit(dev->start_reg, QSTP, 1);

    tpc_hal_heat_flush_fifo();

    mdelay(10);
}

void tpc_latch_quick_stop(void)
{
    struct tpc_module_device *dev = &shift_dev;

    tpc_set_bit(dev->start_reg, QSTP, 1);

    mdelay(10);
}


int tpc_shift_deinit(void)
{
    int ret = shift_device_deinit();
    if (!ret)
        tpc_set_bit(PDENC, PDMEN, 1);

    return ret;
}

int tpc_heat_deinit(void)
{
    int ret = heat_device_deinit();
    if (!ret)
        tpc_set_bit(PDENC, PDMEN, 1);

    return ret;
}

int tpc_latch_deinit(void)
{
    int ret = lat_device_deinit();
    if (!ret)
        tpc_set_bit(PDENC, PDMEN, 1);

    return ret;
}

/*----------------------------------------------------------------------------------------------------------*/

static int check_motor_config(int id, struct tpc_motor_cfg *config)
{
    int max_motor_channels = 4;
    if (APP_libmcu_x2600_tpc_motor_num < 2)
        max_motor_channels = 8;

    if (id >= APP_libmcu_x2600_tpc_motor_num) {
        printf("motor %d not support\n", id);
        return -1;
    }

    if (config->channels > max_motor_channels) {
        printf("motor %d not support %d channels\n", id, config->channels);
        return -1;
    }

    return 0;
}

static int motor_device_init(int id, struct tpc_motor_cfg *config, int dma_loop, tpc_irq_callback motor_callback)
{
    struct tpc_module_device *dev = &motor_dev[id];

    int ret = check_motor_config(id ,config);
    if (ret < 0)
        return -1;

    if (dev->is_init) {
        printf("tpc: motor %d is init\n", id);
        return -1;
    }

    dev->pdata = config;
    tpc_hal_motor_config(id, config);
    dev->unit_size = calculate_motor_unit_size(config);
    tpc_motor_gpio_init(id, config->channels);

    dev->irq_callback = motor_callback;

    device_init(dev, dma_loop);

    return 0;
}

static int motor_device_deinit(int id)
{
    struct tpc_module_device *dev = &motor_dev[id];

    if (!dev->is_init) {
        printf("motor %d device is not init\n",id);
        return -1;
    }


    if (dev->dev_is_start) {
        printf("motor %d device is start, need stop\n", id);
        return -1;
    }

    tpc_motor_gpio_deinit(id);
    tpc_hal_dma_deinit(dev->dma);

    dev->is_init = 0;

    return 0;
}

int tpc_motor_init(int id, struct tpc_motor_cfg *config)
{
    int ret = motor_device_init(id, config, 1, config->irq_callback);
    if (!ret)
        tpc_set_bit(PMENS(id), PMAEN, 1);

    return ret;
}


int tpc_motor_write_data(int id, unsigned int *data, int len)
{
    struct tpc_module_device *dev = &motor_dev[id];
    return device_write_data(dev, data, len);
}

void tpc_motor_data_stream_end(int id)
{
    tpc_hal_dma_stop(motor_dev[id].dma, 0);
}

void tpc_motor_start(int id)
{
    struct tpc_module_device *dev = &motor_dev[id];
    if (dev->dev_is_start) {
        printf("motor %d is already start\n", id);
        return;
    }

    tpc_hal_dma_start(dev->dma);

    tpc_hal_wait_motor_fifo_not_empty(id);

    tpc_set_bit(dev->start_reg, SAT, 1);

    dev->dev_is_start = 1;
    dev->is_err = 0;
}

int tpc_motor_stop(int id, int quick_stop)
{
    struct tpc_module_device *dev = &motor_dev[id];

    if (quick_stop)
        tpc_hal_dma_stop(dev->dma, 1);

    while(dev->dev_is_start && !quick_stop);

    tpc_set_bit(dev->start_reg, QSTP, 1);

    tpc_hal_motor_flush_fifo(id);

    if (quick_stop)
        mdelay(10);

    dev->dev_is_start = 0;

    return dev->is_err;
}

int tpc_motor_deinit(int id)
{
    int ret = motor_device_deinit(id);
    if (!ret)
        tpc_set_bit(PMENC(id), PMAEN, 1);

    return ret;
}

/*-------------------------------------------------------------------------------------------------*/

int tpc_sync_mode_init(struct tpc_shift_cfg *shift, struct tpc_heat_cfg *heat, struct tpc_lat_cfg *lat,
                           struct tpc_motor_cfg *motor, int motor_id)

{
    int ret;

    if (motor) {
        ret = motor_device_init(motor_id, motor, 1, motor->irq_callback);
        if (ret < 0)
            return -1;
    }

    ret = shift_device_init(shift, 1, shift->irq_callback);
    if (ret < 0)
        goto shift_err;

    ret = heat_device_init(heat, 1, heat->irq_callback);
    if (ret < 0)
        goto heat_err;

    ret = lat_device_init(lat, lat->irq_callback);
    if (ret < 0)
        goto lat_err;

    if (!motor) {
        print_head.start_reg = PDDMC;
        print_head.motor_id = -1;
    } else {
        print_head.start_reg = PAFC;
        print_head.motor_id = motor_id;
    }

    tpc_hal_enable_sync_mode(print_head.motor_id);

    print_head.is_err = 0;

    return 0;

lat_err:
    heat_device_deinit();
heat_err:
    shift_device_deinit();

shift_err:
    if (motor)
        motor_device_deinit(motor_id);

    return -1;
}

void tpc_sync_mode_start(void)
{

    if (print_head.is_start) {
        printf("sync mode is already start\n");
        return;
    }
    int motor_id = print_head.motor_id;

    /*开始之前等待fifo 有数据*/
    tpc_hal_dma_start(shift_dev.dma);
    tpc_hal_dma_start(heat_dev.dma);

    tpc_hal_wait_heat_fifo_not_empty();
    tpc_hal_wait_shift_fifo_not_empty();

    if (motor_id >= 0) {
        tpc_hal_dma_start(motor_dev[motor_id].dma);
        tpc_hal_wait_motor_fifo_not_empty(motor_id);
    }

    print_head.is_err = 0;
    print_head.is_start = 1;
    tpc_set_bit(print_head.start_reg, SAT, 1);

}

void tpc_sync_mode_data_stream_end(void)
{
    int motor_id = print_head.motor_id;

    if (motor_id >= 0)
        tpc_hal_dma_stop(motor_dev[motor_id].dma, 0);

    tpc_hal_dma_stop(shift_dev.dma, 0);
    tpc_hal_dma_stop(heat_dev.dma, 0);
}

int tpc_sync_mode_stop(int quick_stop)
{
    int motor_id = print_head.motor_id;

    if (quick_stop) {
        if (motor_id > 0)
            tpc_hal_dma_stop(motor_dev[motor_id].dma, 1);

        tpc_hal_dma_stop(shift_dev.dma, 1);
        tpc_hal_dma_stop(heat_dev.dma, 1);
    }

    while(print_head.is_start && !quick_stop);

    tpc_set_bit(print_head.start_reg, QSTP, 1);

    /*结束之后刷一下fifo， 保证fifo为空*/
    if (motor_id >= 0)
        tpc_hal_motor_flush_fifo(motor_id);


    tpc_hal_heat_flush_fifo();
    tpc_hal_shift_flush_fifo();

    /*不知道为啥快速停止，必须得延时个10ms。不然影响下次开始到正常结束，会进err*/
    if (quick_stop)
        mdelay(10);

    print_head.is_start = 0;

    return print_head.is_err;
}



int tpc_sync_mode_deinit(void)
{
    if (print_head.is_start) {
        printf("tpc sync mode is start, need stop\n");
        return -1;
    }

    int motor_id = print_head.motor_id;
    if (motor_id >= 0)
        motor_device_deinit(motor_id);

    shift_device_deinit();

    heat_device_deinit();

    lat_device_deinit();

    tpc_hal_disable_sync_mode(motor_id);

    return 0;
}

void tpc_init(void)
{
    int i = 0;
    for (i = 0; i < APP_libmcu_x2600_tpc_motor_num; i++) {
        struct tpc_module_device *motor = &motor_dev[i];
        motor->dma = tpc_hal_dma_request(DMA_RQ_TPC_MOTO0_TX + i, (void *)TPC_ADDR(PMTF(i)));
        motor->start_reg = PMMC(i);
    }

    shift_dev.dma = tpc_hal_dma_request(DMA_RQ_TPC_SHIFT_TX, (void *)TPC_ADDR(PDSDF));
    shift_dev.start_reg = PDSMC;

    heat_dev.dma = tpc_hal_dma_request(DMA_RQ_TPC_DEST_TX,(void *)TPC_ADDR(PDDTF));
    heat_dev.start_reg = PDDMC;

    lat_dev.start_reg = PDLMC;

    request_irq(IRQ_TPC, 0, tpc_irq_handle, "tpc", NULL);

    tpc_hal_init();
}

unsigned int tpc_get_one_us_clks(void)
{
    return DEFAULT_TPC_CLK_RATE / (1*1000*1000);
}