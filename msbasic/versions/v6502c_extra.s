.segment "EXTRA"

; ----------------------------------------------------------------------------
; V6502C Monitor Routines
; Uses 6551 ACIA at $C010 for serial I/O
; ----------------------------------------------------------------------------

; Define the MONRDKEY/MONCOUT symbols that other code references
MONRDKEY  := RDKEY
MONRDKEY2 := RDKEY
MONCOUT   := COUT

; ----------------------------------------------------------------------------
; RDKEY - Read a character from the ACIA (blocking)
; Returns: A = character read
; Preserves: X
; Destroys: Y (CONFIG_MONCOUT_DESTROYS_Y is set)
; ----------------------------------------------------------------------------
RDKEY:
        lda     ACIA_STATUS     ; Check status
        and     #ACIA_RDRF      ; Data ready?
        beq     RDKEY           ; No, keep waiting
        lda     ACIA_DATA       ; Read the character
        and     #$7F            ; Strip high bit
        rts

; ----------------------------------------------------------------------------
; COUT - Output a character to the ACIA
; Input: A = character to output
; Preserves: A, X
; Destroys: Y (CONFIG_MONCOUT_DESTROYS_Y is set)
; ----------------------------------------------------------------------------
COUT:
        pha                     ; Save character
@wait:
        lda     ACIA_STATUS     ; Check status
        and     #ACIA_TDRE      ; Transmit buffer empty?
        beq     @wait           ; No, keep waiting
        pla                     ; Restore character
        sta     ACIA_DATA       ; Send it
        rts
