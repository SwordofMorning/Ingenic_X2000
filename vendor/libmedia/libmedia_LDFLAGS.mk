
ifdef APP_libmedia_alsa
use_libhardware2 = y
use_libasound = y
endif

ifdef APP_libmedia_hw_rotater
use_libhardware2 = y
endif

ifdef APP_libmedia_2d_rotator
use_2d = y
endif

ifdef APP_libmedia_hw_encoder
use_libhardware2 = y
endif

ifdef APP_libmedia_fb
use_libhardware2 = y
endif

ifdef APP_libmedia_libisp
use_libisp = y
endif

ifdef APP_libmedia_vic
use_libhardware2 = y
endif

ifdef APP_libmedia_turbo_jpeg
use_turbo_jpeg = y
endif

ifdef APP_libmedia_speex_echo
use_speex = y
endif

ifdef APP_libmedia_speex_echo_cc
use_speex = y
endif

DEP_LIBS-y += -lpthread -lm
DEP_LIBS-y += -lutils2

DEP_LIBS-$(use_libisp) += -lisp
DEP_LIBS-$(use_libhardware2) += -lhardware2
DEP_LIBS-$(use_2d) += -l2d
# DEP_LIBS-$(use_libasound) += -lasound
DEP_LIBS-$(use_turbo_jpeg) += -lturbojpeg
DEP_LIBS-$(use_speex) += -lspeexdsp

LDFLAGS += $(DEP_LIBS-y)
