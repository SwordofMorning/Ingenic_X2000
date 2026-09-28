#include <stdio.h>
#include <string.h>
#include <soc/base.h>
#include <cpu/io.h>
#include <driver/irq.h>
#include <cpu/host_cpu.h>
#include <driver/gpio.h>
#include <common.h>
#include <soc/tpc.h>
#include <delay.h>
#include <driver/systick.h>

static volatile int is_data_comming = 0;

#ifdef APP_libmcu_driver_irq
static void host_irq_cb(void)
{
    is_data_comming = 1;
}
#endif


static void mcu_tpc_is_ready(void)
{

    char buf[] = "mcu_tpc_ready";

    int len = strlen(buf) + 1;
    int send_len = len;

    while (len) {
        while(mcu_test_host_busy());

        while (len) {
            int ret = host_cpu_write(buf, len);
            len -= ret;
            if (!ret)
                break;
        }

        mcu_notify_host(send_len);
    }
}

static int mcu_read_host_tpc_data(unsigned char *buf, int size)
{

    int len = 0;
    int ret;

    while (!is_data_comming);
    is_data_comming = 0;

    while (1) {
        ret = host_cpu_read(buf+len, size-len);
        if (!ret)
            break;

        len += ret;
    }

    return len;
}



/*每行单个数据通道输出多少字节*/
#define SHIFT_LINES_BYTES 48
/*数据通道*/
#define SHIFT_CHANNELS     1

#define HEAT_CHANNELS      2

#define MOTOR_CHANNLES     4
#define MOTOR_REPEAT_STEP  4

#define MOTOR_ALIGN_STEP   4

#define MOTOR_ID  0


#define TPC_US_CLKCNT(us)      ((uint64_t)(us)*(DEFAULT_TPC_CLK_RATE / (1*1000*1000)))

#define TPC_HEAT_ENABLE                 (1 << 24)
#define TPC_HEAT_DISABLE                (0 << 24)
#define TPC_HEAT_TRIGGER_BY_MOTOR       (0 << 24)
#define TPC_HEAT_TRIGGER_BY_LATCH       (1 << 24)



struct tpc_shift_cfg shift_cfg = {
    .big_end = 0,
    .channels = SHIFT_CHANNELS,
    .data_invert = 0,
    .spi_pha = 0,
    .spi_pol = 0,
    .line_bytes = SHIFT_LINES_BYTES,
    .out_clk_rate = 8*1000*1000,
};

struct tpc_heat_cfg heat_cfg = {
    .channels = HEAT_CHANNELS,
    .idle_level = 0,
    .max_heat_clkcnt = TPC_US_CLKCNT(2*1000*1000),
    .wait_clkcnt_out = 1,
};

struct tpc_lat_cfg lat_cfg = {
    .active_clkcnt = TPC_US_CLKCNT(1),
    .idle_level = 1,
    .wait_clkcnt_out = TPC_US_CLKCNT(1),
    .wait_clkcnt_out_shift = TPC_US_CLKCNT(1),
};

struct tpc_motor_cfg motor_cfg_0 = {
    .channels = 4,
    .idle_level = 0b0000,

    .start_step_io = 0b0000,
    .start_step_clkcnt = TPC_US_CLKCNT(12*1000),

    .stop_step_io = 0b0000,
    .stop_step_clkcnt = TPC_US_CLKCNT(12*1000),

    .repeat_step = MOTOR_REPEAT_STEP,
    .repeat_step_io = {
        [0] = 0b1100,
        [1] = 0b0011,
        [2] = 0b0110,
        [3] = 0b1001,
    },

    .max_step_clkcnt = TPC_US_CLKCNT(2*1000*1000),
    .align_step = MOTOR_ALIGN_STEP,
};


struct heat_data {
    unsigned int nxt_trigger_type;
    unsigned int channels[HEAT_CHANNELS];
};


static int write_sync_mode_data(unsigned char *shift_data, void *heat_data, void  *motor_data,
                                          int shift_size, int heat_size, int motor_size)
{

    int ret;
    int motor_w = motor_size;
    int heat_w = heat_size;
    int shift_w = shift_size;


   while(1) {
        ret = tpc_motor_write_data(MOTOR_ID, motor_data, motor_w);
        motor_data += ret;
        motor_w -= ret;

        ret = tpc_heat_write_data((unsigned int *)heat_data, heat_w);
        heat_data += ret;
        heat_w -= ret;


        ret = tpc_shift_write_data(shift_data, shift_w);
        shift_data +=  ret;
        shift_w -= ret;


        if (!motor_w && !heat_w && !shift_w)
            break;
    }

    return 0;
}


void tpc_example(void)
{
    printf("%s is called\n", __func__);

#ifdef APP_libmcu_driver_irq
    host_cpu_set_irq_callback(host_irq_cb);
#endif

    tpc_init();

    int ret;
    ret = tpc_sync_mode_init(&shift_cfg, &heat_cfg, &lat_cfg, NULL, -1);
    if (ret < 0)
        return;

    ret = tpc_motor_init(MOTOR_ID, &motor_cfg_0);
    if (ret < 0)
        return;

    unsigned char shift_data[2048];
    int shift_size = 0;


    int heat_size = sizeof(struct heat_data);
    struct heat_data *heat_data = malloc(heat_size);

    int motor_size = MOTOR_REPEAT_STEP * sizeof(int);
    unsigned int *motor_data = malloc(motor_size);

    int heat_clkcnt = TPC_US_CLKCNT(10 * 1000);

    int cnt = 0;
    while (1) {
        /*通知大核 准备好接收数据*/
        mcu_tpc_is_ready();

        memset(shift_data, 0, sizeof(shift_data));

        shift_size =  mcu_read_host_tpc_data(shift_data, sizeof(shift_data));

        motor_data[0] = TPC_US_CLKCNT(10 * 1000);
        motor_data[1] = TPC_US_CLKCNT(10 * 1000);
        motor_data[2] = TPC_US_CLKCNT(10 * 1000);
        motor_data[3] = TPC_US_CLKCNT(10 * 1000);

        /*与全对齐模式相比，电机不对齐的话，得手动计算加热到下一行的锁存的延时时间*/
        uint64_t heat_delay_clkcnt = motor_data[0] + motor_data[1] + motor_data[2] + motor_data[3] - heat_clkcnt;
        heat_data[0].nxt_trigger_type = TPC_HEAT_TRIGGER_BY_LATCH | heat_delay_clkcnt;
        heat_data[0].channels[0] = TPC_HEAT_ENABLE | heat_clkcnt;
        heat_data[0].channels[1] = TPC_HEAT_ENABLE | heat_clkcnt;


        if (strcmp("tpc_data_end", (char *)shift_data) == 0) {

            tpc_sync_mode_data_stream_end();
            tpc_motor_data_stream_end(MOTOR_ID);

            tpc_sync_mode_stop(0);
            tpc_motor_stop(MOTOR_ID, 0);
            cnt = 0;
            continue;
        }

        write_sync_mode_data(shift_data, (void *)heat_data, (void *)motor_data, shift_size, heat_size, motor_size);

        /*收到32行数据后(内部默认申请32行 buffer大小)开始输出*/
        cnt++;
        if (cnt == 32) {
            tpc_sync_mode_start();
            tpc_motor_start(MOTOR_ID);
        }
    }
}
