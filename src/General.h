#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

// I hate this programming language
#undef near
#undef far

#define GJK_VISUAL_DEBUGGING 0
#define EPA_VISUAL_DEBUGGING 0
#define EPA_LOGGING 0
#define EPA_ERROR_LOGGING 1

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define U8_MAX 0xFF
#define U16_MAX 0xFFFF
#define U32_MAX 0xFFFFFFFF
#define U64_MAX 0xFFFFFFFFFFFFFFFF

#define I8_MIN ((i8)0xFF)
#define I16_MIN ((i16)0xFFFF)
#define I32_MIN ((i32)0xFFFFFFFF)
#define I64_MIN ((i64)0xFFFFFFFFFFFFFFFF)

#define I8_MAX ((i8)0x7F)
#define I16_MAX ((i16)0x7FFF)
#define I32_MAX ((i32)0x7FFFFFFF)
#define I64_MAX ((i64)0x7FFFFFFFFFFFFFFF)

#define DEBUG_BUILD 1
#define DEBUG_ONLY(a) a

#if DEBUG_BUILD
#define ASSERT(expr) do { if (!(expr)) __debugbreak(); } while (false)
#else
#define ASSERT(expr)
#endif

#define NOMANGLE extern "C"

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))
