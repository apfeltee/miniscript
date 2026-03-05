
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#if defined(NAN)
    #define STOD_CONST_NAN NAN
#else
    #define STOD_CONST_NAN (0.0f / 0.0f)
#endif

#if defined(INFINITY)
    #define STOD_CONST_INFINITY INFINITY
#else
    #define STOD_CONST_INFINITY (1e5000f)
#endif

#if !defined(STOD_INLINE)
    #if defined(__STRICT_ANSI__)
        #define STOD_INLINE static
    #else
        #define STOD_INLINE static inline
    #endif
#endif

/**
* lifted out of njs
*/

#define stod_slow_path(x) (x)
#define stod_fast_path(x) (x)

/*
 * Max double: 1.7976931348623157 x 10^308
 * Min non-zero double: 4.9406564584124654 x 10^-324
 * Any x >= 10^309 is interpreted as +infinity.
 * Any x <= 10^-324 is interpreted as 0.
 * Note that 2.5e-324 (despite being smaller than the min double)
 * will be read as non-zero (equal to the min non-zero double).
 */

#define STOD_DECIMAL_POWER_MAX 309
#define STOD_DECIMAL_POWER_MIN (-324)

#define STOD_UINT64_MAX stod_makeuint64(0xFFFFFFFF, 0xFFFFFFFF)
#define STOD_UINT64_DECIMAL_DIGITS_MAX 19

#define STOD_DENOM_LOG 3
#define STOD_DENOM (1 << STOD_DENOM_LOG)

#define stod_makeuint64(h, l) (((uint64_t)(h) << 32) + (l))

#define STOD_DBL_SIGNIFICAND_SIZE 52
#define STOD_DBL_EXPONENT_OFFSET ((int64_t)0x3ff)
#define STOD_DBL_EXPONENT_BIAS (STOD_DBL_EXPONENT_OFFSET + STOD_DBL_SIGNIFICAND_SIZE)
#define STOD_DBL_EXPONENT_MIN (-STOD_DBL_EXPONENT_BIAS)
#define STOD_DBL_EXPONENT_MAX (0x7ff - STOD_DBL_EXPONENT_BIAS)
#define STOD_DBL_EXPONENT_DENORMAL (-STOD_DBL_EXPONENT_BIAS + 1)

#define STOD_DBL_SIGNIFICAND_MASK stod_makeuint64(0x000FFFFF, 0xFFFFFFFF)
#define STOD_DBL_HIDDEN_BIT stod_makeuint64(0x00100000, 0x00000000)
#define STOD_DBL_EXPONENT_MASK stod_makeuint64(0x7FF00000, 0x00000000)
#define STOD_DBL_SIGN_MASK stod_makeuint64(0x80000000, 0x00000000)

#define STOD_DIYFP_SIGNIFICAND_SIZE 64

#define STOD_SIGNIFICAND_SIZE 53
#define STOD_SIGNIFICAND_SHIFT (STOD_DIYFP_SIGNIFICAND_SIZE - STOD_DBL_SIGNIFICAND_SIZE)

#define STOD_DECIMAL_EXPONENT_OFF 348
#define STOD_DECIMAL_EXPONENT_MIN (-348)
#define STOD_DECIMAL_EXPONENT_MAX 340
#define STOD_DECIMAL_EXPONENT_DIST 8

#define STOD_D_1_LOG2_10 0.30102999566398114 /* 1 / log2(10). */

typedef struct mcstoddiyfp_t mcstoddiyfp_t;
typedef struct mcstodcpe_t mcstodcpe_t;
typedef union mcstoddiyfpconv_t mcstoddiyfpconv_t;


struct mcstoddiyfp_t
{
    uint64_t significand;
    int exp;
};

struct mcstodcpe_t
{
    uint64_t significand;
    int16_t bin_exp;
    int16_t dec_exp;
};

union mcstoddiyfpconv_t
{
    double d;
    uint64_t u64;
};

STOD_INLINE mcstoddiyfp_t stod_makediyfp(uint64_t sign, int exp)
{
    mcstoddiyfp_t r;
    r.significand = sign;
    r.exp = exp;
    return r;
}

uint64_t stod_leading_zeros64(uint64_t x)
{
    #if(STOD_HAVE_BUILTIN_CLZLL)
        return (((x) == 0) ? 64 : __builtin_clzll(x));
    #else
        uint64_t n;
        /*
        * There is no sense to optimize this function, since almost
        * all platforms nowadays support the built-in instruction.
        */
        if(x == 0)
        {
            return 64;
        }
        n = 0;
        while((x & 0x8000000000000000) == 0)
        {
            n++;
            x <<= 1;
        }
        return n;

    #endif
}


double stod_diyfp2d(mcstoddiyfp_t v)
{
    int exp;
    uint64_t significand;
    uint64_t biased_exp;
    mcstoddiyfpconv_t conv;
    exp = v.exp;
    significand = v.significand;
    while(significand > STOD_DBL_HIDDEN_BIT + STOD_DBL_SIGNIFICAND_MASK)
    {
        significand >>= 1;
        exp++;
    }
    if(exp >= STOD_DBL_EXPONENT_MAX)
    {
        return -1;
    }
    if(exp < STOD_DBL_EXPONENT_DENORMAL)
    {
        return 0.0;
    }
    while(exp > STOD_DBL_EXPONENT_DENORMAL && (significand & STOD_DBL_HIDDEN_BIT) == 0)
    {
        significand <<= 1;
        exp--;
    }
    if(exp == STOD_DBL_EXPONENT_DENORMAL && (significand & STOD_DBL_HIDDEN_BIT) == 0)
    {
        biased_exp = 0;
    }
    else
    {
        biased_exp = (uint64_t)(exp + STOD_DBL_EXPONENT_BIAS);
    }
    conv.u64 = (significand & STOD_DBL_SIGNIFICAND_MASK) | (biased_exp << STOD_DBL_SIGNIFICAND_SIZE);
    return conv.d;
}

mcstoddiyfp_t stod_diyfp_shift_left(mcstoddiyfp_t v, unsigned shift)
{
    return stod_makediyfp((v.significand << shift), (int)(v.exp - shift));
}

mcstoddiyfp_t stod_diyfp_shift_right(mcstoddiyfp_t v, unsigned shift)
{
    return stod_makediyfp((v.significand >> shift), (int)(v.exp + shift));
}

mcstoddiyfp_t stod_diyfp_mul(mcstoddiyfp_t lhs, mcstoddiyfp_t rhs)
{
    #if(STOD_HAVE_UNSIGNED_INT128)
        uint64_t l;
        uint64_t h;
        stod_uint128_t u128;
        u128 = (stod_uint128_t)(lhs.significand) * (stod_uint128_t)(rhs.significand);
        h = u128 >> 64;
        l = (uint64_t)u128;
        /* rounding. */
        if(l & ((uint64_t)1 << 63))
        {
            h++;
        }
        return stod_makediyfp(h, lhs.exp + rhs.exp + 64);
    #else
        uint64_t a, b, c, d, ac, bc, ad, bd, tmp;
        a = lhs.significand >> 32;
        b = lhs.significand & 0xffffffff;
        c = rhs.significand >> 32;
        d = rhs.significand & 0xffffffff;
        ac = a * c;
        bc = b * c;
        ad = a * d;
        bd = b * d;
        tmp = (bd >> 32) + (ad & 0xffffffff) + (bc & 0xffffffff);
        /* mult_round. */
        tmp += 1U << 31;
        return stod_makediyfp(ac + (ad >> 32) + (bc >> 32) + (tmp >> 32), lhs.exp + rhs.exp + 64);
    #endif
}

mcstoddiyfp_t stod_diyfp_normalize(mcstoddiyfp_t v)
{
    return stod_diyfp_shift_left(v, stod_leading_zeros64(v.significand));
}

/*
 * Reads digits from the buffer and converts them to a uint64.
 * Reads in as many digits as fit into a uint64.
 * When the string starts with "1844674407370955161" no further digit is read.
 * Since 2^64 = 18446744073709551616 it would still be possible read another
 * digit if it was less or equal than 6, but this would complicate the code.
 */
uint64_t stod_read_uint64(const unsigned char* start, size_t length, size_t* ndigits)
{
    unsigned char d;
    uint64_t value;
    const unsigned char *p, *e;
    value = 0;
    p = start;
    e = p + length;
    while(p < e && value <= (STOD_UINT64_MAX / 10 - 1))
    {
        d = *p++ - '0';
        value = 10 * value + d;
    }
    *ndigits = p - start;
    return value;
}

/*
 * Reads a mcstoddiyfp_t from the buffer.
 * The returned mcstoddiyfp_t is not necessarily normalized.
 * If remaining is zero then the returned mcstoddiyfp_t is accurate.
 * Otherwise it has been rounded and has error of at most 1/2 ulp.
 */
static mcstoddiyfp_t stod_diyfp_read(const unsigned char* start, size_t length, int* remaining)
{
    size_t read;
    uint64_t significand;
    significand = stod_read_uint64(start, length, &read);
    /* Round the significand. */
    if(length != read)
    {
        if(start[read] >= '5')
        {
            significand++;
        }
    }
    *remaining = length - read;
    return stod_makediyfp(significand, 0);
}

/* clang-format disable */
static const mcstodcpe_t stod_cached_powers[] = {
    { stod_makeuint64(0xfa8fd5a0, 0x081c0288), -1220, -348 }, { stod_makeuint64(0xbaaee17f, 0xa23ebf76), -1193, -340 },
    { stod_makeuint64(0x8b16fb20, 0x3055ac76), -1166, -332 }, { stod_makeuint64(0xcf42894a, 0x5dce35ea), -1140, -324 },
    { stod_makeuint64(0x9a6bb0aa, 0x55653b2d), -1113, -316 }, { stod_makeuint64(0xe61acf03, 0x3d1a45df), -1087, -308 },
    { stod_makeuint64(0xab70fe17, 0xc79ac6ca), -1060, -300 }, { stod_makeuint64(0xff77b1fc, 0xbebcdc4f), -1034, -292 },
    { stod_makeuint64(0xbe5691ef, 0x416bd60c), -1007, -284 }, { stod_makeuint64(0x8dd01fad, 0x907ffc3c), -980, -276 },
    { stod_makeuint64(0xd3515c28, 0x31559a83), -954, -268 },  { stod_makeuint64(0x9d71ac8f, 0xada6c9b5), -927, -260 },
    { stod_makeuint64(0xea9c2277, 0x23ee8bcb), -901, -252 },  { stod_makeuint64(0xaecc4991, 0x4078536d), -874, -244 },
    { stod_makeuint64(0x823c1279, 0x5db6ce57), -847, -236 },  { stod_makeuint64(0xc2109436, 0x4dfb5637), -821, -228 },
    { stod_makeuint64(0x9096ea6f, 0x3848984f), -794, -220 },  { stod_makeuint64(0xd77485cb, 0x25823ac7), -768, -212 },
    { stod_makeuint64(0xa086cfcd, 0x97bf97f4), -741, -204 },  { stod_makeuint64(0xef340a98, 0x172aace5), -715, -196 },
    { stod_makeuint64(0xb23867fb, 0x2a35b28e), -688, -188 },  { stod_makeuint64(0x84c8d4df, 0xd2c63f3b), -661, -180 },
    { stod_makeuint64(0xc5dd4427, 0x1ad3cdba), -635, -172 },  { stod_makeuint64(0x936b9fce, 0xbb25c996), -608, -164 },
    { stod_makeuint64(0xdbac6c24, 0x7d62a584), -582, -156 },  { stod_makeuint64(0xa3ab6658, 0x0d5fdaf6), -555, -148 },
    { stod_makeuint64(0xf3e2f893, 0xdec3f126), -529, -140 },  { stod_makeuint64(0xb5b5ada8, 0xaaff80b8), -502, -132 },
    { stod_makeuint64(0x87625f05, 0x6c7c4a8b), -475, -124 },  { stod_makeuint64(0xc9bcff60, 0x34c13053), -449, -116 },
    { stod_makeuint64(0x964e858c, 0x91ba2655), -422, -108 },  { stod_makeuint64(0xdff97724, 0x70297ebd), -396, -100 },
    { stod_makeuint64(0xa6dfbd9f, 0xb8e5b88f), -369, -92 },   { stod_makeuint64(0xf8a95fcf, 0x88747d94), -343, -84 },
    { stod_makeuint64(0xb9447093, 0x8fa89bcf), -316, -76 },   { stod_makeuint64(0x8a08f0f8, 0xbf0f156b), -289, -68 },
    { stod_makeuint64(0xcdb02555, 0x653131b6), -263, -60 },   { stod_makeuint64(0x993fe2c6, 0xd07b7fac), -236, -52 },
    { stod_makeuint64(0xe45c10c4, 0x2a2b3b06), -210, -44 },   { stod_makeuint64(0xaa242499, 0x697392d3), -183, -36 },
    { stod_makeuint64(0xfd87b5f2, 0x8300ca0e), -157, -28 },   { stod_makeuint64(0xbce50864, 0x92111aeb), -130, -20 },
    { stod_makeuint64(0x8cbccc09, 0x6f5088cc), -103, -12 },   { stod_makeuint64(0xd1b71758, 0xe219652c), -77, -4 },
    { stod_makeuint64(0x9c400000, 0x00000000), -50, 4 },      { stod_makeuint64(0xe8d4a510, 0x00000000), -24, 12 },
    { stod_makeuint64(0xad78ebc5, 0xac620000), 3, 20 },       { stod_makeuint64(0x813f3978, 0xf8940984), 30, 28 },
    { stod_makeuint64(0xc097ce7b, 0xc90715b3), 56, 36 },      { stod_makeuint64(0x8f7e32ce, 0x7bea5c70), 83, 44 },
    { stod_makeuint64(0xd5d238a4, 0xabe98068), 109, 52 },     { stod_makeuint64(0x9f4f2726, 0x179a2245), 136, 60 },
    { stod_makeuint64(0xed63a231, 0xd4c4fb27), 162, 68 },     { stod_makeuint64(0xb0de6538, 0x8cc8ada8), 189, 76 },
    { stod_makeuint64(0x83c7088e, 0x1aab65db), 216, 84 },     { stod_makeuint64(0xc45d1df9, 0x42711d9a), 242, 92 },
    { stod_makeuint64(0x924d692c, 0xa61be758), 269, 100 },    { stod_makeuint64(0xda01ee64, 0x1a708dea), 295, 108 },
    { stod_makeuint64(0xa26da399, 0x9aef774a), 322, 116 },    { stod_makeuint64(0xf209787b, 0xb47d6b85), 348, 124 },
    { stod_makeuint64(0xb454e4a1, 0x79dd1877), 375, 132 },    { stod_makeuint64(0x865b8692, 0x5b9bc5c2), 402, 140 },
    { stod_makeuint64(0xc83553c5, 0xc8965d3d), 428, 148 },    { stod_makeuint64(0x952ab45c, 0xfa97a0b3), 455, 156 },
    { stod_makeuint64(0xde469fbd, 0x99a05fe3), 481, 164 },    { stod_makeuint64(0xa59bc234, 0xdb398c25), 508, 172 },
    { stod_makeuint64(0xf6c69a72, 0xa3989f5c), 534, 180 },    { stod_makeuint64(0xb7dcbf53, 0x54e9bece), 561, 188 },
    { stod_makeuint64(0x88fcf317, 0xf22241e2), 588, 196 },    { stod_makeuint64(0xcc20ce9b, 0xd35c78a5), 614, 204 },
    { stod_makeuint64(0x98165af3, 0x7b2153df), 641, 212 },    { stod_makeuint64(0xe2a0b5dc, 0x971f303a), 667, 220 },
    { stod_makeuint64(0xa8d9d153, 0x5ce3b396), 694, 228 },    { stod_makeuint64(0xfb9b7cd9, 0xa4a7443c), 720, 236 },
    { stod_makeuint64(0xbb764c4c, 0xa7a44410), 747, 244 },    { stod_makeuint64(0x8bab8eef, 0xb6409c1a), 774, 252 },
    { stod_makeuint64(0xd01fef10, 0xa657842c), 800, 260 },    { stod_makeuint64(0x9b10a4e5, 0xe9913129), 827, 268 },
    { stod_makeuint64(0xe7109bfb, 0xa19c0c9d), 853, 276 },    { stod_makeuint64(0xac2820d9, 0x623bf429), 880, 284 },
    { stod_makeuint64(0x80444b5e, 0x7aa7cf85), 907, 292 },    { stod_makeuint64(0xbf21e440, 0x03acdd2d), 933, 300 },
    { stod_makeuint64(0x8e679c2f, 0x5e44ff8f), 960, 308 },    { stod_makeuint64(0xd433179d, 0x9c8cb841), 986, 316 },
    { stod_makeuint64(0x9e19db92, 0xb4e31ba9), 1013, 324 },   { stod_makeuint64(0xeb96bf6e, 0xbadf77d9), 1039, 332 },
    { stod_makeuint64(0xaf87023b, 0x9bf0ee6b), 1066, 340 },
};
/* clang-format enable */

mcstoddiyfp_t stod_cached_power_dec(int exp, int* dec_exp)
{
    unsigned int index;
    const mcstodcpe_t* cp;
    index = (exp + STOD_DECIMAL_EXPONENT_OFF) / STOD_DECIMAL_EXPONENT_DIST;
    cp = &stod_cached_powers[index];
    *dec_exp = cp->dec_exp;
    return stod_makediyfp(cp->significand, cp->bin_exp);
}

/*
 * Returns 10^exp as an exact mcstoddiyfp_t.
 * The given exp must be in the range [1; STOD_DECIMAL_EXPONENT_DIST[.
 */
mcstoddiyfp_t stod_adjust_pow10(int exp)
{
    switch(exp)
    {
        case 1:
            return stod_makediyfp(stod_makeuint64(0xa0000000, 00000000), -60);
        case 2:
            return stod_makediyfp(stod_makeuint64(0xc8000000, 00000000), -57);
        case 3:
            return stod_makediyfp(stod_makeuint64(0xfa000000, 00000000), -54);
        case 4:
            return stod_makediyfp(stod_makeuint64(0x9c400000, 00000000), -50);
        case 5:
            return stod_makediyfp(stod_makeuint64(0xc3500000, 00000000), -47);
        case 6:
            return stod_makediyfp(stod_makeuint64(0xf4240000, 00000000), -44);
        case 7:
            return stod_makediyfp(stod_makeuint64(0x98968000, 00000000), -40);
        default:
            break;
    }
    return stod_makediyfp(0, 0);
}

/*
 * Returns the significand size for a given order of magnitude.
 * If v = f*2^e with 2^p-1 <= f <= 2^p then p+e is v's order of magnitude.
 * This function returns the number of significant binary digits v will have
 * once its encoded into a double. In almost all cases this is equal to
 * STOD_SIGNIFICAND_SIZE. The only exception are denormals. They start with
 * leading zeroes and their effective significand-size is hence smaller.
 */
int stod_diyfp_sgnd_size(int order)
{
    if(order >= (STOD_DBL_EXPONENT_DENORMAL + STOD_SIGNIFICAND_SIZE))
    {
        return STOD_SIGNIFICAND_SIZE;
    }
    if(order <= STOD_DBL_EXPONENT_DENORMAL)
    {
        return 0;
    }
    return order - STOD_DBL_EXPONENT_DENORMAL;
}

/*
 * Returns either the correct double or the double that is just below
 * the correct double.
 */
static double stod_diyfp_strtod(const unsigned char* start, size_t length, int exp)
{
    int magnitude;
    int prec_digits;
    int remaining;
    int dec_exp;
    int adj_exp;
    int orig_e;
    int shift;
    int64_t error;
    uint64_t prec_bits;
    uint64_t half_way;
    mcstoddiyfp_t value;
    mcstoddiyfp_t pow;
    mcstoddiyfp_t adj_pow;
    mcstoddiyfp_t rounded;
    value = stod_diyfp_read(start, length, &remaining);
    exp += remaining;
    /*
     * Since some digits may have been dropped the value is not accurate.
     * If remaining is different than 0 than the error is at most .5 ulp
     * (unit in the last place).
     * Using a common denominator to avoid dealing with fractions.
     */
    error = (remaining == 0 ? 0 : STOD_DENOM / 2);
    orig_e = value.exp;
    value = stod_diyfp_normalize(value);
    error <<= orig_e - value.exp;
    if(exp < STOD_DECIMAL_EXPONENT_MIN)
    {
        return 0.0;
    }
    pow = stod_cached_power_dec(exp, &dec_exp);
    if(dec_exp != exp)
    {
        adj_exp = exp - dec_exp;
        adj_pow = stod_adjust_pow10(exp - dec_exp);
        value = stod_diyfp_mul(value, adj_pow);
        if(STOD_UINT64_DECIMAL_DIGITS_MAX - (int)length < adj_exp)
        {
            /*
             * The adjustment power is exact. There is hence only
             * an error of 0.5.
             */
            error += STOD_DENOM / 2;
        }
    }
    value = stod_diyfp_mul(value, pow);
    /*
     * The error introduced by a multiplication of a * b equals
     *  error_a + error_b + error_a * error_b / 2^64 + 0.5
     *  Substituting a with 'value' and b with 'pow':
     *  error_b = 0.5  (all cached powers have an error of less than 0.5 ulp),
     *  error_ab = 0 or 1 / STOD_DENOM > error_a * error_b / 2^64.
     */
    error += STOD_DENOM + (error != 0 ? 1 : 0);
    orig_e = value.exp;
    value = stod_diyfp_normalize(value);
    error <<= orig_e - value.exp;
    /*
     * Check whether the double's significand changes when the error is added
     * or substracted.
     */
    magnitude = STOD_DIYFP_SIGNIFICAND_SIZE + value.exp;
    prec_digits = STOD_DIYFP_SIGNIFICAND_SIZE - stod_diyfp_sgnd_size(magnitude);
    if(prec_digits + STOD_DENOM_LOG >= STOD_DIYFP_SIGNIFICAND_SIZE)
    {
        /*
         * This can only happen for very small denormals. In this case the
         * half-way multiplied by the denominator exceeds the range of uint64.
         * Simply shift everything to the right.
         */
        shift = prec_digits + STOD_DENOM_LOG - STOD_DIYFP_SIGNIFICAND_SIZE + 1;
        value = stod_diyfp_shift_right(value, shift);
        /*
         * Add 1 for the lost precision of error, and STOD_DENOM
         * for the lost precision of value.significand.
         */
        error = (error >> shift) + 1 + STOD_DENOM;
        prec_digits -= shift;
    }
    prec_bits = value.significand & (((uint64_t)1 << prec_digits) - 1);
    prec_bits *= STOD_DENOM;
    half_way = (uint64_t)1 << (prec_digits - 1);
    half_way *= STOD_DENOM;
    rounded = stod_diyfp_shift_right(value, prec_digits);
    if(prec_bits >= half_way + error)
    {
        rounded.significand++;
    }
    return stod_diyfp2d(rounded);
}

static double stod_strtod_internal(const unsigned char* start, size_t length, int exp)
{
    int shift;
    size_t left;
    size_t right;
    const unsigned char* p;
    const unsigned char* e;
    const unsigned char* b;
    /* Trim leading zeroes. */
    p = start;
    e = p + length;
    while(p < e)
    {
        if(*p != '0')
        {
            start = p;
            break;
        }
        p++;
    }
    left = e - p;
    /* Trim trailing zeroes. */
    b = start;
    p = b + left - 1;
    while(p > b)
    {
        if(*p != '0')
        {
            break;
        }

        p--;
    }
    right = p - b + 1;
    length = right;
    if(length == 0)
    {
        return 0.0;
    }
    shift = (int)(left - right);
    if(exp >= STOD_DECIMAL_POWER_MAX - shift - (int)length + 1)
    {
        return -1;
    }
    if(exp <= STOD_DECIMAL_POWER_MIN - shift - (int)length)
    {
        return 0.0;
    }
    return stod_diyfp_strtod(start, length, exp + shift);
}

double stod_strtod(const unsigned char** start, const unsigned char* end, int literal)
{
    int exponent;
    int exp;
    int insignf;
    unsigned char c;
    unsigned char* pos;
    int minus;
    const unsigned char* e;
    const unsigned char* p;
    const unsigned char* last;
    const unsigned char* _;
    unsigned char data[128];
    exponent = 0;
    insignf = 0;
    pos = data;
    last = data + sizeof(data);
    p = *start;
    _ = p - 2;
    for(; p < end; p++)
    {
        /* Values less than '0' become >= 208. */
        c = *p - '0';
        if(stod_slow_path(c > 9))
        {
            if(literal)
            {
                if((p - _) == 1)
                {
                    goto done;
                }
                if(*p == '_')
                {
                    _ = p;
                    continue;
                }
            }
            break;
        }
        if(pos < last)
        {
            *pos++ = *p;
        }
        else
        {
            insignf++;
        }
    }
    /* Do not emit a '.', but adjust the exponent instead. */
    if(p < end && *p == '.')
    {
        _ = p;
        for(p++; p < end; p++)
        {
            /* Values less than '0' become >= 208. */
            c = *p - '0';
            if(stod_slow_path(c > 9))
            {
                if(literal && *p == '_' && (p - _) > 1)
                {
                    _ = p;
                    continue;
                }
                break;
            }
            if(pos < last)
            {
                *pos++ = *p;
                exponent--;
            }
            else
            {
                /* Ignore insignificant digits in the fractional part. */
            }
        }
    }
    if(pos == data)
    {
        return STOD_CONST_NAN;
    }
    e = p + 1;
    if(e < end && (*p == 'e' || *p == 'E'))
    {
        minus = 0;
        if(e + 1 < end)
        {
            if(*e == '-')
            {
                e++;
                minus = 1;
            }
            else if(*e == '+')
            {
                e++;
            }
        }
        /* Values less than '0' become >= 208. */
        c = *e - '0';
        if(stod_fast_path(c <= 9))
        {
            exp = c;
            for(p = e + 1; p < end; p++)
            {
                /* Values less than '0' become >= 208. */
                c = *p - '0';
                if(stod_slow_path(c > 9))
                {
                    if(literal && *p == '_' && (p - _) > 1)
                    {
                        _ = p;
                        continue;
                    }
                    break;
                }
                if(exp < (INT_MAX - 9) / 10)
                {
                    exp = exp * 10 + c;
                }
            }
            exponent += minus ? -exp : exp;
        }
        else if(literal && *e == '_')
        {
            p = e;
        }
    }
done:
    *start = p;
    exponent += insignf;
    return stod_strtod_internal(data, pos - data, exponent);
}

