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
USEMODULE += gnrc_netdev_default

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
