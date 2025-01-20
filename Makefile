CC = cc
CCOPTS = -ansi -Wpedantic -Isrc

# This should be the 6502 oldstyle version of vasm.
VASM = vasm6502

all: v6502c hello

v6502c: obj obj/v6502.o src/main.c src/main.h
	${CC} ${CCOPTS} obj/v6502.o src/main.c -o v6502c

obj/v6502.o: obj src/inst.h src/v6502.h src/v6502.c
	${CC} ${CCOPTS} -c src/v6502.c -o obj/v6502.o

hello: obj obj/v6502.o src/hello.c
	${CC} ${CCOPTS} -c src/hello.c -o obj/v6502.o

src/hello.bin: src/hello.s
	${VASM} -Fbin -o src/hello.bin src/hello.s

src/hello.woz: src/hello.s
	${VASM} -Fwoz -o src/hello.woz src/hello.s

obj:
	mkdir -p obj

clean:
	rm -f v6502c *.o
	rm -f obj/*
	rm -f src/*.*~
