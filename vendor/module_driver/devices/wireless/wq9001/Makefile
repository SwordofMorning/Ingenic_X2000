WLAN_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(DRIVERS_DIR)/tools/common_module.mk

WLAN_CFLAGS += -I$(DRIVERS_DIR) -I$(DRIVERS_DIR)/include/
WLAN_CFLAGS += -I$(SOC_DIR)/include/
WLAN_CFLAGS += -include $(DRIVERS_DIR)/config.h

WLAN_CFLAGS += -I$(WLAN_DIR)
WLAN_CFLAGS += -I$(WLAN_DIR)/wdrv
WLAN_CFLAGS += -I$(WLAN_DIR)/wdrv/mwl
WLAN_CFLAGS += -I$(WLAN_DIR)/wproto
WLAN_CFLAGS += -I$(WLAN_DIR)/wproto/net80211
WLAN_CFLAGS += -I$(WLAN_DIR)/oss

EXTRA_CFLAGS += -I$(src)/include/ -I$(src)/

wq9001-y = wifi_drv.o
wq9001-y += oss/osal.o
wq9001-y += oss/mmal.o
wq9001-y += oss/taskq.o
wq9001-y += oss/mbuf.o
wq9001-y += oss/w_debug.o
wq9001-y += oss/net_if.o
wq9001-y += oss/oss.o
wq9001-y += oss/usb.o

#wproto
wq9001-y += wproto/net80211/ieee80211.o
wq9001-y += wproto/net80211/ieee80211_action.o
wq9001-y += wproto/net80211/ieee80211_ageq.o
wq9001-y += wproto/net80211/ieee80211_crypto.o
wq9001-y += wproto/net80211/ieee80211_crypto_ccmp.o
wq9001-y += wproto/net80211/ieee80211_crypto_none.o
wq9001-y += wproto/net80211/ieee80211_crypto_tkip.o
wq9001-y += wproto/net80211/ieee80211_crypto_wep.o
wq9001-y += wproto/net80211/ieee80211_hostap.o
wq9001-y += wproto/net80211/ieee80211_ht.o
wq9001-y += wproto/net80211/ieee80211_input.o
wq9001-y += wproto/net80211/ieee80211_ioctl.o
wq9001-y += wproto/net80211/ieee80211_monitor.o
wq9001-y += wproto/net80211/ieee80211_node.o
wq9001-y += wproto/net80211/ieee80211_output.o
wq9001-y += wproto/net80211/ieee80211_phy.o
wq9001-y += wproto/net80211/ieee80211_power.o
wq9001-y += wproto/net80211/ieee80211_proto.o
wq9001-y += wproto/net80211/ieee80211_regdomain.o
wq9001-y += wproto/net80211/ieee80211_scan.o
wq9001-y += wproto/net80211/ieee80211_scan_sta.o
wq9001-y += wproto/net80211/ieee80211_scan_sw.o
wq9001-y += wproto/net80211/ieee80211_sim.o
wq9001-y += wproto/net80211/ieee80211_sta.o
wq9001-y += wproto/net80211/ieee80211_xauth.o
wq9001-y += wproto/net80211/ieee80211_extap.o

#wdrv
wq9001-y += wdrv/driver_main.o
wq9001-y += wdrv/mwl/if_hawk.o
wq9001-y += wdrv/mwl/if_hawk_port.o
wq9001-y += wdrv/mwl/hawkhal.o
wq9001-y += wdrv/mwl/wq_hal_rx.o
wq9001-y += wdrv/mwl/wq_ipc.o
wq9001-y += wdrv/mwl/wq_htc_desc.o

WLAN_CFLAGS += -DWQ_DEBUG
#WLAN_CFLAGS += -DWQ_SIM_HOST_ONLY
#WLAN_CFLAGS += -DWQ_BUS_TEST
WLAN_CFLAGS += -DWQ_FW_DL
#WLAN_CFLAGS += -DWQ_ADD_AUTH_ASOC_IE
#WLAN_CFLAGS += -DWQ_LOAD_CALI_FROM_FILE


WLAN_CFLAGS += -DSTA_VIF_NAME='"wlan0"'
WLAN_CFLAGS += -DSOFTAP_VIF_NAME='"wlan1"'
WLAN_CFLAGS += -DMON_VIF_NAME='"mon.wlan1"'

#log: 0(none), 1(oops), 2(err), 3(wrn), 4(inf)
WLAN_CFLAGS += -DWQ_LOG_USB_SEV=1
WLAN_CFLAGS += -DWQ_LOG_IPC_SEV=1

#EP_TYPE: 2(BULK), 3(INT)
CONFIG_USB_MSG_TP = 2
WLAN_CFLAGS += -DWQ_MSG_IN_EP_TYPE=$(CONFIG_USB_MSG_TP)
WLAN_CFLAGS += -DWQ_MSG_OUT_EP_TYPE=$(CONFIG_USB_MSG_TP)

#USB_FIFO_NUM
CONFIG_USB_FIFO_NUM ?= 8
WLAN_CFLAGS += -DWQ_USB_FIFO_NUM=$(CONFIG_USB_FIFO_NUM)

#USB_RX_TYPE: 1(DMA), 2(CPU)
CONFIG_USB_RX_TYPE ?= 1
WLAN_CFLAGS += -DWQ_USB_RX_TYPE=$(CONFIG_USB_RX_TYPE)

#fw source: 0(.h), 1(file)
WLAN_CFLAGS += -DWQ_FW_SRC=0

WLAN_CFLAGS += -fmax-errors=5
WLAN_CFLAGS += -Werror
#workaround TODO
WLAN_CFLAGS += -Wno-strict-prototypes
#WLAN_CFLAGS += -Wno-int-to-pointer-cast

#WLAN_CFLAGS += -O0

#branch version: 0(master), 1(TP)
CONFIG_BRANCH_VERSION ?= 0
WLAN_CFLAGS += -DBRANCH_VERSION=$(CONFIG_BRANCH_VERSION)

#build version
CONFIG_BUILD_VERSION ?= 643
WLAN_CFLAGS += -DBUILD_VERSION=$(CONFIG_BUILD_VERSION)

#mac addr: 0(random addr from driver), 1(from efuse)
CONFIG_MAC_FROM_EFUSE ?= 0
WLAN_CFLAGS += -DWQ_MAC_FROM_EFUSE=$(CONFIG_MAC_FROM_EFUSE)

EXTRA_CFLAGS += $(WLAN_CFLAGS)

obj-m = wq9001.o