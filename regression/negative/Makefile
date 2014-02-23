SJASM = ../../sjasmplus
SRC_SUFFIX = .asm
OUT_SUFFIX = .out

tests = $(wildcard *$(SRC_SUFFIX))
results = $(patsubst %$(SRC_SUFFIX),%$(OUT_SUFFIX),$(tests))

all: $(results)

#only negative result is acceptable
%$(OUT_SUFFIX): %$(SRC_SUFFIX) $(SJASM)
	! $(SJASM) --nologo $< 2>&1 > $@
