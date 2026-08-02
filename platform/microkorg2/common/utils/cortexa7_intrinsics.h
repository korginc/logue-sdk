#include <stdint.h>

#ifdef __EMSCRIPTEN__
// websim: the Cortex-A7 DSP ops below are inline ARM assembly that clang cannot
// assemble for the wasm target. Provide portable equivalents with matching
// saturating semantics. SEL depends on the APSR.GE flags set by a preceding SIMD
// instruction, which cannot be reproduced standalone, so it returns its first
// operand (the same compromise as the gen-1 __SEL shim in
// websim/dsp/legacy/arm_math.h). See WEBSIM_FOLLOWUP_PLAN.md §A.

static inline int16_t __websim_sat16(int32_t v) {
    return (int16_t)(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
}
static inline int32_t __websim_qadd32(int32_t a, int32_t b) {
    int64_t s = (int64_t)a + (int64_t)b;
    return (int32_t)(s > INT32_MAX ? INT32_MAX : (s < INT32_MIN ? INT32_MIN : s));
}
static inline int32_t __websim_qsub32(int32_t a, int32_t b) {
    int64_t s = (int64_t)a - (int64_t)b;
    return (int32_t)(s > INT32_MAX ? INT32_MAX : (s < INT32_MIN ? INT32_MIN : s));
}
// Packed dual-16 saturating add/sub (QADD16/QSUB16 on a full 32-bit register).
static inline int32_t __websim_dual16(int32_t a, int32_t b, int sub) {
    int32_t blo = (int32_t)(int16_t)(b & 0xFFFF), bhi = (int32_t)(int16_t)(b >> 16);
    int16_t lo = __websim_sat16((int32_t)(int16_t)(a & 0xFFFF) + (sub ? -blo : blo));
    int16_t hi = __websim_sat16((int32_t)(int16_t)(a >> 16)    + (sub ? -bhi : bhi));
    return (int32_t)(((uint32_t)(uint16_t)lo) | ((uint32_t)(uint16_t)hi << 16));
}

inline int16_t sadd16(int16_t a, int16_t b) { return (int16_t)(a + b); }
inline int16_t qadd16(int16_t a, int16_t b) { return __websim_sat16((int32_t)a + (int32_t)b); }
inline int32_t qadd16_32(int32_t a, int32_t b) { return __websim_dual16(a, b, 0); }
inline int16_t qsub16(int16_t a, int16_t b) { return __websim_sat16((int32_t)a - (int32_t)b); }
inline int32_t qsub16_32(int32_t a, int32_t b) { return __websim_dual16(a, b, 1); }
inline int16_t sel(int16_t a, int16_t b) { (void)b; return a; }
inline int32_t qadd(int32_t a, int32_t b) { return __websim_qadd32(a, b); }
inline int32_t qsub(int32_t a, int32_t b) { return __websim_qsub32(a, b); }
inline int32_t sel_32(int32_t a, int32_t b) { (void)b; return a; }

#else

inline int16_t sadd16(int16_t a, int16_t b)
{
    int16_t res;
    __asm ("SADD16 %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
}

inline int16_t qadd16(int16_t a, int16_t b)
{
    int16_t res;
    __asm ("QADD16 %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)                   
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
} 

inline int32_t qadd16_32(int32_t a, int32_t b)
{
    int32_t res;
    __asm ("QADD16 %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)                   
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
} 

inline int16_t qsub16(int16_t a, int16_t b)
{
    int16_t res;
    __asm ("QSUB16 %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)                   
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
} 

inline int32_t qsub16_32(int32_t a, int32_t b)
{
    int32_t res;
    __asm ("QSUB16 %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)                   
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
} 

inline int16_t sel(int16_t a, int16_t b)
{
    int16_t res;
    __asm ("SEL %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)                   
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
}

// 32 bit
inline int32_t qadd(int32_t a, int32_t b)
{
    int32_t res;
    __asm ("QADD %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)                   
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
} 

inline int32_t qsub(int32_t a, int32_t b)
{
    int32_t res;
    __asm ("QSUB %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)                   
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
} 

inline int32_t sel_32(int32_t a, int32_t b)
{
    int32_t res;
    __asm ("SEL %[result], %[input_i], %[input_j]"
           : [result] "=r" (res)
           : [input_i] "r" (a), [input_j] "r" (b));
    return res;
}

#endif // __EMSCRIPTEN__