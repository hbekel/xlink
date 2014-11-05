.import source "server.h"
	
.var setnam = $ffbd // Set filename
.var setlfs = $ffba // Set logical file parameters
.var open   = $ffc0 // Open file
.var close  = $ffc3 // Close file
.var chkin  = $ffc6 // Select input channel	
.var chkout = $ffc9 // Select output channel
.var chrin  = $ffcf // Read character
.var chrout = $ffd2 // Write character
.var clrchn = $ffcc // Clear channel
.var readst = $ffb7 // Read status byte
	
.pc = $0000

//------------------------------------------------------------------------------

lib: {
offset:
.pseudopc $0100 {
address:

disableIrq: {
	// some io routines use irqs and cli when done,
	// so the sysirq needs to be disabled during io
	lda #$01  
        sta $dc0d
	rts
}

enableIrq: {
	sei
	lda $dc0d
	lda #$81  
	sta $dc0d
	rts
}
	
openBuffer: {
	// open buffer channel
	// open 2,8,2,"#"
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
	rts

channel: .text "#"
}	

withoutCommand:	{
	lda #$00
	tax
	tay
	jsr setnam
	rts
}
	
withBufferPointerReset: {	
	lda #cmdEnd-cmd
	ldx #<cmd
	ldy #>cmd
	jsr setnam
	rts

cmd: .text "B-P 2 0"
cmdEnd:	
}
		
openCommandChannel: {
	
	// open command channel
	// open 15,8,15	

	lda #$0f
	ldx $ba
	bne skip
	ldx #$08
skip:	ldy #$0f
	jsr setlfs

	jsr open
	rts
}

closeAll: {
	jsr clrchn

	lda #$0f
	jsr close

	lda #$02
	jsr close
	rts
}

} // end of .pseudopc 
.label size=*-offset	
}

//------------------------------------------------------------------------------
	
driveStatus: {
offset:	
.pseudopc $0334 {	
address:	
	jsr lib.disableIrq
	jsr lib.withoutCommand jsr lib.openCommandChannel

	ldx #$0f       // open command channel as input
	jsr chkin	
	
	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03
	
loop:	jsr readst    // send status
	bne done
	jsr chrin
	:write()
	jmp loop

done:  	lda #$ff   // send 0xff as EOT marker to the client
	:write()

	lda #$00   // reset CIA2 port B to input
	sta $dd03	
	
	lda #$0f
	jsr close
	jsr clrchn
	
	:ack()

	jsr lib.enableIrq
	rts
}
.label size=*-offset
}

//------------------------------------------------------------------------------
	
dosCommand: {
offset:
.pseudopc $0334 {
address:	
	jsr lib.disableIrq
	jsr lib.withoutCommand jsr lib.openCommandChannel
	
	ldx #$0f
	jsr chkout

	:read() // length of cmd string now in x
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

	:ack()
	
	jsr lib.enableIrq
	rts
}
.label size=*-offset
}

//------------------------------------------------------------------------------
	
sectorRead: {
offset:
.pseudopc $0334 {
address:		
	jsr lib.disableIrq
	jsr lib.openBuffer
	jsr lib.withBufferPointerReset jsr lib.openCommandChannel

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
	
	jsr lib.closeAll

	jsr lib.openBuffer
	jsr lib.withBufferPointerReset jsr lib.openCommandChannel
 	
	ldx #$02
	jsr chkin

	:wait()        // wait until PC has set its port to input
	lda #$ff       // and set CIA2 port B to output
	sta $dd03
	
	// read data from drive buffer and send it to server
	
	ldx #$00
!loop:	jsr chrin
	:write()
	inx
	bne !loop-

	lda #$00   // reset CIA2 port B to input
	sta $dd03	
done:
	jsr lib.closeAll
	
	:ack()

	jsr lib.enableIrq
	rts
}
.label size=*-offset
}

//------------------------------------------------------------------------------

sectorWrite: {
offset:
.pseudopc $0334 {
address:	
	jsr lib.disableIrq
	jsr lib.openBuffer
	jsr lib.withBufferPointerReset jsr lib.openCommandChannel

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
	
	jsr lib.closeAll
	
	:ack()
	
	jsr lib.enableIrq	
	rts
}
.label size=*-offset
}

serverRelocate: {
offset:
.pseudopc $0334 {
address:	
	jsr read // answer ping
	jsr read // read command
	jsr read stx mem
	jsr read stx bank
	jsr read stx start   stx jump
	jsr read stx start+1 stx jump+1
	jsr read stx end
	jsr read stx end+1

	:screenOff()
	
	ldy #$00
	
!loop:  :wait()
	lda $dd01 
	sta (start),y  
	:ack()
	:next()

	:screenOn()
	
	ldx #$31 // reset sysirq vector
	ldy #$ea
	stx $0314
	sty $0315 

	ldx #$ff txs // reset stack pointer

	lda #$a4 pha // rts in server.install will return back to direct mode
	lda #$7f pha // ($a480 -> BASIC REPL)
	
	lda jump+1 pha // push high byte of jump address
	lda jump pha   // push low byte of jump address
	
	lda #$00 tax tay pha // clear registers & push clean flags 
	
	rti // will jump to the install routine of the relocated server

read: {
	:read()
	rts
}

write: {
	:write()
	rts
}
	
jump:	.word $ffff
}
	
.label size=*-offset
}

	
//------------------------------------------------------------------------------	
	
.function extension(name, address, offset, size) {
  .return "tools/make-extension extensions.bin " + name + " " +
	toIntString(address) + " " +
	toIntString(offset) + " " +
	toIntString(size)
}

.print extension("LIB", lib.address, lib.offset, lib.size)
.print extension("DRIVE_STATUS", driveStatus.address, driveStatus.offset, driveStatus.size)
.print extension("DOS_COMMAND", dosCommand.address, dosCommand.offset, dosCommand.size)
.print extension("SECTOR_READ", sectorRead.address, sectorRead.offset, sectorRead.size)
.print extension("SECTOR_WRITE", sectorWrite.address, sectorWrite.offset, sectorWrite.size)
.print extension("SERVER_RELOCATE", serverRelocate.address, serverRelocate.offset, serverRelocate.size)
	

      