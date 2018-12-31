#!/bin/bash

mkdir -p puzzledb

gen() {
    H=$1
    W=$2
    P=$3
    N=$4
    FL=$5
    F="puzzledb/h=${H}_w=${W}_p=${P}"$(echo "$FL" | sed 's/ //g')
    if [ ! -f $F ]; then
        echo Generating $F
        MAP_HEIGHT=$H MAP_WIDTH=$W PIECES=$P cmake .;
        make && \
            (seq 90210 90219 | parallel --will-cite --line-buffer bin/mklinjat --puzzle_count=$(($N/10)) --seed={} $FL) > puzzledb/tmp && \
            mv puzzledb/tmp $F
    fi
}

gen 10 7 15 100
gen 10 7 16 100
gen 10 7 17 100
gen 10 7 18 100
gen 10 7 19 100
gen 10 7 20 100

gen 11 8 19 200 "--disallow_basic"
gen 11 8 20 200 "--disallow_basic"
gen 11 8 21 200 "--disallow_basic"
gen 11 8 22 200 "--disallow_basic"

gen 13 9 23 200 "--disallow_basic --disallow_only_square"
gen 13 9 24 200 "--disallow_basic --disallow_only_square"
gen 13 9 25 200 "--disallow_basic --disallow_only_square"
gen 13 9 26 200 "--disallow_basic --disallow_only_square"

