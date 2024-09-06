    .ORIG x3000
    LEA R0, HW
    PUTS
    ADD R1, R0, x10
    HALT

HW  .STRINGZ "Hello World\n"

    .END

