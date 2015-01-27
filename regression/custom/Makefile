SJASM = ../../sjasmplus

all: testopts

testopts: test.asm
	$(SJASM) --nologo --lstlab --lst=test.lst --sym=test.sym --exp=test.exp --raw=test.raw $<
