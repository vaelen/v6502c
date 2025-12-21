.segment "CODE"

.ifndef CONFIG_NO_INPUTBUFFER_ZP
L2420:
        dex
        bpl     INLIN2
L2423:
        jsr     CRDO
.endif

; ----------------------------------------------------------------------------
; READ A LINE, AND STRIP OFF SIGN BITS
; ----------------------------------------------------------------------------
INLIN:
        ldx     #$00
INLIN2:
        jsr     GETLN
    .ifndef CONFIG_NO_LINE_EDITING
        cmp     #$07
        beq     L2443
    .endif
        cmp     #$0D
        beq     L2453
    .ifndef CONFIG_NO_LINE_EDITING
        cmp     #$20
        bcc     INLIN2
        cmp     #$7D
    .endif
        bcs     INLIN2
        cmp     #$40 ; @
        beq     L2423
        cmp     #$5F ; _
        beq     L2420
L2443:
        cpx     #$47
        bcs     L244C
        sta     INPUTBUFFER,x
        inx
        bne     INLIN2
L244C:
    .ifndef CONFIG_NO_LINE_EDITING
        lda     #$07 ; BEL
        jsr     OUTDO
        bne     INLIN2
    .endif
L2453:
        jmp     L29B9

GETLN:
    .ifdef CONFIG_FILE
        jsr     CHRIN
        ldy     CURDVC
        bne     L2465
    .else
        jsr     MONRDKEY
    .endif
    .ifdef OSI
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        and     #$7F
    .endif
    .ifdef v6502c
        rts
    .endif
