; LC-3 Assembly Program: Count number of bit-1 with User Input from 0-9
; Author: Minh Nguyen Nhat Tuan
; Date: 02/01/2023
; https://github.com/Nguyen-Nhat-Tuan-Minh/LC_3-Assembly-Program

; Description:
; This LC-3 assembly program demonstrates the counting of bit-1 from User Input in true binary number.
; The program converts ASCII input to binary output number
; Counting algorithm: Shift right and check if negative number (leading 1).

; How to Use:
; 1. Run the program on an LC-3 simulator.
; 2. When prompted, enter from keyboard between 0-9.
; 3. The results will be displayed on the monitor.

.ORIG x3000

MAIN
    LD R1, NEGASCII
    IN
    ADD R1, R1, R0      ; Read single-digit integer value

    JSR COUNT_ONE       ; Bit-1 counting subroutine
    JSR OUTPUT          ; Result display subroutine

    BR MAIN             ; do while (true)

    HALT

NEGASCII .FILL #-48     ; -'0'
ASCII    .FILL #48      ; '0'
SAVER7   .FILL #0
COUNTER  .FILL #16      ; Decrements
MASK     .FILL x8000    ; First bit 1, rest 0

; R0    MASK, temporary values for TRAP
; R1    input which gets lshifted, later constant 0x30
; R2    loop counter (decrementing)
; R3    temporary variable for BRz
; R4    count of 1-bits
; R7    return value -- saved to SAVER7

COUNT_ONE
    ST R7, SAVER7       ; [saver7] = R7     ; Save return value
    LD R2, COUNTER      ; R2 = [counter]
    LD R0, MASK         ; R0 = [mask]
    AND R4, R4, #0      ; R4 = 0

COUNT_LOOP              ; for (R2 = [counter]; R2 > 0; --R2) {
    AND R3, R0, R1      ;     R3 = [mask] & R1
    BRz NO_ONE          ;     if (R3 != 0) {    ; If first bit is set
        ADD R4, R4, #1  ;         R4 += 1       ; Increment count of 1-bits
                        ;     }
NO_ONE
    ADD R1, R1, R1      ;     R1 <<= 2          ; R1 += R1

                        ;     // Loop operations
    ADD R2, R2, #-1     ;     R2 -= 1
    BRp COUNT_LOOP      ;     if (R2 > 0) goto COUNT_LOOP
                        ; }
    LD R7, SAVER7       ; R7 = [saver7]
    RET                 ; return R7

OUTPUT
    ST R7, SAVER7       ; [saver7] = R7     ; Save return value
                        ;                   ^ This seems unnecessary
    LD R1, ASCII        ; R1 = 0x30
    ADD R0, R4, R1      ; R0 = R4 + 0x30
    OUT                 ; print(ascii(R4))  ; print(R0)

    LD R7, SAVER7       ; R7 = [saver7]
    RET                 ; return R7

.END

