.export _oam_bar_init,_oam_bar_run
.export _vis_stars_init, _vis_stars_run
.export _vis_sine_init, _vis_sine_run
.export _sine
.import _selector_y_position, _temp, _spr_id, _rand8, _str_buffer

.segment "ZEROPAGE"
testval: .res 1

.segment "DATA"
oam_x_pos_lo:	.res $100
star_speeds:	.res $100
; overlay
cosine = star_speeds
rate_lo = oam_x_pos_lo
rate2_lo = oam_x_pos_lo + 1
table_position = oam_x_pos_lo + 2
effect_rate = oam_x_pos_lo +3

.segment "RODATA"
sine:
_sine:
.byte 128,131,134,137,140,143,146,149,152,156,159,162,165,168,171,174,176,179,182,185,188,191,193,196,199,201,204,206,209,211,213,216
.byte 218,220,222,224,226,228,230,232,234,235,237,239,240,242,243,244,246,247,248,249,250,251,251,252,253,253,254,254,254,255,255,255
.byte 255,255,255,255,254,254,253,253,252,252,251,250,249,248,247,246,245,244,242,241,239,238,236,235,233,231,229,227,225,223,221,219
.byte 217,215,212,210,207,205,202,200,197,195,192,189,186,184,181,178,175,172,169,166,163,160,157,154,151,148,145,142,138,135,132,129
.byte 126,123,120,117,113,110,107,104,101,98,95,92,89,86,83,80,77,74,71,69,66,63,60,58,55,53,50,48,45,43,40,38
.byte 36,34,32,30,28,26,24,22,20,19,17,16,14,13,11,10,9,8,7,6,5,4,3,3,2,2,1,1,0,0,0,0
.byte 0,0,0,1,1,1,2,2,3,4,4,5,6,7,8,9,11,12,13,15,16,18,20,21,23,25,27,29,31,33,35,37
.byte 39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,79,81,84,87,90,93,96,99,103,106,109,112,115,118,121,124,128

.segment "CODE"

_vis_stars_init:
	ldy #0
        :
        lda #$0D
        sta $201,y
        lda #$20
        sta $202,y
        :
	jsr _rand8
        cmp #16
        bcc :-
        cmp #224
        bcs :-
        sta $200,y
        jsr _rand8
        sta $203,y
        iny
        iny
        iny
        iny
        bne :--

        :
        jsr _rand8
        cmp #0
        beq :-
        lsr
        sta star_speeds,y
        iny
        bne :-
        rts
        
_vis_stars_run:
	ldx #0
        ldy #0
        :
        lda oam_x_pos_lo,x
        clc
        adc star_speeds,x
        sta oam_x_pos_lo,x
        lda #0
        adc $203,y
        sta $203,y
        inx
        iny
        iny
        iny
        iny
        bne :-
        sty _spr_id
        rts
        
_vis_sine_init:
	ldy #0        
        :
        lda #'.'
        sta $201,y
        lda #$22
        sta $202,y
        iny
        iny
        iny
        iny
        bne :-
	
	ldy #0
        ldx #0
        :
        lda _str_buffer,x ;#'.'
        beq @end
        inx
        sta $201,y
        iny
        iny
        iny
        iny
        bne :-
      @end:
        
        ldy #$40
        :
        lda sine,y
        sta cosine - $40,y
        iny
        bne :-
        :
        lda sine,y
        sta cosine + $C0,y
        iny
        cpy #$40
        bne :-
        
        jsr _rand8
        and #3
        tax
        inx
        stx effect_rate        
        rts      
        
SPEED = $00c0
SPEED2 = $0010

_vis_sine_run:
	ldy #0        
        ldx table_position
        :
        dey
        dey
        dey
        dey
        lda sine,x
        sec
        sbc table_position
        clc
        adc rate2_lo
        sta $200,y
        lda cosine,x
        sec
        sbc table_position
        sta $203,y
        txa
        clc
        adc effect_rate
        tax
        cpy #0
        bne :-
        lda rate_lo
        clc
        adc #<SPEED
        sta rate_lo
        lda table_position
        adc #>SPEED
        sta table_position
;	inc table_position
	
        inc rate2_lo

	rts
        
        


_oam_bar_init:

        ldx #0        
        :
	lda #$0F
        sta $201,x
        lda #$20
        sta $202,x
        inx
        inx
        inx
        inx
        cpx #(4 * 8)
        bne :-
        :
	lda #$0E
        sta $201,x
        lda #$20
        sta $202,x
        inx
        inx
        inx
        inx
        cpx #(4 * 16)
        bne :-
        
        rts
        
        
_oam_bar_run:
	ldx #0
        :
        lda _selector_y_position
        sec
        sbc #4
        sta $200,x
        inc testval
        lda $203,x
        clc
        ldy testval
        adc sine,y
        sta $203,x
        inx
        inx
        inx
        inx
        cpx #4 * 8
        bne :-
	:
        lda _selector_y_position
        clc
        adc #4
        sta $200,x
        inc testval
        lda $203,x
        clc
        ldy testval
        adc sine,y
        sta $203,x
        inx
        inx
        inx
        inx
        cpx #4 * 16
        bne :-        
        stx _spr_id        
	rts