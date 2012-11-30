.pc = $c000

.var start = $c1    // Transfer start address
.var end   = $c3    // Transfer end address

.var low   = $fe    // Ack value low
.var high  = $ff    // Ack value high
	
.var insnewl  = $a659 // Insert new line into BASIC program
.var restxtpt = $a68e // Reset BASIC text pointer
.var warmst   = $a7ae // Basic warm start (e.g. RUN)

.namespace Command {
.label load  = $01
.label save  = $02
.label jump  = $03
.label run   = $04
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
loop:   jsr read
	sta (start),y
	jsr next
	bne loop

done:   :screenOn()
	jmp irq.done
}

save: {
	:screenOff()
	jsr readHeader

	:wait()    // wait until PC has set its port to input

	lda #$ff   // set CIA2 port B to output
	sta $dd03

	ldy #$00
loop:   lda (start),y
	jsr write
	jsr next
	bne loop

	lda #$00   // reset CIA2 port B to input
	sta $dd03
	
done:   :screenOn()
	jmp irq.done
}

jump: {
	ldx #$ff txs // reset stack pointer

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
	jsr read // mem
	jsr read // crt
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


