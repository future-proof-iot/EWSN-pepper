BASE_MODULE = uwb_ed
SUBMODULES = 1

SRC := uwb_ed.c

ifneq (,$(filter uwb_ed_bpf,$(USEMODULE)))
  SRC += uwb_ed_bpf.c
  BLOBS += bpf/contact_filter.bin
  UWB_ED_BPF_DIR := $(CURDIR)/bpf
endif

include $(RIOTBASE)/Makefile.base
