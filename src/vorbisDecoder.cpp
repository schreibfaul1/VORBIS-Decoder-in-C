
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "misc.h"
#include "ogg.h"
#include "vorbisDecoder.h"

const uint32_t mask[]=
{0x00000000,0x00000001,0x00000003,0x00000007,0x0000000f,
 0x0000001f,0x0000003f,0x0000007f,0x000000ff,0x000001ff,
 0x000003ff,0x000007ff,0x00000fff,0x00001fff,0x00003fff,
 0x00007fff,0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
 0x000fffff,0x001fffff,0x003fffff,0x007fffff,0x00ffffff,
 0x01ffffff,0x03ffffff,0x07ffffff,0x0fffffff,0x1fffffff,
 0x3fffffff,0x7fffffff,0xffffffff };

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
