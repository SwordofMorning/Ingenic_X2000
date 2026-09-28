#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include <time.h>
#include <netdb.h>
#include <assert.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <libhardware2/wifi.h>

#define MAX_COMMAND_LEN     1024 * 4
#define MAX_REPLY_LEN       1024 * 4
#define CONFIG_CTRL_IFACE_IFNAME    "/var/run/wpa_supplicant/wlan0"

enum wifi_callback_event_flag {
    ON_NETWORK_NOT_FOUND,
    ON_NETWORK_WRONG_KEY,
    ON_NETWORK_WIFI_STATE,
    ON_NETWORK_SERVER_STATE,
};

struct wpa_ctrl {
    int sockfd;
    struct sockaddr_un local;
    struct sockaddr_un dest;
};

static struct wpa_ctrl ctrl_sorcket;
static pthread_mutex_t open_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t receive_mutex = PTHREAD_MUTEX_INITIALIZER;

/* hex dump unsigned char */
#define HEX_DUMP(uc) (((uc) > 9 ? 'a' - 10 : '0') + (uc))

static void dump_to_hex(char *buf, const char *str_)
{
    unsigned char *str = (unsigned char *) str_;

    while (*str) {
        *buf++ = HEX_DUMP(*str >> 4);
        *buf++ = HEX_DUMP(*str & 0x0f);
        str++;
    }
}

static int wpa_ctrl_receive_data(int sockfd, char *reply, int timeout_us)
{
    int ret, len;
    fd_set rfds;
    struct timeval tv;

    tv.tv_sec = timeout_us / 1000000;
    tv.tv_usec = timeout_us % 1000000;
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    ret = select(sockfd + 1, &rfds, NULL, NULL, &tv);
    if (ret < 0) {
        fprintf(stderr, "wifi: receive data failed to select fd, %s\n", strerror(errno));
        return -1;
    }


    if (FD_ISSET(sockfd, &rfds)) {
        len = recv(sockfd, reply, MAX_REPLY_LEN - 1, 0);
        if (len < 0) {
            fprintf(stderr, "wifi: receive data failed to recv, %s\n", strerror(errno));
            return len;
        }

        reply[len] = '\0';
    }

    return ret;
}

static int wpa_ctrl_command(int sockfd, char *reply, int timeout_us, const char *fmt, ...)
{
    int ret, len;
    va_list args_list;
    char cmd[MAX_COMMAND_LEN];
    char reply_null[MAX_REPLY_LEN];

    pthread_mutex_lock(&receive_mutex);

    va_start(args_list, fmt);

    len = vsnprintf(cmd, MAX_COMMAND_LEN, fmt, args_list);

    ret = send(sockfd, cmd, len, 0);
    if (ret < 0) {
        fprintf(stderr, "wifi: send %s cmd failed: %s\n", cmd, strerror(errno));
        goto unlock;
    }

    va_end(args_list);

    if (reply == NULL)
        reply = reply_null;

    ret = wpa_ctrl_receive_data(sockfd, reply, timeout_us);
    if (ret < 0)
        fprintf(stderr, "wifi: receive cmd %s data failed: %s\n", cmd, strerror(errno));

unlock:
    pthread_mutex_unlock(&receive_mutex);

    return ret;
}


static int wifi_cmd_set_network_ssid(int sockfd, char *reply, char *network_id, const char *ssid)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "SET_NETWORK %s ssid \"%s\"", network_id, ssid);
}

static int wifi_cmd_set_network_psk(int sockfd, char *reply, char *network_id, const char *psk)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "SET_NETWORK %s psk \"%s\"", network_id, psk);
}

static int wifi_cmd_set_network_bssid(int sockfd, char *reply, char *network_id, const char *bssid)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "BSSID %s %s", network_id, bssid);
}

static int wifi_cmd_select_network(int sockfd, char *reply, char *network_id)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "SELECT_NETWORK %s", network_id);
}

static int wifi_cmd_enable_network(int sockfd, char *reply, char *network_id)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "ENABLE_NETWORK %s", network_id);
}

static int wifi_cmd_set_network_none_key(int sockfd, char *reply, char *network_id)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "SET_NETWORK %s key_mgmt NONE", network_id);
}

static int wifi_cmd_add_network(int sockfd, char *reply)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "ADD_NETWORK");
}

static int wifi_cmd_signal_poll(int sockfd, char *reply)
{
    return wpa_ctrl_command(sockfd, reply, 5000000, "SIGNAL_POLL");
}

static int wifi_cmd_status(int sockfd, char *reply)
{
    return wpa_ctrl_command(sockfd, reply, 5000000, "STATUS");
}

static int wifi_cmd_disconnect(int sockfd, char *reply)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "DISCONNECT");
}

static int wifi_cmd_attach(int sockfd, char *reply)
{
    return wpa_ctrl_command(sockfd, reply, 2000000, "ATTACH");
}

static int wifi_cmd_scan_results(int sockfd, char *reply)
{
    int ret = wpa_ctrl_command(sockfd, reply, 20000000, "SCAN");
    if (ret < 0)
        return ret;

    return wpa_ctrl_command(sockfd, reply, 20000000, "SCAN_RESULTS");
}

static int wpa_ctrl_open(struct wpa_ctrl *ctrl, const char *ctrl_path)
{
    int ret;
    static int counter = 0;

    pthread_mutex_lock(&open_mutex);

    ctrl->sockfd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (ctrl->sockfd < 0)
        goto unlock;

    ctrl->local.sun_family = AF_UNIX;

    snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path), "/tmp/wpa_ctrl_%d-%d", (int)getpid(), counter);

    ret = bind(ctrl->sockfd, (struct sockaddr *)&ctrl->local, sizeof(ctrl->local));
    if (ret < 0)
        goto close_socket;

    ctrl->dest.sun_family = AF_UNIX;
    memcpy(ctrl->dest.sun_path, ctrl_path, strlen(ctrl_path) + 1);

    ret = connect(ctrl->sockfd, (struct sockaddr *)&ctrl->dest, sizeof(ctrl->dest));
    if (ret < 0)
        goto unlink;

    /**
     * Make socket non-blocking so that we don't hang forever if
     * target dies unexpectedly.
     */
    ret = fcntl(ctrl->sockfd, F_GETFL);
    if (ret >= 0) {
        ret |= O_NONBLOCK;
        if (fcntl(ctrl->sockfd, F_SETFL, ret) < 0)
            fprintf(stderr, "wifi: fcntl(sockfd, O_NONBLOCK) failed: %s\n", strerror(errno));
    }

    counter++;

    pthread_mutex_unlock(&open_mutex);

    return ret;

unlink:
    unlink(ctrl->local.sun_path);
close_socket:
    close(ctrl->sockfd);
unlock:
    pthread_mutex_unlock(&open_mutex);

    return -1;
}

static enum wifi_connect_state wifi_get_network_connect_state(int sockfd)
{
    char status[PATH_MAX];

    int ret = send(sockfd, "STATUS", 6, 0);
    if (ret < 0)
        return -1;

    memset(status, 0, sizeof(status));
    ret = wpa_ctrl_receive_data(sockfd, status, 5000000);
    if (ret < 0)
        return -1;

    if (strstr(status, "wpa_state=COMPLETED") && strstr(status, "ip_address="))
        return WIFI_CONNECTED;

    return WIFI_DISCONNECTED;
}

static enum wifi_server_state wifi_get_network_server_state(int sockfd)
{
    char reply[PATH_MAX];

    if (sockfd < 0)
        return SERVER_DISCONNECT;

    int ret = send(sockfd, "PING", 4, 0);

    if (ret < 0) {
        fprintf(stderr, "wifi: send ping error: %s\n", strerror(errno));
        return SERVER_DISCONNECT;
    }

    memset(reply, 0, sizeof(reply));

    ret = wpa_ctrl_receive_data(sockfd, reply, 2000000);
    if (ret < 0) {
        fprintf(stderr, "wifi: receive ping error: %s\n", strerror(errno));
        return SERVER_DISCONNECT;
    }

    return SERVER_CONNECT;

}

static void wifi_event_handle_callback(struct wifi_event_callback *callback, int type, int state)
{
    switch (type) {
        case ON_NETWORK_WIFI_STATE:
            if (callback->network_state_changed_cb)
                callback->network_state_changed_cb(state);
            break;

        case ON_NETWORK_NOT_FOUND:
            if (callback->network_not_found_cb)
                callback->network_not_found_cb();
            break;

        case ON_NETWORK_WRONG_KEY:
            if (callback->network_wrong_key_cb)
                callback->network_wrong_key_cb();
            break;

        case ON_NETWORK_SERVER_STATE:
            if (callback->network_server_changed_cb)
                callback->network_server_changed_cb(state);
            break;
        default:
            break;
    }
}

static void wifi_network_close_client_device(struct wpa_ctrl *ctrl)
{
    if (ctrl->sockfd > 0) {
        close(ctrl->sockfd);
        ctrl->sockfd = -1;
    }

    if (!access(ctrl->local.sun_path, F_OK))
        remove(ctrl->local.sun_path);
}

static int wifi_network_open_client_device0(struct wpa_ctrl *ctrl)
{
    if (ctrl->sockfd <= 0) {
        int ret = wpa_ctrl_open(ctrl, CONFIG_CTRL_IFACE_IFNAME);
        if (ret < 0) {
            ctrl->sockfd = -1;
            return -1;
        }
    }

    return 0;
}

static int wifi_network_open_client_device1(struct wpa_ctrl *ctrl)
{
    if (ctrl->sockfd <= 0) {
        int ret = wpa_ctrl_open(ctrl, CONFIG_CTRL_IFACE_IFNAME);
        if (ret < 0) {
            ctrl->sockfd = -1;
            return -1;
        }

        ret = wifi_cmd_attach(ctrl->sockfd, NULL);
        if (ret < 0) {
            wifi_network_close_client_device(ctrl);
            ctrl->sockfd = -1;
            return -1;;
        }
    }

    return 0;
}

/*
* wifi事件检测
*/
static void* wifi_monitor_event_thread(void *data)
{
    int ret;
    char reply[PATH_MAX];

    int network_connect_event = 0;
    int network_server_state_old = SERVER_DISCONNECT;
    enum wifi_server_state server_state;
    enum wifi_connect_state connect_state;
    struct wpa_ctrl p_ctrl_sorcket = {0};
    struct wpa_ctrl p_monitor_sorcket = {0};

    wifi_network_open_client_device0(&p_ctrl_sorcket);
    wifi_network_open_client_device1(&p_monitor_sorcket);

    wifi_event_callback *callback = (wifi_event_callback *)data;

    while (callback->callback_state) {
        server_state = wifi_get_network_server_state(p_ctrl_sorcket.sockfd);

        if (server_state == SERVER_DISCONNECT) {
            if (network_server_state_old == SERVER_CONNECT) {
                network_server_state_old = SERVER_DISCONNECT;

                /* close all monitor device */
                wifi_network_close_client_device(&p_ctrl_sorcket);
                wifi_network_close_client_device(&p_monitor_sorcket);
                wifi_event_handle_callback(callback, ON_NETWORK_SERVER_STATE, SERVER_DISCONNECT);
            }

            /* open all monitor device */
            wifi_network_open_client_device0(&p_ctrl_sorcket);
            wifi_network_open_client_device1(&p_monitor_sorcket);
            usleep(100*1000);
            continue;
        }

        if (network_server_state_old == SERVER_DISCONNECT) {
            network_server_state_old = SERVER_CONNECT;
            wifi_event_handle_callback(callback, ON_NETWORK_SERVER_STATE, SERVER_CONNECT);
        }

        memset(reply, 0, sizeof(reply));
        ret = wpa_ctrl_receive_data(p_monitor_sorcket.sockfd, reply, 4000);
        if (ret < 0)
            continue;

        if (strstr(reply, "CTRL-EVENT-NETWORK-NOT-FOUND")) {
            /* Network not found */
            wifi_event_handle_callback(callback, ON_NETWORK_NOT_FOUND, -1);
        } else if (strstr(reply, "CTRL-EVENT-CONNECTED")) {
            /* Network connecting */
            wifi_event_handle_callback(callback, ON_NETWORK_WIFI_STATE, WIFI_CONNECTING);
            network_connect_event = 1;
        }
        else if (strstr(reply, "reason=WRONG_KEY")) {
            /* Wrong key */
            wifi_event_handle_callback(callback, ON_NETWORK_WRONG_KEY, -1);
        } else if (strstr(reply, "CTRL-EVENT-DISCONNECTED")) {
            /* Network disconnected */
            wifi_event_handle_callback(callback, ON_NETWORK_WIFI_STATE, WIFI_DISCONNECTED);
        }

        /* Get network connected state */
        if (network_connect_event) {
            connect_state = wifi_get_network_connect_state(p_ctrl_sorcket.sockfd);
            if (connect_state == WIFI_CONNECTED)  {
                /* Network connected */
                wifi_event_handle_callback(callback, ON_NETWORK_WIFI_STATE, WIFI_CONNECTED);
                network_connect_event = 0;
            }
        }

        usleep(100*1000);
    }

    wifi_network_close_client_device(&p_ctrl_sorcket);
    wifi_network_close_client_device(&p_monitor_sorcket);

    return NULL;
}

static int check_data(char *data)
{
    char ch = *data;
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    if ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
        return ch - 'a' + 10;
    }

    return -1;
}

/*
 * 字符串转十六进制 ("\x11\x22\x33" -----> 0x11 0x22 0x33)
*/
static void decode_ssid(char *ssid)
{
    int data_h;
    int data_l;
    char buf[256];
    char *addr = ssid;
    unsigned char data = 0;

    char *ssid_buf = buf;

    while (*addr) {

        if (*addr != '\\') {
            *ssid_buf++ = *addr;
            addr++;
            continue;
        }

        /* 是汉字字符 */
        if (addr[1] == 'x') {
            data_h = check_data(&addr[2]);
            data_l = check_data(&addr[3]);

            addr += 2;

            if (data_h != -1) {
                data = data_h & 0xf;
                addr++;
                if (data_l != -1) {
                    data = (data << 4) + (data_l & 0xf);
                    addr++;
                }
            }
            *ssid_buf++ = data;
        }

        /* 是转义字符 */
        else {
            *ssid_buf++ = addr[1];
            addr += 2;
        }
    }

    *ssid_buf++ = '\0';

    memcpy(ssid, buf, ssid_buf - buf);
}

static int wifi_parse_str(char *reply, const char *prefix, char *info)
{
    char *str = strstr(reply, prefix);
    if (!str)
        return -1;

    str += strlen(prefix);

    while (*str != '\n')
        *info++ = *str++;

    return 0;
}

static void wifi_parse_status_info(struct wifi_network_status_info* info, char* reply)
{
    char buf[128];

    memset(buf, 0, sizeof(buf));

    /* 获取wifi热点mac地址 */
    int ret = wifi_parse_str(reply, "bssid=", info->bssid);
    if (ret)
        memset(info->bssid, 0 , sizeof(info->bssid));

    /* 获取wifi频率MHz */
    ret = wifi_parse_str(reply, "\nfreq=", buf);
    if (ret)
        memset(buf, 0 , sizeof(buf));
    info->freq = atoi(buf);

    /* 获取wifi名称 */
    ret = wifi_parse_str(reply, "\nssid=", info->ssid);
    if (ret)
        memset(info->ssid, 0 , sizeof(info->ssid));

    decode_ssid(info->ssid);

    /* 获取wifi网络id */
    ret = wifi_parse_str(reply, "\nid=", buf);
    if (ret)
        memset(buf, 0 , sizeof(buf));
    info->network_id = atoi(buf);

    /* 获取wifi模式 */
    ret = wifi_parse_str(reply, "\nmode=", info->mode);
    if (ret)
        memset(info->mode, 0 , sizeof(info->mode));

    /* 获取wifi加密套件 */
    ret = wifi_parse_str(reply, "\ngroup_cipher=", info->group_cipher);
    if (ret)
        memset(info->group_cipher, 0 , sizeof(info->group_cipher));

    /* 获取wifi加密模式 */
    ret = wifi_parse_str(reply, "\nkey_mgmt=", info->cipher_mode);
    if (ret)
        memset(info->cipher_mode, 0 , sizeof(info->cipher_mode));

    /* 获取wifi状态 */
    ret = wifi_parse_str(reply, "wpa_state=", info->status);
    if (ret)
        memset(info->status, 0, sizeof(info->status));

    /* 获取wifi ip地址 */
    ret = wifi_parse_str(reply, "\nip_address=", info->ip_addr);
    if (ret)
        memset(info->ip_addr, 0 , sizeof(info->ip_addr));

    /* 获取网卡的mac地址 */
    ret = wifi_parse_str(reply, "\naddress=", info->mac_addr);
    if (ret)
        memset(info->mac_addr, 0 , sizeof(info->mac_addr));

    /* 获取uuid */
    ret = wifi_parse_str(reply, "\nuuid=", info->uuid);
    if (ret)
        memset(info->uuid, 0 , sizeof(info->uuid));
}

static void wifi_parse_signal_poll_info(struct wifi_network_status_info* info, char* reply)
{
    char buf[4];

    memset(buf, 0, sizeof(buf));

    /* 获取wifi信号强度 */
    wifi_parse_str(reply, "RSSI=-", buf);

    info->signal_strength = atoi(buf);
}

static char* parse_scan_info(struct wifi_network_scan_info *info, char *reply)
{
    char buf[64];

    /* 获取wifi 热点mac地址 */
    int i = 0;
    while (*reply != '\t')
        info->bssid[i++] = *reply++;
    reply++;

    /* 获取wifi 频率 */
    i = 0;
    memset(buf, 0, sizeof(buf));
    while (*reply != '\t')
        buf[i++] = *reply++;
    info->freq = atoi(buf);
    reply += 2;

    /* 获取wifi 信号质量 */
    i = 0;
    memset(buf, 0, sizeof(buf));
    while (*reply != '\t')
        buf[i++] = *reply++;
    info->signal_strength = atoi(buf);
    reply++;

    /* 获取wifi 加密模式 */
    i = 0;
    while (*reply != '\t')
        info->cipher_mode[i++] = *reply++;
    reply++;

    /* 获取wifi 名称 */
    i = 0;
    while (*reply != '\n')
        info->ssid[i++] = *reply++;
    reply++;

    decode_ssid(info->ssid);

    return reply;
}

static int wifi_get_scan_network_device_count(char* reply)
{
    int count = 0;

    while (*reply) {
        if (*reply++ == '\n')
            count++;
    }

    count--;

    return count;
}

static int wifi_parse_scan_info(struct wifi_network_scan_info* info, int info_count, char* reply)
{
    int i;
    int count = wifi_get_scan_network_device_count(reply);

    if (count > info_count)
        count = info_count;

    while (*reply++ != '\n');

    for (i = 0; i < count; i++)
        reply = parse_scan_info(&info[i], reply);

    return count;
}

static void wifi_update_network_ip(void)
{
    /* kill udhcpc process */
    system("killall udhcpc");
    /* udhcpc -i wlan0*/
    system("udhcpc -i wlan0 &");
}

static int do_connect_network(int sockfd, const char *ssid, const char *psk, const char *bssid)
{
    int ret;

    /* 保存网络id */
    char id_buf[MAX_REPLY_LEN];
    memset(id_buf, 0, sizeof(id_buf));
    ret = wifi_cmd_add_network(sockfd, id_buf);
    if (ret < 0) {
        fprintf(stderr, "wifi: wifi connect add network failed\n");
        return ret;
    }

    /* 连接的网络名称ssid */
    ret = wifi_cmd_set_network_ssid(sockfd, NULL, id_buf, ssid);
    if (ret < 0) {
        fprintf(stderr, "wifi: wifi set network ssid failed\n");
        return ret;
    }

    /* 连接网络的密钥psk */
    if (psk) {
        ret = wifi_cmd_set_network_psk(sockfd, NULL, id_buf, psk);
        if (ret < 0) {
            fprintf(stderr, "wifi: wifi set network psk failed\n");
            return ret;
        }
    } else {
        ret = wifi_cmd_set_network_none_key(sockfd, NULL, id_buf);
        if (ret < 0) {
            fprintf(stderr, "wifi: wifi set network no key failed\n");
            return ret;
        }
    }
    /* 选择热点的bssid */
    if (bssid) {
        ret = wifi_cmd_set_network_bssid(sockfd, NULL, id_buf, bssid);
        if (ret < 0) {
            fprintf(stderr, "wifi: wifi bssid network failed\n");
            return ret;
        }
    }

    /* 选择连接网络 */
    ret = wifi_cmd_select_network(sockfd, NULL, id_buf);
    if (ret < 0) {
        fprintf(stderr, "wifi: wifi select network failed\n");
        return ret;
    }

    /* 使能连接的网络 */
    ret = wifi_cmd_enable_network(sockfd, NULL, id_buf);
    if (ret < 0) {
        fprintf(stderr, "wifi: wifi enable network psk failed\n");
        return ret;
    }

    wifi_update_network_ip();

    return ret;
}

static int wifi_open_network_device(struct wpa_ctrl *ctrl)
{
    return wifi_network_open_client_device0(ctrl);
}

static int write_file(const char *file_path, char *data, int data_len)
{
    int fp = open(file_path, O_RDWR | O_CREAT | O_APPEND);
    if (fp == -1) {
        fprintf(stderr, "wifi: open %s file failed: %s\n", file_path, strerror(errno));
        return -1;
    }

    int len = write(fp, data, data_len);
    if (len != data_len) {
        fprintf(stderr, "wifi: write %s file failed: %s\n", file_path, strerror(errno));
        close(fp);
        return -1;
    }

    close(fp);

    return 0;
}

static void* scan_ap_once(void *data)
{
    int ret;
    int count = 0;
    struct wpa_ctrl ctrl = {0};

    do {
        ret = wpa_ctrl_open(&ctrl, CONFIG_CTRL_IFACE_IFNAME);
        usleep(4000);
        count++;
    } while (ret < 0 && count < 1000);

    wifi_cmd_scan_results(ctrl.sockfd, NULL);

    wifi_network_close_client_device(&ctrl);

    return NULL;
}


static void wifi_create_scan_once_thread(void)
{
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, scan_ap_once, NULL);
    pthread_attr_destroy(&attr);
}


////////////////////////////////////////////////////////

/*
* 注册监听事件回调
*/
void wifi_register_event_callback(struct wifi_event_callback* callback)
{
    assert(callback);

    callback->callback_state = 1;
    pthread_create(&callback->tid, NULL, wifi_monitor_event_thread, callback);
}

/*
* 注销监听事件回调
*/
void wifi_unregister_event_callback(struct wifi_event_callback* callback)
{
    assert(callback);

    callback->callback_state = 0;
    pthread_join(callback->tid, NULL);
}

/*
 * 查看wifi服务端状态
*/
enum wifi_server_state wifi_check_network_server_state(void)
{
    if (ctrl_sorcket.sockfd > 0)
        return SERVER_CONNECT;

    int ret = wifi_network_open_client_device0(&ctrl_sorcket);
    if (ret < 0)
        return SERVER_DISCONNECT;

    return SERVER_CONNECT;
}

/*
 * 关闭网络服务端
*/
void wifi_stop_network_server(void)
{
    system("wifi_down.sh");
}

/*
 * 打开网络服务端
*/
void wifi_start_network_server(const char *network_config_path)
{
    char cmd[512];

    /* 配置文件不存在 */
    if (access(network_config_path, F_OK)) {
        memset(cmd, 0, sizeof(cmd));
        memcpy(cmd, "ctrl_interface=/var/run/wpa_supplicant\nupdate_config=1\ncountry=GB\n", 67);
        int ret = write_file(network_config_path, cmd, strlen(cmd));
        if (ret)
            return;
    }

    system("ps | grep -v grep | grep \" wpa_supplicant \" >> /dev/null && wifi_down.sh");
    sprintf(cmd, "wifi_up.sh %s", network_config_path);
    system(cmd);

    /* 服务端打开,网络未连接时，第一次一定扫描不到，主动扫描一次 */
    wifi_create_scan_once_thread();
}

/*
 * 连接网络
 */
int wifi_connect_network(const char *network_ssid, const char *network_psk, const char *network_bssid)
{
    int ret;

    enum wifi_server_state state = wifi_check_network_server_state();
    if (state == SERVER_DISCONNECT) {
        fprintf(stderr, "wifi: Please open the wifi server first\n");
        return -1;
    }

    struct wpa_ctrl ctrl = {0};

    ret = wifi_open_network_device(&ctrl);
    if (ret < 0) {
        fprintf(stderr, "wifi: Open the wifi device fail\n");
        return ret;
    }

    ret = wifi_cmd_disconnect(ctrl.sockfd, NULL);
    if (ret < 0)
        fprintf(stderr, "wifi: wifi disconnect failed.\n");


    ret = do_connect_network(ctrl.sockfd, network_ssid, network_psk, network_bssid);
    if (ret < 0)
        fprintf(stderr, "wifi: wifi config network failed\n");

    wifi_network_close_client_device(&ctrl);

    return ret;
}

/*
* wifi 断开当前网络连接
*/
int wifi_disconnect_current_network(void)
{
    int ret;

    enum wifi_server_state state = wifi_check_network_server_state();
    if (state == SERVER_DISCONNECT) {
        fprintf(stderr, "wifi: Please open the wifi server first\n");
        return -1;
    }

    struct wpa_ctrl ctrl = {0};

    ret = wifi_open_network_device(&ctrl);
    if (ret < 0) {
        fprintf(stderr, "wifi: Open the wifi device fail\n");
        return ret;
    }

    ret = wifi_cmd_disconnect(ctrl.sockfd, NULL);
    if (ret < 0)
        fprintf(stderr, "wifi: wifi disconnect failed.\n");

    wifi_network_close_client_device(&ctrl);

    return ret;
}

/*
* wifi 扫描
*/
int wifi_get_scan_info(struct wifi_network_scan_info* network_info, int info_count)
{
    char reply[MAX_REPLY_LEN];

    enum wifi_server_state state = wifi_check_network_server_state();
    if (state == SERVER_DISCONNECT) {
        fprintf(stderr, "wifi: Please open the wifi server first\n");
        return -1;
    }

    struct wpa_ctrl ctrl = {0};

    int ret = wifi_open_network_device(&ctrl);
    if (ret < 0) {
        fprintf(stderr, "wifi: Open the wifi device fail\n");
        return ret;
    }

    memset(reply, 0 , sizeof(reply));

    ret = wifi_cmd_scan_results(ctrl.sockfd, reply);
    if (ret < 0) {
        fprintf(stderr, "wifi: failed to scan wifi\n");
        goto close_device;
    }

    ret = wifi_parse_scan_info(network_info, info_count, reply);

close_device:
    wifi_network_close_client_device(&ctrl);

    return ret;
}

/*
* 获取 wifi 状态
*/
int wifi_get_status_info(struct wifi_network_status_info* neiwork_info)
{
    char reply[MAX_REPLY_LEN];

    enum wifi_server_state state = wifi_check_network_server_state();
    if (state == SERVER_DISCONNECT) {
        fprintf(stderr, "wifi: Please open the wifi server first\n");
        return -1;
    }

    struct wpa_ctrl ctrl = {0};

    int ret = wifi_open_network_device(&ctrl);
    if (ret < 0) {
        fprintf(stderr, "wifi: Open the wifi device fail\n");
        return ret;
    }

    memset(reply, 0 , sizeof(reply));

    ret = wifi_cmd_signal_poll(ctrl.sockfd, reply);
    if (ret < 0)
        fprintf(stderr, "wifi: failed to get wifi status\n");

    wifi_parse_signal_poll_info(neiwork_info, reply);

    ret = wifi_cmd_status(ctrl.sockfd, reply);
    if (ret < 0)
        fprintf(stderr, "wifi: failed to get wifi status\n");

    wifi_parse_status_info(neiwork_info, reply);

    wifi_network_close_client_device(&ctrl);

    return ret;
}

/*
 * 保存wifi网络配置
*/
int wifi_save_network_config(const char *network_config_path, const char *network_ssid, const char *network_psk, const char *network_bssid)
{
    int ret;
    char cmd[600];
    char ssid[512];

    if (access(network_config_path, F_OK)) {
        memset(cmd, 0, sizeof(cmd));
        memcpy(cmd, "ctrl_interface=/var/run/wpa_supplicant\nupdate_config=1\ncountry=GB\n", 67);
        ret = write_file(network_config_path, cmd, strlen(cmd));
        if (ret)
            return -1;
    }

    memset(cmd, 0, sizeof(cmd));
    memcpy(cmd, "network={\n", 11);
    ret = write_file(network_config_path, cmd, strlen(cmd));
    if (ret)
        return -1;

    memset(ssid, 0, sizeof(ssid));
    dump_to_hex(ssid, network_ssid);
    sprintf(cmd, "\tssid=%s\n", ssid);
    ret = write_file(network_config_path, cmd, strlen(cmd));
    if (ret)
        return -1;

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "\t#ssid=\"%s\"\n", network_ssid);
    ret = write_file(network_config_path, cmd, strlen(cmd));
    if (ret)
        return -1;

    memset(cmd, 0, sizeof(cmd));
    if (network_psk)
        sprintf(cmd, "\tpsk=\"%s\"\n", network_psk);
    else
        memcpy(cmd, "\tkey_mgmt=NONE\n", 16);
    ret = write_file(network_config_path, cmd, strlen(cmd));
    if (ret)
        return -1;


    if (network_bssid) {
        sprintf(cmd, "\tbssid=%s\n", network_bssid);
        ret = write_file(network_config_path, cmd, strlen(cmd));
        if (ret)
            return -1;
    }

    memset(cmd, 0, sizeof(cmd));
    memcpy(cmd, "}\n\n", 4);
    ret = write_file(network_config_path, cmd, strlen(cmd));
    if (ret)
        return -1;

    return 0;
}