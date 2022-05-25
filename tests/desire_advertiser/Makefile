APPLICATION = test_desire_advertiser

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Desire BLE Advertiser
USEMODULE += desire_advertiser
USEMODULE += desire_advertiser_threaded
USEMODULE += desire_advertiser_shell
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys

# Shell
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

DEVELHELP ?= 1

include $(RIOTBASE)/Makefile.include
