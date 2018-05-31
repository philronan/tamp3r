/*

    .............................................................
    ....####...######...####...##...##..#####...######..#####....
    ...##........##....##..##..###.###..##..##.....##...##..##...
    ....####.....##....######..##.#.##..#####.....###...#####....
    .......##....##....##..##..##...##..##..........##..##..##...
    ....####.....##....##..##..##...##..##......#####...##..##...
    .............................................................

    A command-line utility for tweaking private bits in MP3 files

    Author:  Phil Ronan

    License: MIT (See LICENSE.md for details)

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "stamp3r.h"

unpacked_frame_header *unpack_header(const unsigned char *src) {
	/* Unpack the bitfield data from an MP3 frame header and return */
	/* as a pointer of type *unpacked_frame_header if valid, else NULL.  */
	/* (Note: "valid" just means the frame sync bits are present.)  */

	static unpacked_frame_header h;

	/* Exit immediately if the frame sync bits are missing */
	if ( src[0] != 0xff || src[1] < 0xe0 ) return NULL;

	/* Unpack the frame header */
	h.version    =  (src[1] >> 3) & 0x03;
	h.layer      =  (src[1] >> 1) & 0x03;
	h.protection =  src[1] & 0x01;
	h.bitratex   =  src[2] >> 4;
	h.sfreqx     =  (src[2] >> 2) & 0x03;
	h.padding    =  (src[2] >> 1) & 0x01;
	h.private    =  src[2] & 0x01;
	h.mode       =  src[3] >> 6;
	h.modext     =  (src[3] >> 4) & 0x03;
	h.copyright  =  (src[3] >> 3) & 0x01;
	h.original   =  (src[3] >> 2) & 0x01;
	h.emphasis   =  src[3] & 0x03;
	h.crc = (h.protection) ? ((int)src[4] << 8) + (int)src[5] : 0;
	return &h;
}


int get_bitrate(const unpacked_frame_header *p) {
	/* Returns the bit rate corresponding to frame header: */
	/* 0 for a "free" bit rate (unusable), -1 for bad data */

	const int bitrates[16][2][3] = {
		{ {      0,      0,      0 }, {      0,      0,      0 } },
		{ {  32000,  32000,  32000 }, {  32000,   8000,   8000 } },
		{ {  64000,  48000,  40000 }, {  48000,  16000,  16000 } },
		{ {  96000,  56000,  48000 }, {  56000,  24000,  24000 } },
		{ { 128000,  64000,  56000 }, {  64000,  32000,  32000 } },
		{ { 160000,  80000,  64000 }, {  80000,  40000,  40000 } },
		{ { 192000,  96000,  80000 }, {  96000,  48000,  48000 } },
		{ { 224000, 112000,  96000 }, { 112000,  56000,  56000 } },
		{ { 256000, 128000, 112000 }, { 128000,  64000,  64000 } },
		{ { 288000, 160000, 128000 }, { 144000,  80000,  80000 } },
		{ { 320000, 192000, 160000 }, { 160000,  96000,  96000 } },
		{ { 352000, 224000, 192000 }, { 176000, 112000, 112000 } },
		{ { 384000, 256000, 224000 }, { 192000, 128000, 128000 } },
		{ { 416000, 320000, 256000 }, { 224000, 144000, 144000 } },
		{ { 448000, 384000, 320000 }, { 256000, 160000, 160000 } },
		{ {     -1,     -1,     -1 }, {     -1,     -1,     -1 } }
	};
	int rate = p->bitratex, version = p->version, layer = p->layer;

	/* Convert MP3's arse-over-tit numbering into logical index values */
	if (version == MP3_VERSION_25) version = MP3_VERSION_2;

	/* Sanity check */
	if ( rate < 0 || rate > 15                                   ||
	     !(version == MP3_VERSION_1 ||  version == MP3_VERSION_2) ||
	     !(layer == MP3_LAYER_I || layer == MP3_LAYER_II || layer == MP3_LAYER_III) ) {
	     		/* Illegal input */
	     		return -1;
	}
	return bitrates[rate][3-version][3-layer];
}


int get_sample_freq(const unpacked_frame_header *p) {
	/* Returns the sampling frequency for an MP3 header, or */
	/* -1 if the value is "reserved" */

	const int sample_rates[4][3] = {
		{ 44100, 22050, 11025 },
		{ 48000, 24000, 12000 },
		{ 32000, 16000,  8000 },
		{    -1,    -1,    -1 }
	};
	int index = p->sfreqx, version = p->version;

	if (version == MP3_VERSION_25) version = MP3_VERSION_2;
	if (index < 0 || index > 3 || version < MP3_VERSION_2 || version > MP3_VERSION_1) {
		return -1;
	}
	return sample_rates[index][3-version];
}


unsigned char *next_frame(const unsigned char *p) {
	/* Check validity of mp3 header frame. If it is valid, return a   */
	/* pointer to the start of the next frame. Otherwise return NULL. */
	int frame_length_bytes;
	unpacked_frame_header *mp3fh;

	mp3fh = unpack_header(p);
	if (!mp3fh) return NULL;

	/* Check for reserved values */
	if ( mp3fh->version  == MP3_VERSION_NONE ||
	     mp3fh->layer    == MP3_LAYER_NONE   ||
	     mp3fh->sfreqx   == 3                ||
	     mp3fh->emphasis == 2                   ) return NULL;

	int bit_rate = get_bitrate(mp3fh);
	int sample_freq = get_sample_freq(mp3fh);

	/* Calculation of frame length depends on layer */
	if (mp3fh->layer == MP3_LAYER_I) {
		frame_length_bytes = (12 * bit_rate / sample_freq + mp3fh->padding) * 4;
	}
	else if (mp3fh->layer == MP3_LAYER_II || mp3fh->layer == MP3_LAYER_III) {
		/* For Layer III, versions 2 and 2.5, use a different multiplier */
		if ( (mp3fh->layer == MP3_LAYER_III) &&
		     (mp3fh->version == MP3_VERSION_2 || mp3fh->version == MP3_VERSION_25) ) {
		     	frame_length_bytes = 72 * bit_rate / sample_freq + mp3fh->padding;
		}
		else {
			frame_length_bytes = 144 * bit_rate / sample_freq + mp3fh->padding;
		}
	}
	else {
		fprintf(stderr, "Programmer error\n");
		frame_length_bytes = 0;
	}

	return (unsigned char *)(p + frame_length_bytes);
}



mp3_err load_mp3_file(const char *filename, mp3_file *f) {
	FILE *infile;

	/* Open file and get size */
	infile = fopen(filename, "rb");
	if (!infile) return MP3_FILE_READ_ERROR;
	fseek(infile, 0, SEEK_END);
	f->length = ftell(infile);
	rewind(infile);

	/* Read into memory */
	f->data = malloc(f->length);
	if (!f->data) return MP3_MEMORY_ERROR;
	if (fread(f->data, f->length, 1, infile) != 1) {
		fclose(infile);
		free(f->data);
		f->data = NULL;
		return MP3_FILE_READ_ERROR;
	}
	fclose(infile);

	/* Find first MP3 frame header */
	f->first_frame_hdr = find_first_frame_header(f);
	if (f->first_frame_hdr == NULL) {
		free(f->data);
		f->data = NULL;
		return MP3_NOT_AN_MP3_FILE;
	}
	f->num_private_bits = count_private_bits(f);
	return MP3_SUCCESS;
}


unsigned char *find_first_frame_header(const mp3_file *f) {
    unsigned char *p = f->data;
    unsigned char *endbuf = p + f->length - 4;

	/* Look for the first MP3 frame header. There may be other garbage at */
	/* the start of a file, so to avoid spurious results, find two valid  */
	/* headers in a row and return the first one. */

	/* Are there any ID3 chunks at the start of the file? */
	/* ID3v1 */
	if (f->length > 128 && strncmp((char *)p, "TAG", 3) == 0) {
		/* Extended tag? */
		if (f->length > 128+227 && p[3] == '+' && strncmp((char *)p+227, "TAG", 3) == 0) {
			p += 128+227;
		}
		else {
			p += 128;
		}
	}
	/* ID3v2 */
	else if (f->length > 13 && strncmp((char *)p, "ID3", 3) == 0 &&
	         p[6]<128 && p[7]<128 && p[8]<128 && p[9]<128) {
		p += ((int)p[6] << 21) + ((int)p[7] << 14) + ((int)p[8] << 7) + (int)p[9] + 10;
	}

	/* Crawl forward until we find an acceptable frame header */
	while (p < endbuf) {
		if (unpack_header(p) && next_frame(p)) return p;
		p++;
	}

	return NULL;  /* Not found. Is this really an MP3 file? */
}


int count_private_bits(const mp3_file *f) {
	unsigned char *p = f->first_frame_hdr, *endbuf = p + f->length - 4;
	int numprivatebits = 0;
	unpacked_frame_header *mp3fh;

	while (p != NULL && p < endbuf) {
		mp3fh = unpack_header(p);
		if (mp3fh == NULL) break;
		numprivatebits++;
		p = next_frame(p);
	}
	return numprivatebits;
}


mp3_err embed_private_data(mp3_file *f, const unsigned char *source, int length) {
    unsigned char m = 0x80, *p = f->first_frame_hdr;
    int nb = f->num_private_bits;
    if (length > (nb >> 3)) return MP3_NOT_ENOUGH_ROOM;
    while (length) {
        if (!p) return MP3_INVALID_DATA;
        p[2] &= 0xfe;
        p[2] |= !!(*source & m);
        m >>= 1;
        if (m == 0) {
            m = 0x80;
            source++;
            length--;
        }
        p = next_frame(p);
        nb--;
    }
    while (p && nb) {
        if (!p) return MP3_INVALID_DATA;
        p[2] &= 0xfe;
        p = next_frame(p);
        nb--;
    }
    return MP3_SUCCESS;
}


mp3_err extract_private_data(const mp3_file *f, unsigned char **receiver) {
    unsigned char m = 0, *p = f->first_frame_hdr, c=0;
    int nb = f->num_private_bits;
    unsigned char *outbuf = malloc((nb >> 3) + 1);
    if (!outbuf) return MP3_MEMORY_ERROR;
    *receiver = outbuf;
    while (p) {
        c =  (c << 1) | PRIV_BIT(p);
        m++;
        if (m == 8) {
            *outbuf++ = c;
            c = m = 0;
        }
        p = next_frame(p);
    }
    *outbuf = '\0';
    return MP3_SUCCESS;
}


mp3_err export_mp3_file(const char *filename, const mp3_file *f) {
	FILE *outfile;
	outfile = fopen(filename, "wb");
	if (!outfile) return MP3_FILE_WRITE_ERROR;
	if (fwrite(f->data, f->length, 1, outfile) != 1) {
		fclose(outfile);
		return MP3_FILE_WRITE_ERROR;
	}
	fclose(outfile);
	return MP3_SUCCESS;
}
