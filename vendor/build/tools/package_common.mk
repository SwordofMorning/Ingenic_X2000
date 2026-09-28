
# package_name-y 优先级高于 package_name
ifneq ($(package_name-y),)
package_name = $(package_name-y)
endif

package_name := $(strip $(package_name))

# 如果 package_name 不存在则调过
ifneq ($(package_name),)

# 默认 package_path 与 package_name 等同
ifeq ($(package_path),)
package_path = $(TOPDIR)/../$(package_name)
endif

# 检查 package_path 是否存在
ifeq ($(wildcard $(package_path)),)
$(error dir not exist: $(package_path))
endif

# 默认为 common_make_hook
ifeq ($(package_make_hook),)
package_make_hook = common_make_hook
endif

# 默认为 common_clean_hook
ifeq ($(package_clean_hook),)
package_clean_hook = common_clean_hook
endif

# 默认为 common_install_hook
ifeq ($(package_install_hook),)
package_install_hook = common_install_hook
endif

# 定义xxx_package_* 变量
$(package_name)_package_path := $(package_path)
$(package_name)_package_make_hook := $(package_make_hook)
$(package_name)_package_clean_hook := $(package_clean_hook)
$(package_name)_package_install_hook := $(package_install_hook)

# 加入package 列表
packages := $(packages) $(package_name)

app_$(package_name):
	$(call CMD_MAKE,$(patsubst app_%,%, $@))

clean_app_$(package_name):
	$(call CMD_CLEAN,$(patsubst clean_app_%,%, $@))

endif # ifneq ($(package_name),)

# 清空变量
package_path :=
package_make_hook :=
package_clean_hook :=
package_install_hook :=
package_name-y :=
package_name :=
