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

#define bit_5(_word) (((_word) >> 5) & 1)
#define bit_11(_word) (((_word) >> 11) & 1)
#define bit_15(_word) (((_word) >> 15) & 1)

#define bits_0_2(_word) ((_word) & BITMASK_LOW_3)
#define bits_0_5(_word) ((_word) & BITMASK_LOW_5)
#define bits_0_6(_word) ((_word) & BITMASK_LOW_6)
#define bits_0_8(_word) ((_word) & BITMASK_LOW_8)
#define bits_3_4(_word) (((_word) >> 3) & BITMASK_LOW_2)
#define bits_6_8(_word) (((_word) >> 6) & BITMASK_LOW_3)
#define bits_8_12(_word) (((_word) >> 8) & BITMASK_LOW_4)
#define bits_9_10(_word) (((_word) >> 9) & BITMASK_LOW_2)
#define bits_9_11(_word) (((_word) >> 9) & BITMASK_LOW_3)
#define bits_12_15(_word) (((_word) >> 12) & BITMASK_LOW_4)

#define bits_high(_word) (((_word) >> 8) & BITMASK_LOW_8)
#define bits_low(_word) ((_word) & BITMASK_LOW_8)

#endif
