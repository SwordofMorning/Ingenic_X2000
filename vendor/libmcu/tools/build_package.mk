
ifeq ($(strip $(package_name)),)
$(error "must define package_name")
endif

ifeq ($(strip $(D)),)
$(error "must define D")
endif

# 去除重复字符串
# $(call uniq, your words words)
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# If the list of objects to link is empty, just create an empty built-in.o
cmd_link_o_target = $(if $(strip $1),\
		      $(Q)$(LD) $(LDFLAGS) -r -o $@ $(strip $1),\
		      $(Q)rm -f $@; $(AR) rcs $@ )

include .config.in

include tools/msg.mk

include $(D)Makefile

CFLAGS += $(CFLAGS-y)
CXXFLAGS += $(CXXFLAGS-y)
ASMFLAGS += $(ASMFLAGS-y)
LDFLAGS += $(LDFLAGS-y)

# 文件去重
# 文件加上包所在的路径
src-y := $(call uniq, $(src-y))
src-y := $(patsubst %,$(D)%,$(src-y))

obj-y := $(filter-out %.c %.d %.cc %.cpp %.s %.S, $(src-y))
src-y := $(filter-out $(obj-y), $(src-y))

src-not-exist := $(filter-out $(wildcard $(src-y)), $(src-y))
ifneq ($(strip $(src-not-exist)),)
$(error error: file "$(src-not-exist)" not exist!)
endif

c_src := $(filter %.c,$(src-y))
cxx_src := $(filter %.cpp %.cc,$(src-y))
s_src := $(filter %.s %.S,$(src-y))

# objs
src_o := $(patsubst %,$(obj_dir)%.o,$(src-y))
c_src_o := $(patsubst %,$(obj_dir)%.o,$(c_src))
cxx_src_o := $(patsubst %,$(obj_dir)%.o,$(cxx_src))
s_src_o := $(patsubst %,$(obj_dir)%.o,$(s_src))

# deps
src_d := $(patsubst %,$(obj_dir)%.d,$(src-y))
c_src_d := $(patsubst %,$(obj_dir)%.d,$(c_src))
cxx_src_d := $(patsubst %,$(obj_dir)%.d,$(cxx_src))
s_src_d := $(patsubst %,$(obj_dir)%.d,$(s_src))

ifeq ($(package_build_cmd),)

package_build_cmd = $(Q)$(CC) $(src_o) $(obj-y) $(LDFLAGS) $(LDFLAGS-y) -o $@

ifneq ($(filter %.so,$(package_name)),)
package_build_cmd = $(Q)$(CC) $(src_o) $(obj-y) $(LDFLAGS) $(LDFLAGS-y) -shared -o $@
endif

ifneq ($(filter %.a,$(package_name)),)
package_build_cmd = $(Q)$(AR) -cr $@ $(src_o) $(obj-y)
endif

ifneq ($(filter %.o,$(package_name)),)
package_build_cmd = $(call cmd_link_o_target, $(src_o) $(obj-y))
endif

endif

ifeq ($(filter clean%, $(MAKECMDGOALS)),)
sinclude $(src_d)
endif

# 定义默认的目标
ALL: $(package_name)

clean_ALL:
	@echo $(MSG_clean) $(package_name)
	$(Q)rm -f $(src_o) $(src_d) $(package_name)

clean_DEP:
	$(Q)rm -f $(src_d)

# 所有deps文件的生成规则
$(c_src_d): $(obj_dir)%.d:%
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -MM $< -MT $(obj_dir)$<.o -MF $@

$(cxx_src_d): $(obj_dir)%.d:%
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CXXFLAGS) -MM $< -MT $(obj_dir)$<.o -MF $@

$(s_src_d): $(obj_dir)%.d:%
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(ASMFLAGS) -MM $< -MT $(obj_dir)$<.o -MF $@

# 所有objs文件的生成规则
$(c_src_o): $(obj_dir)%.o:%
	@echo $(MSG_cc) $<
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(cxx_src_o): $(obj_dir)%.o:%
	@echo $(MSG_cxx) $<
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CXXFLAGS) -c $< -o $@

$(s_src_o): $(obj_dir)%.o:%
	@echo $(MSG_asm) $<
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(ASMFLAGS) -c $< -o $@

# bin 文件的生成规则
$(package_name):$(src_o) $(obj-y) $(package_targets)
	@echo $(MSG_link) $@
	$(Q)mkdir -p $(dir $@)
	$(package_build_cmd)

.PHONY: ALL clean_ALL $(package_name) clean_DEP
