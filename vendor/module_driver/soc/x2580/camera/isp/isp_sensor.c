#include <linux/module.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <common.h>
#include <bit_field.h>

#include "system_sensor_drv.h"
#include "tiziano-core-ctrl.h"
#include "isp.h"

static struct jz_isp_data *g_ispcore = NULL;

static void sensor_hw_reset_enable(void)
{
}

static void sensor_hw_reset_disable(void)
{
}

static int32_t sensor_alloc_analog_gain(int32_t gain, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;
    unsigned int again = 0;

    if (sensor->ops.alloc_again) {
        gain = sensor->ops.alloc_again(gain, LOG2_GAIN_SHIFT, &again);
        p_ctx->again = again;
    } else
        printk(KERN_ERR "sensor->ops.alloc_again is NULL!\n");

    return gain;
}

static int32_t sensor_alloc_analog_gain_short(int32_t gain, sensor_context_t *p_ctx)
{
    unsigned int again = 0;
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    if (sensor->ops.alloc_again_short) {
        gain = sensor->ops.alloc_again_short(gain, LOG2_GAIN_SHIFT, &again);
        p_ctx->again_short = again;
    } else {
        printk(KERN_ERR "sensor->ops.alloc_again_short is NULL!\n");
    }

    return gain;
}

static int32_t sensor_alloc_digital_gain(int32_t gain, sensor_context_t *p_ctx)
{
    unsigned int dgain = 0;
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    if (sensor->ops.alloc_dgain) {
        gain = sensor->ops.alloc_dgain(gain, LOG2_GAIN_SHIFT, &dgain);
        p_ctx->dgain = dgain;
    } else {
        printk(KERN_ERR "sensor->ops.alloc_dgain is NULL!\n");
    }

    return gain;
}

static int32_t sensor_alloc_digital_gain_short(int32_t gain, sensor_context_t *p_ctx)
{
    unsigned int dgain = 0;
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    if(sensor->ops.alloc_dgain_short == NULL) {
        gain = 0;
        p_ctx->dgain_short = 0;
    } else {
        gain = sensor->ops.alloc_dgain_short(gain, LOG2_GAIN_SHIFT, &dgain);
        p_ctx->dgain_short = dgain;
        printk("result gain is 0x%x\n",p_ctx->dgain);
    }

    return gain;
}

static uint32_t sensor_alloc_integration_time(uint32_t int_time, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;
    unsigned int sensor_it = 0;

    if(sensor->ops.alloc_integration_time == NULL){
        p_ctx->it = int_time;
    } else {
        int_time = sensor->ops.alloc_integration_time(int_time, 0, &sensor_it);
        p_ctx->it = sensor_it;
    }

    return int_time;
}

static uint32_t sensor_alloc_integration_time_short(uint32_t int_time, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;
    unsigned int sensor_it = 0;

    if(sensor->ops.alloc_integration_time_short == NULL){
        p_ctx->it_short = int_time;
    } else {
        int_time = sensor->ops.alloc_integration_time_short(int_time, 0, &sensor_it);
        p_ctx->it_short = sensor_it;
    }

    return int_time;

}

static void sensor_set_integration_time(uint16_t int_time, sensor_param_t* param)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;
    /* printk("[ %s:%d ] it id is %d, it is %d,%d\n", __func__, __LINE__, param->sensor_ctx.sensor_id, sensor->sensor_info.integration_time, int_time); */
    if(int_time != sensor->sensor_info.integration_time){
        // printk("%s,%d: int_time = %d\n", __func__, __LINE__, int_time);
        sensor->sensor_info.integration_time = int_time;
        sensor->sensor_info.expo = (sensor->sensor_info.expo & 0xffff0000) + int_time;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_INTEGRATION].flag = 1;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_INTEGRATION].value = int_time;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_EXPO].flag = 1;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_EXPO].value = sensor->sensor_info.expo;
    }
}

static void sensor_set_integration_time_short(uint16_t int_time, sensor_param_t* param)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    if(int_time != sensor->sensor_info.integration_time_short){
        // printk("%s,%d: int_time = %d\n", __func__, __LINE__, int_time);
        sensor->sensor_info.integration_time_short = int_time;
        sensor->sensor_info.expo_short = (sensor->sensor_info.expo_short & 0xffff0000) + int_time;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_INTEGRATION_SHORT].flag = 1;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_INTEGRATION_SHORT].value = int_time;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_EXPO_SHORT].flag = 1;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_EXPO_SHORT].value = sensor->sensor_info.expo_short;
    }
}

static void sensor_set_analog_gain(uint32_t again_reg_val, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;
    if(again_reg_val != sensor->sensor_info.again){
        // printk("[ %s:%d ] gain id is %d, gain is %d,%d\n", __func__, __LINE__, p_ctx->sensor_id, sensor->sensor_info.again, again_reg_val);
        sensor->sensor_info.again = again_reg_val;
        sensor->sensor_info.expo = (sensor->sensor_info.expo & 0xffff) | (again_reg_val << 16);
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_AGAIN].flag = 1;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_AGAIN].value = again_reg_val;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_EXPO].flag = 1;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_EXPO].value = sensor->sensor_info.expo;
    }
}

static void sensor_set_analog_gain_short(uint32_t again_reg_val, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    if(again_reg_val != sensor->sensor_info.again_short){
        sensor->sensor_info.again_short = again_reg_val;
        sensor->sensor_info.expo_short = (sensor->sensor_info.expo_short & 0xffff) | (again_reg_val << 16);
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_AGAIN_SHORT].flag = 1;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_AGAIN_SHORT].value = again_reg_val;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_EXPO_SHORT].flag = 1;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_EXPO_SHORT].value = sensor->sensor_info.expo_short;
    }
}

static void sensor_set_digital_gain(uint32_t dgain_reg_val, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    if(dgain_reg_val != sensor->sensor_info.dgain) {
        sensor->sensor_info.dgain = dgain_reg_val;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_DGAIN].flag = 1;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_DGAIN].value = dgain_reg_val;
    }
}

static void sensor_set_digital_gain_short(uint32_t dgain_reg_val, sensor_context_t *p_ctx)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    if(dgain_reg_val != sensor->sensor_info.dgain_short){
        sensor->sensor_info.dgain_short = dgain_reg_val;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_DGAIN_SHORT].flag = 1;
        g_ispcore->i2c_msgs[p_ctx->sensor_id][TISP_I2C_SET_DGAIN_SHORT].value = dgain_reg_val;
    }
}

static uint16_t sensor_get_normal_fps(sensor_param_t* param)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;
    unsigned int fps = sensor->sensor_info.fps;
    return (((fps >> 16) / (fps & 0xffff)) << 8) + ((((fps >> 16) % (fps & 0xffff)) << 8) / (fps & 0xffff));
}

static uint16_t sensor_read_black_pedestal(int i,uint32_t gain)
{
    unsigned int black = 0;
    return black;
}

static void sensor_set_mode(uint8_t mode, sensor_param_t* param)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    //sensor->ops.set_mode(mode);
    param->active.width = sensor->sensor_info.width;
    param->active.height = sensor->sensor_info.height;
    param->total.width = sensor->sensor_info.total_width;
    param->total.height = sensor->sensor_info.total_height;
    param->integration_time_min = sensor->sensor_info.min_integration_time;
    param->integration_time_min_short =sensor->sensor_info.min_integration_time_short;
    param->integration_time_max_short = sensor->sensor_info.max_integration_time_short;
    param->integration_time_max = sensor->sensor_info.max_integration_time;
    param->integration_time_limit = sensor->sensor_info.integration_time_limit;
    param->mode = mode;

}

static void sensor_start_changes(sensor_context_t *p_ctx)
{
}

static void sensor_end_changes(sensor_context_t *p_ctx)
{
}

static uint16_t sensor_get_id(sensor_context_t *p_ctx)
{
    return 0;
}

static void sensor_set_wdr_mode(uint8_t mode, sensor_param_t* param)
{
    //      printk("^^^ %s ^^^\n",__func__);
    // This sensor does not support native WDR
}

static uint32_t sensor_fps_control(uint8_t fps, sensor_param_t* param)
{
    struct jz_isp_data *isp = g_ispcore;
    struct sensor_attr *sensor = isp->camera.sensor;

    param->total.width = sensor->sensor_info.total_width;
    param->total.height = sensor->sensor_info.total_height;
    param->integration_time_min = sensor->sensor_info.min_integration_time;
    param->integration_time_max = sensor->sensor_info.max_integration_time;
    param->integration_time_min_short = sensor->sensor_info.min_integration_time_short;
    param->integration_time_max_short = sensor->sensor_info.max_integration_time_short;
    param->integration_time_long_max = sensor->sensor_info.max_integration_time;
    param->integration_time_limit = sensor->sensor_info.integration_time_limit;

    return g_ispcore->camera.sensor->sensor_info.fps;
}

static void sensor_set_fps(uint32_t fps, sensor_param_t* param)
{
    uint32_t *ori_fps = &g_ispcore->camera.sensor->sensor_info.fps;

    if (fps != *ori_fps) {
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_FPS].flag = 1;
        g_ispcore->i2c_msgs[param->sensor_ctx.sensor_id][TISP_I2C_SET_FPS].value = fps;
        *ori_fps = fps;
    }
}

static void sensor_disable_isp(sensor_context_t *p_ctx)
{
}

static uint32_t sensor_get_lines_per_second(sensor_param_t* param)
{
    uint32_t lines_per_second=0;
    return lines_per_second;
}

static inline unsigned int fixed16_add_fixed16(unsigned int f1, unsigned int f2)
{
    unsigned int i = ((f1 >> TISP_GAIN_FIXED_POINT) + (f2 >> TISP_GAIN_FIXED_POINT)) << TISP_GAIN_FIXED_POINT;
    unsigned int p = (f1 & ((1 << TISP_GAIN_FIXED_POINT) - 1)) + (f2 & ((1 << TISP_GAIN_FIXED_POINT) - 1));
    return i + p;
}

void sensor_init(sensor_control_t *ctrl, int vinum)
{
    struct jz_isp_data *isp = &g_ispcore[vinum];
    struct sensor_attr *sensor = isp->camera.sensor;

    // printk("^^^ %s ^^^\n",__func__);

    ctrl->param[vinum].again_log2_max = sensor->sensor_info.max_again;
    ctrl->param[vinum].dgain_log2_max = sensor->sensor_info.max_dgain;
    ctrl->param[vinum].integration_time_apply_delay = sensor->sensor_info.integration_time_apply_delay;
    ctrl->param[vinum].analog_gain_apply_delay = sensor->sensor_info.again_apply_delay;
    ctrl->param[vinum].digital_gain_apply_delay = sensor->sensor_info.dgain_apply_delay;
    ctrl->param[vinum].analog_gain_short_apply_delay = sensor->sensor_info.again_short_apply_delay;
    ctrl->param[vinum].digital_gain_short_apply_delay = sensor->sensor_info.dgain_short_apply_delay;
    ctrl->param[vinum].integration_time_min = sensor->sensor_info.min_integration_time;
    ctrl->param[vinum].integration_time_max = sensor->sensor_info.max_integration_time;
    ctrl->param[vinum].integration_time_min_short = sensor->sensor_info.min_integration_time_short;
    ctrl->param[vinum].integration_time_max_short = sensor->sensor_info.max_integration_time_short;
    ctrl->param[vinum].again_log2_max_short = sensor->sensor_info.max_again_short;
    ctrl->param[vinum].dgain_log2_max_short = sensor->sensor_info.max_dgain_short;

    ctrl->hw_reset_disable = sensor_hw_reset_disable;
    ctrl->hw_reset_enable = sensor_hw_reset_enable;
    ctrl->alloc_analog_gain = sensor_alloc_analog_gain;
    ctrl->alloc_analog_gain_short = sensor_alloc_analog_gain_short;
    ctrl->alloc_digital_gain = sensor_alloc_digital_gain;
    ctrl->alloc_digital_gain_short = sensor_alloc_digital_gain_short;
    ctrl->alloc_integration_time = sensor_alloc_integration_time;
    ctrl->alloc_integration_time_short = sensor_alloc_integration_time_short;
    ctrl->set_integration_time = sensor_set_integration_time;
    ctrl->set_integration_time_short = sensor_set_integration_time_short;
    ctrl->start_changes = sensor_start_changes;
    ctrl->end_changes = sensor_end_changes;
    ctrl->set_analog_gain = sensor_set_analog_gain;
    ctrl->set_analog_gain_short = sensor_set_analog_gain_short;
    ctrl->set_digital_gain = sensor_set_digital_gain;
    ctrl->set_digital_gain_short = sensor_set_digital_gain_short;
    ctrl->get_normal_fps = sensor_get_normal_fps;
    ctrl->read_black_pedestal = sensor_read_black_pedestal;
    ctrl->set_mode = sensor_set_mode;
    ctrl->set_wdr_mode = sensor_set_wdr_mode;
    ctrl->fps_control = sensor_fps_control;
    ctrl->get_id = sensor_get_id;
    ctrl->disable_isp = sensor_disable_isp;
    ctrl->get_lines_per_second = sensor_get_lines_per_second;
    ctrl->set_fps = sensor_set_fps;

}

int sensor_early_init(void *ispcore)
{
    if(!ispcore)
            return -EINVAL;
    if(!g_ispcore)
            g_ispcore = ispcore;

    return ISP_SUCCESS;
}