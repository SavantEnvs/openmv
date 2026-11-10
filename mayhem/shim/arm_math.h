// Host stand-in for CMSIS arm_math.h: only the saturation intrinsics imlib uses.
#ifndef SHIM_ARM_MATH_H
#define SHIM_ARM_MATH_H
#include <stdint.h>
#include <math.h>

typedef float float32_t;
typedef double float64_t;
typedef int8_t q7_t;
typedef int16_t q15_t;
typedef int32_t q31_t;
typedef int64_t q63_t;

static inline int32_t __SSAT(int32_t v, uint32_t sat) {
    int32_t max = (int32_t) ((1U << (sat - 1U)) - 1U);
    int32_t min = -1 - max;
    return v > max ? max : (v < min ? min : v);
}

static inline uint32_t __USAT(int32_t v, uint32_t sat) {
    uint32_t max = ((1U << sat) - 1U);
    return v > (int32_t) max ? max : (v < 0 ? 0 : (uint32_t) v);
}

// Unsigned saturate of both signed 16-bit halves to `sat` bits.
static inline uint32_t __USAT16(uint32_t v, uint32_t sat) {
    uint32_t lo = __USAT((int16_t) (v & 0xffff), sat);
    uint32_t hi = __USAT((int16_t) (v >> 16), sat);
    return (hi << 16) | lo;
}

// Packed signed 16-bit subtract (GE flags are not modelled).
static inline uint32_t __SSUB16(uint32_t a, uint32_t b) {
    uint16_t lo = (uint16_t) ((int16_t) a - (int16_t) b);
    uint16_t hi = (uint16_t) ((int16_t) (a >> 16) - (int16_t) (b >> 16));
    return ((uint32_t) hi << 16) | lo;
}

// Dual signed 16-bit multiply, summed with the accumulator.
static inline uint32_t __SMLAD(uint32_t a, uint32_t b, uint32_t acc) {
    return (uint32_t) (((int16_t) a * (int16_t) b) + ((int16_t) (a >> 16) * (int16_t) (b >> 16)) + (int32_t) acc);
}

static inline uint32_t __PKHBT(uint32_t a, uint32_t b, uint32_t shift) {
    return (a & 0x0000ffffU) | ((b << shift) & 0xffff0000U);
}

static inline uint8_t __CLZ(uint32_t v) {
    return v ? (uint8_t) __builtin_clz(v) : 32;
}

// Byte-swap each 16-bit half.
static inline uint32_t __REV16(uint32_t v) {
    return ((v & 0x00ff00ffU) << 8) | ((v >> 8) & 0x00ff00ffU);
}

static inline uint32_t __SMUAD(uint32_t a, uint32_t b) {
    return (uint32_t) (((int16_t) a * (int16_t) b) + ((int16_t) (a >> 16) * (int16_t) (b >> 16)));
}

static inline uint32_t __USAT_ASR(int32_t v, uint32_t sat, uint32_t shift) {
    return __USAT(v >> (shift & 0x1f), sat);
}

static inline int32_t __SSAT_ASR(int32_t v, uint32_t sat, uint32_t shift) {
    return __SSAT(v >> (shift & 0x1f), sat);
}

static inline uint32_t __RBIT(uint32_t v) {
    uint32_t r = 0;
    for (int i = 0; i < 32; i++) {
        r = (r << 1) | ((v >> i) & 1U);
    }
    return r;
}

// Sum of absolute differences of the four unsigned byte lanes.
static inline uint32_t __USAD8(uint32_t a, uint32_t b) {
    uint32_t s = 0;
    for (int i = 0; i < 4; i++) {
        int d = (int) ((a >> (8 * i)) & 0xff) - (int) ((b >> (8 * i)) & 0xff);
        s += (uint32_t) (d < 0 ? -d : d);
    }
    return s;
}

static inline uint32_t __QADD16(uint32_t a, uint32_t b) {
    uint32_t r = 0;
    for (int i = 0; i < 2; i++) {
        int32_t s = (int16_t) (a >> (16 * i)) + (int16_t) (b >> (16 * i));
        s = s > 32767 ? 32767 : (s < -32768 ? -32768 : s);
        r |= ((uint32_t) s & 0xffff) << (16 * i);
    }
    return r;
}

#define arm_sqrt_f32(x, out) (*(out) = sqrtf(x), 0)
#endif
