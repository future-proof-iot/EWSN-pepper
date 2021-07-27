BINS = contact_filter.bin
OBJS = contact_filter.o

LLC = llc-12
CLANG = clang-12
OBJCOPY = llvm-objcopy-12
INC_FLAGS = -nostdinc -isystem `$(CLANG) -print-file-name=include`
EXTRA_CFLAGS = -Os -emit-llvm

BPFINCLUDE ?= -I$(CURDIR)/../include/bpf

all: $(BINS)
	@true

.PHONY: clean

clean:
	@rm -f $(OBJS) $(BINS)

INC_FLAGS = -nostdinc -isystem `$(CLANG) -print-file-name=include`

$(OBJS):  %.o:%.c
	@$(CLANG) $(INC_FLAGS) \
	        $(BPFINCLUDE) \
	        -Wno-unused-value -Wno-pointer-sign -g3\
	        -Wno-compare-distinct-pointer-types \
	        -Wno-gnu-variable-sized-type-not-at-end \
	        -Wno-address-of-packed-member -Wno-tautological-compare \
	        -Wno-unknown-warning-option \
	        $(EXTRA_CFLAGS) -c $< -o -| $(LLC) -march=bpf -filetype=obj -o $@

$(BINS): %.bin:%.o
	@$(OBJCOPY) --output-target=binary -j .text $< $@
