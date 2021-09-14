APPLICATION = pepper_riotfp
BOARD ?= dwm1001

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1
# All uwb-core applications need to enable `-fms-extensions`
CFLAGS += -fms-extensions
ifneq (,$(filter llvm,$(TOOLCHAIN)))
  CFLAGS += -Wno-microsoft-anon-tag
endif

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT
# Path to external modules search directory
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble/
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/uwb/

#### PEPPER required modules ####
USEMODULE += desire_advertiser
USEMODULE += desire_scanner
USEMODULE += desire_scanner_netif
USEMODULE += desire_scanner_time_update
USEMODULE += uwb_epoch
USEMODULE += twr
USEMODULE += state_manager
USEMODULE += current_time
# Add coap utility
USEMODULE += coap_utils

#### RIOT-fp related modules ####
# Add bpf update related modules
USEMODULE += uwb_ed_bpf
# Add Edhoc realted Modules
USEMODULE += state_manager_security

#### Application required modules ####
USEMODULE += event_thread
USEMODULE += event_timeout_ztimer
USEMODULE += event_periodic
USEMODULE += event_callback
USEMODULE += ztimer_epoch
# Add Ipv6
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += nanocoap_sock
# Include shell and process information fonctionality
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
# This will enable demo features on the buttons
FEATURES_OPTIONAL += periph_gpio_irq
# Add suit_storage_flashwrite to update firmware as well
USEMODULE += suit_storage_flashwrite
USEMODULE += suit_periodic_fetch

# The event thread will handle:
# * advertisement
# * twr
# * start/end of epoch
# * other app event
# CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_MAIN
# A very large stack is required when adding EDHOC
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=3*THREAD_STACKSIZE_LARGE
# GCOAP stack will to some COSE decoding so needs a larger stack
CFLAGS += -DGCOAP_STACK_SIZE=THREAD_STACKSIZE_LARGE
# Increase resend buffer to be able to handle GET and BLOCK requests simultaneously
CFLAGS += -DCONFIG_GCOAP_RESEND_BUFS_MAX=4

# SUIT configuration
# If following the README this will be the SUIT_COAP server configuration
SUIT_COAP_SERVER ?= [fd00:dead:beef::1]:5684
SUIT_COAP_FSROOT ?= $(CURDIR)/coaproot
CFLAGS += -DSUIT_COAP_ROOT=\"$(SUIT_COAP_ROOT)\"

# GNRC PKTBUF is allocated in order to handler 4 full-MTU IPV6 packets,
# we are not using the full MTU so there should be no need for such a huge
# buffer size
ifndef CONFIG_GNRC_PKTBUF_SIZE
  CFLAGS += -DCONFIG_GNRC_PKTBUF_SIZE=2048
endif

# Change default timings for faster demo behaviout
CFLAGS+=-DCONFIG_SLICE_ROTATION_T_S=1
CFLAGS+=-DCONFIG_EBID_ROTATION_T_S=30
CFLAGS+=-DMIN_EXPOSURE_TIME_S=10

include $(RIOTBASE)/Makefile.include
