.pc = $e000

.var start  = $c1    // Transfer start address
.var end    = $c3    // Transfer end address
.var bstart = $2b    // Start of Basic program text
.var bend   = $2d    // End of Basic program text
	
.var mem   = $fe    // Memory config
.var bank  = $ff    // bank config

.var relink   = $a533 // Relink Basic program
.var insnewl  = $a659 // Insert new line into BASIC program
.var restxtpt = $a68e // Reset BASIC text pointer
.var warmst   = $a7ae // Basic warm start (e.g. RUN)
	
.namespace Command {
.label load  = $01
.label save  = $02
.label poke  = $03
.label peek  = $04
.label jump  = $05
.label run   = $06
}

.macro wait() { 
loop:	lda $dd0d
	and #$10
	beq loop
}

.macro ack() { 
	lda $dd00
	eor #$04
	sta $dd00
}

.macro read() {
	:wait() ldx $dd01 :ack()
}

.macro write() {
	sta $dd01 :ack() :wait()
}

.macro next() {
	inc start
	bne check
	inc start+1

check:	lda start+1
	cmp end+1
	bne !loop-

	lda start
	cmp end
	bne !loop-
}

.macro checkBasic() {
	lda start
	cmp bstart
	bne no
	lda start+1
	cmp bstart+1
	bne no

yes:	lda #$00
	jmp push

no:     lda #$01

push:	pha
}

.macro relinkBasic() {
	pla             // recall result of checkBasic             
	bne done        // not a Basic program, done

	lda end         // else adjust basic end address and relink
	sta bend
	lda end+1
	sta bend+1
	jsr relink
done:	
}
	
.macro screenOff() {
	lda #$0b
	sta $d011
}

.macro screenOn() {
	lda #$1b
	sta $d011
}

.pc = $ea31
wedge: {
	jmp irq
eof:	
}
	
.pc = $f0d9
tapeLoadDisabledMessage: {
.text "TAPE LOAD DISABLE" .byte $c4
eof:
}

.pc = $e479
powerUpMessage:	{
.text "**** COMMODORE 64 BASIC V2 ****"
eof:
}
	
.pc = $f541 // begin of modified area in kernal "Load Tape" routine

disableTapeLoad: {
ldy #$1b
jsr $f12f
rts
eof:
}
	
irq: {
	lda $dd0d
	and #$10
	beq done

	ldy $dd01
	jsr ack

!next:	cpy #Command.load  // dispatch command
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
	bne done
	jmp run
	
done:	jsr $ffea
	jmp $ea34
eof:
}
	
wait: {
loop:   lda $dd0d
	and #$10
	beq loop
	rts
eof:	
}
	
ack: {
	lda $dd00
	eor #$04
	sta $dd00
	rts
eof:
}

read: {
	jsr wait
	ldx $dd01
	jsr ack
	rts
eof:
}

write: {
	sta $dd01
	jsr ack
	jsr wait
	rts
eof:
}
	
.pc = $f5ab // end of kernal "Load Tape" routine

.pc = $f92c // begin of kernal "Read Tape Bits" routine

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

load: {
	:screenOff()
	jsr readHeader
	:checkBasic()

	ldy #$00
	
	lda mem         // check if specific memory config was requested
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

save: {
	:screenOff()
	jsr readHeader

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

run: {
	ldx #$ff txs     // reset stack pointer
	lda #$01 sta $cc // cursor off

	jsr insnewl     // run BASIC program
	jsr restxtpt
	jmp warmst
eof:
}
	
.pc = $fb8d // end of kernal "Store Tape characters" routine
	
.pc = $10000

.function patch(start, end) {
  .return "PATCH " + toIntString(start-$e000) + " " + toIntString(end-start)
}

.print patch(tapeLoadDisabledMessage, tapeLoadDisabledMessage.eof)
.print patch(powerUpMessage, powerUpMessage.eof)
.print patch(disableTapeLoad, disableTapeLoad.eof)
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
.print patch(wedge, wedge.eof)	
	
.print "free: 0x" + toHexString(write.eof) + "-0xf5ab"
.print "free: 0x" + toHexString(save.eof) + "-0xfb8d"