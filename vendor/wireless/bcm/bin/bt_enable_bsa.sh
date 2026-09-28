#!/bin/sh
tty_dev=BT_UART_DEV_PATH
firmware_path=BT_FIRMWARE_PATH

killall bsa_server

rfkill block bluetooth
rfkill unblock bluetooth

sleep 1
mkdir -p /run/blue_bsa
bsa_server -all=0 -d $tty_dev -p $firmware_path -u /run/blue_bsa/ -k /run/blue_bsa/ble_local_keys -b /tmp/hci_snoop.log > /tmp/bsa_server.log &

