.ORIG x3000

    BRn end
    LEA R0, HW
    PUTS
end
    LEA R0, GW
    PUTS

    HALT

HW  .STRINGZ "Hello World\n"
GW  .STRINGZ "Goodbye World\n"

.END

