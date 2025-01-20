CC = cc
CCOPTS = -ansi -Wpedantic -Isrc

all: v6502c

v6502c: obj obj/v6502.o src/main.c src/main.h
	${CC} ${CCOPTS} obj/v6502.o src/main.c -o v6502c

obj/v6502.o: obj src/inst.h src/v6502.h src/v6502.c
	${CC} ${CCOPTS} -c src/v6502.c -o obj/v6502.o

obj:
	mkdir -p obj

clean:
	rm -f v6502c *.o
	rm -f obj/*
	rm -f src/*.*~
