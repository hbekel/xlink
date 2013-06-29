.pc = $c000

jmp install

//------------------------------------------------------------------------------
	
.var start  = $c1    // Transfer start address
.var end    = $c3    // Transfer end address
.var bstart = $2b    // Start of Basic program text
.var bend   = $2d    // End of Basic program text
	
.var mem    = $fe    // Memory config
.var bank   = $ff    // bank config

.var relink   = $a533 // Relink Basic program
.var insnewl  = $a659 // Insert new line into BASIC program
.var restxtpt = $a68e // Reset BASIC text pointer
.var warmst   = $a7ae // Basic warm start (e.g. RUN)

.var setnam   = $ffbd // Set filename
.var setlfs   = $ffba // Set logical file parameters
.var open     = $ffc0 // Open file
.var close    = $ffc3 // Close file
.var chkin    = $ffc6 // Select input channel	
.var chkout   = $ffc9 // Select output channel
.var chrin    = $ffcf // Read character
.var chrout   = $ffd2 // Write character
.var clrchn   = $ffcc // Clear channel
	
.namespace Command {
.label load         = $01
.label save         = $02
.label poke         = $03
.label peek         = $04
.label jump         = $05
.label run          = $06
.label dos          = $07
.label sector_read  = $08
.label sector_write = $09
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

!next:  cpy #Command.dos
	bne !next+
	jmp dos

!next:	cpy #Command.sector_read
	bne !next+
	jmp sector_read
	
!next:	cpy #Command.sector_write
	bne !next+
	jmp sector_write

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

dos: {
	lda #$01  // disable system irq
	sta $dc0d 	

	lda #$0f
	ldx $ba
	bne skip
	ldx #$08
skip:	ldy #$0f	
	jsr setlfs
	
	jsr open
	bcs done

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

	jsr clrchn
	
	:wait()
	:ack()
	
	sei
	lda $dc0d
	lda #$81  // enable sysirq
	sta $dc0d

	jmp $ea81
}

sector_read: {
	lda #$01  // disable system irq
	sta $dc0d 		

	// open buffer file
	
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

	// open command channel and reset buffer pointer
	lda #cmdEnd-cmd
	ldx #<cmd
	ldy #>cmd
	jsr setnam

	lda #$0f
	ldx $ba
	ldy #$0f
	jsr setlfs

	jsr open

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
	
	jsr clrchn

	lda #$0f
	jsr close

	lda #$02
	jsr close

	// open buffer channel again
	
	lda #$01
	ldx #<channel
	ldy #>channel
	jsr setnam

	lda #$02
	ldx $ba
	ldy #$02
	jsr setlfs	

	jsr open

	// open command channel and reset buffer pointer again
	lda #cmdEnd-cmd
	ldx #<cmd
	ldy #>cmd
	jsr setnam

	lda #$0f
	ldx $ba
	ldy #$0f
	jsr setlfs

	jsr open
	
	ldx #$02
	jsr chkin

	// read drive buffer to screen
	
	ldx #$00
!loop:	jsr chrin
	sta $0400,x
	inx
	bne !loop-
	
	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03
	
	// read data from screen and send it to server
	
	ldx #$00
!loop:	lda $0400,x
	:write()
	inx
	bne !loop-

	lda #$00   // reset CIA2 port B to input
	sta $dd03	
done:
	jsr clrchn
	
	lda #$0f
	jsr close

	lda #$02
	jsr close
	
	:ack()	

	sei
	lda $dc0d
	lda #$81  // enable sysirq
	sta $dc0d
	
	jmp $ea81

channel: .text "#"

cmd: .text "B-P 2 0"
cmdEnd:	
}
	
sector_write: {

	lda #$01  // disable system irq
	sta $dc0d 	
	
	// open buffer channel
	
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
	bcs done

	// open the command channel

	lda #cmdEnd-cmd
	ldx #<cmd
	ldy #>cmd
	jsr setnam
	lda #$0f
	ldx $ba
	ldy #$0f
	jsr setlfs

	jsr open
	bcs done
	
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
	
	jsr clrchn

	lda #$0f
	jsr close

	lda #$02
	jsr close

	:ack()
	
	sei
	lda $dc0d
	lda #$81  // enable sysirq
	sta $dc0d
	
	jmp $ea81

channel: .text "#"
	
cmd: .text "B-P 2 0"
cmdEnd:
}
