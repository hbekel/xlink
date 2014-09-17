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
.label load        = $01
.label save        = $02
.label poke        = $03
.label peek        = $04
.label jump        = $05
.label run         = $06
.label extend      = $07
.label identify    = $fe
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
        beq push

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
	bit mem
	bmi skip
	lda #$0b
	sta $d011
skip:	
}

.macro screenOn() {
	bit mem
	bmi skip
	lda #$1b
	sta $d011
skip:	
}
