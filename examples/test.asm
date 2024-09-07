.ORIG x3010

    ld r0, Newline      ;
    out                 ; print('\n')

    ld r2, Init         ; r2 = 0x3000
    ld r3, Counter      ; r3 = #16
    ld r6, Sto          ; r6 = 0x3200

    jsr Lookup          ; Lookup()

    ld r1, Ascii        ; r1 = 0x30
    and r2, r2, #0      ; r2 = 0
    add r3, r0, #0      ; r3 = r0

OutLoop                 ; do {
    add r2, r2, #1      ;     r2 += 1
    add r3, r3, #-10    ;     r3 -= 10
    BRzp OutLoop        ; } while (r3 >= 0);

    add r2, r2, #-1     ; r2 -= 1
    add r3, r3, #10     ; r3 += 10

    add r0, r1, r2      ;
    out                 ; print(r1 + r2)
    add r0, r1, r3      ;
    out                 ; print(r1 + r3)

    HALT

Init    .FILL x3000     ; int *
Sto     .FILL x3200     ; int *
Counter .FILL #16       ; int
Saver7  .BLKW 0         ; ???
Newline .FILL 0x0a      ; char Newline = '\n'
Ascii   .FILL 0x30      ; char Ascii   = '0'

Lookup                  ; fn Lookup(int *r2, r3, int *r6) {
    st r7, Saver7       ;
    and r5, r5, #0      ;     let r5 = 0

EvenLoop                ;     for ( ; r3 > 0; --r3) {
    ldr r1, r2, #0      ;         int r1 = *r2
    and r4, r1, #1      ;

    BRp Update          ;         if (r1 % 2 == 0) {
    str r1, r6, #0      ;             *r6 = r1
    add r5, r5, #1      ;             ++r5
    add r6, r6, #1      ;             ++r6
                        ;         }
Update
    add r2, r2, #1      ;         ++r2
    add r3, r3, #-1     ;
    BRp EvenLoop        ;     }

    ld r6, Sto          ;     r6 = Sto
    and r2, r2, #0      ;     r2 = 0
    ldr r0, r6, #0      ;     int r0 = *r6

                        ;     // Always runs at least once tho
MaxLoop                 ;     for ( ; r5 > 0; --r5) {
    ldr r1, r6, #0      ;         let r1 = [r6]
    not r3, r1          ;
    add r3, r3, #1      ;
    add r3, r3, r0      ;         // r3 = r0 - r1

    BRzp Date           ;         if (r0 < r1 ) {
    add r0, r1, #0      ;             r0 = r1
                        ;         }
Date
    add r6, r6, #1      ;         ++r6
    add r5, r5, #-1     ;
    BRp MaxLoop         ;     }

    ld r7, Saver7       ;
    RET                 ;
                        ; }

.END
