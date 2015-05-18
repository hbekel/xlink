.import source "server.h"

.pc = $2000

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

//------------------------------------------------------------------------------
	
uninstall: {
	lda #<sysirq
	ldx #>sysirq
	sta $0314
	stx $0315
	rts
}

//------------------------------------------------------------------------------
	
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

!next:	cpy #Command.inject
	bne !next+
	jmp inject

!next:	cpy #Command.identify
	bne !next+
	jmp identify


!next:	
done:   jmp sysirq
}

//------------------------------------------------------------------------------
	
load: {
	jsr readHeader
	:screenOff()
	
	:checkBasic()
	
	ldy #$00
	
!loop:  :wait()
	lda $dd01 
	sta (start),y 
	:ack()
	:next()
	jmp done

done:	:screenOn()
	:relinkBasic()
	jmp sysirq
}

//------------------------------------------------------------------------------

save: {
	jsr readHeader
	:screenOff()
	
        :output()
	ldy #$00

!loop:  lda (start),y  // read with normal memory config
	:write()
	:next()

done:	lda #$00   // reset CIA2 port B to input
	sta $dd03
	
	:screenOn()
	jmp sysirq
}	

//------------------------------------------------------------------------------
	
poke: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

	ldy #$00	

	:wait()
	lda $dd01
	tax :ack() txa
	
	sta (start),y

	jmp sysirq
}

//------------------------------------------------------------------------------
	
peek: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

        :output()
	
	ldy #$00
	lda (start),y

	jsr write

done:	:input()
	
	jmp sysirq
}
	
//------------------------------------------------------------------------------

jump: {
	jsr read stx mem
	jsr read stx bank

	ldx #$ff txs // reset stack pointer

        lda #>repl     pha // make sure the code jumped to can rts to basic
        lda #[<repl-1] pha   
  
	jsr read txa pha // push high byte of jump address
	jsr read txa pha // push low byte of jump address

	lda #$00 tax tay pha // clear registers & push clean flags 
	
	rti // jump via rti
}

//------------------------------------------------------------------------------
	
run: {
	jsr uninstall

	lda #$01    // turn off cursor
	sta cursor

	cli         // allow system interrupts
	jmp $5aa6   // perform RUN
}
	
//------------------------------------------------------------------------------

inject:	{
	lda #>return pha
	lda #<return pha
	
	jsr read txa pha 
	jsr read txa pha
	
	rts
	
return: nop
	jmp sysirq
}

//------------------------------------------------------------------------------
	
identify: {
        :output()

        lda Server.size
        jsr write
  
        lda Server.id
        jsr write

        lda Server.id+1
        jsr write

        lda Server.id+2
        jsr write

        lda Server.id+3
        jsr write

        lda Server.id+4
        jsr write
  
        lda Server.version   
        jsr write

        lda Server.machine
        jsr write

        lda Server.type
        jsr write       

        lda Server.start
        jsr write

        lda Server.start+1
        jsr write

        lda Server.end
        jsr write

        lda Server.end+1
        jsr write

	lda memtop
	jsr write

	lda memtop+1
	jsr write
        
done:   :input()
        
        jmp sysirq
}

//------------------------------------------------------------------------------

readHeader: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1
	jsr read stx end
	jsr read stx end+1
	rts
}

//------------------------------------------------------------------------------
	
read: {
	:read()
	rts
}

//------------------------------------------------------------------------------	
	
write: {
	:write()
	rts
}

//------------------------------------------------------------------------------
	
Server:	{
size:    .byte $05
id:      .byte 'X', 'L', 'I', 'N', 'K'
start:	 .word install
version: .byte $10
type:	 .byte $00 // 0 = RAM, 1 = ROM
machine: .byte $01 // 0 = C64, 1 = C128
end:	 .word *+2
}

//------------------------------------------------------------------------------	