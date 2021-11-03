ifneq (,$(filter ed_uwb_bpf,$(USEMODULE)))
CFLAGS += -DCONFIG_SUIT_STORAGE_RAM_REGIONS=1
CFLAGS += -DCONFIG_SUIT_STORAGE_RAM_SIZE=512

# Required variables defined in riotboot.inc.mk
BINDIR_APP = $(BINDIR)/$(APPLICATION)
EPOCH := $(shell date +%s)
APP_VER ?= $(EPOCH)

UWB_ED_BPF_DIR := $(LAST_MAKEFILEDIR)/bpf
SUIT_UWB_ED_PAYLOAD ?= $(UWB_ED_BPF_DIR)/contact_filter.bin
SUIT_UWB_ED_PAYLOAD_BIN ?= $(BINDIR_APP)/contact_filter.$(APP_VER).bin

$(SUIT_UWB_ED_PAYLOAD_BIN): $(SUIT_UWB_ED_PAYLOAD)
	$(Q)cp $(SUIT_UWB_ED_PAYLOAD) $@

ifneq (, $(filter suit/%_bpf, $(MAKECMDGOALS)))
  SUIT_MANIFEST_NAME ?= bpf
  SUIT_MANIFEST_PAYLOADS ?= $(SUIT_UWB_ED_PAYLOAD_BIN)
  SUIT_MANIFEST_SLOTFILES ?= $(SUIT_UWB_ED_PAYLOAD_BIN):0:ram:0
endif
endif

BUILD_FILES += $(SUIT_UWB_ED_PAYLOAD)
.PHONY: FORCE suit/publish_bpf suit/notify_bpf
$(UWB_ED_BPF_DIR)/contact_filter.bin: FORCE
	$(Q)/usr/bin/env -i \
	RIOTBASE=$(RIOTBASE) \
	EXTRA_CFLAGS="$(BPF_CFLAGS)" \
	"$(MAKE)" --no-print-directory -C $(UWB_ED_BPF_DIR) clean all

suit/publish_bpf: suit/publish
	@true

suit/notify_bpf: suit/notify
	@true
