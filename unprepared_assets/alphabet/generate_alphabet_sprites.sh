#/bin/bash

#typo=/home/pierre-eric/AmericanTypewriter-Condensed.ttf
typo=/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed.ttf
size=60
suffix="_typo.png"
function generate_sprite
{
	convert -background transparent -fill white -font ${typo} -pointsize ${size} label:${1} PNG32:${2}${suffix}
    #convert -resize 70%x100% ${2}${suffix} PNG32:${2}${suffix}
}

echo "Generating A->Z"
for i in {A..Z}; 
do
    dec=`printf "%x" "'$i"`
    generate_sprite ${i} $dec
done

echo "Generating a->z"
for i in {a..z};
do
    dec=`printf "%x" "'$i"`
    generate_sprite ${i} $dec
done

echo "Generating 0->9"
for i in {0..9};
do
    dec=`printf "%x" "'$i"`
    generate_sprite ${i} $dec
done

echo "Generating punctuation"
for i in \! \? \# \' \( \) \, \- \. \; \: \= \¿;
do
    dec=`printf "%x" "'$i"`
    generate_sprite $i $dec
done

echo "Generating accentued letters "
for i in à é è ê î ç ù À É È Ê Î Ç Ù;
do
    dec=`printf "%x" "'$i"`
    generate_sprite $i $dec
done

