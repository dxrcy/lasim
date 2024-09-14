.ORIG x3000

    lea r0, InputPrompt
    puts

    getc

    ld r1, NegAsciiZero
    add r1, r0, r1

    add r2, r2, #0
    add r3, r3, #1

FibLoop
    add r4, r2, r3
    add r3, r2, #0
    add r2, r4, #0

    add r1, r1, #-1
    BRp FibLoop

    ; TODO: Print output

    HALT

InputPrompt     .STRINGZ "Input a number: "
NegAsciiZero    .FILL x-30
AddrResult      .FILL x3100

.END
