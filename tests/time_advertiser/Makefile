APPLICATION = test_desire_advertiser

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# configure and use Nimble pkg with the autoadv without name and manual start from our app
USEPKG += nimble
USEMODULE += nimble_svc_gap
USEMODULE += nimble_autoadv
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_START_MANUALLY=1
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_DEVICE_NAME='"CTB"'
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_FLAG_FIELD=0

# Advertisement packet header
CFLAGS += -I$(CURDIR)/../../modules/ble/include

# Add a USB product string for udev rules mapping
CFLAGS += -DCONFIG_USB_PRODUCT_STR=\"RIOTbletimeadv\"

# event module
USEMODULE += event_timeout_ztimer
USEMODULE += event_thread

USEMODULE += ztimer_epoch
USEMODULE += ztimer_msec

# Shell
USEMODULE += shell
USEMODULE += shell_commands

include $(RIOTBASE)/Makefile.include
