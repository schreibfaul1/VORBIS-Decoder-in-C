/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************/

#define HEAD_ALIGN 64
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define MISC_C
#include "misc.h"
#include <sys/time.h>

static void **pointers=NULL;
static long *insertlist=NULL; /* We can't embed this in the pointer list;
			  a pointer can have any value... */

static int ptop=0;
static int palloced=0;
static int pinsert=0;

typedef struct {
  char *file;
  long line;
  long ptr;
  long bytes;
} head;

long global_bytes=0;
long start_time=-1;

static void *_insert(void *ptr,long bytes,char *file,long line){
  ((head *)ptr)->file=file;
  ((head *)ptr)->line=line;
  ((head *)ptr)->ptr=pinsert;
  ((head *)ptr)->bytes=bytes-HEAD_ALIGN;

  if(pinsert>=palloced){
    palloced+=64;
    if(pointers){
      pointers=(void **)realloc(pointers,sizeof(void **)*palloced);
      insertlist=(long *)realloc(insertlist,sizeof(long *)*palloced);
    }else{
      pointers=(void **)malloc(sizeof(void **)*palloced);
      insertlist=(long *)malloc(sizeof(long *)*palloced);
    }
  }

  pointers[pinsert]=ptr;

  if(pinsert==ptop)
    pinsert=++ptop;
  else
    pinsert=insertlist[pinsert];

  global_bytes+=(bytes-HEAD_ALIGN);

  return(ptr+HEAD_ALIGN);
}

static void _ripremove(void *ptr){
  int insert;

  global_bytes-=((head *)ptr)->bytes;

  insert=((head *)ptr)->ptr;
  insertlist[insert]=pinsert;
  pinsert=insert;

  if(pointers[insert]==NULL){
    fprintf(stderr,"DEBUGGING MALLOC ERROR: freeing previously freed memory\n");
    fprintf(stderr,"\t%s %ld\n",((head *)ptr)->file,((head *)ptr)->line);
  }

  if(global_bytes<0){
    fprintf(stderr,"DEBUGGING MALLOC ERROR: freeing unmalloced memory\n");
  }

  pointers[insert]=NULL;
}

void _VDBG_dump(void){
  int i;
  for(i=0;i<ptop;i++){
    head *ptr=pointers[i];
    if(ptr)
      fprintf(stderr,"unfreed bytes from %s:%ld\n",
	      ptr->file,ptr->line);
  }

}

extern void *_VDBG_malloc(void *ptr,long bytes,char *file,long line){
  bytes+=HEAD_ALIGN;
  if(ptr){
    ptr-=HEAD_ALIGN;
    _ripremove(ptr);
    ptr=realloc(ptr,bytes);
  }else{
    ptr=malloc(bytes);
    memset(ptr,0,bytes);
  }
  return _insert(ptr,bytes,file,line);
}

extern void _VDBG_free(void *ptr,char *file,long line){
  if(ptr){
    ptr-=HEAD_ALIGN;
    _ripremove(ptr);
    free(ptr);
  }
}

