# name of your application
APPLICATION = uwb_ed_suit

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Include uwb_ed_bpf modules
USEMODULE += uwb_ed
USEMODULE += uwb_ed_bpf
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys

# Include required networking modules
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += netdev_default

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

# Include the shell
USEMODULE += shell
USEMODULE += shell_commands

# SUIT configuration
# If following the README this will be the SUIT_COAP server configuration
SUIT_COAP_SERVER ?= [2001:db8::1]
SUIT_CLIENT ?= [2001:db8::2]
SUIT_COAP_FSROOT ?= $(CURDIR)/coaproot

include $(RIOTBASE)/Makefile.include
include $(RIOTMAKE)/default-radio-settings.inc.mk

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS $(MAKE) -C $(RIOTTOOLS)
