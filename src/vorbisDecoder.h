#pragma once

#include "ogg.h"
#include <stdint.h>
#include <stdio.h>
#include "ivorbiscodec.h"




//-------------------------------------------------------------------------------------------------
#define CHUNKSIZE 1024
#define VI_TRANSFORMB 1
#define VI_WINDOWB 1
#define VI_TIMEB 1
#define VI_FLOORB 2
#define VI_RESB 3
#define VI_MAPB 1

#define XdB(n) (n)
#define LSP_FRACBITS 14

#define floor1_rangedB 140 /* floor 1 fixed at -140dB to 0dB range */
#define VIF_POSIT 63

#define _lookspan()   while(!end){\
                        head=head->next;\
                        if(!head) return -1;\
                        ptr=head->buffer->data + head->begin;\
                        end=head->length;\
                      }
//-------------------------------------------------------------------------------------------------
typedef struct codebook{
  int32_t  dim;             /* codebook dimensions (elements per vector) */
  int32_t  entries;         /* codebook entries */
  int32_t  used_entries;    /* populated codebook entries */
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

struct vorbis_dsp_state;
typedef struct vorbis_dsp_state vorbis_dsp_state;

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

typedef struct coupling_step { // Mapping backend generic
	uint8_t mag;
	uint8_t ang;
} coupling_step;

typedef struct submap {
	char floor;
	char residue;
} submap;

typedef struct vorbis_info_mapping {
	int submaps;
	uint8_t *chmuxlist;
	submap *submaplist;
	int coupling_steps;
	coupling_step *coupling;
} vorbis_info_mapping;

typedef struct codec_setup_info { // Vorbis supports only short and int32_t blocks, but allows the
	int32_t blocksizes[2];           // encoder to choose the sizes
	int modes;                    // modes are the primary means of supporting on-the-fly different
	int maps;                     // blocksizes, different channel mappings (LR or M/A),
	int floors;                   // different residue backends, etc.  Each mode consists of a
	int residues;                 // blocksize flag and a mapping (aint32_t with the mapping setup
	int books;
	vorbis_info_mode *mode_param;
	vorbis_info_mapping *map_param;
	char *floor_type;
	vorbis_info_floor **floor_param;
	vorbis_info_residue *residue_param;
	codebook *book_param;
} codec_setup_info;

typedef struct {
	size_t (*read_func)(void *ptr, size_t size, size_t nmemb, void *datasource);
	int (*seek_func)(void *datasource, int64_t offset, int whence);
	int (*close_func)(void *datasource);
	int32_t (*tell_func)(void *datasource);
} ov_callbacks;

typedef struct vorbis_comment {
	char **user_comments;
	int *comment_lengths;
	int comments;
	char *vendor;
} vorbis_comment;

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
void _span(oggpack_buffer *b);
void oggpack_readinit(oggpack_buffer *b, ogg_reference *r);
int32_t oggpack_look(oggpack_buffer *b, int bits);
void oggpack_adv(oggpack_buffer *b, int bits);
int oggpack_eop(oggpack_buffer *b);
int32_t oggpack_read(oggpack_buffer *b, int bits);
int32_t oggpack_bytes(oggpack_buffer *b);
int32_t oggpack_bits(oggpack_buffer *b);
int _ilog(uint32_t v);
int _ilog(uint32_t v);
uint32_t decpack(int32_t entry, int32_t used_entry, int32_t quantvals, codebook *b, oggpack_buffer *opb, int maptype);
int32_t _float32_unpack(int32_t val, int *point);
int _determine_node_bytes(int32_t used, int leafwidth);
int _determine_leaf_words(int nodeb, int leafwidth);
int _make_words(char *l, int32_t n, uint32_t *r, int32_t quantvals, codebook *b, oggpack_buffer *opb, int maptype);
int _make_decode_table(codebook *s, char *lengthlist, int32_t quantvals, oggpack_buffer *opb, int maptype);
int32_t _book_maptype1_quantvals(codebook *b);
void vorbis_book_clear(codebook *b);
int vorbis_book_unpack(oggpack_buffer *opb, codebook *s);
uint32_t decode_packed_entry_number(codebook *book,	oggpack_buffer *b);
int32_t vorbis_book_decode(codebook *book, oggpack_buffer *b);
int decode_map(codebook *s, oggpack_buffer *b, int32_t *v, int point);
int32_t vorbis_book_decodevs_add(codebook *book, int32_t *a, oggpack_buffer *b,	int n, int point);
int32_t vorbis_book_decodev_add(codebook *book, int32_t *a, oggpack_buffer *b, int n, int point);
int32_t vorbis_book_decodev_set(codebook *book, int32_t *a, oggpack_buffer *b, int n, int point);
int32_t vorbis_book_decodevv_add(codebook *book, int32_t **a, int32_t offset, int ch, oggpack_buffer *b, int n, int point);
int vorbis_dsp_restart(vorbis_dsp_state *v);
vorbis_dsp_state* vorbis_dsp_create(vorbis_info *vi);
void vorbis_dsp_destroy(vorbis_dsp_state *v);
int32_t* _vorbis_window(int left);
int vorbis_dsp_pcmout(vorbis_dsp_state *v, int16_t *pcm, int samples);
int vorbis_dsp_read(vorbis_dsp_state *v, int s);
int32_t vorbis_packet_blocksize(vorbis_info *vi, ogg_packet *op);
int vorbis_dsp_synthesis(vorbis_dsp_state *vd, ogg_packet *op, int decodep);
int32_t vorbis_fromdBlook_i(int32_t a);
void render_line(int n, int x0, int x1, int y0, int y1, int32_t *d);
int32_t vorbis_coslook_i(int32_t a);
int32_t vorbis_coslook2_i(int32_t a);
int32_t toBARK(int n);
int32_t vorbis_invsqlook_i(int32_t a, int32_t e);
void vorbis_lsp_to_curve(int32_t *curve, int n, int ln, int32_t *lsp, int m, int32_t amp, int32_t ampoffset, int32_t nyq);
void floor0_free_info(vorbis_info_floor *i);
vorbis_info_floor* floor0_info_unpack(vorbis_info *vi, oggpack_buffer *opb);
int floor0_memosize(vorbis_info_floor *i);
int32_t* floor0_inverse1(vorbis_dsp_state *vd, vorbis_info_floor *i, int32_t *lsp);
int floor0_inverse2(vorbis_dsp_state *vd, vorbis_info_floor *i, int32_t *lsp, int32_t *out);
void floor1_free_info(vorbis_info_floor *i);
void vorbis_mergesort(uint8_t *index, uint16_t *vals, uint16_t n);
vorbis_info_floor* floor1_info_unpack(vorbis_info *vi, oggpack_buffer *opb);
int render_point(int x0, int x1, int y0, int y1, int x);
int floor1_memosize(vorbis_info_floor *i);
int32_t* floor1_inverse1(vorbis_dsp_state *vd, vorbis_info_floor *in, int32_t *fit_value);
int floor1_inverse2(vorbis_dsp_state *vd, vorbis_info_floor *in, int32_t *fit_value, int32_t *out);







extern void vorbis_book_clear(codebook *b);
extern int  vorbis_book_unpack(oggpack_buffer *b,codebook *c);
extern int32_t vorbis_book_decode(codebook *book, oggpack_buffer *b);
extern int32_t vorbis_book_decodevs_add(codebook *book, int32_t *a, oggpack_buffer *b,int n,int point);
extern int32_t vorbis_book_decodev_set(codebook *book, int32_t *a, oggpack_buffer *b,int n,int point);
extern int32_t vorbis_book_decodev_add(codebook *book, int32_t *a, oggpack_buffer *b,int n,int point);
extern int32_t vorbis_book_decodevv_add(codebook *book, int32_t **a, int32_t off,int ch, oggpack_buffer *b,int n,int point);
extern vorbis_info_floor* floor0_info_unpack(vorbis_info*, oggpack_buffer*);
extern void floor0_free_info(vorbis_info_floor*);
extern int floor0_memosize(vorbis_info_floor*);
extern int32_t* floor0_inverse1(struct vorbis_dsp_state*, vorbis_info_floor*, int32_t*);
extern int floor0_inverse2(struct vorbis_dsp_state*, vorbis_info_floor*, int32_t *buffer, int32_t*);
extern vorbis_info_floor* floor1_info_unpack(vorbis_info*, oggpack_buffer*);
extern void floor1_free_info(vorbis_info_floor*);
extern int floor1_memosize(vorbis_info_floor*);
extern int32_t* floor1_inverse1(struct vorbis_dsp_state*, vorbis_info_floor*, int32_t*);
extern int floor1_inverse2(struct vorbis_dsp_state*, vorbis_info_floor*, int32_t *buffer, int32_t*);
extern int mapping_info_unpack(vorbis_info_mapping*, vorbis_info*,	oggpack_buffer*);
extern void mapping_clear_info(vorbis_info_mapping*);
extern int mapping_inverse(struct vorbis_dsp_state*, vorbis_info_mapping*);
extern void res_clear_info(vorbis_info_residue *info);
extern int res_unpack(vorbis_info_residue *info, vorbis_info *vi, oggpack_buffer *opb);
extern int res_inverse(vorbis_dsp_state*, vorbis_info_residue *info, int32_t **in, int *nonzero, int ch);
extern vorbis_dsp_state* vorbis_dsp_create(vorbis_info *vi);
extern void vorbis_dsp_destroy(vorbis_dsp_state *v);
extern int vorbis_dsp_headerin(vorbis_info *vi, vorbis_comment *vc,	ogg_packet *op);
extern int vorbis_dsp_restart(vorbis_dsp_state *v);
extern int vorbis_dsp_synthesis(vorbis_dsp_state *vd, ogg_packet *op, int decodep);
extern int vorbis_dsp_pcmout(vorbis_dsp_state *v, int16_t *pcm, int samples);
extern int vorbis_dsp_read(vorbis_dsp_state *v, int samples);
extern int32_t vorbis_packet_blocksize(vorbis_info *vi, ogg_packet *op);
extern int ov_open(FILE *f,OggVorbis_File *vf,char *initial,int32_t ibytes);
extern vorbis_comment *ov_comment(OggVorbis_File *vf,int link);
extern int32_t ov_read(OggVorbis_File *vf,void *buffer,int length);
extern vorbis_info *ov_info(OggVorbis_File *vf,int link);
extern int64_t ov_pcm_total(OggVorbis_File *vf,int i);
extern int ov_clear(OggVorbis_File *vf);
extern int64_t ov_time_total(OggVorbis_File *vf,int i);
extern void vorbis_info_clear(vorbis_info *vi);
extern void vorbis_comment_clear(vorbis_comment *vc);
extern void vorbis_info_init(vorbis_info *vi);
extern void vorbis_comment_init(vorbis_comment *vc);
extern int ov_raw_seek(OggVorbis_File *vf,int64_t pos);
extern int vorbis_info_blocksize(vorbis_info *vi,int zo);
extern void mdct_shift_right(int n, int32_t *in, int32_t *right);
extern void mdct_unroll_lap(int n0,int n1, int lW,int W,
			    int32_t *in, int32_t *right,
			    const int32_t *w0,
				const int32_t *w1,
			    int16_t *out,
			    int step,
			    int start,int end /* samples, this frame */);
void _v_readstring(oggpack_buffer *o, char *buf, int bytes);
void vorbis_comment_init(vorbis_comment *vc);
int tagcompare(const char *s1, const char *s2, int n);
char* vorbis_comment_query(vorbis_comment *vc, char *tag, int count);
int vorbis_comment_query_count(vorbis_comment *vc, char *tag);
void vorbis_comment_clear(vorbis_comment *vc);
int vorbis_info_blocksize(vorbis_info *vi, int zo);
void vorbis_info_init(vorbis_info *vi);
void vorbis_info_clear(vorbis_info *vi);
int _vorbis_unpack_info(vorbis_info *vi, oggpack_buffer *opb);
int _vorbis_unpack_comment(vorbis_comment *vc, oggpack_buffer *opb);
int _vorbis_unpack_books(vorbis_info *vi, oggpack_buffer *opb);
int vorbis_dsp_headerin(vorbis_info *vi, vorbis_comment *vc, ogg_packet *op);






