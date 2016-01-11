/* -*- mode: kasm -*- */
        
.var target = "c64"

.if(cmdLineVars.containsKey("target")) {	
  .eval target = cmdLineVars.get("target")
}
