#ifndef BITMASKS_HPP
#define BITMASKS_HPP

#define BITMASK_LOW_2 0b0000'0000'0000'0011
#define BITMASK_LOW_3 0b0000'0000'0000'0111
#define BITMASK_LOW_4 0b0000'0000'0000'1111
#define BITMASK_LOW_5 0b0000'0000'0001'1111
#define BITMASK_LOW_6 0b0000'0000'0011'1111
#define BITMASK_LOW_7 0b0000'0000'0111'1111
#define BITMASK_LOW_8 0b0000'0000'1111'1111
#define BITMASK_LOW_9 0b0000'0001'1111'1111
#define BITMASK_LOW_11 0b0000'0111'1111'1111

#define bit_5(word) (((word) >> 5) & 1)
#define bit_11(word) (((word) >> 11) & 1)
#define bit_15(word) (((word) >> 15) & 1)

#define bits_0_2(word) ((word) & BITMASK_LOW_3)
#define bits_0_5(word) ((word) & BITMASK_LOW_5)
#define bits_0_6(word) ((word) & BITMASK_LOW_6)
#define bits_0_8(word) ((word) & BITMASK_LOW_8)
#define bits_3_4(word) (((word) >> 3) & BITMASK_LOW_2)
#define bits_6_8(word) (((word) >> 6) & BITMASK_LOW_3)
#define bits_8_12(word) (((word) >> 8) & BITMASK_LOW_4)
#define bits_9_10(word) (((word) >> 9) & BITMASK_LOW_2)
#define bits_9_11(word) (((word) >> 9) & BITMASK_LOW_3)
#define bits_12_15(word) (((word) >> 12) & BITMASK_LOW_4)

#define bits_high(word) (((word) >> 8) & BITMASK_LOW_8)
#define bits_low(word) ((word) & BITMASK_LOW_8)

#endif
