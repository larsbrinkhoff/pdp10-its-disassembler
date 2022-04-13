# LIBWORD

A library for reading and writing binary words.

### API

**Basic word input/output.**

- `word_t`  
   Type for storing a word.

- `word_t read_word (FILE *file);`  
   Read one `word` from the `file`.

- `void write_word (FILE *file, word_t word);`  
   Write one `word` to the `file`.

- `void rewind_word (FILE *file);`  
   Rewind the `file` to the beginning.

- `void seek_word (FILE *file, long long position);`  
   Seek to word number `position` in the `file`.

- `void flush_word (FILE *file);`  
   Prepare output `file` to be closed.

**Selecting a word format.**

- `word usage_word_format (void);`
   Print a summary of valid word formats.

- `int parse_input_word_format (const char *string);`
   Parse a `string` and set the input word format.
   Return 0 on success, or -1 on error.

- `int parse_output_word_format (const char *string);`
   Parse a `string` and set the output word format.
   Return 0 on success, or -1 on error.

Word formats are:

- `ascii` - PDP-10 ANSI ASCII.
- `alto` - PDP-10 words stored in an Alto file system.
- `bin` - PDP-10 densely packed words.
- `cadr` - 32-bit CADR Lisp machine words stored left aligned in 36-bit words.
- `core` - PDP-10 core dump.
- `data8` - PDP-10 words stored little-endian in eight octets.
- `dta` - 36-bit DECtape image.
- `its` - ITS evacuate format.
- `oct` - 36-bit octal numbers as ASCII text.
- `pt` - 36-bit words in a paper tape image.
- `sail` - Saildart.org text files.
- `tape` - SIMH 9-track tape image.
- `tape7` - SIMH 7-track tape image.

**Tape structured data.**

A `word_t` can have the bits `START_TAPE`, `START_FILE`, or
`START_RECORD` set.  On input, this means a structure boundary was
encountered before the word.  On output, mark such a boundary before
the word.

More complicated media events are supported by a hook which receives
a SIMH tape image code with the most significant bit set.

- `void (*tape_hook) (int code);`
