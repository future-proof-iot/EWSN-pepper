APPLICATION = sdcard_logger

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
USEMODULE += epoch
USEMODULE += epoch_random
USEMODULE += ed_uwb
USEMODULE += ed_ble
USEMODULE += ztimer_epoch
USEMODULE += ztimer_msec
USEMODULE += ztimer_usec
USEMODULE += storage

include $(RIOTBASE)/Makefile.include
