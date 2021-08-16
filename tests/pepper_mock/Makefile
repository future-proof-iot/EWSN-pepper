# name of your application
APPLICATION = pepper_mock

#
DEVELHELP ?= 1

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_ipv6_router_default
USEMODULE += sock_udp
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo
USEMODULE += nanocoap_sock

# Include this for printing IP addresses
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
# For random data generation
USEMODULE += random
# Include Pepper realated modules
USEMODULE += coap_utils
USEMODULE += uwb_epoch
USEMODULE += uwb_ed_bpf
USEMODULE += ztimer_epoch
USEMODULE += state_manager
USEMODULE += state_manager_security

# Include event modules
USEMODULE += event_thread
USEMODULE += event_timeout_ztimer
USEMODULE += event_periodic
USEMODULE += event_callback
# EDHOC requires a big stack
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=3*THREAD_STACKSIZE_LARGE

# Increase resende buffer
CFLAGS += -DCONFIG_GCOAP_RESEND_BUFS_MAX=5

# SUIT configuration
# If following the README this will be the SUIT_COAP server configuration
SUIT_COAP_SERVER ?= [fd00:dead:beef::1]
SUIT_CLIENT ?= [2001:db8::d359:5dff:fe10:4cf8]
SUIT_COAP_FSROOT ?= $(CURDIR)/coaproot

# Include EXTERNAL_MODULE_DIRS
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys

# Uncomment to modify default rotation values
CFLAGS += -DCONFIG_EBID_ROTATION_T_S=10
CFLAGS += -DMIN_EXPOSURE_TIME_S=1

ifneq (native,$(BOARD))
  USEMODULE += stdio_ethos
  USEMODULE += gnrc_uhcpc
  # ethos baudrate can be configured from make command
  ETHOS_BAUDRATE ?= 115200
  CFLAGS += -DETHOS_BAUDRATE=$(ETHOS_BAUDRATE)

  # make sure ethos and uhcpd are built
  TERMDEPS += host-tools

  # For local testing, run
  #
  #     $ cd dist/tools/ethos; sudo ./setup_network.sh riot0 2001:db8::0/64
  #
  #... in another shell and keep it running.
  export TAP ?= riot0
  TERMPROG = $(RIOTTOOLS)/ethos/ethos
  TERMFLAGS = $(TAP) $(PORT)
endif

include $(RIOTBASE)/Makefile.include
include $(RIOTMAKE)/default-radio-settings.inc.mk

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS $(MAKE) -C $(RIOTTOOLS)

