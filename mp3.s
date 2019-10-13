;#resource "output.bin"
;#resource "gtmp3.cfg"
;#define CFGFILE gtmp3.cfg

.export _mp3_command
.export _cv5000, _reg5000
.export _mp3_tags, _mp3_address, _mp3_bank
.import popa, _ppu_wait_nmi


_reg5000 = $5000

.macro beep
        lda #%10011100
        sta $4000
        lda #%00010010
        sta $4003

.endmacro


;
; iNES header
;

.segment "RAWHEADER"

INES_MAPPER = 0 ; 0 = NROM
INES_MIRROR = 1 ; 0 = horizontal mirroring, 1 = vertical mirroring
INES_SRAM   = 0 ; 1 = battery backed SRAM at $6000-7FFF

.byte 'N', 'E', 'S', $1A ; ID
.byte $02 ; 16k PRG chunk count
.byte $01 ; 8k CHR chunk count
.byte INES_MIRROR | (INES_SRAM << 1) | ((INES_MAPPER & $f) << 4)
.byte (INES_MAPPER & %11110000)
.byte $0, $0, $0, $0, $0, $0, $0, $0 ; padding


.segment "ZEROPAGE"
; $00-$04 reserved for emergency interrupt (vectors are all $FF)

   temp_x: .res 1
   temp_y: .res 1
   temp_lo: .res 1
   temp_hi: .res 1
   received: .res 1
   temp_a:  .res 1
   v5000:
   _cv5000: .res 1
   count_hi: .res 1
   
   x_param: .res 1

.segment "TAGDATA"
_mp3_tags:
 .incbin "output.bin"
_mp3_address:
 .incbin "output-address.bin"
_mp3_bank:
 .incbin "output-bank.bin"
 

.segment "CODE"

;----
mp3_send:
   stx temp_x
   sty temp_y
   sta temp_a
   lda v5000
   and #$7F
   sta v5000
   sta $5000   ; start bit

   ; 186.434 cycles/bit
   ldx #33
:
   dex
   bne :-
   nop
   ldy #8
@loop:
   lsr temp_a
   ror
   and #$80
   ora v5000
   sta $5000

   ldx #32
:
   dex
   bne :-
   nop
   nop

   dey
   bne @loop

   inc temp_hi ; waste 5
   nop
   nop

   lda v5000
   ora #$80
   sta $5000   ; stop bit

   ldx #120 ;40
:
   dex
   bne :-
   ldy temp_y
   ldx temp_x
   rts

;----
   
_mp3_command:
	sta temp_y	; parameter 2
        jsr popa
        tax
        jsr popa
        ldy temp_y
        
;mp3_command:
	pha
	lda #$7E
	jsr mp3_send
	lda #$FF
	jsr mp3_send
	lda #$06
	jsr mp3_send
	pla
	jsr mp3_send
	lda #0
	jsr mp3_send
	txa
	jsr mp3_send
	tya
	jsr mp3_send
	lda #$EF
	jsr mp3_send      
                
        
	
;	ldy #1
;	jsr delay
	rts
	
.enum
	CMD_NEXT_SONG		= $01
	CMD_PREVIOUS_SONG
	CMD_TRACK_SELECT
	CMD_VOLUME_UP
	CMD_VOLUME_DOWN
	CMD_SELECT_VOLUME
	CMD_SELECT_EQ
	CMD_SELECT_LOOPED_TRACK
	CMD_SELECT_TF
	CMD_SLEEP_MODE
	CMD_WAKE_UP
	CMD_CHIP_RESET
	CMD_PLAY
	CMD_PAUSE
	CMD_SELECT_FILE	
	CMD_LOOP			= $11
	CMD_SELECT_MP3_FOLDER	= $12
	CMD_ADVERT			= $13
	CMD_STOP			= $16
	CMD_PLAY_FOLDER_LOOP
	CMD_SHUFFLE_PLAY
	CMD_SINGLE_LOOP_ENABLE
	CMD_PLAY_WITH_VOLUME	= $22
.endenum
	

;cmd_next_song:				.byte $7E, $FF, $06, $01, $00, $00, $00, $EF
;cmd_previous_song:			.byte $7E, $FF, $06, $02, $00, $00, $00, $EF
;cmd_track_select:			.byte $7E, $FF, $06, $03, $00, $00, $00, $EF	; parameters: X = track number (1..255)
;cmd_volume_up:				.byte $7E, $FF, $06, $04, $00, $00, $00, $EF
;cmd_volume_down:			.byte $7E, $FF, $06, $05, $00, $00, $00, $EF
;cmd_select_volume:			.byte $7E, $FF, $06, $06, $00, $00, $1E, $EF	; pass - parameters: X = volume (0..30)
;cmd_select_eq:				.byte $7E, $FF, $06, $07, $00, $00, $01, $EF	; parameters: reserved
;cmd_select_looped_track: 	.byte $7E, $FF, $06, $08, $00, $01, $03, $EF	; parameters: Y = folder number, X = track number
;cmd_select_tf:				.byte $7E, $FF, $06, $09, $00, $00, $02, $EF	; pass - select micro SD
;cmd_sleep_mode:			.byte $7E, $FF, $06, $0A, $00, $00, $00, $EF	; pass
;cmd_wake_up:				.byte $7E, $FF, $06, $0B, $00, $00, $00, $EF	; fail
;cmd_chip_reset:			.byte $7E, $FF, $06, $0C, $00, $00, $00, $EF	; pass - note: will cause audible pop
;cmd_play:					.byte $7E, $FF, $06, $0D, $00, $00, $00, $EF
;cmd_pause:					.byte $7E, $FF, $06, $0E, $00, $00, $00, $EF 
;cmd_select_file:			.byte $7E, $FF, $06, $0F, $00, $01, $0E, $EF	; parameters: X = folder number, Y = track number
;cmd_advert:				.byte $7E, $FF, $06, $13, $00, $01, $01, $EF	; pass - parameters: X = high byte, Y = low byte (1..3000)
;cmd_stop:					.byte $7E, $FF, $06, $16, $00, $00, $00, $EF
;cmd_play_folder_loop:		.byte $7E, $FF, $06, $17, $00, $01, $00, $EF 	; parameters: Y = folder number
;cmd_shuffle_play:			.byte $7E, $FF, $06, $18, $00, $00, $00, $EF
;cmd_single_loop_enable:	.byte $7E, $FF, $06, $19, $00, $00, $00, $EF 	; pass - parameter: Y = 0:enable 1:disble
;cmd_play_with_volume:		.byte $7E, $FF, $06, $22, $00, $1E, $01, $EF	; parameters: Y = volume, X = track number


;---- Y second delay
delay:
;   lda v5000
;   ora #$40
;   sta $5000

   ldx #60
:
   lda $2002
   bpl :-
   dex
   bne :-
   ldx #60
   dey
   bne :-

;   lda v5000
;   and #%10111111
;   sta $5000
;   sta v5000
   rts
;----

