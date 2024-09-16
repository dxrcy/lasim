#!/bin/sh

source "$(dirname $0)/shared.sh"

[ -d "$out/examples" ] || mkdir "$out/examples"
rm -f "$out/examples/"*
for asm in "$examples/"*.asm; do
    cp "$asm" -t "$out/examples"
done

echo '------'
for asm in "$out/examples"/*.asm; do
    filename="$(basename "${asm%%.asm}")"
    obj_actual="$out/examples/$filename.actual.obj"
    obj_default="$out/examples/$filename.obj"
    obj_expected="$out/examples/$filename.expected.obj"

    printf 'ASSEMBLE    %-18s' "$filename"

    lc3as "$asm" >/dev/null
    mv -T "$obj_default" "$obj_expected"

    if [ $? -ne 0 ]; then
        echo 'Bad assembly file. Skipping.'
        continue
    fi

    lasim -a "$asm" -o "$obj_actual"
    report_status $?
done

echo '------'
for obj_actual in "$out/examples"/*.actual.obj; do
    filename="$(basename "${obj_actual%%.actual.obj}")"
    obj_expected="$out/examples/$filename.expected.obj"

    printf 'CHECK       %-18s' "$filename"

    if [ ! -f "$obj_expected" ]; then
        echo 'No object file. Skipping.'
        continue
    fi

    diff "$obj_expected" "$obj_actual" >/dev/null
    report_status $?
done

