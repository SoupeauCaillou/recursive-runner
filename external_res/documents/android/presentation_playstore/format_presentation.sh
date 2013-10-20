#!/bin/bash


cd $(dirname $0)

if [ $# = 0 ]; then
    echo -e "No arg provided. Formatting every languages\n"
    sleep 1
    files=*.txt
else
    files=$@
fi

declare -A semantic
#add there your specific presentation format
semantic[1]=""
semantic[2]="\n"
semantic[3]="\n"
semantic[4]=""
semantic[5]="\n"
semantic[6]=""
semantic[7]=""
semantic[8]="\n"
semantic[9]="\n"
semantic[10]="\n*** "
semantic[11]="***\n"


function parse_file {
    c=1
    while read line; do
        echo -ne "$line ${semantic[$c]}"
        c=$(($c + 1))
    done < $file
    echo -ne "$line ${semantic[$c]}"
}



for file in $files; do
    echo -e "##########################Language: $file####################"
    parse_file $file
    echo -e "##########################End language: $file#############"
done
