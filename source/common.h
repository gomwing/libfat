/*
 common.h
 Common definitions and included files for the FATlib

 Copyright (c) 2006 Michael "Chishm" Chisholm

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _COMMON_H
#define _COMMON_H

//#include <fat.h>
#include <stddef.h>
#include <stdint.h>

// When compiling for NDS, make sure NDS is defined
#ifndef NDS
 #if defined ARM9 || defined ARM7
  #define NDS
 #endif
#endif

// Platform specific includes
#if defined(__gamecube__) || defined (__wii__)
   #include <gctypes.h>
   #include <ogc/disc_io.h>
   #include <gccore.h>
#elif defined(NDS_)
   #include <nds/ndstypes.h>
   #include <nds/system.h>
   #include <nds/disc_io.h>
#elif defined(GBA)
   #include <gba_types.h>
   #include <disc_io.h>
#elif defined(GP2X)
  #include <gp2xtypes.h>
  #include <disc_io.h>
#endif

// Platform specific options
#if   defined (__wii__)
   #define DEFAULT_CACHE_PAGES 4
   #define DEFAULT_SECTORS_PAGE 64
   #define USE_LWP_LOCK
   #define USE_RTC_TIME
#elif defined (__gamecube__)
   #define DEFAULT_CACHE_PAGES 4
   #define DEFAULT_SECTORS_PAGE 64
   #define USE_LWP_LOCK
   #define USE_RTC_TIME
#elif defined (NDS)
   #define DEFAULT_CACHE_PAGES 16
   #define DEFAULT_SECTORS_PAGE 8
   //#define USE_RTC_TIME
#elif defined (GBA)
   #define DEFAULT_CACHE_PAGES 2
   #define DEFAULT_SECTORS_PAGE 8
   #define LIMIT_SECTORS 128
#elif defined (GP2X)
  #define DEFAULT_CACHE_PAGES 16
  #define DEFAULT_SECTORS_PAGE 8
#endif

#include <stdbool.h>
typedef unsigned int sec_t;
typedef unsigned int u32;
typedef			 int s32;
typedef unsigned char u8;
typedef unsigned short u16;

typedef bool (*FN_MEDIUM_STARTUP)(void);
typedef bool (*FN_MEDIUM_ISINSERTED)(void);
typedef bool (*FN_MEDIUM_READSECTORS)(sec_t sector, sec_t numSectors, void* buffer);
typedef bool (*FN_MEDIUM_WRITESECTORS)(sec_t sector, sec_t numSectors, const void* buffer);
typedef bool (*FN_MEDIUM_CLEARSTATUS)(void);
typedef bool (*FN_MEDIUM_SHUTDOWN)(void);

struct DISC_INTERFACE_STRUCT {
	unsigned long			ioType;
	unsigned long			features;
	FN_MEDIUM_STARTUP		startup;
	FN_MEDIUM_ISINSERTED	isInserted;
	FN_MEDIUM_READSECTORS	readSectors;
	FN_MEDIUM_WRITESECTORS	writeSectors;
	FN_MEDIUM_CLEARSTATUS	clearStatus;
	FN_MEDIUM_SHUTDOWN		shutdown;
};
typedef struct DISC_INTERFACE_STRUCT DISC_INTERFACE;

#define FEATURE_MEDIUM_CANREAD      0x00000001
#define FEATURE_MEDIUM_CANWRITE     0x00000002

#ifndef _SYS_REENT_H_
#define _SYS_REENT_H_
struct _reent
{
	int _errno;
	// Other members are not used in this file, so they are not defined here
};

typedef struct {
	int device;
	void* dirStruct;
} DIR_ITER;

#endif

#endif // _COMMON_H
