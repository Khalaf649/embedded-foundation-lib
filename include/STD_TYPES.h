//
// Created by khalaf on 3/23/26.
//

#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>

/* A1: Unsigned integer type aliases */
typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;

/* A1: Signed integer type aliases */
typedef int8_t    sint8;
typedef int16_t   sint16;
typedef int32_t   sint32;

/* A1: Optimized Boolean type*/
typedef uint8 boolean;

enum {
   FALSE = 0,
   TRUE  = 1
};

/* A1: Standard function return type */
typedef uint8 Std_ReturnType;
#define E_OK      ((Std_ReturnType)0)
#define E_NOT_OK  ((Std_ReturnType)1)

/* A1: Generic pointer type */
typedef void* P_void;

#endif /* STD_TYPES_H */