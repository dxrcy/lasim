    .ORIG x3000
    LEA R0, HW
    PUTS
    NOT R0, R0
    ADD R0, R0, x10
    ADD R1, R1, x0
    HALT

HW  .STRINGZ "Hello World\n"

    .END

