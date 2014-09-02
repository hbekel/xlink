.var address = $fb

.pc = $1000

.import source "server.h"

jmp main
	
read: { :read() rts }
write: { :write() rts }
ack: { :ack() rts }
wait: { :wait() rts }
	
main: {	
        lda #$00  // set CIA2 port B to input
	sta $dd03
  
	lda $dd02 // set CIA2 PA2 to output
        ora #$04
	sta $dd02

	lda $dd0d // clear stale handshake

        sei

loop:	jsr wait
	ldy $dd01
	jsr ack   

	cpy #Command.load 
	bne loop
	jsr load
	jsr saveServer

done:	cli
	rts
}

load: {
	lda #$0b
	sta $d011
	jsr readHeader
	
	ldy #$00
	
!loop:  jsr wait
	lda $dd01 
	sta (start),y
	jsr ack
	:next()

done:	lda #$1b
	sta $d011
	rts
}

readHeader: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start stx $fb
	jsr read stx start+1 stx $fc
	jsr read stx end
	jsr read stx end+1
	rts
}

saveServer: {

	lda #eot-filename
	ldx #<filename
	ldy #>filename
	jsr $ffbd
	lda #$00
	ldx $BA       
	bne skip
	ldx #$08      
skip:	ldy #$00
	jsr $FFBA     

	ldx end
	ldy end+1
	lda #$fb
	jsr $ffd8 
	rts
	
filename: .text "XLINK-SERVER"
eot:
}
