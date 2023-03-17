/* CpuArch.c -- CPU specific code
2021-07-13 : Igor Pavlov : Public domain */

#include "CpuArch.h"

#ifdef MY_CPU_X86_OR_AMD64

#if (defined(_MSC_VER) && !defined(MY_CPU_AMD64)) || defined(__GNUC__)
#define USE_ASM
#endif

#if !defined(USE_ASM) && _MSC_VER >= 1500
#include <intrin.h>
#endif

#if defined(USE_ASM) && !defined(MY_CPU_AMD64)
static unsigned int CheckFlag(unsigned int flag)
{
  #ifdef _MSC_VER
  __asm pushfd;
  __asm pop EAX;
  __asm mov EDX, EAX;
  __asm xor EAX, flag;
  __asm push EAX;
  __asm popfd;
  __asm pushfd;
  __asm pop EAX;
  __asm xor EAX, EDX;
  __asm push EDX;
  __asm popfd;
  __asm and flag, EAX;
  #else
  __asm__ __volatile__ (
    "pushf\n\t"
    "pop  %%EAX\n\t"
    "movl %%EAX,%%EDX\n\t"
    "xorl %0,%%EAX\n\t"
    "push %%EAX\n\t"
    "popf\n\t"
    "pushf\n\t"
    "pop  %%EAX\n\t"
    "xorl %%EDX,%%EAX\n\t"
    "push %%EDX\n\t"
    "popf\n\t"
    "andl %%EAX, %0\n\t":
    "=c" (flag) : "c" (flag) :
    "%eax", "%edx");
  #endif
  return flag;
}
#define CHECK_CPUID_IS_SUPPORTED if (CheckFlag(1 << 18) == 0 || CheckFlag(1 << 21) == 0) return False;
#else
#define CHECK_CPUID_IS_SUPPORTED
#endif

#ifndef USE_ASM
  #ifdef _MSC_VER
    #if _MSC_VER >= 1600
      #define MY__cpuidex  __cpuidex
    #else

/*
 __cpuid (function == 4) requires subfunction number in ECX.
  MSDN: The __cpuid intrinsic clears the ECX register before calling the cpuid instruction.
   __cpuid() in new MSVC clears ECX.
   __cpuid() in old MSVC (14.00) doesn't clear ECX
 We still can use __cpuid for low (function) values that don't require ECX,
 but __cpuid() in old MSVC will be incorrect for some function values: (function == 4).
 So here we use the hack for old MSVC to send (subFunction) in ECX register to cpuid instruction,
 where ECX value is first parameter for FAST_CALL / NO_INLINE function,
 So the caller of MY__cpuidex_HACK() sets ECX as subFunction, and
 old MSVC for __cpuid() doesn't change ECX and cpuid instruction gets (subFunction) value.
 
 DON'T remove MY_NO_INLINE and MY_FAST_CALL for MY__cpuidex_HACK() !!!
*/

static
__declspec(noinline)
void __fastcall MY__cpuidex_HACK(unsigned int subFunction, int *CPUInfo, unsigned int function)
{
  UNUSED_VAR(subFunction);
  __cpuid(CPUInfo, function);
}

      #define MY__cpuidex(info, func, func2)  MY__cpuidex_HACK(func2, info, func)
      #pragma message("======== MY__cpuidex_HACK WAS USED ========")
    #endif
  #else
     #define MY__cpuidex(info, func, func2)  __cpuid(info, func)
     #pragma message("======== (INCORRECT ?) cpuid WAS USED ========")
  #endif
#endif


void MyCPUID(unsigned int function, unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d)
{
  
  int CPUInfo[4];

  MY__cpuidex(CPUInfo, (int)function, 0);

  *a = (unsigned int)CPUInfo[0];
  *b = (unsigned int)CPUInfo[1];
  *c = (unsigned int)CPUInfo[2];
  *d = (unsigned int)CPUInfo[3];

}

BoolInt x86cpuid_CheckAndRead(Cx86cpuid *p)
{
  CHECK_CPUID_IS_SUPPORTED
  MyCPUID(0, &p->maxFunc, &p->vendor[0], &p->vendor[2], &p->vendor[1]);
  MyCPUID(1, &p->ver, &p->b, &p->c, &p->d);
  return True;
}

static const unsigned int kVendors[][3] =
{
  { 0x756E6547, 0x49656E69, 0x6C65746E},
  { 0x68747541, 0x69746E65, 0x444D4163},
  { 0x746E6543, 0x48727561, 0x736C7561}
};

int x86cpuid_GetFirm(const Cx86cpuid *p)
{
  unsigned i;
  for (i = 0; i < sizeof(kVendors) / sizeof(kVendors[i]); i++)
  {
    const unsigned int *v = kVendors[i];
    if (v[0] == p->vendor[0] &&
        v[1] == p->vendor[1] &&
        v[2] == p->vendor[2])
      return (int)i;
  }
  return -1;
}

#define CHECK_SYS_SSE_SUPPORT

static unsigned int X86_CPUID_ECX_Get_Flags()
{
  Cx86cpuid p;
  CHECK_SYS_SSE_SUPPORT
  if (!x86cpuid_CheckAndRead(&p))
    return 0;
  return p.c;
}

BoolInt CPU_IsSupported_AES()
{
  return (X86_CPUID_ECX_Get_Flags() >> 25) & 1;
}

BoolInt CPU_IsSupported_SSSE3()
{
  return (X86_CPUID_ECX_Get_Flags() >> 9) & 1;
}

BoolInt CPU_IsSupported_SHA()
{
  Cx86cpuid p;
  CHECK_SYS_SSE_SUPPORT
  if (!x86cpuid_CheckAndRead(&p))
    return False;

  if (p.maxFunc < 7)
    return False;
  {
    unsigned int d[4] = { 0 };
    MyCPUID(7, &d[0], &d[1], &d[2], &d[3]);
    return (d[1] >> 29) & 1;
  }
}

// #include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#endif