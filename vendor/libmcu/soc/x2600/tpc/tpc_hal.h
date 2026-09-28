#ifndef __TPC_HAL_H__
#define __TPC_HAL_H__

#include "soc/tpc_data.h"
#include <soc/tpc.h>
#include <driver/dma.h>

#define MAX_SHIFT_FIFO  1284
#define MAX_HEAT_FIFO   132
#define MAX_MOTOR_FIFO  260

struct tpc_dma;


/*控制器相关*/
void tpc_hal_init(void);
void tpc_hal_shift_config(struct tpc_shift_cfg *config);
void tpc_hal_lat_config(struct tpc_lat_cfg *config);
void tpc_hal_heat_config(struct tpc_heat_cfg *config);
void tpc_hal_motor_config(int id, struct tpc_motor_cfg *config);


void tpc_hal_enable_sync_mode(int motor_id);
void tpc_hal_disable_sync_mode(int motor_id);

void tpc_hal_set_shift_trigger(int size);
void tpc_hal_set_heat_trigger(int size);
void tpc_hal_set_motor_trigger(int id, int size);

void tpc_hal_heat_flush_fifo(void);
void tpc_hal_shift_flush_fifo(void);
void tpc_hal_motor_flush_fifo(int id);


void tpc_hal_wait_heat_fifo_not_empty(void);
void tpc_hal_wait_shift_fifo_not_empty(void);
void tpc_hal_wait_motor_fifo_not_empty(int id);


void tpc_hal_wait_heat_fifo_empty(void);
void tpc_hal_wait_shift_fifo_empty(void);
void tpc_hal_wait_motor_fifo_empty(int id);


/*gpio 相关*/
void tpc_motor_gpio_init(int id, int channels);
void tpc_heat_gpio_init(int channels);
void tpc_shift_gpio_init(int channels);
void tpc_lat_gpio_init(void);

void tpc_motor_gpio_deinit(int id);
void tpc_heat_gpio_deinit(void);
void tpc_shift_gpio_deinit(void);
void tpc_lat_gpio_deinit(void);


struct tpc_dma *tpc_hal_dma_request(enum DMA_request_type type, void *dst);
void tpc_hal_dma_init(struct tpc_dma *t_dma, int is_loop, int size, int tpc_unit_size);
void tpc_hal_dma_deinit(struct tpc_dma *t_dma);
int tpc_hal_dma_write_data(struct tpc_dma *t_dma, void *mem, int bytes);
void tpc_hal_dma_start(struct tpc_dma *t_dma);
void tpc_hal_dma_stop(struct tpc_dma *t_dma, int is_quick);



#endif