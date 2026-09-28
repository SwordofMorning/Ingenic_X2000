#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <libhardware2/pwm.h>

#define MAX_DMA_DATA    16

struct pwm_handle {
    int fd;
    int id;
};

static char *command;
static int usage(int status)
{
    fprintf(stderr,"Usage1:%s config <gpio> <freq=value> <max_level=value> [active_level=value] [accuracy_priority=freq(levels)]\n", command);
    fprintf(stderr,"Example1:\n");
    fprintf(stderr,"\t%s config pc11 freq=1000000 max_level=300 active_level=1 accuracy_priority=freq\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage2:%s set_level <gpio> <level>\n", command);
    fprintf(stderr,"Example2:\n");
    fprintf(stderr,"\t%s set_level pc11 100\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage3:%s disable <gpio>\n", command);
    fprintf(stderr,"Example3:\n");
    fprintf(stderr,"\t%s disable pc11\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage4:%s [-h/--help]\n", command);
    fprintf(stderr,"Example4:\n");
    fprintf(stderr,"\t%s --help\n", command);

    fprintf(stderr, "\n");
    fprintf(stderr,"Usage1:%s dma_init <gpio> [idle_level=value] [start_level=value]\n", command);
    fprintf(stderr,"Example1:\n");
    fprintf(stderr,"\t%s dma_init pc8 idle_level=1 start_level=0\n", command);
    fprintf(stderr,"Usage2:%s dma_update <gpio> <data_high> <data_low> ......\n", command);
    fprintf(stderr,"Example2:\n");
    fprintf(stderr,"\t%s dma_update pc8 1000 1000 2000 2000 3000 3000 4000 4000\n", command);
    fprintf(stderr,"Usage3:%s enable_dma_loop <gpio> <data_high> <data_low> ......\n", command);
    fprintf(stderr,"Example3:\n");
    fprintf(stderr,"\t%s enable_dma_loop pc8 1000 1000 2000 2000 3000 3000 4000 4000\n", command);
    fprintf(stderr,"Usage4:%s disable_dma_loop <gpio>\n", command);
    fprintf(stderr,"Example4:\n");
    fprintf(stderr,"\t%s disable_dma_loop pc8\n", command);

    fprintf(stderr, "\n");
    fprintf(stderr,"multi-channel synchronization mode\n");
    fprintf(stderr,"Usage1:%s config <gpio> <freq=value> <max_level=value> [active_level=value] [accuracy_priority=freq(levels)]\n", command);
    fprintf(stderr,"Example1:\n");
    fprintf(stderr,"\t%s config pc00 freq=1000000 max_level=300 active_level=1 accuracy_priority=freq\n", command);
    fprintf(stderr,"\t%s config pc01 freq=1000000 max_level=300 active_level=1 accuracy_priority=freq\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage2:%s not_really_enable <gpio>\n", command);
    fprintf(stderr,"Example2:\n");
    fprintf(stderr,"\t%s not_really_enable pc00\n", command);
    fprintf(stderr,"\t%s not_really_enable pc01\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage3:%s set_level <gpio> <level>\n", command);
    fprintf(stderr,"Example3:\n");
    fprintf(stderr,"\t%s set_level pc00 100\n", command);
    fprintf(stderr,"\t%s set_level pc01 100\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage4:%s enable_channels <gpio> ..\n", command);
    fprintf(stderr,"Example4:\n");
    fprintf(stderr,"\t%s enable_channels pc00 pc01\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage5:%s not_really_disable <gpio>\n", command);
    fprintf(stderr,"Example5:\n");
    fprintf(stderr,"\t%s not_really_disable pc00\n", command);
    fprintf(stderr,"\t%s not_really_disable pc01\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage6:%s disable <gpio>\n", command);
    fprintf(stderr,"Example6:\n");
    fprintf(stderr,"\t%s disable pc00\n", command);
    fprintf(stderr,"\t%s disable pc01\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage7:%s disable_channels <gpio> ..\n", command);
    fprintf(stderr,"Example7:\n");
    fprintf(stderr,"\t%s disable_channels pc00 pc01\n", command);
    fprintf(stderr, "\n");
    fprintf(stderr,"Usage8:%s [-h/--help]\n", command);
    fprintf(stderr,"Example8:\n");
    fprintf(stderr,"\t%s --help\n", command);

    exit(status);
}

static void error_arg(const char *arg)
{
    fprintf(stderr, "error: not support this arg: %s\n", arg);
    exit(-1);
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


int main(int argc, char **argv)
{
    int ret, i;
    int level;
    int freq = 0;
    int max_level = 0;
    long pwm;
    struct pwm_config_data config;
    struct pwm_data pwm_data[MAX_DMA_DATA];
    command = argv[0];

    if (argc < 2)
        usage(-1);

    if (strcmp(argv[1], "config") == 0) {
        if (argc < 5)
            usage(-1);

        config.idle_level = 0;
        config.accuracy_priority = PWM_accuracy_freq_first;
        config.shutdown_mode = PWM_abrupt_shutdown;

        for (i = 3; i < argc; i++) {
            if (parse_int(argv[i], "freq=", &freq, 10))
                continue;

            if (parse_int(argv[i], "max_level=", &max_level, 10))
                continue;

            if (strcmp(argv[i], "active_level=1") == 0) {
                config.idle_level = 0;
                continue;
            }

            if (strcmp(argv[i], "active_level=0") == 0) {
                config.idle_level = 1;
                continue;
            }

            if (strcmp(argv[i], "accuracy_priority=freq") == 0) {
                config.accuracy_priority = PWM_accuracy_freq_first;
                continue;
            }

            if (strcmp(argv[i], "accuracy_priority=levels") == 0) {
                config.accuracy_priority = PWM_accuracy_levels_first;
                continue;
            }

            error_arg(argv[i]);
        }

        if (freq == 0 || max_level == 0) {
            fprintf(stderr,"must configure freq and levels\n");
            usage(-1);
        }

        config.freq = freq;
        config.levels = max_level;

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        if (pwm_config(pwm, &config) < 0)
            return -1;

        return 0;
    }

    if (strcmp(argv[1], "not_really_enable") == 0) {
        ret = 0;
        if (argc < 2)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        if (pwm_not_really_enable(pwm))
            return -1;
        return 0;
    }

    if (strcmp(argv[1], "not_really_disable") == 0) {
        ret = 0;
        if (argc < 2)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        if (pwm_not_really_disable(pwm))
            return -1;
        return 0;
    }

    if (strcmp(argv[1], "set_level") == 0) {
        if (argc != 4)
            usage(-1);

        ret = sscanf(argv[3], "%d", &level);
        if (ret != 1 || level < 0)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        if (pwm_set_level(pwm, level) < 0)
            return -1;

        return 0;
    }

    if (strcmp(argv[1], "enable_channels") == 0) {
        unsigned int channels = 0;
        struct pwm_handle *pwm_id;
        ret = 0;
        if (argc == 2)
            usage(-1);

        for (i = 2; i < argc; i++) {
            pwm = pwm_request(argv[i]);
            if (pwm < 0)
                return -1;
            pwm_id = (struct pwm_handle *)pwm;
            channels |= (1 << (pwm_id->id));
        }
        if (pwm_enable_channels(channels))
            return -1;
        return 0;
    }

    if (strcmp(argv[1], "disable_channels") == 0) {
        unsigned int channels = 0;
        struct pwm_handle *pwm_id;
        ret = 0;
        if (argc == 2)
            usage(-1);

        for (i = 2; i < argc; i++) {
            pwm = pwm_request(argv[i]);
            if (pwm < 0)
                return -1;
            pwm_id = (struct pwm_handle *)pwm;
            channels |= (1 << (pwm_id->id));
        }
        if (pwm_disable_channels(channels))
            return -1;
        return 0;
    }

    if (strcmp(argv[1], "disable") == 0) {
        ret = 0;
        if (argc != 3)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        pwm_release(pwm);

        return ret;
    }

    if (strcmp(argv[1], "dma_init") == 0) {
        enum pwm_idle_level idle_level;
        enum pwm_dma_start_level start_level;
        ret = 0;
        if (argc < 3)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        idle_level = PWM_idle_high;
        start_level = PWM_start_low;

        for (i = 3; i < argc; i++) {
            if (strcmp(argv[i], "idle_level=1") == 0) {
                idle_level = PWM_idle_high;
                continue;
            }

            if (strcmp(argv[i], "idle_level=0") == 0) {
                idle_level = PWM_idle_low;
                continue;
            }

            if (strcmp(argv[i], "start_level=1") == 0) {
                start_level = PWM_start_high;
                continue;
            }

            if (strcmp(argv[i], "start_level=0") == 0) {
                start_level = PWM_start_low;
                continue;
            }

            error_arg(argv[i]);
        }

        ret = pwm_dma_init(pwm, idle_level, start_level);
        if(ret <= 0)
            return ret;

        printf("pwm freq %d\n", ret);
        return ret;
    }

    if (strcmp(argv[1], "dma_update") == 0) {
        unsigned int data_count;
        if (argc < 5)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        data_count = argc - 3;
        if (data_count % 2) {
            printf("pwm dma data have high data, not have low data\n");
            return -1;
        }

        data_count  = data_count / 2;
        if (data_count > MAX_DMA_DATA)
            data_count = MAX_DMA_DATA;

        for (i = 0; i < data_count; i++) {
            pwm_data[i].high = atoi(argv[i * 2 + 3]);
            pwm_data[i].low = atoi(argv[i * 2 + 4]);
        }

        if (pwm_dma_update(pwm, pwm_data, data_count) < 0)
            return -1;

        return 0;
    }

    if (strcmp(argv[1], "enable_dma_loop") == 0) {
        unsigned int data_count;
        if (argc < 11)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        data_count = argc - 3;
        if (data_count % 2) {
            printf("pwm dma data have high data, not have low data\n");
            return -1;
        }

        if (data_count % 8) {
            printf("pwm dma data length must 4 word align\n");
            return -1;
        }

        data_count  = data_count / 2;
        if (data_count > MAX_DMA_DATA)
            data_count = MAX_DMA_DATA;

        for (i = 0; i < data_count; i++) {
            pwm_data[i].high = atoi(argv[i * 2 + 3]);
            pwm_data[i].low = atoi(argv[i * 2 + 4]);
        }

        if (pwm_dma_enable_loop(pwm, pwm_data, data_count) < 0)
            return -1;

        return 0;
    }

    if (strcmp(argv[1], "disable_dma_loop") == 0) {
        ret = 0;
        if (argc != 3)
            usage(-1);

        pwm = pwm_request(argv[2]);
        if (pwm < 0)
            return -1;

        ret = pwm_dma_disable_loop(pwm);

        return ret;
    }

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
        usage((argc != 2) ? -1: 0);

    usage(-1);
    return -1;
}
