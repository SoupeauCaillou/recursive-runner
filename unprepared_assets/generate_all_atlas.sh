#!/bin/sh

folders="decor alphabet fumee logo dummy arbre"

for folder in $folders
do
    echo "Building '$folder' atlas"
    ./generate_atlas.sh $folder
done

echo "Removing unneeded alphabet color"
rm ../assets/alphabet.p*
rm ../assetspc/alphabet.p*
