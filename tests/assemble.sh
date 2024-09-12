#!/bin/sh

examples='./examples'

alias lasim='./lasim'
alias laser='laser'

echo '------'
for asm in "$examples"/*.asm; do
    filename="$(basename "${asm%%.asm}")"
    obj_actual="$examples/$filename.a.obj"

    printf 'ASSEMBLE    %-18s' "$filename"

    laser -a "$asm" >/dev/null
    if [ "$?" -ne 0 ]; then
        echo 'Bad assembly file. Skipping.'
        continue
    fi

    lasim -a "$asm" -o "$obj_actual"
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

    if [ ! -f "$obj_expected" ]; then
        echo 'No object file. Skipping.'
        continue
    fi

    diff "$obj_expected" "$obj_actual" >/dev/null
    if [ "$?" -eq 0 ];
        then echo 'pass'
        else echo 'FAIL'
    fi
done
#
