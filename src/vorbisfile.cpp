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

 function: stdio-based convenience library for opening/seeking/decoding
 last mod: $Id: vorbisfile.c,v 1.6.2.5 2003/11/20 06:16:17 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "misc.h"
#include "vorbisfile.h"

#define  NOTOPEN   0
#define  PARTOPEN  1
#define  OPENED    2
#define  STREAMSET 3 /* serialno and link set, but not to current link */
#define  LINKSET   4 /* serialno and link set to current link */
#define  INITSET   5


/* A 'chained bitstream' is a Vorbis bitstream that contains more than
 one logical bitstream arranged end to end (the only form of Ogg
 multiplexing allowed in a Vorbis bitstream; grouping [parallel
 multiplexing] is not allowed in Vorbis) */

/* A Vorbis file can be played beginning to end (streamed) without
 worrying ahead of time about chaining (see decoder_example.c).  If
 we have the whole file, however, and want random access
 (seeking/scrubbing) or desire to know the total length/time of a
 file, we need to account for the possibility of chaining. */

/* We can handle things a number of ways; we can determine the entire
 bitstream structure right off the bat, or find pieces on demand.
 This example determines and caches structure for the entire
 bitstream, but builds a virtual decoder on the fly when moving
 between links in the chain. */

/* There are also different ways to implement seeking.  Enough
 information exists in an Ogg bitstream to seek to
 sample-granularity positions in the output.  Or, one can seek by
 picking some portion of the stream roughly in the desired area if
 we only want coarse navigation through the stream. */

/*************************************************************************
 * Many, many internal helpers.  The intention is not to be confusing; 
 * rampant duplication and monolithic function implementation would be 
 * harder to understand anyway.  The high level functions are last.  Begin
 * grokking near the end of the file */
//-------------------------------------------------------------------------------------------------
/* read a little more data from the file/pipe into the ogg_sync framer */
long _get_data(OggVorbis_File *vf) {
//	errno = 0;
//	if (vf->datasource) {
//		unsigned char *buffer = ogg_sync_bufferin(vf->oy, CHUNKSIZE);
//		long bytes = (vf->callbacks.read_func)(buffer, 1, CHUNKSIZE,
//				vf->datasource);
//		if (bytes > 0)
//			ogg_sync_wrote(vf->oy, bytes);
//		if (bytes == 0 && errno)
//			return -1;
//		return bytes;
//	} else
//		return 0;
}
//-------------------------------------------------------------------------------------------------
/* save a tiny smidge of verbosity to make the code more readable */
void _seek_helper(OggVorbis_File *vf, int64_t offset) {
	printf("seek");
	if (vf->datasource) {
		(vf->callbacks.seek_func)(vf->datasource, offset, SEEK_SET);
		vf->offset = offset;
		ogg_sync_reset(vf->oy);
	} else {
		/* shouldn't happen unless someone writes a broken callback */
		return;
	}
}
//-------------------------------------------------------------------------------------------------
/* The read/seek functions track absolute position within the stream */

/* from the head of the stream, get the next page.  boundary specifies
 if the function is allowed to fetch more data from the stream (and
 how much) or only use internally buffered data.

 boundary: -1) unbounded search
 0) read no additional data; use cached only
 n) search for a new page beginning for n bytes

 return:   <0) did not find a page (OV_FALSE, OV_EOF, OV_EREAD)
 n) found a page at absolute offset n

 produces a refcounted page */

int64_t _get_next_page(OggVorbis_File *vf, ogg_page *og, int64_t boundary) {
//	printf("get");
//	if (boundary > 0)
//		boundary += vf->offset;
//	while (1) {
//		long more;
//
//		if (boundary > 0 && vf->offset >= boundary)
//			return OV_FALSE;
//		more = ogg_sync_pageseek(vf->oy, og);
//
//		if (more < 0) {
//			/* skipped n bytes */
//			vf->offset -= more;
//		} else {
//			if (more == 0) {
//				/* send more paramedics */
//				if (!boundary)
//					return OV_FALSE;
//				{
//					long ret = _get_data(vf);
//					if (ret == 0)
//						return OV_EOF;
//					if (ret < 0)
//						return OV_EREAD;
//				}
//			} else {
//				/* got a page.  Return the offset at the page beginning,
//				 advance the internal offset past the page end */
//				int64_t ret = vf->offset;
//				vf->offset += more;
//				return ret;
//
//			}
//		}
//	}
}
//-------------------------------------------------------------------------------------------------
/* find the latest page beginning before the current stream cursor
 position. Much dirtier than the above as Ogg doesn't have any
 backward search linkage.  no 'readp' as it will certainly have to
 read. */
/* returns offset or OV_EREAD, OV_FAULT and produces a refcounted page */

int64_t _get_prev_page(OggVorbis_File *vf, ogg_page *og) {
	int64_t begin = vf->offset;
	int64_t end = begin;
	int64_t ret;
	int64_t offset = -1;

	while (offset == -1) {
		begin -= CHUNKSIZE;
		if (begin < 0)
			begin = 0;
		_seek_helper(vf, begin);
		while (vf->offset < end) {
			ret = _get_next_page(vf, og, end - vf->offset);
			if (ret == OV_EREAD)
				return OV_EREAD;
			if (ret < 0) {
				break;
			} else {
				offset = ret;
			}
		}
	}

	/* we have the offset.  Actually snork and hold the page now */
	_seek_helper(vf, offset);
	ret = _get_next_page(vf, og, CHUNKSIZE);
	if (ret < 0)
		/* this shouldn't be possible */
		return OV_EFAULT;

	return offset;
}
//-------------------------------------------------------------------------------------------------
/* finds each bitstream link one at a time using a bisection search
 (has to begin by knowing the offset of the lb's initial page).
 Recurses for each link so it can alloc the link storage after
 finding them all, then unroll and fill the cache at the same time */
int _bisect_forward_serialno(OggVorbis_File *vf, int64_t begin,	int64_t searched, int64_t end, uint32_t currentno, long m) {
	int64_t endsearched = end;
	int64_t next = end;
	ogg_page og = { 0, 0, 0, 0 };
	int64_t ret;

	/* the below guards against garbage seperating the last and
	 first pages of two links. */
	while (searched < endsearched) {
		int64_t bisect;

		if (endsearched - searched < CHUNKSIZE) {
			bisect = searched;
		} else {
			bisect = (searched + endsearched) / 2;
		}

		_seek_helper(vf, bisect);
		ret = _get_next_page(vf, &og, -1);
		if (ret == OV_EREAD)
			return OV_EREAD;
		if (ret < 0 || ogg_page_serialno(&og) != currentno) {
			endsearched = bisect;
			if (ret >= 0)
				next = ret;
		} else {
			searched = ret + og.header_len + og.body_len;
		}
		ogg_page_release(&og);
	}

	_seek_helper(vf, next);
	ret = _get_next_page(vf, &og, -1);
	if (ret == OV_EREAD)
		return OV_EREAD;

	if (searched >= end || ret < 0) {
		ogg_page_release(&og);
		vf->links = m + 1;
		vf->offsets = (int64_t*) malloc((vf->links + 1) * sizeof(*vf->offsets));
		vf->serialnos = (uint32_t*) malloc(vf->links * sizeof(*vf->serialnos));
		vf->offsets[m + 1] = searched;
	} else {
		ret = _bisect_forward_serialno(vf, next, vf->offset, end,
				ogg_page_serialno(&og), m + 1);
		ogg_page_release(&og);
		if (ret == OV_EREAD)
			return OV_EREAD;
	}

	vf->offsets[m] = begin;
	vf->serialnos[m] = currentno;
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* uses the local ogg_stream storage in vf; this is important for
 non-streaming input sources */
/* consumes the page that's passed in (if any) */
/* state is LINKSET upon successful return */

int _fetch_headers(OggVorbis_File *vf, vorbis_info *vi,	vorbis_comment *vc, uint32_t *serialno, ogg_page *og_ptr) {
	ogg_page og = { 0, 0, 0, 0 };
	ogg_packet op = { 0, 0, 0, 0, 0, 0 };
	int i, ret;

	if (!og_ptr) {
		int64_t llret = _get_next_page(vf, &og, CHUNKSIZE);
		if (llret == OV_EREAD)
			return OV_EREAD;
		if (llret < 0)
			return OV_ENOTVORBIS;
		og_ptr = &og;
	}

	ogg_stream_reset_serialno(vf->os, ogg_page_serialno(og_ptr));
	if (serialno)
		*serialno = vf->os->serialno;

	/* extract the initial header from the first page and verify that the
	 Ogg bitstream is in fact Vorbis data */

	vorbis_info_init(vi);
	vorbis_comment_init(vc);

	i = 0;
	while (i < 3) {
		ogg_stream_pagein(vf->os, og_ptr);
		while (i < 3) {
			int result = ogg_stream_packetout(vf->os, &op);
			if (result == 0)
				break;
			if (result == -1) {
				ret = OV_EBADHEADER;
				goto bail_header;
			}
			if ((ret = vorbis_dsp_headerin(vi, vc, &op))) {
				goto bail_header;
			}
			i++;
		}
		if (i < 3)
			if (_get_next_page(vf, og_ptr, CHUNKSIZE) < 0) {
				ret = OV_EBADHEADER;
				goto bail_header;
			}
	}

	ogg_packet_release(&op);
	ogg_page_release(&og);
	vf->ready_state = LINKSET;
	return 0;

	bail_header: ogg_packet_release(&op);
	ogg_page_release(&og);
	vorbis_info_clear(vi);
	vorbis_comment_clear(vc);
	vf->ready_state = OPENED;

	return ret;
}
//-------------------------------------------------------------------------------------------------
/* we no longer preload all vorbis_info (and the associated
 codec_setup) structs.  Call this to seek and fetch the info from
 the bitstream, if needed */
int _set_link_number(OggVorbis_File *vf, int link) {
	if (vf->ready_state < STREAMSET) {
		_seek_helper(vf, vf->offsets[link]);
		ogg_stream_reset_serialno(vf->os, vf->serialnos[link]);
		vf->current_serialno = vf->serialnos[link];
		vf->current_link = link;
		return _fetch_headers(vf, &vf->vi, &vf->vc, &vf->current_serialno, NULL);
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* last step of the OggVorbis_File initialization; get all the offset
 positions.  Only called by the seekable initialization (local
 stream storage is hacked slightly; pay attention to how that's
 done) */

/* this is void and does not propogate errors up because we want to be
 able to open and use damaged bitstreams as well as we can.  Just
 watch out for missing information for links in the OggVorbis_File
 struct */
void _prefetch_all_offsets(OggVorbis_File *vf, int64_t dataoffset) {
	ogg_page og = { 0, 0, 0, 0 };
	int i;
	int64_t ret;

	vf->dataoffsets = (int64_t*) malloc(vf->links * sizeof(*vf->dataoffsets));
	vf->pcmlengths = (int64_t*) malloc(vf->links * 2 * sizeof(*vf->pcmlengths));

	for (i = 0; i < vf->links; i++) {
		if (i == 0) {
			/* we already grabbed the initial header earlier.  Just set the offset */
			vf->dataoffsets[i] = dataoffset;
			_seek_helper(vf, dataoffset);

		} else {

			/* seek to the location of the initial header */

			_seek_helper(vf, vf->offsets[i]);
			if (_fetch_headers(vf, &vf->vi, &vf->vc, NULL, NULL) < 0) {
				vf->dataoffsets[i] = -1;
			} else {
				vf->dataoffsets[i] = vf->offset;
			}
		}

		/* fetch beginning PCM offset */

		if (vf->dataoffsets[i] != -1) {
			int64_t accumulated = 0, pos;
			long lastblock = -1;
			int result;

			ogg_stream_reset_serialno(vf->os, vf->serialnos[i]);

			while (1) {
				ogg_packet op = { 0, 0, 0, 0, 0, 0 };

				ret = _get_next_page(vf, &og, -1);
				if (ret < 0)
					/* this should not be possible unless the file is
					 truncated/mangled */
					break;

				if (ogg_page_serialno(&og) != vf->serialnos[i])
					break;

				pos = ogg_page_granulepos(&og);

				/* count blocksizes of all frames in the page */
				ogg_stream_pagein(vf->os, &og);
				while ((result = ogg_stream_packetout(vf->os, &op))) {
					if (result > 0) { /* ignore holes */
						long thisblock = vorbis_packet_blocksize(&vf->vi, &op);
						if (lastblock != -1)
							accumulated += (lastblock + thisblock) >> 2;
						lastblock = thisblock;
					}
				}
				ogg_packet_release(&op);

				if (pos != -1) {
					/* pcm offset of last packet on the first audio page */
					accumulated = pos - accumulated;
					break;
				}
			}

			/* less than zero?  This is a stream with samples trimmed off
			 the beginning, a normal occurrence; set the offset to zero */
			if (accumulated < 0)
				accumulated = 0;

			vf->pcmlengths[i * 2] = accumulated;
		}

		/* get the PCM length of this link. To do this,
		 get the last page of the stream */
		{
			int64_t end = vf->offsets[i + 1];
			_seek_helper(vf, end);

			while (1) {
				ret = _get_prev_page(vf, &og);
				if (ret < 0) {
					/* this should not be possible */
					vorbis_info_clear(&vf->vi);
					vorbis_comment_clear(&vf->vc);
					break;
				}
				if (ogg_page_granulepos(&og) != -1) {
					vf->pcmlengths[i * 2 + 1] = ogg_page_granulepos(&og)
							- vf->pcmlengths[i * 2];
					break;
				}
				vf->offset = ret;
			}
		}
	}
	ogg_page_release(&og);
}
//-------------------------------------------------------------------------------------------------
int _make_decode_ready(OggVorbis_File *vf) {
	int i;
	switch (vf->ready_state) {
	case OPENED:
	case STREAMSET:
		for (i = 0; i < vf->links; i++)
			if (vf->offsets[i + 1] >= vf->offset)
				break;
		if (i == vf->links)
			return -1;
		/* fall through */
	case LINKSET:
		vf->vd = vorbis_dsp_create(&vf->vi);
		vf->ready_state = INITSET;
		vf->bittrack = 0;
		vf->samptrack = 0; // @suppress("No break at end of case")
	case INITSET:
		return 0;
	default:
		return -1;
	}

}
//-------------------------------------------------------------------------------------------------
int _open_seekable2(OggVorbis_File *vf) {
	uint32_t serialno = vf->current_serialno;
	uint32_t tempserialno;
	int64_t dataoffset = vf->offset, end;
	ogg_page og = { 0, 0, 0, 0 };

	/* we're partially open and have a first link header state in
	 storage in vf */
	/* we can seek, so set out learning all about this file */
	(vf->callbacks.seek_func)(vf->datasource, 0, SEEK_END);
	vf->offset = vf->end = (vf->callbacks.tell_func)(vf->datasource);

	/* We get the offset for the last page of the physical bitstream.
	 Most OggVorbis files will contain a single logical bitstream */
	end = _get_prev_page(vf, &og);
	if (end < 0)
		return end;

	/* more than one logical bitstream? */
	tempserialno = ogg_page_serialno(&og);
	ogg_page_release(&og);

	if (tempserialno != serialno) {

		/* Chained bitstream. Bisect-search each logical bitstream
		 section.  Do so based on serial number only */
		if (_bisect_forward_serialno(vf, 0, 0, end + 1, serialno, 0) < 0)
			return OV_EREAD;

	} else {

		/* Only one logical bitstream */
		if (_bisect_forward_serialno(vf, 0, end, end + 1, serialno, 0))
			return OV_EREAD;

	}

	/* the initial header memory is referenced by vf after; don't free it */
	_prefetch_all_offsets(vf, dataoffset);
	return ov_raw_seek(vf, 0);
}
//-------------------------------------------------------------------------------------------------
/* fetch and process a packet.  Handles the case where we're at a
 bitstream boundary and dumps the decoding machine.  If the decoding
 machine is unloaded, it loads it.  It also keeps pcm_offset up to
 date (seek and read both use this.  seek uses a special hack with
 readp).

 return: <0) error, OV_HOLE (lost packet) or OV_EOF
 0) need more data (only if readp==0)
 1) got a packet
 */

int _fetch_and_process_packet(OggVorbis_File *vf, int readp, int spanp) {

	ogg_page og = { 0, 0, 0, 0 };
	ogg_packet op = { 0, 0, 0, 0, 0, 0 };
	int ret = 0;

	/* handle one packet.  Try to fetch it from current stream state */
	/* extract packets from page */
	while (1) {

		/* process a packet if we can.  If the machine isn't loaded,
		 neither is a page */
		if (vf->ready_state == INITSET) {
			while (1) {
				int result = ogg_stream_packetout(vf->os, &op);
				int64_t granulepos;

				if (result < 0) {
					ret = OV_HOLE; /* hole in the data. */
					goto cleanup;
				}
				if (result > 0) {
					/* got a packet.  process it */
					granulepos = op.granulepos;
					if (!vorbis_dsp_synthesis(vf->vd, &op, 1)) { /* lazy check for lazy
					 header handling.  The
					 header packets aren't
					 audio, so if/when we
					 submit them,
					 vorbis_synthesis will
					 reject them */

						vf->samptrack += vorbis_dsp_pcmout(vf->vd, NULL, 0);
						vf->bittrack += op.bytes * 8;

						/* update the pcm offset. */
						if (granulepos != -1 && !op.e_o_s) {
							int link = (vf->seekable ? vf->current_link : 0);
							int i, samples;

							/* this packet has a pcm_offset on it (the last packet
							 completed on a page carries the offset) After processing
							 (above), we know the pcm position of the *last* sample
							 ready to be returned. Find the offset of the *first*

							 As an aside, this trick is inaccurate if we begin
							 reading anew right at the last page; the end-of-stream
							 granulepos declares the last frame in the stream, and the
							 last packet of the last page may be a partial frame.
							 So, we need a previous granulepos from an in-sequence page
							 to have a reference point.  Thus the !op.e_o_s clause
							 above */

							if (vf->seekable && link > 0)
								granulepos -= vf->pcmlengths[link * 2];
							if (granulepos < 0)
								granulepos = 0; /* actually, this
								 shouldn't be possible
								 here unless the stream
								 is very broken */

							samples = vorbis_dsp_pcmout(vf->vd, NULL, 0);

							granulepos -= samples;
							for (i = 0; i < link; i++)
								granulepos += vf->pcmlengths[i * 2 + 1];
							vf->pcm_offset = granulepos;
						}
						ret = 1;
						goto cleanup;
					}
				} else
					break;
			}
		}

		if (vf->ready_state >= OPENED) {
			int ret;
			if (!readp) {
				ret = 0;
				goto cleanup;
			}
			if ((ret = _get_next_page(vf, &og, -1)) < 0) {
				ret = OV_EOF; /* eof. leave unitialized */
				goto cleanup;
			}

			/* bitrate tracking; add the header's bytes here, the body bytes
			 are done by packet above */
			vf->bittrack += og.header_len * 8;

			/* has our decoding just traversed a bitstream boundary? */
			if (vf->ready_state == INITSET) {
				if (vf->current_serialno != ogg_page_serialno(&og)) {
					if (!spanp) {
						ret = OV_EOF;
						goto cleanup;
					}
				}
			}
		}

		/* Do we need to load a new machine before submitting the page? */
		/* This is different in the seekable and non-seekable cases.

		 In the seekable case, we already have all the header
		 information loaded and cached; we just initialize the machine
		 with it and continue on our merry way.

		 In the non-seekable (streaming) case, we'll only be at a
		 boundary if we just left the previous logical bitstream and
		 we're now nominally at the header of the next bitstream
		 */

		if (vf->ready_state != INITSET) {
			int link, ret;

			if (vf->ready_state < STREAMSET) {
				if (vf->seekable) {
					vf->current_serialno = ogg_page_serialno(&og);

					/* match the serialno to bitstream section.  We use this rather than
					 offset positions to avoid problems near logical bitstream
					 boundaries */
					for (link = 0; link < vf->links; link++)
						if (vf->serialnos[link] == vf->current_serialno)
							break;
					if (link == vf->links) {
						ret = OV_EBADLINK; /* sign of a bogus stream.  error out,
						 leave machine uninitialized */
						goto cleanup;
					}

					vf->current_link = link;
					ret = _fetch_headers(vf, &vf->vi, &vf->vc,
							&vf->current_serialno, &og);
					if (ret)
						goto cleanup;

				} else {
					/* we're streaming */
					/* fetch the three header packets, build the info struct */

					int ret = _fetch_headers(vf, &vf->vi, &vf->vc,
							&vf->current_serialno, &og);
					if (ret)
						goto cleanup;
					vf->current_link++;
				}
			}

			if (_make_decode_ready(vf))
				return OV_EBADLINK;
		}
		ogg_stream_pagein(vf->os, &og);
	}
	cleanup: ogg_packet_release(&op);
	ogg_page_release(&og);
	return ret;
}
//-------------------------------------------------------------------------------------------------
/* if, eg, 64 bit stdio is configured by default, this will build with
 fseek64 */
int _fseek64_wrap(FILE *f, int64_t off, int whence) {
	if (f == NULL)
		return -1;
	return fseek(f, off, whence);
}
//-------------------------------------------------------------------------------------------------
int _ov_open1(void *f, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks) {
	int offsettest = (f ? callbacks.seek_func(f, 0, SEEK_CUR) : -1);
	int ret;

	memset(vf, 0, sizeof(*vf));

	/* Tremor assumes in multiple places that right shift of a signed
	 integer is an arithmetic shift */
	if ((-1 >> 1) != -1)
		return OV_EIMPL;

	vf->datasource = f;
	vf->callbacks = callbacks;

	/* init the framing state */
	vf->oy = ogg_sync_create();

	/* perhaps some data was previously read into a buffer for testing
	 against other stream types.  Allow initialization from this
	 previously read data (as we may be reading from a non-seekable
	 stream) */
	if (initial) {
		unsigned char *buffer = ogg_sync_bufferin(vf->oy, ibytes);
		memcpy(buffer, initial, ibytes);
		ogg_sync_wrote(vf->oy, ibytes);
	}

	/* can we seek? Stevens suggests the seek test was portable */
	if (offsettest != -1)
		vf->seekable = 1;

	/* No seeking yet; Set up a 'single' (current) logical bitstream
	 entry for partial open */
	vf->links = 1;
	vf->os = ogg_stream_create(-1); /* fill in the serialno later */

	/* Try to fetch the headers, maintaining all the storage */
	if ((ret = _fetch_headers(vf, &vf->vi, &vf->vc, &vf->current_serialno, NULL))
			< 0) {
		vf->datasource = NULL;
		ov_clear(vf);
	} else if (vf->ready_state < PARTOPEN)
		vf->ready_state = PARTOPEN;
	return ret;
}
//-------------------------------------------------------------------------------------------------
int _ov_open2(OggVorbis_File *vf) {
	if (vf->ready_state < OPENED)
		vf->ready_state = OPENED;
	if (vf->seekable) {
		int ret = _open_seekable2(vf);
		if (ret) {
			vf->datasource = NULL;
			ov_clear(vf);
		}
		return ret;
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* clear out the OggVorbis_File struct */
int ov_clear(OggVorbis_File *vf) {
	if (vf) {
		vorbis_dsp_destroy(vf->vd);
		vf->vd = 0;
		ogg_stream_destroy(vf->os);
		vorbis_info_clear(&vf->vi);
		vorbis_comment_clear(&vf->vc);
		if (vf->dataoffsets)
			free(vf->dataoffsets);
		if (vf->pcmlengths)
			free(vf->pcmlengths);
		if (vf->serialnos)
			free(vf->serialnos);
		if (vf->offsets)
			free(vf->offsets);
		ogg_sync_destroy(vf->oy);

		if (vf->datasource)
			(vf->callbacks.close_func)(vf->datasource);
		memset(vf, 0, sizeof(*vf));
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* inspects the OggVorbis file and finds/documents all the logical
 bitstreams contained in it.  Tries to be tolerant of logical
 bitstream sections that are truncated/woogie.

 return: -1) error
 0) OK
 */

int ov_open_callbacks(void *f, OggVorbis_File *vf, char *initial, long ibytes, ov_callbacks callbacks) {
	int ret = _ov_open1(f, vf, initial, ibytes, callbacks);
	if (ret)
		return ret;
	return _ov_open2(vf);
}
//-------------------------------------------------------------------------------------------------
int ov_open(FILE *f, OggVorbis_File *vf, char *initial, int32_t ibytes) {
	ov_callbacks callbacks = { (size_t (*)(void*, size_t, size_t, void*)) fread,
			(int (*)(void*, int64_t, int)) _fseek64_wrap,
			(int (*)(void*)) fclose, (int32_t (*)(void*)) ftell };

	return ov_open_callbacks((void*) f, vf, initial, ibytes, callbacks);
}
//-------------------------------------------------------------------------------------------------
/* returns the bitrate for a given logical bitstream or the entire
 physical bitstream.  If the file is open for random access, it will
 find the *actual* average bitrate.  If the file is streaming, it
 returns the nominal bitrate (if set) else the average of the
 upper/lower bounds (if set) else -1 (unset).

 If you want the actual bitrate field settings, get them from the
 vorbis_info structs */

long ov_bitrate(OggVorbis_File *vf, int i) {
	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	if (i >= vf->links)
		return OV_EINVAL;
	if (!vf->seekable && i != 0)
		return ov_bitrate(vf, 0);
	if (i < 0) {
		int64_t bits = 0;
		int i;
		for (i = 0; i < vf->links; i++)
			bits += (vf->offsets[i + 1] - vf->dataoffsets[i]) * 8;
		/* This once read: return(rint(bits/ov_time_total(vf,-1)));
		 * gcc 3.x on x86 miscompiled this at optimisation level 2 and above,
		 * so this is slightly transformed to make it work.
		 */
		return bits * 1000 / ov_time_total(vf, -1);
	} else {
		if (vf->seekable) {
			/* return the actual bitrate */
			return (vf->offsets[i + 1] - vf->dataoffsets[i]) * 8000
					/ ov_time_total(vf, i);
		} else {
			/* return nominal if set */
			if (vf->vi.bitrate_nominal > 0) {
				return vf->vi.bitrate_nominal;
			} else {
				if (vf->vi.bitrate_upper > 0) {
					if (vf->vi.bitrate_lower > 0) {
						return (vf->vi.bitrate_upper + vf->vi.bitrate_lower) / 2;
					} else {
						return vf->vi.bitrate_upper;
					}
				}
				return OV_FALSE;
			}
		}
	}
}
//-------------------------------------------------------------------------------------------------
/* returns the actual bitrate since last call.  returns -1 if no
 additional data to offer since last call (or at beginning of stream),
 EINVAL if stream is only partially open
 */
long ov_bitrate_instant(OggVorbis_File *vf) {
	long ret;
	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	if (vf->samptrack == 0)
		return OV_FALSE;
	ret = vf->bittrack / vf->samptrack * vf->vi.rate;
	vf->bittrack = 0;
	vf->samptrack = 0;
	return ret;
}
//-------------------------------------------------------------------------------------------------
/* Guess */
long ov_serialnumber(OggVorbis_File *vf, int i) {
	if (i >= vf->links)
		return ov_serialnumber(vf, vf->links - 1);
	if (!vf->seekable && i >= 0)
		return ov_serialnumber(vf, -1);
	if (i < 0) {
		return vf->current_serialno;
	} else {
		return vf->serialnos[i];
	}
}
//-------------------------------------------------------------------------------------------------
/* returns: total raw (compressed) length of content if i==-1
 raw (compressed) length of that logical bitstream for i==0 to n
 OV_EINVAL if the stream is not seekable (we can't know the length)
 or if stream is only partially open
 */
int64_t ov_raw_total(OggVorbis_File *vf, int i) {
	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	if (!vf->seekable || i >= vf->links)
		return OV_EINVAL;
	if (i < 0) {
		int64_t acc = 0;
		int i;
		for (i = 0; i < vf->links; i++)
			acc += ov_raw_total(vf, i);
		return acc;
	} else {
		return vf->offsets[i + 1] - vf->offsets[i];
	}
}
//-------------------------------------------------------------------------------------------------
/* returns: total PCM length (samples) of content if i==-1 PCM length
 (samples) of that logical bitstream for i==0 to n
 OV_EINVAL if the stream is not seekable (we can't know the
 length) or only partially open
 */
int64_t ov_pcm_total(OggVorbis_File *vf, int i) {
	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	if (!vf->seekable || i >= vf->links)
		return OV_EINVAL;
	if (i < 0) {
		int64_t acc = 0;
		int i;
		for (i = 0; i < vf->links; i++)
			acc += ov_pcm_total(vf, i);
		return acc;
	} else {
		return vf->pcmlengths[i * 2 + 1];
	}
}
//-------------------------------------------------------------------------------------------------
/* returns: total milliseconds of content if i==-1
 milliseconds in that logical bitstream for i==0 to n
 OV_EINVAL if the stream is not seekable (we can't know the
 length) or only partially open
 */
int64_t ov_time_total(OggVorbis_File *vf, int i) {
	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	if (!vf->seekable || i >= vf->links)
		return OV_EINVAL;
	if (i < 0) {
		int64_t acc = 0;
		int i;
		for (i = 0; i < vf->links; i++)
			acc += ov_time_total(vf, i);
		return acc;
	} else {
		return ((int64_t) vf->pcmlengths[i * 2 + 1]) * 1000 / vf->vi.rate;
	}
}
//-------------------------------------------------------------------------------------------------
/* seek to an offset relative to the *compressed* data. This also
 scans packets to update the PCM cursor. It will cross a logical
 bitstream boundary, but only if it can't get any packets out of the
 tail of the bitstream we seek to (so no surprises).

 returns zero on success, nonzero on failure */

int ov_raw_seek(OggVorbis_File *vf, int64_t pos) {
	ogg_stream_state *work_os = NULL;
	ogg_page og = { 0, 0, 0, 0 };
	ogg_packet op = { 0, 0, 0, 0, 0, 0 };

	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	if (!vf->seekable)
		return OV_ENOSEEK; /* don't dump machine if we can't seek */

	if (pos < 0 || pos > vf->end)
		return OV_EINVAL;

	/* don't yet clear out decoding machine (if it's initialized), in
	 the case we're in the same link.  Restart the decode lapping, and
	 let _fetch_and_process_packet deal with a potential bitstream
	 boundary */
	vf->pcm_offset = -1;
	ogg_stream_reset_serialno(vf->os, vf->current_serialno); /* must set serialno */
	vorbis_dsp_restart(vf->vd);

	_seek_helper(vf, pos);

	/* we need to make sure the pcm_offset is set, but we don't want to
	 advance the raw cursor past good packets just to get to the first
	 with a granulepos.  That's not equivalent behavior to beginning
	 decoding as immediately after the seek position as possible.

	 So, a hack.  We use two stream states; a local scratch state and
	 the shared vf->os stream state.  We use the local state to
	 scan, and the shared state as a buffer for later decode.

	 Unfortuantely, on the last page we still advance to last packet
	 because the granulepos on the last page is not necessarily on a
	 packet boundary, and we need to make sure the granpos is
	 correct.
	 */

	{
		int lastblock = 0;
		int accblock = 0;
		int thisblock;
		int eosflag;

		work_os = ogg_stream_create(vf->current_serialno); /* get the memory ready */
		while (1) {
			if (vf->ready_state >= STREAMSET) {
				/* snarf/scan a packet if we can */
				int result = ogg_stream_packetout(work_os, &op);

				if (result > 0) {

					if (vf->vi.codec_setup) {
						thisblock = vorbis_packet_blocksize(&vf->vi, &op);
						if (thisblock < 0) {
							ogg_stream_packetout(vf->os, NULL);
							thisblock = 0;
						} else {

							if (eosflag)
								ogg_stream_packetout(vf->os, NULL);
							else if (lastblock)
								accblock += (lastblock + thisblock) >> 2;
						}

						if (op.granulepos != -1) {
							int i, link = vf->current_link;
							int64_t granulepos = op.granulepos
									- vf->pcmlengths[link * 2];
							if (granulepos < 0)
								granulepos = 0;

							for (i = 0; i < link; i++)
								granulepos += vf->pcmlengths[i * 2 + 1];
							vf->pcm_offset = granulepos - accblock;
							break;
						}
						lastblock = thisblock;
						continue;
					} else
						ogg_stream_packetout(vf->os, NULL);
				}
			}

			if (!lastblock) {
				if (_get_next_page(vf, &og, -1) < 0) {
					vf->pcm_offset = ov_pcm_total(vf, -1);
					break;
				}
			} else {
				/* huh?  Bogus stream with packets but no granulepos */
				vf->pcm_offset = -1;
				break;
			}

			/* did we just grab a page from other than current link? */
			if (vf->ready_state >= STREAMSET)

			if (vf->ready_state < STREAMSET) {
				int link;

				vf->current_serialno = ogg_page_serialno(&og);
				for (link = 0; link < vf->links; link++)
					if (vf->serialnos[link] == vf->current_serialno)
						break;
				if (link == vf->links)
					goto seek_error;
				/* sign of a bogus stream.  error out,
				 leave machine uninitialized */

				ogg_stream_reset_serialno(vf->os, vf->current_serialno);
				ogg_stream_reset_serialno(work_os, vf->current_serialno);

			}

			{
				ogg_page dup;
				ogg_page_dup(&dup, &og);
				eosflag = ogg_page_eos(&og);
				ogg_stream_pagein(vf->os, &og);
				ogg_stream_pagein(work_os, &dup);
			}
		}
	}

	ogg_packet_release(&op);
	ogg_page_release(&og);
	ogg_stream_destroy(work_os);
	vf->bittrack = 0;
	vf->samptrack = 0;
	return 0;

	seek_error: ogg_packet_release(&op);
	ogg_page_release(&og);

	/* dump the machine so we're in a known state */
	vf->pcm_offset = -1;
	ogg_stream_destroy(work_os);
	return OV_EBADLINK;
}
//-------------------------------------------------------------------------------------------------
/* seek to a sample offset relative to the decompressed pcm stream
 returns zero on success, nonzero on failure */

int ov_pcm_seek(OggVorbis_File *vf, int64_t pos) {
	ogg_packet op = { 0, 0, 0, 0, 0, 0 };
	ogg_page og = { 0, 0, 0, 0 };
	int thisblock, lastblock = 0;
	if (_make_decode_ready(vf))
		return OV_EBADLINK;

	/* discard leading packets we don't need for the lapping of the
	 position we want; don't decode them */

	while (1) {

		int ret = ogg_stream_packetpeek(vf->os, &op);
		if (ret > 0) {
			thisblock = vorbis_packet_blocksize(&vf->vi, &op);
			if (thisblock < 0) {
				ogg_stream_packetout(vf->os, NULL);
				continue; /* non audio packet */
			}
			if (lastblock)
				vf->pcm_offset += (lastblock + thisblock) >> 2;

			if (vf->pcm_offset
					+ ((thisblock + vorbis_info_blocksize(&vf->vi, 1)) >> 2)
					>= pos)
				break;

			/* remove the packet from packet queue and track its granulepos */
			ogg_stream_packetout(vf->os, NULL);
			vorbis_dsp_synthesis(vf->vd, &op, 0); /* set up a vb with
			 only tracking, no
			 pcm_decode */

			/* end of logical stream case is hard, especially with exact
			 length positioning. */

			if (op.granulepos > -1) {
				int i;
				/* always believe the stream markers */
				vf->pcm_offset = op.granulepos
						- vf->pcmlengths[vf->current_link * 2];
				if (vf->pcm_offset < 0)
					vf->pcm_offset = 0;
				for (i = 0; i < vf->current_link; i++)
					vf->pcm_offset += vf->pcmlengths[i * 2 + 1];
			}

			lastblock = thisblock;

		} else {
			if (ret < 0 && ret != OV_HOLE)
				break;

			/* suck in a new page */
//			if (_get_next_page(vf, &og, -1) < 0)
				break;

			if (vf->ready_state < STREAMSET) {
				int link, ret;

				vf->current_serialno = ogg_page_serialno(&og);
				for (link = 0; link < vf->links; link++)
					if (vf->serialnos[link] == vf->current_serialno)
						break;
				if (link == vf->links) {
					ogg_page_release(&og);
					ogg_packet_release(&op);
					return OV_EBADLINK;
				}

				vf->current_link = link;
				ret = _fetch_headers(vf, &vf->vi, &vf->vc,
						&vf->current_serialno, &og);
				if (ret)
					return ret;
				if (_make_decode_ready(vf))
					return OV_EBADLINK;
				lastblock = 0;
			}

			ogg_stream_pagein(vf->os, &og);
		}
	}

	vf->bittrack = 0;
	vf->samptrack = 0;
	/* discard samples until we reach the desired position. Crossing a
	 logical bitstream boundary with abandon is OK. */
	while (vf->pcm_offset < pos) {
		int64_t target = pos - vf->pcm_offset;
		long samples = vorbis_dsp_pcmout(vf->vd, NULL, 0);

		if (samples > target)
			samples = target;
		vorbis_dsp_read(vf->vd, samples);
		vf->pcm_offset += samples;

		if (samples < target)
			if (_fetch_and_process_packet(vf, 1, 1) <= 0)
				vf->pcm_offset = ov_pcm_total(vf, -1); /* eof */
	}

	ogg_page_release(&og);
	ogg_packet_release(&op);
	return 0;
}
//-------------------------------------------------------------------------------------------------
/* return PCM offset (sample) of next PCM sample to be read */
int64_t ov_pcm_tell(OggVorbis_File *vf) {
	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	return vf->pcm_offset;
}
//-------------------------------------------------------------------------------------------------
/* return time offset (milliseconds) of next PCM sample to be read */
int64_t ov_time_tell(OggVorbis_File *vf) {
	int link = 0;
	int64_t pcm_total = 0;
	int64_t time_total = 0;

	if (vf->ready_state < OPENED)
		return OV_EINVAL;
	if (vf->seekable) {
		pcm_total = ov_pcm_total(vf, -1);
		time_total = ov_time_total(vf, -1);

		/* which bitstream section does this time offset occur in? */
		for (link = vf->links - 1; link >= 0; link--) {
			pcm_total -= vf->pcmlengths[link * 2 + 1];
			time_total -= ov_time_total(vf, link);
			if (vf->pcm_offset >= pcm_total)
				break;
		}
	}

	return time_total + (1000 * vf->pcm_offset - pcm_total) / vf->vi.rate;
}
//-------------------------------------------------------------------------------------------------
/*  link:   -1) return the vorbis_info struct for the bitstream section
 currently being decoded
 0-n) to request information for a specific bitstream section

 In the case of a non-seekable bitstream, any call returns the
 current bitstream.  NULL in the case that the machine is not
 initialized */

vorbis_info* ov_info(OggVorbis_File *vf, int link) {
	if (vf->seekable) {
		if (link >= vf->links)
			return NULL;
	}
	return &vf->vi;
}
//-------------------------------------------------------------------------------------------------
/* grr, strong typing, grr, no templates/inheritence, grr */
vorbis_comment* ov_comment(OggVorbis_File *vf, int link) {
	if (vf->seekable) {
		if (link >= vf->links)
			return NULL;
	}
	return &vf->vc;
}
//-------------------------------------------------------------------------------------------------
/* up to this point, everything could more or less hide the multiple
 logical bitstream nature of chaining from the toplevel application
 if the toplevel application didn't particularly care.  However, at
 the point that we actually read audio back, the multiple-section
 nature must surface: Multiple bitstream sections do not necessarily
 have to have the same number of channels or sampling rate.

 ov_read returns the sequential logical bitstream number currently
 being decoded along with the PCM data in order that the toplevel
 application can take action on channel/sample rate changes.  This
 number will be incremented even for streamed (non-seekable) streams
 (for seekable streams, it represents the actual logical bitstream
 index within the physical bitstream.  Note that the accessor
 functions above are aware of this dichotomy).

 input values: buffer) a buffer to hold packed PCM data for return
 length) the byte length requested to be placed into buffer

 return values: <0) error/hole in data (OV_HOLE), partial open (OV_EINVAL)
 0) EOF
 n) number of bytes of PCM actually returned.  The
 below works on a packet-by-packet basis, so the
 return length is not related to the 'length' passed
 in, just guaranteed to fit.

 *section) set to the logical bitstream number */

int32_t ov_read(OggVorbis_File *vf, void *buffer, int bytes_req) {

	long samples;
	long channels;

	if (vf->ready_state < OPENED)
		return OV_EINVAL;

	while (1) {
		if (vf->ready_state == INITSET) {
			channels = vf->vi.channels;
			samples = vorbis_dsp_pcmout(vf->vd, (int16_t*) buffer,
					(bytes_req >> 1) / channels);
			if (samples) {
				if (samples > 0) {
					vorbis_dsp_read(vf->vd, samples);
					vf->pcm_offset += samples;
					return samples * 2 * channels;
				}
				return samples;
			}
		}

		/* suck in another packet */
		{
			int ret = _fetch_and_process_packet(vf, 1, 1);
			if (ret == OV_EOF)
				return 0;
			if (ret <= 0)
				return ret;
		}

	}
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


