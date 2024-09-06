; input two one-digit numbers and print out the sum.
; Correct only if the sum is less than 10
    .ORIG x3000

    LD R3, nThirty      ; load constant x-30
    LEA R0, Prompt      ; print prompt to enter number
    PUTS
    GETC                ; get first number
    OUT                 ; echo it
    ADD R0, R0, R3      ; convert ASCII to value
    ADD R1, R0, #0      ; save first number in R1

    LD R0, CR           ; print Return
    OUT

    LEA R0, Prompt      ; print prompt to enter number
    PUTS
    GETC                ; get second number
    OUT                 ; echo it
    ADD R0, R0, R3      ; convert ASCII to value
    ADD R2, R0, #0      ; save second number in R2

    ; ADD R0, R1, #30
    ; OUT
    ; ADD R0, R2, #30
    ; OUT

    ADD R2, R1, R2      ; add R2 <- R1+R2

    LD R0, CR           ; print Return
    OUT

    LEA R0, Sum
    PUTS
    JSR Convert         ; call function to convert number to ASCII
    OUT

    LD R0, CR           ; print Return
    OUT

    HALT                ; end of main program

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; subroutine to convert number in R2 to ASCII
; need to add x30
Convert
    LD R0, Thirty
    ADD R0, R2, R0
    RET

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; start of constants
nThirty .FILL x-30
Thirty  .FILL x30
CR      .FILL x0D
Prompt  .STRINGZ "Enter a number? "
Sum     .STRINGZ "The sum is "

    .END
