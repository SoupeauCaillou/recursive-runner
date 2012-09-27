cd $1
texture_packer *png | ../../sac/tools/texture_packer/texture_packer.sh $1
cd -
convert /tmp/$1_alpha.png -alpha extract -depth 8 ../assets/$1_alpha.png
convert ../assets/$1_alpha.png -background white -flatten +matte -depth 8 ../assets/$1_alpha.png
convert /tmp/$1.png -background white -alpha remove -type TrueColor PNG24:../assets/$1.png
#~/perso/PVRTexTool/PVRTexToolCL/Linux_x86_64/PVRTexTool -f OGLPVRTC4 -yflip0 -i ../../assets/$1.png -p -pvrlegacy -o ../../assets/$1.pvr
cp -v /tmp/$1.desc ../assets/
#~/perso/android-sdk-linux/tools/etc1tool --encode ../../assets/$1.png -o ../../assets/$1.pkm

