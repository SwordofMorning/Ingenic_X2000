#!/bin/bash

img_file=$1
block_size=$2

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

# 检查参数是否存在
check_param()
{
    local param_type=$1
    local param=$2

    if [ "$param" = "" ]; then
        echo "error: $param_type must give"
        exit 1
    fi
}

# 检查文件是否存在
check_file()
{
    local file_type=$1
    local file=$2

    if [ "$file" = "" ]; then
        echo "error: $file_type must give"
        exit 1
    fi

    if [ ! -e "$file" ]; then
        echo "error: %file not exist"
        exit 1
    fi
}

# 获得文件大小
size_file()
{
    local file=$1
    local result

    result="`ls -l -n -L $file`"
    if [ "$?" != "0" ]; then
        echo "ls failed: $file"
        exit 1
    fi

    result=`get_word "$result" 4`
    if [ "result" = "" ]; then
        echo "ls failed 2: $file"
        exit 1
    fi
    echo $result
}

# 获得文件的md5sum
md5sum_file()
{
    local file=$1
    local result

    result="`md5sum $file`"
    if [ $? != 0 ]; then
        echo "md5sum failed: $file"
        exit 1
    fi

    result=`get_word "$result" 0`
    if [ "$result" = "" ]; then
        echo "md5sum get failed: $file"
        exit 1
    fi
    echo $result
}

check_file img_file "$img_file"
check_param block_size $block_size

file_size=`size_file $img_file`
if [ "$file_size" = "0" ]; then
    echo "error: do not support empty file"
    exit 1
fi

img_name=`basename $img_file`
split --bytes=$block_size --suffix-length=4 --numeric-suffixes  $img_file ota/$img_name.

prev_file_md5=`md5sum_file $img_file`

md5_file=ota/ota_md5_$img_name.$prev_file_md5

> $md5_file

for file in `ls ota/$img_name.*`
do
    result=`md5sum_file $file`
    mv $file $file.$prev_file_md5
    prev_file_md5=$result

    echo $result >> $md5_file
done
