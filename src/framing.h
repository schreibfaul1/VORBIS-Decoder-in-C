#pragma once
#include "vorbisfile.h"

extern ogg_buffer* _fetch_buffer(ogg_buffer_state *bs, long bytes);
unsigned char* ogg_sync_bufferin(ogg_sync_state_t *oy, long bytes);