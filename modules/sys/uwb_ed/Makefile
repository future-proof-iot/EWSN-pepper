ifneq (,$(filter bpf,$(USEMODULE)))
  BLOBS += bpf/meaningfull_contact.bin
  UWB_ED_BPF_DIR := $(CURDIR)/bpf
endif

include $(RIOTBASE)/Makefile.base

.PHONY: FORCE
bpf/meaningfull_contact.bin: FORCE
	$(Q)/usr/bin/env -i \
	"$(MAKE)" --no-print-directory -C $(UWB_ED_BPF_DIR) clean all
