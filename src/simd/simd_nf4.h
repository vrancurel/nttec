/*
 * Copyright 2017-2018 Scality
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QUAD_SIMD_NF4_H__
#define __QUAD_SIMD_NF4_H__

namespace quadiron {
namespace simd {

typedef uint32_t aint32 __attribute__((aligned(ALIGNMENT)));

/** Return __uint128_t integer from a _m128i register */
static inline __uint128_t m128i_to_uint128(__m128i v)
{
    __uint128_t i;
    _mm_store_si128((__m128i*)&i, v);

    return i; // NOLINT(clang-analyzer-core.uninitialized.UndefReturn)
}

inline __uint128_t expand16(uint16_t* arr, int n)
{
    // since n <= 4
    uint16_t _arr[4] __attribute__((aligned(ALIGNMENT))) = {0, 0, 0, 0};
    std::copy_n(arr, n, _arr);

    __m128i b = _mm_set_epi16(0, 0, 0, 0, _arr[3], _arr[2], _arr[1], _arr[0]);

    return m128i_to_uint128(b);
}

inline __uint128_t expand32(uint32_t* arr, int n)
{
    // since n <= 4
    uint32_t _arr[4] __attribute__((aligned(simd::ALIGNMENT))) = {0, 0, 0, 0};
    std::copy_n(arr, n, _arr);

    __m128i b = _mm_set_epi32(_arr[3], _arr[2], _arr[1], _arr[0]);

    return m128i_to_uint128(b);
}

inline GroupedValues<__uint128_t> unpack(__uint128_t a, int n)
{
    uint16_t ai[8];
    __uint128_t values;

    __m128i _a = _mm_loadu_si128((__m128i*)&a);
    ai[0] = _mm_extract_epi16(_a, 0);
    ai[1] = _mm_extract_epi16(_a, 1);
    ai[2] = _mm_extract_epi16(_a, 2);
    ai[3] = _mm_extract_epi16(_a, 3);
    ai[4] = _mm_extract_epi16(_a, 4);
    ai[5] = _mm_extract_epi16(_a, 5);
    ai[6] = _mm_extract_epi16(_a, 6);
    ai[7] = _mm_extract_epi16(_a, 7);

    const uint32_t flag =
        ai[1] | (!!ai[3] << 1u) | (!!ai[5] << 2u) | (!!ai[7] << 3u);

    __m128i val = _mm_set_epi16(0, 0, 0, 0, ai[6], ai[4], ai[2], ai[0]);
    _mm_store_si128((__m128i*)&values, val);

    GroupedValues<__uint128_t> b = {values, flag};

    return b;
}

inline void unpack(__uint128_t a, GroupedValues<__uint128_t>& b, int n)
{
    uint16_t ai[8];
    __uint128_t values;

    __m128i _a = _mm_loadu_si128((__m128i*)&a);
    ai[0] = _mm_extract_epi16(_a, 0);
    ai[1] = _mm_extract_epi16(_a, 1);
    ai[2] = _mm_extract_epi16(_a, 2);
    ai[3] = _mm_extract_epi16(_a, 3);
    ai[4] = _mm_extract_epi16(_a, 4);
    ai[5] = _mm_extract_epi16(_a, 5);
    ai[6] = _mm_extract_epi16(_a, 6);
    ai[7] = _mm_extract_epi16(_a, 7);

    const uint32_t flag =
        ai[1] | (!!ai[3] << 1u) | (!!ai[5] << 2u) | (!!ai[7] << 3u);

    __m128i val = _mm_set_epi16(0, 0, 0, 0, ai[6], ai[4], ai[2], ai[0]);
    _mm_store_si128((__m128i*)&values, val);

    b.flag = flag;
    b.values = values; // NOLINT(clang-analyzer-core.uninitialized.Assign)
}

inline __uint128_t pack(__uint128_t a)
{
    __m128i _a = _mm_loadu_si128((__m128i*)&a);
    __m128i b = _mm_set_epi32(
        _mm_extract_epi16(_a, 3),
        _mm_extract_epi16(_a, 2),
        _mm_extract_epi16(_a, 1),
        _mm_extract_epi16(_a, 0));

    return m128i_to_uint128(b);
}

inline __uint128_t pack(__uint128_t a, uint32_t flag)
{
    aint32 b0, b1, b2, b3;
    __m128i _a = _mm_loadu_si128((__m128i*)&a);

    if (flag & 1)
        b0 = 65536;
    else
        b0 = _mm_extract_epi16(_a, 0);
    flag >>= 1;
    if (flag & 1)
        b1 = 65536;
    else
        b1 = _mm_extract_epi16(_a, 1);
    flag >>= 1;
    if (flag & 1)
        b2 = 65536;
    else
        b2 = _mm_extract_epi16(_a, 2);
    flag >>= 1;
    if (flag & 1)
        b3 = 65536;
    else
        b3 = _mm_extract_epi16(_a, 3);

    __m128i b = _mm_set_epi32(b3, b2, b1, b0);

    return m128i_to_uint128(b);
}

/* ================= Basic operations for NF4 ================= */

#if defined(__AVX2__)

inline VecType LoadToReg(HalfVecType x)
{
    return _mm256_castsi128_si256(_mm_load_si128(&x));
}

inline VecType LoadToReg(__uint128_t x)
{
    const HalfVecType* _x = reinterpret_cast<const HalfVecType*>(&x);
    return LoadToReg(*_x);
}

inline void StoreLowHalfToMem(HalfVecType* address, VecType reg)
{
    _mm_store_si128(address, _mm256_castsi256_si128(reg));
}

inline __uint128_t add(__uint128_t a, __uint128_t b)
{
    HalfVecType res;
    VecType _a = LoadToReg(a);
    VecType _b = LoadToReg(b);
    StoreLowHalfToMem(&res, ModAdd(_a, _b, F4));
    return reinterpret_cast<__uint128_t>(res);
}

inline __uint128_t sub(__uint128_t a, __uint128_t b)
{
    HalfVecType res;
    VecType _a = LoadToReg(a);
    VecType _b = LoadToReg(b);
    StoreLowHalfToMem(&res, ModSub(_a, _b, F4));
    return reinterpret_cast<__uint128_t>(res);
}

inline __uint128_t mul(__uint128_t a, __uint128_t b)
{
    HalfVecType res;
    VecType _a = LoadToReg(a);
    VecType _b = LoadToReg(b);
    StoreLowHalfToMem(&res, ModMulSafe(_a, _b, F4));
    return reinterpret_cast<__uint128_t>(res);
}

inline void add_buf_to_two_bufs_rem(
    unsigned n,
    __uint128_t* x,
    __uint128_t* x_half,
    __uint128_t* y)
{
    // add last _y[] to x and x_next
    HalfVecType* _x = reinterpret_cast<HalfVecType*>(x);
    HalfVecType* _x_half = reinterpret_cast<HalfVecType*>(x_half);
    HalfVecType* _y = reinterpret_cast<HalfVecType*>(y);
    for (unsigned i = 0; i < n; ++i) {
        VecType _x_p = LoadToReg(_x[i]);
        VecType _x_next_p = LoadToReg(_x_half[i]);
        VecType _y_p = LoadToReg(_y[i]);

        StoreLowHalfToMem(_x + i, ModAdd(_x_p, _y_p, F4));
        StoreLowHalfToMem(_x_half + i, ModAdd(_x_next_p, _y_p, F4));
    }
}

inline void hadamard_mul_rem(unsigned n, __uint128_t* x, __uint128_t* y)
{
    HalfVecType* _x = reinterpret_cast<HalfVecType*>(x);
    HalfVecType* _y = reinterpret_cast<HalfVecType*>(y);
    for (unsigned i = 0; i < n; ++i) {
        VecType _x_p = LoadToReg(_x[i]);
        VecType _y_p = LoadToReg(_y[i]);

        StoreLowHalfToMem(_x + i, ModMulSafe(_x_p, _y_p, F4));
    }
}

inline void hadamard_mul_doubled_rem(
    unsigned n,
    __uint128_t* x,
    __uint128_t* x_half,
    __uint128_t* y)
{
    HalfVecType* _x = reinterpret_cast<HalfVecType*>(x);
    HalfVecType* _x_half = reinterpret_cast<HalfVecType*>(x_half);
    HalfVecType* _y = reinterpret_cast<HalfVecType*>(y);
    for (unsigned i = 0; i < n; ++i) {
        VecType _x_p = LoadToReg(_x[i]);
        VecType _x_next_p = LoadToReg(_x_half[i]);
        VecType _y_p = LoadToReg(_y[i]);

        StoreLowHalfToMem(_x + i, ModMulSafe(_x_p, _y_p, F4));
        StoreLowHalfToMem(_x_half + i, ModMulSafe(_x_next_p, _y_p, F4));
    }
}

#elif defined(__SSE4_1__)

inline VecType LoadToReg(__uint128_t x)
{
    const VecType* _x = reinterpret_cast<const VecType*>(&x);
    return _mm_load_si128(_x);
}

inline __uint128_t add(__uint128_t a, __uint128_t b)
{
    VecType res;
    VecType _a = LoadToReg(a);
    VecType _b = LoadToReg(b);
    StoreToMem(&res, ModAdd(_a, _b, F4));
    return reinterpret_cast<__uint128_t>(res);
}

inline __uint128_t sub(__uint128_t a, __uint128_t b)
{
    VecType res;
    VecType _a = LoadToReg(a);
    VecType _b = LoadToReg(b);
    StoreToMem(&res, ModSub(_a, _b, F4));
    return reinterpret_cast<__uint128_t>(res);
}

inline __uint128_t mul(__uint128_t a, __uint128_t b)
{
    VecType res;
    VecType _a = LoadToReg(a);
    VecType _b = LoadToReg(b);
    StoreToMem(&res, ModMulSafe(_a, _b, F4));
    return reinterpret_cast<__uint128_t>(res);
}

inline void add_buf_to_two_bufs_rem(
    unsigned n,
    __uint128_t* x,
    __uint128_t* x_half,
    __uint128_t* y)
{
    // do nothing
}

inline void hadamard_mul_rem(unsigned n, __uint128_t* x, __uint128_t* y)
{
    // do nothing
}

inline void hadamard_mul_doubled_rem(
    unsigned n,
    __uint128_t* x,
    __uint128_t* x_half,
    __uint128_t* y)
{
    // do nothing
}

#endif

/* ==================== Operations for NF4 =================== */

/** Add buffer `y` to two halves of `x`. `x` is of length `n` */
inline void add_buf_to_two_bufs(unsigned n, __uint128_t* _x, __uint128_t* _y)
{
    unsigned i;
    VecType* x = reinterpret_cast<VecType*>(_x);
    VecType* y = reinterpret_cast<VecType*>(_y);

    const unsigned ratio = sizeof(*x) / sizeof(*_x);
    const unsigned half_len = n / 2;
    const unsigned vec_len = half_len / ratio;
    const unsigned num_len = vec_len * ratio;
    const unsigned rem_len = half_len - num_len;

    __uint128_t* x_half = _x + half_len;
    VecType* x_next = reinterpret_cast<VecType*>(x_half);

    // add y to the first half of `x`
    for (i = 0; i < vec_len; ++i) {
        x[i] = ModAdd(x[i], y[i], F4);
    }

    // add y to the second half of `x`
    for (i = 0; i < vec_len; ++i) {
        x_next[i] = ModAdd(x_next[i], y[i], F4);
    }

    if (rem_len > 0) {
        add_buf_to_two_bufs_rem(
            rem_len, _x + num_len, x_half + num_len, _y + num_len);
    }
}

inline void hadamard_mul(unsigned n, __uint128_t* _x, __uint128_t* _y)
{
    unsigned i;
    VecType* x = reinterpret_cast<VecType*>(_x);
    VecType* y = reinterpret_cast<VecType*>(_y);

    const unsigned ratio = sizeof(*x) / sizeof(*_x);
    const unsigned vec_len = n / ratio;
    const unsigned num_len = vec_len * ratio;
    const unsigned rem_len = n - num_len;

    // multiply y to the first half of `x`
    for (i = 0; i < vec_len; ++i) {
        x[i] = ModMulSafe(x[i], y[i], F4);
    }

    if (rem_len > 0) {
        // add last _y[] to x
        hadamard_mul_rem(rem_len, _x + num_len, _y + num_len);
    }
}

} // namespace simd
} // namespace quadiron

#endif
