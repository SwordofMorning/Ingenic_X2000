
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/src
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/src/ext_widgets
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/src/custom_widgets
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/tools
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/agge
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/agg/include
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/include
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/3rdparty/everest/include
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/fribidi
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/libunibreak
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/gpinyin/include
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/gtest/googletest
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/gtest/googletest/include
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg/gl
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg/base
CFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/res

CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/src
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/src/ext_widgets
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/src/custom_widgets
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/tools
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/agge
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/agg/include
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/include
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/3rdparty/everest/include
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/fribidi
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/libunibreak
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/gpinyin/include
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/gtest/googletest
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/gtest/googletest/include
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg/gl
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg/base
CXXFLAGS += -I$(AWTK_DIR)/$(AWTK_DIR_NAME)/res

CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/awtk_global.c
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/tkc/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/base/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/widgets/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/xml/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/ui_loader/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/blend/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/svg/*.c)	#vgcanvas
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/ext_widgets/*.c)	#extern widget
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/ext_widgets/*/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/custom_widgets/*.c)	#custom widget
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/custom_widgets/*/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/font_loader/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/image_loader/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/layouters/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/clip_board/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/designer_support/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/widget_animators/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/window_animators/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/dialog_highlighters/*.c)
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/window_manager/window_manager_default.c

CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/platforms/pc/*.c)

ifdef GRAPHIC_BUFFER_DEFAULT
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/graphic_buffer/graphic_buffer_default.c
endif

ifdef NATIVE_WINDOW_SDL
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/native_window/native_window_sdl.c
else
	ifdef NATIVE_WINDOW_FB_GL
	CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/native_window/native_window_fb_gl.c
	else
	CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/native_window/native_window_raw.c
	endif
endif

CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/lcd/lcd_mono.c
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/lcd/lcd_mem_*.c)

CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/main_loop/main_loop_simple.c

ifdef VGCANVAS_CAIRO
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/vgcanvas/vgcanvas_cairo.c
else
	ifdef NATIVE_WINDOW_NANOVG_PLUS
	CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/vgcanvas/vgcanvas_nanovg_plus.c
	else
		ifdef NANOVG_BACKEND_BGFX
		CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/vgcanvas/vgcanvas_nanovg_bgfx.c
		else
			ifdef NANOVG_BACKEND_AGG
			CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/vgcanvas/vgcanvas_nanovg_soft.c
			else
				ifdef NANOVG_BACKEND_AGGE
				CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/vgcanvas/vgcanvas_nanovg_soft.c
				else
				CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/vgcanvas/vgcanvas_nanovg_gl.c
				endif
			endif
		endif
	endif
endif

CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_methods/input_method_creator.c
ifdef INPUT_ENGINE_NULL
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/input_engine_null.c
else
	ifdef NATIVE_WINDOW_T9EXT
	CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/ime_utils.c
	CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/input_engine_t9ext.c
	else
		ifdef NATIVE_WINDOW_T9
		CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/ime_utils.c
		CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/input_engine_t9.c
		else
			ifdef NATIVE_WINDOW_SPINYIN
			CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/ime_utils.c
			CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/input_engine_spinyin.c
			else
			CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/input_engines/input_engine_pinyin.cpp
			endif
		endif
	endif
endif

ifdef WITH_CSV
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/csv/*.c)
endif

ifdef WITH_CONF_IO
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/conf_io/*.c)
endif

ifdef WITH_HAL
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/hal/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/hal/linux/*.c)
endif

ifdef WITH_STREAMS
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/buffered/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/file/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/inet/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/mem/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/serial/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/process/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/shdlc/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/noisy/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/misc/*.c)
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/streams/statistics/*.c)
endif

ifdef WITH_COMPRESSORS
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/compressors/*.c)
endif

ifdef WITH_UBJSON
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/ubjson/*.c)
endif

ifdef WITH_DEBUGGER
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/debugger/*.c)
endif

ifdef WITH_FSCRIPT_EXT
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/fscript_ext/*.c)
endif

ifdef WITH_CHARSET
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/charset/*.c)
endif

ifdef WITH_ROMFS
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/src/romfs/*.c)
endif

ifdef WITH_3RD_AGG
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/src/agg/src/*.cpp)
endif

ifdef WITH_3RD_AGGE
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/agge/agge/*.cpp)
endif

ifdef WITH_3RD_CAIRO
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/cairo/*.c)
CFLAGS += -Wno-unused-variable -Wno-enum-conversion
endif

ifdef WITH_3RD_CJSON
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/cjson/*.cpp)
endif

ifdef WITH_3RD_FRIBIDI
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/fribidi/*.c)
endif

ifdef WITH_3RD_GLAD
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/glad/*.c)
endif

ifdef WITH_3RD_GPINYIN
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/gpinyin/src/*.cpp)
endif

ifdef WITH_3RD_LIBUNIBREAK
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/libunibreak/*.c)
endif

ifdef WITH_3RD_LZ4
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/lz4/*.c)
endif

ifdef WITH_3RD_MBEDTLS
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/library/*.c)
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/3rdparty/everest/library/everest.c
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/3rdparty/everest/library/x25519.c
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/mbedtls/3rdparty/everest/library/Hacl_Curve25519_joined.c
endif

ifdef WITH_3RD_MINIZ
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/miniz/*.c)
endif

ifdef WITH_3RD_NANOVG
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg/base/*.c)
	ifdef NANOVG_BACKEND_AGG
	CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg/agg/*.cpp)
	else
		ifdef NANOVG_BACKEND_AGGE
		CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg/agge/*.cpp)
		endif
	endif
endif

ifdef WITH_3RD_NANOG_PLUG
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg_plus/base/*.c)
endif

ifdef WITH_3RD_PIXMAN
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-access-accessors.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-access.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-bits-image.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-combine32.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-combine-float.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-conical-gradient.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-edge-accessors.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-edge.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-fast-path.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-filter.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-general.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-glyph.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-gradient-walker.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-image.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-implementation.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-linear-gradient.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-matrix.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-noop.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-radial-gradient.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-region16.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-region32.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-solid-fill.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-timer.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-trap.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-utils.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-mips.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-arm.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-x86.c,
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-ppc."
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-mips-dspr2.c
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-mips-dspr2-asm.S
CSRCS += $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/pixman/pixman/pixman-mips-memcpy-asm.S
endif

ifdef WITH_3RD_NANOG_PLUG
CSRCS += $(wildcard $(AWTK_DIR)/$(AWTK_DIR_NAME)/3rd/nanovg_plus/base/*.c)
endif

include build_awtk_ingenic.mk
