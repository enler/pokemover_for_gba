#!/bin/bash

file_path=$1
offset=$(( 0x$2 - 0x2000000 ))
file_size=$(stat -c %s "$file_path")
file_size=$(( $file_size + 0x2000000 ))
value=$(printf "%08x" $file_size)

echo -n -e "\\x${value:6:2}\\x${value:4:2}\\x${value:2:2}\\x${value:0:2}" | dd of="$file_path" bs=1 seek=$offset count=4 conv=notrunc
