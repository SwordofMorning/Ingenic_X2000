#ifndef _ROTATE_H_
#define _ROTATE_H_

void rotate_init(void);

void rotate_destory(void);

void rotate_argb_part(
    unsigned int *src, int width, int height, int from, int to,
     int angle, unsigned int color, unsigned int *dst);

void rotate_argb(
    unsigned int *src, int width, int height,
    int angle, unsigned int color, unsigned int *dst);

void rotate_argb_j2(
    unsigned int *src, int width, int height,
    int angle, unsigned int color, unsigned int *dst);

#endif /* _ROTATE_H_ */