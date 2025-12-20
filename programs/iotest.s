; iotest.s - Serial port echo program
; Polls the primary ACIA at $C010 and echoes received characters back

        .org $D000

; ACIA registers (primary at $C010-$C01F)
ACIA_DATA   = $C010     ; Data register (read/write)
ACIA_STATUS = $C011     ; Status register (read-only)

; Status register bits
PE          = $01       ; Bit 0: Parity Error
FE          = $02       ; Bit 1: Framing Error
OVR         = $04       ; Bit 2: Overrun
RDRF        = $08       ; Bit 3: Receive Data Register Full
TDRE        = $10       ; Bit 4: Transmit Data Register Empty
DCD         = $20       ; Bit 5: Data Carrier Detect
DSR         = $40       ; Bit 6: Data Set Ready
IRQ         = $80       ; Bit 7: Interrupt Request

; Reset vector
        .org $FFFE
        .word START     ; Reset vector points to START

        .org $D000
START:
        ; Main loop - poll for received data
POLL:
        LDA ACIA_STATUS ; Read status register
        AND #RDRF       ; Check if data received (bit 3)
        BEQ POLL        ; No data, keep polling

        ; Data received - read it
        LDA ACIA_DATA   ; Read received byte
        PHA             ; Save the received byte

        ; Print "0x"
        ; LDA #'0'
        ; JSR SEND_CHAR
        ; LDA #'x'
        ; JSR SEND_CHAR

        ; Print hex value of received byte
        ; PLA             ; Restore received byte
        ; PHA             ; Save it again
        ; JSR PRINT_HEX

        ; Print space
        ; LDA #' '
        ; JSR SEND_CHAR

        ; Print the character itself
        PLA             ; Restore received byte
        JSR SEND_CHAR

        ; Print space
        ; LDA #' '
        ; JSR SEND_CHAR

        JMP POLL        ; Continue polling

; Subroutine: Send character in A to serial port
; Waits for transmitter ready, then sends
SEND_CHAR:
        PHA             ; Save the byte to send
WAIT_TX:
        LDA ACIA_STATUS ; Read status
        AND #TDRE       ; Check if transmit register empty (bit 4)
        BEQ WAIT_TX     ; Not ready, keep waiting
        PLA             ; Restore the byte
        STA ACIA_DATA   ; Send it
        RTS

; Subroutine: Print byte in A as two hex digits
PRINT_HEX:
        PHA             ; Save the byte
        LSR A           ; Shift high nibble to low
        LSR A
        LSR A
        LSR A
        JSR PRINT_NIBBLE
        PLA             ; Restore byte
        AND #$0F        ; Mask low nibble
        JSR PRINT_NIBBLE
        RTS

; Subroutine: Print low nibble of A as hex digit
PRINT_NIBBLE:
        CMP #$0A        ; Is it >= 10?
        BCS LETTER      ; Yes, print A-F
        CLC
        ADC #'0'        ; Convert 0-9 to ASCII
        JMP SEND_CHAR
LETTER:
        CLC
        ADC #'A'-10     ; Convert 10-15 to A-F
        JMP SEND_CHAR
