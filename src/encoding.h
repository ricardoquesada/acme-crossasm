// ACME - a crossassembler for producing 6502/65c02/65816 code.
// Copyright (C) 1998-2014 Marco Baye
// Have a look at "acme.c" for further info
//
// Character encoding stuff
#ifndef encoding_H
#define encoding_H


//struct encoder;
extern const struct encoder	*encoder_current;	// gets set before each pass
extern const struct encoder	encoder_raw;
extern const struct encoder	encoder_pet;
extern const struct encoder	encoder_scr;
extern const struct encoder	encoder_file;
extern char			*encoding_loaded_table;	// ...loaded from file


// prototypes

// convert character using current encoding
extern char encoding_encode_char(char byte);
// set "raw" as default encoding
extern void encoding_passinit(void);
// try to load encoding table from given file
extern void encoding_load(char target[256], const char *filename);
// lookup encoder held in DynaBuf and return its struct pointer (or NULL on failure)
extern const struct encoder *encoding_find(void);


#endif
