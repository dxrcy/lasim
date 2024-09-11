#!/bin/sh

examples='./examples'

alias lc3='./lc3'
alias laser='laser'

echo '------'
for asm in "$examples"/*.asm; do
    filename="$(basename "${asm%%.asm}")"
    obj_actual="$examples/$filename.a.obj"

    laser -a "$asm" >/dev/null || break

    printf 'ASSEMBLE    %-18s' "$filename"
    lc3 -a "$asm" -o "$obj_actual"
    if [ "$?" -eq 0 ];
        then echo 'pass'
        else echo 'FAIL'
    fi
done

echo '------'
for obj_actual in "$examples"/*.a.obj; do
    filename="$(basename "${obj_actual%%.a.obj}")"
    obj_expected="$examples/$filename.obj"

    printf 'CHECK       %-18s' "$filename"

    diff "$obj_expected" "$obj_actual" >/dev/null
    if [ "$?" -eq 0 ];
        then echo 'pass'
        else echo 'FAIL'
    fi
done
#
