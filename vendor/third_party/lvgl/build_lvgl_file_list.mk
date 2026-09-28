
# ------------------------------------------------------------
# 生成 lvgl 需要编译的文件列表 (需要将相对路径转换为绝对路径)
# ------------------------------------------------------------

include $(LVGL_DIR)/$(LVGL_DIR_NAME)/lvgl.mk
CLISTS := $(patsubst %, %.list,$(CSRCS))

gen_lvgl_file_list: $(CLISTS)
	$(Q)echo 'CFLAGS += $(CFLAGS)' > .lvgl_cflags.mk
	$(Q)echo "  .lvgl_cflags.mk .lvgl_files.mk"

empty_file_list := $(shell > .lvgl_files.mk)

%.list: %
	@echo "src-y += $<" >> .lvgl_files.mk

.PHONY: gen_lvgl_file_list
