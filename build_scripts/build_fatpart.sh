#!/bin/sh

# Args: <build dir> <dest img file>

bdir=$1
dest=$1/$2

if [ ! -f $1 ]; then
	# If the file does not already exist
	dd status=none if=/dev/zero of=$dest bs=1M count=16
	mformat -i $dest -T 32256 -h 16 -s 63 ::
fi

mcopy -i $dest $bdir/init ::/

