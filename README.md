## Tools for analysing PDP-10 ITS files.

- Disassembler for ITS executables.
- Extract files from an ITS archive file.
- Extract files from Alan Snyder's IPAK archives.
- View contents, and make MAGDMP tape images.
- View disk image contents.
- Extract files from a DECtape image in MACDMP format.
- Create a MACDMP image.
- Print the contents of SYSENG; MACRO TAPES and .TAPEn; TAPE nnn files.
- Convert PALX binary to PDP-11 paper tape image.
- Convert CROSS binary to Atari DOS binary.
- Convert files to a RP04 bootable KLDCP disk image.
- Scramble or unscramble an encrypted file.
- Make a picture file suitable for displaying on a Knight TV.
- Write out a core image in some of the supported executable formats.
- Analyze a CONSTANTS area.

A Linux FUSE implementation of the networking filesystem protocol MLDEV
is elsewhere: http://github.com/larsbrinkhoff/mldev

## Tools for other PDP-10 systems.

- List or extract files from a TITO tape (Tymshare TYMCOM-X).
- List, extract, or write files on a DART tape (SAIL WAITS).
- Write files on a DUMPER tape (BBN TENEX, DEC TOPS-20).

## File formats supported.

Most tools support these PDP-10 36-bit word encodings:

- ASCII text, with an additional bit stored in every fifth character.
- Binary image.
- Core dump 9-track tape format.
- DATA8, one 36-bit word stored right alinged per little endian 64-bit word.
- DTA, DECtape image.
- FASL, Maclisp compiled fast load files.
- ITS evacuate format.
- Octal digits.
- Paper tape image.
- Saildart.org UTF-8.
- Tape images, 7 or 9 track.

Most tools that work with executable programs support these formats:

- ITS PDUMP and SBLK.
- Muddle/MDL save files.
- Raw files.
- Read-in mode.
- Stanford WAITS .DMP files.
- TOPS-20 and TOPS-10 sharable save .EXE files.

In addition, some mini and micro computer program formats are supported:

- PDP-11 PALX binaries.
- PDP-11 absolute loader tapes.
- CROSS binaries.
- CROSS "ASCII HEX" files.
- Atari DOS executables.
- Imlac "speciall TTY" files.
