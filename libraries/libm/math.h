#pragma once

#if FLT_EVAL_METHOD == 0
    typedef float float_t;
    typedef double double_t;
#elif FLT_EVAL_METHOD == 1
    typedef double float_t;
    typedef double double_t;
#elif FLT_EVAL_METHOD == 2
    typedef long double float_t;
    typedef long double double_t;
#endif

#define FP_INFINITE 1
#define FP_NAN 2
#define FP_NORMAL 3
#define FP_SUBNORMAL 4
#define FP_ZERO 5

#define fpcalssify(x) __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x)

#define M_E        2.718281828459045
#define M_LOG2E    1.4426950408889634
#define M_LOG10E   0.4342944819032518
#define M_LN2      0.6931471805599453
#define M_LN10     2.302585092994045
#define M_PI       3.141592653589793
#define M_PI_2     1.5707963267948966
#define M_PI_4     0.7853981633974483
#define M_1_PI     0.3183098861837907
#define M_2_PI     0.6366197723675814
#define M_2_SQRTPI 1.1283791670955126
#define M_SQRT2    1.4142135623730951
#define M_SQRT1_2  0.7071067811865475

#define HUGE_VAL __builtin_huge_val()
#define HUGE_VALF __builtin_huge_valf()
#define HUGE_VALL __builtin_huge_vall()
#define INFINITY __builtin_inf()
#define NAN __builtin_nan("")