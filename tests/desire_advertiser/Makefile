APPLICATION = test_desire_advertiser

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Desire BLE Advertiser
# set EBID to static for testing, DDESIRE_STATIC_EBID=1 comment for dynamic ebid
USEMODULE += desire_advertiser
CFLAGS += -DDESIRE_STATIC_EBID=1
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/desire_advertiser

include $(RIOTBASE)/Makefile.include
