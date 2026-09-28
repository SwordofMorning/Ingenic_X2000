#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <utils/gpio.h>
#include <utils/i2c.h>

#include <bit_field.h>
#include "eta6919_core.h"

#define ETA_I2C_ADDR    (0x6A)

enum eta6919_mode {
    ETA6919_NONE,
    ETA6919_CHG,
    ETA6919_OTG,
};

enum eta6919_chg_status {
    ETA6919_CHG_NONE,
    ETA6919_CHG_TRICKLE,
    ETA6919_CHG_FAST,
    ETA6919_CHG_COMPLETE,
};

struct eta6919_plat {
    int irq;
    int int_gpio; // PB07
    int i2c_bus_num; // data: PB09 scl: PB10  100000

    int mode;
    int input_current;
    int input_voltage;
    int charge_current;
    int charge_voltage;
    int charge_pre_current;
    int charge_term_current;
};

struct eta6919_state {
    struct power_supply *psy;
    struct i2c_client *client;
    struct eta6919_plat pdata;
};

struct eta6919_state eta6919;

module_param_gpio_named(pmu_int_gpio, eta6919.pdata.int_gpio, 0644);
module_param_named(pmu_i2c_bus_num, eta6919.pdata.i2c_bus_num, int, 0644);

module_param_named(pmu_mode, eta6919.pdata.mode, int, 0644);
module_param_named(pmu_input_current, eta6919.pdata.input_current, int, 0644);
module_param_named(pmu_input_voltage, eta6919.pdata.input_voltage, int, 0644);
module_param_named(pmu_charge_current, eta6919.pdata.charge_current, int, 0644);
module_param_named(pmu_charge_voltage, eta6919.pdata.charge_voltage, int, 0644);
module_param_named(pmu_charge_pre_current, eta6919.pdata.charge_pre_current, int, 0644);
module_param_named(pmu_charge_term_current, eta6919.pdata.charge_term_current, int, 0644);

static inline int eta_write_reg(struct i2c_client *client, unsigned char reg,
                             unsigned char val)
{
    int ret;
    ret = i2c_smbus_write_byte_data(client, reg, val);
    if (ret < 0) {
        printk(KERN_ERR "ETA: failed writing 0x%02x to 0x%02x\n",
                val, reg);
        return ret;
    }

    return 0;
}

static inline int eta_read_reg(struct i2c_client *client, unsigned char reg,
                            unsigned char *val)
{
    int ret;
    ret = i2c_smbus_read_byte_data(client, reg);
    if (ret < 0) {
        printk(KERN_ERR "ETA: failed reading at 0x%02x\n", reg);
        return ret;
    }

    *val = (unsigned char)ret;
    return 0;
}

static inline int eta_get_bits(struct i2c_client *client, unsigned char reg, int mask, int shift)
{
    int ret;
    unsigned char val;
    ret = eta_read_reg(client, reg, &val);
    if (ret < 0)
        return ret;

    val &= (mask << shift);
    return val >> shift;
}

static int eta_set_bits(struct i2c_client *client, unsigned char reg,
                int mask, int shift, unsigned char val)
{
    int ret;
    unsigned char regval;

    ret = eta_read_reg(client, reg, &regval);
    if (ret < 0)
        return ret;

    regval &= ~(mask << shift);
    regval |= (val << shift);

    return eta_write_reg(client, reg, regval);
}

static int eta_get_bit_field(unsigned char val, int mask, int shift)
{
    return (val & (mask << shift)) >> shift;
}

void eta6919_dump(void)
{
    int i;
    unsigned char regval;
    printk(KERN_ERR "=======================dump eta6919=======================\n");
    for (i = 0; i < 13; i++) {
        eta_read_reg(eta6919.client, i, &regval);
        printk(KERN_ERR "ETACON%X: 0x%02x\n", i, regval);
    }
    printk(KERN_ERR "==========================================================\n");
}

static inline int m_gpio_request(int gpio, enum gpio_function func, const char *name)
{
    if (gpio < 0)
        return 0;

    int ret = gpio_request(gpio, name);
    if (ret) {
        char buf[20];
        printk(KERN_ERR "ETA: failed to request %s: %s\n", name, gpio_to_str(gpio, buf));
        return ret;
    }

    gpio_set_func(gpio, func);

    return 0;
}

static inline void m_gpio_free(int gpio)
{
    if (gpio >= 0)
        gpio_free(gpio);
}

/////////////////////////////////////////////////////////////////////////////////////

static unsigned int bmt_find_closest_regval(const unsigned int *pList,
        unsigned int number, unsigned int level)
{
    int i;

    for (i = 1; i < number; i++) {
        if (level < pList[i])
            break;
    }

    return i-1;
}

static int eta6919_get_input_current_limit(struct eta6919_state *st,
        union power_supply_propval *val)
{
    unsigned int regval;

    regval = eta_get_bits(st->client, ETACON0, ETACON0_IINDPM_MASK, ETACON0_IINDPM_SHIFT);
    if (regval < 0)
        return regval;

    val->intval = IINDPM_REG[regval];

    return 0;
}

static int eta6919_set_input_current_limit(struct eta6919_state *st, int val)
{
    unsigned int regval;
    regval = bmt_find_closest_regval(IINDPM_REG, ARRAY_SIZE(IINDPM_REG), val);
    return eta_set_bits(st->client, ETACON0, ETACON0_IINDPM_MASK, ETACON0_IINDPM_SHIFT, regval);
}

static int eta6919_get_input_voltage_limit(struct eta6919_state *st,
                   union power_supply_propval *val)
{
    unsigned int regval;

    regval = eta_get_bits(st->client, ETACON6, ETACON6_VINDPM_MASK, ETACON6_VINDPM_SHIFT);
    if (regval < 0)
        return regval;

    val->intval = VINDPM_REG[regval];

    return 0;
}

static int eta6919_set_input_voltage_limit(struct eta6919_state *st, int val)
{
    unsigned int regval;
    regval = bmt_find_closest_regval(VINDPM_REG, ARRAY_SIZE(VINDPM_REG), val);
    return eta_set_bits(st->client, ETACON6, ETACON6_VINDPM_MASK, ETACON6_VINDPM_SHIFT, regval);
}

static int eta6918_get_current(struct eta6919_state *st,
                   union power_supply_propval *val)
{
    unsigned int regval;

    regval = eta_get_bits(st->client, ETACON2, ETACON2_ICHG_MASK, ETACON2_ICHG_SHIFT);
    if (regval < 0)
        return regval;

    val->intval = BAT_ICHG[regval];

    return 0;
}

static int eta6919_set_current(struct eta6919_state *st, int val)
{
    unsigned int regval;
    regval = bmt_find_closest_regval(BAT_ICHG, ARRAY_SIZE(BAT_ICHG), val);
    return eta_set_bits(st->client, ETACON2, ETACON2_ICHG_MASK, ETACON2_ICHG_SHIFT, regval);
}

static int eta6919_set_pre_current(struct eta6919_state *st, int val)
{
    unsigned int regval;
    regval = bmt_find_closest_regval(CS_IPRECHG, ARRAY_SIZE(CS_IPRECHG), val);
    return eta_set_bits(st->client, ETACON3, ETACON3_IPRECHG_MASK, ETACON3_IPRECHG_SHIFT, regval);
}

static int eta6919_set_term_current(struct eta6919_state *st, int val)
{
    unsigned int regval;
    regval = bmt_find_closest_regval(CS_ITERM, ARRAY_SIZE(CS_ITERM), val);
    return eta_set_bits(st->client, ETACON3, ETACON3_ITERM_MASK, ETACON3_ITERM_SHIFT, regval);
}

static int eta6919_get_voltage(struct eta6919_state *st,
                   union power_supply_propval *val)
{
    unsigned int regval;

    regval = eta_get_bits(st->client, ETACON4, ETACON4_VREG_MASK, ETACON4_VREG_SHIFT);
    if (regval < 0)
        return regval;

    val->intval = BAT_VREG[regval];

    return 0;
}

static int eta6919_set_voltage(struct eta6919_state *st, int val)
{
    unsigned int regval;
    regval = bmt_find_closest_regval(BAT_VREG, ARRAY_SIZE(BAT_VREG), val);
    return eta_set_bits(st->client, ETACON4, ETACON4_VREG_MASK, ETACON4_VREG_SHIFT, regval);
}

static int eta6919_set_mode(struct eta6919_state *st, enum eta6919_mode mode)
{
    int enable_chg = 0, enable_otg = 0;
    switch (mode)
    {
    case ETA6919_NONE:
        break;
    case ETA6919_CHG:
        enable_chg = 1;
        break;
    case ETA6919_OTG:
        enable_otg = 1;
        break;

    default:
        printk(KERN_ERR "ETA: no support this mode(%d)\n", mode);
        return -EINVAL;
    }
    int ret;
    unsigned char regval;

    ret = eta_read_reg(st->client, ETACON1, &regval);
    if (ret < 0)
        return ret;

    regval &= ~(ETACON1_CHG_CONFIG_MASK << ETACON1_CHG_CONFIG_SHIFT);
    regval |= (enable_chg << ETACON1_CHG_CONFIG_SHIFT);

    regval &= ~(ETACON1_OTG_CONFIG_MASK << ETACON1_OTG_CONFIG_SHIFT);
    regval |= (enable_otg << ETACON1_OTG_CONFIG_SHIFT);

    ret = eta_write_reg(st->client, ETACON1, regval);
    if (ret)
        printk(KERN_ERR "ETA: failed to set mode\n");

    eta6919.pdata.mode = mode;

    return ret;
}

static void eta6919_wdt_reset(struct eta6919_state *st)
{
    eta_set_bits(st->client, ETACON1, ETACON1_WDT_RST_MASK, ETACON1_WDT_RST_SHIFT, 1);
    eta_set_bits(st->client, ETACON5, ETACON5_WATCHDOG_MASK, ETACON5_WATCHDOG_SHIFT, 0);
}

static int eta6919_pmu_init(struct eta6919_state *st)
{
    eta_set_bits(st->client, ETACONB, ETACONB_REG_RST_MASK, ETACONB_REG_RST_SHIFT, 1);
    eta6919_wdt_reset(st);
    eta6919_set_mode(st, eta6919.pdata.mode);

    eta_set_bits(st->client, ETACON0, ETACON0_EN_HIZ_MASK, ETACON0_EN_HIZ_SHIFT, 0);
    eta_set_bits(st->client, ETACON5, ETACON5_EN_TERM_MASK, ETACON5_EN_TERM_SHIFT, 1);
    eta_set_bits(st->client, ETACON5, ETACON5_WATCHDOG_MASK, ETACON5_WATCHDOG_SHIFT, 0);
    eta_get_bits(st->client, ETACON9, ETACON9_WATCHDOG_FAULT_MASK, ETACON9_WATCHDOG_FAULT_SHIFT);

    eta6919_set_input_current_limit(st, st->pdata.input_current);
    eta6919_set_input_voltage_limit(st, st->pdata.input_voltage);
    eta6919_set_current(st, st->pdata.charge_current);
    eta6919_set_voltage(st, st->pdata.charge_voltage);
    eta6919_set_pre_current(st, st->pdata.charge_pre_current);
    eta6919_set_term_current(st, st->pdata.charge_term_current);

    return 0;
}

static int eta6919_get_status(struct eta6919_state *st,
                      union power_supply_propval *val)
{
    int ret;

    ret = eta_get_bits(st->client, ETACON8, ETACON8_CHRG_STAT_MASK, ETACON8_CHRG_STAT_SHIFT);
    if (ret < 0)
        return ret;

    switch (ret) {
    case ETA6919_CHG_NONE:
        if (eta_get_bits(st->client, ETACON1, ETACON1_OTG_CONFIG_MASK, ETACON1_OTG_CONFIG_SHIFT))
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
        else
            val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
        break;
    case ETA6919_CHG_TRICKLE:
    case ETA6919_CHG_FAST:
        val->intval = POWER_SUPPLY_STATUS_CHARGING;
        break;
    case ETA6919_CHG_COMPLETE:
        val->intval = POWER_SUPPLY_STATUS_FULL;
        break;
    default:
        val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
    }

    return 0;
}

static int eta6919_set_status(struct eta6919_state *st, int val)
{
    int ret;
    switch (val) {
    case POWER_SUPPLY_STATUS_NOT_CHARGING:
        ret = eta6919_set_mode(st, ETA6919_NONE);
        break;
    case POWER_SUPPLY_STATUS_CHARGING:
        ret = eta6919_set_mode(st, ETA6919_CHG);
        break;
    case POWER_SUPPLY_STATUS_DISCHARGING:
        ret = eta6919_set_mode(st, ETA6919_OTG);
        break;
    default:
        return -EINVAL;
    }

    return ret;
}

static int eta6919_get_chg_type(struct eta6919_state *st,
                union power_supply_propval *val)
{
    int ret;

    ret = eta_get_bits(st->client, ETACON8, ETACON8_CHRG_STAT_MASK, ETACON8_CHRG_STAT_SHIFT);
    if (ret < 0)
        return ret;

    switch (ret) {
    case ETA6919_CHG_NONE:
    case ETA6919_CHG_COMPLETE:
        val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
        break;
    case ETA6919_CHG_TRICKLE:
        val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
        break;
    case ETA6919_CHG_FAST:
        val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
        break;
    default:
        val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
    }

    return 0;
}

static int eta6919_get_property(struct power_supply *psy,
                    enum power_supply_property psp,
                    union power_supply_propval *val)
{
    struct eta6919_state *st = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        return eta6919_get_status(st, val);
    case POWER_SUPPLY_PROP_CHARGE_TYPE:
        return eta6919_get_chg_type(st, val);
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        return eta6919_get_input_current_limit(st, val);
    case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
        return eta6919_get_input_voltage_limit(st, val);
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        return eta6918_get_current(st, val);
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
        return eta6919_get_voltage(st, val);
    default:
        return -EINVAL;
    }

    return 0;
}

static int eta6919_set_property(struct power_supply *psy,
                    enum power_supply_property psp,
                    const union power_supply_propval *val)
{
    struct eta6919_state *st = power_supply_get_drvdata(psy);

    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        return eta6919_set_status(st, val->intval);
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
        return eta6919_set_input_current_limit(st, val->intval);
    case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
        return eta6919_set_input_voltage_limit(st, val->intval);
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        return eta6919_set_current(st, val->intval);
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
        return eta6919_set_voltage(st, val->intval);
    default:
        return -EPERM;
    }

    return 0;
}

static enum power_supply_property eta6919_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
    POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
    POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
};

static int eta6919_prop_writeable(struct power_supply *psy,
                         enum power_supply_property psp)
{
    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
    case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
    case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
        return 1;
    default:
        break;
    }

    return 0;
}

static const struct power_supply_desc eta6919_desc = {
    .name = "eta6919",
    .type = POWER_SUPPLY_TYPE_UNKNOWN,
    .get_property = eta6919_get_property,
    .set_property = eta6919_set_property,
    .property_is_writeable = eta6919_prop_writeable,
    .properties = eta6919_props,
    .num_properties = ARRAY_SIZE(eta6919_props),
};

static irqreturn_t eta6919_irq_handle(int irq, void *dev_id)
{
    struct eta6919_state *st = dev_id;
    unsigned char status0, status1, status2, stat;

    eta_read_reg(st->client, ETACON8, &status0);
    eta_read_reg(st->client, ETACON9, &status1);eta_read_reg(st->client, ETACON9, &stat);
    eta_read_reg(st->client, ETACONA, &status2);eta_read_reg(st->client, ETACONA, &stat);

    if (eta_get_bit_field(status0, ETACON8_CHRG_STAT_MASK, ETACON8_CHRG_STAT_SHIFT) == ETA6919_CHG_COMPLETE)
        printk(KERN_ERR "ETA: Charge Termination\n");

    if (!eta_get_bit_field(status0, ETACON8_PG_STAT_MASK, ETACON8_PG_STAT_SHIFT))
        printk(KERN_ERR "ETA: power not good\n");

    if (eta_get_bit_field(status1, ETACON9_WATCHDOG_FAULT_MASK, ETACON9_WATCHDOG_FAULT_SHIFT))
        printk(KERN_ERR "ETA: failed to start watchdog timer\n");

    if (eta_get_bit_field(status1, ETACON9_BOOST_FAULT_MASK, ETACON9_BOOST_FAULT_SHIFT))
        printk(KERN_ERR "ETA: failed to start boost function\n");

    if (eta_get_bit_field(status1, ETACON9_CHRG_FAULT_MASK, ETACON9_CHRG_FAULT_SHIFT))
        printk(KERN_ERR "ETA: failed to charging\n");

    if (eta_get_bit_field(status1, ETACON9_BAT_FAULT_MASK, ETACON9_BAT_FAULT_SHIFT))
        printk(KERN_ERR "ETA: battery overvoltage\n");

    if (eta_get_bit_field(status1, ETACON9_NTC_FAULT_MASK, ETACON9_NTC_FAULT_SHIFT))
        printk(KERN_ERR "ETA: temperature fault\n");

    if (eta_get_bit_field(status1, ETACONA_VINDPM_INT_MASK_MASK, ETACONA_VINDPM_INT_MASK_SHIFT))
        printk(KERN_ERR "ETA: the voltage falls below Vindpm, in Vindpm regulation\n");

    if (eta_get_bit_field(status1, ETACONA_IINDPM_INT_MASK_MASK, ETACONA_IINDPM_INT_MASK_SHIFT))
        printk(KERN_ERR "ETA: the current exceeds Iindpm, in Iindpm regulation\n");

    return IRQ_HANDLED;
}

static int eta6919_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int ret = 0;
    struct eta6919_state *st = &eta6919;
    struct power_supply_config psy_cfg = {};

    i2c_set_clientdata(client, st);
    psy_cfg.drv_data = st;

    ret = m_gpio_request(st->pdata.int_gpio, GPIO_INPUT, "gpio_pmu_int");
    if (ret) {
        printk(KERN_ERR "ETA: failed to request int pin!\n");
        return ret;
    }

    ret = eta6919_pmu_init(st);
    if (ret < 0) {
        printk(KERN_ERR "ETA: failed to init eta6919\n");
        goto int_err;
    }

    st->psy = devm_power_supply_register(&client->dev, &eta6919_desc,
                              &psy_cfg);
    if (IS_ERR(st->psy)) {
        ret = PTR_ERR(st->psy);
        printk(KERN_ERR "ETA: failed to register power supply\n");
        goto init_err;
    }

    if (st->pdata.int_gpio >= 0) {
        st->pdata.irq = gpio_to_irq(st->pdata.int_gpio);
        ret = request_irq(st->pdata.irq, eta6919_irq_handle, IRQ_TYPE_EDGE_FALLING, client->name, st);
        if (ret) {
            printk(KERN_ERR "ETA: failed to request_irq\n");
            goto register_err;
        }
    }

    return 0;
register_err:
init_err:
int_err:
    m_gpio_free(st->pdata.int_gpio);
    return ret;
}

static const struct i2c_device_id eta_id_table[] = {
    { "eta6919", 0 },
    {}
};

static struct i2c_driver eta_driver = {
    .driver = {
        .name = "eta6919",
        .owner = THIS_MODULE,
    },
    .probe = eta6919_probe,
    .id_table = eta_id_table,
};

static struct i2c_board_info eta_device = {
    .type               = "eta6919",
    .addr               = ETA_I2C_ADDR,
};

static int __init eta6919_init(void)
{
    int ret;
    struct eta6919_state *st = &eta6919;

    st->client = i2c_register_device(&eta_device, st->pdata.i2c_bus_num);
    if (!st->client) {
        printk(KERN_ERR "ETA: failed to register i2c device\n");
        return -1;
    }

    ret = i2c_add_driver(&eta_driver);
    if (ret) {
        printk(KERN_ERR "ETA: failed to register i2c driver\n");
        goto err_i2c_add_driver;
    }

    return 0;

err_i2c_add_driver:
    i2c_unregister_device(st->client);

    return ret;
}
module_init(eta6919_init);

static void __exit eta6919_exit(void)
{
    struct eta6919_state *st = &eta6919;
    i2c_unregister_device(st->client);

    i2c_del_driver(&eta_driver);
}
module_exit(eta6919_exit);

MODULE_DESCRIPTION("JZ PMU ETA6919");
MODULE_LICENSE("GPL");
