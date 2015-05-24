.pc = $e000

.import source "server.h"

//------------------------------------------------------------------------------	
	
.pc = $fa66 // wedge into system irq
wedge: {
	jmp irq
resume: 
eof:	
}

//------------------------------------------------------------------------------
  
.pc = $f6cc
tapeIODisabledMessage: {
.text "TAPE IO DISABLE" .byte $c4 
eof:
}        

//------------------------------------------------------------------------------

.pc = $f32f
disableTapeLoad: {
  ldy #27
  jsr $f722
  rts
eof:    
}        

//------------------------------------------------------------------------------
        
.pc = $f5c8        
disableTapeSave: {
  ldy #27
  jsr $f722
  rts
eof:    
}
        
//------------------------------------------------------------------------------

.pc = $e8d0 // start of search tape header routine
irq: {
        lda $dd0d
	and #$10
	beq done

	ldy $dd01
	jsr ack
        
done:   jsr jrsirq
        jmp wedge.resume
eof:    
}        

//------------------------------------------------------------------------------
	
wait: {
loop:   lda $dd0d
	and #$10
	beq loop
	rts
eof:	
}

//------------------------------------------------------------------------------		
	
ack: {
	lda $dd00
	eor #$04
	sta $dd00
	rts
eof:
}

//------------------------------------------------------------------------------		

read: {
	jsr wait
	ldx $dd01
	jsr ack
	rts
eof:
}

//------------------------------------------------------------------------------	
	
write: {
	sta $dd01
	jsr ack
	jsr wait
	rts
eof:
}
        
//------------------------------------------------------------------------------		
        
.pc = $10000

//------------------------------------------------------------------------------		
	
.function patch(start, end) {
	
	.var offset = start - $e000
	.var count = end-start

        .return " " + toIntString(offset) + " " + toIntString(count)
}
	
//------------------------------------------------------------------------------		

.var command = "tools/make-kernal c128 kernal128.bin"

.eval command = command + patch(wedge, wedge.eof)	
.eval command = command + patch(irq, irq.eof)
.eval command = command + patch(wait, wait.eof)
.eval command = command + patch(ack, ack.eof)
.eval command = command + patch(read, read.eof)                
.eval command = command + patch(write, write.eof)
        
.eval command = command + patch(disableTapeLoad, disableTapeLoad.eof)
.eval command = command + patch(disableTapeSave, disableTapeSave.eof)            
.eval command = command + patch(tapeIODisabledMessage, tapeIODisabledMessage.eof)  

.print command
//------------------------------------------------------------------------------			
