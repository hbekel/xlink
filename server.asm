.pc = $1000

.var start = $c1    // Transfer start address
.var end   = $c3    // Transfer end address

.var mem   = $fc    // Memory config
.var bank  = $fd
.var low   = $fe    // Ack value low
.var high  = $ff    // Ack value high
	
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
.macro strobe() { :ack() }

.macro screenOff() {
	lda #$0b
	sta $d011
}

.macro screenOn() {
	lda #$1b
	sta $d011
}

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

	ldy #$00
	
	lda mem         // check if specific memory config was requested
	cmp #$37
	beq fast
	jmp slow
	
fast:	
!loop:  jsr read        
	sta (start),y   // write with normal memory config
	jsr next
	bne !loop-
	jmp done

slow:	
!loop:  jsr read        
	lda mem         // write with requested memory config
	sta $01
	sta (start),y
	lda #$37
	sta $01
	jsr next
	bne !loop-

done:   :screenOn()
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
	jsr write
	jsr next
	bne !loop-
	jmp done

slow:
!loop:  lda mem        // read with requested memory config
	sta $01
	lda (start),y
	lda #$37
	sta $01
	jsr write
	jsr next
	bne !loop-
	
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
	ldx #$ff txs // reset stack pointer

	jsr read sta mem
	jsr read sta bank

	lda mem
	sta $01
	
	jsr read pha // push high byte of jump address
	jsr read pha // push low byte of jump address

	lda #$00 tax tay pha // clear registers & push clear flags 
	
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
	
readHeader: {
	jsr read sta mem
	jsr read sta bank
	jsr read sta start
	jsr read sta start+1
	jsr read sta end
	jsr read sta end+1
	rts
}

read: {
	:wait() lda $dd01 :ack()
	rts
}

write: {
	sta $dd01 :strobe() :wait()
	rts
}
	
next: {
	inc start
	bne check
	inc start+1

check:	lda start+1
	cmp end+1
	bne done

	lda start
	cmp end
	
done:	rts
}


