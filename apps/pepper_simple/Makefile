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
USEMODULE += pepper_shell
USEMODULE += pepper_util
USEMODULE += ed_uwb
USEMODULE += ed_ble
USEMODULE += twr_shell

# Include shell and, process information fonctionality
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# The event thread will handle:
# * advertisement
# * twr
# * start/end of epoch
# * other app events
CFLAGS += -DEVENT_THREAD_HIGHEST_STACKSIZE=THREAD_STACKSIZE_LARGE
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_LARGE

include $(RIOTBASE)/Makefile.include
