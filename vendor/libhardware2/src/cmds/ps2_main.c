#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <linux/input.h>
#include <libhardware2/keyboard.h>
#include <libhardware2/ps2.h>

enum ps2_key_code_type {
    NORMAL_KEY_MAKE_CODE = 2,
    NORMAL_KEY_BREAK_CODE = 3,
    SPECIAL_KEY_MAKE_CODE = 4,
    SPECIAL_KEY_BREAK_CODE = 6,
    SPECIAL_KEY_ONLY_MAKE_CODE = 6,
    MAX_KEY_CODE_COUNT = 6,
};

struct ps2_scan_code {
    unsigned char m[NORMAL_KEY_MAKE_CODE];
    unsigned char b[NORMAL_KEY_BREAK_CODE];
};

struct ps2_special_key_code {
    unsigned char m[SPECIAL_KEY_MAKE_CODE];
    unsigned char b[SPECIAL_KEY_BREAK_CODE];
};

enum special_key {
    SP_KEY_PRINT,
    SP_KEY_PAUSE,
};

struct ps2_special_key_code special_key_code[] = {
    [SP_KEY_PRINT] = {.m = {0xe0,0x12,0x10,0x7c},.b = {0xe0,0xf0,0x7c,0xe0,0xf0,0x12,}},
    [SP_KEY_PAUSE] = {.m = {0xe1,0x14,0x77,0xe1},.b = {0xf0,0x14,0xf0,0x77,}},
};

static struct ps2_scan_code key_code_table[] = {
    [KEY_A] = {.m = {0x1c},.b = {0xf0,0x1c,}},
    [KEY_B] = {.m = {0x32},.b = {0xf0,0x32,}},
    [KEY_C] = {.m = {0x21},.b = {0xf0,0x21,}},
    [KEY_D] = {.m = {0x23},.b = {0xf0,0x23,}},
    [KEY_E] = {.m = {0x24},.b = {0xf0,0x24,}},
    [KEY_F] = {.m = {0x2b},.b = {0xf0,0x2b,}},
    [KEY_G] = {.m = {0x34},.b = {0xf0,0x34,}},
    [KEY_H] = {.m = {0x33},.b = {0xf0,0x33,}},
    [KEY_I] = {.m = {0x43},.b = {0xf0,0x43,}},
    [KEY_J] = {.m = {0x3b},.b = {0xf0,0x3b,}},
    [KEY_K] = {.m = {0x42},.b = {0xf0,0x42,}},
    [KEY_L] = {.m = {0x4b},.b = {0xf0,0x4b,}},
    [KEY_M] = {.m = {0x3a},.b = {0xf0,0x3a,}},
    [KEY_N] = {.m = {0x31},.b = {0xf0,0x31,}},
    [KEY_O] = {.m = {0x44},.b = {0xf0,0x44,}},
    [KEY_P] = {.m = {0x4d},.b = {0xf0,0x4d,}},
    [KEY_Q] = {.m = {0x15},.b = {0xf0,0x15,}},
    [KEY_R] = {.m = {0x2d},.b = {0xf0,0x2d,}},
    [KEY_S] = {.m = {0x1b},.b = {0xf0,0x1b,}},
    [KEY_T] = {.m = {0x2c},.b = {0xf0,0x2c,}},
    [KEY_U] = {.m = {0x3c},.b = {0xf0,0x3c,}},
    [KEY_V] = {.m = {0x2a},.b = {0xf0,0x2a,}},
    [KEY_W] = {.m = {0x1d},.b = {0xf0,0x1d,}},
    [KEY_X] = {.m = {0x22},.b = {0xf0,0x22,}},
    [KEY_Y] = {.m = {0x35},.b = {0xf0,0x35,}},
    [KEY_Z] = {.m = {0x1a},.b = {0xf0,0x1a,}},
    [KEY_0] = {.m = {0x45},.b = {0xf0,0x45,}},
    [KEY_1] = {.m = {0x16},.b = {0xf0,0x16,}},
    [KEY_2] = {.m = {0x1e},.b = {0xf0,0x1e,}},
    [KEY_3] = {.m = {0x26},.b = {0xf0,0x26,}},
    [KEY_4] = {.m = {0x25},.b = {0xf0,0x25,}},
    [KEY_5] = {.m = {0x2e},.b = {0xf0,0x2e,}},
    [KEY_6] = {.m = {0x36},.b = {0xf0,0x36,}},
    [KEY_7] = {.m = {0x3d},.b = {0xf0,0x3d,}},
    [KEY_8] = {.m = {0x3e},.b = {0xf0,0x3e,}},
    [KEY_9] = {.m = {0x46},.b = {0xf0,0x46,}},
    [KEY_GRAVE] = {.m = {0x0e},.b = {0xf0,0x0e,}},    /* ` */
    [KEY_MINUS] = {.m = {0x4e},.b = {0xf0,0x4e,}},
    [KEY_EQUAL] = {.m = {0x55},.b = {0xf0,0x55,}},
    [KEY_BACKSLASH] = {.m = {0x5d},.b = {0xf0,0x5d,}},
    [KEY_RIGHTBRACE] = {.m = {0x5b},.b = {0xf0,0x5b,}},
    [KEY_SEMICOLON] = {.m = {0x4c},.b = {0xf0,0x4c,}},
    [KEY_APOSTROPHE] = {.m = {0x52},.b = {0xf0,0x52,}},
    [KEY_KPCOMMA] = {.m = {0x41},.b = {0xf0,0x41,}},
    [KEY_DOT] = {.m = {0x49},.b = {0xf0,0x49,}},
    [KEY_SLASH] = {.m = {0x4a},.b = {0xf0,0x4a,}},
    [KEY_LEFTBRACE] = {.m = {0x54},.b = {0xf0,0x54,}},
    [KEY_BACKSPACE] = {.m = {0x66},.b = {0xf0,0x66,}},    /* BKSP */
    [KEY_SPACE] = {.m = {0x29},.b = {0xf0,0x29,}},   /* SPACE */
    [KEY_TAB] = {.m = {0x0d},.b = {0xf0,0x0d,}},    /* TAB */
    [KEY_CAPSLOCK] = {.m = {0x58},.b = {0xf0,0x58,}},   /* CAPS */
    [KEY_LEFTSHIFT] = {.m = {0x12},.b = {0xf0,0x12,}},   /* L SHIFT */
    [KEY_LEFTCTRL] = {.m = {0x14},.b = {0xf0,0x14,}},   /* L CTRL */
    [KEY_LEFTALT] = {.m = {0x11},.b = {0xf0,0x11,}},   /* L ALT */
    [KEY_RIGHTSHIFT] = {.m = {0x59},.b = {0xf0,0x59,}},   /* R SHIFT */
    [KEY_RIGHTCTRL] = {.m = {0xe0,0x14},.b = {0xe0,0xf0,0x14,}},   /* R CTRL */
    [KEY_RIGHTALT] = {.m = {0xe0,0x11},.b = {0xe0,0xf0,0x11,}},   /* R ALT */
    [KEY_ENTER] = {.m = {0x5a},.b = {0xf0,0x5a,}},   /* ENTER */
    // L GUI none
    [KEY_ESC] = {.m = {0x76},.b = {0xf0,0x76,}},   /* ESC */
    [KEY_F1] = {.m = {0x5},.b = {0xf0,0x05,}},   /* F1 */
    [KEY_F2] = {.m = {0x6},.b = {0xf0,0x06,}},   /* F2 */
    [KEY_F3] = {.m = {0x4},.b = {0xf0,0x04,}},   /* F3 */
    [KEY_F4] = {.m = {0xc},.b = {0xf0,0x0c,}},   /* F4 */
    [KEY_F5] = {.m = {0x3},.b = {0xf0,0x03,}},   /* F5 */
    [KEY_F6] = {.m = {0xb},.b = {0xf0,0x0b,}},   /* F6 */
    [KEY_F7] = {.m = {0x83},.b = {0xf0,0x83,}},  /* F7 */
    [KEY_F8] = {.m = {0x0a},.b = {0xf0,0x0a,}},  /* F8 */
    [KEY_F9] = {.m = {0x01},.b = {0xf0,0x01,}},  /* F9 */
    [KEY_F10] = {.m = {0x09},.b = {0xf0,0x09,}},  /* F10 */
    [KEY_F11] = {.m = {0x78},.b = {0xf0,0x78,}},  /* F11 */
    [KEY_F12] = {.m = {0x07},.b = {0xf0,0x07,}},  /* F12 */
    [KEY_INSERT] = {.m = {0xe0,0x70},.b = {0xe0,0xf0,0x70,}},   /* INS */
    [KEY_HOME] = {.m = {0xe0,0x6c},.b = {0xe0,0xe0,0x6c,}},   /* HOME */
    [KEY_PAGEUP] = {.m = {0xe0,0x7d},.b = {0xe0,0xe0,0x7d,}},   /* PAGE UP */
    [KEY_DELETE] = {.m = {0xe0,0x71},.b = {0xe0,0xe0,0x71,}},   /* DELETE */
    [KEY_END] = {.m = {0xe0,0x69},.b = {0xe0,0xe0,0x69,}},   /* END */
    [KEY_PAGEDOWN] = {.m = {0xe0,0x7a},.b = {0xe0,0xe0,0x7a,}},   /* PAGE DN */
    [KEY_UP] = {.m = {0xe0,0x75},.b = {0xe0,0xe0,0x75,}},   /* UP ARROW */
    [KEY_LEFT] = {.m = {0xe0,0x6b},.b = {0xe0,0xe0,0x6b,}},   /* LEFT ARROW */
    [KEY_DOWN] = {.m = {0xe0,0x72},.b = {0xe0,0xe0,0x72,}},   /* DOWN ARROW */
    [KEY_RIGHT] = {.m = {0xe0,0x74},.b = {0xe0,0xe0,0x74,}},   /* RIGHT ARROW */
    /* kp */
    [KEY_NUMLOCK] = {.m = {0x77},.b = {0xf0,0x77,}},  /* NUM LOCK */
    [KEY_KPSLASH] = {.m = {0xe0,0x4a},.b = {0xe0,0xf0,0x4a,}},  /* kp:/ */
    [KEY_KPASTERISK] = {.m = {0x7c},.b = {0xf0,0x7c,}},  /* kp:* */
    [KEY_KPMINUS] = {.m = {0x7b},.b = {0xf0,0x7b,}},  /* kp:- */
    [KEY_KPPLUS] = {.m = {0x79},.b = {0xf0,0x79,}},  /* kp:+ */
    [KEY_KPENTER] = {.m = {0xe0,0x5a},.b = {0xe0,0xf0,0x5a,}},    /* kp:ENTER */
    [KEY_KPDOT] = {.m = {0x71},.b = {0xf0,0x71,}},  /* kp:. */
    [KEY_KP0] = {.m = {0x70},.b = {0xf0,0x70,}},
    [KEY_KP1] = {.m = {0x69},.b = {0xf0,0x69,}},
    [KEY_KP2] = {.m = {0x72},.b = {0xf0,0x72,}},
    [KEY_KP3] = {.m = {0x7a},.b = {0xf0,0x7a,}},
    [KEY_KP4] = {.m = {0x6b},.b = {0xf0,0x6b,}},
    [KEY_KP5] = {.m = {0x73},.b = {0xf0,0x73,}},
    [KEY_KP6] = {.m = {0x74},.b = {0xf0,0x74,}},
    [KEY_KP7] = {.m = {0x6c},.b = {0xf0,0x6c,}},
    [KEY_KP8] = {.m = {0x75},.b = {0xf0,0x75,}},
    [KEY_KP9] = {.m = {0x7d},.b = {0xf0,0x7d,}},
    [KEY_SCROLLLOCK] = {.m = {0x7e},.b = {0xf0,0x7e,}},
};

static char *command;
static int usage(int status)
{
    printf("Usage1:%s <dev>\n", command);
    printf("Example1:\n");
    printf("\t%s /dev/ps2_0\n\n", command);
    printf("Usage2:%s <dev> [mode= 0:KEYBOARD, 1:custom]\n", command);
    printf("Example2:\n");
    printf("\t%s /dev/ps2_0 1\n\n", command);
    printf("Usage3:%s [-h/--help]\n", command);
    printf("Example3:\n");
    printf("\t%s --help\n", command);
    exit(status);
}

static int send_code(int fd, void *code_, enum ps2_key_code_type type)
{
    int i;
    int ret = 0;

    unsigned char *code = (unsigned char *)code_;

    for (i = 0; i < type; i++) {
        if (!code[i])
            break;

        ret = ps2_write_raw_byte(fd, code[i]);
        if (ret < 0)
            break;
    }

    return ret;
}

static void set_keyboard_leds(int fd, unsigned char ch)
{
    unsigned char ack = 0xFA;

/**
 * |Caps lock|Num lock|Scroll lock|
 *
 * 000:off|off|off|
 * 111: on| on| on|
 * 101: on|off| on|
 *
 */

    if (ch & 0x1)
        printf("ps2: set srcoll_lock led on\n");
    else
        printf("ps2: set srcoll_lock led off\n");

    if (ch & 0x2)
        printf("ps2: set num_lock led on\n");
    else
        printf("ps2: set num_lock led off\n");

    if (ch & 0x4)
        printf("ps2: set caps_lock led on\n");
    else
        printf("ps2: set caps_lock led off\n");

    ps2_write_raw_byte(fd, ack);
}

void keyboard_init(int fd)
{
    int ret = 0;
    int is_first = 1;
    unsigned char ch;

    unsigned char pre_send_ch = 0;
    unsigned char last_recv_ch = 0;

    while (1) {
        ch = 0;
        ret = ps2_read_raw_byte(fd, &ch);
        if (ret && is_first) {
            /* power on */
            ret = ps2_write_raw_byte(fd, 0xAA);
            pre_send_ch = 0xAA;
            if (!ret)
                is_first = 0;
        }

        if (!ret) {
            /* 设置LED灯 */
            if (last_recv_ch == 0xED) {
                set_keyboard_leds(fd, ch);
                if (ch == 2)
                    break;
                continue;
            }

            last_recv_ch = ch;

            /* 非命令集内容，要求重发(仅部分键盘会收到) */
            if (ch == 0x30) {
                ret = ps2_write_raw_byte(fd, 0xFE);
                pre_send_ch = 0xFE;
                continue;
            }

            /* 重发命令 */
            if (ch == 0xFE) {
                ret = ps2_write_raw_byte(fd, pre_send_ch);
                continue;
            }

            /* 应答命令 */
            ret = ps2_write_raw_byte(fd, 0xFA);
            pre_send_ch = 0xFA;

            /* 进入键盘reset模式 */
            if (ch == 0xFF) {
                /* power on */
                ret = ps2_write_raw_byte(fd, 0xAA);
                pre_send_ch = 0xAA;
                continue;
            }

            /* 读键盘ID号 */
            if (ch == 0xF2) {
                ret = ps2_write_raw_byte(fd, 0xAB);
                pre_send_ch = 0xAB;
                if (ret < 0)
                    continue;

                ret = ps2_write_raw_byte(fd, 0x83);
                pre_send_ch = 0x83;
                if (ret < 0)
                    continue;
            }

        }
    }
    printf("ps2: ps2 keyboard inited.\n");
}

static void* read_keyboard_event_thread(void *data)
{
    int fd = *(int *)data;

    int ret;
    unsigned char ch;
    unsigned char last_recv_ch = 0;
    unsigned char pre_send_ch = 0;

    while (1) {
        ch = 0;
        ret = ps2_read_raw_byte(fd, &ch);
        if (ret < 0)
            continue;

        // printf("ps2: ps2 keyboard recv ch = %x.\n", ch);

        /* 设置LED灯 */
        if (last_recv_ch == 0xED) {
            set_keyboard_leds(fd, ch);
            continue;
        }

        last_recv_ch = ch;

        /* 非命令集内容，要求重发(仅部分键盘会收到) */
        if (ch == 0x30) {
            ret = ps2_write_raw_byte(fd, 0xFE);
            pre_send_ch = 0xFE;
            continue;
        }

        /* 重发命令 */
        if (ch == 0xFE) {
            ret = ps2_write_raw_byte(fd, pre_send_ch);
            continue;
        }

        /* 应答命令 */
        ret = ps2_write_raw_byte(fd, 0xFA);
        pre_send_ch = 0xFA;

        /* 进入键盘reset模式 */
        if (ch == 0xFF) {
            /* power on */
            ret = ps2_write_raw_byte(fd, 0xAA);
            pre_send_ch = 0xAA;
            continue;
        }

        /* 读键盘ID号 */
        if (ch == 0xF2) {
            ret = ps2_write_raw_byte(fd, 0xAB);
            pre_send_ch = 0xAB;
            if (ret < 0)
                continue;

            ret = ps2_write_raw_byte(fd, 0x83);
            pre_send_ch = 0x83;
            if (ret < 0)
                continue;
        }
    }

    return NULL;
}

static void do_something_and_send_msg(int fd)
{
    /**
     * do something
     */

    unsigned char *s = "I am here\n";
    int len = strlen(s);

    int ret = ps2_write_str(fd, s, len);
    if (ret < 0)
        printf("ps2: send msg %s err!\n", s);
}

int main(int argc, char *argv[])
{
    int ret;
    struct key_event event;
    int mode = 0;
    command = argv[0];

    if (argc < 2)
        usage(-1);

    if (argc == 2) {
        if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))
            usage((argc != 2) ? -1: 0);
    }

    const char *dev = argv[1];

    if (argc > 2)
        ret = sscanf(argv[2], "%d", &mode);

    int ps2_fd = ps2_open(dev);
    if (ps2_fd < 0)
        return -1;

    int keys_fds = keys_open();
    if (keys_fds < 0)
        return -1;

    keyboard_init(ps2_fd);

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, read_keyboard_event_thread, &ps2_fd);
    pthread_attr_destroy(&attr);

    while ((ret = read_key_event(keys_fds, &event, -1)) > 0) {
        // printf("Key %d %s\n", event.key_type, event.is_press ? "press" : "release");
        if (mode) {
            if (event.is_press)
                do_something_and_send_msg(ps2_fd);
            continue;
        }

        int special_key_val = -1;

        if (event.key_type == KEY_PRINT)
            special_key_val = SP_KEY_PRINT;
        if (event.key_type == KEY_PAUSE)
            special_key_val = SP_KEY_PAUSE;

        if (special_key_val >= 0) {
            struct ps2_special_key_code *code = &special_key_code[special_key_val];

            if (event.key_type == KEY_PAUSE) {
                ret = send_code(ps2_fd, code, SPECIAL_KEY_ONLY_MAKE_CODE);
                continue;
            }

            if (event.is_press)
                ret = send_code(ps2_fd, code->m, SPECIAL_KEY_MAKE_CODE);
            else
                ret = send_code(ps2_fd, code->b, SPECIAL_KEY_BREAK_CODE);
        } else {
            struct ps2_scan_code *code = &key_code_table[event.key_type];
            if (event.is_press)
                ret = send_code(ps2_fd, code->m, NORMAL_KEY_MAKE_CODE);
            else
                ret = send_code(ps2_fd, code->b, NORMAL_KEY_BREAK_CODE);
        }
    }

    keys_close(keys_fds);

    ps2_close(ps2_fd);

    return 0;
}
