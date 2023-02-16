#pragma once

#define _lookspan()   while(!end){\
                        head=head->next;\
                        if(!head) return -1;\
                        ptr=head->buffer->data + head->begin;\
                        end=head->length;\
                      }





void _span(oggpack_buffer *b);
void oggpack_readinit(oggpack_buffer *b, ogg_reference *r);
int32_t oggpack_look(oggpack_buffer *b, int bits);
void oggpack_adv(oggpack_buffer *b, int bits);
int oggpack_eop(oggpack_buffer *b);
int32_t oggpack_read(oggpack_buffer *b, int bits);
int32_t oggpack_bytes(oggpack_buffer *b);
int32_t oggpack_bits(oggpack_buffer *b);




