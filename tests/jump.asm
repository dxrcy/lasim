.ORIG x3000

    and r1, r1, #0
    REG

    lea r1, X
    JMP r1

    and r1, r1, #0
    REG

X
    and r1, r1, #0
    add r1, r1, x0e
    REG

    JSR Foo

    and r1, r1, #0
    REG

    lea r1, Foo
    JSRR r1

    and r1, r1, #0
    REG

    HALT

Foo
    and r1, r1, #0
    add r1, r1, x03
    REG
    RET

.END
