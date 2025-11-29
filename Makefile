CC = clang-18
CCOPTS = -ansi -Wpedantic -Isrc

# This should be the 6502 oldstyle version of vasm.
VASM = vasm6502

all: v6502c hello bin2woz

v6502c: bin/v6502c

hello: bin/hello

bin2woz: bin/bin2woz

cputest: bin/cputest

devtest: bin/devtest

addrtest: bin/addrtest

test: bin/cputest bin/devtest bin/addrtest
	./bin/cputest
	./bin/devtest
	./bin/addrtest

bin/v6502c: bin obj obj/v6502.o obj/devices.o obj/addrlist.o src/main.c src/main.h
	${CC} ${CCOPTS} obj/v6502.o obj/devices.o obj/addrlist.o src/main.c -o bin/v6502c

obj/v6502.o: obj src/inst.h src/v6502.h src/v6502.c src/vtypes.h
	${CC} ${CCOPTS} -c src/v6502.c -o obj/v6502.o

obj/devices.o: obj src/devices.h src/devices.c src/v6502.h src/vtypes.h
	${CC} ${CCOPTS} -c src/devices.c -o obj/devices.o

obj/addrlist.o: obj src/addrlist.h src/addrlist.c src/vtypes.h
	${CC} ${CCOPTS} -c src/addrlist.c -o obj/addrlist.o

bin/hello: bin obj obj/v6502.o src/hello.c src/hello.h
	${CC} ${CCOPTS} obj/v6502.o src/hello.c -o bin/hello

bin/cputest: bin obj obj/v6502.o src/cputest.c src/cputest.h
	${CC} ${CCOPTS} obj/v6502.o src/cputest.c -o bin/cputest

bin/devtest: bin obj obj/v6502.o obj/devices.o src/devtest.c src/devices.h
	${CC} ${CCOPTS} obj/v6502.o obj/devices.o src/devtest.c -o bin/devtest

bin/addrtest: bin obj obj/addrlist.o src/addrtest.c src/addrlist.h
	${CC} ${CCOPTS} obj/addrlist.o src/addrtest.c -o bin/addrtest

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
