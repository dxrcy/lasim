.ORIG x3000

    and r1, r1, x0
    REG

    add r1, r1, x1
    REG

    ld r0, Hex0010
    add r1, r1, r0
    REG

    ld r0, Dec10
    add r1, r1, r0
    REG

    and r1, r1, x1
    REG
    and r1, r1, x1
    REG

    not r1, r1
    REG

    and r1, r1, x0
    REG
    and r1, r1, x0
    REG

    ld r0, Hexd69b
    add r1, r1, r0
    REG

    not r1, r1
    REG
    not r1, r1
    REG

    ld r0, Hex2d5a
    and r1, r1, r0
    REG

    HALT

Dec10   .FILL #10
Hex0010 .FILL x10
Hexd69b .FILL xd69b
Hex2d5a .FILL x2d5a

.END
