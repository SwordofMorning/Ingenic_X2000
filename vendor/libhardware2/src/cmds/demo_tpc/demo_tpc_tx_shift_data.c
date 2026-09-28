#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <libhardware2/mcu.h>


#include "ingenic_image.h"


#define SHIFT_LINES_BYTES  48

void Print_Picture(unsigned char startx,unsigned char  *picdata,unsigned short widthpix,unsigned short heightpix, void *dst, int device_w)
{
    unsigned short iline;//列变量
    unsigned char  widthbyte,ptbyte;//宽度字节/实际打印字节
    unsigned char  *onepixline = dst;

    widthbyte = widthpix / 8 + (widthpix % 8 ? 1 : 0);//计算图片宽多少字节
    if (startx > device_w - 1)
        startx = device_w - 1;
    //如果宽度超过48字节，将图片裁剪到48字节

    if ((startx + widthbyte) > device_w)
        ptbyte = device_w - startx;
    else
        ptbyte = widthbyte;

    //行扫描并打印一行
        //起始位置之前的 存入空数据
    for (iline = 0;iline < startx; iline++)
        onepixline[iline] = 0x00;
    for (iline = 0; iline < ptbyte; iline++)
        onepixline[iline+startx]=*(picdata++);

    //跳过裁剪掉的部分
    if (ptbyte != widthbyte) {
        picdata += (startx + widthbyte - device_w);
    } else {
    //存入空数据
        for(iline = 0; iline < device_w - startx - ptbyte; iline++)
            onepixline[iline + startx + ptbyte] = 0x00;
    }
}


static int wait_mcu_is_ready(int mcu_fd)
{
    int ret;
    char read_buf[256] = {0};
    ret = mcu_read_str_timeout(mcu_fd, read_buf, sizeof(read_buf), -1);
    if (ret <= 0)
        return -1;

    if (strcmp(read_buf, "mcu_tpc_ready") == 0)
        return 0;

    return -1;

}

int main(void)
{
    int mcu_fd = mcu_open();
    if (mcu_fd < 0) {
        fprintf(stderr, "failed to open mcu\n");
        return -1;
    }

    int width = (gImage_ingenic_image[2] << 8) + gImage_ingenic_image[3];
    int height = (gImage_ingenic_image[4] << 8) + gImage_ingenic_image[5];


    int ret;

    unsigned char *src_data = &gImage_ingenic_image[6];
    unsigned char *dst_data = malloc(SHIFT_LINES_BYTES);

    int cnt = height;

    int widthbyte = width / 8 + (width % 8 ? 1 : 0);

    while(cnt) {
        /*准备好一行 图片数据*/
        Print_Picture(0, src_data, width, height, dst_data, SHIFT_LINES_BYTES);
        src_data += widthbyte;

        /*等待小核准备好*/
        ret = wait_mcu_is_ready(mcu_fd);
        if (ret < 0)
            continue;

        /*写一行数据*/
        mcu_write_data(mcu_fd, dst_data, SHIFT_LINES_BYTES);

        cnt--;
    }

    /*等待小核准备好收数据*/
    wait_mcu_is_ready(mcu_fd);

    /*发送结束信息*/
    mcu_write_data(mcu_fd, "tpc_data_end", strlen("tpc_data_end"));

    mcu_close(mcu_fd);

    return 0;
}