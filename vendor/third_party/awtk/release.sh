#!/bin/bash

#example: ./release.sh ../awtk-examples/HelloWorld-Demo exename

EXE_NAME=awtk_demo_ui
APP_ROOT="./awtk"
BIN_ROOT="output"

if [ $1 ]; then
  if [ ! -d $1 ]; then
    echo "input dir : $1 is not exist!"
	  exit
  fi
  APP_ROOT=$1
fi

if [ $2 ]; then
  EXE_NAME=$2
fi

echo "EXE_NAME = ${EXE_NAME}"
echo "APP_ROOT = ${APP_ROOT}"

rm -rf release
python3 ./awtk/scripts/release.py ${EXE_NAME} ${APP_ROOT} ${BIN_ROOT}
