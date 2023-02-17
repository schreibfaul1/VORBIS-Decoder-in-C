/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2003    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: miscellaneous math and prototypes

 ********************************************************************/

#ifndef _V_RANDOM_H_
#define _V_RANDOM_H_
#include "ivorbiscodec.h"
#include "os_types.h"
#include <stdint.h>

#define _V_WIDE_MATH

/* 64 bit multiply */

#include <sys/types.h>

#if BYTE_ORDER==LITTLE_ENDIAN

#endif 




static inline int32_t MULT31_SHIFT15(int32_t x, int32_t y) {
  union magic magic;
  magic.whole  = (int64_t)x * y;
  return ((uint32_t)(magic.halves.lo)>>15) | ((magic.halves.hi)<<17);
}
#endif



