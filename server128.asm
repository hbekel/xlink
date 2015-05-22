.import source "server.h"

.pc = cmdLineVars.get("pc").asNumber()
	
//------------------------------------------------------------------------------

install: {
        lda #$00   // set CIA2 port B to input
	sta $dd03
  
	lda $dd02  // set CIA2 PA2 to output
        ora #$04
	sta $dd02

	lda $dd0d  // clear stale handshake

	sei        // setup irq	
        lda #<irq 
        ldx #>irq
        sta $0314
        stx $0315       
        cli

	lda #$4c   // install re-entry via "SYS6400"
	sta $1900
	lda #<install
	sta $1901
	lda #>install
	sta $1902

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
	
	:checkBank()
	
close:	ldy #$00	
!loop:  :wait()
	lda $dd01
	sta (start),y 
	:ack()
	:next()
	jmp done

far:	
!loop:  ldy #$00
	:wait()

	lda #start
	sta stashptr	

	ldx mem	
	lda $dd01 
	jsr stash
		
	:ack()
	:next()
	
done:   :screenOn()
	:relinkBasic()
	jmp sysirq
}

//------------------------------------------------------------------------------

save: {
	jsr readHeader
	:screenOff()
	
        :output()

	:checkBank()
	
close:	ldy #$00
!loop:  lda (start),y  
	:write()
	:next()
	jmp done

far:	
!loop:  ldy #$00
	lda #start
	sta fetchptr	

	ldx mem	
	jsr fetch
		
	:write()
	:next()

done:	:input()
	:screenOn()
	jmp sysirq
}	

//------------------------------------------------------------------------------
	
poke: {
	jsr read stx mem 
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

	:wait()
	lda $dd01 pha
	:ack()
	
	:checkBank()
	
close:  ldy #$00
	pla sta (start),y
	jmp done

far:    ldy #$00
	lda #start
	sta stashptr

	ldx mem
	pla jsr stash
	
done:   jmp sysirq
}

//------------------------------------------------------------------------------
	
peek: {
	jsr read stx mem
	jsr read stx bank
	jsr read stx start
	jsr read stx start+1

        :output()
	
	:checkBank()
	
close:	ldy #$00
	lda (start),y
	:write()
	jmp done

far:	lda #start
	sta fetchptr

	ldx mem
	jsr fetch
	
	:write()

done:   :input()
	
	jmp sysirq
}
	
//------------------------------------------------------------------------------

jump: {
 	jsr read stx mem
	jsr read stx bank

	ldx #$ff txs        // reset stack pointer

        lda #>repl     pha  // make sure the code jumped to can rts to basic
        lda #[<repl-1] pha  // (only if the bank includes basic rom, of course) 

	// prepare jmpfar...
	
	lda bank sta $02    // setup requested bank value

	jsr read stx $03    // setup jump address (sent msb first by client)
	jsr read stx $04

	// set clean registers and flags...
	
	lda #$00 sta $05 sta $06 sta $07 sta $08 

	jmp jmpfar          // implies rti
}
	
//------------------------------------------------------------------------------
	
run: {
	jsr uninstall

	lda #$01 sta cursor // cursor off

	cli jmp basrun      // perform RUN
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
