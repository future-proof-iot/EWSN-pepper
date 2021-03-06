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
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_LARGE

FEATURES_OPTIONAL += periph_gpio_irq

# pepper server
USEMODULE += pepper_srv
USEMODULE += pepper_srv_shell
USEMODULE += pepper_srv_coap
# USEMODULE += pepper_srv_leds

# Epoch data generation : include basic uwb and ble
USEMODULE += epoch_random
USEMODULE += ed_uwb
USEMODULE += ed_ble

ifneq (native,$(BOARD))
  USEMODULE += ble_scanner
  USEMODULE += ble_scanner_netif
endif

DEVELHELP ?= 1

include $(RIOTBASE)/Makefile.include
