
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <stdint.h>
#include <time.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <libutils2/cJSON.h>


#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#define INGENIC_VIEW_SERVER "/tmp/ingenic_view_server"
#define INGENIC_VIEW_CLIENT "/tmp/ingenic_view_client"


#define INGENIC_VIEW_CMD_create_view  "create_view"
#define INGENIC_VIEW_CMD_delete_view  "delete_view"
#define INGENIC_VIEW_CMD_set_disp_cfg  "set_disp_cfg"
#define INGENIC_VIEW_CMD_start_catch  "start_catch"
#define INGENIC_VIEW_CMD_stop_catch  "stop_catch"
#define INGENIC_VIEW_CMD_start_display "start_display"
#define INGENIC_VIEW_CMD_stop_display "stop_display"
#define INGENIC_VIEW_CMD_start_record  "start_record"
#define INGENIC_VIEW_CMD_stop_record  "stop_record"



int recv_fd;
struct sockaddr_un server_addr;
struct sockaddr_un client_addr;


static char *create_cjson(char *cmd, char *str_name[], char *string[], int str_cnt,
                          char *num_name[], int number[], int num_cnt)
{
    int i;
    char *pJson;

    if (!cmd)
        return NULL;


    cJSON *pJsonRoot = cJSON_CreateObject();

    if (cmd)
        cJSON_AddStringToObject(pJsonRoot, "cmd", cmd);

    for (i = 0; i < num_cnt; i++) {
        if (!num_name[i])
            continue;
        cJSON_AddNumberToObject(pJsonRoot, num_name[i], number[i]);
    }


    for (i = 0; i < str_cnt; i++) {
        if (!str_name[i] || !string[i])
            continue;
        cJSON_AddStringToObject(pJsonRoot, str_name[i], string[i]);
    }

    pJson = cJSON_Print(pJsonRoot);

    cJSON_Delete(pJsonRoot);

    return pJson;
}


static void delete_cjson(char *pJson)
{
    free(pJson);
}


static int client_send_cmd(const char *cmd, int cmd_len)
{

    int recvbytes;
    char buf[64] = {0};
    unsigned int addr_len = sizeof(struct sockaddr_un);

    if (sendto(recv_fd, cmd, strlen(cmd), 0, (const struct sockaddr *)&server_addr, sizeof(struct sockaddr_un)) == -1) {
        fprintf(stderr, "SAMPLE_CLIENT: sendto error:%s\n", strerror(errno));
        return -1;
    }

    recvbytes = recvfrom(recv_fd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &addr_len);
    if (recvbytes < 0) {
        fprintf(stderr, "recvfrom error:%s\n", strerror(errno));
        return -1;
    }

    if (strcmp("OK", buf) != 0) {
        fprintf(stderr, "SAMPLE_CLIENT: cmd work err: %s\n", buf);
        return -1;
    }

    return 0;
}


int ingenic_h264_view_client_create(char *view_name, char *data)
{
    char *str_name[] = {"view_name", "data"};
    char *str_data[] = {view_name, data};

    char *pjson =  create_cjson(INGENIC_VIEW_CMD_create_view, str_name, str_data, ARRAY_SIZE(str_name),
                                NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_create_view);

    delete_cjson(pjson);

    return ret;
}

int ingenic_h264_view_client_delete(char *view_name)
{
    char *param_name[] = {"view_name"};
    char *param_data[] = {view_name};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_delete_view, param_name, param_data,  ARRAY_SIZE(param_name), NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_delete_view);

    delete_cjson(pjson);

    return ret;
}

int ingenic_h264_view_client_start_catch(char *view_name)
{
    char *param_name[] = {"view_name"};
    char *param_data[] = {view_name};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_start_catch, param_name, param_data,  ARRAY_SIZE(param_name), NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_start_catch);


    delete_cjson(pjson);

    return ret;

}

int ingenic_h264_view_client_stop_catch(char *view_name)
{
    char *param_name[] = {"view_name"};
    char *param_data[] = {view_name};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_stop_catch, param_name, param_data,  ARRAY_SIZE(param_name), NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_stop_catch);



    delete_cjson(pjson);

    return ret;

}


int ingenic_h264_view_client_start_display(char *view_name)
{
    char *param_name[] = {"view_name"};
    char *param_data[] = {view_name};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_start_display, param_name, param_data,  ARRAY_SIZE(param_name), NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_start_display);


    delete_cjson(pjson);

    return ret;
}

int ingenic_h264_view_client_stop_display(char *view_name)
{
    char *param_name[] = {"view_name"};
    char *param_data[] = {view_name};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_stop_display, param_name, param_data,  ARRAY_SIZE(param_name), NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_stop_display);


    delete_cjson(pjson);

    return ret;
}


int ingenic_h264_view_client_start_record(char *view_name, char *file_path)
{
    char *param_name[] = {"view_name", "file_path"};
    char *param_data[] = {view_name, file_path};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_start_record, param_name, param_data,  ARRAY_SIZE(param_name), NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_start_record);


    delete_cjson(pjson);

    return ret;

}

int ingenic_h264_view_client_stop_record(char *view_name)
{
    char *param_name[] = {"view_name"};
    char *param_data[] = {view_name};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_stop_record, param_name, param_data,  ARRAY_SIZE(param_name), NULL, NULL, 0);
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_stop_record);


    delete_cjson(pjson);

    return ret;
}


int ingenic_h264_view_client_set_area(char *view_name, int disp_w, int disp_h, int disp_x, int disp_y)
{
    char *str_name[] = {"view_name"};
    char *string[] = {view_name};

    char *num_name[] = {"disp_w", "disp_h", "disp_x", "disp_y"};
    int num[] = {disp_w, disp_h, disp_x, disp_y};

    char *pjson = create_cjson(INGENIC_VIEW_CMD_set_disp_cfg, str_name, string,  ARRAY_SIZE(str_name), num_name, num, ARRAY_SIZE(num_name));
    if (!pjson) {
        fprintf(stderr, "ingenic view client: faile to create view, create json err\n");
        return -1;
    }

    int ret = client_send_cmd(pjson, strlen(pjson));
    if (ret < 0)
        fprintf(stderr, "send %s err\n", INGENIC_VIEW_CMD_set_disp_cfg);


    delete_cjson(pjson);

    return ret;
}


int ingenic_h264_view_client_init(void)
{
    int ret;
    recv_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(recv_fd < 0){
        fprintf(stderr, "create socket fail!\n");
        return -1;
    }

    struct timeval tv_out;
    tv_out.tv_sec = 5;
    tv_out.tv_usec = 0;
    ret = setsockopt(recv_fd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    if(ret < 0) {
        fprintf(stderr, "hi_wifi_status: failed to set cli recv timeout\n");
        return -1;
    }

    struct sockaddr_un *ser_addr = &server_addr;
    memset(ser_addr, 0, sizeof(struct sockaddr_un));
    ser_addr->sun_family = AF_UNIX;
    strcpy(ser_addr->sun_path, INGENIC_VIEW_SERVER);

    struct sockaddr_un *cli_addr = &client_addr;
    memset(cli_addr, 0, sizeof(struct sockaddr_un));
    cli_addr->sun_family = AF_UNIX;


    char cli_path_name[108] = {0};
    sprintf(cli_path_name, "%s_%x_%lx", INGENIC_VIEW_CLIENT, getpid(), pthread_self());
    strcpy(cli_addr->sun_path, cli_path_name);

    ret = bind(recv_fd, (struct sockaddr *)cli_addr, sizeof(struct sockaddr_un));
    if (ret < 0) {
        fprintf(stderr, "failed to bind cli addr %s\n", strerror(errno));
        return -1;
    }

    return 0;
}


