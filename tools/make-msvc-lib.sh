#!/bin/bash

MSVC_PATH="/cygdrive/c/Program Files (x86)/Microsoft Visual Studio 10.0/"
export PATH="$PATH:$MSVC_PATH/Common7/IDE:$MSVC_PATH/VC/bin"

cat <<EOF > xlink.def
EXPORTS
xlink_error
xlink_version
xlink_set_debug
xlink_has_device
xlink_set_device
xlink_get_device
xlink_identify
xlink_relocate
xlink_ping
xlink_reset
xlink_ready
xlink_load
xlink_save
xlink_peek
xlink_poke
xlÃink_fil
xlink_jump
xlink_run
EOF

lib /MACHINE:X86 /def:xlink.def /out:xlink.lib && \
    rm xlink.def xlink.exp
