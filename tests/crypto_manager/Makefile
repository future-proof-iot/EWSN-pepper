APPLICATION = test_crypto_manager

BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

USEMODULE += embunit
USEMODULE += crypto_manager
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys/crypto_manager

DEFAULT_MODULE += test_utils_interactive_sync

include $(RIOTBASE)/Makefile.include
