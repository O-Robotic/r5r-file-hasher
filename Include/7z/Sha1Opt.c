/* Sha1Opt.c -- SHA-1 optimized code for SHA-1 hardware instructions
2021-04-01 : Igor Pavlov : Public domain */

#include "CpuArch.h"

#ifdef MY_CPU_X86_OR_AMD64
  #if defined(_MSC_VER)
    #ifdef USE_MY_MM
      #define USE_VER_MIN 1300
    #else
      #define USE_VER_MIN 1910
    #endif
    #if _MSC_VER >= USE_VER_MIN
      #define USE_HW_SHA
    #endif
  #endif
// #endif // MY_CPU_X86_OR_AMD64

#ifdef USE_HW_SHA


#if !defined(_MSC_VER) || (_MSC_VER >= 1900)
#include <immintrin.h>
#else
#include <emmintrin.h>
#endif

/*
SHA1 uses:
SSE2:
  _mm_loadu_si128
  _mm_storeu_si128
  _mm_set_epi32
  _mm_add_epi32
  _mm_shuffle_epi32 / pshufd
  _mm_xor_si128
  _mm_cvtsi128_si32
  _mm_cvtsi32_si128
SSSE3:
  _mm_shuffle_epi8 / pshufb

SHA:
  _mm_sha1*
*/

#define ADD_EPI32(dest, src)      dest = _mm_add_epi32(dest, src);
#define XOR_SI128(dest, src)      dest = _mm_xor_si128(dest, src);
#define SHUFFLE_EPI8(dest, mask)  dest = _mm_shuffle_epi8(dest, mask);
#define SHUFFLE_EPI32(dest, mask) dest = _mm_shuffle_epi32(dest, mask);

#define SHA1_RND4(abcd, e0, f)  abcd = _mm_sha1rnds4_epu32(abcd, e0, f);
#define SHA1_NEXTE(e, m)        e = _mm_sha1nexte_epu32(e, m);



#define SHA1_MSG1(dest, src) dest = _mm_sha1msg1_epu32(dest, src);
#define SHA1_MSG2(dest, src) dest = _mm_sha1msg2_epu32(dest, src);


#define LOAD_SHUFFLE(m, k) \
    m = _mm_loadu_si128((const __m128i *)(const void *)(data + (k) * 16)); \
    SHUFFLE_EPI8(m, mask); \

#define SM1(m0, m1, m2, m3) \
    SHA1_MSG1(m0, m1); \

#define SM2(m0, m1, m2, m3) \
    XOR_SI128(m3, m1); \
    SHA1_MSG2(m3, m2); \

#define SM3(m0, m1, m2, m3) \
    XOR_SI128(m3, m1); \
    SM1(m0, m1, m2, m3) \
    SHA1_MSG2(m3, m2); \

#define NNN(m0, m1, m2, m3)


#define R4(k, e0, e1, m0, m1, m2, m3, OP) \
    e1 = abcd; \
    SHA1_RND4(abcd, e0, (k) / 5); \
    SHA1_NEXTE(e1, m1); \
    OP(m0, m1, m2, m3); \

#define R16(k, mx, OP0, OP1, OP2, OP3) \
    R4 ( (k)*4+0, e0,e1, m0,m1,m2,m3, OP0 ) \
    R4 ( (k)*4+1, e1,e0, m1,m2,m3,m0, OP1 ) \
    R4 ( (k)*4+2, e0,e1, m2,m3,m0,m1, OP2 ) \
    R4 ( (k)*4+3, e1,e0, m3,mx,m1,m2, OP3 ) \

#define PREPARE_STATE \
    SHUFFLE_EPI32 (abcd, 0x1B); \
    SHUFFLE_EPI32 (e0,   0x1B); \

void __fastcall Sha1_UpdateBlocks_HW(unsigned int state[5], const unsigned char *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void __fastcall Sha1_UpdateBlocks_HW(unsigned int state[5], const unsigned char *data, size_t numBlocks)
{
  const __m128i mask = _mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);

  __m128i abcd, e0;
  
  if (numBlocks == 0)
    return;
  
  abcd = _mm_loadu_si128((const __m128i *) (const void *) &state[0]); // dbca
  e0 = _mm_cvtsi32_si128((int)state[4]); // 000e
  
  PREPARE_STATE
  
  do
  {
    __m128i abcd_save, e2;
    __m128i m0, m1, m2, m3;
    __m128i e1;
    

    abcd_save = abcd;
    e2 = e0;

    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)

    ADD_EPI32(e0, m0);
    
    R16 ( 0, m0, SM1, SM3, SM3, SM3 );
    R16 ( 1, m0, SM3, SM3, SM3, SM3 );
    R16 ( 2, m0, SM3, SM3, SM3, SM3 );
    R16 ( 3, m0, SM3, SM3, SM3, SM3 );
    R16 ( 4, e2, SM2, NNN, NNN, NNN );
    
    ADD_EPI32(abcd, abcd_save);
    
    data += 64;
  }
  while (--numBlocks);

  PREPARE_STATE

  _mm_storeu_si128((__m128i *) (void *) state, abcd);
  *(state+4) = (int)_mm_cvtsi128_si32(e0);
}

#endif // USE_HW_SHA

#ifndef USE_HW_SHA

// #error Stop_Compiling_UNSUPPORTED_SHA
// #include <stdlib.h>

// #include "Sha1.h"
void __fastcall Sha1_UpdateBlocks(unsigned int state[5], const Byte *data, size_t numBlocks);

#pragma message("Sha1   HW-SW stub was used")

void __fastcall Sha1_UpdateBlocks_HW(unsigned int state[5], const Byte *data, size_t numBlocks);
void __fastcall Sha1_UpdateBlocks_HW(unsigned int state[5], const Byte *data, size_t numBlocks)
{
  Sha1_UpdateBlocks(state, data, numBlocks);
  /*
  UNUSED_VAR(state);
  UNUSED_VAR(data);
  UNUSED_VAR(numBlocks);
  exit(1);
  return;
  */
}

#endif

#endif