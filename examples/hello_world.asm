.ORIG x3000

    LEA R0, HELLO_WORLD
    ADD R0, R0, x12
    AND R0, R0, #12
    ; PUTS

    HALT

HELLO_WORLD
    .STRINGZ "Hello world!\n"

.END
