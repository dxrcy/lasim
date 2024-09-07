; LC-3 Assembly Program: Division with User Input
; Author: Minh Nguyen Nhat Tuan
; Date: 02/01/2024
; https://github.com/Nguyen-Nhat-Tuan-Minh/LC_3-Assembly-Program

; Description:
; This LC-3 assembly program demonstrates a simple division operation where the user
; is prompted to input a 5-digit number

; How to Use:
; 1. Run the program on an LC-3 simulator.
; 2. When prompted, enter 5 digits, then the input terminate.

.ORIG X3000

    ld r2, Init
    ld r1, Count
    ld r3, NegAscii

LoopInput
    getc
    out
    
    add r0, r0, r3
    str r0, r2, #0
    add r2, r2, #1
    add r1, r1, #-1
    BRp LoopInput

    ld r2, Init
    and r0, r0, #0
    ldr r3, r2, #0
    add r2, r2, #1
    ld r1, LThousands

Loop1
    add r0, r0, r3
    add r1, r1, #-1
    BRp Loop1

    sti r0, Save1
    ldr r3, r2, #0
    add r2, r2, #1 
    ld r1, Thousands

Loop2
    add r0, r0, r3
    add r1, r1, #-1
    BRp Loop2

    sti r0, Save2
    ldr r3, r2, #0
    add r2, r2, #1 
    ld r1, Hundreds

Loop3
    add r0, r0, r3
    add r1, r1, #-1
    BRp Loop3

    sti r0, Save3
    ldr r3, r2, #0
    add r2, r2, #1 
    ld r1, Tens

Loop4
    add r0, r0, r3
    add r1, r1, #-1
    BRp Loop4

    sti r0, Save4
    ldr r3, r2, #0
    add r2, r2, #1 

    add r0, r0, r3
    sti r0, Save5

    HALT

NegAscii    .FILL #-48
Init        .FILL x4000
Count       .FILL #5
Tens        .FILL #10
Hundreds    .FILL #100
Thousands   .FILL #1000
LThousands  .FILL #10000
Save1       .FILL x4005
Save2       .FILL x4006
Save3       .FILL x4007
Save4       .FILL x4008
Save5       .FILL x4009

.END

