
# nor_flash脱机烧录多型号兼容

## 1 概述

不同的nor flash指令有差异,目前是通过烧录工具识别到nor flash id再把对应的参数信息写入flash,flash参数匹配是在烧录工具完成. 

如果需要脱机烧录,需使用烧录工具合并镜像功能,把flash参数信息和分区信息烧录打包起来,但是烧录工具只支持指定一颗flash型号.

所以默认功能不支持一个固件兼容多款nor flash.

我们通过把需要兼容的nor flash信息烧录到flash里面,上电后通过通用读取nor flash信息再对比nor flash id实现动态匹配参数.

## 2 烧录工具打包脱机烧录镜像

![1](img/1.png)

以AD100为例在配置界面添加nor id 和 参数偏移点击合并镜像,会在attribute的镜像目录下生成image.bin文件.

nor id 添加主用的flash id信息.

参数偏移: 

​	参考uboot代码 include/configs/ad100_base_common.h

​	\#define CONFIG_SPIFLASH_PART_OFFSET   0x5800   



### 3 生成额外兼容的nor flash信息

上面的默认方式只支持的一款nor flash参数,如果需要多款nor flash兼容需要生成兼容信息烧录到flash.

nor flash 信息用struct spi_nor_info结构体存储,可以通过烧录工具导出flash信息添加一下代码的nor_info_array结构体,或者根据nor 芯片手册整理出结构体信息.

![2](img/2.png)

会在烧录工具目录下生成norinfo.json文件,里面就包含nor flash结构体信息和分区信息.



以下是生成nor flash动态匹配信息的代码,需要手动修改nor_info_array数组添加需要适配的flash信息

```c
/* write_norinfo.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

struct spi_nor_cmd_info {
	unsigned short cmd;
	unsigned char dummy_byte;
	unsigned char addr_nbyte;
	unsigned char transfer_mode;
};

struct spi_nor_st_info {
	unsigned short cmd;
	unsigned char bit_shift;
	unsigned char mask;
	unsigned char val;
	unsigned char len;	/* length of byte to operate from register */
	unsigned char dummy;
};

struct spi_nor_info {
	unsigned char name[32];
	unsigned int id;

	struct spi_nor_cmd_info read_standard;
	struct spi_nor_cmd_info read_quad;

	struct spi_nor_cmd_info write_standard;
	struct spi_nor_cmd_info write_quad;

	struct spi_nor_cmd_info sector_erase;

	struct spi_nor_cmd_info wr_en;
	struct spi_nor_cmd_info en4byte;
	struct spi_nor_st_info	quad_set;
	struct spi_nor_st_info	quad_get;
	struct spi_nor_st_info	busy;

	unsigned short quad_ops_mode;
	unsigned short addr_ops_mode;

	unsigned int tCHSH;      /* hold */
	unsigned int tSLCH;      /* setup */
	unsigned int tSHSL_RD;   /* interval */
	unsigned int tSHSL_WR;

	unsigned int chip_size;
	unsigned int page_size;
	unsigned int erase_size;

	unsigned char chip_erase_cmd;
};

struct mini_spi_nor_info {
	unsigned char name[32];
	unsigned int id;

	struct spi_nor_cmd_info read_standard;
	struct spi_nor_cmd_info read_quad;
	struct spi_nor_cmd_info wr_en;
	struct spi_nor_cmd_info en4byte;

	struct spi_nor_st_info	quad_set;
	struct spi_nor_st_info	quad_get;
	struct spi_nor_st_info	busy;

	unsigned short quad_ops_mode;
	unsigned short addr_ops_mode;

	unsigned int chip_size;
	unsigned int page_size;
	unsigned int erase_size;

};

struct spi_nor_info nor_info_array[] =
{
    {
        .name = "GD25Q127CSIG",
        .id = 0xc84018,
        .read_standard ={.cmd=0x3,.dummy_byte=0x0,.addr_nbyte=0x3,.transfer_mode=0x0},
        .read_quad     ={.cmd=0x6b,.dummy_byte=0x8,.addr_nbyte=0x3,.transfer_mode=0x5},
        .write_standard={.cmd=0x2,.dummy_byte=0x0,.addr_nbyte=0x3,.transfer_mode=0x0},
        .write_quad    ={.cmd=0x32,.dummy_byte=0x0,.addr_nbyte=0x3,.transfer_mode=0x5},
        .sector_erase  ={.cmd=0x52,.dummy_byte=0x0,.addr_nbyte=0x3,.transfer_mode=0x0},
        .wr_en         ={.cmd=0x6,.dummy_byte=0x0,.addr_nbyte=0x0,.transfer_mode=0x0},
        .en4byte       ={.cmd=0xb7,.dummy_byte=0x0,.addr_nbyte=0x0,.transfer_mode=0x0},
        .quad_set={.cmd=0x31,.bit_shift=0x1,.mask=0x1,.val=0x1,.len=0x1,.dummy=0x0},
        .quad_get={.cmd=0x35,.bit_shift=0x1,.mask=0x1,.val=0x1,.len=0x1,.dummy=0x0},
        .busy    ={.cmd=0x5,.bit_shift=0x0,.mask=0x1,.val=0x0,.len=0x1,.dummy=0x0},
        .quad_ops_mode=1,
        .addr_ops_mode=0,
        .tCHSH=5,
        .tSLCH=5,
        .tSHSL_RD=20,
        .tSHSL_WR=20,
        .chip_size=16777216,
        .page_size=256,
        .erase_size=32768,
        .chip_erase_cmd=0x60,
    },
};

struct spi_nor_info_tag {
    char tag[8];
    int array_size;
};

int main(int argc, char *argv[])
{
    FILE *file = stdout;
    int ret = 0;

    if (argc > 2) {
        fprintf(stderr, "%s: error: too many args!\n"
                        "usage: %s [output_file]\n"
                        "       if output_file not set, write to stdout\n",
                 argv[0], argv[0]);
        return -1;
    }

    if (argv[1]) {
        file = fopen(argv[1], "w");
        if (!file) {
            fprintf(stderr, "%s: failed to open file: %s [%s]\n",
                 argv[0], strerror(errno), argv[1]);
            return -1;
        }
    }

    struct spi_nor_info_tag nor_tag = {
        .tag = "nor_tag",
        .array_size = sizeof(nor_info_array)/sizeof(nor_info_array[0]),
    };

    ret = fwrite(&nor_tag, 1, sizeof(nor_tag), file);
    if (ret != sizeof(nor_tag)) {
        fprintf(stderr, "%s: failed to write nor tag: %s [%s]\n",
                 argv[0], strerror(errno), argv[1]);
        ret = -1;
        goto close_file;
    }

    ret = fwrite(nor_info_array, 1, sizeof(nor_info_array), file);
    if (ret != sizeof(nor_info_array)) {
        fprintf(stderr, "%s: failed to write nor info: %s [%s]\n",
                 argv[0], strerror(errno), argv[1]);
        ret = -1;
        goto close_file;
    }

    ret = 0;

close_file:
    if (file != stdout)
        fclose(file);

    return ret;
}

```

```shell
gcc write_norinfo.c -o write_norinfo  #编译代码
./write_norinfo norinfo.bin		#生成nor info信息
```

### 4 烧录额外兼容的nor flash信息

烧录额外的兼容的nor flash一般有三种方式.

**1.烧录到spl固件里面的后1K的位置,需要注意SPL固件是否使用到这个位置.**

```shell
#/bin/sh

#dd_to_file.sh

# 获得字符串中的第几个word
get_word()
{
    local str="$1"
    local n=$2
    local i=0

    for word in $str
    do
        if [ $i = $n ]; then
            echo $word
            return 0
        fi
        let i=i+1
    done

    return 1
}

# 获得文件大小
size_file()
{
    local file=$1
    local result

    result="`ls -l -n -L $file`"
    if [ "$?" != "0" ]; then
        echo "ls failed: $file" 1>&2
        return 1
    fi

    result=`get_word "$result" 4`
    if [ "result" = "" ]; then
        echo "ls failed 2: $file" 1>&2
        return 1
    fi
    echo $result
}

infile=$1
outfile=$2
offset=$3

if [ "$3" == "" ] || [ "$4" != "" ]; then
    echo "error: too few or too many args" 1>&2
    echo "usage: $0 infile outfile offset" 1>&2
    exit 1
fi

if [ ! -e $infile ];then
    echo "error: $infile not exist" 1>&2
    exit 1
fi
if [ ! -e $outfile ];then
    touch $outfile
    if [ ! -e $outfile ]; then
        echo "error: can't create outfile $outfile" 1>&2
        exit 1
    fi
fi

infile_size=`size_file "$infile"`
outfile_size=`size_file "$outfile"`
tmp_file=`basename $outfile`

if [ $offset -gt $outfile_size ]; then
    echo "error: $offset is greater than outfile size" 1>&2
    exit 1
fi

dd if=$outfile of=/tmp/$tmp_file.file0 bs=1 count=$offset status=none
if [ $? != 0 ]; then
    echo "error failed to create file0: /tmp/$tmp_file.file0" 1>&2
    exit 1
fi

let skip_size=$offset+$infile_size
if [ $skip_size -gt $outfile_size ]; then
    let skip_size=$outfile_size
fi
let file2_size=$outfile_size-$skip_size

dd if=$outfile of=/tmp/$tmp_file.file2 skip=$skip_size bs=1 count=$file2_size status=none
if [ $? != 0 ]; then
    echo "error failed to create file0: /tmp/$tmp_file.file2" 1>&2
    exit 1
fi

cat /tmp/$tmp_file.file0 $infile /tmp/$tmp_file.file2 > $outfile

rm /tmp/$tmp_file.file0 /tmp/$tmp_file.file2
```

```shell
#以ad100为例  spl固件是24K大小 用上面的shell脚本把额外norinfo信息写到spl的23K的位置
./dd_to_file.sh norinfo.bin u-boot-spl-pad.bin 23552
```

**2.拼接到SPL的后面存储.**

```shell
#以ad100为例  spl固件是24K大小   那么偏移位置就是24576
cat u-boot-spl-pad.bin norinfo.bin > u-boot-spl-pad-nor-info.bin
```

**3.烧录工具定义多一个分区单独存储.**



### 5 配置驱动使能扩展nor flash探测功能

**驱动程序会默认使用烧录工具指定的nor flash信息,如果id匹配失败就会重新匹配扩展的nor flash信息.**



**uboot需要配置 CONFIG_SPL_EXTRA_NOR_INFO_ENABLE 和 CONFIG_SPL_EXTRA_NOR_INFO_OFF使能额外nor flash匹配和指定额外信息存储位置.**

以ad100为例修改  include/configs/ad100_base.h 文件添加

```c
#define CONFIG_SPL_EXTRA_NOR_INFO_ENABLE
#define CONFIG_SPL_EXTRA_NOR_INFO_OFF	23552 //根据实际偏移配置添加
```



**freertos需要配置CONFIG_EXTRA_NOR_INFO_OFF额外的nor flash 信息偏移地址.**

![3](img/3.png)



**kernel需要配置 CONFIG_INGENIC_EXTRA_NOR_INFO_ENABLE 和 CONFIG_INGENIC_EXTRA_NOR_INFO_OFF使能额外nor flash匹配和指定额外信息存储位置.**

```
Ingenic device-drivers Configurations  --->  

​	[SFC] (SPI Nand/Nor Flash) Drivers  --->

​		 Use Extra Nor Flash Info  --->

​			(0x5c00) Extra Nor Info Offset
```

