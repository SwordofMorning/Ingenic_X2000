#
# 定义默认目标
#
all: all_targets

#
# 编译选项
#
CC= $(CROSS_COMPILE)gcc
AR= $(CROSS_COMPILE)ar
RANLIB= $(CROSS_COMPILE)ranlib

CFLAGS = -O2
CFLAGS += -Iinclude/hping3/
CFLAGS += -Iinit/include/
CFLAGS += -include config.h
CFLAGS += -Wall
CFLAGS += -g
CFLAGS += -DUSE_TCL

obj_dir = .objs/

# ----------------------
# 编译 hping3
# ----------------------
LDFLAGS = $(CFLAGS) -lpcap -ltcl8.6 -lm -lpthread
ARSOBJ = ars.o apd.o split.o rapd.o
OBJ=	main.o getifname.o getlhs.o \
	parseoptions.o datafiller.o \
	datahandler.o gethostname.o \
	binding.o getusec.o opensockraw.o \
	logicmp.o waitpacket.o resolve.o \
	sendip.o sendicmp.o sendudp.o \
	sendtcp.o cksum.o statistics.o \
	usage.o version.o antigetopt.o \
	sockopt.o listen.o \
	sendhcmp.o memstr.o rtt.o \
	relid.o sendip_handler.o \
	libpcap_stuff.o memlockall.o memunlockall.o \
	memlock.o memunlock.o ip_opt_build.o \
	display_ipopt.o sendrawip.o signal.o send.o \
	strlcpy.o arsglue.o random.o scan.o \
	hstring.o script.o interface.o \
	adbuf.o hex.o apdutils.o sbignum.o \
	sbignum-tables.o $(ARSOBJ)
module_name = output/hping3
SRC_DIR = ./src
src-y += $(wildcard $(SRC_DIR)/*.c)
# src-y += init/src/tables.c
include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"
	@echo "$(FS_TARGET_DIR)/usr/sbin/"

libs = $(all_modules)

ifneq ($(libs),)
define install_libs
	$(Q)cp $(all_modules) $(FS_TARGET_DIR)/usr/sbin/
	$(Q)ln -s ./hping3 $(FS_TARGET_DIR)/usr/sbin/hping
	$(Q)ln -s ./hping3 $(FS_TARGET_DIR)/usr/sbin/hping2
	@echo "  installed $(all_modules)"
endef
endif

install:
	$(install_libs)

clean_install:
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(all_modules)))

clean:
	$(Q)rm -rf output/
	$(Q)rm -rf $(module_clean_files)

.PHONY: all clean all_targets clean_install install
	
