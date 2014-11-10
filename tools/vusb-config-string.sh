#!/bin/bash

STRING="$1"

for (( i=0; i<${#STRING}; i++ )); do
    echo -n "'${STRING:$i:1}',"   
done
