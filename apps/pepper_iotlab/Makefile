APPLICATION = pepper_iotlab

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
USEMODULE += ed_uwb
## Include all debug data
USEMODULE += ed_uwb_stats
USEMODULE += ed_uwb_los
USEMODULE += ed_uwb_rssi
USEMODULE += ed_ble
## Add leds
USEMODULE += ed_leds
## Include current_time_shell
USEMODULE += current_time_shell

# Include shell and, process information fonctionality
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Change default timings for faster demo behaviout
ADV_PER_SLICE ?= 10           # 10 advertisements per slice
CFLAGS += -DCONFIG_ADV_PER_SLICE=$(ADV_PER_SLICE)
EPOCH_DURATION_SEC ?= 150     # 5min
CFLAGS += -DCONFIG_EPOCH_DURATION_SEC=$(EPOCH_DURATION_SEC)
MIN_EXPOSURE_TIME_S ?= 30
CFLAGS+=-DMIN_EXPOSURE_TIME_S=$(MIN_EXPOSURE_TIME_S)
MAX_DISTANCE_CM ?= 1000       # 10m so everything is logged
CFLAGS += -DMAX_DISTANCE_CM=$(MAX_DISTANCE_CM)
TWR_LISTEN_WINDOW_US ?= 2000  # 2ms listen windows
CFLAGS += -DCONFIG_TWR_LISTEN_WINDOW_US=$(TWR_LISTEN_WINDOW_US)
ADV_ITVL_MS ?= 1000           # 1 adv per second
CFLAGS += -DCONFIG_BLE_ADV_ITVL_MS=$(ADV_ITVL_MS)

# Disable shell echo
CFLAGS += -DCONFIG_SHELL_NO_ECHO=1
# Force blocking shell
CFLAGS += -DCONFIG_PEPPER_SHELL_BLOCKING=1

# Log BLE scan information over serial
PEPPER_LOG_BLE ?= 1
CFLAGS += -DCONFIG_PEPPER_LOG_BLE=$(PEPPER_LOG_BLE)
# Log UWB scan information over serial
PEPPER_LOG_UWB ?= 1
CFLAGS += -DCONFIG_PEPPER_LOG_UWB=$(PEPPER_LOG_UWB)

# The event thread will handle:
# * advertisement
# * twr
# * start/end of epoch
# * other app event
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_LARGE
CFLAGS += -DEVENT_THREAD_HIGHEST_STACKSIZE=THREAD_STACKSIZE_LARGE

# Increase ISR stacksize
CFLAGS += -DISR_STACKSIZE=1024

# Use a different PANID
CFLAGS += -DMYNEWT_VAL_PANID=0xcafe

# Increase memory buffers to account for more neighbors
CFLAGS += -DCONFIG_TWR_EVENT_BUF_SIZE=40
CFLAGS += -DCONFIG_ED_BUF_SIZE=20

include $(RIOTBASE)/Makefile.include
