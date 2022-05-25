ifneq (,$(filter ed_uwb_bpf,$(USEMODULE)))
## Add an application for sofwate updates
UWB_ED_BPF_DIR := $(LAST_MAKEFILEDIR)/bpf
F12R_DIRS  += $(UWB_ED_BPF_DIR)
F12R_BLOBS  += $(UWB_ED_BPF_DIR)/contact_filter.bin

# Define RAM Regions
CFLAGS += -DCONFIG_SUIT_STORAGE_RAM_REGIONS=1
CFLAGS += -DCONFIG_SUIT_STORAGE_RAM_SIZE=512

# Required variables defined in riotboot.inc.mk or Makefile.include
BINDIR_APP = $(CURDIR)/bin/$(BOARD)/$(APPLICATION)
$(BINDIR_APP): $(CLEAN)
	$(Q)mkdir -p $(BINDIR_APP)
EPOCH := $(shell date +%s)
APP_VER ?= $(EPOCH)

SUIT_UWB_ED_PAYLOAD ?= $(UWB_ED_BPF_DIR)/contact_filter.bin
SUIT_UWB_ED_PAYLOAD_BIN ?= $(BINDIR_APP)/contact_filter.$(APP_VER).bin
BUILD_FILES += $(SUIT_UWB_ED_PAYLOAD)
SUIT_UWB_ED_STORAGE ?= ram:0

$(SUIT_UWB_ED_PAYLOAD_BIN): $(SUIT_UWB_ED_PAYLOAD)
	$(Q)cp $(SUIT_UWB_ED_PAYLOAD) $@

ifneq (, $(filter suit/%_bpf, $(MAKECMDGOALS)))
  SUIT_MANIFEST_BASENAME ?= bpf_suit
  SUIT_MANIFEST_PAYLOADS ?= $(SUIT_UWB_ED_PAYLOAD_BIN)
  SUIT_MANIFEST_SLOTFILES ?= $(SUIT_UWB_ED_PAYLOAD_BIN):0:$(SUIT_UWB_ED_STORAGE)
endif
endif

.PHONY: FORCE suit/publish_bpf suit/notify_bpf

suit/publish_bpf: suit/publish
	@true

suit/notify_bpf: suit/notify
	@true
