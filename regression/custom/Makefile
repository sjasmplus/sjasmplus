SJASM = ../../sjasmplus

all: testopts trd

testopts: test.asm
	$(SJASM) --nologo --lstlab --lst=test.lst --sym=test.sym --exp=test.exp --raw=test.raw $<

trd: trd.asm
	$(SJASM) --nologo $<
