#ifndef _RGB_TO_YUV_H_
#define _RGB_TO_YUV_H_

#include <stdint.h>
#include <msa.h>

static inline int _fix_range(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

/* _c_g[i][0] = i * 129; so must convert to unsigned int for short signed overfollow */
#define to_y(r_, g_, b_) \
    _fix_range(((((int)r_ * 66 + g_ * 129 + b_ * 25)>>8) + 16), 0, 255)
#define to_u(r_, g_, b_) \
    _fix_range(((((int)r_ * -38 + g_ * -74 + b_ * 112)>>8) + 128), 0, 255)
#define to_v(r_, g_, b_) \
    _fix_range(((((int)r_ * 112 + g_ * -94 + b_ * -18)>>8) + 128), 0, 255)

#endif /* _RGB_TO_YUV_H_ */
