.PHONY: all clean build test init-submodules static-checks

# These applications do not currently build since they still depend on state_manager
IGNORE_APPS ?= $(dir $(wildcard $(CURDIR)/apps/pepper_riotfp/ $(CURDIR)/tests/uwb_ed_bpf_suit/))
APPLICATIONS = $(filter-out $(IGNORE_APPS),$(dir $(wildcard $(CURDIR)/tests/*/Makefile $(CURDIR)/apps/*/Makefile)))
TEST_APPLICATIONS = $(dir $(wildcard $(CURDIR)/tests/unittests/))

all: build
	@true

clean:
	@for app in $(APPLICATIONS); do $(MAKE) -C $$app distclean; done

build:
	@for app in $(APPLICATIONS); do $(MAKE) -C $$app; done

test:
	@for app in $(TEST_APPLICATIONS); do $(MAKE) -C $$app flash test; done

init-submodules:
	@git submodule update --init --recursive
	@git submodule update

info-debug-variable-%:
	@echo $($*)

static-checks:
	@cppcheck --std=c11 --enable=style --quiet ./apps ./tests ./modules
