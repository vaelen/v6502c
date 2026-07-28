	;;  A simple Hello World program.

CHROUT	= $FF00			; write a character to the console
HALT	= $FF01			; write anything here to stop the CPU

	.ORG $1000

	LDX #$FF
	TXS

	LDX #$0
LOOP:	LDA MSG,X
	BEQ DONE
	STA CHROUT
	INX
	JMP LOOP

DONE:	LDA #$0A
	STA CHROUT
	STA HALT

MSG:	.asciiz "Hello, world!"
