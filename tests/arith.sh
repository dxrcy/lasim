#!/bin/sh

tests_dir="$(dirname $0)"
out_dir="$tests_dir/out"

lasim() {
    "$tests_dir/../lasim" $@ || exit $?;
}
alias diff='delta'

asm_file="$tests_dir/arith.asm"
obj_file="$out_dir/arith.obj"
output_actual_file="$out_dir/arith.actual"
output_expected_file="$tests_dir/arith.expected"

extract_reg() {
    sed -n 's/ *. *r1 *\([^ ]*\).*/\1/p'
}

lasim -a "$asm_file" -o "$obj_file" || exit $?
./lasim -x "$obj_file" | extract_reg > "$output_actual_file"

diff "$output_expected_file" "$output_actual_file"
if [ $? -eq 0 ];
    then printf '\x1b[32mpass\x1b[0m\n'
    else printf '\x1b[31mFAIL\x1b[0m\n'
fi

