
APPLICATION ?= tests_$(notdir $(patsubst %/,%,$(CURDIR)))

ifneq (,$(wildcard $(CURDIR)/tests*/.))
  DEFAULT_MODULE += test_utils_interactive_sync
endif

BOARD ?= native
RIOTBASE ?= $(CURDIR)/../../RIOT
QUIET ?= 1
DEVELHELP ?= 0

USEMODULE += embunit

ifeq (, $(filter tests-%, $(MAKECMDGOALS)))
  # the $(dir) Makefile function leaves a trailing slash after the directory
  # name, therefore we use patsubst instead.
  UNIT_TESTS := $(patsubst %/Makefile,%,$(wildcard tests-*/Makefile))
else
  UNIT_TESTS := $(filter tests-%, $(MAKECMDGOALS))
endif

DISABLE_MODULE += auto_init auto_init_%

# boards using stdio via CDC ACM require auto_init to automatically
# initialize stdio over USB.
FEATURES_BLACKLIST += highlevel_stdio

# Pull in `Makefile.include`s and `Makefile.dep` from the test suites:
-include $(UNIT_TESTS:%=$(CURDIR)/%/Makefile.dep)
-include $(UNIT_TESTS:%=$(CURDIR)/%/Makefile.include)
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/sys
EXTERNAL_MODULE_DIRS += $(CURDIR)/../../modules/ble

DIRS += $(UNIT_TESTS)
BASELIBS += $(UNIT_TESTS:%=%.module)
INCLUDES += -I$(RIOTBASE)/tests/unittests/common

# some tests need more stack
CFLAGS += -DTHREAD_STACKSIZE_MAIN=2*THREAD_STACKSIZE_LARGE

include $(RIOTBASE)/Makefile.include

.PHONY: $(UNIT_TESTS)

all:

info-unittests:
	@echo $(UNIT_TESTS)

$(UNIT_TESTS): all

ifeq (, $(UNIT_TESTS))
  CFLAGS += -DNO_TEST_SUITES
  $(warning There was no test suite specified!)
else
  CFLAGS += -DTEST_SUITES='$(subst $() $(),$(comma),$(UNIT_TESTS:tests-%=%))'
endif
