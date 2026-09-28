#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <libhardware2/adc.h>

#define CPM_IOBASE                      0x10000000
#define CPM_EXCLK_DS                    (CPM_IOBASE + 0xE0)
#define DVP_VOLTAGE_BIT                 30
#define SD_VOLTAGE_BIT                  31

#define SOC_GPIO_VOLTAGE_3_3V           0
#define SOC_GPIO_VOLTAGE_1_8V           1

#define ADC_VREF_VOLTAGE                1800

static const char *prg_name;

static int parse_uint(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        exit(-1);
    }

    *value = v;
    return 1;
}

int gpio_get_voltage(unsigned int adc_ch, int R0, int R1)
{
    int adc_voltage, real_voltage;
    char adc_path[32];
    int count = 1000;

    snprintf(adc_path, 31, "/dev/jz_adc_aux_%d", adc_ch);
    while (access("/dev/jz_adc_seq", F_OK) && access(adc_path, F_OK)) {
        usleep(100);
        if (count-- < 0)
            return -1;
    }

    long adc_handle = adc_enable(adc_ch);

    adc_voltage = adc_get_voltage(adc_handle);
    if (adc_voltage < 0)
        return -1;

    /* 电路分压, 根据实际电路计算 */
    real_voltage = adc_voltage * (R0 + R1) / R1;

    adc_disable(adc_handle);

    return real_voltage;
}

static int cpm_set_bit(unsigned int bit, unsigned int val)
{
    int fd = open("/dev/mem", O_RDWR | O_NDELAY);
    if (fd < 0) {
        printf("open /dev/mem failed !\n");
        return -1;
    }

    void *mmap_addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CPM_IOBASE);
    volatile unsigned int *exclk_ds = (volatile unsigned int *)(mmap_addr + 0xE0);
    if (val)
        *exclk_ds |= (1 << bit);
    else
        *exclk_ds &= ~(1 << bit);

    munmap(mmap_addr, 4096);
    close(fd);

    return 0;
}

void dvp_gpio_voltage_set(int voltage)
{
    assert(voltage >= 0);

    if (voltage == SOC_GPIO_VOLTAGE_3_3V) {
        cpm_set_bit(DVP_VOLTAGE_BIT, 0);
        printf("gpio: VDDIO_CIM = 3.3V\n");
    }
    else if (voltage == SOC_GPIO_VOLTAGE_1_8V) {
        cpm_set_bit(DVP_VOLTAGE_BIT, 1);
        printf("gpio: VDDIO_CIM = 1.8V\n");
    }
}

void sd_gpio_voltage_set(int voltage)
{
    assert(voltage >= 0);

    if (voltage == SOC_GPIO_VOLTAGE_3_3V) {
        cpm_set_bit(SD_VOLTAGE_BIT, 0);
        printf("gpio: VDDIO_SD = 3.3V\n");
    }
    else if (voltage == SOC_GPIO_VOLTAGE_1_8V) {
        cpm_set_bit(SD_VOLTAGE_BIT, 1);
        printf("gpio: VDDIO_SD = 1.8V\n");
    }
}

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help          : show help info\n");
    fprintf(stderr, "    dvp_adc_ch=        : VDDIO_CIM hardware connected adc channel\n");
    fprintf(stderr, "    dvp_R0=            : as shown in the figure\n");
    fprintf(stderr, "    dvp_R1=            : as shown in the figure\n");
    fprintf(stderr, "    sd_adc_ch=         : VDDIO_SD hardware connected adc channel\n");
    fprintf(stderr, "    sd_R0=             : as shown in the figure\n");
    fprintf(stderr, "    sd_R1=             : as shown in the figure\n");
    fprintf(stderr, "    Example: %s dvp_adc_ch=12 dvp_R0=100000 dvp_R1=100000\n", prg_name);
    fprintf(stderr, "    Example: %s sd_adc_ch=13 sd_R0=100000 sd_R1=100000\n", prg_name);
    fprintf(stderr, "    Example: %s dvp_adc_ch=12 dvp_R0=100000 dvp_R1=100000 sd_adc_ch=13 sd_R0=100000 sd_R1=100000\n", prg_name);
    fprintf(stderr, "          +++   vddio_cim(sd) (3.3v or 1.8v)\n");
    fprintf(stderr, "           |                            \n");
    fprintf(stderr, "           |                            \n");
    fprintf(stderr, "          [ ]                           \n");
    fprintf(stderr, "          [ ]   R0                      \n");
    fprintf(stderr, "           |                            \n");
    fprintf(stderr, "adc _______|                            \n");
    fprintf(stderr, "           |                            \n");
    fprintf(stderr, "          [ ]  R1                       \n");
    fprintf(stderr, "          [ ]                           \n");
    fprintf(stderr, "           |                            \n");
    fprintf(stderr, "           |                            \n");
    fprintf(stderr, "          ---   GND                     \n");
    fprintf(stderr, "    ADC_CIM(SD) = VDDIO_CIM(SD) * R1 / (R0 + R1)\n");
    fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
    int dvp_adc_ch = -1;
    unsigned int dvp_R0 = 100 * 1000;
    unsigned int dvp_R1 = 100 * 1000;
    int sd_adc_ch = -1;
    unsigned int sd_R0 = 100 * 1000;
    unsigned int sd_R1 = 100 * 1000;

    prg_name = argv[0];

    if ((argc < 2) || (!strcmp(argv[1], "-h")) || (!strcmp(argv[1], "--help"))) {
        usage(0);
        return 0;
    }

    int i;
    for (i = 1; i < argc; i++) {
        if (parse_uint(argv[i], "dvp_adc_ch=", &dvp_adc_ch, 10))
            continue;
        if (parse_uint(argv[i], "dvp_R0=", &dvp_R0, 10))
            continue;
        if (parse_uint(argv[i], "dvp_R1=", &dvp_R1, 10))
            continue;
        if (parse_uint(argv[i], "sd_adc_ch=", &sd_adc_ch, 10))
            continue;
        if (parse_uint(argv[i], "sd_R0=", &sd_R0, 10))
            continue;
        if (parse_uint(argv[i], "sd_R1=", &sd_R1, 10))
            continue;
    }

    if (dvp_adc_ch >= 0) {
        /* 基于 SADC 读出 VDDIO_CIM 接入电压, 然后进行配置 */
        unsigned int dvp_voltage = gpio_get_voltage(dvp_adc_ch, dvp_R0, dvp_R1);

        /* 单位mV */
        if (dvp_voltage >= 1600 && dvp_voltage <= 1980)
            dvp_gpio_voltage_set(SOC_GPIO_VOLTAGE_1_8V);
        else if (dvp_voltage >= 3000 && dvp_voltage <= 3630)
            dvp_gpio_voltage_set(SOC_GPIO_VOLTAGE_3_3V);
        else {
            printf("gpio dvp voltage set failed, voltage: %d\n", dvp_voltage);
        }
    }

    if (sd_adc_ch >= 0) {
        /* 基于 SADC 读出 VDDIO_SD 接入电压, 然后进行配置 */
        unsigned int sd_voltage = gpio_get_voltage(sd_adc_ch, sd_R0, sd_R1);

        /* 单位mV */
        if (sd_voltage >= 1600 && sd_voltage <= 1980)
            sd_gpio_voltage_set(SOC_GPIO_VOLTAGE_1_8V);
        else if (sd_voltage >= 3000 && sd_voltage <= 3630)
            sd_gpio_voltage_set(SOC_GPIO_VOLTAGE_3_3V);
        else {
            printf("gpio sd voltage set failed, voltage: %d\n", sd_voltage);
        }
    }

    return 0;
}
