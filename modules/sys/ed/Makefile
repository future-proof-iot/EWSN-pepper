SUBMODULES = 1

SRC += common.c

ifneq (,$(filter ed_uwb_bpf,$(USEMODULE)))
  BLOBS += bpf/contact_filter.bin
endif

include $(RIOTBASE)/Makefile.base
