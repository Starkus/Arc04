#if defined(WIN32) || defined(_WIN32)
#define TARGET_WINDOWS 1
#else
#define TARGET_WINDOWS 0
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#if !TARGET_WINDOWS
#include <assert.h>
#define MAX_PATH PATH_MAX
#endif

// I hate this programming language
#undef near
#undef far

#define EPA_LOGGING 0
#define EPA_ERROR_LOGGING 1

#if DEBUG_BUILD
#define DEBUG_ONLY(...) __VA_ARGS__
#else
#define DEBUG_ONLY(...)
#endif

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

#if DEBUG_BUILD
#if TARGET_WINDOWS
#define ASSERT(expr) do { if (!(expr)) __debugbreak(); } while (false)
#else
#define ASSERT(expr) assert(expr)
#endif
#else
#define ASSERT(expr)
#endif

#define NOMANGLE extern "C"

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))
