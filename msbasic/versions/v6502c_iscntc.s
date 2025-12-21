.segment "CODE"
; ----------------------------------------------------------------------------
; SEE IF CONTROL-C TYPED
; Uses ACIA_PEEK to check if a character is available without consuming it.
; If Ctrl-C is found, it consumes the character and triggers STOP.
; Returns: Z flag set if Ctrl-C was pressed
; ----------------------------------------------------------------------------
ISCNTC:
        jsr     ACIA_PEEK       ; Peek at next char (non-blocking)
        beq     @no_key         ; No character available
        cmp     #$03            ; Is it Ctrl-C?
        beq     @ctrl_c         ; Yes, handle it
@no_key:
        lda     #$01            ; Return non-zero (Z clear)
        rts
@ctrl_c:
        ; It's Ctrl-C - consume it and fall through to STOP
        jsr     ACIA_GET        ; Consume the Ctrl-C
        nop
        nop
        cmp     #$03
;!!! runs into "STOP"
