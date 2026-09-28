# file: build_bin.mk

ifeq ($(strip $(module_name)),)
$(error set "module_name" for your module name)
endif

ifdef $(module_name)_elf
$(error module name "$(module_name)" has been defined)
endif

$(module_name)_elf := $(module_name)

#
# 如果src-y为空，则不编译module
#
ifneq ($(strip $(src-y)),)

# ----------------------------------------------------
# 以下用于检查 CFLAGS CXXFLAGS LDFLAGS ASMFLAGS 是否更新
# 如果有编译选项更新,更新对应的模块临时头文件,让模块的源码重新编译

to_hide_file = $(dir $1).$(notdir $1)

create_file_if_not_exist = $(if $(wildcard $1), ,$(shell mkdir -p $(dir $1))$(shell touch $1))

_m_tmp_cflags = $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(ASMFLAGS)
_m_tmp_cflags_h = $(call to_hide_file, $(obj_dir)$(module_name)_tmp_cflags.h)
_m_tmp_cflags_mk = $(call to_hide_file, $(obj_dir)$(module_name)_tmp_cflags.mk)

make_sure_tmp_cflags_h_is_exist := $(call create_file_if_not_exist, $(_m_tmp_cflags_h))
make_sure_tmp_cflags_mk_is_exist := $(call create_file_if_not_exist, $(_m_tmp_cflags_mk))

ifneq ($(shell echo $(_m_tmp_cflags)), $(shell cat $(_m_tmp_cflags_mk)))
make_sure_tmp_cflags_mk_is_same := $(shell echo $(_m_tmp_cflags) > $(_m_tmp_cflags_mk))
make_sure_tmp_cflags_h_is_update := $(shell touch $(_m_tmp_cflags_h))
endif

# ----------------------------------------------------

ifeq ($(filter clean%, $(MAKECMDGOALS)),)
src_not_exist = $(filter-out $(wildcard $(src-y)),$(src-y))
ifneq ($(src_not_exist),)
$(error file not exist: $(src_not_exist))
endif
endif

ifneq ($(strip $(V)),1)
Q=@
else
Q=
endif

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
CXX = $(CROSS_COMPILE)g++

MSG_link  := "  Linking  "
MSG_cc    := "  CC       "
MSG_cxx   := "  CXX      "
MSG_asm   := "  ASM      "
MSG_clean := "  CLEAN    "


obj-y = $(filter-out %.c %.d %.cc %.cpp %.s %.S, $(src-y))

$(module_name)_c_src := $(filter %.c,$(src-y))
$(module_name)_cxx_src := $(filter %.cpp %.cc,$(src-y))
$(module_name)_s_src := $(filter %.s %.S,$(src-y))

# objs
$(module_name)_src_o := $(patsubst %,$(obj_dir)%.o,$(src-y))
$(module_name)_c_src_o := $(patsubst %,$(obj_dir)%.o,$($(module_name)_c_src))
$(module_name)_cxx_src_o := $(patsubst %,$(obj_dir)%.o,$($(module_name)_cxx_src))
$(module_name)_s_src_o := $(patsubst %,$(obj_dir)%.o,$($(module_name)_s_src))

# deps
$(module_name)_src_d := $(patsubst %,%.d,$(src-y))
$(module_name)_c_src_d := $(patsubst %,%.d,$($(module_name)_c_src))
$(module_name)_cxx_src_d := $(patsubst %,%.d,$($(module_name)_cxx_src))
$(module_name)_s_src_d := $(patsubst %,%.d,$($(module_name)_s_src))

ifeq ($(module_build_cmd),)

module_build_cmd = $(Q)$(CC) $$^ $(obj-y) $(LDFLAGS) $(LDFLAGS-y) -o $$@

ifneq ($(filter %.so,$(module_name)),)
module_build_cmd = $(Q)$(CC) $$^ $(obj-y) $(LDFLAGS) $(LDFLAGS-y) -shared -o $$@
endif

ifneq ($(filter %.a,$(module_name)),)
module_build_cmd = $(Q)$(AR) -cr $$@ $$^ $(obj-y)
endif

endif

ifeq ($(filter clean%, $(MAKECMDGOALS)),)
sinclude $($(module_name)_src_d)
endif

# 定义默认的目标
$(module_name)_ALL:  $(module_name)

define build_elf_rules

# 所有deps文件的生成规则
# $($(module_name)_c_src_d): %.d:%
# 	$(Q)$(CC) $(CFLAGS) $(CFLAGS-y) -MM $$< -MT $(obj_dir)$$<.o -MF $$@

# $($(module_name)_cxx_src_d): %.d:%
# 	$(Q)$(CXX) $(CXXFLAGS) $(CXXFLAGS-y) -MM $$< -MT $(obj_dir)$$<.o -MF $$@

# $($(module_name)_s_src_d): %.d:%
# 	$(Q)$(CC) $(ASMFLAGS) $(ASMFLAGS-y) -MM $$< -MT $(obj_dir)$$<.o -MF $$@

# 所有objs文件的生成规则
$($(module_name)_c_src_o): $(obj_dir)%.o:%
	@echo $(MSG_cc) $$<
	$(Q)mkdir -p $$(dir $$@)
	$(Q)$(CC) $(CFLAGS) $(CFLAGS-y) -c $$< -o $$@

$($(module_name)_cxx_src_o): $(obj_dir)%.o:%
	@echo $(MSG_cxx) $$<
	$(Q)mkdir -p $$(dir $$@)
	$(Q)$(CXX) $(CXXFLAGS) $(CXXFLAGS-y) -c $$< -o $$@

$($(module_name)_s_src_o): $(obj_dir)%.o:%
	@echo $(MSG_asm) $$<
	$(Q)mkdir -p $$(dir $$@)
	$(Q)$(CC) $(ASMFLAGS) $(ASMFLAGS-y) -c $$< -o $$@

# bin 文件的生成规则
$(module_name):$($(module_name)_src_o) $(module_depends) $(module_depends-y)
	@echo $(MSG_link) $$@
	$(Q)mkdir -p $$(dir $$@)
	$(module_build_cmd)

# # 删除所有依赖文件
# $(module_name)_CLEAN_DEPS:
# 	$(Q)rm -f $($(module_name)_src_d)

endef

.PHONY: $(module_name)_ALL
# .PHONY: $(module_name)_CLEAN_DEPS

$(module_name):$($(module_name)_src_o)

$(eval $(build_elf_rules))
# $(info $(build_bin_rules))

# 添加到所有的目标
module_targets := $(module_targets) $(module_name)_ALL
all_modules := $(all_modules) $(module_name)

# 添加到所有需要删除的文件
module_clean_files := $(module_clean_files)\
$(module_name) $($(module_name)_src_d) $($(module_name)_src_o)

endif # src-y

# 清空变量
src-y :=
CFLAGS-y :=
CXXFLAGS-y :=
ASMFLAGS-y :=
LDFLAGS-y :=

module_name :=
module_build_cmd :=
module_depends-y :=
