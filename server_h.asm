/* -*- mode: kasm -*- */

.import source "target.asm"
	
.var start = $c1        // Transfer start address
.var end   = $c3        // Transfer end address

.var basic  = $0801     // Default Basic start address
.var bstart = $2b       // Start of Basic program text
.var bend   = $2d       // End of Basic program text
.var mem    = $fe       // Memory config
.var bank   = $ff       // bank config
.var mode   = $9d       // Error mode flag

.var sysirq   = $ea31   // System IRQ
.var jiffy    = $ffea   // Update jiffy clock   
.var relink   = $a533   // Relink Basic program
.var insnewl  = $a659   // Insert new line into BASIC program
.var restxtpt = $a68e   // Reset BASIC text pointer
.var warmst   = $a7ae   // Basic warm start (e.g. RUN)
.var basrun   = $a871   // Perform RUN
.var memtop   = $0283   // Top of lower memory area
.var repl     = $a480   // BASIC read-eval-print loop
.var cursor   = $cc     // Cursor blink flag (00=blinking)
.var default  = $37     // Default processor port value
.var booted   = repl    // How to exit the bootstrap server 
   
.if(target == "c128") {	
  .eval start = $c1     // Transfer start address
  .eval end   = $fd     // Transfer end address

  .eval basic  = $1c01  // Default Basic start address	
  .eval bstart = $2d    // Start of Basic program text
  .eval bend   = $1210  // End of Basic program text
  .eval mem    = $fb    // Memory config
  .eval bank   = $fc    // bank config
  .eval mode   = $7f    // Error mode flag 

  .eval sysirq  = $fa65  // System IRQ
  .eval relink  = $4f4f  // Relink Basic program
  .eval basrun  = $5aa6  // Perform RUN
  .eval memtop  = $1212  // Top of lower memory area	
  .eval repl    = $4dc6  // BASIC read-eval-print loop
  .eval cursor  = $0a27  // Cursor blink flag (00=blinking)
  .eval default = $73    // Default processor port value
  .eval booted  = basrun // How to exit the bootstrap server 
}

// C128 specific:
	
.var mmu      = $ff00
.var bank2mmu = $f7f0

.var fetch    = $02a2
.var fetchptr = $02aa
	
.var stash    = $02af
.var stashptr = $02b9

.var jmpfar   = $02e3
.var jrsirq   = $c024

.var common  = $02a2
.var reinst  = $e0ee
   
.var saved = $ff
   
// Commands:
	
.namespace Command {
.label load        = $01
.label save        = $02
.label poke        = $03
.label peek        = $04
.label jump        = $05
.label run         = $06
.label inject      = $07
.label identify    = $fe
}
	
.macro wait() { // Wait for handshake from PC (falling edge on FLAG)
loop:
	lda $dd0d
	and #$10
	beq loop
}

.macro ack() { // Send handshake to PC (flip bit on CIA2 PA2) 
	lda $dd00
	eor #$04
	sta $dd00
}

.macro strobe() { :ack() }
	
.macro read() {
	:wait()
	ldx $dd01
	:ack()
}

.macro write() {
	sta $dd01
	:strobe()
	:wait()
}

.macro output() {
	:wait()       
	lda #$ff      
	sta $dd03
}   

.macro input() {
	lda #$00   
	sta $dd03
}
  
.macro checkBank() {
	lda mem
	cmp mmu	
	// bne far
	  
	ldx bank
	lda bank2mmu,x
	sta mem
	cmp mmu
	// bne far
}   

.macro checkIO() {
	lda mem
	and #1
	cmp #0
	// beq fast
}
	
   
.macro jsrcommon(code) {
	ldx #[code.eof-code]

!loop:	lda code,x
	sta common,x
	dex
	bpl !loop-

	jsr common
	jsr reinst
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
