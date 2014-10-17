#!/bin/bash

cat <<EOF
SUBSYSTEMS=="usb", ATTRS{idVendor}=="${USB_VID:2}", ATTRS{idProduct}=="${USB_PID:2}", MODE:="0666", SYMLINK+="xlink"

SUBSYSTEMS=="usb", ATTR{idVendor}=="03eb", ATTR{idProduct}=="2ffa", SYMLINK+="dfu", MODE:="666"
EOF
