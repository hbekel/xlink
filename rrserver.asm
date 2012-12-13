.pc = $9990

jmp rom.install
	
//------------------------------------------------------------------------------
	
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
	
.macro wait() { // Wait for handshake from PC (falling edge on FLAG <- Parport STROBE)
loop:	lda $dd0d
	and #$10
	beq loop
}

.macro ack() { // Send handshake to PC (rising edge on CIA2 PA2 -> Parport ACK) 
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
rom: {
install: {
	
	lda $0314    // check if already installed
	cmp #<ram.irq
	bne install
	lda $0315
	cmp #>ram.irq
	bne install

	jsr uninstall // is installed -> uninstall
	jmp done
	
install:
	lda #$00  // set CIA2 port B to input
	sta $dd03
	
	lda $dd02 // set CIA2 PA2 to output
	ora #$04
	sta $dd02

	lda $dd0d // clear stale handshake

	sei
	lda #<ram.irq // setup irq
	ldx #>ram.irq
	sta $0314
	stx $0315	
	cli

	ldx #eof-ram-1 // install ramcode
copy:	lda ram,x
	sta $02a7,x
	dex
	bpl copy

done:	jmp ram.dorts
}

uninstall: {
	jsr hidemsg
	lda #$31
	ldx #$ea
	sta $0314
	stx $0315
	rts
}
	
irq: {
	jsr showmsg
	
	lda $dd0d // check for strobe from PC
	and #$10
	beq done  // no command

	ldy $dd01 // read command

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
	
done:	jmp ram.dorti
}

load: {
	:screenOff()
	jsr readHeader
	:checkBasic()

	ldy #$00        // prepare read
	
	lda mem         // check if specific memory config was requested
	cmp #$37        
	bne slow        
	
fast:	
!loop:  :wait()         // write to ram with normal memory config
	lda $dd01 
	sta (start),y   
	:ack()
	:next()
	jmp done
	                // write to ram with io disabled ($33)
slow:	
!loop:  :wait()
	lda $dd01
	ldx #$33
	stx $01
	sta (start),y
	lda #$37
	sta $01
	:ack()
	:next()

done:   :relinkBasic()
	:screenOn()
	jmp irq.done
}

save: {
	:screenOff()
	jsr readHeader

	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03

	ldy #$00      // prepare reading
	
	lda mem       // check memory config...
	and #$03
	cmp #$03
	bne slow      // have to read from ram unless highram and lowram are 1  

	lda mem       // if mem is $37, we can simply read fastest
	cmp #$37
	bne fast
	
fastest:
!loop:  lda (start),y
	:write()
	:next()
	jmp done

fast:
	ldx mem       // need to change memory, but cartridge is still here
!loop:	stx $01
	lda (start),y
	ldx #$37
	stx $01
	:write()
	:next()
	jmp done	

slow:	
!loop:  jsr ram.read  // need to read from ram since cartridge will be gone
	:write()
	:next()
	
done:	lda #$00       // reset CIA2 port B to input
	sta $dd03
	
	:screenOn()
	jmp irq.done
}
	
poke: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

	lda mem
	cmp #$37
	bne slow

fast:   ldy #$00
	jsr read
	txa
	sta (start),y
	jmp done
	
slow:   ldy #$00
	jsr read
	txa
	ldx #$33
	stx $01
	sta (start),y
	lda #$37
	sta $01

done:	jmp irq.done
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
	jsr ram.read   
	jsr write

done:	lda #$00   // reset CIA2 port B to input
	sta $dd03
	
	jmp irq.done
}
	
jump: {
	jsr uninstall	

	jsr read stx mem
	jsr read stx bank

	ldx #$ff txs // reset stack pointer
	
	jsr read txa pha // push high byte of jump address
	jsr read txa pha // push low byte of jump address

	lda mem  // apply requested memory config
	sta $01

	lda #$00 tax tay pha // clear registers & push clean flags 
	
	jmp ram.jump
}

run: {
	jsr uninstall

	ldx #$ff txs     // reset stack pointer
	lda #$01 sta $cc // cursor off

	jsr insnewl     // run BASIC program
	jsr restxtpt

	jmp ram.run
}

showmsg: {
	ldx #$27
loop:	lda msg,x
	sta $0400,x
	lda #$07
	sta $d800,x
	dex
	bpl loop
	rts	

msg: .byte $A0, $A0, $A0, $A0, $A0, $A0, $83, $B6, $B4, $8C
     .byte $89, $8E, $8B, $A0, $93, $85, $92, $96, $85, $92
     .byte $A0, $B1, $AE, $B0, $A0, $89, $8E, $93, $94, $81
     .byte $8C, $8C, $85, $84, $A0, $A0, $A0, $A0, $A0, $A0
}

hidemsg: {
	ldx #$27
	lda #$20
loop:	sta $0400,x
	dex
	bpl loop
	rts
}
	
} // end rom
	
ram: { // copied to $02a7
.pseudopc $02a7 {
	
irq:	lda #$18   // enable rr bank 3
	sta $de00	  	
	jmp rom.irq // jump to service irq routine in rom

dorti:	lda #$48   // reset rr bank
	sta $de00
	jmp $ea31  // jump to system irq

dorts:	lda #$48   // enable rr bank 0
	sta $de00
	rts

read:	lda mem
	sta $01
	lda (start),y  // read from ram
	ldy #$37
	sty $01
	ldy #$00
	rts
	
jump:   lda #$48   // enable rr bank 0
	sta $de00
	rti
	
run:    lda #$48   // enable rr bank 0
	sta $de00

	jmp warmst
}
}
eof:

.print "ramcode end: 0x" + toHexString(eof-ram+ram.irq)

	
