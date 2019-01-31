make && bin/mklinjat --solve="$(grep -F ' 4  2  3 ' puzzledb/h=13_w=9_p=26_cv=1_cf=3_sq=50_dep=100_of=60 | perl -MJSON -ane 'print join ",", "", @{(decode_json $_)->{puzzle}}')" | less

