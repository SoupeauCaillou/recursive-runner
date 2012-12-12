#/bin/bash

typo=/home/pierre-eric/AmericanTypewriter-Condensed.ttf
size=60
suffix="_typo.png"
function generate_sprite
{
	convert -background transparent -fill white -font ${typo} -pointsize ${size} label:${1} PNG32:${2}${suffix}
    #convert -resize 70%x100% ${2}${suffix} PNG32:${2}${suffix}
}

function char_to_dec
{
    val=`printf "%d" "'$1"`

    if [ $val -le 191 ]
    then
        return $val
    else #if [ $val -le 191 ]
        val=`expr $val - 192`
        val=`expr $val + 128`
        return $val
    fi
}


echo "Generating A->Z"
base=65
for i in {A..Z}; 
do 
    generate_sprite ${i} ${base}
    base=`expr $base + 1`
done

echo "Generating a->z"
base=97
for i in {a..z}; 
do 
    generate_sprite ${i} ${base}
    base=`expr $base + 1`
done

echo "Generating 0->9"
base=48
for i in {0..9};
do 
	generate_sprite ${i} ${base}
    base=`expr $base + 1`
done

echo "Generating punctuation"
for i in \! \? \# \' \( \) \, \- \. \; \: \=;
do
    char_to_dec $i
    dec=$?
    generate_sprite $i $dec
done

echo "Generating accentued letters "
for i in à é è ê î ç ù À É È Ê Î Ç Ù;
do
    char_to_dec $i
    dec=$?
    generate_sprite $i $dec
done

