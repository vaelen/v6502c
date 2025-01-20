CC = cc
CCOPTS = -ansi -Wpedantic -Isrc

# This should be the 6502 oldstyle version of vasm.
VASM = vasm6502

all: v6502c hello

v6502c: bin/v6502c

hello: bin/hello

bin/v6502c: bin obj obj/v6502.o src/main.c src/main.h
	${CC} ${CCOPTS} obj/v6502.o src/main.c -o bin/v6502c

obj/v6502.o: obj src/inst.h src/v6502.h src/v6502.c
	${CC} ${CCOPTS} -c src/v6502.c -o obj/v6502.o

bin/hello: bin obj obj/v6502.o src/hello.c
	${CC} ${CCOPTS} obj/v6502.o src/hello.c -o bin/hello

src/hello.bin: src/hello.s
	${VASM} -Fbin -o src/hello.bin src/hello.s

src/hello.woz: src/hello.s
	${VASM} -Fwoz -o src/hello.woz src/hello.s

bin:
	mkdir -p bin

obj:
	mkdir -p obj

clean:
	rm -f bin/*
	rm -f obj/*
	rm -f src/*.*~
	rm -f *.*~
