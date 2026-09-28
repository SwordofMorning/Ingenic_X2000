#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum test_type {
    alarm_irq = 1,
    update_irq,
    period_irq,
};

static void usage(int status, const char *cmd)
{
    fprintf(stderr, "Usage : \n");
    fprintf(stderr, "%s [rtcdev] [test_type] <timeout=[sec]> \n", cmd);
    fprintf(stderr, "test type : alarm or update or period\n");
    fprintf(stderr, "Example : \n");
    fprintf(stderr, "%s /dev/rtc0 test_update\n", cmd);
    fprintf(stderr, "%s /dev/rtc0 test_alarm timeout=10\n", cmd);
    exit(status);
}

static void add_alarm_time(struct rtc_time *rtc_time, int timeout)
{
    rtc_time->tm_sec += timeout;

    if (rtc_time->tm_sec >= 60) {
        rtc_time->tm_sec %= 60;
        rtc_time->tm_min++;
    }

    if (rtc_time->tm_min >= 60) {
        rtc_time->tm_min %= 60;
        rtc_time->tm_hour++;
    }

    if (rtc_time->tm_hour >= 24) {
        rtc_time->tm_hour %= 24;
        rtc_time->tm_mday++;
    }
}

static int test_alarm(int fd, int timeout)
{
    int ret;
    struct rtc_time rtc_time;
    struct timeval current;
    fd_set rd;

    FD_ZERO(&rd);
    FD_SET(fd, &rd);

    fprintf(stderr, "start read current time\n");
    ret = ioctl(fd, RTC_RD_TIME, &rtc_time);
    if (ret < 0) {
        fprintf(stderr, "read current time failed\n");
        return -1;
    }
    fprintf(stderr, "time : hour = %02d min = %02d sec = %02d\n", rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);

    add_alarm_time(&rtc_time, timeout);
    ret = ioctl(fd, RTC_ALM_SET, &rtc_time);
    if (ret < 0) {
        fprintf(stderr, "alarm set time failed\n");
        return -1;
    }
    fprintf(stderr, "start set alarm time, add %d sec\n", timeout);

    fprintf(stderr, "start read alarm time\n");
    ret = ioctl(fd, RTC_ALM_READ, &rtc_time);
    if (ret < 0) {
        fprintf(stderr, "read alarm time failed\n");
        return -1;
    }
    fprintf(stderr, "alarm time : hour = %02d min = %02d sec = %02d\n", rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);

    ret = ioctl(fd, RTC_AIE_ON, &rtc_time);
    if (ret < 0) {
        fprintf(stderr, "enable alarm failed\n");
        return -1;
    }
    fprintf(stderr, "enable alarm interrupt\n");

    gettimeofday(&current, NULL);
    fprintf(stderr, "current time is %ldsec %ldusec\n", current.tv_sec, current.tv_usec);

    ret = select(fd+1, &rd, NULL, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "can not detected alarm irq");
    }
    fprintf(stderr, "detected alarm irq\n");

    gettimeofday(&current, NULL);
    fprintf(stderr, "current time is %ldsec %ldusec\n", current.tv_sec, current.tv_usec);

    ret = ioctl(fd, RTC_AIE_OFF, &rtc_time);
    if (ret < 0) {
        fprintf(stderr, "disable alarm failed\n");
        return -1;
    }
    fprintf(stderr, "disable alarm interrupt\n");

    return 0;
}

int test_period(int fd)
{
    int ret, i;
    unsigned long freq, data;
    struct timeval current;

    ret = ioctl(fd, RTC_IRQP_READ, &freq);
    if (ret < 0) {
        fprintf(stderr, "read period irq rate failed\n");
        return -1;
    }
    fprintf(stderr, "period irq rate is %ldHZ\n", freq);

    freq = 64;
    ret = ioctl(fd, RTC_IRQP_SET, &freq);
    if (ret < 0)
        fprintf(stderr, "not support set period irq rate\n");
    else
        fprintf(stderr, "set period irq rate is %ldHZ\n", freq);

    ret = ioctl(fd, RTC_PIE_ON, &freq);
    if (ret < 0) {
        fprintf(stderr, "enable period irq failed\n");
        return -1;
    }
    fprintf(stderr, "enable period irq, counting 5 times\n");

    for (i=1; i<6; i++) {
        ret = read(fd, &data, sizeof(unsigned long));
        if (ret < 0) {
            fprintf(stderr, "cant not read period failed : %d\n", i);
        }
        gettimeofday(&current, NULL);
        fprintf(stderr, "current time is %ldsec %ldusec\n", current.tv_sec, current.tv_usec);
    }

    ret = ioctl(fd, RTC_PIE_OFF, &freq);
    if (ret < 0) {
        fprintf(stderr, "disable period irq failed\n");
        return -1;
    }
    fprintf(stderr, "disable period irq\n");

    return 0;
}

int test_update(int fd)
{
    int ret, i;
    unsigned long update, data;
    struct timeval current;

    ret = ioctl(fd, RTC_UIE_ON, &update);
    if (ret < 0) {
        fprintf(stderr, "enable update irq failed\n");
        return -1;
    }
    fprintf(stderr, "enable update irq\n");

    for (i=1; i<6; i++) {
        ret = read(fd, &data, sizeof(unsigned long));
        if (ret < 0) {
            fprintf(stderr, "cant not read period failed : %d\n", i);
        }
        gettimeofday(&current, NULL);
        fprintf(stderr, "current time is %ldsec %ldusec\n", current.tv_sec, current.tv_usec);
    }

    ret = ioctl(fd, RTC_UIE_OFF, &update);
    if (ret < 0) {
        fprintf(stderr, "disable update irq failed\n");
        return -1;
    }
    fprintf(stderr, "disable update irq\n");

    return 0;
}

int main(int argc, char **argv)
{
    int fd, ret, i;
    const char *dev = argv[1];
    static const char *cmd = NULL;
    cmd = argv[0];
    int timeout = 0;
    char *endptr = NULL;
    int test_type = 0;

    if (argc == 1)
        usage(0, cmd);

    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
        usage(argc == 2 ? 0 : -1, cmd);

    for (i = 2; i < argc; i++) {

        if (!strcmp(argv[i], "test_alarm")) {
            test_type = alarm_irq;
            i++;
            timeout = (int)strtoul(argv[i] + 8, &endptr, 0);
            if (*endptr != '\0') {
                fprintf(stderr, "The timeout value is not a number : %d%s\n", timeout, endptr);
                usage(-1, cmd);
            }
            continue;
        }

        if (!strcmp(argv[i], "test_update")) {
            test_type = update_irq;
            continue;
        }

        if (!strcmp(argv[i], "test_period")) {
            test_type = period_irq;
            continue;
        }

        usage(-1, cmd);
    }

    if (!test_type) {
        fprintf(stderr, "please set test type\n");
        return -1;
    }

    fprintf(stderr, "start test\n");
    fd = open(dev, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "open failed\n");
        return -1;
    }

    if (test_type == alarm_irq) {
        ret = test_alarm(fd, timeout);
        if (ret < 0)
            fprintf(stderr, "test alarm failed\n");
    }

    if (test_type == period_irq) {
        ret = test_period(fd);
        if (ret < 0)
            fprintf(stderr, "test period failed\n");
    }

    if (test_type == update_irq) {
        ret = test_update(fd);
        if (ret < 0)
            fprintf(stderr, "test update failed\n");
    }

    close(fd);
    fprintf(stderr, "over test\n");
    return 0;
}