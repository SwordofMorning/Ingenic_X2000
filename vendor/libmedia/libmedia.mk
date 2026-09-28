sinclude .config.in

# 此文件中处理需要依赖的库,添加到 LDFLAGS
sinclude libmedia_LDFLAGS.mk

obj_dir = .objs/

#
# 定义默认目标
#
all: all_targets


# ---------------------------
# libmedia sources
# ---------------------------

src-y += core/audio_frame.c
src-y += core/video_frame.c
src-y += core/video_frame_copy.c
src-y += core/media_alloter.c
src-y += core/media_packet.c
src-y += core/audio_reader.c
src-y += core/audio_player.c
src-y += core/video_reader.c
src-y += core/video_player.c
src-y += core/video_rotater.c
src-y += core/audio_resampler.c
src-y += core/audio_encoder.c
src-y += core/video_encoder.c
src-y += core/media_muxing.c
src-y += core/audio_decoder.c
src-y += core/video_decoder.c
src-y += core/video_scaler.c
src-y += core/video_overlayer.c
src-y += core/media_demuxing.c
src-y += core/media_previewer.c
src-y += core/media_recorder.c
src-y += core/media_player.c
src-y += core/font.c

src-y += core/blend/video_frame_blend.c
src-y += core/blend/blend_yuv.c
src-y += core/blend/blend_rgb.c

src-y += core/convert/rgb_to_yuv.c
src-y += core/convert/yuv_to_rgb.c

src-y += core/video_display_mode.c

src-y += core/video_frame_rotate.c
src-y += core/rotate/rotate_16.c
src-y += core/rotate/rotate_32.c
src-y += core/rotate/rotate_8.c
src-y += core/rotate/rotate.c

src-y += lib/file_utils.c
src-y += lib/fifo_utils.c
src-y += lib/thread_utils.c
src-y += lib/pkt_parse.c
ifdef APP_libmedia_signal_handler
src-y += lib/signal_handler.c
endif
src-y += lib/unicode_utils.c

src-y += devices/async/async_audio_player.c
src-y += devices/async/async_video_player.c

ifdef APP_libmedia_alsa
src-y += devices/alsa/alsa_audio_player.c
src-y += devices/alsa/alsa_audio_reader.c
ifdef APP_libmedia_speex_echo
src-y += devices/alsa/alsa_speex_aec_card.c
endif
ifdef APP_libmedia_speex_echo_cc
src-y += devices/alsa/alsa_speex_aec_cc_card.c
endif
endif

ifdef APP_libmedia_hw_rotater
src-y += devices/hw_rotater/hw_video_rotater.c
endif

ifdef APP_libmedia_2d_rotator
src-y += devices/hw_2d_rotater/hw_video_2d_rotater.c
endif

ifdef APP_libmedia_hw_encoder
src-y += devices/hw_encoder/hw_h264_video_encoder.c
endif

ifdef APP_libmedia_hw_decoder
src-y += devices/hw_decoder/hw_h264_decoder.c
endif

ifdef APP_libmedia_x2000_hw_jpeg_decoder
src-y += devices/hw_decoder/hw_jpeg_decoder.c
endif

ifdef APP_libmedia_x2600_hw_jpeg_decoder
src-y += devices/hw_decoder/hw_jpegd_decoder.c
endif

ifdef APP_libmedia_fb
src-y += devices/fb/fb_video_player.c
endif

ifdef APP_libmedia_simple_fb
src-y += devices/fb/fb_simple_video_player.c
endif

ifdef APP_libmedia_uvc
src-y += devices/uvc/uvc_video_reader.c
src-y += devices/uvc/uvc_demuxing.c
endif

ifdef APP_libmedia_libisp
src-y += devices/isp/isp_video_reader.c
endif

ifdef APP_libmedia_vic
src-y += devices/vic/vic_video_reader.c
endif

ifdef APP_libmedia_stb
src-y += devices/stb/stb_font.c
endif

ifdef APP_libmedia_turbo_jpeg
src-y += devices/turbo_jpeg/turbo_jpeg_decoder.c
endif

ifdef APP_libmedia_static
CFLAGS += -static -ffunction-sections -fdata-sections
module_name = output/libmedia.a
else
CFLAGS += -fPIC
module_name = output/libmedia.so
endif

ifdef APP_libmedia_msa_enable
CFLAGS += -mmsa
endif

CFLAGS += -Wall
CFLAGS += -Iinclude/
CFLAGS += -O2
CFLAGS += -include config.h
CFLAGS-$(APP_libmedia_2d_rotator) += -I../lib2d/include

include tools/build_elf.mk


# ---------------------------
# libmedia_ffmpeg
# ---------------------------

ifdef APP_libmedia_ffmpeg
src-y += devices/ffmpeg/ffmpeg_audio_resampler.c
src-y += devices/ffmpeg/ffmpeg_audio_encoder.c
src-y += devices/ffmpeg/ffmpeg_audio_decoder.c
src-y += devices/ffmpeg/ffmpeg_video_encoder.c
src-y += devices/ffmpeg/ffmpeg_video_decoder.c
src-y += devices/ffmpeg/ffmpeg_video_scaler.c
src-y += devices/ffmpeg/ffmpeg_video_overlayer.c
src-y += devices/ffmpeg/ffmpeg_demuxing.c
src-y += devices/ffmpeg/ffmpeg_muxing.c
src-y += devices/ffmpeg/ffmpeg_utils.c

LDFLAGS-y += -lavformat
LDFLAGS-y += -lavcodec
LDFLAGS-y += -lavutil
LDFLAGS-y += -lswresample
LDFLAGS-y += -lavfilter
endif

ifdef APP_libmedia_static
module_name = output/libmedia_ffmpeg.a
else
module_name = output/libmedia_ffmpeg.so
endif

include tools/build_elf.mk

# ---------------------------
# the end
# ---------------------------
all_targets: $(module_targets)
	@echo "  compiled $(all_modules)"

liba = $(filter %.a,$(all_modules))
libso = $(filter %.so,$(all_modules))

ifneq ($(libso),)
define install_libso
	$(Q)cp -f $(libso) $(FS_TARGET_DIR)/usr/lib/
	$(Q)cp -f $(libso) $(FS_STAGING_DIR)/usr/lib/
endef
define clean_libso
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(libso)))
	$(Q)rm -f $(addprefix $(FS_STAGING_DIR)/usr/lib/, $(notdir $(libso)))
endef
endif

ifneq ($(liba),)
define install_liba
	$(Q)cp -f $(liba) $(FS_STAGING_DIR)/usr/lib/
endef
define clean_libso
	$(Q)rm -f $(addprefix $(FS_TARGET_DIR)/usr/lib/, $(notdir $(liba)))
endef
endif

install:
	$(install_libso)
	$(install_liba)
	$(Q)cp -rf include/libmedia $(FS_STAGING_DIR)/usr/include/
	@echo "  installed $(liba) $(libso) include/libmedia/"

clean_install:
	$(clean_libso)
	$(clean_liba)
	$(Q)rm -rf $(FS_STAGING_DIR)/usr/include/libmedia
	@echo "  removed $(liba) $(libso) include/libmedia/"

clean:
	$(Q)rm -rf $(module_clean_files) $(all_modules)

.PHONY: all clean all_targets clean_install install