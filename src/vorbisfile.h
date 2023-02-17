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

 function: subsumed libogg includes

 ********************************************************************/
#ifndef _OGG_H
#define _OGG_H


#include "os_types.h"
#include <stdint.h>
#include <stdio.h>

typedef struct ogg_buffer_state{
  struct ogg_buffer    *unused_buffers;
  struct ogg_reference *unused_references;
  int                   outstanding;
  int                   shutdown;
} ogg_buffer_state;

typedef struct ogg_buffer {
  unsigned char      *data;
  long                size;
  int                 refcount;
  
  union {
    ogg_buffer_state  *owner;
    struct ogg_buffer *next;
  } ptr;
} ogg_buffer;

typedef struct ogg_reference {
  ogg_buffer    *buffer;
  long           begin;
  long           length;

  struct ogg_reference *next;
} ogg_reference;

typedef struct oggpack_buffer {
  int            headbit;
  unsigned char *headptr;
  long           headend;

  /* memory management */
  ogg_reference *head;
  ogg_reference *tail;

  /* render the byte/bit counter API constant time */
  long              count; /* doesn't count the tail */
} oggpack_buffer;

typedef struct oggbyte_buffer {
  ogg_reference *baseref;

  ogg_reference *ref;
  unsigned char *ptr;
  long           pos;
  long           end;
} oggbyte_buffer;

typedef struct ogg_sync_state {
  /* decode memory management pool */
  ogg_buffer_state *bufferpool;

  /* stream buffers */
  ogg_reference    *fifo_head;
  ogg_reference    *fifo_tail;
  long              fifo_fill;

  /* stream sync management */
  int               unsynced;
  int               headerbytes;
  int               bodybytes;

} ogg_sync_state;

typedef struct ogg_stream_state {
  ogg_reference *header_head;
  ogg_reference *header_tail;
  ogg_reference *body_head;
  ogg_reference *body_tail;

  int            e_o_s;    /* set when we have buffered the last
                              packet in the logical bitstream */
  int            b_o_s;    /* set after we've written the initial page
                              of a logical bitstream */
  long           serialno;
  long           pageno;
  int64_t    packetno; /* sequence number for decode; the framing
                              knows where there's a hole in the data,
                              but we need coupling so that the codec
                              (which is in a seperate abstraction
                              layer) also knows about the gap */
  int64_t    granulepos;

  int            lacing_fill;
  uint32_t   body_fill;

  /* decode-side state data */
  int            holeflag;
  int            spanflag;
  int            clearflag;
  int            laceptr;
  uint32_t   body_fill_next;
  
} ogg_stream_state;

typedef struct {
  ogg_reference *packet;
  long           bytes;
  long           b_o_s;
  long           e_o_s;
  int64_t    granulepos;
  int64_t    packetno;     /* sequence number for decode; the framing
                                  knows where there's a hole in the data,
                                  but we need coupling so that the codec
                                  (which is in a seperate abstraction
                                  layer) also knows about the gap */
} ogg_packet;

typedef struct {
  ogg_reference *header;
  int            header_len;
  ogg_reference *body;
  long           body_len;
} ogg_page;

typedef struct vorbis_info{
  int version;  // The below bitrate declarations are *hints*. Combinations of the three values carry
  int channels; // the following implications: all three set to the same value: implies a fixed rate bitstream
  int32_t rate;    // only nominal set:  implies a VBR stream that averages the nominal bitrate.  No hard
  int32_t bitrate_upper; // upper/lower limit upper and or lower set:  implies a VBR bitstream that obeys the
  int32_t bitrate_nominal; // bitrate limits. nominal may also be set to give a nominal rate. none set:
  int32_t bitrate_lower; //  the coder does not care to speculate.
  int32_t bitrate_window;
  void *codec_setup;
} vorbis_info;

typedef void vorbis_info_floor;

struct vorbis_dsp_state { // vorbis_dsp_state buffers the current vorbis audio analysis/synthesis state.
	vorbis_info *vi;      // The DSP state beint32_ts to a specific logical bitstream
	oggpack_buffer opb;
	int32_t **work;
	int32_t **mdctright;
	int out_begin;
	int out_end;
	int32_t lW;
	int32_t W;
	int64_t granulepos;
	int64_t sequence;
	int64_t sample_count;
};

typedef struct {
	int order;
	int32_t rate;
	int32_t barkmap;
	int ampbits;
	int ampdB;
	int numbooks; /* <= 16 */
	char books[16];
} vorbis_info_floor0;

typedef struct {
	char class_dim; /* 1 to 8 */
	char class_subs; /* 0,1,2,3 (bits: 1<<n poss) */
	uint8_t class_book; /* subs ^ dim entries */
	uint8_t class_subbook[8]; /* [VIF_CLASS][subs] */
} floor1class;

typedef struct {
	floor1class *_class; /* [VIF_CLASS] */
	uint8_t *partitionclass; /* [VIF_PARTS]; 0 to 15 */
	uint16_t *postlist; /* [VIF_POSIT+2]; first two implicit */
	uint8_t *forward_index; /* [VIF_POSIT+2]; */
	uint8_t *hineighbor; /* [VIF_POSIT]; */
	uint8_t *loneighbor; /* [VIF_POSIT]; */
	int partitions; /* 0 to 31 */
	int posts;
	int mult; /* 1 2 3 or 4 */
} vorbis_info_floor1;

typedef struct vorbis_info_residue {
	int type;
	uint8_t *stagemasks;
	uint8_t *stagebooks;
	/* block-partitioned VQ coded straight residue */
	int32_t begin;
	int32_t end;
	/* first stage (lossless partitioning) */
	int grouping; /* group n vectors per partition */
	char partitions; /* possible codebooks for a partition */
	uint8_t groupbook; /* huffbook for partitioning */
	char stages;
} vorbis_info_residue;

typedef struct {  // mode
	uint8_t blockflag;
	uint8_t mapping;
} vorbis_info_mode;


typedef struct vorbis_comment {
	char **user_comments;
	int *comment_lengths;
	int comments;
	char *vendor;
} vorbis_comment;

typedef struct {
	size_t (*read_func)(void *ptr, size_t size, size_t nmemb, void *datasource);
	int (*seek_func)(void *datasource, int64_t offset, int whence);
	int (*close_func)(void *datasource);
	int32_t (*tell_func)(void *datasource);
} ov_callbacks;

struct vorbis_dsp_state;
typedef struct vorbis_dsp_state vorbis_dsp_state;

typedef struct OggVorbis_File {
	void *datasource; /* Pointer to a FILE *, etc. */
	int seekable;
	int64_t offset;
	int64_t end;
	ogg_sync_state *oy; //If the FILE handle isn't seekable (eg, a pipe), only the current
    int              links;//stream appears */
    int64_t     *offsets;
    int64_t     *dataoffsets;
    uint32_t    *serialnos;
    int64_t     *pcmlengths;
    vorbis_info     vi;
    vorbis_comment  vc;
    int64_t      pcm_offset;/* Decoding working state local storage */
    int              ready_state;
    uint32_t     current_serialno;
    int              current_link;
    int64_t      bittrack;
    int64_t      samptrack;
    ogg_stream_state *os; /* take physical pages, weld into a logical stream of packets */
    vorbis_dsp_state *vd; /* central working state for the packet->PCM decoder */
    ov_callbacks callbacks;
} OggVorbis_File;

//-------------------------------------------------------------------------------------------------
long _get_data(OggVorbis_File *vf);
void _seek_helper(OggVorbis_File *vf, int64_t offset);
int64_t _get_next_page(OggVorbis_File *vf, ogg_page *og, int64_t boundary);
int64_t _get_prev_page(OggVorbis_File *vf, ogg_page *og);
int _bisect_forward_serialno(OggVorbis_File *vf, int64_t begin,	int64_t searched, int64_t end, uint32_t currentno, long m);
int _decode_clear(OggVorbis_File *vf);
int _fetch_headers(OggVorbis_File *vf, vorbis_info *vi,	vorbis_comment *vc, uint32_t *serialno, ogg_page *og_ptr);
int _set_link_number(OggVorbis_File *vf, int link);
int _set_link_number_preserve_pos(OggVorbis_File *vf, int link);
void _prefetch_all_offsets(OggVorbis_File *vf, int64_t dataoffset);
int _make_decode_ready(OggVorbis_File *vf);
int _open_seekable2(OggVorbis_File *vf);
int _fetch_and_process_packet(OggVorbis_File *vf, int readp, int spanp);
int _fseek64_wrap(FILE *f, int64_t off, int whence);
int _ov_open1(void *f, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks);
int _ov_open2(OggVorbis_File *vf);
int ov_clear(OggVorbis_File *vf);
int ov_open_callbacks(void *f, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks);
int ov_open(FILE *f, OggVorbis_File *vf, char *initial, int32_t ibytes);
int ov_test_callbacks(void *f, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks);
int ov_test_open(OggVorbis_File *vf);
long ov_streams(OggVorbis_File *vf);
long ov_seekable(OggVorbis_File *vf);
long ov_bitrate(OggVorbis_File *vf, int i);
long ov_bitrate_instant(OggVorbis_File *vf);
long ov_serialnumber(OggVorbis_File *vf, int i);
int64_t ov_raw_total(OggVorbis_File *vf, int i);
int64_t ov_pcm_total(OggVorbis_File *vf, int i);
int64_t ov_time_total(OggVorbis_File *vf, int i);
int ov_raw_seek(OggVorbis_File *vf, int64_t pos);
int ov_pcm_seek_page(OggVorbis_File *vf, int64_t pos);
int ov_pcm_seek(OggVorbis_File *vf, int64_t pos);
int ov_time_seek(OggVorbis_File *vf, int64_t milliseconds);
int ov_time_seek_page(OggVorbis_File *vf, int64_t milliseconds);
int64_t ov_raw_tell(OggVorbis_File *vf);
int64_t ov_pcm_tell(OggVorbis_File *vf);
int64_t ov_time_tell(OggVorbis_File *vf);
vorbis_info* ov_info(OggVorbis_File *vf, int link);
vorbis_comment* ov_comment(OggVorbis_File *vf, int link);
int32_t ov_read(OggVorbis_File *vf, void *buffer, int bytes_req);

void oggpack_readinit(oggpack_buffer *b, ogg_reference *r);
int32_t oggpack_look(oggpack_buffer *b, int bits);
void oggpack_adv(oggpack_buffer *b, int bits);
int oggpack_eop(oggpack_buffer *b);
int32_t oggpack_read(oggpack_buffer *b, int bits);
int32_t oggpack_bytes(oggpack_buffer *b);
int32_t oggpack_bits(oggpack_buffer *b);










//
//
//
//
///* Ogg BITSTREAM PRIMITIVES: bitstream ************************/
//
//extern void  oggpack_readinit(oggpack_buffer *b,ogg_reference *r);
//extern int32_t  oggpack_look(oggpack_buffer *b,int bits);
//extern void  oggpack_adv(oggpack_buffer *b,int bits);
//extern int32_t  oggpack_read(oggpack_buffer *b,int bits);
//extern int32_t  oggpack_bytes(oggpack_buffer *b);
//extern int32_t  oggpack_bits(oggpack_buffer *b);
//extern int   oggpack_eop(oggpack_buffer *b);
//
///* Ogg BITSTREAM PRIMITIVES: decoding **************************/
//
extern ogg_sync_state *ogg_sync_create(void);
extern int      ogg_sync_destroy(ogg_sync_state *oy);
extern int      ogg_sync_reset(ogg_sync_state *oy);
//
extern unsigned char *ogg_sync_bufferin(ogg_sync_state *oy, long size);
extern int      ogg_sync_wrote(ogg_sync_state *oy, long bytes);
extern long     ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og);
extern int      ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og);
extern int      ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);
extern int      ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op);
//
///* Ogg BITSTREAM PRIMITIVES: general ***************************/
//
extern ogg_stream_state *ogg_stream_create(int serialno);
extern int      ogg_stream_destroy(ogg_stream_state *os);
extern int      ogg_stream_reset(ogg_stream_state *os);
extern int      ogg_stream_reset_serialno(ogg_stream_state *os,int serialno);
extern int      ogg_stream_eos(ogg_stream_state *os);
//
extern int      ogg_page_checksum_set(ogg_page *og);
//
extern int      ogg_page_version(ogg_page *og);
extern int      ogg_page_continued(ogg_page *og);
extern int      ogg_page_bos(ogg_page *og);
extern int      ogg_page_eos(ogg_page *og);
extern int64_t  ogg_page_granulepos(ogg_page *og);
extern uint32_t ogg_page_serialno(ogg_page *og);
extern uint32_t ogg_page_pageno(ogg_page *og);
extern int      ogg_page_packets(ogg_page *og);
extern int      ogg_page_getbuffer(ogg_page *og, unsigned char **buffer);
//
extern int      ogg_packet_release(ogg_packet *op);
extern int      ogg_page_release(ogg_page *og);
//
extern void     ogg_page_dup(ogg_page *d, ogg_page *s);
//
///* Ogg BITSTREAM PRIMITIVES: return codes ***************************/

#define  OGG_SUCCESS   0

#define  OGG_HOLE     -10
#define  OGG_SPAN     -11
#define  OGG_EVERSION -12
#define  OGG_ESERIAL  -13
#define  OGG_EINVAL   -14
#define  OGG_EEOS     -15




#endif  /* _OGG_H */
