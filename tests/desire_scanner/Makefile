APPLICATION = test_desire_scanner

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Desire BLE Scanner
USEMODULE += desire_scanner
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble

# EBID module
USEMODULE += ebid
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys

# Shell module for tests
USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include
