.pc = $e000

.import source "server.h"

//------------------------------------------------------------------------------	
	
.pc = $fa66 // wedge into system irq
irqwedge: {
	jmp irq
resume: 
eof:	
}

//------------------------------------------------------------------------------	
	
.pc = $ff56 // wedge into system boot routine
bootwedge: {
	jmp boot
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

.pc = $f326
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

!next:	cpy #Command.load 
	bne !next+
	jmp load

!next:	cpy #Command.save
	bne !next+
	jmp save

!next:	cpy #Command.peek
	bne !next+
	jmp peek
	
!next:	cpy #Command.poke
	bne !next+
	jmp poke

!next:	cpy #Command.jump
	bne !next+
	jmp jump

!next:	cpy #Command.run
	bne !next+
	jmp run

!next:	cpy #Command.inject
	bne !next+
	jmp inject	
	
!next:	cpy #Command.identify
	bne !next+
	jmp identify
        
!next:        
done:   jsr jrsirq
        jmp irqwedge.resume
eof:    
}        

//------------------------------------------------------------------------------
	
load: {
	jsr readHeader
	:screenOff()	
	:checkBasic()
	
	:checkBank()
	
near:	ldy #$00	
!loop:  :wait()
	lda $dd01
	sta (start),y 
	:ack()
	:next()
	jmp done

far:    :checkIO()
	
slow:   :jsrcommon(code.slow_receivefar)
	jmp done

fast:   :jsrcommon(code.fast_receivefar)
	
done:   :screenOn()
	:relinkBasic()
	jmp irq.done
eof:	
}

//------------------------------------------------------------------------------

save: {
	jsr readHeader
	:screenOff()
	
        :output()

	:checkBank()
	
near:	ldy #$00
!loop:  lda (start),y  
	:write()
	:next()
	jmp done

far:    :checkIO()
	
slow:	:jsrcommon(code.slow_sendfar)
	jmp done

fast:	:jsrcommon(code.fast_sendfar)
	
done:	:input()
	:screenOn()
	jmp irq.done
eof:	
}	
	
//------------------------------------------------------------------------------
	
poke: {
	jsr read stx mem 
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

	:wait()
	lda $dd01 pha
	:ack()
	
	:checkBank()
	
near:  ldy #$00
	pla sta (start),y
	jmp done

far:    ldy #$00
	lda #start
	sta stashptr

	ldx mem
	pla jsr stash
	
done:   jmp irq.done
eof:	
}

//------------------------------------------------------------------------------
	
peek: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

        :output()
	
	:checkBank()
	
near:	ldy #$00
	lda (start),y
	:write()
	jmp done

far:	lda #start
	sta fetchptr

	ldx mem
	jsr fetch
	
	:write()

done:   :input()
	
	jmp irq.done
eof:	
}
	
//------------------------------------------------------------------------------

jump: {
 	jsr read stx mem
	jsr read stx bank

	ldx #$ff txs        // reset stack pointer

        lda #>repl     pha  // make sure the code jumped to can rts to basic
        lda #[<repl-1] pha  // (only if the bank includes basic rom, of course) 

	// prepare jmpfar...
	
	lda bank sta $02    // setup requested bank value

	jsr read stx $03    // setup jump address (sent msb first by client)
	jsr read stx $04

	// set clean registers and flags...
	
	lda #$00 sta $05 sta $06 sta $07 sta $08 

	jmp jmpfar          // implies rti
eof:	
}
	
//------------------------------------------------------------------------------
	
run: {
	lda #$01 sta cursor // cursor off

	cli jmp basrun      // perform RUN
eof:	
}
	
//------------------------------------------------------------------------------

inject:	{
	lda #>return pha
	lda #<return pha
	
	jsr read txa pha 
	jsr read txa pha
	
	rts
	
return: nop
	jmp irq.done
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

code: {
	
slow_receivefar: {
.pseudopc common {
	lda mmu sta saved

	ldy #$00	
!loop:  :wait()
	lda $dd01
	ldx mem stx mmu
	sta (start),y 
	ldx saved stx mmu
	:ack()
	:next()

	rts
}
eof:
}

fast_receivefar: {
.pseudopc common {
	lda mmu sta saved
	lda mem sta mmu
	
	ldy #$00	
!loop:  :wait()
	lda $dd01
	sta (start),y 
	:ack()
	:next()

	lda saved sta mmu
	
	rts
}
eof:
}
	
slow_sendfar: {
.pseudopc common {
	lda mmu
	sta saved

	ldy #$00
!loop:  ldx mem stx mmu
	lda (start),y
	ldx saved stx mmu
	:write()
	:next()
	
	rts
}
eof:	
}

fast_sendfar: {
.pseudopc common {
	lda mmu sta saved
	lda mem sta mmu
	
	ldy #$00
!loop:  lda (start),y	
	:write()
	:next()
	
	lda saved sta mmu
	rts
}
eof:	
}
	
eof:	
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

boot: {
	ldx #12     // center boot message...
	            // 12 spaces in 40 column mode

	bit $d7     // check for 80 columns
	bpl spaces  // no...
	
	ldx #32     // 32 spaces in 80 column mode

spaces: lda #$20
	
!loop:  jsr $ffd2
	dex
	bne !loop-
	             
message:            // print message...
	ldx #$00
!loop:	lda text,x
	beq check
	jsr $ffd2
	inx
	jmp !loop-
	
check:	lda #%01111111 // check for Control key...
	sta $dc00
	lda $dc01
	and #%00000100
	cmp #%00000100
	beq skip       // not pressed -> skip boot

	jmp $f867      // boot

skip:	rts

text: .text "XLINK KERNAL V1.0" .byte $0d, $00
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

.eval command = command + patch(irqwedge, irqwedge.eof)
.eval command = command + patch(bootwedge, bootwedge.eof)		
.eval command = command + patch(irq, irq.eof)
.eval command = command + patch(load, load.eof)
.eval command = command + patch(save, save.eof)
.eval command = command + patch(peek, peek.eof)
.eval command = command + patch(poke, poke.eof)
.eval command = command + patch(jump, jump.eof)
.eval command = command + patch(run, run.eof)
.eval command = command + patch(inject, inject.eof)
.eval command = command + patch(code, code.eof)
.eval command = command + patch(readHeader, readHeader.eof)	
.eval command = command + patch(wait, wait.eof)
.eval command = command + patch(ack, ack.eof)
.eval command = command + patch(read, read.eof)                
.eval command = command + patch(write, write.eof)
.eval command = command + patch(identify, identify.eof)
.eval command = command + patch(boot, boot.eof)	
.eval command = command + patch(Server, Server.eof)                
        
.eval command = command + patch(disableTapeLoad, disableTapeLoad.eof)
.eval command = command + patch(disableTapeSave, disableTapeSave.eof)            
.eval command = command + patch(tapeIODisabledMessage, tapeIODisabledMessage.eof)  

.print command
//------------------------------------------------------------------------------			
