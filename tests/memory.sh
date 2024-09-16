#!/bin/sh

source "$(dirname $0)/shared.sh"

asm_file="$tests/memory.asm"
obj_file="$out/memory.obj"
output_actual_file="$out/memory.actual"
output_expected_file="$tests/memory.expected"

extract_reg() {
    sed -n 's/ *. *r1 *\([^ ]*\).*/\1/p'
}

lasim -a "$asm_file" -o "$obj_file"
lasim -x "$obj_file" | extract_reg > "$output_actual_file"

diff "$output_expected_file" "$output_actual_file"
report_status $?


