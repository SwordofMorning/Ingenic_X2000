#!/bin/bash

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

# 获取关键字的值
get_key_word()
{
    local str=$1
    local key=$2
    local word=
    local tmp
    local len

    #　情况1　"key = words"
    tmp=`get_word "$str" 0`
    if [ "$tmp" = "$key" ]; then
        tmp=`get_word "$str" 1`
        if [ "$tmp" != "=" ]; then
            return 1
        fi

        echo ${str##*=}
        return 0
    fi

    #　情况2　"key=words"
    len=${#key}
    let len=len+1
    tmp=${str:0:$len}
    if [ "$tmp" = "$key=" ]; then
        echo ${str:$len}
        return 0
    fi

    return 1;
}

# 获得文件大小
size_file()
{
    local file=$1
    local result

    result="`ls -l -L $file`"
    if [ "$?" != "0" ]; then
        echo "ls failed: $file" 1>&2
        exit 1
    fi

    result=`get_word "$result" 4`
    if [ "result" = "" ]; then
        echo "ls failed 2: $file" 1>&2
        exit 1
    fi
    echo $result
}

# 检查文件是否存在，以及是否为空
check_file()
{
    local file=$1
    local size

    if [ ! -e "$file" ]; then
        echo "error: $file not exist" 1>&2
        exit 1
    fi

    size=`size_file $file`
    if [ $size = 0 ]; then
        echo "error: do not support empty file" 1>&2
        exit 1
    fi
}

# 获得文件的md5sum
md5sum_file()
{
    local file=$1
    local result

    result="`md5sum $file`"
    if [ $? != 0 ]; then
        echo "md5sum failed: $file" 1>&2
        exit 1
    fi

    result=`get_word "$result" 0`
    if [ "$result" = "" ]; then
        echo "md5sum failed: $file" 1>&2
        exit 1
    fi
    echo $result
}

set_update()
{
    echo $1 >> ota/ota_update.in
}

# 创建ota img
create_ota_img()
{
    local img_type=$1
    local img_file=$2
    local block_size=$3

    set_update "img_type=$img_type"
    set_update "img_name=`basename $img_file`"
    set_update "img_size=`size_file $img_file`"
    set_update "img_md5=`md5sum_file $img_file`"
    set_update

    ./host_tools/mk_ota_img.sh $img_file $block_size
    if [ "$?" != "0" ]; then
        echo "failed to create $img_type: $img_file ota img" 1>&2
        exit 1
    fi
}

ota_version=
block_size=
kernel_img=
rootfs_img=
rtos_img=

tmp=
for str in $@
do
    tmp=`get_key_word $str ota_version`
    if [ $? = 0 ]; then
        ota_version=$tmp
        continue
    fi

    tmp=`get_key_word $str block_size`
    if [ $? = 0 ]; then
        block_size=$tmp
        continue
    fi

    tmp=`get_key_word $str kernel_img`
    if [ $? = 0 ]; then
        kernel_img=$tmp
        continue
    fi

    tmp=`get_key_word $str rootfs_img`
    if [ $? = 0 ]; then
        rootfs_img=$tmp
        continue
    fi

    tmp=`get_key_word $str rtos_img`
    if [ $? = 0 ]; then
        rtos_img=$tmp
        continue
    fi

    echo "can't support this option: $str" 1>&2
    exit 1
done

if [ "$ota_version" = "" ]; then
    echo "ota_version not set" 1>&2
    exit 1
fi

if [ "$block_size" = "" ]; then
    echo "block_size not set" 1>&2
    exit 1
fi

if [ "$kernel_img" = "" ]; then
    echo "kernel_img not set" 1>&2
fi

if [ "$rootfs_img" = "" ]; then
    echo "rootfs_img not set" 1>&2
fi

if [ "$rtos_img" = "" ]; then
    echo "rtos_img not set" 1>&2
fi

if [ "$kernel_img" != "" ]; then
    check_file $kernel_img
fi

if [ "$rootfs_img" != "" ]; then
    check_file $rootfs_img
fi

if [ "$rtos_img" != "" ]; then
    check_file $rtos_img
fi

mkdir -p ota/
rm ota/* -rf

set_update "ota_version=$ota_version"
set_update

if [ "$kernel_img" != "" ]; then
    create_ota_img kernel "$kernel_img" $block_size
fi

if [ "$rootfs_img" != "" ]; then
    create_ota_img rootfs "$rootfs_img" $block_size
fi

if [ "$rtos_img" != "" ]; then
    create_ota_img rtos "$rtos_img" $block_size
fi