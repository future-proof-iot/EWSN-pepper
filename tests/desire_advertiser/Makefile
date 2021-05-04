APPLICATION = test_desire_advertiser

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Desire BLE Advertiser
USEMODULE += desire_advertiser
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/desire_advertiser

# Shell
USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include
