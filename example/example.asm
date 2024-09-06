    .ORIG x3000
    LEA R0, HW
    ADD R1, R0, x10
    PUTS
    HALT

HW  .STRINGZ "Hello World\n"

    .END

