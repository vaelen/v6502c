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
; ACIA_GET_RAW - Get character without echo (for line editor)
; Returns: A = character read
; Preserves: X
; Note: This is a blocking call - waits until a character is available
; Note: Never echoes, regardless of ACIA_NOECHO setting
; ----------------------------------------------------------------------------
ACIA_GET_RAW:
        lda     ACIA_PENDING    ; Check if we have a pending char
        beq     @read_acia      ; No, read from ACIA
        ; Return pending char and clear buffer
        pha                     ; Save the character
        lda     #$00
        sta     ACIA_PENDING    ; Clear pending
        pla                     ; Restore character
        rts
@read_acia:
        lda     ACIA_STATUS     ; Check status
        and     #ACIA_RDRF      ; Data ready?
        beq     @read_acia      ; No, keep waiting
        lda     ACIA_DATA       ; Read the character
        and     #$7F            ; Strip high bit
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
; GET_UPPER - Convert lowercase to uppercase for input
; Input: X = index into INPUTBUFFERX
; Output: A = character (uppercased if a-z)
; Preserves: X, Y
; ----------------------------------------------------------------------------
GET_UPPER:
        lda     INPUTBUFFERX,x
        cmp     #'a'
        bcc     @ret
        cmp     #'z'+1
        bcs     @ret
        sbc     #$1F            ; A-Z = a-z - 32, carry clear so -31
@ret:   rts

; ----------------------------------------------------------------------------
; Constants for line editing
; ----------------------------------------------------------------------------
CHAR_BS     := $08              ; Backspace key
CHAR_DEL    := $7F              ; Delete key
CHAR_CR     := $0D              ; Carriage return
CHAR_BEL    := $07              ; Bell
CHAR_USCORE := $5F              ; Underscore (legacy backspace)
CHAR_AT     := $40              ; @ (legacy clear line)

; ----------------------------------------------------------------------------
; GETLN_BUFFERED - Read a line with editing support
; Input: X = starting buffer index (usually 0)
; Output: A = CR, line in INPUTBUFFER, X = length
; Modifies: INPUTBUFFER
; Preserves: nothing
; ----------------------------------------------------------------------------
GETLN_BUFFERED:
        jsr     ECHO_OFF        ; Disable auto-echo during line input
@loop:
        jsr     ACIA_GET_RAW    ; Get char without echo
        cmp     #CHAR_CR
        beq     @done
        cmp     #CHAR_BS
        beq     @backspace
        cmp     #CHAR_DEL
        beq     @backspace
        cmp     #CHAR_USCORE    ; Legacy backspace (_)
        beq     @backspace_echo
        cmp     #CHAR_AT        ; Legacy clear line (@)
        beq     @clearline
        cmp     #$20            ; < space?
        bcc     @loop           ; Ignore control chars
        cmp     #$7F            ; >= DEL?
        bcs     @loop           ; Ignore
        ; Printable character
        cpx     #$47            ; Buffer full? (71 chars max)
        bcs     @bell
        jsr     COUT            ; Echo it
        sta     INPUTBUFFER,x
        inx
        bne     @loop           ; Always branches (X won't wrap to 0)
@bell:
        lda     #CHAR_BEL
        jsr     COUT
        jmp     @loop
@backspace:
        cpx     #$00            ; Buffer empty?
        beq     @loop           ; Yes, ignore
        dex
        lda     #CHAR_BS        ; Send BS
        jsr     COUT
        lda     #' '            ; Send space (erase)
        jsr     COUT
        lda     #CHAR_BS        ; Send BS again
        jsr     COUT
        jmp     @loop
@backspace_echo:
        cpx     #$00            ; Buffer empty?
        beq     @loop           ; Yes, ignore
        dex
        lda     #CHAR_USCORE    ; Echo the underscore
        jsr     COUT
        jmp     @loop
@clearline:
        lda     #CHAR_AT        ; Echo the @
        jsr     COUT
        lda     #CHAR_CR        ; New line
        jsr     COUT
        ldx     #$00            ; Reset buffer
        jmp     @loop
@done:
        jsr     COUT            ; Echo the CR
        jsr     ECHO_ON         ; Re-enable echo
        lda     #CHAR_CR
        rts

; ----------------------------------------------------------------------------
; CPU Vectors
; Placed at $FFFA-$FFFF by the linker
; ----------------------------------------------------------------------------
.segment "CPUVECTORS"

        .word   PR_WRITTEN_BY   ; $FFFA: NMI vector (use BASIC entry)
        .word   PR_WRITTEN_BY   ; $FFFC: RESET vector -> BASIC cold start
        .word   PR_WRITTEN_BY   ; $FFFE: IRQ vector (use BASIC entry)
