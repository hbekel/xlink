.import source "server.h"

.pc = cmdLineVars.get("pc").asNumber()

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

	lda #$4c   // install re-entry via "SYS1000"
	sta $03e8
	lda #<install
	sta $03e9
	lda #>install
	sta $03ea
	
	rts
}

//------------------------------------------------------------------------------
	
uninstall: {
	lda #>sysirq
	ldx #<sysirq
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
done:   jsr jiffy
	jmp sysirq+3
}

//------------------------------------------------------------------------------
	
load: {
	jsr readHeader
	:screenOff()
	
	:checkBasic()
	
	ldy #$00
	
	lda mem         // check if specific memory config was requested
	and #$7f
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

//------------------------------------------------------------------------------
	
save: {
	jsr readHeader
	:screenOff()
	
        :output()
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
	
	ldx mem
	stx $01
	sta (start),y
	lda #$37
	sta $01

	jmp irq.done
}

//------------------------------------------------------------------------------
	
peek: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

        :output()
	
	ldy #$00
	ldx mem
	stx $01
	lda (start),y
	ldx #$37
	stx $01

	jsr write

done:	:input()
	
	jmp irq.done
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

	lda mem  // apply requested memory config
	sta $01
	
	lda #$00 tax tay pha // clear registers & push clean flags 
	
	rti // jump via rti
}

//------------------------------------------------------------------------------
	
run: {
	jsr uninstall
	
	ldx #$ff txs        // reset stack pointer
	lda #$01 sta cursor // cursor off

	jsr insnewl         // preprare run BASIC program
	jsr restxtpt

	lda #$00            // flag program mode
	sta mode
	
	jmp warmst
}

//------------------------------------------------------------------------------
	
inject:	{
	lda #>return pha
	lda #<return pha
	
	jsr read txa pha 
	jsr read txa pha
	
	rts
	
return: nop
	jmp irq.done
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
        
        jmp irq.done
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
machine: .byte $00 // 0 = C64
end:	 .word *+2
}

//------------------------------------------------------------------------------	


