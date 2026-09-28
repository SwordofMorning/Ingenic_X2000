# x1600 sensor 出图格式配置
此文档在介绍如何配置 sensor 的出图格式。

camera 的出图方式为 mipi/dvp -> cim -> ddr。

# 1 x1600 camera 控制器功能说明

x1600 的 camera 控制器硬件框架：

![image-20221208160200077](img/1.png)

即支持 mipi 和 dvp 接口，在帧写回时，即 Wrbk dma 模块，可以是 sensor 出什么格式的图，就以什么格式写回内存，也可以通过硬件处理，只将图片以 Y8 的格式写回内存，这样可以减少内存的占用。

x1600 的 camera 控制器支持的输入输出格式：

![image-20221208155413794](img/2.png)



# 2. sensor 配置用到的宏
帧写回格式：即 sensor 出图后经过 cim，然后将一帧图片写回到内存时，保存的格式。有以下两种方式：

* SENSOR\_DATA\_DMA\_MODE\_RAW：保留 sensor 出图格式，sensor 出什么格式的图，帧写回就是什么格式。
* SENSOR\_DATA\_DMA\_MODE\_GREY：camera 控制器会根据 sensor 的出图格式进行装换，即将接收到的像素格式转换为 Y8，所以帧写回格式为 Y8。



表2-1 使用举例：对于 sensor 的出图格式为 10 bit 的 mono，需要修改出图格式为 SENSOR_PIXEL_FMT_Y10_1X10；如果想要 camera 的出图为 mono，则需修改帧写回格式为 SENSOR_DATA_DMA_MODE_RAW，如图 2-1；如果想要 camera 的出图为 Y8，这需修改帧写回格式为 SENSOR_DATA_DMA_MODE_GREY，如图2-2：



<img src="img/3.png" alt="image-20221208180534417" style="zoom: 50%;" />                            <img src="img/4.png" alt="image-20221208180802734" style="zoom: 50%;" />

​												图 2-1																													图2-2



​																				表 2-1 x1600 sensor 出图配置对照表

<table>
    <tr>
        <th colspan="2">sensor 出图格式</th>
        <td colspan="2">camera 出图格式(与帧写回格式相关)</td>
    </tr>
    <tr>
        <th>格式</th>
        <td>对应宏定义</td>
        <td>SENSOR_DATA_DMA_MODE_RAW</td>
        <td>SENSOR_DATA_DMA_MODE_GREY</td>
    </tr>
    <tr>
        <th>mono</th>
        <td>SENSOR_PIXEL_FMT_Y8_1X8<br>SENSOR_PIXEL_FMT_Y10_1X10<br>SENSOR_PIXEL_FMT_Y12_1X12</td>
        <td>mono (对于 10/12bit 的 mono，由控制器裁剪为 8bit )<br>(控制器裁剪：对于 mono10：右移 2 位；对于 mono12：右移 4 位)</td>
        <td>Y8</td>
    </tr>
    <tr>
        <th>raw8</th>
        <td>SENSOR_PIXEL_FMT_SBGGR8_1X8<br>SENSOR_PIXEL_FMT_SGBRG8_1X8<br>SENSOR_PIXEL_FMT_SGRBG8_1X8<br>SENSOR_PIXEL_FMT_SRGGB8_1X8</td>
        <td>raw8</td>
        <td>Y8</td>
    </tr>
    <tr>
        <th>raw10</th>
        <td>SENSOR_PIXEL_FMT_SBGGR10_1X10<br>SENSOR_PIXEL_FMT_SGBRG10_1X10<br>SENSOR_PIXEL_FMT_SGRBG10_1X10<br>SENSOR_PIXEL_FMT_SRGGB10_1X10</td>
        <td>raw16（软件扩展： 左移 6 位，补 6 个 0）</td>
        <td>不支持</td>
    </tr>
    <tr>
        <th>raw12</th>
        <td>SENSOR_PIXEL_FMT_SBGGR12_1X12<br>SENSOR_PIXEL_FMT_SGBRG12_1X12<br>SENSOR_PIXEL_FMT_SGRBG12_1X12<br>SENSOR_PIXEL_FMT_SRGGB12_1X12</td>
        <td>raw16（软件扩展：左移 4 位，补 4 个 0）</td>
        <td>不支持</td>
    </tr>
    <tr>
        <th>raw10</th>
        <td>SENSOR_PIXEL_FMT_SBGGR10_ALAW8_1X8<br>SENSOR_PIXEL_FMT_SGBRG10_ALAW8_1X8<br>SENSOR_PIXEL_FMT_SGRBG10_ALAW8_1X8<br>SENSOR_PIXEL_FMT_SRGGB10_ALAW8_1X8</td>
        <td>raw8（控制器裁剪：右移2 位）</td>
        <td>Y8</td>
    </tr>
    <tr>
        <th>raw12</th>
        <td>SENSOR_PIXEL_FMT_SBGGR12_ALAW8_1X8<br>SENSOR_PIXEL_FMT_SGBRG12_ALAW8_1X8<br>SENSOR_PIXEL_FMT_SGRBG12_ALAW8_1X8<br>SENSOR_PIXEL_FMT_SRGGB12_ALAW8_1X8</td>
        <td>raw8（控制器裁剪：右移4 位）</td>
        <td>Y8</td>
    </tr>
    <tr>
    <th>YUV422</th>
    <td>SENSOR_PIXEL_FMT_UYVY8_2X8<br>SENSOR_PIXEL_FMT_VYUY8_2X8<br>SENSOR_PIXEL_FMT_YUYV8_2X8<br>SENSOR_PIXEL_FMT_YVYU8_2X8</td>
    <td>YUV422</td>
    <td>Y8</td>
</tr>
<tr>
    <th>RGB565</th>
    <td>SENSOR_PIXEL_FMT_RGB565_2X8_BE<br>SENSOR_PIXEL_FMT_RGB565_2X8_LE</td>
    <td>RGB565</td>
    <td>Y8</td>
</tr>
<tr>
    <th>RGB888</th>
    <td>SENSOR_PIXEL_FMT_RGB888_1X24<br>SENSOR_PIXEL_FMT_RBG888_1X24<br>SENSOR_PIXEL_FMT_BGR888_1X24<br>SENSOR_PIXEL_FMT_GBR888_1X24</td>
    <td>待测试</td>
    <td>待测试</td>
</tr>
</table>


# 3. 配置方法

1. 修改 sensor 出图的格式，不同的 sensor 配置的寄存器不同，具体查看 sensor 手册
2. 修改驱动
在 sensor 驱动文件中修改 sensor 属性，以 sc031 为例：
根据 sensor 出图格式，还有想要的 camera 出图格式，对照表 2-1 修改驱动文件：module\_driver/devices/camera/x1600/sc031\_mipi/sc031\_mipi.c，如下：

<img src="img/5.png" alt="image-20221208180534417" style="zoom:80%;" />

# 4. 内存大小计算

这个内存大小计算主要是计算运行 camera 时，需要 alloc 的内存空间。

## 4.1 计算方法

在运行 camera 时，主要开辟内存的计算公式为：

**size** = `align( mem_cnt * frame_lumi_alloc_size,  4096）`

公式解释：

* 所有运算都为整数运算，单位为字节， sensor 分辨率为 width \* hight。

* align 表示对齐运算，公式为：`align（x，n）= (((x) + (n) - 1) - ((x) + (n) - 1) % (n))` ，表示 x 对齐到 n

* `frame_lumi_alloc_size = align( frame_align_size + lumi_size, 4096）`

* `frame_align_size = align( line_length * pixel_bytes * width, 4）`

* mem\_cnt = cam\_mem\_cnt + 1，而 cam\_mem\_cnt 是可以在 iconfigure 中配置的，其默认值为 2，mem\_cnt 为实际的帧缓冲数。

  修改方式：

![16](img/6.png)

* lumi\_size 为亮度区域的大小，是固定值，为 9 个字长，在 32 位主机中为 36 字节

* pixel\_bytes：每个像素点占多少个字节

  <table>
      <tr>
          <th>sensor 出图格式</th>
          <td>帧写回格式</td>
          <td>pixel_bytes 的 取值</td>
      </tr>
      <tr>
          <td rowspan="2">YUV422</td>
          <td>SENSOR_DATA_DMA_MODE_RAW</td>
          <td>2</td>
      </tr>
      <tr>
          <td>SENSOR_DATA_DMA_MODE_GREY</td>
          <td>1</td>
      </tr>
      <tr>
          <td rowspan="2">mono</td>
          <td>SENSOR_DATA_DMA_MODE_RAW</td>
          <td>1</td>
      </tr>
      <tr>
          <td>SENSOR_DATA_DMA_MODE_GREY</td>
          <td>1</td>
      </tr>
      <tr>
          <td rowspan="2">raw8</td>
          <td>SENSOR_DATA_DMA_MODE_RAW</td>
          <td>1</td>
      </tr>
      <tr>
          <td>SENSOR_DATA_DMA_MODE_GREY</td>
          <td>1</td>
      </tr>
      <tr>
          <td rowspan="2">raw10</td>
          <td>SENSOR_DATA_DMA_MODE_RAW</td>
          <td>1 (每一行的像素点数扩增到 2 倍)</td>
      </tr>
      <tr>
          <td>SENSOR_DATA_DMA_MODE_GREY</td>
          <td>1</td>
      </tr>
      <tr>
          <td rowspan="2">RGB565</td>
          <td>SENSOR_DATA_DMA_MODE_RAW</td>
          <td>2</td>
      </tr>
      <tr>
          <td>SENSOR_DATA_DMA_MODE_GREY</td>
          <td>1</td>
      </tr>
      <tr>
          <td rowspan="2">RGB888</td>
          <td>SENSOR_DATA_DMA_MODE_RAW</td>
          <td>3</td>
      </tr>
      <tr>
          <td>SENSOR_DATA_DMA_MODE_GREY</td>
          <td>1</td>
      </tr>
  </table>

  > 其他在表格中没有列出来的 sensor 出图格式，pixel\_bytes 都等于 1

* line\_length：一帧图片的一行像素有多少个像素点
   1. 当 sensor 出图为 raw10，camera 出图为 raw16 且 sensor 接口为 mipi 时，`line_length= width * 2`
   2. 当 sensor 出图为 raw12，camera 出图为 raw16 且 sensor 接口为 mipi 时，`line_length= width * 2`
   3. 其他配置 line\_length = width

## 4.2 举例
以 sc031 mipi 为例：

sensor 分辨率为 640 \* 480。

帧写回格式为 SENSOR_DATA_DMA\_MODE\_RAW，sensor 出图格式为 SENSOR\_PIXEL\_FMT\_SBGGR10\_1X10。

```c
line_length = 640 * 2 = 1280

pixel_bytes = 1

frame_align_size = align(line_length * pixel_bytes * hight)
    							    = align(1280 * 480 * 1, 4) = 614400

frame_lumi_alloc_size = align(frame_align_size, 4096)
    				  							= align(614400 + 36,4096) = 618496

size = align(frame_lumi_alloc_size, 4096)
	     = align(618496 * (2+1), 4096) = 1855488
```

