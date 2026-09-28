#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libhardware2/adc.h>

static char *command;
static void usage(int status)
{
    printf("Usage1: \t%s <operation> <channel>\n", command);
    printf("Example1:\n");
    printf("\t%s get_value 0\n", command);
    printf("\t%s get_voltage 0\n\n", command);
    printf("Usage2: \t%s <operation> [ref_voltage]\n", command);
    printf("Example2:\n");
    printf("\t%s set_vref 1200\n", command);
    printf("\t%s get_vref\n", command);

    printf("Usage3:%s seq1_enable\n \
        <channels=0,1,2,3...>       input channels in sequence and convert them in order\n \
        [enable_continue=1]         enable continue convert, awd mode suggest enable\n \
        [continus_clk_div=30000]    continus convert clk: adc_clk(30M)/continus_clk_div; max=2^24\n \
        [delay_clk_div=30000]       convert delay clk: adc_clk(30M)/delay_clk_div; max=2^24\n \
        [enable_channel_num=1]      enable save channel num, save in [12:15] bits of read data\n \
        [is_enable_irq=1]           enable irq to save convert data, usually awd mode need to disable this\n \
        [continus_delay=1000]       continus convert delay time, unit is (adc_clk(30M)/continus_clk_div)\n", command);
    printf("Example3:\n");
    printf("\t%s seq1_enable channels=0,1,2,3,0,1,2,3\n", command);
    printf("\t%s seq1_enable channels=0,1,2,3,4,6,8,9,10,12,13,14 enable_continue=0\n", command);

    printf("Usage4:%s seq1_disable\n", command);
    printf("Example4:\n");
    printf("\t%s seq1_disable\n", command);

    printf("Usage5:%s seq1_read [timeout=3000]\n", command);
    printf("Example5:\n");
    printf("\t%s seq1_read\n", command);

    printf("Usage6:%s awd_enable <channel> [low_threshold=[0,4095]] [high_threshold=[0,4095]]\n \
        low_threshold:      adc value < low_threshold, save awd low flags, when < 0 disable\n \
        high_threshold:     adc value > high_threshold, save awd high flags, when < 0 disable\n", command);
    printf("Example6:\n");
    printf("\t%s awd_enable 1 low_threshold=400 high_threshold=2000\n", command);
    printf("\t%s awd_enable 2 low_threshold=-1 high_threshold=2000\n", command);
    printf("\t%s awd_enable 5 low_threshold=400 high_threshold=-1\n", command);

    printf("Usage7:%s awd_disable <channel>\n", command);
    printf("Example7:\n");
    printf("\t%s awd_disable 1\n", command);

    printf("Usage8:%s awd_wait [timeout=3000]\n", command);
    printf("Example8:\n");
    printf("\t%s awd_wait\n", command);

    printf("Usage9:%s [-h/--help]\n", command);
    printf("Example9:\n");
    printf("\t%s --help\n", command);

    exit(status);
}

static int parse_int(const char *str, const char *prefix, int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtol(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;

    return 1;
}

static int parse_array(const char *str, const char *prefix, int byte, char *array, int array_len)
{
    int max_len = strlen(str);
    int pre_len = strlen(prefix);
    int index = 0;
    unsigned char *m_str = (void *)malloc(max_len - pre_len);
    memcpy(m_str, str+pre_len, max_len - pre_len);

    if (strncmp(str, prefix, pre_len))
        return -1;

    unsigned char *temp = strtok(m_str, ",");

    while (temp != NULL) {
        if (index < array_len)
            array[index * byte] = atoi(temp);

        index++;
        temp = strtok(NULL, ",");
    }

    free(m_str);

    if (index >= array_len)
        fprintf(stderr, "%s:array number > array len\n", __func__);

    return index;
}

static inline void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
}

int main(int argc, char **argv)
{
    int ret;
    int data;
    int vref;
    long handle;
    int channel;
    int i;
    command = argv[0];

    if (argc < 2)
        usage(-1);

    if (strcmp(argv[1], "set_vref") == 0) {
        if (argc != 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &vref);
        if (ret != 1 || vref < 0)
            usage(-1);

        handle = adc_enable(0);
        if (handle < 0)
            return -1;

        adc_set_vref(handle, vref);

        adc_disable(handle);

        return 0;
    }

    if (strcmp(argv[1], "get_value") == 0) {
        if (argc != 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel);
        if (ret != 1 || channel < 0)
            usage(-1);

        handle = adc_enable(channel);
        if (handle < 0)
            return -1;

        data = adc_get_value(handle);
        if (data < 0) {
            printf("adc read data failure\n");
            adc_disable(handle);
            return -1;
        }

        adc_disable(handle);

        printf("channel %d:adc sample original value = %d\n", channel, data);
        return 0;
    }

    if (strcmp(argv[1], "get_voltage") == 0) {
        if (argc != 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel);
        if (ret != 1 || channel < 0)
            usage(-1);

        handle = adc_enable(channel);
        if (handle < 0)
            return -1;

        data = adc_get_voltage(handle);
        if (data < 0) {
            printf("adc read data failure\n");
            adc_disable(handle);
            return -1;
        }

        adc_disable(handle);

        printf("channel %d:adc sample voltage = %d\n", channel, data);
        return 0;
    }

    if (strcmp(argv[1], "get_vref") == 0) {
        if (argc != 2)
            usage(-1);

        handle = adc_enable(0);
        if (handle < 0)
            return -1;

        data = adc_get_vref(handle);

        adc_disable(handle);

        printf("channel:adc vref = %d\n", data);
        return 0;
    }

    if (strcmp(argv[1], "seq1_enable") == 0) {
        struct adc_seq1_config adc_seq1 = {
            .channel_cnt = 0,
            .channel_delays = {10, 10, 10, 10,
                10, 10, 10, 10, 10, 10, 10, 10},
            .continus_clk_div = 30000,
            .delay_clk_div = 30000,
            .enable_channel_num = 1,
            .group_cnt = 1,
            .group_delays = {1000},
            .is_enable_irq = 1,
            .trigger = adc_trigger_software,
        };

        if (argc < 3)
            usage(-1);

        for (i = 2; i < argc; i++) {
            if (parse_int(argv[i], "enable_continue=", (int *)&adc_seq1.group_cnt, 10))
                continue;

            if ((ret = parse_array(argv[i], "channels=", sizeof(unsigned char), adc_seq1.channels, sizeof(adc_seq1.channels)/sizeof(adc_seq1.channels[0]))) > 0) {
                if (ret > sizeof(adc_seq1.channels)/sizeof(adc_seq1.channels[0]))
                    adc_seq1.channel_cnt = sizeof(adc_seq1.channels)/sizeof(adc_seq1.channels[0]);
                else
                    adc_seq1.channel_cnt = ret;
                continue;
            }

            if (parse_int(argv[i], "continus_clk_div=", &adc_seq1.continus_clk_div, 10) > 0) {
                continue;
            }

            if (parse_int(argv[i], "delay_clk_div=", &adc_seq1.delay_clk_div, 10) > 0) {
                continue;
            }

            if (parse_int(argv[i], "enable_channel_num=", (int *)&adc_seq1.enable_channel_num, 10) > 0) {
                continue;
            }

            if (parse_int(argv[i], "is_enable_irq=", (int *)&adc_seq1.is_enable_irq, 10) > 0) {
                continue;
            }

            if (parse_int(argv[i], "continus_delay=", (int *)&adc_seq1.group_delays[0], 10) > 0) {
                continue;
            }

            error_arg(argv[i]);
        }

        if (adc_seq1.group_cnt) {
            adc_seq1.groups[0] = adc_seq1.channel_cnt;
        }

        handle = adc_open();
        if (handle < 0)
            return -1;

        adc_seq1_enable(handle, adc_seq1);

        adc_close(handle);

        return 0;
    }

    if (strcmp(argv[1], "seq1_disable") == 0) {
        if (argc < 2)
            usage(-1);

        handle = adc_open();
        if (handle < 0)
            return -1;

        struct adc_seq1_config adc_seq1;
        adc_seq1_disable(handle, adc_seq1);

        adc_close(handle);

        return 0;
    }

    if (strcmp(argv[1], "seq1_read") == 0) {
        int timeout = 3000;
        if (argc < 2)
            usage(-1);

        for (i = 2; i < argc; i++) {
            if (parse_int(argv[i], "timeout=", &timeout, 10))
                continue;

            error_arg(argv[i]);
        }

        handle = adc_open();
        if (handle < 0)
            return -1;

        unsigned short seq1_buf[16];
        int count = adc_seq1_read_timeout(handle, timeout, seq1_buf);

        if (count > 0) {
            for (i = 0; i < count; i++)
                printf("[%d:%d] ", seq1_buf[i] >> 12, seq1_buf[i] & 0xfff);
            printf("\n");
        }

        adc_close(handle);

        return 0;
    }

    if (strcmp(argv[1], "awd_enable") == 0) {
        int low_threshold = -1;
        int high_threshold = -1;
        if (argc < 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel);
        if (ret != 1 || channel < 0)
            usage(-1);

        for (i = 3; i < argc; i++) {
            if (parse_int(argv[i], "low_threshold=", &low_threshold, 10))
                continue;

            if (parse_int(argv[i], "high_threshold=", &high_threshold, 10))
                continue;

            error_arg(argv[i]);
        }

        handle = adc_open();
        if (handle < 0)
            return -1;

        adc_awd_enable(handle, channel, low_threshold, high_threshold);

        adc_close(handle);

        return 0;
    }

    if (strcmp(argv[1], "awd_disable") == 0) {
        if (argc < 3)
            usage(-1);

        ret = sscanf(argv[2], "%d", &channel);
        if (ret != 1 || channel < 0)
            usage(-1);

        handle = adc_open();
        if (handle < 0)
            return -1;

        adc_awd_disable(handle);

        adc_close(handle);

        return 0;
    }

    if (strcmp(argv[1], "awd_wait") == 0) {
        int timeout = 3000;
        unsigned short awd_low_flags, awd_high_flags;
        if (argc < 2)
            usage(-1);

        for (i = 2; i < argc; i++) {
            if (parse_int(argv[i], "timeout=", &timeout, 10))
                continue;

            error_arg(argv[i]);
        }

        handle = adc_open();
        if (handle < 0)
            return -1;

        adc_awd_read_timeout(handle, timeout, &awd_low_flags, &awd_high_flags);

        if (awd_low_flags) {
            for (i = 0; i < 16; i++) {
                if (awd_low_flags & (1 << i))
                    printf("adc channel %d watch dog low threshold trigger\n", i);
            }
        }

        if (awd_high_flags) {
            for (i = 0; i < 16; i++) {
                if (awd_high_flags & (1 << i))
                    printf("adc channel %d watch dog high threshold trigger\n", i);
            }
        }

        adc_close(handle);

        return 0;
    }

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
        usage((argc != 2) ? -1: 0);

    usage(-1);
    return -1;
}