// -----------------------------------------------------------------------------
// File:        firmware/include/axis-types.h
// Description: Crusan & Co. / AxisOS Pure Freestanding Fixed-Width Type Contract
//              (Zero <stdint.h>, <stdbool.h>, or <stddef.h> dependency).
// Author:      Marc F. Crusan (MFC) / Crusan & Co. w/ PP & AGY (AXEL)
// Date:        2026112301
// Version:     v1.0.0-Silicon
// -----------------------------------------------------------------------------
// 2026112301 (MFC/AGY) Created freestanding type header using compiler built-ins to eliminate <stdint.h>/<stdbool.h>/<stddef.h> across AxisOS-Firmware.

#ifndef AXIS_TYPES_H
#define AXIS_TYPES_H

/* Compiler-builtin exact-width types (works under -nostdinc on ARM, AArch64, x86_64, Xtensa) */
#if !defined(_STDINT_H) && !defined(_STDINT_H_) && !defined(__stdint_h) && !defined(_GCC_STDINT_H) && !defined(_SYS__STDINT_H)
#ifdef __UINT8_TYPE__
typedef __INT8_TYPE__   int8_t;
typedef __UINT8_TYPE__  uint8_t;
typedef __INT16_TYPE__  int16_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __INT32_TYPE__  int32_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __INT64_TYPE__  int64_t;
typedef __UINT64_TYPE__ uint64_t;
#else
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef signed short       int16_t;
typedef unsigned short     uint16_t;
typedef signed int         int32_t;
typedef unsigned int       uint32_t;
typedef signed long long   int64_t;
typedef unsigned long long uint64_t;
#endif
#endif

#if !defined(_SIZE_T) && !defined(_SIZE_T_DEFINED) && !defined(__size_t__) && !defined(_BSD_SIZE_T_DEFINED_) && !defined(__SIZE_T_DEFINED)
#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ size_t;
#else
typedef unsigned long size_t;
#endif
#define _SIZE_T
#define _SIZE_T_DEFINED
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef __cplusplus
#if !defined(__bool_true_false_are_defined) && !defined(bool)
#define bool  _Bool
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
#endif
#endif

#endif /* AXIS_TYPES_H */
