ifeq ($(APP_libmcu_gcc_optimization_O0), y)
root.COMMON_FLAGS += -O0
else
root.COMMON_FLAGS += $(APP_libmcu_gcc_optimization)
endif

root.COMMON_FLAGS-$(APP_libmcu_gcc_function_sections) += -ffunction-sections
root.COMMON_FLAGS-$(APP_libmcu_gcc_data_sections) += -fdata-sections

root.TARGET_LDFLAGS-$(APP_libmcu_gcc_gc_sections) += --gc-sections

touch_always_compile := $(shell touch include/always_compile.h)
