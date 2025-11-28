.segment "CODE"
; ----------------------------------------------------------------------------
; SEE IF CONTROL-C TYPED
; Check the ACIA status register to see if a character is available.
; If so, check if it's Ctrl-C ($03).
; Returns: Z flag set if Ctrl-C was pressed
; ----------------------------------------------------------------------------
ISCNTC:
        lda     ACIA_STATUS     ; Check ACIA status
        and     #ACIA_RDRF      ; Is there a character?
        beq     @no_key         ; No, return with Z clear
        lda     ACIA_DATA       ; Read the character
        cmp     #$03            ; Is it Ctrl-C?
        beq     @ctrl_c         ; Yes, handle it
@no_key:
        lda     #$01            ; Return non-zero (Z clear)
        rts
@ctrl_c:
        ; Fall through to STOP handler
        nop
        nop
        cmp     #$03
;!!! runs into "STOP"
