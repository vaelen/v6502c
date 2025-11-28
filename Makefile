CC = clang-18
CCOPTS = -ansi -Wpedantic -Isrc

# This should be the 6502 oldstyle version of vasm.
VASM = vasm6502

all: v6502c hello test bin2woz

v6502c: bin/v6502c

hello: bin/hello

test: bin/test

bin2woz: bin/bin2woz

bin/v6502c: bin obj obj/v6502.o obj/devices.o src/main.c src/main.h
	${CC} ${CCOPTS} obj/v6502.o obj/devices.o src/main.c -o bin/v6502c

obj/v6502.o: obj src/inst.h src/v6502.h src/v6502.c
	${CC} ${CCOPTS} -c src/v6502.c -o obj/v6502.o

obj/devices.o: obj src/devices.h src/devices.c src/v6502.h
	${CC} ${CCOPTS} -c src/devices.c -o obj/devices.o

bin/hello: bin obj obj/v6502.o src/hello.c src/hello.h
	${CC} ${CCOPTS} obj/v6502.o src/hello.c -o bin/hello

bin/test: bin obj obj/v6502.o src/test.c src/test.h
	${CC} ${CCOPTS} obj/v6502.o src/test.c -o bin/test

bin/bin2woz: bin src/bin2woz.c
	${CC} ${CCOPTS} src/bin2woz.c -o bin/bin2woz

src/hello.h: src/hello.s
	${VASM} -Fbin -dotdir -o src/hello.bin src/hello.s
	${VASM} -Fwoz -dotdir -o src/hello.woz src/hello.s
	xxd -i src/hello.bin > src/hello.h

infloop: src/infloop.woz

src/infloop.woz: src/infloop.s
	${VASM} -Fwoz -dotdir -o src/infloop.woz src/infloop.s

bin:
	mkdir -p bin

obj:
	mkdir -p obj

clean:
	rm -f bin/*
	rm -f obj/*
	rm -f src/*.*~
	rm -f *.*~
