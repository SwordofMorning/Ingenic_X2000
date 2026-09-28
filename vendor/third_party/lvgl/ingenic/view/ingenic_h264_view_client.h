#ifndef __INGENIC_H264_VIEW_CLIENT_H__
#define __INGENIC_H264_VIEW_CLIENT_H__

int ingenic_h264_view_client_init(void);

int ingenic_h264_view_client_create(char *view_name, char *data);

int ingenic_h264_view_client_delete(char *view_name);

int ingenic_h264_view_client_set_area(char *view_name, int disp_w, int disp_h, int disp_x, int disp_y);

int ingenic_h264_view_client_start_catch(char *view_name);

int ingenic_h264_view_client_stop_catch(char *view_name);

int ingenic_h264_view_client_start_display(char *view_name);

int ingenic_h264_view_client_stop_display(char *view_name);

int ingenic_h264_view_client_start_record(char *view_name, char *file_path);

int ingenic_h264_view_client_stop_record(char *view_name);

#endif
