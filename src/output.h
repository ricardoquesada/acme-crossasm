// ACME - a crossassembler for producing 6502/65c02/65816 code.
// Copyright (C) 1998-2014 Marco Baye
// Have a look at "acme.c" for further info
//
// Output stuff
#ifndef output_H
#define output_H


#include <stdio.h>
#include "config.h"


// constants
#define MEMINIT_USE_DEFAULT	256
// segment flags
#define	SEGMENT_FLAG_OVERLAY	(1u << 0)	// do not warn about this segment overwriting another one
#define	SEGMENT_FLAG_INVISIBLE	(1u << 1)	// do not warn about other segments overwriting this one


// current CPU state
// FIXME - move struct definition to .c file and change other .c files' accesses to fn calls
struct vcpu {
	const struct cpu_type	*type;		// current CPU type (default 6502)	(FIXME - move out of struct again?)
	struct result		pc;		// current program counter (pseudo value)
	int			add_to_pc;	// add to PC after statement
	int			a_is_long;
	int			xy_are_long;
};


// variables
// FIXME - restrict visibility to .c file
extern struct vcpu	CPU_state;	// current CPU state


// Prototypes

// alloc and init mem buffer (done later)
extern void Output_init(signed long fill_value);
// clear segment list and disable output
extern void Output_passinit(void);
// call this if really calling Output_byte would be a waste of time
extern void Output_fake(int size);
// Send low byte of arg to output buffer and advance pointer
extern void (*Output_byte)(intval_t);
// Output 8-bit value with range check
extern void output_8(intval_t);
// Output 16-bit value with range check
extern void output_le16(intval_t);
// Output 24-bit value with range check
extern void output_le24(intval_t);
// Output 32-bit value (without range check)
extern void output_le32(intval_t);
// define default value for empty memory ("!initmem" pseudo opcode)
// returns zero if ok, nonzero if already set
extern int output_initmem(char content);
// try to set output format held in DynaBuf. Returns zero on success.
extern int output_set_output_format(void);
// if file format was already chosen, returns zero.
// if file format isn't set, chooses CBM and returns 1.
extern int output_prefer_cbm_file_format(void);
// try to set output file name held in DynaBuf. Returns zero on success.
extern int output_set_output_filename(void);
// write smallest-possible part of memory buffer to file
extern void Output_save_file(FILE *fd);
// change output pointer and enable output
extern void Output_start_segment(intval_t address_change, int segment_flags);
// Show start and end of current segment
extern void Output_end_segment(void);

// set program counter to defined value (FIXME - allow undefined!)
extern void vcpu_set_pc(intval_t new_pc, int flags);
// get program counter
extern void vcpu_read_pc(struct result *target);
// get size of current statement (until now) - needed for "!bin" verbose output
extern int vcpu_get_statement_size(void);
// adjust program counter (called at end of each statement)
extern void vcpu_end_statement(void);


#endif
