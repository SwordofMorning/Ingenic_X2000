#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libhardware2/wifi.h>

#include <sys/types.h>
#include <sys/syscall.h>

enum {
    cmd_scan_ap,
    cmd_start_server,
    cmd_stop_server,
    cmd_connect_network,
    cmd_save_network_config,
    cmd_disconnect_current_network,
    cmd_get_current_network_status,
};

#define MAX_SCAN_COUNT     50

static const char *prg_name;

static void usage(int status)
{
    fprintf(stderr, "%s usage\n", prg_name);
    fprintf(stderr, "   -h/--help               : show help info\n");

    fprintf(stderr, "   start_server [path=/usr/data/wpa_supplicant.conf]   : start wifi server\n");
    fprintf(stderr, "   Example: %s start_server path=/usr/data/wpa_supplicant.conf \n", prg_name);
    fprintf(stderr, "\n");

    fprintf(stderr, "   stop_server                        : stop wifi server\n");
    fprintf(stderr, "   Example: %s stop_server\n", prg_name);
    fprintf(stderr, "\n");

    fprintf(stderr, "   connect_network <ssid=xx> [psk=xx] [bssid=xx:xx:xx:xx:xx:xx] : connect network\n");
    fprintf(stderr, "   Example: %s connect_network ssid=Guest psk=ingenic_guest\n", prg_name);
    fprintf(stderr, "\n");

    fprintf(stderr, "   disconnect_current_network          : disconnect current network\n");
    fprintf(stderr, "   Example: %s  disconnect_current_network\n", prg_name);
    fprintf(stderr, "\n");

    fprintf(stderr, "   scan_ap_info                        : scan ap info\n");
    fprintf(stderr, "   Example: %s  scan_ap_info\n", prg_name);
    fprintf(stderr, "\n");

    fprintf(stderr, "   get_current_network_status_info     : get current network status info\n");
    fprintf(stderr, "   Example: %s  get_current_network_status_info\n", prg_name);
    fprintf(stderr, "\n");

    fprintf(stderr, "   save_network_config <path=/usr/data/wifi_network.conf> <ssid=xx> [psk=xx] [bssid=xx:xx:xx:xx:xx:xx]: save network config\n");
    fprintf(stderr, "   Example: %s  save_network_config path=/usr/data/wifi_network.conf ssid=Guest psk=ingenic_guest\n", prg_name);
    fprintf(stderr, "\n");

    exit(status);
}

static char *parse_str(char *str, const char *prefix)
{
    int len = strlen(prefix);

    if (strncmp(str, prefix, len))
        return NULL;

    return str + len;
}


static void network_state_changed_cb(enum wifi_connect_state state)
{
    if (state == WIFI_CONNECTED) {
        /* connect success processing event */
    } else if (state == WIFI_DISCONNECTED) {
        /* connect fail processing event */
    } else if (state == WIFI_CONNECTING) {
        /* connecting processing event */
    }
}

static void network_not_found_cb(void)
{
    /* not found network processing event */
}

static void network_wrong_key_cb(void)
{
    /* network key wrong processing event */
}

static void network_server_changed_cb(enum wifi_server_state state)
{
    if (state == SERVER_CONNECT) {
        /* network server open processing event */
    } else if (state == SERVER_DISCONNECT) {
        /* network server close processing event */
    }
}


static wifi_event_callback callback = {
    network_not_found_cb,
    network_wrong_key_cb,
    network_server_changed_cb,
    network_state_changed_cb,
};


int main(int argc, char *argv[])
{
    int i;
    int ret = 0;
    int cmd = -1;
    const char *str = NULL;
    const char *ssid = NULL;
    const char *psk = NULL;
    const char *bssid = NULL;
    const char *save_network_config_path = NULL;
    const char *server_network_config_path = "/usr/data/wpa_supplicant.conf";

    prg_name = argv[0];

    while (1) {
        if (argc < 2)
            usage(-1);

        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
            usage(-1);

        if (!strcmp(argv[1], "start_server")) {

            if (argc > 3)
                usage(-1);
            cmd = cmd_start_server;
            break;
        }

        if (!strcmp(argv[1], "stop_server")) {
            if (argc != 2)
                usage(-1);
            cmd = cmd_stop_server;
            break;
        }

        if (!strcmp(argv[1], "connect_network")) {
            if (argc < 3 || argc > 6)
                usage(-1);

            cmd = cmd_connect_network;
            break;
        }

        if (!strcmp(argv[1], "disconnect_current_network")) {
            if (argc != 2)
                usage(-1);
            cmd = cmd_disconnect_current_network;
            break;
        }

        if (!strcmp(argv[1], "scan_ap_info")) {
            if (argc != 2)
                usage(-1);
            cmd = cmd_scan_ap;
            break;
        }

        if (!strcmp(argv[1], "get_current_network_status_info")) {
            if (argc != 2)
                usage(-1);
            cmd = cmd_get_current_network_status;
            break;
        }

        if (!strcmp(argv[1], "save_network_config")) {
            if (argc < 4)
                usage(-1);
            cmd = cmd_save_network_config;
            break;
        }

        fprintf(stderr, "error: not support this cmd: %s\n", argv[1]);
        exit(-1);
    }

    wifi_register_event_callback(&callback);

    if (cmd == cmd_start_server) {
        if (argc == 3)
            server_network_config_path = parse_str(argv[2], "path=");

        wifi_start_network_server(server_network_config_path);
        goto end;
    }

    if (cmd == cmd_stop_server) {
        wifi_stop_network_server();
        goto end;
    }

    if (cmd == cmd_connect_network || cmd == cmd_save_network_config) {
        for (i = 1; i < argc; i++) {
            if ((str = parse_str(argv[i], "ssid="))) {
                ssid = str;
                continue;
            }
            if ((str = parse_str(argv[i], "psk="))) {
                psk = str;
                continue;
            }
            if ((str = parse_str(argv[i], "bssid="))) {
                bssid = str;
                continue;
            }
            if ((str = parse_str(argv[i], "path="))) {
                save_network_config_path = str;
                continue;
            }
        }

        if (psk) {
            if (strlen(psk) < 8) {
                fprintf(stderr, "wifi_shell: connect %s wifi psk length less than 8\n", ssid);
                goto end;
            }
        }

        if (cmd == cmd_save_network_config)
            wifi_save_network_config(save_network_config_path, ssid, psk, bssid);
        else if (cmd == cmd_connect_network) {
            ret = wifi_connect_network(ssid, psk, bssid);
            if (ret < 0)
                fprintf(stderr, "wifi_shell: config %s network fail\n", ssid);
        }

        goto end;
    }

    if (cmd == cmd_disconnect_current_network) {
        ret = wifi_disconnect_current_network();
        if (ret < 0)
            fprintf(stderr, "wifi_shell: disconnect wifi fail\n");

        goto end;
    }

    if (cmd == cmd_scan_ap) {
        struct wifi_network_scan_info scan_info[MAX_SCAN_COUNT];
        memset(scan_info, 0, sizeof(scan_info));
        ret = wifi_get_scan_info(scan_info, MAX_SCAN_COUNT);
        if (ret < 0) {
            fprintf(stderr, "wifi_shell: scan wifi fail\n");
            goto end;
        }

        fprintf(stderr, "bssid / frequency / signal level / flags / ssid\n");
        for (i = 0; i < ret; i++)
            fprintf(stderr, "%s\t%d\t-%d\t%s\t%s\n", scan_info[i].bssid, scan_info[i].freq, scan_info[i].signal_strength, scan_info[i].cipher_mode, scan_info[i].ssid);

        goto end;
    }

    if (cmd == cmd_get_current_network_status) {
        struct wifi_network_status_info status_info;
        memset(&status_info, 0, sizeof(status_info));
        ret = wifi_get_status_info(&status_info);
        if (ret < 0) {
            fprintf(stderr, "wifi_shell: get wifi status fail\n");
            goto end;
        }

        fprintf(stderr, "bssid=%s\n", status_info.bssid);
        fprintf(stderr, "freq=%d\n", status_info.freq);
        fprintf(stderr, "ssid=%s\n", status_info.ssid);
        fprintf(stderr, "signal_strength=%d\n", status_info.signal_strength);
        fprintf(stderr, "mode=%s\n", status_info.mode);
        fprintf(stderr, "group_cipher=%s\n", status_info.group_cipher);
        fprintf(stderr, "cipher_mode=%s\n", status_info.cipher_mode);
        fprintf(stderr, "wpa_state=%s\n", status_info.status);
        fprintf(stderr, "ip_address=%s\n", status_info.ip_addr);
        fprintf(stderr, "address=%s\n", status_info.mode);
        fprintf(stderr, "uuid=%s\n", status_info.uuid);

        goto end;
    }

end:
    wifi_unregister_event_callback(&callback);
    return ret;
}
