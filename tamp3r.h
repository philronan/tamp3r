/*

    .................................................................
    '########::::'###::::'##::::'##:'########:::'#######::'########::
    ... ##..::::'## ##::: ###::'###: ##.... ##:'##.... ##: ##.... ##:
    ::: ##:::::'##:. ##:: ####'####: ##:::: ##:..::::: ##: ##:::: ##:
    ::: ##::::'##:::. ##: ## ### ##: ########:::'#######:: ########::
    ::: ##:::: #########: ##. #: ##: ##.....::::...... ##: ##.. ##:::
    ::: ##:::: ##.... ##: ##:.:: ##: ##::::::::'##:::: ##: ##::. ##::
    ::: ##:::: ##:::: ##: ##:::: ##: ##::::::::. #######:: ##:::. ##:
    :::..:::::..:::::..::..:::::..::..::::::::::.......:::..:::::..::

    A command-line utility for tweaking private bits in MP3 files

    Author:  Phil Ronan

    License: MIT (See LICENSE.md for details)

*/


#ifndef STAMP3R_H
#define STAMP3R_H

enum mp3_err_code {
    MP3_SUCCESS = 0,
    MP3_FILE_READ_ERROR,
    MP3_FILE_WRITE_ERROR,
    MP3_MEMORY_ERROR,
    MP3_NOT_AN_MP3_FILE,
    MP3_NOT_ENOUGH_ROOM,
    MP3_INVALID_DATA,
    MP3_NO_INPUT_FILE
};

typedef enum mp3_err_code mp3_err;

typedef struct {
    unsigned char   *data;              /* MP3 file data          */
    size_t          length;             /* File size              */
    unsigned char   *first_frame_hdr;   /* 1st frame header addr  */
    unsigned char   *index;             /* Data access pointer    */
    int             num_private_bits;   /* Non-zero if valid      */
} mp3_file;

/* MP3 frame headers are packed into 4-byte bitfields, but we can */
/* avoid endianness and packing differences between platforms by  */
/* unpacking them into a set of integers.                         */

typedef struct {
   int version;        /* MPEG version: {v2.5, resvd, v2, v1}     */
   int layer;          /* {resvd, III, II, I}                     */
   int protection;     /* {protected by CRC, not protected}       */
   int bitratex;       /* Bitrate index (0-15)                    */
   int sfreqx;         /* Sampling frequency index (0-2; 3=resvd) */
   int padding;        /* One extra slot at end of frame if set   */
   int private;        /* Private bit                             */
   int mode;           /* {stereo, joint stereo, dual ch, mono}   */
   int modext;         /* (if joint stereo)                       */
   int copyright;      /* 1 = copyright                           */
   int original;       /* 1 = original media                      */
   int emphasis;       /* {none, 50/15 ms, resvd, CCIT J.17}      */
   int crc;            /* (if protection==0)                      */
} unpacked_frame_header;

/* MP3 magic numbers */
#define MP3_VERSION_1          3
#define MP3_VERSION_2          2
#define MP3_VERSION_NONE       1
#define MP3_VERSION_25         0
#define MP3_LAYER_I            3
#define MP3_LAYER_II           2
#define MP3_LAYER_III          1
#define MP3_LAYER_NONE         0
#define MP3_MODE_STEREO        0
#define MP3_MODE_JOINT_STEREO  1
#define MP3_MODE_DUAL_CHANNEL  2
#define MP3_MODE_MONO          3

/* Access private bit from MP3 frame */
#define PRIV_BIT(p) (!!(p[2] & 1))

/* Helper functions (probably not much use outside tamp3r.c) */
unpacked_frame_header *unpack_header(const unsigned char *src);
int get_bitrate(const unpacked_frame_header *p);
int get_sample_freq(const unpacked_frame_header *p);
unsigned char *next_frame(const unsigned char *p);
unsigned char *find_first_frame_header(const mp3_file *f);
int count_private_bits(const mp3_file *f);

/* These functions are more useful */
mp3_err load_mp3_file(const char *filename, mp3_file *f);
mp3_err export_mp3_file(const char *filename, const mp3_file *f);
mp3_err extract_private_data(const mp3_file *f, unsigned char **receiver);
mp3_err embed_private_data(mp3_file *f, const unsigned char *source, int length);

#endif
