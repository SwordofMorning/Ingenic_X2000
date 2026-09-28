#include <stdio.h>
#include <libhardware2/alsa.h>

const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "    -h/--help                   : show help info\n"
                    "    record                      : record pcm from device\n"
                    "    play                        : write pcm to device\n"
                    "           rate=value         : pcm sample rate, default 48000\n"
                    "           channels=value     : pcm channles, default 2\n"
                    "           format=value       : pcm format, \"s16_le, s24_le, s32_le\", default s16_le\n"
                    "           buffer_time=value  : pcm buffer time, in msecs, defualt get from device\n"
                    "           period_time=value  : pcm period time, in msecs, defualt 50\n"
                    "           time=value         : record/paly time, in secs, defualt no limit\n"
                    "           device=value       : pcm device name, \"plughw:0,0 plughw:1,0 hw:0,0 ...\" default is defualt\n"
                    "           file=value         : in/out file path, default is stdin or stdout\n"
                    "    list_ctls                   : list the controls\n"
                    "    set_ctl                     : set the ctl value\n"
                    "    get_ctl                     : get the ctl value\n"
                    "           card=value         : card name, \"hw:0 hw:1 ...\" default is default\n"
                    "           ctl=value          : ctl name, see result of list_ctls, muset be set for get_ctl, set_ctl\n"
                    "           value=value        : ctl value, see result of list_ctls, muset be set for set_ctl\n"
                    " example:\n"
    );
    fprintf(stderr, "   %s record device=hw:1,0 rate=48000 channels=1 time=10 > /tmp/xxx\n", prg_name);
    fprintf(stderr, "   %s play device=plughw:1,0 rate=48000 channels=1 file=/tmp/xxx\n", prg_name);
    fprintf(stderr, "   %s list_ctls card=hw:0\n", prg_name);
    fprintf(stderr, "   %s list_ctls card=hw:1\n", prg_name);
    fprintf(stderr, "   %s set_ctl card=hw:1 ctl=\"Master Playback Volume\" value=18\n", prg_name);
    fprintf(stderr, "   %s get_ctl card=hw:1 ctl=\"Master Playback Volume\"\n", prg_name);
    fprintf(stderr, "   %s get_ctl card=hw:1 ctl=\"Playback Mute\"\n", prg_name);
    exit(status);
}

static void cmd_error(const char *cmd, const char *info)
{
    fprintf(stderr, "error: %s: %s\n", cmd, info);
    usage(-1);
}

enum {
    cmd_read_pcm,
    cmd_write_pcm,
    cmd_list_ctls,
    cmd_read_ctl,
    cmd_write_ctl,
};

static const char *card_name = "default";
static const char *ctl_name = NULL;
static const char *device_name = "default";
static const char *file_name = NULL;
static long m_value;
static int m_value_is_set;
static unsigned int pcm_time = -1;

struct alsa_params params = {
    .rate = 48000,
    .channels = 2,
    .format = SND_PCM_FORMAT_S16_LE,
    .buffer_frames = 0,
    .period_time = 50 * 1000,
};

static int parse_uint(const char *str, const char *prefix, unsigned int *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    int v = strtoul(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static int parse_long(const char *str, const char *prefix, long *value, int base)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return 0;

    char *end = NULL;
    long v = strtol(str+len, &end, base);
    if (*end != '\0') {
        fprintf(stderr, "error: is not valid: %s\n", str);
        usage(-1);
    }

    *value = v;
    return 1;
}

static const char *parse_str(const char *str, const char *prefix)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return NULL;

    return str + len;
}

void parse_pcm_args(int argc, char *argv[])
{
    int i;
    const char *str;

    for (i = 2; i < argc; i++) {
        if ((str = parse_str(argv[i], "device="))) {
            device_name = str;
            continue;
        }

        if ((str = parse_str(argv[i], "file="))) {
            file_name = str;
            continue;
        }

        if ((str = parse_str(argv[i], "format="))) {
            if (!strcasecmp(str, "u8"))
                params.format = SND_PCM_FORMAT_U8;
            else if (!strcasecmp(str, "s16_le"))
                params.format = SND_PCM_FORMAT_S16_LE;
            else if (!strcasecmp(str, "s24_le"))
                params.format = SND_PCM_FORMAT_S24_LE;
            else if (!strcasecmp(str, "s32_le"))
                params.format = SND_PCM_FORMAT_S32_LE;
            else
                cmd_error(str, "unkown format");
            continue;
        }

        if (parse_uint(argv[i], "channels=", &params.channels, 10))
            continue;
        if (parse_uint(argv[i], "rate=", &params.rate, 10))
            continue;
        if (parse_uint(argv[i], "buffer_time=", &params.buffer_time, 10)) {
            params.buffer_time *= 1000;
            continue;
        }
        if (parse_uint(argv[i], "period_time=", &params.period_time, 10)) {
            params.period_time *= 1000;
            continue;
        }
        if (parse_uint(argv[i], "time=", &pcm_time, 10))
            continue;

        cmd_error(argv[i], "unkown parameter");
    }
}

void parse_ctl_args(int argc, char *argv[])
{
    int i;
    const char *str;

    for (i = 2; i < argc; i++) {
        if ((str = parse_str(argv[i], "card="))) {
            card_name = str;
            continue;
        }

        if ((str = parse_str(argv[i], "ctl="))) {
            ctl_name = str;
            continue;
        }

        if (parse_long(argv[i], "value=", &m_value, 10)) {
            m_value_is_set = 1;
            continue;
        }

        cmd_error(argv[i], "unkown parameter");
    }
}

static int read_pcm(void)
{
    int ret = -1;

    FILE *file = stdout;
    if (file_name) {
        file = fopen(file_name, "w");
        if (file == NULL) {
            fprintf(stderr, "failed to open %s: %s\n", file_name, strerror(errno));
            return -1;
        }
    }

    struct alsa_pcm *alsa = alsa_pcm_open_capture_device(device_name);
    if (alsa == NULL)
        goto close_file;

    ret = alsa_pcm_set_params(alsa, &params);
    if (ret)
        goto out;

    unsigned int frames = -1;
    if (pcm_time != -1)
        frames = (uint64_t)pcm_time * params.rate;

    unsigned char buffer[512];

    while (1) {
        int n = sizeof(buffer) / params.frame_bytes;
        ret = alsa_pcm_read(alsa, buffer, n);
        if (ret)
            goto out;

        ret = fwrite(buffer, params.frame_bytes, n, file);
        if (ret <= 0 && ferror(file)) {
            fprintf(stderr, "failed to write %s: %s\n",
                file_name ? file_name : "stdout", strerror(errno));
            ret = -1;
            goto out;
        }

        if (frames == -1)
            continue;
        if (frames <= n) {
            ret = 0;
            goto out;
        }
        frames -= n;
    }

out:
    alsa_pcm_close(alsa);
close_file:
    if (file != stdout)
        fclose(file);
    return ret;
}

static int write_pcm(void)
{
    int ret = -1;

    FILE *file = stdin;
    if (file_name) {
        file = fopen(file_name, "r");
        if (file == NULL) {
            fprintf(stderr, "failed to open %s: %s\n", file_name, strerror(errno));
            return -1;
        }
    }

    struct alsa_pcm *alsa = alsa_pcm_open_playback_device(device_name);
    if (alsa == NULL)
        goto close_file;

    ret = alsa_pcm_set_params(alsa, &params);
    if (ret)
        goto out;

    unsigned int frames = -1;
    if (pcm_time != -1)
        frames = (uint64_t)pcm_time * params.rate;

    unsigned char buffer[512];
    while (1) {
        int n = sizeof(buffer) / params.frame_bytes;
        ret = fread(buffer, params.frame_bytes, n, file);
        if (ret <= 0) {
            if (ferror(file)) {
                fprintf(stderr, "failed to read %s: %s\n",
                    file_name ? file_name : "stdin", strerror(errno));
                ret = -1;
            }
            goto drain_pcm;
        }

        ret = alsa_pcm_write(alsa, buffer, n);
        if (ret)
            goto out;

        if (frames == -1)
            continue;
        if (frames <= n) {
            ret = 0;
            goto drain_pcm;
        }
        frames -= n;
    }

drain_pcm:
    alsa_pcm_drain(alsa);
out:
    alsa_pcm_close(alsa);
close_file:
    if (file != stdout)
        fclose(file);
    return ret;
}

static int write_ctl(void)
{
    struct alsa_ctl *ctl = alsa_ctl_open(card_name, ctl_name);
    if (ctl == NULL)
        return -1;

    int ret = alsa_ctl_set_value(ctl, m_value);

    alsa_ctl_close(ctl);

    return ret;
}

static int read_ctl(void)
{
    struct alsa_ctl *ctl = alsa_ctl_open(card_name, ctl_name);
    if (ctl == NULL)
        return -1;

    long value;
    int ret = alsa_ctl_get_value(ctl, &value);
    if (ret == 0)
        printf("%ld\n", value);

    alsa_ctl_close(ctl);

    return ret;
}

int main(int argc, char *argv[])
{
    int cmd = -1;

    prg_name = argv[0];

    while (1) {
        if (argc < 2)
            usage(-1);

        if (!strcmp(argv[1], "-h") ||
            !strcmp(argv[1], "--help"))
            usage(argc == 2 ? 0 : -1);

        if (!strcmp(argv[1], "record")) {
            parse_pcm_args(argc, argv);
            cmd = cmd_read_pcm;
            break;
        }

        if (!strcmp(argv[1], "play")) {
            parse_pcm_args(argc, argv);
            cmd = cmd_write_pcm;
            break;
        }

        if (!strcmp(argv[1], "list_ctls")) {
            parse_ctl_args(argc, argv);
            cmd = cmd_list_ctls;
            break;
        }

        if (!strcmp(argv[1], "get_ctl")) {
            parse_ctl_args(argc, argv);
            if (ctl_name == NULL)
                cmd_error(argv[1], "ctl=ctl_name not set");
            cmd = cmd_read_ctl;
            break;
        }

        if (!strcmp(argv[1], "set_ctl")) {
            parse_ctl_args(argc, argv);
            if (ctl_name == NULL)
                cmd_error(argv[1], "ctl=ctl_name not set");
            if (m_value_is_set == 0)
                cmd_error(argv[1], "value=value not set");
            cmd = cmd_write_ctl;
            break;
        }

        cmd_error(argv[1], "unknown cmd");
    }

    if (cmd == cmd_read_pcm)
        return read_pcm();
    if (cmd == cmd_write_pcm)
        return write_pcm();
    if (cmd == cmd_list_ctls)
        return alsa_ctl_list_all(card_name);
    if (cmd == cmd_write_ctl)
        return write_ctl();
    if (cmd == cmd_read_ctl)
        return read_ctl();

    return 0;
}
