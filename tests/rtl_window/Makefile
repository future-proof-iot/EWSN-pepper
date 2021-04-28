APPLICATION = test_edl

BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += embunit

USEMODULE += edl
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/edl
USEMODULE += rdl_window
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/rdl_window

DEFAULT_MODULE += test_utils_interactive_sync

USEMODULE += ztimer_usec

include $(RIOTBASE)/Makefile.include
