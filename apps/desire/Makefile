APPLICATION = desire

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += desire_advertiser
USEMODULE += desire_scanner
USEMODULE += desire_scanner_time_update
USEMODULE += epoch
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/

USEMODULE += event_thread_highest
USEMODULE += event_thread_medium
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_epoch

# Uncomment to modify default rotation values
# CFLAGS+=-DCONFIG_SLICE_ROTATION_T_S=1
# CFLAGS+=-DCONFIG_EBID_ROTATION_T_S=20
# CFLAGS+=-DMIN_EXPOSURE_TIME_S=10

CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_MEDIUM
CFLAGS += -DEVENT_THREAD_HIGHEST_STACKSIZE=THREAD_STACKSIZE_LARGE

USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include
