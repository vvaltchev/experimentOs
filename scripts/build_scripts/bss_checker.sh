#!/bin/sh

have_bss=`readelf -S $1 | grep bss`

if [ -z "$have_bss" ]; then
   exit 0
fi

echo
echo '**** ERROR: the kernel has contents in BSS! ****'
echo

exit 1
