APPLICATION = pepper_simple

BOARD ?= dwm1001

DEVELHELP ?= 1

# All uwb-core applications need to enable `-fms-extensions`
CFLAGS += -fms-extensions
ifneq (,$(filter llvm,$(TOOLCHAIN)))
  CFLAGS += -Wno-microsoft-anon-tag
endif

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT
# External modules serach path
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/uwb/

# PEPPER related modules
USEMODULE += pepper
USEMODULE += pepper_controller
USEMODULE += pepper_current_time
USEMODULE += pepper_shell
USEMODULE += pepper_util
USEMODULE += pepper_status_led
## Include pepper server
USEMODULE += pepper_srv
USEMODULE += pepper_srv_storage
USEMODULE += ed_uwb
USEMODULE += ed_uwb_stats
# USEMODULE += ed_uwb_los
USEMODULE += ed_ble
USEMODULE += ed_leds
FEATURES_REQUIED += periph_gpio_irq

# Include shell and, process information fonctionality
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Change default timings for faster demo behaviout
ADV_PER_SLICE ?= 10           # 10 advertisements per slice
CFLAGS += -DCONFIG_ADV_PER_SLICE=$(ADV_PER_SLICE)
EPOCH_DURATION_SEC ?= 300     # 5min
CFLAGS += -DCONFIG_EPOCH_DURATION_SEC=$(EPOCH_DURATION_SEC)
MIN_EXPOSURE_TIME_S ?= 100
CFLAGS+=-DMIN_EXPOSURE_TIME_S=$(MIN_EXPOSURE_TIME_S)
MAX_DISTANCE_CM ?= 1000       # 10m so everything is logged
CFLAGS += -DMAX_DISTANCE_CM=$(MAX_DISTANCE_CM)
TWR_LISTEN_WINDOW_US ?= 2000  # 2ms listen windows
CFLAGS += -DCONFIG_TWR_LISTEN_WINDOW_US=$(TWR_LISTEN_WINDOW_US)
ADV_ITVL_MS ?= 1000           # 1 adv per second
CFLAGS += -DCONFIG_BLE_ADV_ITVL_MS=$(ADV_ITVL_MS)

# The event thread will handle:
# * advertisement
# * twr
# * start/end of epoch
# * other app event
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_LARGE
CFLAGS += -DEVENT_THREAD_HIGHEST_STACKSIZE=THREAD_STACKSIZE_LARGE

include $(RIOTBASE)/Makefile.include
