:BasicUpstart2(main)

.var size = cmdLineVars.get("size").asNumber()
.var dest = $d000-size
	
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
	lda cnt+1
	bne copy
	lda cnt
	bne copy

	jsr dest
	rts
}
	
code:
	
