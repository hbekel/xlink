.pc = $c000

jmp install

//------------------------------------------------------------------------------
	
.var start  = $c1    // Transfer start address
.var end    = $c3    // Transfer end address
.var bstart = $2b    // Start of Basic program text
.var bend   = $2d    // End of Basic program text
	
.var mem   = $fc    // Memory config
.var bank  = $fd    // bank config
.var low   = $fe    // $dd00 | #$04 (ack bit low)
.var high  = $ff    // $dd00 & #$fb (ack bit high)

.var relink   = $a533 // Relink Basic program
.var insnewl  = $a659 // Insert new line into BASIC program
.var restxtpt = $a68e // Reset BASIC text pointer
.var warmst   = $a7ae // Basic warm start (e.g. RUN)

.namespace Command {
.label load  = $01
.label save  = $02
.label peek  = $03
.label poke  = $04
.label jump  = $05
.label run   = $06
}
	
.macro wait() { // Wait for handshake from PC (falling edge on FLAG <- Parport STROBE)
loop:	lda $dd0d
	and #$10
	beq loop
}

.macro ack() { // Send handshake to PC (rising edge on CIA2 PA2 -> Parport ACK) 
	ldx low
	stx $dd00
	ldx high
	stx $dd00
}

.macro fastack() { // assumes ($dd00 | $04) in X
	xaa #$fb
	sta $dd00
	stx $dd00
}
	
.macro strobe() { :ack() }
.macro faststrobe() { :fastack() }

.macro read() {
	:wait() lda $dd01 :ack()
}

.macro write() {
	sta $dd01 :strobe() :wait()
}

.macro fastwrite() {
	sta $dd01 :faststrobe() :wait()
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
	jsr read sta mem
	jsr read sta bank
	jsr read sta start
	jsr read sta start+1
	jsr read sta end
	jsr read sta end+1
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

	lda $dd00 // prepare ack values
	and #$fb sta low   
	ora #$04 sta high

	:ack()   // ack command 

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
	
done:	jmp $ea31
}
	
load: {
	:screenOff()
	jsr readHeader
	:checkBasic()
	
	lda mem         // check if specific memory config was requested
	cmp #$37
	beq fast
	jmp slow
	
fast:	ldy #$00
	ldx high        // prepare fastack

!loop:  :wait()
	lda $dd01 
	sta (start),y   // write with normal memory config
	:fastack()
	:next()
	jmp done

slow:	ldx high        // prepare fastack
!loop:  :wait()
	ldy mem         // write with requested memory config
	sty $01
	ldy #$00
	sta (start),y
	lda #$37
	sta $01
	:fastack()
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

fast:	ldx high       // prepare fastwrite
!loop:  lda (start),y  // read with normal memory config
	:fastwrite()
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

peek: {
	jsr read sta mem
	jsr read sta bank
	jsr read sta start
	jsr read sta start+1

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
	
poke: {
	jsr read sta mem
	jsr read sta bank
	jsr read sta start
	jsr read sta start+1
	jsr read

	ldy #$00
	ldx mem
	stx $01
	sta (start),y
	lda #$37
	sta $01

	jmp irq.done
}
	
jump: {
	jsr read sta mem
	jsr read sta bank

	ldx #$ff txs // reset stack pointer
	
	jsr read pha // push high byte of jump address
	jsr read pha // push low byte of jump address

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


