#ifndef __TIZIANO_TIZIANO_CORE_TUNING_H__
#define __TIZIANO_TIZIANO_CORE_TUNING_H__

#include "tiziano-isp.h"
#include "tiziano-core-ctrl.h"

typedef struct tisp_custom_effect {
    unsigned char brightness;
    unsigned char sharpness;
    unsigned char contrast;
    unsigned char saturation;
    unsigned int brightness_ori_day[20];
    unsigned int brightness_ori_night[20];
    unsigned int saturation_ori_day[9];
    unsigned int sharpen_ori_day[36];
    unsigned int sharpen_ori_night[36];

} tisp_custom_effect_t;

typedef struct tisp_mdns_ratio {
    int dysad_thres_def_day[9];
    int dysta_thres_def_day[9];
    int dypbt_thres_def_day[9];
    int dywei_max_def_day[9];
    int dywei_min_def_day[9];
    int dysad_thres_def_night[9];
    int dysta_thres_def_night[9];
    int dypbt_thres_def_night[9];
    int dywei_max_def_night[9];
    int dywei_min_def_night[9];
} tisp_mdns_ratio_t;

typedef struct tisp_ncu_info {
    uint32_t width;
    uint32_t height;
    uint32_t sta_y_block_size;
    uint32_t sta_y_stride;
    uint32_t sta_y_buf_size;
} tisp_ncu_info_t;

// typedef struct tisp_gamma_lut{
//     unsigned int gamma[129];
// } tisp_gamma_lut_t;

// typedef struct tisp_3a_weight{
//     unsigned int weight[225];
// } tisp_3a_weight_t;

// typedef struct tisp_zone_info{
//     unsigned int zone[15][15];
// } tisp_zone_info_t;

typedef struct tisp_ev_attr {
    unsigned int ae_manual;
    unsigned int ev;
    unsigned int integration_time;
    unsigned int min_integration_time;
    unsigned int max_integration_time;
    unsigned int integration_time_us;
    unsigned int sensor_again;
    unsigned int max_sensor_again;
    unsigned int sensor_dgain;
    unsigned int max_sensor_dgain;
    unsigned int isp_dgain;
    unsigned int max_isp_dgain;
    unsigned int total_gain;
} tisp_ev_attr_t;

// typedef struct tisp_ae_ex_min {
//     unsigned int min_it;
//     unsigned int min_again;
// } tisp_ae_ex_min_t;

// typedef union tisp_module_control {
//     unsigned int key;
//     struct {
//         unsigned int bitBypassDPC : 1; /* [0]  */
//         unsigned int bitBypassGIB : 1; /* [1]  */
//         unsigned int bitBypassLSC : 1; /* [2]  */
//         unsigned int bitBypassAWB : 1; /* [3]  */
//         unsigned int bitBypassADR : 1; /* [4]  */
//         unsigned int bitBypassDMSC : 1; /* [5]  */
//         unsigned int bitBypassCCM : 1; /* [6]  */
//         unsigned int bitBypassGAMMA : 1; /* [7]  */
//         unsigned int bitBypassDEFOG : 1; /* [8]  */
//         unsigned int bitBypassCLM : 1; /* [9]  */
//         unsigned int bitBypassYSHARPEN : 1; /* [10]  */
//         unsigned int bitBypassMDNS : 1; /* [11]  */
//         unsigned int bitBypassSDNS : 1; /* [12]  */
//         unsigned int bitBypassHLDC : 1; /* [13]  */
//         unsigned int bitBypassTP : 1; /* [14]  */
//         unsigned int bitBypassFONT : 1; /* [15]  */
//         unsigned int bitRsv : 15; /* [16 ~ 30]  */
//         unsigned int bitRsv2 : 1; /* [31]  */
//     };
// } tisp_module_control_t;

typedef struct tisp_autozoom_control {
    int32_t zoom_chx_en[3];
    int32_t zoom_left[3];
    int32_t zoom_top[3];
    int32_t zoom_width[3];
    int32_t zoom_height[3];
} tisp_autozoom_attr_t;

typedef struct tisp_core_tuning {
    int day_night;
    int hflip;
    int vflip;
    int flicker_hz;
    tisp_custom_effect_t custom_eff;
    tisp_mdns_ratio_t mdns_ratio;

    void *core;     /*tisp_core_t in tiziano_core.h*/
} tisp_core_tuning_t;

int tisp_enable_tuning(void);
void tisp_disable_tuning(void);
tisp_core_tuning_attr* tisp_get_tuning(void);

int32_t tisp_day_or_night_s_ctrl(int vinum, TISP_MODE_DN_E dn);
TISP_MODE_DN_E tisp_day_or_night_g_ctrl(int vinum);
int tisp_set_fps(int vinum, int fps);

int tisp_ae_face_get(int vinum, tisp_face_t *attr);
int tisp_ae_face_set(int vinum, tisp_face_t *attr);
int tisp_awb_face_get(int vinum, tisp_face_t *attr);
int tisp_awb_face_set(int vinum, tisp_face_t *attr);
void tisp_set_brightness(int vinum, unsigned char brightness);
void tisp_set_sharpness(int vinum, unsigned char sharpness);
void tisp_set_saturation(int vinum, unsigned char saturation);
void tisp_set_contrast(int vinum, unsigned char contrast);
void tisp_set_hue(int vinum, unsigned char hue);
int tisp_set_csc_attr(int vinum, tisp_csc_attr_t *attr);
void tisp_get_csc_attr(int vinum, tisp_csc_attr_t *attr);
int tisp_s_ccm_attr(int vinum, tisp_ccm_attr_t *ccm_attr);
int tisp_g_ccm_attr(int vinum, tisp_ccm_attr_t *ccm_attr);
int tisp_s_Gamma(int vinum, tisp_gamma_attr_t *gammas);
int tisp_g_Gamma(int vinum, tisp_gamma_attr_t *gammag);
void tisp_s_module_control(int vinum, tisp_module_control_t top);
void tisp_g_module_control(int vinum, tisp_module_control_t *top);
int tisp_s_antiflick(int vinum, tisp_anfiflicker_attr_t *attr);
int tisp_g_antiflick(int vinum, tisp_anfiflicker_attr_t *attr);
int32_t tisp_s_hv_flip(int vinum, tisp_mirr_flip_t *state);
int32_t tisp_g_hv_flip(int vinum, tisp_mirr_flip_t *state);
int tisp_s_autozoom_control(int vinum, tisp_autozoom_attr_t *autozoom_attr);
int tisp_g_autozoom_control(int vinum, tisp_autozoom_attr_t *autozoom_attr);
int tisp_s_mscaler_mask_block_attr(int vinum, tisp_mask_block_attr_t *attr);
int tisp_g_mscaler_mask_block_attr(int vinum, tisp_mask_block_attr_t *attr);
int tisp_s_osd_attr(int vinum, tisp_osd_attr_t *attr);
int tisp_g_osd_attr(int vinum, tisp_osd_attr_t *attr);
int tisp_s_osd_block_attr(int vinum, tisp_osd_block_attr_t *attr);
int tisp_g_osd_block_attr(int vinum, tisp_osd_block_attr_t *attr);
int tisp_s_draw_block_attr(int vinum, tisp_draw_block_attr_t *attr);
int tisp_g_draw_block_attr(int vinum, tisp_draw_block_attr_t *attr);
int tisp_s_ae_scence_attr(int vinum, tisp_ae_scence_attr_t *attr);
int tisp_g_ae_scence_attr(int vinum, tisp_ae_scence_attr_t *attr);
int tisp_g_af_metric_attr(int vinum, tisp_af_metric_info_t *metrics);
int tisp_g_ae_exprinfo_attr(int vinum, tisp_ae_exprinfo_t *attr);
int tisp_g_awb_attr(int vinum, tisp_awb_attr_t *attr);

int tisp_s_wdr_en(int vinum, int enable, int bayer);
int tisp_g_wdr_en(int *enable);
void wdr_init_mine(void);
int tisp_s_af_weight_attr(int vinum, tisp_3a_weight_t *attr);
int tisp_g_af_weight_attr(int vinum, tisp_3a_weight_t *attr);
int tisp_s_awb_weight_attr(int vinum, tisp_3a_weight_t *attr);
int tisp_g_awb_weight_attr(int vinum, tisp_3a_weight_t *attr);
int tisp_g_awb_attr(int vinum, tisp_awb_attr_t *attr);
int tisp_s_awb_attr(int vinum, tisp_awb_attr_t *attr);
int tisp_g_awb_statis_attr(int vinum, tisp_wb_statis_info_t *attr);
int tisp_g_awb_global_statis_attr(int vinum, tisp_awb_global_statics_t *attr);
int tisp_s_ae_exprinfo_attr(int vinum, tisp_ae_exprinfo_t *attr);
int tisp_g_ae_statis_attr(int vinum, tisp_ae_statis_info_t *attr);
int tisp_g_af_statis_attr(int vinum, tisp_af_statis_info_t *attr);
int tisp_s_ae_weight_attr(int vinum, tisp_ae_weight_t *attr);
int tisp_g_ae_weight_attr(int vinum, tisp_ae_weight_t *attr);
int tisp_s_statis_config_attr(int vinum, tisp_statis_config_t *attr);
int tisp_g_statis_config_attr(int vinum, tisp_statis_config_t *attr);
int tisp_s_module_ratio_attr(int vinum, tisp_module_ratio_t *attr);
int tisp_g_module_ratio_attr(int vinum, tisp_module_ratio_t *attr);
int32_t tisp_g_csccr_mode(int32_t vinum, tisp_csccr_mode_t *csccr_attr);
int32_t tisp_s_csccr_mode(int32_t vinum, tisp_csccr_mode_t *csccr_attr);
int tisp_switch_bin(int vinum, tisp_bin_t *attr);
int tisp_core_switch_bin(int num, tisp_bin_t *attr);
void tisp_msca_Shd_ctrl(int vinum);
uint32_t tisp_msca_state(void);
void tisp_ae_algo_handle(int vinum, tisp_ae_algo_attr_self_t *ae_attr);
void tisp_ae_algo_init(int vinum, int enable, tisp_ae_algo_init_self_t *ae_init);
void tisp_awb_algo_handle(int vinum, tisp_awb_algo_attr_self_t *awb_attr);
void tisp_awb_algo_init(int vinum, int enable, tisp_awb_algo_init_self_t *awb_init);
int tisp_get_antiflicker_step(int vinum, uint16_t *gdeflick_lut, uint16_t *gnodes);
void tisp_get_ae_tgain(int vinum, unsigned int *tgain);
void tisp_ae_algo_deinit(int vinum);
void tisp_awb_algo_deinit(int vinum);
int32_t tisp_set_wdr_output_mode(int vinum, tisp_wdr_output_mode_t *mode);
int32_t tisp_get_wdr_output_mode(int vinum, tisp_wdr_output_mode_t *mode);
int32_t tisp_set_frame_drop(int vinum, int chx, tisp_frame_drop_t *tfd);
int32_t tisp_get_frame_drop(int vinum, int chx, tisp_frame_drop_t *tfd);
int32_t tisp_set_scaler_level_control(int vinum, tisp_scaler_opt_attr_t *scaler_opt);
int32_t tisp_msca_crop(uint8_t chx);
int32_t tisp_lsc_hvflip(int32_t width, int32_t height, int32_t flip_en, int32_t mirror_en);
int32_t tisp_mdns_addr_alloc(int32_t vinum, uint32_t *vaddr, uint32_t *paddr);
int32_t tisp_mdns_addr_free(int32_t vinum);
int32_t tisp_s_ae_explist(int32_t vinum, tisp_ae_explist_t *elist);
int32_t tisp_g_ae_explist(int32_t vinum, tisp_ae_explist_t *elist);
uint32_t tisp_sync_ivdc_state(int vinum, int dm_state);
int32_t tisp_s_raw_rw_control(int32_t vinum, tisp_raw_rw_control_t *rctrl);
int32_t tisp_g_raw_rw_handler(int32_t vinum, uint32_t *addr);
int32_t tisp_s_raw_rw_handler(int32_t vinum, uint32_t *addr);
int32_t tisp_s_raw_row_control(int32_t vinum, tisp_raw_row_control_t *rctrl);
int32_t tisp_s_coefft_wb(int vinum, tisp_bcsh_offset_rgb_attr_t *attr);
int32_t tisp_g_coefft_wb(int vinum, tisp_bcsh_offset_rgb_attr_t *attr);
#endif
