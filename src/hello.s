	;;  A simple Hello World program.

	ORG $1000

	LDX #$FF
	TXS

	LDX #$0
LOOP	LDA MSG,X
	BEQ DONE
	STA $FF00
	INX
	JMP LOOP

DONE	BRK
	
MSG	asciiz "Hello, world!\n"
