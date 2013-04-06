#!/bin/sh

folders="decor alphabet fumee logo dummy arbre"

for folder in $folders
do
    echo "Building '$folder' atlas"
    ../sac/tools/generate_atlas.sh $folder
done

echo "Removing unneeded alphabet color"
rm ../assets/alphabet.p*
rm ../assetspc/alphabet.p*
