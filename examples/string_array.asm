.ORIG x3000

    lea r0, Words       ; r0 = &words
Loop
    puts                ; print(r0)
    add r0, r0, x8      ; r0 += 8 WORDS
    ldr r1, r0, x0      ; r2 = [r0]
    BRnp Loop           ; if (r2 != 0) goto Loop

    halt

Words  ; Each [word + null block] takes up 8 WORDS
    .STRINGZ "this"
    .BLKW 3
    .STRINGZ "is"
    .BLKW 5
    .STRINGZ "some"
    .BLKW 3
    .STRINGZ "words"
    .BLKW 2
    .STRINGZ "in"
    .BLKW 5
    .STRINGZ "array"
    .BLKW 2
    .FILL x0000 ; Equivalent to an empty null-terminated string

.END
