APPLICATION = time_server

BOARD ?= dwm1001

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/

# configure and use Nimble pkg with the autoadv without name and manual start from our app
USEPKG += nimble
USEMODULE += nimble_svc_gap
USEMODULE += nimble_adv_ext
USEMODULE += bluetil_ad
USEMODULE += current_time
USEMODULE += rtc_utils

# Add a USB product string for udev rules mapping
CFLAGS += -DCONFIG_NIMBLE_AUTOADV_DEVICE_NAME=\"RIOTTimeServer\"
CFLAGS += -DCONFIG_USB_PRODUCT_STR=\"RIOTTimeServer\"

# event module
USEMODULE += event_timeout_ztimer
USEMODULE += event_thread
CFLAGS += -DEVENT_THREAD_MEDIUM_STACKSIZE=THREAD_STACKSIZE_LARGE

# this application uses advertisement instance 0, so bump the initial instances
# here, when it becomes an external module this should not be needed
BLE_MULTI_ADV_INSTANCES = 0

# Shell
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
