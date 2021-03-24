.PHONY: all clean build init-submodules RIOT-branch-%

APPLICATIONS = $(wildcard $(CURDIR)/tests/* )

all: build

clean:
	for app in $(APPLICATIONS); do make -C $$app distclean; done

build:
	for app in $(APPLICATIONS); do make -C $$app all; done

test:
	for app in $(APPLICATIONS); do make -C $$app flash test; done

init-submodules:
	git submodule update --init --recursive
	git submodule update

info-debug-variable-%:
	@echo $($*)
