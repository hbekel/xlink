#!/bin/bash

cat <<EOF
SUBSYSTEMS=="usb", ATTRS{idVendor}=="1d50", ATTRS{idProduct}=="60c8", MODE:="0666", SYMLINK+="xlink"

SUBSYSTEMS=="usb", ATTR{idVendor}=="03eb", ATTR{idProduct}=="2ffa", SYMLINK+="dfu", MODE:="666"
EOF
