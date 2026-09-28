#ifndef _V4L2_H264_DECODE_H_
#define _V4L2_H264_DECODE_H_


#ifdef  __cplusplus
extern "C" {
#endif

struct v4l2_h264_decoder_config {
    const char *video_path;
    unsigned int width;
    unsigned int height;
    unsigned int output_fmt;

    /* 以下参数为传出参数，调用 decoder open 后获取 */
    unsigned int linesize;    //解码输出的宽, 为128对齐
    unsigned int colunmsize;  //解码输出的高, 为16对齐
};

struct v4l2_h264_decoder;

/*
 * 解码输出nv12, 到给定返回地址, 返回地址如下:
 * y分量数据:out_mem[0],uv分量数据:out_mem[1]
*/
struct v4l2_h264_decoder *v4l2_h264_decoder_open(struct v4l2_h264_decoder_config *config);

int v4l2_h264_decoder_work(struct v4l2_h264_decoder *decoder, void *input_mem, unsigned int input_size,
                            void **output_mem);

int v4l2_h264_decoder_work_release(struct v4l2_h264_decoder *decoder);

int v4l2_h264_decoder_close(struct v4l2_h264_decoder *decoder);



/*
 * 解码输出nv12, 直通到指定地址, 输出地址如下:
 * y 分量数据：y_mem, uv分量数据：uv_mem,
 * 并且 y_mem、uv_mem 地址必须页对齐
*/
struct v4l2_h264_decoder *v4l2_h264_decoder_direct_open(struct v4l2_h264_decoder_config *config);

int v4l2_h264_decoder_direct_work(struct v4l2_h264_decoder *decoder, void *input_mem, unsigned int input_size,
                            void *y_mem, void *uv_mem);

int v4l2_h264_decoder_direct_close(struct v4l2_h264_decoder *decoder);

#ifdef  __cplusplus
}
#endif

#endif /* _V4L2_H264_DECODE_H_ */
