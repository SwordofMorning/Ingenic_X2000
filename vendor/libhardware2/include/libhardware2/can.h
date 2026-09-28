#ifndef __LIBHARDWARE2_CAN_H__
#define __LIBHARDWARE2_CAN_H__

#ifdef  __cplusplus
extern "C" {
#endif

/* 若afid设置为该值, 可接收所有帧 */
#define CAN_ALL_ACCEPT_AFID 0xFFFFFFFF

enum can_frame_mode {
    CAN_STANDARD,
    CAN_EXTENDED,
};

enum can_frame_type {
    CAN_REGULAR_DATA,
    CAN_REMOTE_REQUEST,
};

struct can_frame_cfg {
    int frm_id;
    int len;
    unsigned char data[8];
    enum can_frame_mode mode;
    enum can_frame_type type;
};

/**
 * @brief 获得 can 设备句柄
 * @param dev_path can 设备路径, 如/dev/can0
 * @return 成功返回设备句柄, 失败返回-1
 */
int can_open(char *dev_path);

/**
 * @brief 关闭 can 设备
 * @param fd 设备句柄
 * @return 无返回值
 */
void can_close(int fd);

/**
 * @brief 设置 can 设备传输速率
 * @param fd 设备句柄
 * @param rate 传输速率
 * @return 成功返回0, 失败返回负数
 */
int can_set_rate(int fd, int rate);

/**
 * @brief 使能 can 设备接收仲裁器, 共有 4 个, 设置单个仲裁器可接收的ID
 * @param fd 设备句柄
 * @param filter_num 接收仲裁器序号, 0-3
 * @param filter_id 接收仲裁器ID, standard:0x0-0x7FF, extended:0x0-0x1FFFFFFF
 *                  设置为 CAN_ALL_ACCEPT_AFID 表示该仲裁器可接受所有ID
 * @return 成功返回0, 失败返回负数
 *
 * example: filter_num  filter_id
 *              0         0x11
 *              1         0x34
 *              3         0xaace28
 *      仅接收 id 为 0x11/0x34/0x628/0xaace28 的帧
 */
int can_set_filter(int fd, int filter_num, int filter_id);

/**
 * @brief 失能 can 设备接收仲裁器, 共有 4 个, 停用单个仲裁器
 * @param fd 设备句柄
 * @param filter_num 接收仲裁器序号, 0-3
 * @return 成功返回0, 失败返回负数
 */
int can_put_filter(int fd, int filter_num);

/**
 * @brief 打印 can 设备接收仲裁器是否使能 及 可接收的ID
 * @param fd 设备句柄
 * @return 成功返回0, 失败返回负数
 */
int can_get_filter(int fd);

/**
 * @brief 使能 can 设备
 * @param fd 设备句柄
 * @return 成功返回0, 失败返回负数
 */
int can_enable(int fd);

/**
 * @brief can 设备发送一帧
 * @param fd can 设备句柄
 * @param cfg 帧信息结构体, 包含帧ID,帧数据长度,帧数据,帧模式,帧类型
 * @return 成功返回发送成功字节数, 失败返回负数
 */
int can_write(int fd, struct can_frame_cfg *cfg);

/**
 * @brief can 设备接收一帧
 * @param fd can 设备句柄
 * @param cfg 帧信息结构体, 包含帧ID,帧数据长度,帧数据,帧模式,帧类型
 * @return 成功返回接收成功字节数, 失败返回负数
 */
int can_read(int fd, struct can_frame_cfg *cfg);

/**
 * @brief can 设备循环接收并打印帧信息
 * @param fd can 设备句柄
 * @return 失败返回负数
 */
int can_dump(int fd);

/**
 * @brief 失能 can 设备
 * @param fd 句柄
 * @return 成功返回0, 失败返回负数
 */
int can_disable(int fd);

#ifdef  __cplusplus
}
#endif

#endif
