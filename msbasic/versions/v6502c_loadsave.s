.segment "CODE"

; ----------------------------------------------------------------------------
; LOAD/SAVE Implementation for V6502C
; Uses the File I/O device at $C040
; ----------------------------------------------------------------------------

; ----------------------------------------------------------------------------
; SAVE - Save BASIC program to file
; Syntax: SAVE "filename"
; ----------------------------------------------------------------------------
SAVE:
        jsr     GET_FILENAME    ; Get filename from input
        bcs     @error          ; Error if no filename

        ; Open file for writing
        lda     #FILEIO_CMD_OPEN_W
        sta     FILEIO_STATUS
        lda     FILEIO_STATUS
        and     #FILEIO_ST_ERR
        bne     @error

        ; Save program: write bytes from TXTTAB to VARTAB-1
        lda     TXTTAB
        sta     INDEX
        lda     TXTTAB+1
        sta     INDEX+1

@save_loop:
        ; Check if we've reached VARTAB
        lda     INDEX+1
        cmp     VARTAB+1
        bcc     @save_byte
        lda     INDEX
        cmp     VARTAB
        bcs     @save_done

@save_byte:
        ldy     #$00
        lda     (INDEX),y
        sta     FILEIO_DATA
        lda     #FILEIO_CMD_WRITE
        sta     FILEIO_STATUS

        ; Check for error
        lda     FILEIO_STATUS
        and     #FILEIO_ST_ERR
        bne     @error

        ; Increment pointer
        inc     INDEX
        bne     @save_loop
        inc     INDEX+1
        jmp     @save_loop

@save_done:
        ; Close file
        lda     #FILEIO_CMD_CLOSE
        sta     FILEIO_STATUS
        rts

@error:
        ; Close file on error
        lda     #FILEIO_CMD_CLOSE
        sta     FILEIO_STATUS
        jmp     IQERR           ; File error

; ----------------------------------------------------------------------------
; LOAD - Load BASIC program from file
; Syntax: LOAD "filename"
; ----------------------------------------------------------------------------
LOAD:
        jsr     GET_FILENAME    ; Get filename from input
        bcs     @error          ; Error if no filename

        ; Open file for reading
        lda     #FILEIO_CMD_OPEN_R
        sta     FILEIO_STATUS
        lda     FILEIO_STATUS
        and     #FILEIO_ST_ERR
        bne     @error

        ; Load program starting at TXTTAB
        lda     TXTTAB
        sta     INDEX
        lda     TXTTAB+1
        sta     INDEX+1

@load_loop:
        ; Read a byte
        lda     #FILEIO_CMD_READ
        sta     FILEIO_STATUS

        ; Check for EOF
        lda     FILEIO_STATUS
        and     #FILEIO_ST_EOF
        bne     @load_done

        ; Check for error
        lda     FILEIO_STATUS
        and     #FILEIO_ST_ERR
        bne     @error

        ; Store the byte
        lda     FILEIO_DATA
        ldy     #$00
        sta     (INDEX),y

        ; Increment pointer
        inc     INDEX
        bne     @load_loop
        inc     INDEX+1
        jmp     @load_loop

@load_done:
        ; Close file
        lda     #FILEIO_CMD_CLOSE
        sta     FILEIO_STATUS

        ; Update VARTAB to point past the loaded program
        lda     INDEX
        sta     VARTAB
        lda     INDEX+1
        sta     VARTAB+1

        ; Fix up line links
        lda     TXTTAB
        ldy     TXTTAB+1
        jsr     FIX_LINKS
        jmp     RESTART

@error:
        ; Close file on error
        lda     #FILEIO_CMD_CLOSE
        sta     FILEIO_STATUS
        jmp     IQERR           ; File error

; ----------------------------------------------------------------------------
; GET_FILENAME - Parse filename from BASIC line
; Expects quoted string after command
; Returns: C=0 if success, C=1 if error
; ----------------------------------------------------------------------------
GET_FILENAME:
        jsr     CHRGET          ; Get next character
        cmp     #'"'            ; Must start with quote
        bne     @error

        ; Reset filename index
        lda     #$00
        sta     FILEIO_NAMEINDEX

@copy_loop:
        jsr     CHRGET          ; Get next character
        beq     @error          ; End of line = error
        cmp     #'"'            ; End quote?
        beq     @done
        sta     FILEIO_NAMECHAR ; Store char (auto-increments index)
        jmp     @copy_loop

@done:
        clc                     ; Success
        rts

@error:
        sec                     ; Error
        rts
