#!/bin/sh

CC=${CC:=cc}
CCOPT="-Wall -W -O2 -lm"

$CC -I../../include/hping3/ -I../include/ ../src/gentables.c -o gentables $CCOPT
./gentables > ../src/tables.c
./gentables h > ../include/tables.h
echo Tables generated
