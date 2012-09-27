#!/bin/sh

for i in `ls *.png`
do
	convert -crop 144x169+56+87 $i $i
done
