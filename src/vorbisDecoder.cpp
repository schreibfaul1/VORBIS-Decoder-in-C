
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "misc.h"
#include "ogg.h"
#include "ivorbiscodec.h"
#include "vorbisDecoder.h"
#include "os.h"
#include "window_lookup.h"
#include "mdct.h"

const uint32_t mask[]=
{0x00000000,0x00000001,0x00000003,0x00000007,0x0000000f,
 0x0000001f,0x0000003f,0x0000007f,0x000000ff,0x000001ff,
 0x000003ff,0x000007ff,0x00000fff,0x00001fff,0x00003fff,
 0x00007fff,0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
 0x000fffff,0x001fffff,0x003fffff,0x007fffff,0x00ffffff,
 0x01ffffff,0x03ffffff,0x07ffffff,0x0fffffff,0x1fffffff,
 0x3fffffff,0x7fffffff,0xffffffff };

const int32_t FLOOR_fromdB_LOOKUP[256]={
  XdB(0x000000e5), XdB(0x000000f4), XdB(0x00000103), XdB(0x00000114),
  XdB(0x00000126), XdB(0x00000139), XdB(0x0000014e), XdB(0x00000163),
  XdB(0x0000017a), XdB(0x00000193), XdB(0x000001ad), XdB(0x000001c9),
  XdB(0x000001e7), XdB(0x00000206), XdB(0x00000228), XdB(0x0000024c),
  XdB(0x00000272), XdB(0x0000029b), XdB(0x000002c6), XdB(0x000002f4),
  XdB(0x00000326), XdB(0x0000035a), XdB(0x00000392), XdB(0x000003cd),
  XdB(0x0000040c), XdB(0x00000450), XdB(0x00000497), XdB(0x000004e4),
  XdB(0x00000535), XdB(0x0000058c), XdB(0x000005e8), XdB(0x0000064a),
  XdB(0x000006b3), XdB(0x00000722), XdB(0x00000799), XdB(0x00000818),
  XdB(0x0000089e), XdB(0x0000092e), XdB(0x000009c6), XdB(0x00000a69),
  XdB(0x00000b16), XdB(0x00000bcf), XdB(0x00000c93), XdB(0x00000d64),
  XdB(0x00000e43), XdB(0x00000f30), XdB(0x0000102d), XdB(0x0000113a),
  XdB(0x00001258), XdB(0x0000138a), XdB(0x000014cf), XdB(0x00001629),
  XdB(0x0000179a), XdB(0x00001922), XdB(0x00001ac4), XdB(0x00001c82),
  XdB(0x00001e5c), XdB(0x00002055), XdB(0x0000226f), XdB(0x000024ac),
  XdB(0x0000270e), XdB(0x00002997), XdB(0x00002c4b), XdB(0x00002f2c),
  XdB(0x0000323d), XdB(0x00003581), XdB(0x000038fb), XdB(0x00003caf),
  XdB(0x000040a0), XdB(0x000044d3), XdB(0x0000494c), XdB(0x00004e10),
  XdB(0x00005323), XdB(0x0000588a), XdB(0x00005e4b), XdB(0x0000646b),
  XdB(0x00006af2), XdB(0x000071e5), XdB(0x0000794c), XdB(0x0000812e),
  XdB(0x00008993), XdB(0x00009283), XdB(0x00009c09), XdB(0x0000a62d),
  XdB(0x0000b0f9), XdB(0x0000bc79), XdB(0x0000c8b9), XdB(0x0000d5c4),
  XdB(0x0000e3a9), XdB(0x0000f274), XdB(0x00010235), XdB(0x000112fd),
  XdB(0x000124dc), XdB(0x000137e4), XdB(0x00014c29), XdB(0x000161bf),
  XdB(0x000178bc), XdB(0x00019137), XdB(0x0001ab4a), XdB(0x0001c70e),
  XdB(0x0001e4a1), XdB(0x0002041f), XdB(0x000225aa), XdB(0x00024962),
  XdB(0x00026f6d), XdB(0x000297f0), XdB(0x0002c316), XdB(0x0002f109),
  XdB(0x000321f9), XdB(0x00035616), XdB(0x00038d97), XdB(0x0003c8b4),
  XdB(0x000407a7), XdB(0x00044ab2), XdB(0x00049218), XdB(0x0004de23),
  XdB(0x00052f1e), XdB(0x0005855c), XdB(0x0005e135), XdB(0x00064306),
  XdB(0x0006ab33), XdB(0x00071a24), XdB(0x0007904b), XdB(0x00080e20),
  XdB(0x00089422), XdB(0x000922da), XdB(0x0009bad8), XdB(0x000a5cb6),
  XdB(0x000b091a), XdB(0x000bc0b1), XdB(0x000c8436), XdB(0x000d5471),
  XdB(0x000e3233), XdB(0x000f1e5f), XdB(0x001019e4), XdB(0x001125c1),
  XdB(0x00124306), XdB(0x001372d5), XdB(0x0014b663), XdB(0x00160ef7),
  XdB(0x00177df0), XdB(0x001904c1), XdB(0x001aa4f9), XdB(0x001c603d),
  XdB(0x001e384f), XdB(0x00202f0f), XdB(0x0022467a), XdB(0x002480b1),
  XdB(0x0026dff7), XdB(0x002966b3), XdB(0x002c1776), XdB(0x002ef4fc),
  XdB(0x0032022d), XdB(0x00354222), XdB(0x0038b828), XdB(0x003c67c2),
  XdB(0x004054ae), XdB(0x004482e8), XdB(0x0048f6af), XdB(0x004db488),
  XdB(0x0052c142), XdB(0x005821ff), XdB(0x005ddc33), XdB(0x0063f5b0),
  XdB(0x006a74a7), XdB(0x00715faf), XdB(0x0078bdce), XdB(0x0080967f),
  XdB(0x0088f1ba), XdB(0x0091d7f9), XdB(0x009b5247), XdB(0x00a56a41),
  XdB(0x00b02a27), XdB(0x00bb9ce2), XdB(0x00c7ce12), XdB(0x00d4ca17),
  XdB(0x00e29e20), XdB(0x00f15835), XdB(0x0101074b), XdB(0x0111bb4e),
  XdB(0x01238531), XdB(0x01367704), XdB(0x014aa402), XdB(0x016020a7),
  XdB(0x017702c3), XdB(0x018f6190), XdB(0x01a955cb), XdB(0x01c4f9cf),
  XdB(0x01e269a8), XdB(0x0201c33b), XdB(0x0223265a), XdB(0x0246b4ea),
  XdB(0x026c9302), XdB(0x0294e716), XdB(0x02bfda13), XdB(0x02ed9793),
  XdB(0x031e4e09), XdB(0x03522ee4), XdB(0x03896ed0), XdB(0x03c445e2),
  XdB(0x0402efd6), XdB(0x0445ac4b), XdB(0x048cbefc), XdB(0x04d87013),
  XdB(0x05290c67), XdB(0x057ee5ca), XdB(0x05da5364), XdB(0x063bb204),
  XdB(0x06a36485), XdB(0x0711d42b), XdB(0x0787710e), XdB(0x0804b299),
  XdB(0x088a17ef), XdB(0x0918287e), XdB(0x09af747c), XdB(0x0a50957e),
  XdB(0x0afc2f19), XdB(0x0bb2ef7f), XdB(0x0c759034), XdB(0x0d44d6ca),
  XdB(0x0e2195bc), XdB(0x0f0cad0d), XdB(0x10070b62), XdB(0x1111aeea),
  XdB(0x122da66c), XdB(0x135c120f), XdB(0x149e24d9), XdB(0x15f525b1),
  XdB(0x176270e3), XdB(0x18e7794b), XdB(0x1a85c9ae), XdB(0x1c3f06d1),
  XdB(0x1e14f07d), XdB(0x200963d7), XdB(0x221e5ccd), XdB(0x2455f870),
  XdB(0x26b2770b), XdB(0x29363e2b), XdB(0x2be3db5c), XdB(0x2ebe06b6),
  XdB(0x31c7a55b), XdB(0x3503ccd4), XdB(0x3875c5aa), XdB(0x3c210f44),
  XdB(0x4009632b), XdB(0x4432b8cf), XdB(0x48a149bc), XdB(0x4d59959e),
  XdB(0x52606733), XdB(0x57bad899), XdB(0x5d6e593a), XdB(0x6380b298),
  XdB(0x69f80e9a), XdB(0x70dafda8), XdB(0x78307d76), XdB(0x7fffffff),
};

//-------------------------------------------------------------------------------------------------
/* spans forward, skipping as many bytes as headend is negative; if
 headend is zero, simply finds next byte.  If we're up to the end
 of the buffer, leaves headend at zero.  If we've read past the end,
 halt the decode process. */

void _span(oggpack_buffer *b) {
	while (b->headend - (b->headbit >> 3) < 1) {
		b->headend -= b->headbit >> 3;
		b->headbit &= 0x7;

		if (b->head->next) {
			b->count += b->head->length;
			b->head = b->head->next;

			if (b->headend + b->head->length > 0)
				b->headptr = b->head->buffer->data + b->head->begin
						- b->headend;

			b->headend += b->head->length;
		} else {
			/* we've either met the end of decode, or gone past it. halt
			 only if we're past */
			if (b->headend * 8 < b->headbit)
				/* read has fallen off the end */
				b->headend = -1;
			break;
		}
	}
}
//-------------------------------------------------------------------------------------------------
void oggpack_readinit(oggpack_buffer *b, ogg_reference *r) {
	memset(b, 0, sizeof(*b));

	b->tail = b->head = r;
	b->count = 0;
	b->headptr = b->head->buffer->data + b->head->begin;
	b->headend = b->head->length;
	_span(b);
}
//-------------------------------------------------------------------------------------------------

/* Read in bits without advancing the bitptr; bits <= 32 */
int32_t oggpack_look(oggpack_buffer *b, int bits) {
	uint32_t m = mask[bits];
	uint32_t ret;

	bits += b->headbit;

	if (bits >= b->headend << 3) {
		int end = b->headend;
		unsigned char *ptr = b->headptr;
		ogg_reference *head = b->head;

		if (end < 0)
			return -1;

		if (bits) {
			_lookspan();
			ret = *ptr++ >> b->headbit;
			if (bits > 8) {
				--end;
				_lookspan();
				ret |= *ptr++ << (8 - b->headbit);
				if (bits > 16) {
					--end;
					_lookspan();
					ret |= *ptr++ << (16 - b->headbit);
					if (bits > 24) {
						--end;
						_lookspan();
						ret |= *ptr++ << (24 - b->headbit);
						if (bits > 32 && b->headbit) {
							--end;
							_lookspan();
							ret |= *ptr << (32 - b->headbit);
						}
					}
				}
			}
		}

	} else {

		/* make this a switch jump-table */
		ret = b->headptr[0] >> b->headbit;
		if (bits > 8) {
			ret |= b->headptr[1] << (8 - b->headbit);
			if (bits > 16) {
				ret |= b->headptr[2] << (16 - b->headbit);
				if (bits > 24) {
					ret |= b->headptr[3] << (24 - b->headbit);
					if (bits > 32 && b->headbit)
						ret |= b->headptr[4] << (32 - b->headbit);
				}
			}
		}
	}

	ret &= m;
	return ret;
}
//-------------------------------------------------------------------------------------------------
/* limited to 32 at a time */
void oggpack_adv(oggpack_buffer *b, int bits) {
	bits += b->headbit;
	b->headbit = bits & 7;
	b->headend -= (bits >> 3);
	b->headptr += (bits >> 3);
	if (b->headend < 1)
		_span(b);
}
//-------------------------------------------------------------------------------------------------
int oggpack_eop(oggpack_buffer *b) {
	if (b->headend < 0)
		return -1;
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* bits <= 32 */
int32_t oggpack_read(oggpack_buffer *b, int bits) {
	int32_t ret = oggpack_look(b, bits);
	oggpack_adv(b, bits);
	return (ret);
}
//-------------------------------------------------------------------------------------------------
int32_t oggpack_bytes(oggpack_buffer *b) {
	if (b->headend < 0)
		return b->count + b->head->length;
	return b->count + b->head->length - b->headend + (b->headbit + 7) / 8;
}
//-------------------------------------------------------------------------------------------------
int32_t oggpack_bits(oggpack_buffer *b) {
	if (b->headend < 0)
		return (b->count + b->head->length) * 8;
	return (b->count + b->head->length - b->headend) * 8 + b->headbit;
}
//-------------------------------------------------------------------------------------------------
int ilog(uint32_t v) {
	int ret = 0;
	if (v)
		--v;
	while (v) {
		ret++;
		v >>= 1;
	}
	return (ret);
}
//-------------------------------------------------------------------------------------------------

int _ilog(uint32_t v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}
//-------------------------------------------------------------------------------------------------
uint32_t decpack(int32_t entry, int32_t used_entry, int32_t quantvals, codebook *b, oggpack_buffer *opb, int maptype) {
	uint32_t ret = 0;
	int j;

	switch (b->dec_type) {

	case 0:
		return (uint32_t) entry;

	case 1:
		if (maptype == 1) {
			/* vals are already read into temporary column vector here */
			for (j = 0; j < b->dim; j++) {
				uint32_t off = entry % quantvals;
				entry /= quantvals;
				ret |= ((uint16_t*) (b->q_val))[off] << (b->q_bits * j);
			}
		} else {
			for (j = 0; j < b->dim; j++)
				ret |= oggpack_read(opb, b->q_bits) << (b->q_bits * j);
		}
		return ret;

	case 2:
		for (j = 0; j < b->dim; j++) {
			uint32_t off = entry % quantvals;
			entry /= quantvals;
			ret |= off << (b->q_pack * j);
		}
		return ret;

	case 3:
		return (uint32_t) used_entry;

	}
	return 0; /* silence compiler */
}
//-------------------------------------------------------------------------------------------------
/* 32 bit float (not IEEE; nonnormalized mantissa +
 biased exponent) : neeeeeee eeemmmmm mmmmmmmm mmmmmmmm
 Why not IEEE?  It's just not that important here. */

int32_t _float32_unpack(int32_t val, int *point) {
	int32_t mant = val & 0x1fffff;
	int sign = val & 0x80000000;

	*point = ((val & 0x7fe00000L) >> 21) - 788;

	if (mant) {
		while (!(mant & 0x40000000)) {
			mant <<= 1;
			*point -= 1;
		}
		if (sign)
			mant = -mant;
	} else {
		*point = -9999;
	}
	return mant;
}
//-------------------------------------------------------------------------------------------------
/* choose the smallest supported node size that fits our decode table.
 Legal bytewidths are 1/1 1/2 2/2 2/4 4/4 */
int _determine_node_bytes(int32_t used, int leafwidth) {

	/* special case small books to size 4 to avoid multiple special
	 cases in repack */
	if (used < 2)
		return 4;

	if (leafwidth == 3)
		leafwidth = 4;
	if (_ilog((3 * used - 6)) + 1 <= leafwidth * 4)
		return leafwidth / 2 ? leafwidth / 2 : 1;
	return leafwidth;
}
//-------------------------------------------------------------------------------------------------
/* convenience/clarity; leaves are specified as multiple of node word
 size (1 or 2) */
int _determine_leaf_words(int nodeb, int leafwidth) {
	if (leafwidth > nodeb)
		return 2;
	return 1;
}
//-------------------------------------------------------------------------------------------------
/* given a list of word lengths, number of used entries, and byte
 width of a leaf, generate the decode table */
int _make_words(char *l, int32_t n, uint32_t *r, int32_t quantvals, codebook *b, oggpack_buffer *opb, int maptype) {
	int32_t i, j, count = 0;
	int32_t top = 0;
	uint32_t marker[33];

	if (n < 2) {
		r[0] = 0x80000000;
	} else {
		memset(marker, 0, sizeof(marker));

		for (i = 0; i < n; i++) {
			int32_t length = l[i];
			if (length) {
				uint32_t entry = marker[length];
				int32_t chase = 0;
				if (count && !entry)
					return -1; /* overpopulated tree! */

				/* chase the tree as far as it's already populated, fill in past */
				for (j = 0; j < length - 1; j++) {
					int bit = (entry >> (length - j - 1)) & 1;
					if (chase >= top) {
						top++;
						r[chase * 2] = top;
						r[chase * 2 + 1] = 0;
					} else if (!r[chase * 2 + bit])
						r[chase * 2 + bit] = top;
					chase = r[chase * 2 + bit];
				}
				{
					int bit = (entry >> (length - j - 1)) & 1;
					if (chase >= top) {
						top++;
						r[chase * 2 + 1] = 0;
					}
					r[chase * 2 + bit] = decpack(i, count++, quantvals, b, opb,
							maptype) | 0x80000000;
				}

				/* Look to see if the next shorter marker points to the node
				 above. if so, update it and repeat.  */
				for (j = length; j > 0; j--) {
					if (marker[j] & 1) {
						marker[j] = marker[j - 1] << 1;
						break;
					}
					marker[j]++;
				}

				/* prune the tree; the implicit invariant says all the int32_ter
				 markers were dangling from our just-taken node.  Dangle them
				 from our *new* node. */
				for (j = length + 1; j < 33; j++)
					if ((marker[j] >> 1) == entry) {
						entry = marker[j];
						marker[j] = marker[j - 1] << 1;
					} else
						break;
			}
		}
	}

	return 0;
}
//-------------------------------------------------------------------------------------------------
int _make_decode_table(codebook *s, char *lengthlist, int32_t quantvals, oggpack_buffer *opb, int maptype) {
	int i;
	uint32_t *work;

	if (s->dec_nodeb == 4) {
		s->dec_table = malloc((s->used_entries * 2 + 1) * sizeof(*work));
		/* +1 (rather than -2) is to accommodate 0 and 1 sized books,
		 which are specialcased to nodeb==4 */
		if (_make_words(lengthlist, s->entries, (uint32_t*)s->dec_table, quantvals, s, opb,
				maptype))
			return 1;

		return 0;
	}

	work = (uint32_t*)alloca((s->used_entries * 2 - 2) * sizeof(*work));
	if (_make_words(lengthlist, s->entries, work, quantvals, s, opb, maptype))
		return 1;
	s->dec_table = malloc(
			(s->used_entries * (s->dec_leafw + 1) - 2) * s->dec_nodeb);

	if (s->dec_leafw == 1) {
		switch (s->dec_nodeb) {
		case 1:
			for (i = 0; i < s->used_entries * 2 - 2; i++)
				((unsigned char*) s->dec_table)[i] = ((work[i] & 0x80000000UL)
						>> 24) | work[i];
			break;
		case 2:
			for (i = 0; i < s->used_entries * 2 - 2; i++)
				((uint16_t*) s->dec_table)[i] = ((work[i] & 0x80000000UL) >> 16)
						| work[i];
			break;
		}

	} else {
		/* more complex; we have to do a two-pass repack that updates the
		 node indexing. */
		int32_t top = s->used_entries * 3 - 2;
		if (s->dec_nodeb == 1) {
			unsigned char *out = (unsigned char*) s->dec_table;

			for (i = s->used_entries * 2 - 4; i >= 0; i -= 2) {
				if (work[i] & 0x80000000UL) {
					if (work[i + 1] & 0x80000000UL) {
						top -= 4;
						out[top] = (work[i] >> 8 & 0x7f) | 0x80;
						out[top + 1] = (work[i + 1] >> 8 & 0x7f) | 0x80;
						out[top + 2] = work[i] & 0xff;
						out[top + 3] = work[i + 1] & 0xff;
					} else {
						top -= 3;
						out[top] = (work[i] >> 8 & 0x7f) | 0x80;
						out[top + 1] = work[work[i + 1] * 2];
						out[top + 2] = work[i] & 0xff;
					}
				} else {
					if (work[i + 1] & 0x80000000UL) {
						top -= 3;
						out[top] = work[work[i] * 2];
						out[top + 1] = (work[i + 1] >> 8 & 0x7f) | 0x80;
						out[top + 2] = work[i + 1] & 0xff;
					} else {
						top -= 2;
						out[top] = work[work[i] * 2];
						out[top + 1] = work[work[i + 1] * 2];
					}
				}
				work[i] = top;
			}
		} else {
			uint16_t *out = (uint16_t*) s->dec_table;
			for (i = s->used_entries * 2 - 4; i >= 0; i -= 2) {
				if (work[i] & 0x80000000UL) {
					if (work[i + 1] & 0x80000000UL) {
						top -= 4;
						out[top] = (work[i] >> 16 & 0x7fff) | 0x8000;
						out[top + 1] = (work[i + 1] >> 16 & 0x7fff) | 0x8000;
						out[top + 2] = work[i] & 0xffff;
						out[top + 3] = work[i + 1] & 0xffff;
					} else {
						top -= 3;
						out[top] = (work[i] >> 16 & 0x7fff) | 0x8000;
						out[top + 1] = work[work[i + 1] * 2];
						out[top + 2] = work[i] & 0xffff;
					}
				} else {
					if (work[i + 1] & 0x80000000UL) {
						top -= 3;
						out[top] = work[work[i] * 2];
						out[top + 1] = (work[i + 1] >> 16 & 0x7fff) | 0x8000;
						out[top + 2] = work[i + 1] & 0xffff;
					} else {
						top -= 2;
						out[top] = work[work[i] * 2];
						out[top + 1] = work[work[i + 1] * 2];
					}
				}
				work[i] = top;
			}
		}
	}

	return 0;
}
//-------------------------------------------------------------------------------------------------
/* most of the time, entries%dimensions == 0, but we need to be
 well defined.  We define that the possible vales at each
 scalar is values == entries/dim.  If entries%dim != 0, we'll
 have 'too few' values (values*dim<entries), which means that
 we'll have 'left over' entries; left over entries use zeroed
 values (and are wasted).  So don't generate codebooks like
 that */
/* there might be a straightforward one-line way to do the below
 that's portable and totally safe against roundoff, but I haven't
 thought of it.  Therefore, we opt on the side of caution */
int32_t _book_maptype1_quantvals(codebook *b) {
	/* get us a starting hint, we'll polish it below */
	int bits = _ilog(b->entries);
	int vals = b->entries >> ((bits - 1) * (b->dim - 1) / b->dim);

	while (1) {
		int32_t acc = 1;
		int32_t acc1 = 1;
		int i;
		for (i = 0; i < b->dim; i++) {
			acc *= vals;
			acc1 *= vals + 1;
		}
		if (acc <= b->entries && acc1 > b->entries) {
			return (vals);
		} else {
			if (acc > b->entries) {
				vals--;
			} else {
				vals++;
			}
		}
	}
}
//-------------------------------------------------------------------------------------------------
void vorbis_book_clear(codebook *b) {
	/* static book is not cleared; we're likely called on the lookup and
	 the static codebook beint32_ts to the info struct */
	if (b->q_val)
		free(b->q_val);
	if (b->dec_table)
		free(b->dec_table);

	memset(b, 0, sizeof(*b));
}
//-------------------------------------------------------------------------------------------------
int vorbis_book_unpack(oggpack_buffer *opb, codebook *s) {
	char *lengthlist = NULL;
	int quantvals = 0;
	int32_t i, j;
	int maptype;

	memset(s, 0, sizeof(*s));

	/* make sure alignment is correct */
	if (oggpack_read(opb, 24) != 0x564342)
		goto _eofout;

	/* first the basic parameters */
	s->dim = oggpack_read(opb, 16);
	s->entries = oggpack_read(opb, 24);
	if (s->entries == -1)
		goto _eofout;

	/* codeword ordering.... length ordered or unordered? */
	switch ((int) oggpack_read(opb, 1)) {
	case 0:
		/* unordered */
		lengthlist = (char*) alloca(sizeof(*lengthlist) * s->entries);

		/* allocated but unused entries? */
		if (oggpack_read(opb, 1)) {
			/* yes, unused entries */

			for (i = 0; i < s->entries; i++) {
				if (oggpack_read(opb, 1)) {
					int32_t num = oggpack_read(opb, 5);
					if (num == -1)
						goto _eofout;
					lengthlist[i] = num + 1;
					s->used_entries++;
					if (num + 1 > s->dec_maxlength)
						s->dec_maxlength = num + 1;
				} else
					lengthlist[i] = 0;
			}
		} else {
			/* all entries used; no tagging */
			s->used_entries = s->entries;
			for (i = 0; i < s->entries; i++) {
				int32_t num = oggpack_read(opb, 5);
				if (num == -1)
					goto _eofout;
				lengthlist[i] = num + 1;
				if (num + 1 > s->dec_maxlength)
					s->dec_maxlength = num + 1;
			}
		}

		break;
	case 1:
		/* ordered */
	{
		int32_t length = oggpack_read(opb, 5) + 1;

		s->used_entries = s->entries;
		lengthlist = (char*) alloca(sizeof(*lengthlist) * s->entries);

		for (i = 0; i < s->entries;) {
			int32_t num = oggpack_read(opb, _ilog(s->entries - i));
			if (num == -1)
				goto _eofout;
			for (j = 0; j < num && i < s->entries; j++, i++)
				lengthlist[i] = length;
			s->dec_maxlength = length;
			length++;
		}
	}
		break;
	default:
		/* EOF */
		goto _eofout;
	}

	/* Do we have a mapping to unpack? */

	if ((maptype = oggpack_read(opb, 4)) > 0) {
		s->q_min = _float32_unpack(oggpack_read(opb, 32), &s->q_minp);
		s->q_del = _float32_unpack(oggpack_read(opb, 32), &s->q_delp);
		s->q_bits = oggpack_read(opb, 4) + 1;
		s->q_seq = oggpack_read(opb, 1);

		s->q_del >>= s->q_bits;
		s->q_delp += s->q_bits;
	}

	switch (maptype) {
	case 0:

		/* no mapping; decode type 0 */

		/* how many bytes for the indexing? */
		/* this is the correct boundary here; we lose one bit to
		 node/leaf mark */
		s->dec_nodeb = _determine_node_bytes(s->used_entries,
				_ilog(s->entries) / 8 + 1);
		s->dec_leafw = _determine_leaf_words(s->dec_nodeb,
				_ilog(s->entries) / 8 + 1);
		s->dec_type = 0;

		if (_make_decode_table(s, lengthlist, quantvals, opb, maptype))
			goto _errout;
		break;

	case 1:

		/* mapping type 1; implicit values by lattice  position */
		quantvals = _book_maptype1_quantvals(s);

		/* dec_type choices here are 1,2; 3 doesn't make sense */
		{
			/* packed values */
			int32_t total1 = (s->q_bits * s->dim + 8) / 8; /* remember flag bit */
			/* vector of column offsets; remember flag bit */
			int32_t total2 = (_ilog(quantvals - 1) * s->dim + 8) / 8
					+ (s->q_bits + 7) / 8;

			if (total1 <= 4 && total1 <= total2) {
				/* use dec_type 1: vector of packed values */

				/* need quantized values before  */
				s->q_val = alloca(sizeof(uint16_t) * quantvals);
				for (i = 0; i < quantvals; i++)
					((uint16_t*) s->q_val)[i] = oggpack_read(opb, s->q_bits);

				if (oggpack_eop(opb)) {
					s->q_val = 0; /* cleanup must not free alloca memory */
					goto _eofout;
				}

				s->dec_type = 1;
				s->dec_nodeb = _determine_node_bytes(s->used_entries,
						(s->q_bits * s->dim + 8) / 8);
				s->dec_leafw = _determine_leaf_words(s->dec_nodeb,
						(s->q_bits * s->dim + 8) / 8);
				if (_make_decode_table(s, lengthlist, quantvals, opb,
						maptype)) {
					s->q_val = 0; /* cleanup must not free alloca memory */
					goto _errout;
				}

				s->q_val = 0; /* about to go out of scope; _make_decode_table
				 was using it */

			} else {
				/* use dec_type 2: packed vector of column offsets */

				/* need quantized values before */
				if (s->q_bits <= 8) {
					s->q_val = malloc(quantvals);
					for (i = 0; i < quantvals; i++)
						((unsigned char*) s->q_val)[i] = oggpack_read(opb,
								s->q_bits);
				} else {
					s->q_val = malloc(quantvals * 2);
					for (i = 0; i < quantvals; i++)
						((uint16_t*) s->q_val)[i] = oggpack_read(opb,
								s->q_bits);
				}

				if (oggpack_eop(opb))
					goto _eofout;

				s->q_pack = _ilog(quantvals - 1);
				s->dec_type = 2;
				s->dec_nodeb = _determine_node_bytes(s->used_entries,
						(_ilog(quantvals - 1) * s->dim + 8) / 8);
				s->dec_leafw = _determine_leaf_words(s->dec_nodeb,
						(_ilog(quantvals - 1) * s->dim + 8) / 8);
				if (_make_decode_table(s, lengthlist, quantvals, opb, maptype))
					goto _errout;

			}
		}
		break;
	case 2:

		/* mapping type 2; explicit array of values */
		quantvals = s->entries * s->dim;
		/* dec_type choices here are 1,3; 2 is not possible */

		if ((s->q_bits * s->dim + 8) / 8 <= 4) { /* remember flag bit */
			/* use dec_type 1: vector of packed values */

			s->dec_type = 1;
			s->dec_nodeb = _determine_node_bytes(s->used_entries,
					(s->q_bits * s->dim + 8) / 8);
			s->dec_leafw = _determine_leaf_words(s->dec_nodeb,
					(s->q_bits * s->dim + 8) / 8);
			if (_make_decode_table(s, lengthlist, quantvals, opb, maptype))
				goto _errout;

		} else {
			/* use dec_type 3: scalar offset into packed value array */

			s->dec_type = 3;
			s->dec_nodeb = _determine_node_bytes(s->used_entries,
					_ilog(s->used_entries - 1) / 8 + 1);
			s->dec_leafw = _determine_leaf_words(s->dec_nodeb,
					_ilog(s->used_entries - 1) / 8 + 1);
			if (_make_decode_table(s, lengthlist, quantvals, opb, maptype))
				goto _errout;

			/* get the vals & pack them */
			s->q_pack = (s->q_bits + 7) / 8 * s->dim;
			s->q_val = malloc(s->q_pack * s->used_entries);

			if (s->q_bits <= 8) {
				for (i = 0; i < s->used_entries * s->dim; i++)
					((unsigned char*) (s->q_val))[i] = oggpack_read(opb,
							s->q_bits);
			} else {
				for (i = 0; i < s->used_entries * s->dim; i++)
					((uint16_t*) (s->q_val))[i] = oggpack_read(opb, s->q_bits);
			}
		}
		break;
	default:
		goto _errout;
	}

	if (oggpack_eop(opb))
		goto _eofout;

	return 0;
	_errout: _eofout: vorbis_book_clear(s);
	return -1;
}
//-------------------------------------------------------------------------------------------------
uint32_t decode_packed_entry_number(codebook *book,	oggpack_buffer *b) {
	uint32_t chase = 0;
	int read = book->dec_maxlength;
	int32_t lok = oggpack_look(b, read), i;

	while (lok < 0 && read > 1)
		lok = oggpack_look(b, --read);

	if (lok < 0) {
		oggpack_adv(b, 1); /* force eop */
		return -1;
	}

	/* chase the tree with the bits we got */
	if (book->dec_nodeb == 1) {
		if (book->dec_leafw == 1) {

			/* 8/8 */
			unsigned char *t = (unsigned char*) book->dec_table;
			for (i = 0; i < read; i++) {
				chase = t[chase * 2 + ((lok >> i) & 1)];
				if (chase & 0x80UL)
					break;
			}
			chase &= 0x7fUL;

		} else {

			/* 8/16 */
			unsigned char *t = (unsigned char*) book->dec_table;
			for (i = 0; i < read; i++) {
				int bit = (lok >> i) & 1;
				int next = t[chase + bit];
				if (next & 0x80) {
					chase = (next << 8)
							| t[chase + bit + 1 + (!bit || (t[chase] & 0x80))];
					break;
				}
				chase = next;
			}
			chase &= 0x7fffUL;
		}

	} else {
		if (book->dec_nodeb == 2) {
			if (book->dec_leafw == 1) {

				/* 16/16 */
				for (i = 0; i < read; i++) {
					chase = ((uint16_t*) (book->dec_table))[chase * 2
							+ ((lok >> i) & 1)];
					if (chase & 0x8000UL)
						break;
				}
				chase &= 0x7fffUL;

			} else {

				/* 16/32 */
				uint16_t *t = (uint16_t*) book->dec_table;
				for (i = 0; i < read; i++) {
					int bit = (lok >> i) & 1;
					int next = t[chase + bit];
					if (next & 0x8000) {
						chase = (next << 16)
								| t[chase + bit + 1
										+ (!bit || (t[chase] & 0x8000))];
						break;
					}
					chase = next;
				}
				chase &= 0x7fffffffUL;
			}

		} else {

			for (i = 0; i < read; i++) {
				chase = ((uint32_t*) (book->dec_table))[chase * 2
						+ ((lok >> i) & 1)];
				if (chase & 0x80000000UL)
					break;
			}
			chase &= 0x7fffffffUL;

		}
	}

	if (i < read) {
		oggpack_adv(b, i + 1);
		return chase;
	}
	oggpack_adv(b, read + 1);
	return (-1);
}
//-------------------------------------------------------------------------------------------------
/* returns the [original, not compacted] entry number or -1 on eof *********/
int32_t vorbis_book_decode(codebook *book, oggpack_buffer *b) {
	if (book->dec_type)
		return -1;
	return decode_packed_entry_number(book, b);
}
//-------------------------------------------------------------------------------------------------
int decode_map(codebook *s, oggpack_buffer *b, int32_t *v, int point) {

	uint32_t entry = decode_packed_entry_number(s, b);
	int i;
	if (oggpack_eop(b))
		return (-1);

	/* according to decode type */
	switch (s->dec_type) {
	case 1: {
		/* packed vector of values */
		int mask = (1 << s->q_bits) - 1;
		for (i = 0; i < s->dim; i++) {
			v[i] = entry & mask;
			entry >>= s->q_bits;
		}
		break;
	}
	case 2: {
		/* packed vector of column offsets */
		int mask = (1 << s->q_pack) - 1;
		for (i = 0; i < s->dim; i++) {
			if (s->q_bits <= 8)
				v[i] = ((unsigned char*) (s->q_val))[entry & mask];
			else
				v[i] = ((uint16_t*) (s->q_val))[entry & mask];
			entry >>= s->q_pack;
		}
		break;
	}
	case 3: {
		/* offset into array */
		void *ptr = s->q_val + entry * s->q_pack;

		if (s->q_bits <= 8) {
			for (i = 0; i < s->dim; i++)
				v[i] = ((unsigned char*) ptr)[i];
		} else {
			for (i = 0; i < s->dim; i++)
				v[i] = ((uint16_t*) ptr)[i];
		}
		break;
	}
	default:
		return -1;
	}

	/* we have the unpacked multiplicands; compute final vals */
	{
		int shiftM = point - s->q_delp;
		int32_t add = point - s->q_minp;
		if (add > 0)
			add = s->q_min >> add;
		else
			add = s->q_min << -add;

		if (shiftM > 0)
			for (i = 0; i < s->dim; i++)
				v[i] = add + ((v[i] * s->q_del) >> shiftM);
		else
			for (i = 0; i < s->dim; i++)
				v[i] = add + ((v[i] * s->q_del) << -shiftM);

		if (s->q_seq)
			for (i = 1; i < s->dim; i++)
				v[i] += v[i - 1];
	}

	return 0;
}
//-------------------------------------------------------------------------------------------------
/* returns 0 on OK or -1 on eof *************************************/
/* decode vector / dim granularity guarding is done in the upper layer */
int32_t vorbis_book_decodevs_add(codebook *book, int32_t *a, oggpack_buffer *b,	int n, int point) {
	if (book->used_entries > 0) {
		int step = n / book->dim;
		int32_t *v = (int32_t*) alloca(sizeof(*v) * book->dim);
		int i, j, o;

		for (j = 0; j < step; j++) {
			if (decode_map(book, b, v, point))
				return -1;
			for (i = 0, o = j; i < book->dim; i++, o += step)
				a[o] += v[i];
		}
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* decode vector / dim granularity guarding is done in the upper layer */
int32_t vorbis_book_decodev_add(codebook *book, int32_t *a, oggpack_buffer *b, int n, int point) {
	if (book->used_entries > 0) {
		int32_t *v = (int32_t*) alloca(sizeof(*v) * book->dim);
		int i, j;

		for (i = 0; i < n;) {
			if (decode_map(book, b, v, point))
				return -1;
			for (j = 0; i < n && j < book->dim; j++)
				a[i++] += v[j];
		}
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* unlike the others, we guard against n not being an integer number
 * of <dim> internally rather than in the upper layer (called only by
 * floor0) */
int32_t vorbis_book_decodev_set(codebook *book, int32_t *a, oggpack_buffer *b, int n, int point) {
	if (book->used_entries > 0) {
		int32_t *v = (int32_t*) alloca(sizeof(*v) * book->dim);
		int i, j;

		for (i = 0; i < n;) {
			if (decode_map(book, b, v, point))
				return -1;
			for (j = 0; i < n && j < book->dim; j++)
				a[i++] = v[j];
		}
	} else {
		int i;

		for (i = 0; i < n;) {
			a[i++] = 0;
		}
	}

	return 0;
}
//-------------------------------------------------------------------------------------------------
/* decode vector / dim granularity guarding is done in the upper layer */
int32_t vorbis_book_decodevv_add(codebook *book, int32_t **a, int32_t offset, int ch, oggpack_buffer *b, int n, int point) {
	if (book->used_entries > 0) {

		int32_t *v = (int32_t*) alloca(sizeof(*v) * book->dim);
		int32_t i, j;
		int chptr = 0;
		int32_t m = offset + n;

		for (i = offset; i < m;) {
			if (decode_map(book, b, v, point))
				return -1;
			for (j = 0; i < m && j < book->dim; j++) {
				a[chptr++][i] += v[j];
				if (chptr == ch) {
					chptr = 0;
					i++;
				}
			}
		}
	}

	return 0;
}

//-------------------------------------------------------------------------------------------------
int vorbis_dsp_restart(vorbis_dsp_state *v) {
	if(!v)	return -1;
	vorbis_info *vi = v->vi;
	codec_setup_info *ci;
	if(!vi)	return -1;

	ci = (codec_setup_info*)vi->codec_setup;
	if(!ci)	return -1;

	v->out_end = -1;
	v->out_begin = -1;

	v->granulepos = -1;
	v->sequence = -1;
	v->sample_count = -1;

	return 0;
}
//-------------------------------------------------------------------------------------------------
vorbis_dsp_state* vorbis_dsp_create(vorbis_info *vi) {
	int i;

	vorbis_dsp_state *v = (vorbis_dsp_state*) calloc(1, sizeof(*v));
	codec_setup_info *ci = (codec_setup_info*) vi->codec_setup;

	v->vi = vi;

	v->work = (int32_t**) malloc(vi->channels * sizeof(*v->work));
	v->mdctright = (int32_t**) malloc(vi->channels * sizeof(*v->mdctright));

	for (i = 0; i < vi->channels; i++) {
		v->work[i] = (int32_t*) calloc(1,
				(ci->blocksizes[1] >> 1) * sizeof(*v->work[i]));
		v->mdctright[i] = (int32_t*) calloc(1,
				(ci->blocksizes[1] >> 2) * sizeof(*v->mdctright[i]));
	}

	v->lW = 0; /* previous window size */
	v->W = 0; /* current window size  */

	vorbis_dsp_restart(v);
	return v;
}
//-------------------------------------------------------------------------------------------------
void vorbis_dsp_destroy(vorbis_dsp_state *v) {
	int i;
	if (v) {
		vorbis_info *vi = v->vi;

		if (v->work) {
			for (i = 0; i < vi->channels; i++) {
				if (v->work[i])
					free(v->work[i]);
			}
			free(v->work);
		}
		if (v->mdctright) {
			for (i = 0; i < vi->channels; i++) {
				if (v->mdctright[i])
					free(v->mdctright[i]);
			}
			free(v->mdctright);
		}
		free(v);
	}
}
//-------------------------------------------------------------------------------------------------
int32_t* _vorbis_window(int left) {
	switch (left) {
	case 32:
		return (int32_t*) vwin64;
	case 64:
		return (int32_t*) vwin128;
	case 128:
		return (int32_t*) vwin256;
	case 256:
		return (int32_t*) vwin512;
	case 512:
		return (int32_t*) vwin1024;
	case 1024:
		return (int32_t*) vwin2048;
	case 2048:
		return (int32_t*) vwin4096;
	case 4096:
		return (int32_t*) vwin8192;
	default:
		return (0);
	}
}
//-------------------------------------------------------------------------------------------------
/* pcm==0 indicates we just want the pending samples, no more */
int vorbis_dsp_pcmout(vorbis_dsp_state *v, int16_t *pcm, int samples) {
	vorbis_info *vi = v->vi;
	codec_setup_info *ci = (codec_setup_info*) vi->codec_setup;
	if (v->out_begin > -1 && v->out_begin < v->out_end) {
		int n = v->out_end - v->out_begin;
		if (pcm) {
			int i;
			if (n > samples)
				n = samples;
			for (i = 0; i < vi->channels; i++)
				mdct_unroll_lap(ci->blocksizes[0], ci->blocksizes[1], v->lW,
						v->W, v->work[i], v->mdctright[i],
						_vorbis_window(ci->blocksizes[0] >> 1),
						_vorbis_window(ci->blocksizes[1] >> 1), pcm + i,
						vi->channels, v->out_begin, v->out_begin + n);
		}
		return (n);
	}
	return (0);
}
//-------------------------------------------------------------------------------------------------
int vorbis_dsp_read(vorbis_dsp_state *v, int s) {
	if (s && v->out_begin + s > v->out_end)
		return (OV_EINVAL);
	v->out_begin += s;
	return (0);
}
//-------------------------------------------------------------------------------------------------
int32_t vorbis_packet_blocksize(vorbis_info *vi, ogg_packet *op) {
	codec_setup_info *ci = (codec_setup_info*) vi->codec_setup;
	oggpack_buffer opb;
	int mode;
	int modebits = 0;
	int v = ci->modes;

	oggpack_readinit(&opb, op->packet);

	/* Check the packet type */
	if (oggpack_read(&opb, 1) != 0) {
		/* Oops.  This is not an audio data packet */
		return (OV_ENOTAUDIO);
	}

	while (v > 1) {
		modebits++;
		v >>= 1;
	}

	/* read our mode and pre/post windowsize */
	mode = oggpack_read(&opb, modebits);
	if (mode == -1)
		return (OV_EBADPACKET);
	return (ci->blocksizes[ci->mode_param[mode].blockflag]);
}

//-------------------------------------------------------------------------------------------------
int vorbis_dsp_synthesis(vorbis_dsp_state *vd, ogg_packet *op, int decodep) {
	vorbis_info *vi = vd->vi;
	codec_setup_info *ci = (codec_setup_info*) vi->codec_setup;
	int mode, i;

	oggpack_readinit(&vd->opb, op->packet);

	/* Check the packet type */
	if (oggpack_read(&vd->opb, 1) != 0) {
		/* Oops.  This is not an audio data packet */
		return OV_ENOTAUDIO;
	}

	/* read our mode and pre/post windowsize */
	mode = oggpack_read(&vd->opb, ilog(ci->modes));
	if (mode == -1 || mode >= ci->modes)
		return OV_EBADPACKET;

	/* shift information we still need from last window */
	vd->lW = vd->W;
	vd->W = ci->mode_param[mode].blockflag;
	for (i = 0; i < vi->channels; i++)
		mdct_shift_right(ci->blocksizes[vd->lW], vd->work[i], vd->mdctright[i]);

	if (vd->W) {
		int temp;
		oggpack_read(&vd->opb, 1);
		temp = oggpack_read(&vd->opb, 1);
		if (temp == -1)
			return OV_EBADPACKET;
	}

	/* packet decode and portions of synthesis that rely on only this block */
	if (decodep) {
		mapping_inverse(vd, ci->map_param + ci->mode_param[mode].mapping);

		if (vd->out_begin == -1) {
			vd->out_begin = 0;
			vd->out_end = 0;
		} else {
			vd->out_begin = 0;
			vd->out_end = ci->blocksizes[vd->lW] / 4
					+ ci->blocksizes[vd->W] / 4;
		}
	}

	/* track the frame number... This is for convenience, but also
	 making sure our last packet doesn't end with added padding.

	 This is not foolproof!  It will be confused if we begin
	 decoding at the last page after a seek or hole.  In that case,
	 we don't have a starting point to judge where the last frame
	 is.  For this reason, vorbisfile will always try to make sure
	 it reads the last two marked pages in proper sequence */

	/* if we're out of sequence, dump granpos tracking until we sync back up */
	if (vd->sequence == -1 || vd->sequence + 1 != op->packetno - 3) {
		/* out of sequence; lose count */
		vd->granulepos = -1;
		vd->sample_count = -1;
	}

	vd->sequence = op->packetno;
	vd->sequence = vd->sequence - 3;

	if (vd->sample_count == -1) {
		vd->sample_count = 0;
	} else {
		vd->sample_count += ci->blocksizes[vd->lW] / 4
				+ ci->blocksizes[vd->W] / 4;
	}

	if (vd->granulepos == -1) {
		if (op->granulepos != -1) { /* only set if we have a
		 position to set to */

			vd->granulepos = op->granulepos;

			/* is this a short page? */
			if (vd->sample_count > vd->granulepos) {
				/* corner case; if this is both the first and last audio page,
				 then spec says the end is cut, not beginning */
				if (op->e_o_s) {
					/* trim the end */
					/* no preceeding granulepos; assume we started at zero (we'd
					 have to in a short single-page stream) */
					/* granulepos could be -1 due to a seek, but that would result
					 in a long coun t, not short count */

					vd->out_end -= vd->sample_count - vd->granulepos;
				} else {
					/* trim the beginning */
					vd->out_begin += vd->sample_count - vd->granulepos;
					if (vd->out_begin > vd->out_end)
						vd->out_begin = vd->out_end;
				}

			}

		}
	} else {
		vd->granulepos += ci->blocksizes[vd->lW] / 4
				+ ci->blocksizes[vd->W] / 4;
		if (op->granulepos != -1 && vd->granulepos != op->granulepos) {

			if (vd->granulepos > op->granulepos) {
				long extra = vd->granulepos - op->granulepos;

				if (extra)
					if (op->e_o_s) {
						/* partial last frame.  Strip the extra samples off */
						vd->out_end -= extra;
					} /* else {Shouldn't happen *unless* the bitstream is out of
					 spec.  Either way, believe the bitstream } */
			} /* else {Shouldn't happen *unless* the bitstream is out of
			 spec.  Either way, believe the bitstream } */
			vd->granulepos = op->granulepos;
		}
	}
	return (0);
}
//-------------------------------------------------------------------------------------------------
/* interpolated lookup based fromdB function, domain -140dB to 0dB only */
/* a is in n.12 format */

int32_t vorbis_fromdBlook_i(long a) {
	if (a > 0)
		return 0x7fffffff;
	if (a < (-140 << 12))
		return 0;
	return FLOOR_fromdB_LOOKUP[((a + (140 << 12)) * 467) >> 20];
}
//-------------------------------------------------------------------------------------------------
void render_line(int n, int x0, int x1, int y0, int y1, int32_t *d) {
	int dy = y1 - y0;
	int adx = x1 - x0;
	int ady = abs(dy);
	int base = dy / adx;
	int sy = (dy < 0 ? base - 1 : base + 1);
	int x = x0;
	int y = y0;
	int err = 0;

	if (n > x1)
		n = x1;
	ady -= abs(base * adx);

	if (x < n)
		d[x] = MULT31_SHIFT15(d[x], FLOOR_fromdB_LOOKUP[y]);

	while (++x < n) {
		err = err + ady;
		if (err >= adx) {
			err -= adx;
			y += sy;
		} else {
			y += base;
		}
		d[x] = MULT31_SHIFT15(d[x], FLOOR_fromdB_LOOKUP[y]);
	}
}









