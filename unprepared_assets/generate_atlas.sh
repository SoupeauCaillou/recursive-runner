#!/bin/sh

# path need : 
# location of texture_packer (your_build_dir/sac/build/cmake)
# location of PVRTexToolCL (need to install it from http://www.imgtec.com/powervr/insider/powervr-pvrtextool.asp)
# location of etc1tool (android-sdk-linux/tools)


if [ $# != 1 ]; then
	echo "need the image directory"
	exit
fi

rm -r /tmp/$1
mkdir /tmp/$1
for file in `ls $1/*png`
do
    used_rect=`../sac/tools/texture_packer/tiniest_rectangle.py ${file}`
    ww=`echo $used_rect | cut -d, -f1`
    hh=`echo $used_rect | cut -d, -f2`
    xx=`echo $used_rect | cut -d, -f3`
    yy=`echo $used_rect | cut -d, -f4`
    convert -crop ${ww}x${hh}+${xx}+${yy} ${file} PNG32:/tmp/$1/`basename ${file}`
done

cd $1
texture_packer /tmp/$1/*png | ../../sac/tools/texture_packer/texture_packer.sh $1
cd -
convert /tmp/$1.png -alpha extract -depth 8 ../assets/$1_alpha.png
# mais pourquoi j'ai fait ca ?convert ../assets/$1_alpha.png -background white -flatten +matte -depth 8 ../assets/$1_alpha.png
echo "Create $1.png"
convert /tmp/$1.png -background white -alpha off -type TrueColor PNG24:../assetspc/$1.png
echo "Create $1.pvr.*"
PVRTexToolCL -f OGLPVRTC4 -yflip0 -i ../assetspc/$1.png -p -pvrlegacy -m -o /tmp/$1.pvr
split -d -b 1024K /tmp/$1.pvr $1.pvr.
mv $1.pvr.0* ../assets/

echo "Create $1.pkm.*"
PVRTexToolCL -f ETC -yflip0 -i ../assetspc/$1.png -q 3 -m -pvrlegacy -o /tmp/$1.pkm
#etc1tool --encode ../assets/$1.png -o /tmp/$1.pkm
# PVRTexToolCL ignore name extension
split -d -b 1024K /tmp/$1.pvr $1.pkm.
mv $1.pkm.0* ../assets/


cp -v /tmp/$1.desc ../assets/
