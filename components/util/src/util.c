#include "util.h"

int clamp_int(int val, int lo, int hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

int abs_int(int val)
{
    return val < 0 ? -val : val;
}
