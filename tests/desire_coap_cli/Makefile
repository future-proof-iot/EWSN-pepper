# name of your application
APPLICATION = gcoap_desire_cli

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# TODO: remove this should be in its own module abstracted from the app
USEPKG += nanocbor

# Include coap_utils module
USEMODULE += coap_utils
USEMODULE += state_manager
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble

ifneq (native,$(BOARD))
  USEMODULE += desire_scanner
  USEMODULE += desire_scanner_netif
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1
include $(RIOTBASE)/Makefile.include
