#if defined(__mips_msa)
#include <msa.h>
/**
 *******************************************************************************
 * @file :msa_convert.c
 * @author :sz_ingenic
 * @date : 2022/04/02
 * @brief: 使用mips msa 128 位寄存器优化rgb转yuv速度
 *******************************************************************************
 */
/*
 *******************************************使用到的msa api 说明：*******************************************************************************
 * v16i8 ----> char [16];
 * v8i16 ----> short[8];
 * v16u8 ----> unsigned char [16];
 * v8u16 ----> unsigned short [8];
 *
 *
 * v16i8 data
 * unsigned char *addr;
 *
 * 从addr+off 地址加载128位数数据到data
 * __msa_ld_b(addr, off)   -------> data = {addr[0+off], addr[1+off], addr[2+off], addr[3+off],addr[4+off], addr[5+off], addr[6+off], addr[7+off],
 *                                                  addr[8+off], addr[9+off], addr[10+off], addr[11+off], addr[12+off], addr[13+off], addr[14+off], addr[15+off]}
 *
 * v16i8 d0, d1
 * 取偶数位的d0 再取偶数位的d1数据，并组合成128位数据（以8位为单位）
 * __msa_pckev_b(d1, d0) -------> v16i8 data = { d0[0] d0[2] d0[4] d0[6] d0[8] d0[10] d0[12] d0[14]
 *                                                       d1[0] d1[2] d1[4] d1[6] d1[8] d1[10] d1[12] d1[14] }
 *
 * 取奇数位的d0, 再取奇数位的d1数据，并组成128位数据 （以8位为单位）
 * __msa_pckod_b(d1, d0) -------> v16i8 data = { d0[1] d0[3] d0[5] d0[7] d0[9] d0[11] d0[13] d0[15]
 *                                                       d1[1] d1[3] d1[5] d1[7] d1[9] d1[11] d1[13] d1[15] }
 * v16i8 d0,d1;
 * d0低64位的数据与d1的低64位两两相乘（以8位为单位相乘，得到以16位为单位的数据）
 * __msa_mulur_h(d1, d0) ------> v8u16 data = { d0[0]*d1[0], d0[1]*d1[1], d0[2]*d1[2], d0[3]*d1[3],
 *                                                      d0[4]*d1[4], d0[5]*d1[5], d0[6]*d1[6], d0[7]*d1[7] }
 *
 * d0高64位的数据与d1的高64位两两相乘（以8位为单位相乘，得到以16位为单位的数据）
 * __msa_mulur_u(d1, d0) ------> v8u16 data = { d0[8]*d1[8], d0[9]*d1[9], d0[10]*d1[10], d0[11]*d1[11],
 *                                                      d0[12]*d1[12], d0[13]*d1[13], d0[14]*d1[14], d0[15]*d1[15] }
 * v8i16 d0,d1;
 * d0数据与d1数据两两相加（以16位为单位）
 * __msa_addv_h(d1, d0) ------> v8u16 data = { d0[0]+d1[0], d0[1]+d1[1], d0[2]+d1[2], d0[3]+d1[3],
 *                                                     d0[4]+d1[4], d0[5]+d1[5], d0[6]+d1[6], d0[7]+d1[7] }
 * v16i8 d0;
 * d0数据分别加上n（0~31）（以8位为单位）
 * __msa_addvi_b(d0, n) -----> v16i8 data = {d0[0]+n, d0[1]+n, d0[2]+n, d0[3]+n, d0[4]+n, d0[5]+n, d0[6]+n, d0[7]+n
 *                                                   d0[8]+n, d0[9]+n, d0[10]+n, d0[11]+n, d0[12]+n, d0[13]+n, d0[14]+n, d0[15]+n}
 *
 * v8u16 d0, d1;
 * d0数据分别与d1数据两两相乘 （以16位为单位）
 * __msa_mulv_h(d0, d1) -----> v8u16 data = { d0[0]*d1[0], d0[1]*d1[1], d0[2]*d1[2], d0[3]*d1[3],
 *                                                    d0[4]*d1[4], d0[5]*d1[5], d0[6]*d1[6], d0[7]*d1[7] }
 *
 * v8i16 d0, d1;
 * d0 数据分别减去d1数据（以16位为单位）
 * __msa_subs_s_h(d0, d1) ------> v8i16 data = { d0[0]-d1[0], d0[1]-d1[1], d0[2]-d1[2], d0[3]-d1[3],
 *                                                       d0[4]-d1[4], d0[5]-d1[5], d0[6]-d1[6], d0[7]-d1[7]}
 * v16i8 d0, d1;
 * 按顺序取d0 d1的奇数位数据（以8位为单位）
 * __msa_ilvod_b(d1, d0) -------> v16i8 = {d0[1], d1[1], d0[3], d1[3], d0[5], d1[5], d0[7], d1[7],
 *                                                 d0[9], d1[9], d0[11], d1[11], d0[13], d1[13], d0[15], d1[15]}
 * v16i8 d0, d1;
 * d0数据与d1数据两两相加（以8位为单位）
 * __msa_addv_b(d0, d1) --------> v16i8 = {d0[0]+d1[0], d[0][1]+d1[1], d0[2]+d1[2],d0[3]+d1[3], d0[4]+d1[4]
 *                                                 d0[5]+d1[5] ........., d0[15]+d1[15]}
 *
 **********************************************************************************************************************************************
 */

/**
 * msa_ld_bgr(p, b, g, r)的功能：将bgra数据，单独分离成b、g、r数据，并加载到寄存器
 * msa_ld_bgr(p, b, g, r, a)的功能：将bgra数据，单独分离成b、g、r、a数据，并加载到寄存器
 *
 *
 *    p0                                            d00 = { b g r a b g r a b g r a b g r a }
 *    p0 + 16 ----> __msa_ld_b() ------>    d01 = { b g r a b g r a b g r a b g r a }
 *    p0 + 32                                       d02 = { b g r a b g r a b g r a b g r a }
 *    p0 + 48                                       d03 = { b g r a b g r a b g r a b g r a }
 *
 *
 *    __msa_pckev_b(d01, d00) ---->  br00 = { b r b r b r b r b r b r b r b r }
 *    __msa_pckev_b(d03, d02) ---->  br01 = { b r b r b r b r b r b r b r b r }
 *
 *
 *    __msa_pckod_b(d01, d00) ---->  d00 = { g a g a g a g a g a g a g a g a }
 *    __msa_pckod_b(d03, d02) ---->  d01 = { g a g a g a g a g a g a g a g a }
 *
 *    __msa_pckev_b(br01, br00) ----> b = { b b b b b b b b b b b b b b b b }
 *    __msa_pckev_b(d01, d00) ------> g = { g g g g g g g g g g g g g g g g }
 *    __msa_pckod_b(br01, br00) ----> r = { r r r r r r r r r r r r r r r r }
 *    ——builtin_msa_pckod_b(d01, d00) ------> a = { a a a a a a a a a a a a a a a a }
 *
 */

#define msa_ld_bgr(p, b, g, r)\
do {\
    v16i8 d00 = __msa_ld_b(p0, 0);\
    v16i8 d01 = __msa_ld_b(p0 + 16, 0);\
    v16i8 d02 = __msa_ld_b(p0 + 32, 0);\
    v16i8 d03 = __msa_ld_b(p0 + 48, 0);\
\
    v16i8 br00 = __msa_pckev_b(d01, d00);\
    v16i8 br01 = __msa_pckev_b(d03, d02);\
\
    d00 = __msa_pckod_b(d01, d00);\
    d01 = __msa_pckod_b(d03, d02);\
\
    b = __msa_pckev_b(br01, br00);\
    g = __msa_pckev_b(d01, d00);\
    r = __msa_pckod_b(br01, br00);\
} while(0)


#define msa_ld_bgra(p, b, g, r, a)\
do {\
    v16i8 d00 = __msa_ld_b(p0, 0);\
    v16i8 d01 = __msa_ld_b(p0 + 16, 0);\
    v16i8 d02 = __msa_ld_b(p0 + 32, 0);\
    v16i8 d03 = __msa_ld_b(p0 + 48, 0);\
\
    v16i8 br00 = __msa_pckev_b(d01, d00);\
    v16i8 br01 = __msa_pckev_b(d03, d02);\
\
    d00 = __msa_pckod_b(d01, d00);\
    d01 = __msa_pckod_b(d03, d02);\
\
    b = __msa_pckev_b(br01, br00);\
    g = __msa_pckev_b(d01, d00);\
    r = __msa_pckod_b(br01, br00);\
    a = __msa_pckod_b(d01, d00);\
} while(0)

/**
 * msa_to_y(b, g, r, y): 将 bgr 转化成 y 数据， 一次性转化16个y数据
 *
 *****************************************计算y数据时的数据流如下：********************************************************************************
 * Y = (66*R + 129*G + 25*B) >> 8 + 16
 *
 * v16i8 y_b_mut = {25,25,25,.......25};
 * v16i8 y_g_mut = {129,129,129,.......129};
 * v16i8 y_r_mut = {66,66,66,.......66};
 *
 * v8u16 y_b0, y_b1, y_g0, y_g1, y_r0, y_r1;
 *
 * 25*B:
 * __msa_mulur_h(b, y_b_mut) ------> y_b0 = {b[0]*25, b[1]*25, .......b[7]*25};
 * __msa_mulul_h(b, y_b_mut) ------> y_b1 = {b[8]*25, b[9]*25, .......b[15]*25};
 *
 * 129*G:
 * __msa_mulur_h(g, y_g_mut) ------> y_g0 = {g[0]*129, g[1]*129, .......g[7]*129};
 * __msa_mulul_h(g, y_g_mut) ------> y_g1 = {g[8]*129, g[9]*129, .......g[15]*129};
 *
 * 66*R:
 * __msa_mulur_h(r, y_r_mut) ------> y_r0 = {r[0]*66, r[1]*66, .......r[7]*66};
 * __msa_mulul_h(r, y_r_mut) ------> y_r1 = {r[8]*66, r[9]*66, .......r[15]*66};
 *
 *
 * 25*B + 129*G + 66*R:
 * __msa_addv_h(y_r0, y_b0) ------> y_r0 = y_b0 + y_r0 (25*B + 66*R)
 * __msa_addv_h(y_r0, y_g0) ------> y_r0 = y_g0 + y_r0 (129*G + (25*B + 66*R))
 *
 * __msa_addv_h(y_r1, y_b1) ------> y_r1 = y_b1 + y_r1 (25*B + 66*R)
 * __msa_addv_h(y_r1, y_g1) ------> y_r1 = y_g1 + y_r1 (129*G + (25*B + 66*R))
 *
 * (66*R + 129*G + 25*B) >> 8:
 * 每16位只取高8位：
 * __msa_pckod_b((v16i8)y_r1, (v16i8)y_r0) ------> y = {y_r0[1], y_r0[3].....y_r0[15]
 *                                                              y_r1[1], y_r1[3].....y_r1[15]}
 *
 * ((66*R + 129*G + 25*B) >> 8) + 16
 *
 * __msa_addvi_b(y, 16) --------> y= { y[0]+16, y[1]+16.....y[15]+16 }
 *
 **************************************************************************************************************
*/

#define msa_to_y(b, g, r, y)\
do {\
    v8u16 y_b0 = __msa_mulur_h((v16u8)b, (v16u8)y_b_mut);\
    v8u16 y_b1 = __msa_mulul_h((v16u8)b, (v16u8)y_b_mut);\
\
    v8u16 y_g0 = __msa_mulur_h((v16u8)g, (v16u8)y_g_mut);\
    v8u16 y_g1 = __msa_mulul_h((v16u8)g, (v16u8)y_g_mut);\
\
    v8u16 y_r0 = __msa_mulur_h((v16u8)r, (v16u8)y_r_mut);\
    v8u16 y_r1 = __msa_mulul_h((v16u8)r, (v16u8)y_r_mut);\
\
    y_r0 = (v8u16)__msa_addv_h((v8i16)y_r0, (v8i16)y_b0);\
    y_r0 = (v8u16)__msa_addv_h((v8i16)y_r0, (v8i16)y_g0);\
\
    y_r1 = (v8u16)__msa_addv_h((v8i16)y_r1, (v8i16)y_b1);\
    y_r1 = (v8u16)__msa_addv_h((v8i16)y_r1, (v8i16)y_g1);\
\
    y = __msa_pckod_b((v16i8)y_r1, (v16i8)y_r0);\
    y = __msa_addvi_b(y, 16);\
}while(0)


/**
 * msa_to_u(b, g, r, u): 将 bgr 转化成 u 数据， 一次性转化8个u数据
 *
 *****************************************计算u数据时的数据流如下：********************************************************************************
 * U = (112*B - (38*R + 74*G))>>8 + 128
 *
 * v8i16 u_b_mut = {112,112,112,.......112};
 * v8i16 u_g_mut = {74,74,74,.......74};
 * v8i16 u_r_mut = {38,38,38,.......38};
 * v16i8 uv_add  = {128,128,128....128};
 *
 * v8u16 u_b, u_g, u_r;
 *
 * 112*B:
 * __msa_mulv_h(b, u_b_mut) ------> u_b = {b[0]*112, b[1]*112, .......b[7]*112};
 *
 * 74*G:
 * __msa_mulv_h(g, u_g_mut) ------> u_g = {g[0]*129, g[1]*129, .......g[7]*129};
 *
 * 38*R:
 * __msa_mulv_h(r, u_r_mut) ------> u_r = {r[0]*66, r[1]*66, .......r[7]*66};
 *
 *
 * 112*B - (38*R + 74*G):
 * __msa_addv_h(u_r, u_g) ------> u_r = u_r + u_g (38*R + 74*G)
 * __msa_subs_s_h(u_b, u_r) ------> u = u_b - u_r (112*B - (38*R + 74*G))
 *
 *
 * (112*B - (38*R + 74*G))>>8 + 128
 * __msa_addv_b((v16i8)u, uv_add) -------> (v16i8)u = {u[0]+uv_add, u[1]+uv_add, ....., u[15]+uv_add }
 *
 * 注意：算出来的每16位的高8位有效，低八位无效！
 *
 **************************************************************************************************************
*/
#define msa_to_u(b, g, r, u)\
do {\
    v8u16 u_b = (v8u16)__msa_mulv_h((v8i16)b, u_b_mut);\
    v8u16 u_g = (v8u16)__msa_mulv_h((v8i16)g, u_g_mut);\
    v8u16 u_r = (v8u16)__msa_mulv_h((v8i16)r, u_r_mut);\
\
    u_r = (v8u16)__msa_addv_h((v8i16)u_r, (v8i16)u_g);\
    u = (v16i8)__msa_subs_s_h((v8i16)u_b, (v8i16)u_r);\
    u = (v16i8)__msa_addv_b((v16i8)u, uv_add);\
}while(0)


/**
 * msa_to_v(b, g, r, v): 将 bgr 转化成 v 数据， 一次性转化8个v数据
 *
 *****************************************计算v数据时的数据流如下：********************************************************************************
 * V = (112*R - (94*G + 18*B))>>8 + 128
 *
 * v8i16 v_b_mut = {18,18,18,.......18};
 * v8i16 v_g_mut = {94,94,94,.......94};
 * v8i16 v_r_mut = {112,112,112,.......112};
 * v16i8 uv_add  = {128,128,128....128};
 *
 * v8u16 v_b, v_g, v_r;
 *
 * 18*B:
 * __msa_mulv_h(b, u_b_mut) ------> v_b = {b[0]*112, b[1]*112, .......b[7]*112};
 *
 * 94*G:
 * __msa_mulv_h(g, u_g_mut) ------> v_g = {g[0]*129, g[1]*129, .......g[7]*129};
 *
 * 112*R:
 * __msa_mulv_h(r, u_r_mut) ------> v_r = {r[0]*66, r[1]*66, .......r[7]*66};
 *
 *
 * 112*R - (94*G + 18*B):
 * __msa_addv_h(v_b, v_g) ------>   v_b = v_b + v_g (94*G + 18*B)
 * __msa_subs_s_h(v_r, v_b) ------> v = v_r - v_b (112*R - (94*G + 18*B))
 *
 *
 *(112*R - (94*G + 18*B))>>8 + 128:
 * __msa_addv_b((v16i8)v, uv_add) -------> (v16i8)v = {v[0]+uv_add, v[1]+uv_add, ....., v[15]+uv_add }
 *
 * 注意：算出来的每16位的高8位有效，低八位无效！
 *
 **************************************************************************************************************
*/
#define msa_to_v(b, g, r, v)\
do {\
    v8u16 v_b = (v8u16)__msa_mulv_h((v8i16)b, v_b_mut);\
    v8u16 v_g = (v8u16)__msa_mulv_h((v8i16)g, v_g_mut);\
    v8u16 v_r = (v8u16)__msa_mulv_h((v8i16)r, v_r_mut);\
\
    v_b = (v8u16)__msa_addv_h((v8i16)v_b, (v8i16)v_g);\
    v = (v16i8)__msa_subs_s_h((v8i16)v_r, (v8i16)v_b);\
    v = (v16i8)__msa_addv_b((v16i8)v, uv_add);\
}while(0)
/**
 * msa_to_uv(b, g, r, uv): 将 bgr 转化成 v 数据， 一次性转化8组uv数据
 * *****************************************计算uv数据时的数据流如下：******************************************************************************
 * 该函数等同于：
 * v8u16 u,v;
 * msa_to_u(b,g,r,u)
 * msa_to_v(b,g,r,v)
 *
 *
 * u、v 都是每16位只取高8位
 * __msa_ilvod_b((v16i8)v, (v16i8)u) -------> uv = {u[1], v[1], u[3], v[3], u[5], v[5], ......., u[15], v[15]}
*/

#define msa_to_uv(b, g, r, uv)\
do {\
    v8u16 u_b = (v8u16)__msa_mulv_h((v8i16)b, u_b_mut);\
    v8u16 u_g = (v8u16)__msa_mulv_h((v8i16)g, u_g_mut);\
    v8u16 u_r = (v8u16)__msa_mulv_h((v8i16)r, u_r_mut);\
\
    v8u16 v_b = (v8u16)__msa_mulv_h((v8i16)b, v_b_mut);\
    v8u16 v_g = (v8u16)__msa_mulv_h((v8i16)g, v_g_mut);\
    v8u16 v_r = (v8u16)__msa_mulv_h((v8i16)r, v_r_mut);\
\
    u_r = (v8u16)__msa_addv_h((v8i16)u_r, (v8i16)u_g);\
    v8i16 u = __msa_subs_s_h((v8i16)u_b, (v8i16)u_r);\
\
    v_b = (v8u16)__msa_addv_h((v8i16)v_b, (v8i16)v_g);\
    v8i16 v = __msa_subs_s_h((v8i16)v_r, (v8i16)v_b);\
\
    uv = (v16i8)__msa_ilvod_b((v16i8)v, (v16i8)u);\
    uv = __msa_addv_b(uv, uv_add);\
}while(0)

#endif /* defined(__mips_msa) */