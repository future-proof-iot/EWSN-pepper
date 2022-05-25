APPLICATION = test_fading

BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += ztimer_periph_timer
USEMODULE += ztimer_usec
USEMODULE += ztimer_msec
USEMODULE += random
USEMODULE += printf_float
DEFAULT_MODULE += test_utils_interactive_sync

include $(RIOTBASE)/Makefile.include
