
# ------------------------------------------------------------
# 生成 awtk 需要编译的文件列表 (需要将相对路径转换为绝对路径)
# ------------------------------------------------------------
include config.mk
include .config.in

include build_awtk.mk
CLISTS := $(patsubst %, %.list,$(CSRCS))

gen_awtk_file_list: $(CLISTS)
	$(Q)echo 'CFLAGS += $(CFLAGS)' > .awtk_cflags.mk
	$(Q)echo 'CXXFLAGS += $(CXXFLAGS)' >> .awtk_cflags.mk
	$(Q)echo "  .awtk_cflags.mk .awtk_files.mk"

empty_file_list := $(shell > .awtk_files.mk)

%.list: %
	@echo "src-y += $<" >> .awtk_files.mk

.PHONY: gen_awtk_file_list
