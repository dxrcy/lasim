.ORIG x3000

    LD R0, HW
    OUT
    ADD R1, R0, x2
    ST R1, HW
    LD R0, HW
    OUT

    HALT

HW  .FILL x6261
    .FILL x6463
    .FILL x0000

.END

