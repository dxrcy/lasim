.ORIG x3000
    lea r1, A
    REG
A
    lea r1, A
    REG

    ld r1, X
    REG

    not r1, r1
    st r1, X
    REG

    ld r1, X
    REG

    ld r1, InstrValue
    REG
    st r1, InstrLocation

InstrLocation
    .FILL 0xd000
    REG

    ld r1, Addr
    REG
    and r2, r2, #0
    add r2, r2, x000e
    str r2, r1, #0

    ld r1, Addr
    REG
    ldr r1, r1, #0
    REG

    and r1, r1, #0
    REG
    ldi r1, Addr
    REG

    add r1, r1, #-5
    REG
    sti r1, Addr

    and r1, r1, #0
    REG
    ldi r1, Addr
    REG

    HALT

X           .FILL xbeef
InstrValue  .FILL x1265 ; add r1, r1, #5
Addr        .FILL x3100

.END
