# mjpeg+h264单节点解析

根据 doc/开发使用说明/方案应用文档/AD100/mjpeg+h264单节点解析/指安双码流协议.pdf 

![image-20240103204432721](mjpeg+h264单节点解析.assets/image-20240103204432721.png)

对于指安的高清码流格式标识字段定义，这里使用下述结构体描述

```
typedef struct appn_field_format
{
    unsigned char *flag; // 标志码的低位值 (指安为0xE7、0xE8、0xE9)
    int flag_num; // 标志码个数

    int total_len_h_offset; // H26x 字段长度, 不包括标识码, 即从该字段到 h26x图像数据内容结束的长度
    int total_len_l_offset; // 具体见《指安双码流协议.pdf》 2.1节表格

    unsigned char *stream_format; // 视频流格式有效值
    int format_num; // 视频流格式有效值个数
    int format_offset; // 视频流格式在字段中的位置偏移

    int fps_offset; // 每秒帧数

    unsigned char *reserved; // 预留字段值 指安默认都为 0x00
    int reserved_len; // 预留字段长度
    int reserved_offset; // 预留字段在APPn字段中的位置偏移

    int h264x_len_h_offset; // h26x视频数据 长度高8位偏移
    int h264x_len_l_offset; // h26x视频数据 长度低8位偏移

    int header_len; // 除去标识码字段后，头部信息长。具体见《指安双码流协议.pdf》 2.1节表格
} appn_fmt;
```

解析得到的 H26x数据信息，使用下述结构体描述

```
typedef struct h26x_info {
    void *addr;
    int len;
} h26x;
```

下面是供参考的解析用例

```
typedef struct h26x_info {
    void *addr;
    int len;
} h26x;

typedef struct appn_field_format
{
    unsigned char *flag; // 标志码的低位值 (指安为0xE7、0xE8、0xE9)
    int flag_num; // 标志码个数

    int total_len_h_offset; // H26x 字段长度, 不包括标识码, 即从该字段到 h26x图像数据内容结束的长度
    int total_len_l_offset; // 具体见《指安双码流协议.pdf》 2.1节表格

    unsigned char *stream_format; // 视频流格式有效值
    int format_num; // 视频流格式有效值个数
    int format_offset; // 视频流格式在字段中的位置偏移

    int fps_offset; // 每秒帧数

    unsigned char *reserved; // 预留字段值 指安默认都为 0x00
    int reserved_len; // 预留字段长度
    int reserved_offset; // 预留字段在APPn字段中的位置偏移

    int h264x_len_h_offset; // h26x视频数据 长度高8位偏移
    int h264x_len_l_offset; // h26x视频数据 长度低8位偏移

    int header_len; // 除去标识码字段后，头部信息长。具体见《指安双码流协议.pdf》 2.1节表格
} appn_fmt;

/**
 * parse_h26x_info
 * 从获取到的一帧 UVC数据中解析出 H264信息，成功返回0，失败返回非0
 * @data: host侧获取到的一帧数据内容
 * @data_len: host侧获取到的一帧数据内容长度
 */
int parse_h26x_info(unsigned char *data, int data_len, appn_fmt *fmt, h26x *parse_info)
{
    unsigned char *pbuf = data;
    unsigned char *pbuf_end = data + data_len;
    int is_sos = 0;

    unsigned char c;
    int i;

    while(1) {
        if(is_sos || pbuf >= pbuf_end) {
            break;
        }

        c = *pbuf++;
        if(c != 0xff)
            continue;

        while((c = *pbuf++) == 0xff);
        if(c == 0) {
            continue;
        }

        switch(c) {
        case 0xda :
            /* BS left can be decoded by vpu.*/
            is_sos = 1;
            break;

        default :
            // 检查 标识码
            for (i = 0; i < fmt->flag_num; i++) {
                if (c == fmt->flag[i])
                    break;
            }
            if (i == fmt->flag_num)
                break;

            // 检查 视频流格式
            for (i = 0; i < fmt->format_num; i++) {
                if (pbuf[fmt->format_offset] == fmt->stream_format[i])
                    break;
            }
            if (i == fmt->format_num)
                break;

            // 检查 视频帧率
            if ((pbuf + fmt->header_len) >= pbuf_end)
                break;

            // 检查 保留信息
            for (i = 0; i < fmt->reserved_len; i++) {
                if (pbuf[fmt->reserved_offset+i] != fmt->reserved[i])
                    break;
            }
            if (i != fmt->reserved_len)
                break;

            // 记录 视频数据起始地址、视频数据长度
            parse_info->addr = (void *)(pbuf + fmt->header_len);
            parse_info->len = pbuf[fmt->h264x_len_h_offset] << 8 | pbuf[fmt->h264x_len_l_offset];

            printf("h26x format[%#x] fps[%d]\n", pbuf[fmt->format_offset], pbuf[fmt->fps_offset]);
            printf("h26x addr[%#x] video_data_len[%d]\n", parse_info->addr, parse_info->len);

            // 一帧 mjpeg数据中仅夹杂一帧 H26x数据
            return 0;
        }
    }

    return -ENODATA;
}

/**
 * decode_h26x_data_example
 * @data: host侧获取到的一帧数据内容
 * @data_len: host侧获取到的一帧数据内容长度
 */
void decode_h26x_data_example(unsigned char *data, int data_len)
{
    struct h26x_info parse_info;
    FILE *h26x_fp;
    int ret;

    // 相关字段匹配值
    unsigned char flag_list[] = {0xE7, 0xE8, 0xE9};
    unsigned char stream_format_list[] = {0x01, 0x02};
    unsigned char reserved_list[] = {0, 0, 0, 0, 0, 0};

    // 解析格式描述
    appn_fmt fmt = {
        // 《指安双码流协议.pdf》 2.1节 标识码信息
        .flag = flag_list,
        .flag_num = 3,

        // 数据长度
        .total_len_h_offset = 0,
        .total_len_l_offset = 1,

        // 视频信息
        .stream_format = stream_format_list,
        .format_num = 2,
        .format_offset = 2,

        // 视频帧率
        .fps_offset = 3,

        // 保留信息
        .reserved = reserved_list,
        .reserved_len = 6,
        .reserved_offset = 4,

        // 视频数据长度
        .h264x_len_h_offset = 10,
        .h264x_len_l_offset = 11,

        // 除标识码、视频数据字段的头部信息长
        .header_len = 12,
    };

    memset(&parse_info, 0, sizeof(h26x));
    ret = parse_h26x_info(data, data_len, &fmt, &parse_info);
    if (ret) {
        fprintf(stderr, "There is no h26x content in this mjpeg frame!\n");
        return;
    }

    h26x_fp = fopen("/usr/data/h26x_parse_data", "w");
    if (!h26x_fp) {
        fprintf(stderr, "failed to [%s]: %s %s\n", "create file", "h26x_data", strerror(errno));
        return;
    }
    fwrite(parse_info.addr, parse_info.len, 1, h26x_fp);
    fflush(NULL);
    fclose(h26x_fp);
}
```

