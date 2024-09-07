.ORIG x3000

    ld r1, Height
Rows

    ld r2, Width
Columns

    add r3, r1, r2
    and r3, r3, x1
    BRnp CharElse

    ld r0, Solid1
    out
    ld r0, Solid2
    out
    BR CharEnd

CharElse
    ld r0, Empty
    out
    out

CharEnd

    add r2, r2, #-1
    BRp Columns

    ld r0, Newline
    out

    add r1, r1, #-1
    BRp Rows

    HALT

Newline .FILL x0a   ; '\n'

Width   .FILL #10
Height  .FILL #10

Solid1  .FILL x5b   ; '['
Solid2  .FILL x5d   ; ']'
Empty   .FILL x20   ; ' '

.END
