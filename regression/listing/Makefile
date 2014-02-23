SJASM = ../../sjasmplus
SRC_SUFFIX = .asm
LST_SUFFIX = .lst

tests = $(wildcard *$(SRC_SUFFIX))
results = $(patsubst %$(SRC_SUFFIX),%$(LST_SUFFIX),$(tests))

all: $(results)

%$(LST_SUFFIX): %$(SRC_SUFFIX) $(SJASM)
	$(SJASM) --nologo --lst=$@ $<
