#!/bin/sh

for i in `ls *.png`
do
	 newname=`echo $i | sed 's/l2r/r2l/'`
	convert +repage $i PNG32:$i
done
