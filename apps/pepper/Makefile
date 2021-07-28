APPLICATION = pepper

BOARD ?= dwm1001

DEVELHELP ?= 1

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
USEMODULE += uwb_ed_bpf
USEMODULE += twr
USEMODULE += state_manager
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/uwb/

USEMODULE += event_thread
USEMODULE += event_timeout_ztimer
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_epoch

# Uncomment to modify default rotation values
CFLAGS+=-DCONFIG_SLICE_ROTATION_T_S=1
CFLAGS+=-DCONFIG_EBID_ROTATION_T_S=30
CFLAGS+=-DMIN_EXPOSURE_TIME_S=10

# The event thread will handle:
# * advertisement
# * twr
# * start/end of epoch
# * other app event
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_MAIN

# Include shell and, process information fonctionality
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# This will enable demo features on the buttons
FEATURES_OPTIONAL += periph_gpio_irq

# Add Ipv6
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
# Add coap utility
USEMODULE += coap_utils

# SUIT configuration
# If following the README this will be the SUIT_COAP server configuration
SUIT_COAP_SERVER ?= [fd00:dead:beef::1]
SUIT_CLIENT ?= [2001:db8::d359:5dff:fe10:4cf8]
SUIT_COAP_FSROOT ?= $(CURDIR)/coaproot

# GNRC PKTBUF is allocated in order to handler 4 full-MTU IPV6 packets,
# we are not using the full MTU so there should be no need for such a huge
# buffer size
ifndef CONFIG_GNRC_PKTBUF_SIZE
  CFLAGS += -DCONFIG_GNRC_PKTBUF_SIZE=2048
endif

include $(RIOTBASE)/Makefile.include
