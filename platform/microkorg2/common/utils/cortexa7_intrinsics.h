#include <stdint.h>

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