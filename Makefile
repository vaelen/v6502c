CC = clang-18
CCOPTS = -ansi -Wpedantic -Isrc

# This should be the 6502 oldstyle version of vasm.
VASM = vasm6502

all: libv6502 v6502c hello bin2woz

libv6502: lib/libv6502.a lib/libv6502.so

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

obj/vmachine.o: obj src/vmachine.h src/vmachine.c src/v6502.h src/vtypes.h src/devices.h src/addrlist.h
	${CC} ${CCOPTS} -c src/vmachine.c -o obj/vmachine.o

obj/monitor.o: obj src/monitor.h src/monitor.c src/vmachine.h src/v6502.h src/vtypes.h
	${CC} ${CCOPTS} -c src/monitor.c -o obj/monitor.o

obj/v6502.o: obj src/inst.h src/v6502.h src/v6502.c src/vtypes.h
	${CC} ${CCOPTS} -c src/v6502.c -o obj/v6502.o

obj/devices.o: obj src/devices.h src/devices.c src/v6502.h src/vtypes.h
	${CC} ${CCOPTS} -c src/devices.c -o obj/devices.o

obj/addrlist.o: obj src/addrlist.h src/addrlist.c src/vtypes.h
	${CC} ${CCOPTS} -c src/addrlist.c -o obj/addrlist.o

# Static library
lib/libv6502.a: lib obj/v6502.o obj/devices.o obj/addrlist.o obj/vmachine.o obj/monitor.o
	ar rcs lib/libv6502.a obj/v6502.o obj/devices.o obj/addrlist.o obj/vmachine.o obj/monitor.o

# Dynamic library (requires PIC object files)
lib/libv6502.so: lib obj/v6502.pic.o obj/devices.pic.o obj/addrlist.pic.o obj/vmachine.pic.o obj/monitor.pic.o
	${CC} -shared obj/v6502.pic.o obj/devices.pic.o obj/addrlist.pic.o obj/vmachine.pic.o obj/monitor.pic.o -o lib/libv6502.so

# PIC object files for shared library
obj/v6502.pic.o: obj src/inst.h src/v6502.h src/v6502.c src/vtypes.h
	${CC} ${CCOPTS} -fPIC -c src/v6502.c -o obj/v6502.pic.o

obj/devices.pic.o: obj src/devices.h src/devices.c src/v6502.h src/vtypes.h
	${CC} ${CCOPTS} -fPIC -c src/devices.c -o obj/devices.pic.o

obj/addrlist.pic.o: obj src/addrlist.h src/addrlist.c src/vtypes.h
	${CC} ${CCOPTS} -fPIC -c src/addrlist.c -o obj/addrlist.pic.o

obj/vmachine.pic.o: obj src/vmachine.h src/vmachine.c src/v6502.h src/vtypes.h src/devices.h src/addrlist.h
	${CC} ${CCOPTS} -fPIC -c src/vmachine.c -o obj/vmachine.pic.o

obj/monitor.pic.o: obj src/monitor.h src/monitor.c src/vmachine.h src/v6502.h src/vtypes.h
	${CC} ${CCOPTS} -fPIC -c src/monitor.c -o obj/monitor.pic.o

bin/hello: bin lib/libv6502.a src/hello.c src/hello.h
	${CC} ${CCOPTS} src/hello.c lib/libv6502.a -o bin/hello

bin/cputest: bin lib/libv6502.a tests/cputest.c tests/cputest.h
	${CC} ${CCOPTS} tests/cputest.c lib/libv6502.a -o bin/cputest

bin/devtest: bin lib/libv6502.a tests/devtest.c src/devices.h
	${CC} ${CCOPTS} tests/devtest.c lib/libv6502.a -o bin/devtest

bin/addrtest: bin lib/libv6502.a tests/addrtest.c src/addrlist.h
	${CC} ${CCOPTS} tests/addrtest.c lib/libv6502.a -o bin/addrtest

bin/v6502c: bin lib/libv6502.a utils/cli.c utils/cli.h
	${CC} ${CCOPTS} utils/cli.c lib/libv6502.a -o bin/v6502c

bin/bin2woz: bin utils/bin2woz.c
	${CC} ${CCOPTS} utils/bin2woz.c -o bin/bin2woz

src/hello.h: src/hello.s
	${VASM} -Fbin -dotdir -o src/hello.bin src/hello.s
	${VASM} -Fwoz -dotdir -o src/hello.woz src/hello.s
	xxd -i src/hello.bin > src/hello.h

infloop: src/infloop.woz

src/infloop.woz: src/infloop.s
	${VASM} -Fwoz -dotdir -o src/infloop.woz src/infloop.s

rom/iotest.woz: programs/iotest.s
	${VASM} -Fwoz -dotdir -o rom/iotest.woz programs/iotest.s

bin:
	mkdir -p bin

obj:
	mkdir -p obj

lib:
	mkdir -p lib

clean:
	rm -f bin/*
	rm -f obj/*
	rm -f lib/*
	rm -f src/*.*~
	rm -f *.*~
