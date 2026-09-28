#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/byteorder/generic.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/input/mt.h>

#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

#include <utils/i2c.h>
#include <utils/gpio.h>

#include "ts_cst8xx_fw.h"


//驱动IC i2c从机地址 0x15(7 位)，加上读写为对应是：0x2A 写，0x2B 读
#define HYN_I2C_SLAVE_ADDR              0x15
#define HYN_I2C_SLAVE_WRITE             0x2A
#define HYN_I2C_SLAVE_READ              0x2B

//进入 BootLoader 升级模式 IIC 地址是：0x6A(7 位)，加上读写为对应是：0xD4 写，0xD5 读
#define HYN_I2C_UPDATA_SLAVE_ADDR       0x6A
#define HYN_I2C_UPDATA_SLAVE_WRITE      0xD4
#define HYN_I2C_UPDATA_SLAVE_READ       0xD5

#define HYN_I2C_SLAVE_ADDR_LEN          1
#define HYN_ONE_TCH_LEN                 9    //40//8 一次触摸读取数据长度

#define MAX_TOUCH_POINT_NUM             2 //最大检测点个数 10

#define HYN_TP_EVENT_DOWN               0x00 //表示第一拍按下
#define HYN_TP_EVENT_STAY               0x80 //表示手一直按下
#define HYN_TP_EVENT_UP                 0x40 //表示松手

#define HYN_REG_CHIP_ID                 0xAA
#define HYN_REG_FW_VER                  0xA6

#define TPD_DRIVER_NAME     "cst8xx-ts"

struct tp_board_info {
    int screen_min_x;
    int screen_min_y;
    int screen_max_x;
    int screen_max_y;

    int max_touch_number;
    int x_coords_flip;
    int y_coords_flip;
    int x_y_coords_exchange;

    int power_pin;
    int power_en_level;
    char *cst_regulator_name;
    struct regulator *cst_regulator;
    int irq_pin;
    int reset_pin;
    int i2c_channel;
};

struct cst8xx_ts_data {
    struct tp_board_info pdata;
    spinlock_t irq_lock;
    struct i2c_client *client;
    struct input_dev  *input_dev;
    struct work_struct  work;
    struct workqueue_struct *cst8xx_wq;
    int use_irq;
    int irq_is_disable;
    bool power_on;

    struct mutex lock;
};
enum {
    POWER_OFF = 0,
    POWER_ON,
};
static struct i2c_client *i2c_client;
struct cst8xx_ts_data g_ts;

#define printk(fmt,arg...)  printk(KERN_ERR "CST8xx linc HYN:[LINE=%d] "fmt,__LINE__, ##arg)

/* get ts data */
module_param_named(cst_i2c_bus_num, g_ts.pdata.i2c_channel, int, 0644);
// for display coords
module_param_named(cst_x_coords_min, g_ts.pdata.screen_min_x, int, 0644);
module_param_named(cst_y_coords_min, g_ts.pdata.screen_min_y, int, 0644);
module_param_named(cst_x_coords_max, g_ts.pdata.screen_max_x, int, 0644);
module_param_named(cst_y_coords_max, g_ts.pdata.screen_max_y, int, 0644);

// for reset, irq gpio info
module_param_gpio_named(cst_reset_gpio, g_ts.pdata.reset_pin, 0644);
module_param_gpio_named(cst_irq_gpio, g_ts.pdata.irq_pin, 0644);
module_param_gpio_named(cst_power_en_gpio, g_ts.pdata.power_pin, 0644);
module_param_named(cst_power_en_level, g_ts.pdata.power_en_level, int, 0644);
module_param_named(cst_regulator_name, g_ts.pdata.cst_regulator_name, charp, 0644);

module_param_named(cst_max_touch_number, g_ts.pdata.max_touch_number, int, 0644);
module_param_named(cst_x_coords_flip, g_ts.pdata.x_coords_flip, int, 0644);
module_param_named(cst_y_coords_flip, g_ts.pdata.y_coords_flip, int, 0644);
module_param_named(cst_x_y_coords_exchange, g_ts.pdata.x_y_coords_exchange, int, 0644);


static inline int m_gpio_request(int gpio, const char *name)
{
    if (gpio < 0)
        return -1;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk("failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

static inline void m_gpio_direction_output(int gpio, int value)
{
    if (gpio >= 0)
        gpio_direction_output(gpio, value);
}

static inline void m_gpio_direction_input(int gpio)
{
    if (gpio >= 0)
        gpio_direction_input(gpio);
}

extern unsigned char cst8_fw[];
static unsigned char *p_cst836u_upgrade_firmware;
static unsigned char fw_version=0x00;

#define HYN_FW_UPDATA 0


/*----------------------- Function Prototype --------------------------------*/

static int cst8xx_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
    int ret = -1;

    if (client == NULL) {
        printk("[IIC][%s]i2c_client==NULL!", __func__);
        return -1;
    }

    ret = i2c_master_send(client, writebuf, writelen);
    if (ret < 0) {
        printk("i2c_master_send i2c write error\n");
        return ret;
    }

    ret = i2c_master_recv(client, readbuf, readlen);
    if (ret < 0){
        printk("i2c_master_recv i2c read error.\n");
        return ret;
    }

    return ret;
}

int cst8xx_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
    int ret = -1;

    if (client == NULL) {
        printk("[IIC][%s]i2c_client==NULL!", __func__);
        return -1;
    }

    ret = i2c_master_send(client, writebuf, writelen);
    if (ret < 0)
        printk("i2c_master_send i2c write error\n");

    return ret;
}

int cst8xx_i2c_write_bytes(unsigned short reg,unsigned char *buf,unsigned short len,unsigned char reg_len)
{
    int ret;
    unsigned char mbuf[600];
    if (reg_len == 1) {
        mbuf[0] = reg;
        memcpy(mbuf+1, buf, len);
    } else {
        mbuf[0] = reg>>8;
        mbuf[1] = reg;
        memcpy(mbuf+2, buf, len);
    }

    ret = cst8xx_i2c_write(g_ts.client, mbuf, len+reg_len);
    if (ret < 0)
        printk("%s i2c write error.\n", __func__);

    return ret;
}

int cst8xx_i2c_read_bytes(unsigned short reg,unsigned char *buf,unsigned short len,unsigned char reg_len)
{
    int ret;
    unsigned char reg_buf[2];
    if (reg_len == 1) {
        reg_buf[0] = reg;
    } else {
        reg_buf[0] = reg>>8;
        reg_buf[1] = reg;
    }

    ret = cst8xx_i2c_read(g_ts.client, reg_buf, reg_len, buf,len);
    if (ret < 0)
        printk("%s i2c read error.\n",__func__);

    return ret;
}

static int cst8xx_power_ctrl(bool power_on)
{
    int ret = 0;

    if (power_on) {
        m_gpio_direction_output(g_ts.pdata.power_pin, g_ts.pdata.power_en_level);
        if(g_ts.pdata.cst_regulator) {
            ret = regulator_enable(g_ts.pdata.cst_regulator);
            if (ret) {
                printk("can not enable: %d", ret);
                return ret;
            }
        }
    } else {
        m_gpio_direction_output(g_ts.pdata.power_pin, !g_ts.pdata.power_en_level);
        if(g_ts.pdata.cst_regulator) {
            ret = regulator_disable(g_ts.pdata.cst_regulator);
            if (ret) {
                printk("can not disable: %d", ret);
                return ret;
            }
        }
    }

    printk("cst8xx_power_ctrl done,power_on = %d,ret = %d\n",power_on,ret);
    return ret;
}

static void cst8xx_irq_enable(struct cst8xx_ts_data *ts)
{
    unsigned long irqflags = 0;

    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (g_ts.irq_is_disable) {
        enable_irq(ts->client->irq);
        g_ts.irq_is_disable = 0;
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

static void cst8xx_irq_disable(struct cst8xx_ts_data *ts)
{
    unsigned long irqflags;

    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!g_ts.irq_is_disable) {
        g_ts.irq_is_disable = 1;
        disable_irq_nosync(ts->client->irq);
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

static int cst8xx_enter_bootmode(void)
{
    int ret= -1;

    printk("=======cst8xx_enter_bootmode==============\n");
    char retryCnt = 10;

    m_gpio_direction_output(g_ts.pdata.reset_pin, 0);
    mdelay(10);
    m_gpio_direction_output(g_ts.pdata.reset_pin, 1);
    mdelay(10);

    while (retryCnt--) {
        u8 cmd[3];
        cmd[0] = 0xAA;

        ret = cst8xx_i2c_write_bytes(0xA001,cmd,1,2);
        if (-1 == ret) {  // enter program mode
            mdelay(2); // 4ms
            continue;
        }

        ret = cst8xx_i2c_read_bytes(0xA003, cmd, 1,2);
        if (-1 == ret) { // read flag
            mdelay(2); // 4ms
            continue;
        } else {
            if (cmd[0] != 0x55) {
                msleep(2); // 4ms
                continue;
            } else {
                return 0;
            }
        }
    }

    printk("=======cst8xx_enter_bootmode=== end===========\n");
    return -1;
}

static u32 cst8xx_read_checksum(u16 startAddr,u16 len)
{
    union {
        u32 sum;
        u8 buf[4];
    } checksum;
    char cmd[3];
    char readback[4] = {0};
    int ret = -1;

    if (cst8xx_enter_bootmode() == -1)
       return -1;

    cmd[0] = 0x00;

    ret = cst8xx_i2c_write_bytes(0xA003, cmd, 1, 2);
    if (-1 == ret)
        return -1;

    msleep(500);

    ret = cst8xx_i2c_read_bytes(0xA000, readback, 1, 2);
    if (-1 == ret)
        return -1;

    if (readback[0] != 1)
        return -1;

    ret = cst8xx_i2c_read_bytes(0xA008, checksum.buf, 4, 2);
    if (-1 == ret)
        return -1;

    return checksum.sum;
}

static int cst8xx_update(u16 startAddr,u16 len,u8* src)
{
    u16 sum_len;
    u8 cmd[10];
    int ret = 0;

    printk("=======cst8xx_update==============\n");

    if (cst8xx_enter_bootmode() == -1)
       return -1;

    sum_len = 0;

    printk("=======cst8xx_enter_bootmode=====out=============\n");
    #define PER_LEN    512
    do {
        if (sum_len >= len)
            return -1;

        // send address
        cmd[1] = startAddr>>8;
        cmd[0] = startAddr&0xFF;
        ret = cst8xx_i2c_write_bytes(0xA014,cmd,2,2);

        ret = cst8xx_i2c_write_bytes(0xA018, src, PER_LEN, 2);

        cmd[0] = 0xEE;
        ret = cst8xx_i2c_write_bytes(0xA004, cmd, 1, 2);
        mdelay(100);
        {
            u8 retrycnt = 50;
            while (retrycnt--) {
                cmd[0] = 0;
                ret = cst8xx_i2c_read_bytes(0xA005, cmd, 1, 2);
                if (cmd[0] == 0x55) // success
                    break;
                msleep(10);
            }

            if (cmd[0]!=0x55)
                ret = -1;
        }
        startAddr += PER_LEN;
        src       += PER_LEN;
        sum_len   += PER_LEN;
    } while(len);

    // exit program mode
    cmd[0] = 0x00;

    ret = cst8xx_i2c_write_bytes(0xA003, cmd, 1, 2);

    printk("=======cst8xx_update=====end=============\n");

    return ret;
}

int hyn_ctpm_fw_upgrade_with_i_file(void)
{
    unsigned short startAddr;
    unsigned short length;
    unsigned short checksum;
    unsigned short chipchecksum;

    startAddr = *(p_cst836u_upgrade_firmware+1);
    length = *(p_cst836u_upgrade_firmware+3);
    checksum = *(p_cst836u_upgrade_firmware+5);
    startAddr <<= 8;
    startAddr |= *(p_cst836u_upgrade_firmware+0);
    length <<= 8;
    length |= *(p_cst836u_upgrade_firmware+2);
    checksum <<= 8;
    checksum |= *(p_cst836u_upgrade_firmware+4);
    cst8xx_update(startAddr, length, (p_cst836u_upgrade_firmware+6));

    chipchecksum = cst8xx_read_checksum(startAddr, length);;

    printk("\r\nCTP cst8xx update %s, checksum-0x%04x \n",((chipchecksum==checksum) ? "success" : "fail"),chipchecksum);

    return 0;
}

/***********************************************************************/

unsigned char hyn_ctpm_get_upg_ver(void)
{
    unsigned int ui_sz;

    p_cst836u_upgrade_firmware = (unsigned char *)cst8_fw;

    ui_sz = sizeof(cst8_fw);
    if (ui_sz > 2)
        return *(p_cst836u_upgrade_firmware+0x3BFC+6);
    else
        return 0xff;
}

int cst8xx_get_fw_info(void)
{
    int ret = -1;
    unsigned char buf[8] = {0};

    memset(buf, 0, sizeof(buf));
    //工作模式 IIC 地址是：0x15(7 位)，加上读写为对应是：0x2A 写，0x2B 读
    ret = cst8xx_i2c_read_bytes(0xAA, buf, 1, 1);
    if (ret < 0)
        printk("read chip ID fail\n");
    else
        printk("touch:%s(%d) read chip ID[0x%2x][0x%2x]\n",__FUNCTION__,__LINE__,HYN_REG_CHIP_ID,buf[0]);

    memset(buf, 0, sizeof(buf));

    ret = cst8xx_i2c_read_bytes(0xA6, buf, 1, 1);
    if(ret < 0)
        printk("read fw ver fail\n");
    else {
        fw_version = buf[0];
        printk("touch:%s(%d) read fw_version[0x%2x][0x%2x]\n",__FUNCTION__,__LINE__,HYN_REG_FW_VER,fw_version);
    }

    printk("======[hyn] start upgrade new verison 0x%2x\n", hyn_ctpm_get_upg_ver());//0x10

    return 0;
}

void cst8xx_gpio_int(void)
{
    if (strcmp("-1", g_ts.pdata.cst_regulator_name) && strlen(g_ts.pdata.cst_regulator_name)) {
        g_ts.pdata.cst_regulator = regulator_get(NULL, g_ts.pdata.cst_regulator_name);
        if (!g_ts.pdata.cst_regulator) {
            printk("Failed to get lcd_regulator!\n");
            return;
        }
    }

    m_gpio_request(g_ts.pdata.power_pin, "CST POWER");
    m_gpio_request(g_ts.pdata.irq_pin, "CST IRQ");
    m_gpio_request(g_ts.pdata.reset_pin, "CST RST");
    m_gpio_direction_input(g_ts.pdata.irq_pin);
    m_gpio_direction_output(g_ts.pdata.reset_pin, 0);
}

int cst8xx_reset(int hdelayms)
{
    m_gpio_direction_output(g_ts.pdata.reset_pin, 0);
    mdelay(20);
    m_gpio_direction_output(g_ts.pdata.reset_pin, 1);
    mdelay(hdelayms);
    return 0;
}

static void cst8xx_touch_down(struct input_dev *input_dev,s32 id,s32 x,s32 y)
{
    s32 temp;
    if (g_ts.pdata.x_y_coords_exchange) {
        temp = x;
        x = y;
        y = temp;
    }

    if (g_ts.pdata.x_coords_flip)
        x = g_ts.pdata.screen_max_x - x;

    if (g_ts.pdata.y_coords_flip)
        y = g_ts.pdata.screen_max_y - y;

    if (x < g_ts.pdata.screen_min_x)
        x = g_ts.pdata.screen_min_x;

    if (y < g_ts.pdata.screen_min_y)
        y = g_ts.pdata.screen_min_y;

    input_report_key(input_dev, BTN_TOUCH, 1);
    input_report_abs(input_dev, ABS_MT_TOOL_TYPE, 1);
    input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
    input_report_abs(input_dev, ABS_MT_PRESSURE, 20);
    input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, 20);
    input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, 20);
    input_report_key(input_dev, BTN_TOOL_FINGER, true);
    input_report_abs(input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(input_dev);
    // printk("cst8xx_touch_down  coordinate=X=%d,Y=%d,ID=%d\n",x,y,id);
}

static void cst8xx_touch_up(struct input_dev *input_dev, int id)
{
    input_report_key(input_dev, BTN_TOUCH, 0);
    input_mt_sync(input_dev);
}

static void ts_msg_callback(struct work_struct *work)
{
    int ret = -1;
    unsigned char rxdata[HYN_ONE_TCH_LEN] = {0}; //15
    unsigned char touch_id = 0x00;
    int touches; //按下点的个数，目前驱动只支持1个点按下
    int x = 0;
    int y = 0;

    memset(rxdata, 0x00, HYN_ONE_TCH_LEN);

    ret = cst8xx_i2c_read_bytes(0x00, rxdata, HYN_ONE_TCH_LEN, 1);
    if (ret < 0) {
        printk("i2c_read point fail \n");
        goto end;
    }

    touches = rxdata[2] & 0x07; //点的个数
    if (touches > g_ts.pdata.max_touch_number) {
        printk(KERN_ERR "%d touch points reported, only %d are supported\n", touches, g_ts.pdata.max_touch_number);
        touches = g_ts.pdata.max_touch_number;
    }

    x = (((s16)(rxdata[3] & 0x0F))<<8) | ((s16)rxdata[4]);
    y = (((s16)(rxdata[5] & 0x0F))<<8) | ((s16)rxdata[6]);

    //03寄存器会存触摸状态，BUF[3]&0xf0 == 0x00表示第一拍按下，==0x80表示手一直按下，==0x40表示松手
    touch_id = (rxdata[3]&0XF0);

    if (touch_id == HYN_TP_EVENT_DOWN || touch_id == HYN_TP_EVENT_STAY) //按下 / stay
        cst8xx_touch_down(g_ts.input_dev, rxdata[1], x, y);
    else if (touch_id == HYN_TP_EVENT_UP)    // 松开
        cst8xx_touch_up(g_ts.input_dev, rxdata[1]);

    input_sync(g_ts.input_dev);

end:
    if (g_ts.use_irq)
        cst8xx_irq_enable(&g_ts);
}

static irqreturn_t cst8xx_irq_callback(int irq, void *dev_id)
{
    if (g_ts.use_irq)
        cst8xx_irq_disable(&g_ts);

    queue_work(g_ts.cst8xx_wq, &g_ts.work);

    return IRQ_HANDLED;
}

void ts_fw_update(void)
{
    hyn_ctpm_fw_upgrade_with_i_file();
    return;
}

unsigned char ts_fw_VersionGet(void)
{
    return fw_version;
}

static int cst8xx_tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct input_dev *input;

    INIT_WORK(&g_ts.work, ts_msg_callback);
    spin_lock_init(&g_ts.irq_lock);

    g_ts.power_on = POWER_OFF;
    g_ts.use_irq = 1;
    g_ts.irq_is_disable = 1;
    g_ts.client = client;

    cst8xx_gpio_int();

    /*power on*/
    if (!g_ts.power_on) {
        ret = cst8xx_power_ctrl(POWER_ON);
        if (ret) {
            printk("power on failed\n");
            goto err_power_failed;
        }
        g_ts.power_on = POWER_ON;
    }

#if HYN_FW_UPDATA
    printk("======[hyn] start upgrade new verison 0x%2x\n", hyn_ctpm_get_upg_ver());//0x10
    hyn_ctpm_fw_upgrade_with_i_file();
#endif

    int resettime = 200;
    printk("[cst8xx_ts_open] cst8xx_gpio_int  ok resettime [%d]\n",resettime);
    cst8xx_reset(resettime); //rest plus delay 200ms waite touch ic ready

    cst8xx_get_fw_info();

    input = devm_input_allocate_device(&client->dev);
    if (input == NULL) {
        ret = -ENOMEM;
        goto err_power_failed;
    }

    g_ts.input_dev = input;
    input->name = client->name;
    input->id.bustype = BUS_I2C;

    __set_bit(EV_SYN, input->evbit);
    __set_bit(EV_KEY, input->evbit);
    __set_bit(EV_ABS, input->evbit);
    __set_bit(BTN_TOUCH, input->keybit);
    __set_bit(INPUT_PROP_DIRECT, input->propbit);

    if (g_ts.pdata.x_y_coords_exchange) {
        input_set_abs_params(input, ABS_MT_POSITION_X, 0, g_ts.pdata.screen_max_y, 0, 0);
        input_set_abs_params(input, ABS_MT_POSITION_Y, 0, g_ts.pdata.screen_max_x, 0, 0);
    } else {
        input_set_abs_params(input, ABS_MT_POSITION_X, 0, g_ts.pdata.screen_max_x, 0, 0);
        input_set_abs_params(input, ABS_MT_POSITION_Y, 0, g_ts.pdata.screen_max_y, 0, 0);
    }

    input_set_abs_params(input, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(input, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

    input_set_abs_params(input, ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0);
    input_set_abs_params(input, ABS_MT_PRESSURE, 0, 63, 0, 0);

    ret = input_register_device(g_ts.input_dev);
    if (ret) {
        printk(KERN_ERR "cst8xx_touch: failed to register input device: %d\n", ret);
        goto err_alloc_input_failed;
    }

    printk("[cst8xx_ts_open] irq IRQ_TYPE_EDGE_FALLING\n");
    ret = request_irq(gpio_to_irq(g_ts.pdata.irq_pin), cst8xx_irq_callback, IRQ_TYPE_EDGE_FALLING, "cst irq", NULL);
    if (ret) {
        printk("Request IRQ failed!ERRNO:%d.", ret);
        goto err_register_input_failed;
    } else {
        cst8xx_irq_disable(&g_ts);
    }

    if (g_ts.use_irq)
        cst8xx_irq_enable(&g_ts);

    return 0;

err_register_input_failed:
    input_unregister_device(g_ts.input_dev);
err_alloc_input_failed:
    input_free_device(g_ts.input_dev);
err_power_failed:
    cst8xx_power_ctrl(POWER_OFF);
    m_gpio_free(g_ts.pdata.irq_pin);
    m_gpio_free(g_ts.pdata.reset_pin);
    m_gpio_free(g_ts.pdata.power_pin);
    if (g_ts.pdata.cst_regulator)
        regulator_put(g_ts.pdata.cst_regulator);
    return -1;
}

static int cst8xx_tpd_remove(struct i2c_client *client)
{
    m_gpio_free(g_ts.pdata.irq_pin);
    m_gpio_free(g_ts.pdata.reset_pin);
    if (gpio_to_irq(g_ts.pdata.irq_pin))
        free_irq(gpio_to_irq(g_ts.pdata.irq_pin), NULL);

    m_gpio_free(g_ts.pdata.power_pin);
    if (g_ts.pdata.cst_regulator)
        regulator_put(g_ts.pdata.cst_regulator);

    input_unregister_device(g_ts.input_dev);
    input_free_device(g_ts.input_dev);

    return 0;
}

static const struct i2c_device_id cst8xx_tpd_id[] = {{TPD_DRIVER_NAME,0},{}};

static struct i2c_driver cst8xx_ts_driver = {
    .driver = {
        .name = TPD_DRIVER_NAME,
    },
    .probe    = cst8xx_tpd_probe,
    .remove   = cst8xx_tpd_remove,
    .id_table = cst8xx_tpd_id,
};

static struct i2c_board_info touch_cst8xx_ts_info = {
    .type = TPD_DRIVER_NAME,
};

/* called when loaded into kernel */
static int __init cst8xx_ts_init(void)
{
    int ret;
    printk(" linc ts is entering cst8xx_ts_init.\n");
    touch_cst8xx_ts_info.addr = HYN_I2C_SLAVE_ADDR;

    g_ts.cst8xx_wq = create_singlethread_workqueue("cst8xx_wq");
    if (!g_ts.cst8xx_wq) {
        printk("Creat workqueue failed.");
        return -ENOMEM;
    }

    ret = i2c_add_driver(&cst8xx_ts_driver);

    i2c_client = i2c_register_device(&touch_cst8xx_ts_info, g_ts.pdata.i2c_channel);
    if (i2c_client == NULL) {
        printk(KERN_ERR "failed to register i2c device\n");
        i2c_del_driver(&cst8xx_ts_driver);
        return -EINVAL;
    }

    return ret;
}

static void __exit cst8xx_ts_exit(void)
{
    printk(" linc ts is entering cst8xx_ts_exit.\n");

    i2c_unregister_device(i2c_client);

    i2c_del_driver(&cst8xx_ts_driver);
    if (g_ts.cst8xx_wq) {
        destroy_workqueue(g_ts.cst8xx_wq);
        g_ts.cst8xx_wq = NULL;
    }
}

module_init(cst8xx_ts_init);
module_exit(cst8xx_ts_exit);

MODULE_DESCRIPTION("CST Series Driver");
MODULE_LICENSE("GPL");
