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

.var setnam = $ffbd // Set filename
.var setlfs = $ffba // Set logical file parameters
.var open   = $ffc0 // Open file
.var close  = $ffc3 // Close file
.var chkin  = $ffc6 // Select input channel	
.var chkout = $ffc9 // Select output channel
.var chrin  = $ffcf // Read character
.var chrout = $ffd2 // Write character
.var clrchn = $ffcc // Clear channel
.var readst = $ffb7 // Read status byte
	
.namespace Command {
.label load        = $01
.label save        = $02
.label poke        = $03
.label peek        = $04
.label jump        = $05
.label run         = $06
.label dosCommand  = $07
.label sectorRead  = $08
.label sectorWrite = $09
.label driveStatus = $0a
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

.macro strobe() { :ack() }

.macro read() {
	:wait() ldx $dd01 :ack()
}

.macro write() {
	sta $dd01 :strobe() :wait()
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
	bne !next+
	jmp run

!next:  cpy #Command.dosCommand
	bne !next+
	jmp dosCommand

!next:	cpy #Command.sectorRead
	bne !next+
	jmp sectorRead
	
!next:	cpy #Command.sectorWrite
	bne !next+
	jmp sectorWrite

!next:	cpy #Command.driveStatus
	bne !next+
	jmp driveStatus

!next:
done:	jsr $ffea
	jmp $ea34
eof:
}

.pc = $f5ab // end of kernal "Load Tape" routine

.pc = $f82e // begin of kernal "Check Tape Status" routine
	
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

disableIrq: {
	// some io routines use irqs and cli when done,
	// so the sysirq needs to be disabled during io
	lda #$01  
	sta $dc0d
	rts
eof:	
}

enableIrq: {
	sei
	lda $dc0d
	lda #$81  
	sta $dc0d
	rts
eof:	
}
	
openBuffer: {
	// open buffer channel
	// open 2,8,2,"#"
	lda #$01
	ldx #<channel
	ldy #>channel
	jsr setnam

	lda #$02
	ldx $ba
	bne skip
	ldx #$08
skip:	ldy #$02
	jsr setlfs	

	jsr open
	rts

channel: .text "#"
eof:	
}	

withoutCommand:	{
	lda #$00
	tax
	tay
	jsr setnam
	rts
eof:	
}
	
withBufferPointerReset: {	
	lda #cmdEnd-cmd
	ldx #<cmd
	ldy #>cmd
	jsr setnam
	rts

cmd: .text "B-P 2 0"
cmdEnd:
eof:	
}
		
openCommandChannel: {
	
	// open command channel
	// open 15,8,15	

	lda #$0f
	ldx $ba
	bne skip
	ldx #$08
skip:	ldy #$0f
	jsr setlfs

	jsr open
	rts
eof:		
}

closeAll: {
	jsr clrchn

	lda #$0f
	jsr close

	lda #$02
	jsr close
	rts
eof:	
}

dosCommand: {
	jsr disableIrq
	jsr withoutCommand jsr openCommandChannel
	
	ldx #$0f
	jsr chkout

	jsr read // length of cmd string now in x
loop:	:wait()
	lda $dd01
	jsr chrout
	:ack()
	dex
	bne loop

	lda #$0d
	jsr chrout
	
done:	jsr clrchn

	lda #$0f
	jsr close

	:ack()
	
	jsr enableIrq
	jmp irq.done
eof:	
}

driveStatus: {
	jsr disableIrq
	jsr withoutCommand jsr openCommandChannel

	ldx #$0f
	jsr chkin	

	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03
	
loop:	jsr readst
	bne done
	jsr chrin
	:write()
	jmp loop

done:  	lda #$ff
	:write()

	lda #$00   // reset CIA2 port B to input
	sta $dd03	
	
	lda #$0f
	jsr close
	jsr clrchn
	
	:ack()

	jsr enableIrq
	jmp irq.done
eof:	
}
	
sectorRead: {
	
	jsr disableIrq
	jsr openBuffer
	jsr withBufferPointerReset jsr openCommandChannel

	// select command channel	
	ldx #$0f
	jsr chkout
	
	// read command from client and write on command channel

	ldy #00
!loop:  :wait()
	lda $dd01 
	jsr chrout
	:ack()
	iny
	cpy #12
	bne !loop-

	lda #$0d
	jsr chrout
	
	jsr closeAll

	jsr openBuffer
	jsr withBufferPointerReset jsr openCommandChannel
 	
	ldx #$02
	jsr chkin

	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03
	
	// read data from drive buffer and send it to server
	
	ldx #$00
!loop:	jsr chrin
	:write()
	inx
	bne !loop-

	lda #$00   // reset CIA2 port B to input
	sta $dd03	
done:
	jsr closeAll
	
	:ack()

	jsr enableIrq
	jmp $ea81
eof:	
}
	
sectorWrite: {

	jsr disableIrq
	jsr openBuffer
	jsr withBufferPointerReset jsr openCommandChannel

	// select buffer as output
	
	ldx #$02
	jsr chkout

	// read sector data from client and write to buffer

	ldy #$00	
!loop:  :wait()
	lda $dd01 
	jsr chrout
	:ack()
	iny
	bne !loop-

	// select command channel
	
	ldx #$0f
	jsr chkout

	// read command from client and write on command channel

	ldy #00
!loop:  :wait()
	lda $dd01 
	jsr chrout
	:ack()
	iny
	cpy #12
	bne !loop-

	// execute command	
	
	lda #$0d
	jsr chrout 

done:	// close files and channels	
	
	jsr closeAll
	
	:ack()
	
	jsr enableIrq	
	jmp $ea81
eof:	
}
	
.pc = $fc92 // end of kernal "Write Tape Leader" routine
	
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
.print patch(disableIrq, disableIrq.eof)
.print patch(enableIrq, enableIrq.eof)
.print patch(openBuffer, openBuffer.eof)
.print patch(withoutCommand, withoutCommand.eof)
.print patch(withBufferPointerReset, withBufferPointerReset.eof)
.print patch(openCommandChannel, openCommandChannel.eof)
.print patch(closeAll, closeAll.eof)
.print patch(dosCommand, dosCommand.eof)
.print patch(driveStatus, driveStatus.eof)
.print patch(sectorRead, sectorRead.eof)
.print patch(sectorWrite, sectorWrite.eof)
.print patch(wedge, wedge.eof)	
	
.print "free: 0x" + toHexString(irq.eof) + "-0xf5ab" + ": " + toIntString($f5ab-irq.eof) + " bytes"
.print "free: 0x" + toHexString(sectorWrite.eof) + "-0xfc92" + " " + toIntString($fc92-sectorWrite.eof) + " bytes"