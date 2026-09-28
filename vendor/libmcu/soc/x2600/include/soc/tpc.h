#ifndef __TPC_H__
#define __TPC_H__

#include "tpc_data.h"

#define MOTOR_MAX_TIME_ERR           -2
#define MOTOR_DATA_UNDER             -3
#define PRINT_HEAD_ERR               -4

#define HEAT_DATA_UNDER              -5
#define HEAT_MAX_TIME_ERR            -6

#define SHIFT_DATA_UNDER             -7


#ifndef DEFAULT_TPC_CLK_RATE
#define DEFAULT_TPC_CLK_RATE   200*1000*1000
#endif



void tpc_init(void);

unsigned int tpc_get_one_us_clks(void);

/*-----------------------------shift lat heat设备单步接口， 用的是普通dma---------------------------------------------------------------------*/
int tpc_shift_init(struct tpc_shift_cfg *config);
int tpc_heat_init(struct tpc_heat_cfg *config);
int tpc_latch_init(struct tpc_lat_cfg *config);


int tpc_shift_write_data(unsigned char *data, int len);
int tpc_heat_write_data(unsigned int *data, int len);

int tpc_shift_start(void);
int tpc_heat_start(void);
int tpc_latch_start(void);

/* quick_stop 用于马上停止设备，一般不会用到*/
void tpc_shift_quick_stop(void);
void tpc_heat_quick_stop(void);
void tpc_latch_quick_stop(void);


int tpc_shift_deinit(void);
int tpc_heat_deinit(void);
int tpc_latch_deinit(void);

/*-----------------------------------------电机接口， 用的是loop dma---------------------------------------------------*/

int tpc_motor_init(int id, struct tpc_motor_cfg *motor);
int tpc_motor_write_data(int id, unsigned int *data, int len);

void tpc_motor_data_stream_end(int id);

void tpc_motor_start(int id);
int tpc_motor_stop(int id, int quick_stop);
int tpc_motor_deinit(int id);


/*-----------------------------------------同步模式接口, 用的是 loop dma------------------------------------------------*/
int tpc_sync_mode_init(struct tpc_shift_cfg *shift, struct tpc_heat_cfg *heat, struct tpc_lat_cfg *lat,
                       struct tpc_motor_cfg *motor, int motor_id);

void tpc_sync_mode_start(void);

void tpc_sync_mode_data_stream_end(void);

int tpc_sync_mode_stop(int quick_stop);

int tpc_sync_mode_deinit(void);


#endif
