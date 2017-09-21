#!/bin/bash

# Args: <build dir> <dest img file>

bdir=$1
dest=$1/$2

if [ ! -f $1 ]; then
   # If the file does not already exist
   dd status=none if=/dev/zero of=$dest bs=1M count=16
   mformat -i $dest -T 32256 -h 16 -s 63 ::
   mlabel -i $dest ::EXOS
fi

rm -rf $bdir/sysroot
cp -r $bdir/../sysroot $bdir/

cd $bdir/sysroot

# hard-link init to sysroot/sbin
ln $bdir/init sbin/

# first, create the directories
for f in $(find * -type d); do
   mmd -i $dest $f
done

# then, copy all the files in sysroot
for f in $(find * -type f); do
   mcopy -i $dest $f ::/$f
done
