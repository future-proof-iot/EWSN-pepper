APPLICATION = pepper_demo

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
## Encounter data modules, only UWB
USEMODULE += ed_uwb
## Include pepper server
USEMODULE += pepper_srv
USEMODULE += pepper_srv_coap
USEMODULE += pepper_srv_leds
## Include to declare infections, and schedule esr updates
FEATURES_REQUIED += periph_gpio_irq
USEMODULE += event_periodic
USEMODULE += ztimer_msec

# Include shell and, process information fonctionality
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Add ble_scanner with netif support, could be moved elsewhere
USEMODULE += ble_scanner
USEMODULE += ble_scanner_netif

# Change default timings for faster demo behaviout
ADV_PER_SLICE ?= 5            # 5 advertisements per slice
CFLAGS += -DCONFIG_ADV_PER_SLICE=$(ADV_PER_SLICE)
EPOCH_DURATION_SEC ?= 120     # 2min
CFLAGS += -DCONFIG_EPOCH_DURATION_SEC=$(EPOCH_DURATION_SEC)
MAX_DISTANCE_CM ?= 200        # 2m
CFLAGS += -DMAX_DISTANCE_CM=$(MAX_DISTANCE_CM)
MIN_EXPOSURE_TIME_S ?= 30     # 30s minimum exposure
CFLAGS+=-DMIN_EXPOSURE_TIME_S=$(MIN_EXPOSURE_TIME_S)
TWR_LISTEN_WINDOW_US ?= 2000  # 2ms listen windows
CFLAGS += -DCONFIG_TWR_LISTEN_WINDOW_US=$(TWR_LISTEN_WINDOW_US)
ADV_ITVL_MS ?= 1000           # 1 adv per second
CFLAGS += -DCONFIG_BLE_ADV_ITVL_MS=$(ADV_ITVL_MS)
SCAN_ITVL_MS ?= 1280          # scan all the time for the demo
CFLAGS += -DCONFIG_BLE_SCAN_ITVL_MS=$(SCAN_ITVL_MS)
SCAN_WIN_MS ?= 1280
CFLAGS += -DCONFIG_BLE_SCAN_WIN_MS=$(SCAN_WIN_MS)

# Log BLE scan information over serial
PEPPER_LOG_BLE ?= 0
CFLAGS += -DCONFIG_PEPPER_LOG_BLE=$(PEPPER_LOG_BLE)
# Log UWB scan information over serial
PEPPER_LOG_UWB ?= 0
CFLAGS += -DCONFIG_PEPPER_LOG_UWB=$(PEPPER_LOG_UWB)

# The event thread will handle:
# * advertisement
# * twr
# * start/end of epoch
# * other app event
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_LARGE
CFLAGS += -DEVENT_THREAD_HIGHEST_STACKSIZE=THREAD_STACKSIZE_LARGE

include $(RIOTBASE)/Makefile.include
