APPLICATION = desire

BOARD ?= dwm1001

DEVELHELP ?= 1

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT
# External modules search path
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/

# DESIRE related modules
USEMODULE += desire_advertiser
USEMODULE += desire_scanner
USEMODULE += desire_scanner_time_update
USEMODULE += epoch
# Modules used by the application
USEMODULE += event_thread
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_epoch
USEMODULE += current_time
# Include shell and, process information fonctionality
USEMODULE += ps
USEMODULE += shell
USEMODULE += shell_commands

# Uncomment to modify default rotation values
# CFLAGS+=-DCONFIG_SLICE_ROTATION_T_S=1
# CFLAGS+=-DCONFIG_EBID_ROTATION_T_S=20
# CFLAGS+=-DMIN_EXPOSURE_TIME_S=10

# If HaCL is used for crypto_manager then additional stack is needed so increase
# for that case
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_LARGE

include $(RIOTBASE)/Makefile.include
