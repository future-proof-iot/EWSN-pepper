APPLICATION = pepper

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
USEMODULE += desire_advertiser
USEMODULE += desire_scanner
USEMODULE += desire_scanner_netif
USEMODULE += desire_scanner_time_update
USEMODULE += uwb_epoch
USEMODULE += twr
USEMODULE += state_manager
USEMODULE += current_time
# Modules used in the application
USEMODULE += event_thread
USEMODULE += event_timeout_ztimer
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_epoch
USEMODULE += ztimer_sec
# Include shell and, process information fonctionality
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Uncomment to modify default rotation values
# CFLAGS+=-DCONFIG_SLICE_ROTATION_T_S=1
# CFLAGS+=-DCONFIG_EBID_ROTATION_T_S=30
# CFLAGS+=-DMIN_EXPOSURE_TIME_S=10

# The event thread will handle:
# * advertisement
# * twr
# * start/end of epoch
# * other app event
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_MAIN

include $(RIOTBASE)/Makefile.include
