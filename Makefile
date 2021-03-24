.PHONY: all clean build init-submodules RIOT-branch-%

APPLICATIONS =      \
    01_uwb_rng      \
    02_ble_scan_rss \
    #

all: build

clean:
	for app in $(APPLICATIONS); do make -C $$app distclean; done

build:
	for app in $(APPLICATIONS); do make -C $$app all; done

init-submodules:
	git submodule update --init --recursive
	git submodule update
