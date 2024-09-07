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

Main
    ld r1, NegAscii
    in
    add r1, r1, r0      ; read single-digit integer value

    jsr CountOne        ; bit-1 counting subroutine
    jsr Output          ; result display subroutine

    BR Main             ; do while (true)

    HALT

NegAscii .fill #-48     ; -'0'
Ascii    .fill #48      ; '0'
Saver7   .fill #0
Counter  .fill #16      ; decrements
Mask     .fill x8000    ; first bit 1, rest 0

; r0    mask, temporary values for trap
; r1    input which gets lshifted, later constant 0x30
; r2    loop counter (decrementing)
; r3    temporary variable for brz
; r4    count of 1-bits
; r7    return value -- saved to saver7

CountOne
    st r7, Saver7       ; [Saver7] = r7     ; save return value
    ld r2, Counter      ; r2 = [counter]
    ld r0, Mask         ; r0 = [mask]
    and r4, r4, #0      ; r4 = 0

CountLoop               ; for (r2 = [counter]; r2 > 0; --r2) {
    and r3, r0, r1      ;     r3 = [mask] & r1
    BRz NoOne          ;     if (r3 != 0) {    ; if first bit is set
        add r4, r4, #1  ;         r4 += 1       ; increment count of 1-bits
                        ;     }
NoOne
    add r1, r1, r1      ;     r1 <<= 2          ; r1 += r1

                        ;     // loop operations
    add r2, r2, #-1     ;     r2 -= 1
    BRp CountLoop      ;     if (r2 > 0) goto count_loop
                        ; }
    ld r7, Saver7       ; r7 = [saver7]
    ret                 ; return r7

Output
    st r7, Saver7       ; [Saver7] = r7     ; save return value
                        ;                   ^ this seems unnecessary
    ld r1, Ascii        ; r1 = 0x30
    add r0, r4, r1      ; r0 = r4 + 0x30
    out                 ; print(ascii(r4))  ; print(r0)

    ld r7, Saver7       ; r7 = [Saver7]
    RET                 ; return r7

.END

