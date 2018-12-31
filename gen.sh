mkdir -p puzzledb

gen() {
    H=$1
    W=$2
    P=$3
    N=$4
    F="puzzledb/h=${H}_w=${W}_p=${P}"
    if [ ! -f $F ]; then
        echo Generating $F
        MAP_HEIGHT=$H MAP_WIDTH=$W PIECES=$P cmake .;
        make && \
            bin/mklinjat 98098 $N > puzzledb/tmp && \
            mv puzzledb/tmp $F
    fi
}

gen 10 7 15 100
gen 10 7 16 100
gen 10 7 17 100
gen 10 7 18 100
gen 10 7 19 100
gen 10 7 20 100
