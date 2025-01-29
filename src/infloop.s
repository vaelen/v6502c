	;;  An infinite loop

	.ORG $400

	LDX #$FF
	TXS

	LDX #$0
	LDA #'.'
LOOP:
	STA $FF00
	INX
	JMP LOOP

