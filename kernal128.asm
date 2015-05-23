.pc = $e000

.import source "server.h"

//------------------------------------------------------------------------------	
	
.pc = $fa66 // wedge into system irw
wedge: {
	jmp irq
resume: 
eof:	
}

//------------------------------------------------------------------------------
.pc = $e8d8 // start of search tape header routine
irq: {
done:   jsr jrsirq
        jmp wedge.resume
eof:    
}        

//------------------------------------------------------------------------------		
        
.pc = $10000

//------------------------------------------------------------------------------		
	
.function patch(start, end) {
	
	.var offset = start - $e000
	.var count = end-start

        .return " " + toIntString(offset) + " " + toIntString(count)
}
	
//------------------------------------------------------------------------------		

.var command = "tools/make-kernal c128 kernal128.bin"

.eval command = command + patch(wedge, wedge.eof)	
.eval command = command + patch(irq, irq.eof)

.print command
//------------------------------------------------------------------------------			
