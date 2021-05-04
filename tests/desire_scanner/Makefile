APPLICATION = test_desire_scanner

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Desire BLE Scanner
USEMODULE += desire_scanner
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/desire_scanner

# Shell module for tests
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += xtimer

include $(RIOTBASE)/Makefile.include
