APPLICATION = test_desire_server

BOARD ?= dwm1001

# All uwb-core applications need to enable `-fms-extensions`
CFLAGS += -fms-extensions
ifneq (,$(filter llvm,$(TOOLCHAIN)))
  CFLAGS += -Wno-microsoft-anon-tag
endif

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Pepper Server interface
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/uwb/

# Test App requirements
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += event_thread
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_msec

FEATURES_OPTIONAL += periph_gpio_irq

# pepper server
USEMODULE += pepper_srv
USEMODULE += pepper_srv_coaps
USEMODULE += pepper_srv_leds
# EDHOC requires a big stack
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=3*THREAD_STACKSIZE_LARGE

# Epoch data generation : include basic uwb and ble
USEMODULE += epoch_random
USEMODULE += ed_uwb

# SUIT configuration
# If following the README this will be the SUIT_COAP server configuration
SUIT_COAP_SERVER ?= [fd00:dead:beef::1]
SUIT_CLIENT ?= [2001:db8::d359:5dff:fe10:4cf8]
SUIT_COAP_FSROOT ?= $(CURDIR)/coaproot

ifneq (native,$(BOARD))
  USEMODULE += ble_scanner
  USEMODULE += ble_scanner_netif
endif

DEVELHELP ?= 1

include $(RIOTBASE)/Makefile.include
