#pragma once

#include "ogg.h"
#include <stdint.h>






//-------------------------------------------------------------------------------------------------

#define _lookspan()   while(!end){\
                        head=head->next;\
                        if(!head) return -1;\
                        ptr=head->buffer->data + head->begin;\
                        end=head->length;\
                      }
//-------------------------------------------------------------------------------------------------
typedef struct codebook{
  long  dim;             /* codebook dimensions (elements per vector) */
  long  entries;         /* codebook entries */
  long  used_entries;    /* populated codebook entries */
  int   dec_maxlength;
  void *dec_table;
  int   dec_nodeb;
  int   dec_leafw;
  int   dec_type; /* 0 = entry number
     			     1 = packed vector of values
	    		     2 = packed vector of column offsets, maptype 1
		    	     3 = scalar offset into value array,  maptype 2  */
  int32_t q_min;
  int         q_minp;
  int32_t q_del;
  int         q_delp;
  int         q_seq;
  int         q_bits;
  int         q_pack;
  void       *q_val;
} codebook;


//-------------------------------------------------------------------------------------------------
void _span(oggpack_buffer *b);
void oggpack_readinit(oggpack_buffer *b, ogg_reference *r);
int32_t oggpack_look(oggpack_buffer *b, int bits);
void oggpack_adv(oggpack_buffer *b, int bits);
int oggpack_eop(oggpack_buffer *b);
int32_t oggpack_read(oggpack_buffer *b, int bits);
int32_t oggpack_bytes(oggpack_buffer *b);
int32_t oggpack_bits(oggpack_buffer *b);
int _ilog(unsigned int v);
uint32_t decpack(long entry, long used_entry, long quantvals, codebook *b, oggpack_buffer *opb, int maptype);
int32_t _float32_unpack(long val, int *point);
int _determine_node_bytes(long used, int leafwidth);
int _determine_leaf_words(int nodeb, int leafwidth);
int _make_words(char *l, long n, uint32_t *r, long quantvals, codebook *b, oggpack_buffer *opb, int maptype);
int _make_decode_table(codebook *s, char *lengthlist, long quantvals, oggpack_buffer *opb, int maptype);
int32_t _book_maptype1_quantvals(codebook *b);
void vorbis_book_clear(codebook *b);
int vorbis_book_unpack(oggpack_buffer *opb, codebook *s);
uint32_t decode_packed_entry_number(codebook *book,	oggpack_buffer *b);
int32_t vorbis_book_decode(codebook *book, oggpack_buffer *b);
int decode_map(codebook *s, oggpack_buffer *b, int32_t *v, int point);
int32_t vorbis_book_decodevs_add(codebook *book, int32_t *a, oggpack_buffer *b,	int n, int point);
int32_t vorbis_book_decodev_add(codebook *book, int32_t *a, oggpack_buffer *b, int n, int point);
int32_t vorbis_book_decodev_set(codebook *book, int32_t *a, oggpack_buffer *b, int n, int point);
int32_t vorbis_book_decodevv_add(codebook *book, int32_t **a, long offset, int ch, oggpack_buffer *b, int n, int point);














extern void vorbis_book_clear(codebook *b);
extern int  vorbis_book_unpack(oggpack_buffer *b,codebook *c);
extern int32_t vorbis_book_decode(codebook *book, oggpack_buffer *b);
extern int32_t vorbis_book_decodevs_add(codebook *book, int32_t *a, oggpack_buffer *b,int n,int point);
extern int32_t vorbis_book_decodev_set(codebook *book, int32_t *a, oggpack_buffer *b,int n,int point);
extern int32_t vorbis_book_decodev_add(codebook *book, int32_t *a, oggpack_buffer *b,int n,int point);
extern int32_t vorbis_book_decodevv_add(codebook *book, int32_t **a, long off,int ch, oggpack_buffer *b,int n,int point);




