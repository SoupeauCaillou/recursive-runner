#!/bin/sh

# path need : 
# location of texture_packer (your_build_dir/sac/build/cmake)
# location of PVRTexToolCL (need to install it from http://www.imgtec.com/powervr/insider/powervr-pvrtextool.asp)
# location of etc1tool (android-sdk-linux/tools)


if [ $# != 1 ]; then
	echo "need the image directory"
	exit
fi

cd $1
texture_packer *png | ../../sac/tools/texture_packer/texture_packer.sh $1
cd -
convert /tmp/$1.png -alpha extract -depth 8 ../assets/$1_alpha.png
# mais pourquoi j'ai fait ca ?convert ../assets/$1_alpha.png -background white -flatten +matte -depth 8 ../assets/$1_alpha.png
convert /tmp/$1.png -background white -alpha off -type TrueColor PNG24:../assets/$1.png

PVRTexToolCL -f OGLPVRTC4 -yflip0 -i ../assets/$1.png -p -pvrlegacy -m -o ../assets/$1.pvr
cp -v /tmp/$1.desc ../assets/
etc1tool --encode ../assets/$1.png -o ../assets/$1.pkm

