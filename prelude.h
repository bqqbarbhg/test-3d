#ifndef PRELUDE_H
#define PRELUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>
typedef int8_t I8;
typedef uint8_t U8;
typedef int16_t I16;
typedef uint16_t U16;
typedef int32_t I32;
typedef uint32_t U32;
typedef int64_t I64;
typedef uint64_t U64;
#define Count(array) (sizeof(array)/sizeof(*(array)))

#define KB(amount) ((amount) * 1024)
#define MB(amount) (KB(amount) * 1024)

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

#define FLT_PI 3.14159f

#endif
