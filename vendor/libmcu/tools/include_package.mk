#
# 递归包含 package-y 中定义的 .mk 文件
#

# 保存当前的 package-y 变量
package-old := $(package-y)

# 包含单个 package
ifeq ($(no_including_info),)
$(info ..include $(package)Makefile)
endif
include $(package)Makefile

ifneq ($(filter $(package), $(packages)),)
$(error error: [$(package)] has been defined)
endif

packages += $(package)

# 继续包含 package-y 变量增加的内容
$(foreach file,$(filter-out $(package-old),$(package-y)),$(eval package:=$(file))$(eval include tools/include_package.mk))
