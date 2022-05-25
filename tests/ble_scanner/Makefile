APPLICATION = ble_scanner

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += ble_scanner
USEMODULE += current_time
USEMODULE += current_time_ble_scanner
USEMODULE += ble_scanner_netif

EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys

# Shell module for tests
USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include
