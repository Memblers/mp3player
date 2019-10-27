.export _oam_bar_init,_oam_bar_run
.import _selector_y_position, _temp, _spr_id

.segment "ZEROPAGE"
testval: .res 1

.segment "DATA"
oam_x_pos_lo:	.res $100

.segment "RODATA"
sine:
.byte 128,131,134,137,140,143,146,149,152,156,159,162,165,168,171,174,176,179,182,185,188,191,193,196,199,201,204,206,209,211,213,216
.byte 218,220,222,224,226,228,230,232,234,235,237,239,240,242,243,244,246,247,248,249,250,251,251,252,253,253,254,254,254,255,255,255
.byte 255,255,255,255,254,254,253,253,252,252,251,250,249,248,247,246,245,244,242,241,239,238,236,235,233,231,229,227,225,223,221,219
.byte 217,215,212,210,207,205,202,200,197,195,192,189,186,184,181,178,175,172,169,166,163,160,157,154,151,148,145,142,138,135,132,129
.byte 126,123,120,117,113,110,107,104,101,98,95,92,89,86,83,80,77,74,71,69,66,63,60,58,55,53,50,48,45,43,40,38
.byte 36,34,32,30,28,26,24,22,20,19,17,16,14,13,11,10,9,8,7,6,5,4,3,3,2,2,1,1,0,0,0,0
.byte 0,0,0,1,1,1,2,2,3,4,4,5,6,7,8,9,11,12,13,15,16,18,20,21,23,25,27,29,31,33,35,37
.byte 39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,79,81,84,87,90,93,96,99,103,106,109,112,115,118,121,124,128

.segment "CODE"
_oam_bar_init:

	lda _selector_y_position
        sec
        sbc #4
        tay
        ldx #0        
        :
        tya
        sta $200,x
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
        tya
        clc
        adc #8
        tay
        :
        tya
        sta $200,x
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