APPLICATION = blebr

BOARD ?= nrf52840dongle

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Include packages that pull up and auto-init the link layer.
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_sixlowpan_border_router_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_icmpv6_error

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# BLE modules
USEMODULE += nimble_autoconn_ipsp
USEMODULE += nimble_netif
NIMBLE_MAX_CONN ?= 10

# Better debugging
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6
USEMODULE += netstats_rpl

USEMODULE += netstats_neighbor_etx
USEMODULE += netstats_neighbor_count
USEMODULE += netstats_neighbor_rssi
USEMODULE += netstats_neighbor_lqi
USEMODULE += netstats_neighbor_tx_time

# GNRC configurations
CFLAGS +=-DMYNEWT_VAL_BLE_LL_TX_PWR_DBM=4
CFLAGS += -DCONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF=3
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_OFFL_NUMOF=32
CFLAGS += -DCONFIG_GNRC_IPV6_NIB_NUMOF=16
# USB configuration
CFLAGS += -DCONFIG_USB_PRODUCT_STR=\"RIOTblebr\"

# Change this to 0 to not use ethos
UPLINK ?= cdc-ecm
# Default prefix
IPV6_PREFIX ?= 2001:db8::/64

ifeq (ethos,$(UPLINK))
  # ethos baudrate can be configured from make command
  USEMODULE += gnrc_uhcpc
  USEMODULE += stdio_ethos
  include $(CURDIR)/Makefile.ethos.conf
else ifeq (cdc-ecm,$(UPLINK))
  USEMODULE += gnrc_uhcpc
  include $(CURDIR)/Makefile.cdc-ecm.conf
else
  $(error Supported uplinks are `ethos`, `cdc-ecm`)
endif

include $(RIOTBASE)/Makefile.include

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS $(MAKE) -C $(RIOTTOOLS)

uhcpd-daemon: host-tools
	$(RIOTTOOLS)/uhcpd/bin/uhcpd $(TAP) $(IPV6_PREFIX)
