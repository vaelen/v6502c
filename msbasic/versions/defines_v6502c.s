; V6502C Emulator configuration
; Based on CONFIG_2C (MICROTAN) feature set

CONFIG_2C := 1

CONFIG_NULL := 1
CONFIG_MONCOUT_DESTROYS_Y := 1
CONFIG_PEEK_SAVE_LINNUM := 1
CONFIG_PRINT_CR := 1 ; print CR when line end reached
CONFIG_SCRTCH_ORDER := 1

; zero page
ZP_START1 = $17
ZP_START2 = $2F
ZP_START3 = $24
ZP_START4 = $85

;extra ZP variables
USR             := $0021
TXPSV           := $00BA

; constants
STACK_TOP       := $FE
SPACE_FOR_GOSUB := $3E
NULL_MAX        := $F0
WIDTH           := 80
WIDTH2          := 56
CONFIG_FIXED_MEMSIZE := $C000   ; 49152 bytes (48K) - skip memory probe

; memory layout - RAM starts at $0400
RAMSTART2 := $0400

; ACIA input buffering - zero page location for pending character
; Used by ACIA_PEEK/ACIA_GET to allow peeking at input without consuming
ACIA_PENDING := $16

; ACIA echo control - zero page flag for character echo
; $00 = echo enabled (default), non-zero = echo disabled
ACIA_NOECHO := $15

; I/O addresses for 6551 ACIA at $C010
ACIA_DATA    := $C010
ACIA_STATUS  := $C011
ACIA_COMMAND := $C012
ACIA_CONTROL := $C013

; Status register bits
ACIA_RDRF    := $08    ; Receive Data Register Full
ACIA_TDRE    := $10    ; Transmit Data Register Empty

; File I/O device at $C040
FILEIO_STATUS    := $C040
FILEIO_DATA      := $C041
FILEIO_NAMEINDEX := $C042
FILEIO_NAMECHAR  := $C043

; File I/O commands
FILEIO_CMD_RESET   := $00
FILEIO_CMD_OPEN_R  := $01
FILEIO_CMD_OPEN_W  := $02
FILEIO_CMD_READ    := $03
FILEIO_CMD_WRITE   := $04
FILEIO_CMD_CLOSE   := $05

; File I/O status bits
FILEIO_ST_OPEN  := $01
FILEIO_ST_EOF   := $02
FILEIO_ST_ERR   := $04
FILEIO_ST_READY := $80

; Monitor I/O functions - implemented by v6502c_extra.s
; NOTE: RDKEY and COUT are defined in v6502c_extra.s
; The MONRDKEY/MONCOUT aliases are defined there as well
