/* CpuArch.h -- CPU specific code
2022-07-15 : Igor Pavlov : Public domain */

#ifndef __CPU_ARCH_H
#define __CPU_ARCH_H

#include "7zTypes.h"

EXTERN_C_BEGIN

/*
MY_CPU_LE means that CPU is LITTLE ENDIAN.
MY_CPU_BE means that CPU is BIG ENDIAN.
If MY_CPU_LE and MY_CPU_BE are not defined, we don't know about ENDIANNESS of platform.

MY_CPU_LE_UNALIGN means that CPU is LITTLE ENDIAN and CPU supports unaligned memory accesses.

MY_CPU_64BIT means that processor can work with 64-bit registers.
  MY_CPU_64BIT can be used to select fast code branch
  MY_CPU_64BIT doesn't mean that (sizeof(void *) == 8)
*/

#if  defined(_M_X64) \
  || defined(_M_AMD64) \
  || defined(__x86_64__) \
  || defined(__AMD64__) \
  || defined(__amd64__)
  #define MY_CPU_AMD64
    #define MY_CPU_NAME "x64"
    #define MY_CPU_SIZEOF_POINTER 8
  #define MY_CPU_64BIT
#endif

#if defined(MY_CPU_X86) || defined(MY_CPU_AMD64)
#define MY_CPU_X86_OR_AMD64
#endif

#if defined(MY_CPU_X86_OR_AMD64) \
    || defined(__LITTLE_ENDIAN__) \
    || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
  #define MY_CPU_LE
#endif

#ifdef _MSC_VER
  #if _MSC_VER >= 1300
    #define MY_CPU_pragma_pack_push_1   __pragma(pack(push, 1))
    #define MY_CPU_pragma_pop           __pragma(pack(pop))
  #else
    #define MY_CPU_pragma_pack_push_1
    #define MY_CPU_pragma_pop
  #endif
#else
  #ifdef __xlC__
    #define MY_CPU_pragma_pack_push_1   _Pragma("pack(1)")
    #define MY_CPU_pragma_pop           _Pragma("pack()")
  #else
    #define MY_CPU_pragma_pack_push_1   _Pragma("pack(push, 1)")
    #define MY_CPU_pragma_pop           _Pragma("pack(pop)")
  #endif
#endif

#ifndef MY_CPU_NAME
  #ifdef MY_CPU_LE
    #define MY_CPU_NAME "LE"
  #elif defined(MY_CPU_BE)
    #define MY_CPU_NAME "BE"
  #else
    /*
    #define MY_CPU_NAME ""
    */
  #endif
#endif


#ifdef MY_CPU_LE
  #if defined(MY_CPU_X86_OR_AMD64)
    #define MY_CPU_LE_UNALIGN
    #define MY_CPU_LE_UNALIGN_64
#endif
#endif

#ifdef MY_CPU_LE_UNALIGN

#define GetUi16(p) (*(const uint16_t *)(const void *)(p))
#define GetUi32(p) (*(const uint32_t *)(const void *)(p))
#ifdef MY_CPU_LE_UNALIGN_64
#define GetUi64(p) (*(const uint64_t  *)(const void *)(p))
#endif

#define SetUi16(p, v) { *(uint16_t *)(void *)(p) = (v); }
#define SetUi32(p, v) { *(unsigned int *)(void *)(p) = (v); }
#ifdef MY_CPU_LE_UNALIGN_64
#define SetUi64(p, v) { *(uint64_t *)(void *)(p) = (v); }
#endif

#else

#define GetUi16(p) ( (uint16_t) ( \
             ((const unsigned char *)(p))[0] | \
    ((uint16_t)((const unsigned char *)(p))[1] << 8) ))

#define GetUi32(p) ( \
             ((const unsigned char *)(p))[0]        | \
    ((uint32_t)((const unsigned char *)(p))[1] <<  8) | \
    ((uint32_t)((const unsigned char *)(p))[2] << 16) | \
    ((uint32_t)((const unsigned char *)(p))[3] << 24))

#define SetUi16(p, v) { unsigned char *_ppp_ = (unsigned char *)(p); uint32_t _vvv_ = (v); \
    _ppp_[0] = (unsigned char)_vvv_; \
    _ppp_[1] = (unsigned char)(_vvv_ >> 8); }

#define SetUi32(p, v) { unsigned char *_ppp_ = (unsigned char *)(p); uint32_t _vvv_ = (v); \
    _ppp_[0] = (unsigned char)_vvv_; \
    _ppp_[1] = (unsigned char)(_vvv_ >> 8); \
    _ppp_[2] = (unsigned char)(_vvv_ >> 16); \
    _ppp_[3] = (unsigned char)(_vvv_ >> 24); }

#endif


#ifndef MY_CPU_LE_UNALIGN_64

#define GetUi64(p) (GetUi32(p) | ((uint64_t)GetUi32(((const unsigned char *)(p)) + 4) << 32))

#define SetUi64(p, v) { unsigned char *_ppp2_ = (unsigned char *)(p); uint64_t _vvv2_ = (v); \
    SetUi32(_ppp2_    , (uint32_t)_vvv2_); \
    SetUi32(_ppp2_ + 4, (uint32_t)(_vvv2_ >> 32)); }

#endif


#ifdef __has_builtin
  #define MY__has_builtin(x) __has_builtin(x)
#else
  #define MY__has_builtin(x) 0
#endif

#if defined(MY_CPU_LE_UNALIGN) && /* defined(_WIN64) && */ defined(_MSC_VER) && (_MSC_VER >= 1300)

/* Note: we use bswap instruction, that is unsupported in 386 cpu */

#include <stdlib.h>

#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)

/* #define GetBe16(p) _byteswap_ushort(*(const UInt16 *)(const Byte *)(p)) */
#define GetBe32(p) _byteswap_ulong (*(const uint32_t *)(const void *)(p))
#define GetBe64(p) _byteswap_uint64(*(const uint64_t *)(const void *)(p))

#define SetBe32(p, v) (*(unsigned int *)(void *)(p)) = _byteswap_ulong(v)

#elif defined(MY_CPU_LE_UNALIGN) && ( \
       (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))) \
    || (defined(__clang__) && MY__has_builtin(__builtin_bswap16)) )

/* #define GetBe16(p) __builtin_bswap16(*(const UInt16 *)(const void *)(p)) */
#define GetBe32(p) __builtin_bswap32(*(const uint32_t *)(const void *)(p))
#define GetBe64(p) __builtin_bswap64(*(const uint64_t *)(const void *)(p))

#define SetBe32(p, v) (*(uint32_t *)(void *)(p)) = __builtin_bswap32(v)

#else

#define GetBe32(p) ( \
    ((uint32_t)((const unsigned char *)(p))[0] << 24) | \
    ((uint32_t)((const unsigned char *)(p))[1] << 16) | \
    ((uint32_t)((const unsigned char *)(p))[2] <<  8) | \
             ((const unsigned char *)(p))[3] )

#define GetBe64(p) (((uint64_t)GetBe32(p) << 32) | GetBe32(((const unsigned char *)(p)) + 4))

#define SetBe32(p, v) { unsigned char *_ppp_ = (unsigned char *)(p); uint32_t _vvv_ = (v); \
    _ppp_[0] = (unsigned char)(_vvv_ >> 24); \
    _ppp_[1] = (unsigned char)(_vvv_ >> 16); \
    _ppp_[2] = (unsigned char)(_vvv_ >> 8); \
    _ppp_[3] = (unsigned char)_vvv_; }

#endif


#ifndef GetBe16

#define GetBe16(p) ( (uint16_t) ( \
    ((uint16_t)((const unsigned char *)(p))[0] << 8) | \
             ((const unsigned char *)(p))[1] ))

#endif



#ifdef MY_CPU_X86_OR_AMD64

typedef struct
{
  unsigned int maxFunc;
  unsigned int vendor[3];
  unsigned int ver;
  unsigned int b;
  unsigned int c;
  unsigned int d;
} Cx86cpuid;

enum
{
  CPU_FIRM_INTEL,
  CPU_FIRM_AMD,
  CPU_FIRM_VIA
};

void MyCPUID(unsigned int function, unsigned int*a, unsigned int*b, unsigned int*c, unsigned int*d);

BoolInt x86cpuid_CheckAndRead(Cx86cpuid *p);
int x86cpuid_GetFirm(const Cx86cpuid *p);

#define x86cpuid_GetFamily(ver) (((ver >> 16) & 0xFF0) | ((ver >> 8) & 0xF))
#define x86cpuid_GetModel(ver)  (((ver >> 12) &  0xF0) | ((ver >> 4) & 0xF))
#define x86cpuid_GetStepping(ver) (ver & 0xF)

BoolInt CPU_IsSupported_SSSE3(void);
BoolInt CPU_IsSupported_SHA(void);

#endif

EXTERN_C_END

#endif
