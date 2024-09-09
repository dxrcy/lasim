.ORIG x3000

    LEA R0, HELLO_WORLD
    ADD R0, R0, #123
    PUTS

    HALT

HELLO_WORLD
    .STRINGZ "Hello world!\n"

.END
