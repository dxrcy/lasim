#!/bin/bash

source "$(dirname $0)/shared.sh"

asm_file="$out/branch.asm"
obj_file="$out/branch.obj"
output_actual_file="$out/branch.actual"
output_expected_file="$tests/branch.expected"

ccs=('' 'n' 'z' 'p' 'nz' 'zp' 'np' 'nzp')
reg='r1'

cat >> "$asm_file" << EOF
.ORIG x3000

EOF

for cc in "${ccs[@]}"; do
    inputs=('n' 'z' 'p')
    for input in "${inputs[@]}"; do
        if   [ "$input" = 'n' ]; then value=-1
        elif [ "$input" = 'z' ]; then value=0
        elif [ "$input" = 'p' ]; then value=1
        else exit 1; fi
        name="${cc}_$input"
        cat >> "$asm_file" << EOF
    ; Branch <$cc> if $value ($input)
    and $reg, $reg #0
    add $reg, $reg #$value
    lea r0, Msg_$name
    puts
    BR${cc} Branch_$name
    lea r0, MsgNoBranch
    puts
Branch_$name

EOF
    done
done

cat >> "$asm_file" << EOF
    HALT

MsgNoBranch .STRINGZ "nobranch\n"

EOF

for cc in "${ccs[@]}"; do
    cat >> "$asm_file" << EOF
Msg_${cc}_n .STRINGZ "$cc:n\n"
Msg_${cc}_z .STRINGZ "$cc:z\n"
Msg_${cc}_p .STRINGZ "$cc:p\n"

EOF
done

cat >> "$asm_file" << EOF

.END
EOF

lasim -a "$asm_file" -o "$obj_file" || exit $?
lasim -x "$obj_file" > "$output_actual_file" || exit $?

diff "$output_expected_file" "$output_actual_file"
report_status $?

