This package contains some basic tools needed to create working games. They are
backwards compatible with O/NScripter (except for recoder). I may decide later
on to add incompatible features, such as compression.


                                    crypt

crypt can be used to decrypt O/NScripter and ONSlaught script files. It uses
XOR-based encryption, which is symmetric. This means that the same code is used
for both encryption and decryption.
The original tool, nscmake, was crap. A 450 MHz took over twenty minutes to
process a 4 MiB script. crypt takes less than two seconds.
Run it with "crypt --help" for details on how to use it. xor84 encryption
works fine for most cases.


                                    recoder

A simple encoding conversion tool. It can convert between the following
encodings: Shift JIS, UTF-8, UCS-2, and ISO 8859-1.
Run it with "recoder --help" for details on how to use it.
Notes on encodings:
UTF-8 conversion is limited to the range [U+0000;U+FFFF]. Code points above that
are converted to '?'. UTF-8 will work fine for most cases.
UCS-2 by definition can only encode [U+0000;U+FFFF]. UCS-2 has a slightly better
bytes-per-character ratio than UTF-8 for asian languages, so if your script
contains a lot of such characters, you might want to use it instead of UTF-8
ISO 8859-1 is simply the first 256 code points of Unicode encoded into a single
byte. It's slighly better data compactness-wise than UTF-8 for Western European
languages.
Shift JIS: An obsolete Japanese code page O/NScripter uses, only included here
for decoding purposes. It should not be used for new scripts.


The tools below have the following requirements:
Boost 1.35 or above.
bz2lib (UNIX only)

                                    nsaio

Compared to the other two tools, rather complex. It can extract and list both
SAR and NSA archives, and create NSA archives. At the moment (2009-11-18) it
supports LZSS and NBZ (bzip2) de/compression. SPB is only supported for
decompression. If you find source code or an example of SPB decompression or you
stumble upon the sources of nsaarc.exe (a feat I have yet to accomplish) please
send them to me (helios.vmg@gmail.com).

Usage:
nsaio <mode> <options> <input files>

MODES:
    e - Extract
    c - Create
    l - List

OPTIONS:
    -h, -?, --help
        Print a short description of the arguments
    --version
        Print the version
    -o <filename> (default: output)
        Use for mode c. Gives the created archive that name. Do not include the
        extension in the name.
    -c {none|lzss|bz2|auto|automt} (default: automt)
        Selects the compression algorithm to use.
        none: Do not compress any file.
        lzss: Apply LZSS (Lempel-Ziv-Storer-Szymanski) to all files.
        bz2: Apply BZ2 to all files.
        auto: Find the algorithm that will yield the best compression for each
            file. Takes longer and uses more memory, but gives the best possible
            results.
        automt: Same as auto, but uses multi-threading to speed up the process.
            On systems with two or more cores, it can run almost twice as fast
            as auto.
    -r <directory>
        Instead of adding the directory to the root of the archive, add its
        contents.
        Example:
        For this directory structure
            ./arc/0.bmp
            ./arc/icon/
            ./arc/image/
            ./arc/wave/
        this command line
            nsaio arc
        gives this archive
            output.nsa/arc/0.bmp
            output.nsa/arc/icon/
            output.nsa/arc/image/
            output.nsa/arc/wave/
        but this one
            nsaio -r arc
        gives this archive
            output.nsa/0.bmp
            output.nsa/icon/
            output.nsa/image/
            output.nsa/wave/


                                      zip

It has basically the same interface as nsaio, but instead it just creates ZIP
files. It supports DEFLATE (the most common ZIP compression algorithm), BZ2,
and LZMA compression, and can generate split archives. An archive will be
forcibly split if it needs 2 GiB or more.

ZIP tools and features:
            Split archive      DEFLATE      BZ2      LZMA
WinZip           Yes           Yes          Yes      Yes
WinRAR            ?            Yes          No**     No**
7-Zip            No*           Yes          Yes      Yes
GNU zip          No*            ?           No       No

(*)These don't support true split ZIP archives. They implement splitting by
generating a single ZIP archive axed into parts of varying size. The engine
doesn't support archives of this type.
(**)WinRAR doesn't support compression with these algorithms; it only uses
DEFLATE for compression. Support for decompression is unknown.

Usage:
zip <options> <input files>

OPTIONS:
    -h, -?, --help
        Print a short description of the arguments
    --version
        Print the version
    -o <filename> (default: output)
        Gives the created archive that name. Do not include the extension in the
        name.
    -c {none|bz2|lzma|auto|automt} (default: automt)
        Selects the compression algorithm to use.
        none: Do not compress any file.
        bz2: Apply BZ2 to all files.
        lzma: Apply LZMA (Lempel-Ziv-Markov chain-Algorithm) to all files.
        auto: Find the algorithm that will yield the best compression for each
            file. Takes longer and uses more memory, but gives the best possible
            results.
        automt: Same as auto, but uses multi-threading to speed up the process.
            On systems with two or more cores, it can run almost twice as fast
            as auto.
    -r <directory>
        See nsaio.
    -s <file size> (default [and max]: 2147483647)
        Generates a split archive with parts no bigger than the size passed.
        The size should be in bytes, but the following multipliers are
        supported: K (or k), M (or m), and G (or g).
        1K == 1024
        1M == 1024K
        1G == 1024M
