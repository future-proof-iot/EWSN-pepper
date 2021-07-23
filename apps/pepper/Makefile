APPLICATION = pepper
BOARD ?= dwm1001

# All uwb-core applications need to enable `-fms-extensions`
CFLAGS += -fms-extensions
ifneq (,$(filter llvm,$(TOOLCHAIN)))
  CFLAGS += -Wno-microsoft-anon-tag
endif

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += desire_advertiser
USEMODULE += desire_scanner
USEMODULE += desire_scanner_time_update
USEMODULE += desire_scanner_netif
USEMODULE += desire_scanner_netif
USEMODULE += uwb_epoch
USEMODULE += twr
USEMODULE += state_manager
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/uwb/

USEMODULE += event_thread_highest
USEMODULE += event_thread_medium
USEMODULE += event_timeout_ztimer
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_epoch

# Uncomment to modify default rotation values
CFLAGS+=-DCONFIG_SLICE_ROTATION_T_S=1
CFLAGS+=-DCONFIG_EBID_ROTATION_T_S=30
CFLAGS+=-DMIN_EXPOSURE_TIME_S=10

# increase stack size
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_MEDIUM
CFLAGS += -DEVENT_THREAD_HIGHEST_STACKSIZE=THREAD_STACKSIZE_LARGE

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
# Include SS TWR
USEMODULE += uwb-core_twr_ss

FEATURES_OPTIONAL += periph_gpio_irq

# Add Ipv6
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
# Add coap utility
USEMODULE += coap_utils

include $(RIOTBASE)/Makefile.include
