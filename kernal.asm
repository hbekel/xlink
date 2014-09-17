.pc = $e000

.import source "server.h"

//------------------------------------------------------------------------------	
	
.pc = $ea31
wedge: {
	jmp irq
eof:	
}

//------------------------------------------------------------------------------	
	
.pc = $f0d9
tapeLoadDisabledMessage: {
.text "TAPE LOAD DISABLE" .byte $c4 
eof:
}

//------------------------------------------------------------------------------	
	
.pc = $e479
powerUpMessage:	{
.text "**** COMMODORE 64 BASIC V2 ****"
eof:
}

//------------------------------------------------------------------------------		
	
.pc = $fd6c 
fastMemoryCheck: { // fast memory check unless cmb key is pressed

	lda #%01111111 	// check for cbm key
	sta $dc00
	lda $dc01
	and #%00100000
	bne fast
	jmp memoryCheck
	
fast:	jsr $fd02 // check for cartridge
	beq cart
nocart:
	ldx #$00
	ldy #$a0  // assume memory top at $a000
	jmp done
	
cart:
	ldy #$80 // assume memory top at $8000
	
done:	jmp $fd8c 
eof:	
}

//------------------------------------------------------------------------------		
	
.pc = $f541 // begin of modified area in kernal "Load Tape" routine
disableTapeLoad: {
ldy #$1b
jsr $f12f
rts
eof:
}

//------------------------------------------------------------------------------	
	
irq: {
	lda $dd0d
	and #$10
	beq done

	ldy $dd01
	jsr ack

!next:	cpy #Command.load  // dispatch command...
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

!next:	cpy #Command.extend
	bne !next+
	jmp extend

!next:	cpy #Command.identify
	bne !next+
	jmp identify
        
!next:
done:	jsr $ffea
	jmp $ea34
eof:
}

//------------------------------------------------------------------------------		

.pc = $f5ab // end of kernal "Load Tape" routine

//------------------------------------------------------------------------------		

.pc = $f82e // begin of kernal "Check Tape Status" routine
	

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
	
ram: {

install:
	ldy #codeend-codestart-1
copy:	lda codestart,y
	sta $0100,y
	dey
	bpl copy
	rts
	
codestart:	
.pseudopc $0100 {
read:	lda mem
	sta $01
	lda (start),y
	ldx #$37
	stx $01
	rts
}
codeend:
	
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
	
load: {
	jsr readHeader
	:screenOff()
	
	:checkBasic()

	ldy #$00
	
	lda mem // check if specific memory config was requested
	and #$7f
	cmp #$37
	bne slow
	
fast:	
!loop:  :wait()
	lda $dd01 
	sta (start),y   // write with normal memory config
	:ack()
	:next()
	jmp done

slow:	
!loop:  :wait()
	ldx #$33        // write to ram with io disabled
	stx $01
	sta (start),y
	lda #$37
	sta $01
	:ack()
	:next()

done:	:relinkBasic()
	:screenOn()
	jmp irq.done
eof:	
}

//------------------------------------------------------------------------------		

save: {
	jsr readHeader
	:screenOff()
	
	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03

	ldy #$00      // prepare reading
	
	lda mem       // check memory config...
	and #$02
	cmp #$02
	bne slow      // have to read from ram unless highram is 1

	lda mem       // if mem is $37, we can simply read fastest
	cmp #$37
	bne fast
	
fastest:
!loop:  lda (start),y
	:write()
	:next()
	jmp done

fast:
!loop:	ldx mem       // need to change memory, but kernal is still here
	stx $01
	lda (start),y
	ldx #$37
	stx $01
	:write()
	:next()

	jmp done	

slow:	jsr ram.install
!loop:  jsr ram.read // need to read from ram since kernal will be gone
	:write()
	:next()
	
done:	lda #$00       // reset CIA2 port B to input
	sta $dd03
	
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

	ldy #$00
	
	lda mem
	cmp #$37
	bne slow

fast:   jsr read txa
	sta (start),y
	jmp done
	
slow:   jsr read txa
	ldx #$33
	stx $01
	sta (start),y
	lda #$37
	sta $01

done:	jmp irq.done
eof:	
}

//------------------------------------------------------------------------------	
	
peek: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03

	jsr ram.install
	
	ldy #$00
	jsr ram.read   
	jsr write

done:	lda #$00   // reset CIA2 port B to input
	sta $dd03
	
	jmp irq.done
eof:	
}


//------------------------------------------------------------------------------
	
jump: {
	jsr read stx mem
	jsr read stx bank

	ldx #$ff txs // reset stack pointer
	
	jsr read txa pha // push high byte of jump address
	jsr read txa pha // push low byte of jump address

	lda mem  // apply requested memory config
	ora #$02 // but keep highram on
	sta $01
	
	lda #$00 tax tay pha // clear registers & push clean flags 
	
	rti // jump via rti
eof:	
}

//------------------------------------------------------------------------------		

run: {
	ldx #$ff txs     // reset stack pointer
	lda #$01 sta $cc // cursor off

	jsr insnewl     // run BASIC program
	jsr restxtpt
	jmp warmst
eof:
}

//------------------------------------------------------------------------------	
	
extend:	{
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
	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03

        lda #$10       // version 1.0
        :write()

        lda #$00       // C64
        :write()

        lda #$01       // ROM based server
        :write()       

        lda #<irq
        :write()

        lda #>irq
        :write()

        lda #<eos
        :write()

        lda #>eos
        :write()        
        
done:   lda #$00   // reset CIA2 port B to input
	sta $dd03
        
        jmp irq.done
eof:    
}

//------------------------------------------------------------------------------	                
memoryCheck: { // relocated original memory check routine 

high:	inc $c2
low:	lda ($c1),y
	tax
	lda #$55
	sta ($c1),y
	cmp ($c1),y
	bne done
	rol
	sta ($c1),y
	cmp ($c1),y
	bne done
	txa
	sta ($c1),y
	iny
	bne low
	beq high
	
done:  	tya
	tax
	ldy $c2
	clc
	jsr $fe2d
	lda #$08
	sta $0282
	lda #$04
	sta $0288
	rts
eof:	
}

eos:   
        
//------------------------------------------------------------------------------	
	
.pc = $fc92 // end of kernal "Write Tape Leader" routine

//------------------------------------------------------------------------------		
	
.pc = $10000

//------------------------------------------------------------------------------		

.function patch(start, end) {
	
	.var offset = start - $e000
	.var count = end-start

        .return "dd conv=notrunc if=kernal.bin of=xlink-kernal.rom bs=1 " +
	        "skip=" + toIntString(offset) + " " +
	        "seek=" + toIntString(offset) + " " +
	        "count=" + toIntString(count)
}

//------------------------------------------------------------------------------		

.print patch(wedge, wedge.eof)	
.print patch(tapeLoadDisabledMessage, tapeLoadDisabledMessage.eof)
.print patch(powerUpMessage, powerUpMessage.eof)
.print patch(disableTapeLoad, disableTapeLoad.eof)
.print patch(fastMemoryCheck, fastMemoryCheck.eof)
.print patch(irq, irq.eof)
.print patch(ack, ack.eof)
.print patch(wait, wait.eof)
.print patch(read, read.eof)
.print patch(write, write.eof)
.print patch(ram, ram.eof)
.print patch(readHeader, readHeader.eof)
.print patch(load, load.eof)
.print patch(save, save.eof)
.print patch(poke, poke.eof)
.print patch(peek, peek.eof)	
.print patch(jump, jump.eof)
.print patch(run, run.eof)
.print patch(extend, extend.eof)
.print patch(identify, identify.eof)
.print patch(memoryCheck, memoryCheck.eof)

//------------------------------------------------------------------------------			
.print "free: 0x" + toHexString(irq.eof) + "-0xf5ab" + ": " + toIntString($f5ab-irq.eof) + " bytes"
.print "free: 0x" + toHexString(memoryCheck.eof) + "-0xfc92" + " " + toIntString($fc92-memoryCheck.eof) + " bytes"