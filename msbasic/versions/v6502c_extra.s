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
; ACIA Input Buffering
;
; ACIA_PENDING (zero page) holds a "peeked" character:
;   $00 = no pending character
;   else = the pending character value
;
; ACIA_PEEK: Look at next char without consuming (for ISCNTC)
;   - If ACIA_PENDING != 0, return it
;   - Else read from ACIA, store in ACIA_PENDING, return it
;   - Returns: A = character, Z flag set if no char available
;
; ACIA_GET: Get next char, consuming it (for RDKEY)
;   - If ACIA_PENDING != 0, return it and clear ACIA_PENDING
;   - Else read from ACIA and return it
;   - Returns: A = character
; ----------------------------------------------------------------------------

; ----------------------------------------------------------------------------
; ACIA_PEEK - Peek at the next character without consuming it
; Returns: A = character (or 0 if none available)
;          Z flag set if no character available
; Preserves: X
; ----------------------------------------------------------------------------
ACIA_PEEK:
        lda     ACIA_PENDING    ; Check if we have a pending char
        bne     @done           ; Yes, return it
        ; No pending char - check ACIA
        lda     ACIA_STATUS     ; Check status
        and     #ACIA_RDRF      ; Data ready?
        beq     @none           ; No, return 0
        ; Read and buffer the character
        lda     ACIA_DATA       ; Read the character
        and     #$7F            ; Strip high bit
        sta     ACIA_PENDING    ; Store as pending
@done:
        ora     #$00            ; Set flags (clear Z if A != 0)
        rts
@none:
        lda     #$00            ; No character available
        rts

; ----------------------------------------------------------------------------
; ACIA_GET - Get the next character, consuming it
; Returns: A = character read
; Preserves: X
; Note: This is a blocking call - waits until a character is available
; Note: Echoes character to output if ACIA_NOECHO is 0
; ----------------------------------------------------------------------------
ACIA_GET:
        lda     ACIA_PENDING    ; Check if we have a pending char
        beq     @read_acia      ; No, read from ACIA
        ; Return pending char and clear buffer
        pha                     ; Save the character
        lda     #$00
        sta     ACIA_PENDING    ; Clear pending
        pla                     ; Restore character
        jmp     @do_echo        ; Check if we should echo
@read_acia:
        lda     ACIA_STATUS     ; Check status
        and     #ACIA_RDRF      ; Data ready?
        beq     @read_acia      ; No, keep waiting
        lda     ACIA_DATA       ; Read the character
        and     #$7F            ; Strip high bit
@do_echo:
        pha                     ; Save the character
        ldy     ACIA_NOECHO     ; Check echo flag
        bne     @no_echo        ; Non-zero = echo disabled
        jsr     COUT            ; Echo the character
@no_echo:
        pla                     ; Restore character
        rts

; ----------------------------------------------------------------------------
; ECHO_ON - Enable character echo
; ----------------------------------------------------------------------------
ECHO_ON:
        lda     #$00
        sta     ACIA_NOECHO
        rts

; ----------------------------------------------------------------------------
; ECHO_OFF - Disable character echo
; ----------------------------------------------------------------------------
ECHO_OFF:
        lda     #$01
        sta     ACIA_NOECHO
        rts

; ----------------------------------------------------------------------------
; RDKEY - Read a character from the ACIA (blocking)
; Returns: A = character read
; Preserves: X
; Destroys: Y (CONFIG_MONCOUT_DESTROYS_Y is set)
; ----------------------------------------------------------------------------
RDKEY:
        jsr     ACIA_GET        ; Use buffered get
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

; ----------------------------------------------------------------------------
; CPU Vectors
; Placed at $FFFA-$FFFF by the linker
; ----------------------------------------------------------------------------
.segment "CPUVECTORS"

        .word   PR_WRITTEN_BY   ; $FFFA: NMI vector (use BASIC entry)
        .word   PR_WRITTEN_BY   ; $FFFC: RESET vector -> BASIC cold start
        .word   PR_WRITTEN_BY   ; $FFFE: IRQ vector (use BASIC entry)
