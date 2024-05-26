#ifndef U_INT_H_
#define U_INT_H_

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#if defined(__STDC_IEC_559__) || defined (__ANDROID__)
typedef float f32;
typedef double f64;
#else
#error Platforms that do not conform to the IEC 60559 floating-point arithmetic standard are not supported.
#endif

#endif /* U_INT_H_ */
