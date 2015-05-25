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

!next:	cpy #Command.identify
	bne !next+
	jmp identify
        
!next:        
done:   jsr jrsirq
        jmp wedge.resume
eof:    
}        

//------------------------------------------------------------------------------

identify: {
        :output()
  
        lda Server.size
        jsr write

        lda Server.id
        jsr write

        lda Server.id+1
        jsr write

        lda Server.id+2
        jsr write

        lda Server.id+3
        jsr write

        lda Server.id+4
        jsr write        
        
        lda Server.version
        jsr write

        lda Server.machine
        jsr write

        lda Server.type
        jsr write       

        lda Server.start
        jsr write

        lda Server.start+1
        jsr write

        lda Server.end
        jsr write

        lda Server.end+1
        jsr write        

	lda memtop
	jsr write

	lda memtop+1
	jsr write
	
done:   :input()
        jmp irq.done
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

Server: {
size:    .byte $05
id:      .byte 'X', 'L', 'I', 'N', 'K'
start:   .word irq
version: .byte $10
type:    .byte $01 // 0 = RAM, 1 = ROM
machine: .byte $01 // 0 = C64, 1 = C128
end:     .word *+2
eof:   
}
        
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
.eval command = command + patch(identify, identify.eof)
.eval command = command + patch(Server, Server.eof)                
        
.eval command = command + patch(disableTapeLoad, disableTapeLoad.eof)
.eval command = command + patch(disableTapeSave, disableTapeSave.eof)            
.eval command = command + patch(tapeIODisabledMessage, tapeIODisabledMessage.eof)  

.print command
//------------------------------------------------------------------------------			
