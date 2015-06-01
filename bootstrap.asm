.import source "server.h"

.if(target == "c64") {
  .pc = $3000
}

.if(target == "c128") {
  .pc = $0d00
}	
	
//------------------------------------------------------------------------------	
	
jmp main

//------------------------------------------------------------------------------		
	
read: { :read() rts }
write: { :write() rts }
ack: { :ack() rts }
wait: { :wait() rts }

//------------------------------------------------------------------------------		
	
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
        
done:	cli
        jmp booted
}

//------------------------------------------------------------------------------	
	
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

done:   lda end
        sta bend
        lda end+1
        sta bend+1

        lda #$1b
	sta $d011
	rts
}

//------------------------------------------------------------------------------	
	
readHeader: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start 
	jsr read stx start+1
	jsr read stx end
	jsr read stx end+1
	rts
}

//------------------------------------------------------------------------------
