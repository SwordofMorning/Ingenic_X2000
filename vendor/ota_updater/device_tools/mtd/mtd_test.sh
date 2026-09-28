#!/bin/sh

mtd_name_to_num()
{
    local partition_name=$1
    local mtd_info=`cat /proc/mtd | grep -w \""$partition_name"\"`
    if [ "$mtd_info" = "" ]; then
        echo "mtd partition not find: $partition_name" 1>&2
        return 1
    fi

    local mtd_n=${mtd_info%%:*}
    if [ "$mtd_info" = "$mtd_n" ]; then
        echo "mtd_info not ok: $mtd_info" 1>&2
        return 1
    fi

    local n=${mtd_n#mtd}
    if [ "$n" = "$mtd_n" ]; then
        echo "mtd_info not ok2: $mtd_n" 1>&2
        return 1
    fi

    echo $n
}

mtd_name_to_dev()
{
    local partition_name=$1
    local num

    num=`mtd_name_to_num $partition_name`
    if [ $? != 0 ]; then
        return 1
    fi

    local dev=/dev/mtd$num
    if [ ! -e $dev ]; then
        echo "why $dev not exist" 1>&2
        return 1
    fi

    echo $dev
}

mtd_write_str()
{
    local partition_name=$1
    local str=$2
    local dev

    dev=`mtd_name_to_dev "$partition_name"`
    if [ $? != 0 ]; then
        echo "failed to get mtd dev $partition_name" 1>&2
        return 1
    fi

    flash_erase $dev 0 1
    if [ $? != 0 ]; then
        echo "failed to erase $dev" 1>&2
        return 1
    fi

    printf "%-256s" "$str" | nandwrite -s 0 -p $dev -
    if [ $? != 0 ]; then
        echo "failed to write $dev" 1>&2
        return 1
    fi

    return 0
}

mtd_read_str()
{
    local partition_name=$1
    local dev

    dev=`mtd_name_to_dev "$partition_name"`
    if [ $? != 0 ]; then
        return 1
    fi

    nanddump -s 0 -l 256 /dev/mtd5 -a
    if [ $? != 0 ]; then
        return 1
    fi

    return 0
}
