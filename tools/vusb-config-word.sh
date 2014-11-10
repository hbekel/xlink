#!/bin/bash

WORD="$1"
echo -n "0x${WORD:4:2},${WORD:0:4}"
