.import source "target.asm"
	
.var start = $c1        // Transfer start address
.var end   = $c3        // Transfer end address

.var basic  = $0801     // Default Basic start address
.var bstart = $2b       // Start of Basic program text
.var bend   = $2d       // End of Basic program text
.var mem    = $fe       // Memory config
.var bank   = $ff       // bank config
.var mode   = $9d       // Error mode flag (00 = Program mode, 80 = direct mode)

.var sysirq   = $ea34   // System IRQ
.var relink   = $a533   // Relink Basic program
.var insnewl  = $a659   // Insert new line into BASIC program
.var restxtpt = $a68e   // Reset BASIC text pointer
.var warmst   = $a7ae   // Basic warm start (e.g. RUN)
.var basrun   = $a871   // Perform RUN
.var memtop   = $0283   // Top of lower memory area
.var repl     = $a480   // BASIC read-eval-print loop
.var cursor   = $cc     // Cursor blink flag (00=blinking)
.var default  = $37     // Default processor port value
   
.if(target == "c128") {	
  .eval start = $c1     // Transfer start address
  .eval end   = $fd     // Transfer end address

  .eval basic  = $1c01  // Default Basic start address	
  .eval bstart = $2d    // Start of Basic program text
  .eval bend   = $1210  // End of Basic program text
  .eval mem    = $fb    // Memory config
  .eval bank   = $fc    // bank config
  .eval mode   = $7f    // Error mode flag (00 = Program mode, 80 = direct mode)	

  .eval sysirq  = $fa65  // System IRQ
  .eval relink  = $4f4f  // Relink Basic program
  .eval basrun  = $5aa6  // Perform RUN
  .eval memtop  = $1212  // Top of lower memory area	
  .eval repl    = $4dc6  // BASIC read-eval-print loop
  .eval cursor  = $0a27  // Cursor blink flag (00=blinking)
  .eval default = $73    // Default processor port value
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
loop:	lda $dd0d
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
	:wait() ldx $dd01 :ack()
}

.macro write() {
	sta $dd01 :strobe() :wait()
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

.macro checkBank() {
        lda mem
	cmp mmu	
	bne far
	  
        ldx bank
	lda bank2mmu,x
	sta mem
	cmp mmu
	bne far
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
