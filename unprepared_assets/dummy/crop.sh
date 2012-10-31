#!/bin/sh

for i in `ls jump*.png`
do
	convert -crop 120x135+0+0 $i $i
done


