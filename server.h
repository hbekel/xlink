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
