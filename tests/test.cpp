#include <cassert>

#include "../src/assemble.cpp"
#include "../src/execute.cpp"

#define assert_eq(_msg, _left, _right)           \
    {                                            \
        auto left = (_left);                     \
        auto right = (_right);                   \
        if (left != right) {                     \
            printf("Failed: %s\n", (_msg));      \
            printf("\t left: %s\n", #_left);     \
            printf("\t     = 0x%04hx\n", left);  \
            printf("\tright: %s\n", #_right);    \
            printf("\t     = 0x%04hx\n", right); \
            return 1;                            \
        }                                        \
    }

int main() {
    assert_eq("Sign extend zero", sign_extend(0x00, 5), (SignedWord)0x0000);
    assert_eq("Sign extend positive", sign_extend(0x01, 5), (SignedWord)0x0001);
    assert_eq("Sign extend positive", sign_extend(0x0f, 5), (SignedWord)0x000f);
    assert_eq("Sign extend negative", sign_extend(0x1f, 5), (SignedWord)0xffff);
    assert_eq("Sign extend negative", sign_extend(0x10, 5), (SignedWord)0xfff0);

    // 5 bits, high bit is sign bit
    assert_eq("Zero fits in size", does_positive_integer_fit_size(0x00, 5),
              true);
    assert_eq("Zero fits in size", does_negative_integer_fit_size(0x00, 5),
              true);
    assert_eq("Positive number fits in size",
              does_positive_integer_fit_size(0x0f, 5), true);
    assert_eq("Positive number doesn't fit in size",
              does_positive_integer_fit_size(0x10, 5), false);
    assert_eq("Positive number doesn't fit in size",
              does_positive_integer_fit_size(0xffff, 5), false);
    assert_eq("Negative number fits in size",
              does_negative_integer_fit_size(-0x01, 5), true);
    assert_eq("Negative number fits in size",
              does_negative_integer_fit_size(-0x0f, 5), true);
    assert_eq("Negative number fits in size",
              does_negative_integer_fit_size(-0x10, 5), true);
    assert_eq("Negative number doesn't fit in size",
              does_positive_integer_fit_size(-0x11, 5), false);
    assert_eq("Negative number doesn't fit in size",
              does_positive_integer_fit_size(-0x7fff, 5), false);
    assert_eq("Negative number doesn't fit in size",
              does_positive_integer_fit_size(-0x8000, 5), false);
}
