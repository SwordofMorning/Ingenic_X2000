#ifndef __YUV_TO_RGB_H__
#define __YUV_TO_RGB_H__

static inline int _range(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

#define to_r(y_, v_) \
    _range(((((int)y_ * 298 + v_ * 411 - 57344)>>8)), 0, 255)
#define to_g(y_, u_, v_) \
    _range(((((int)y_ * 298 - u_ * 101 -  v_ * 211 + 34739)>>8)), 0, 255)
#define to_b(y_, u_) \
    _range(((((int)y_ * 298 + u_ * 519 - 71117)>>8)), 0, 255)

#endif