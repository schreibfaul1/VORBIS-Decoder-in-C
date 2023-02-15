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

#define _V_WIDE_MATH

/* 64 bit multiply */

#include <sys/types.h>

#if BYTE_ORDER==LITTLE_ENDIAN
union magic {
  struct {
    ogg_int32_t lo;
    ogg_int32_t hi;
  } halves;
  ogg_int64_t whole;
};
#endif 

static inline ogg_int32_t MULT32(ogg_int32_t x, ogg_int32_t y) {
  union magic magic;
  magic.whole = (ogg_int64_t)x * y;
  return magic.halves.hi;
}

static inline ogg_int32_t MULT31(ogg_int32_t x, ogg_int32_t y) {
  return MULT32(x,y)<<1;
}

static inline ogg_int32_t MULT31_SHIFT15(ogg_int32_t x, ogg_int32_t y) {
  union magic magic;
  magic.whole  = (ogg_int64_t)x * y;
  return ((ogg_uint32_t)(magic.halves.lo)>>15) | ((magic.halves.hi)<<17);
}


/*
 * This should be used as a memory barrier, forcing all cached values in
 * registers to wr writen back to memory.  Might or might not be beneficial
 * depending on the architecture and compiler.
 */
#define MB()

/*
 * The XPROD functions are meant to optimize the cross products found all
 * over the place in mdct.c by forcing memory operation ordering to avoid
 * unnecessary register reloads as soon as memory is being written to.
 * However this is only beneficial on CPUs with a sane number of general
 * purpose registers which exclude the Intel x86.  On Intel, better let the
 * compiler actually reload registers directly from original memory by using
 * macros.
 */



static inline void XPROD32(ogg_int32_t  a, ogg_int32_t  b,
			   ogg_int32_t  t, ogg_int32_t  v,
			   ogg_int32_t *x, ogg_int32_t *y)
{
  *x = MULT32(a, t) + MULT32(b, v);
  *y = MULT32(b, t) - MULT32(a, v);
}

static inline void XPROD31(ogg_int32_t  a, ogg_int32_t  b,
			   ogg_int32_t  t, ogg_int32_t  v,
			   ogg_int32_t *x, ogg_int32_t *y)
{
  *x = MULT31(a, t) + MULT31(b, v);
  *y = MULT31(b, t) - MULT31(a, v);
}

static inline void XNPROD31(ogg_int32_t  a, ogg_int32_t  b,
			    ogg_int32_t  t, ogg_int32_t  v,
			    ogg_int32_t *x, ogg_int32_t *y)
{
  *x = MULT31(a, t) - MULT31(b, v);
  *y = MULT31(b, t) + MULT31(a, v);
}



#ifndef _V_CLIP_MATH
#define _V_CLIP_MATH

static inline ogg_int32_t CLIP_TO_15(ogg_int32_t x) {
  int ret=x;
  ret-= ((x<=32767)-1)&(x-32767);
  ret-= ((x>=-32768)-1)&(x+32768);
  return(ret);
}



#endif


#endif



