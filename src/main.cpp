
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ivorbiscodec.h"
#include "vorbisDecoder.h"
using namespace std;

char pcmout[4096];

int main() {
	OggVorbis_File vf;
	int eof = 0;
	// int current_section;
	// char buff[100];

	FILE *fptrIn = NULL;
	fptrIn = fopen("in.ogg", "r");
	if (fptrIn == NULL){printf("in.ogg not found\n");exit(1);}

	FILE *fptrOut = NULL;
	fptrOut = fopen("out.wav", "w");
	if (fptrIn == NULL){printf("out.ogg not found\n");exit(1);}

	if(ov_open(fptrIn, &vf, NULL, 0) < 0) {
		printf("Input does not appear to be an Ogg bitstream.\n");
	    exit(1);
	}

    char **ptr=ov_comment(&vf,-1)->user_comments;
    vorbis_info *vi=ov_info(&vf,-1);
    while(*ptr){
    	fprintf(stderr,"%s\n",*ptr);
    	++ptr;
    }
    fprintf(stderr,"\nBitstream is %d channel, %idHz\n",vi->channels,vi->rate);
    fprintf(stderr,"\nDecoded length: %ld samples\n", (long)ov_pcm_total(&vf,-1));
    fprintf(stderr,"Encoded by: %s\n\n",ov_comment(&vf,-1)->vendor);

    // Write the WAV header
    fwrite("RIFF", 4, 1, fptrOut); // Chunk ID
    fwrite("\x00\x00\x00\x00", 4, 1, fptrOut); // Chunk Size (will be filled later)
    fwrite("WAVE", 4, 1, fptrOut); // Format
    fwrite("fmt ", 4, 1, fptrOut); // Subchunk 1 ID
    fwrite("\x10\x00\x00\x00", 4, 1, fptrOut); // Subchunk 1 Size
    fwrite("\x01\x00", 2, 1, fptrOut); // AudioFormat
    fwrite("\x02\x00", 2, 1, fptrOut); // NumChannels
    fwrite("\x44\xAC\x00\x00", 4, 1, fptrOut); // Sample Rate
    fwrite("\x44\xAC\x00\x00", 4, 1, fptrOut); // Byte Rate
    fwrite("\x02\x00", 2, 1, fptrOut); // Block Align
    fwrite("\x10\x00", 2, 1, fptrOut); // Bits Per Sample
    fwrite("data", 4, 1, fptrOut); // Subchunk 2 ID
    fwrite("\x00\x00\x00\x00", 4, 1, fptrOut); // Subchunk 2 Size (will be filled later)

    while(!eof){
    	long ret=ov_read(&vf,pcmout,sizeof(pcmout));
    	if (ret == 0) {  		/* EOF */
    		eof=1;
    	} else if (ret < 0) {
    		/* error in the stream.  Not a problem, just reporting it in case we (the app) cares.  In this case, we don't. */
    	} else {
    		/* we don't bother dealing with sample rate changes, etc, but you'll have to*/
    		fwrite(pcmout,1, ret, fptrOut);
    	}
    }

    /* cleanup */
    ov_clear(&vf);

	printf("ready!!!\n"); // prints !!!Hello World!!!
	return 0;
}
