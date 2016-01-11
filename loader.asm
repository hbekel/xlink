/* -*- mode: kasm -*- */
        
.import source "server.h"

.pc = basic

.byte $0b, >basic, $0a, $00, $9e, $ff, $ff, $ff, $ff
	
.pc = basic+$10

.var size = cmdLineVars.get("size").asNumber()
.var dest = $4000-size
	
.var cnt = $22
.var src = $fc
.var dst = $fe

.macro incword(ptr) {
	inc ptr
	bne done
	inc ptr+1
done:	
}

.macro decword(ptr) {
	dec ptr
        lda ptr
        cmp #$ff
	bne done
	dec ptr+1
done:	
}
		
main: {
	lda #<size
	sta cnt
	lda #>size
	sta cnt+1

	lda #<code
	sta src
	lda #>code
	sta src+1

	lda #<dest
	sta dst
	lda #>dest
	sta dst+1

	ldy #$00
	
copy:	lda (src),y
	sta (dst),y
	:incword(src)
	:incword(dst)
	:decword(cnt)
	lda cnt
	bne copy
	lda cnt+1
	bne copy

        jmp dest
}
	
code:
	
