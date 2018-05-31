# <i>ta</i><b>mp3</b><i>r</i>

### What is it?

A command line utility for reading and writing data to and from the private bits of MP3 frame headers. This makes it an unobtrusive (albeit trivially circumvented) tool for embedding information into MP3 files without affecting the audio quality in any way.

### What are MP3 frame headers?

Apart from a few additional items like `ID3` tags, an MP3 file basically consists of a stream of data blocks called "frames". Each frame typically contains about half a second of audio, and begins with a 32-bit frame header containing information about the bit rate, sampling frequency and so on.

One of these 32 bits is designated as a "private" bit, and has no effect on the audio reproduction. That means it can be used to store other data. Each second of audio typically consists of about 40 MP3 frames, and can thus contain about five bytes of private data.

### Compiling

The code has no dependencies and should compile successfully on any Linux or OS X system. Just type `make` at the command line.

### Running

Once the code has compiled, type in `./tamp3r -h` for instructions. The basic usage is as follows:

    # show the data embedded in audio.mp3
    ./tamp3r -p audio.mp3

    # load input.mp3, write "Hello world" to the private data, and
    # save the marked MP3 data as output.mp3
    ./tamp3r -s "Hello world" -o output.mp3 input.mp3

### Todo

There is plenty of room for improvement, but the first item on the list is to allow embedding of binary data as well as text strings

### Bugs

Yes, probably. Let me know if you find one.

### License

Published under the MIT license. See [LICENSE.md](LICENSE.md) for details.
