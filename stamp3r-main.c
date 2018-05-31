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
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "stamp3r.h"


void usage(const char *prog_name, int verbose) {
    fprintf(stderr,
        "usage: %s [-p] [-s <stego string>] [-o <output MP3 file>] <input MP3 file>\n",
        prog_name);
    if (verbose) {
        fprintf(stderr,
            "\n"
            "This program can store and retrieve data into/from the private bits of MP3\n"
            "frame headers. You might like to use it as a way of applying watermarks to\n"
            "your music files (although it is of course easy to circumvent).\n"
            "\n"
            "Options:\n"
            "  -p  Print out the hidden bit data of the input file\n"
            "  -s  Provide a string to be embedded into the MP3 data\n"
            "  -o  Specify an output filename for the modified MP3\n"
            "  -h  Ignore all other options and print this message instead\n"
            "\n"
            "After the options, provide the name of the source file you want to examine\n"
            "and/or modify.\n");
    }
    else {
        fprintf(stderr,
            "       (or %s -h for more help)\n",
            prog_name);
    }
}

int error(mp3_err e) {
    const char *errmsgs[] = { "No error", "Can'd read MP3 file", "Can't write to MP3 file",
                            "Out of memory", "Not an MP3 file", "Insufficient space in file",
                            "Invalid data in MP3 file", "No input file specified" };
    fprintf(stderr, "Error: %s\n", errmsgs[e]);
    return e;
}

int main(int argc, char *argv[]) {
    int opt, printflag = 0, haveoptions = 0;
    char *stego_string = NULL;
    char *input_file = NULL;
    char *output_file = NULL;
    mp3_file infile;
    mp3_err result;
    unsigned char *s;
    char *me;

    /* Fix argv[0] for cleaner messages from getopt and usage functions */
    me = argv[0] = basename(argv[0]);

    while ((opt = getopt(argc, argv, "ps:o:h")) != -1) {
        haveoptions = 1;
        switch (opt) {
            case 'p':
                printflag = 1;
                break;
            case 's':
                stego_string = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'h':
                usage(me, 1);
                return 0;
            case '?':
            default:
                usage(me, 0);
                return 1;
        }
    }
    argc -= optind;
    argv += optind;
    input_file = argv[0];

    /* If no options were specified and there's no input file, then */
    /* show the usage message */

    if (!haveoptions) {
        usage(me, 0);
        return 1;
    }

    /* Make sure an input file was specified */
    if (!input_file) return error(MP3_NO_INPUT_FILE);

    /* Load the file */
    result = load_mp3_file(input_file, &infile);
    if (result) return error(result);
    printf("File \"%s\" loaded, %d bits available (%d bytes)\n",
        basename(input_file),
        infile.num_private_bits,
        infile.num_private_bits >> 3);

    /* Extract data if requested */
    if (printflag) {
        result = extract_private_data(&infile, &s);
        if (result) return error(result);
        if (*s == '\0') {
            puts("The file contains no hidden data");
        }
        else {
            printf("Hidden data:\n%s\n", s);
        }
    }

    /* Insert data into MP3 frame headers */
    if (stego_string && output_file) {
        result = embed_private_data(&infile, (unsigned char*)stego_string, strlen(stego_string));
        if (result) return error(result);

        result = export_mp3_file(output_file, &infile);
        if (result) return error(result);
    }

    return 0;
}
