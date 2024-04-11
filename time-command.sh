#!/bin/bash

# runs command, prints time it took in ms
ts=$(date +%s%N)
$@
echo $((($(date +%s%N) - $ts)/1000000))