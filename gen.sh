#!/bin/bash

mkdir -p puzzledb

gen() {
    H=$1
    W=$2
    P=$3
    N=$4
    CV=$5
    CF=$6
    SQ=$7
    DEP=$8
    MW=$9
    F="puzzledb/h=${H}_w=${W}_p=${P}_cv=${CV}_cf=${CF}_sq=${SQ}_dep=${DEP}"
    if [ ! -f $F ]; then
        echo Generating $F
        MAP_HEIGHT=$H MAP_WIDTH=$W PIECES=$P cmake .;
        make && \
            (seq 90210 90219 | parallel --will-cite --line-buffer bin/mklinjat --puzzle_count=$(($N/10)) --seed={} --score_cover=$CV --score_cant_fit=$CF --score_square=$SQ --score_dep=$DEP --score_max_width=$MW) > puzzledb/tmp && \
            mv puzzledb/tmp $F
    fi
}

gen 9 6 7 100 1 1 0 0 -1
gen 9 6 8 100 1 1 0 0 -1
gen 9 6 9 100 1 1 0 0 -1
gen 9 6 10 100 1 1 0 0 -1

gen 9 6 7 100 1 3 0 0 -1
gen 9 6 8 100 1 3 0 0 -1
gen 9 6 9 100 1 3 0 0 -1
gen 9 6 10 100 1 3 0 0 -1

gen 10 7 15 100 1 3 0 0 -1
gen 10 7 16 100 1 3 0 0 -1
gen 10 7 17 100 1 3 0 0 -1
gen 10 7 18 100 1 3 0 0 -1
gen 10 7 19 100 1 3 0 0 -1
gen 10 7 20 100 1 3 0 0 -1

gen 11 8 19 200 1 3 50 0 -2
gen 11 8 20 200 1 3 50 0 -2
gen 11 8 21 200 1 3 50 0 -2
gen 11 8 22 200 1 3 50 0 -2

gen 13 9 23 200 1 3 50 100 -2
gen 13 9 24 200 1 3 50 100 -2
gen 13 9 25 200 1 3 50 100 -2
gen 13 9 26 200 1 3 50 100 -2

