.pc = $c000

jmp install

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
	
.macro wait() { // Wait for handshake from PC (falling edge on FLAG <- Parport STROBE)
loop:	lda $dd0d
	and #$10
	beq loop
}

.macro ack() { // Send handshake to PC (flip bit on CIA2 PA2 -> Parport BUSY) 
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

readHeader: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1
	jsr read stx end
	jsr read stx end+1
	rts
}

read: { :read() rts }
write: { :write() rts }
	
//------------------------------------------------------------------------------
	
install: {
	lda #$00  // set CIA2 port B to input
	sta $dd03
	
	lda $dd02 // set CIA2 PA2 to output
	ora #$04
	sta $dd02

	lda $dd0d // clear stale handshake

	sei
	lda #<irq // setup irq
	ldx #>irq
	sta $0314
	stx $0315	
	cli
	
	rts
}

uninstall: {
	lda #$31
	ldx #$ea
	sta $0314
	stx $0315
	rts
}
	
irq: {
	lda $dd0d // check for strobe from PC
	and #$10
	beq done  // no command

	ldy $dd01 // read command
	:ack()   

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
done:	jmp $ea31
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
}

save: {
	:screenOff()
	jsr readHeader

	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03

	ldy #$00

	lda mem        // check if specific memory config was requested
	cmp #$37
	beq fast
	jmp slow	

fast:	
!loop:  lda (start),y  // read with normal memory config
	:write()
	:next()
	jmp done

slow:
!loop:  lda mem        // read with requested memory config
	sta $01
	lda (start),y
	ldx #$37
	stx $01
	:write()
	:next()
	
done:	lda #$00   // reset CIA2 port B to input
	sta $dd03
	
	:screenOn()
	jmp irq.done
}
	
poke: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

	ldy #$00	

	:wait()
	lda $dd01
	tax :ack() txa
	
	ldx mem
	stx $01
	sta (start),y
	lda #$37
	sta $01

	jmp irq.done
}

peek: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03
	
	ldy #$00
	ldx mem
	stx $01
	lda (start),y
	ldx #$37
	stx $01

	jsr write

done:	lda #$00   // reset CIA2 port B to input
	sta $dd03
	
	jmp irq.done
}
	
jump: {
	jsr read stx mem
	jsr read stx bank

	ldx #$ff txs // reset stack pointer
	
	jsr read txa pha // push high byte of jump address
	jsr read txa pha // push low byte of jump address

	lda mem  // apply requested memory config
	sta $01
	
	lda #$00 tax tay pha // clear registers & push clean flags 
	
	rti // jump via rti
}

run: {
	jsr uninstall
	
	ldx #$ff txs     // reset stack pointer
	lda #$01 sta $cc // cursor off

	jsr insnewl     // run BASIC program
	jsr restxtpt
	jmp warmst
}

disableIrq: {
	// some io routines use irqs and cli when done,
	// so the sysirq needs to be disabled during io
	lda #$01  
	sta $dc0d
	rts
}

enableIrq: {
	sei
	lda $dc0d
	lda #$81  
	sta $dc0d
	rts
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
}	

withoutCommand:	{
	lda #$00
	tax
	tay
	jsr setnam
	rts
}
	
withBufferPointerReset: {	
	lda #cmdEnd-cmd
	ldx #<cmd
	ldy #>cmd
	jsr setnam
	rts

cmd: .text "B-P 2 0"
cmdEnd:	
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
}

closeAll: {
	jsr clrchn

	lda #$0f
	jsr close

	lda #$02
	jsr close
	rts
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

done:  	lda #$ff   // send 0xff as EOT marker to the client
	:write()

	lda #$00   // reset CIA2 port B to input
	sta $dd03	
	
	lda #$0f
	jsr close
	jsr clrchn
	
	:ack()

	jsr enableIrq
	jmp irq.done
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
}
