#!/bin/sh

source "$(dirname $0)/shared.sh"

echo '------'
for asm in "$examples"/*.asm; do
    filename="$(basename "${asm%%.asm}")"
    obj_actual="$examples/$filename.a.obj"

    printf 'ASSEMBLE    %-18s' "$filename"

    lc3as "$asm" >/dev/null
    if [ $? -ne 0 ]; then
        echo 'Bad assembly file. Skipping.'
        continue
    fi

    lasim -a "$asm" -o "$obj_actual"
    report_status $?
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
    report_status $?
done

