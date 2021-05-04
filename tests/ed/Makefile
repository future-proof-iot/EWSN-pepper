APPLICATION = test_ed

BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += embunit

USEMODULE += ed
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/ed

DEFAULT_MODULE += test_utils_interactive_sync

include $(RIOTBASE)/Makefile.include
