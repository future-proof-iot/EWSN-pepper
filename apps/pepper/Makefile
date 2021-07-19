APPLICATION = desire

BOARD ?= dwm1001

# All uwb-core applications need to enable `-fms-extensions`
CFLAGS += -fms-extensions
ifneq (,$(filter llvm,$(TOOLCHAIN)))
  CFLAGS += -Wno-microsoft-anon-tag
endif

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += desire_advertiser
USEMODULE += desire_scanner
USEMODULE += desire_scanner_time_update
USEMODULE += uwb_epoch
USEMODULE += twr
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/uwb/

USEMODULE += event_thread_highest
USEMODULE += event_thread_medium
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_epoch

# Uncomment to modify default rotation values
CFLAGS+=-DCONFIG_SLICE_ROTATION_T_S=1
CFLAGS+=-DCONFIG_EBID_ROTATION_T_S=30
CFLAGS+=-DMIN_EXPOSURE_TIME_S=10

CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_MEDIUM
CFLAGS += -DEVENT_THREAD_HIGHEST_STACKSIZE=THREAD_STACKSIZE_LARGE

CFLAGS += -DMYNEWT_VAL_UWB_PKG_INIT_LOG=0

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
# Include all ranging algorithms
USEMODULE += uwb-core_twr_ss
USEMODULE += uwb-core_twr_ss_ack
USEMODULE += uwb-core_twr_ss_ext
USEMODULE += uwb-core_twr_ds
USEMODULE += uwb-core_twr_ds_ext

FEATURES_OPTIONAL += periph_gpio_irq

include $(RIOTBASE)/Makefile.include
