// ACME - a crossassembler for producing 6502/65c02/65816 code.
// Copyright (C) 1998-2014 Marco Baye
// Have a look at "acme.c" for further info
//
// Global stuff - things that are needed by several modules
//  4 Oct 2006	Fixed a typo in a comment
// 22 Nov 2007	Added warn_on_indented_labels
//  2 Jun 2014	Added warn_on_old_for and warn_on_type_mismatch
// 19 Nov 2014	Merged Johann Klasek's report listing generator patch
// 23 Nov 2014	Merged Martin Piper's "--msvc" error output patch
#include "global.h"
#include <stdio.h>
#include "platform.h"
#include "acme.h"
#include "cpu.h"
#include "dynabuf.h"
#include "input.h"
#include "macro.h"
#include "output.h"
#include "pseudoopcodes.h"
#include "section.h"
#include "symbol.h"
#include "tree.h"
#include "typesystem.h"


// constants

const char	s_65816[]	= "65816";
const char	s_and[]		= "and";
const char	s_asl[]		= "asl";
const char	s_asr[]		= "asr";
const char	s_bra[]		= "bra";
const char	s_brl[]		= "brl";
const char	s_cbm[]		= "cbm";
const char	s_eor[]		= "eor";
const char	s_error[]	= "error";
const char	s_lsr[]		= "lsr";
const char	s_scrxor[]	= "scrxor";
char		s_untitled[]	= "<untitled>";	// FIXME - this is actually const
const char	s_Zone[]	= "Zone";
const char	s_subzone[]	= "subzone";
const char	s_pet[]		= "pet";
const char	s_raw[]		= "raw";
const char	s_scr[]		= "scr";


// Exception messages during assembly
const char	exception_cannot_open_input_file[] = "Cannot open input file.";
const char	exception_missing_string[]	= "No string given.";
const char	exception_no_left_brace[]	= "Missing '{'.";
const char	exception_no_memory_left[]	= "Out of memory.";
const char	exception_no_right_brace[]	= "Found end-of-file instead of '}'.";
//const char	exception_not_yet[]	= "Sorry, feature not yet implemented.";
const char	exception_number_out_of_range[]	= "Number out of range.";
const char	exception_pc_undefined[]	= "Program counter undefined.";
const char	exception_syntax[]		= "Syntax error.";
// default value for number of errors before exiting
#define MAXERRORS	10

// Flag table:
// This table contains flags for all the 256 possible byte values. The
// assembler reads the table whenever it needs to know whether a byte is
// allowed to be in a label name, for example.
//   Bits	Meaning when set
// 7.......	Byte allowed to start keyword
// .6......	Byte allowed in keyword
// ..5.....	Byte is upper case, can be lowercased by OR-ing this bit(!)
// ...4....	special character for input syntax: 0x00 TAB LF CR SPC : ; }
// ....3...	preceding sequence of '-' characters is anonymous backward
//		label. Currently only set for ')', ',' and CHAR_EOS.
// .....210	currently unused
const char	Byte_flags[256]	= {
/*$00*/	0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,// control characters
	0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*$20*/	0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,// " !"#$%&'"
	0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,// "()*+,-./"
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,// "01234567"
	0x40, 0x40, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00,// "89:;<=>?"
/*$40*/	0x00, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,// "@ABCDEFG"
	0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,// "HIJKLMNO"
	0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0,// "PQRSTUVW"
	0xe0, 0xe0, 0xe0, 0x00, 0x00, 0x00, 0x00, 0xc0,// "XYZ[\]^_"
/*$60*/	0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,// "`abcdefg"
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,// "hijklmno"
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,// "pqrstuvw"
	0xc0, 0xc0, 0xc0, 0x00, 0x00, 0x10, 0x00, 0x00,// "xyz{|}~" BACKSPACE
/*$80*/	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,// umlauts etc. ...
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
/*$a0*/	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
/*$c0*/	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
/*$e0*/	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
	0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
};


// variables
int		pass_count;			// number of current pass (starts 0)
char		GotByte;			// Last byte read (processed)
int		Process_verbosity	= 0;	// Level of additional output
int		warn_on_indented_labels	= TRUE;	// warn if indented label is encountered
int		warn_on_old_for		= TRUE;	// warn if "!for" with old syntax is found
int		warn_on_type_mismatch	= FALSE;	// use type-checking system
// global counters
int		pass_undefined_count;	// "NeedValue" type errors
int		pass_real_errors;	// Errors yet
signed long	max_errors		= MAXERRORS;	// errors before giving up
FILE		*msg_stream		= NULL;	// set to stdout by --use-stdout
int		format_msvc		= FALSE;	// actually bool, enabled by --msvc
struct report 	*report			= NULL;


// memory allocation stuff

// allocate memory and die if not available
void *safe_malloc(size_t size)
{
	void	*block;

	if ((block = malloc(size)) == NULL)
		Throw_serious_error(exception_no_memory_left);
	return block;
}


// Parser stuff

// Parse (re-)definitions of program counter
static void parse_pc_def(void)	// Now GotByte = "*"
{
	NEXTANDSKIPSPACE();	// proceed with next char
	// re-definitions of program counter change segment
	if (GotByte == '=') {
		GetByte();	// proceed with next char
		notreallypo_setpc();
		Input_ensure_EOS();
	} else {
		Throw_error(exception_syntax);
		Input_skip_remainder();
	}
}


// Check and return whether first label of statement. Complain if not.
static int first_label_of_statement(int *statement_flags)
{
	if ((*statement_flags) & SF_IMPLIED_LABEL) {
		Throw_error(exception_syntax);
		Input_skip_remainder();
		return FALSE;
	}
	(*statement_flags) |= SF_IMPLIED_LABEL;	// now there has been one
	return TRUE;
}


// Parse global symbol definition or assembler mnemonic
static void parse_mnemo_or_global_symbol_def(int *statement_flags)
{
	// It is only a label if it isn't a mnemonic
	if ((CPU_state.type->keyword_is_mnemonic(Input_read_keyword()) == FALSE)
	&& first_label_of_statement(statement_flags)) {
		// Now GotByte = illegal char
		// 04 Jun 2005: this fix should help to explain "strange" error messages.
		// 17 May 2014: now it works for UTF-8 as well.
		if ((*GLOBALDYNABUF_CURRENT == (char) 0xa0)
		|| ((GlobalDynaBuf->size >= 2) && (GLOBALDYNABUF_CURRENT[0] == (char) 0xc2) && (GLOBALDYNABUF_CURRENT[1] == (char) 0xa0)))
			Throw_first_pass_warning("Label name starts with a shift-space character.");
		symbol_parse_definition(ZONE_GLOBAL, *statement_flags);
	}
}


// parse local symbol definition
static void parse_local_symbol_def(int *statement_flags)
{
	if (!first_label_of_statement(statement_flags))
		return;
	GetByte();	// start after '.'
	if (Input_read_keyword())
		symbol_parse_definition(Section_now->zone, *statement_flags);
}


// parse anonymous backward label definition. Called with GotByte == '-'
static void parse_backward_anon_def(int *statement_flags)
{
	if (!first_label_of_statement(statement_flags))
		return;
	DYNABUF_CLEAR(GlobalDynaBuf);
	do
		DYNABUF_APPEND(GlobalDynaBuf, '-');
	while (GetByte() == '-');
	DynaBuf_append(GlobalDynaBuf, '\0');
	symbol_set_label(Section_now->zone, *statement_flags, 0, TRUE);	// this "TRUE" is the whole secret
}


// parse anonymous forward label definition. called with GotByte == ?
static void parse_forward_anon_def(int *statement_flags)
{
	if (!first_label_of_statement(statement_flags))
		return;
	DYNABUF_CLEAR(GlobalDynaBuf);
	DynaBuf_append(GlobalDynaBuf, '+');
	while (GotByte == '+') {
		DYNABUF_APPEND(GlobalDynaBuf, '+');
		GetByte();
	}
	symbol_fix_forward_anon_name(TRUE);	// TRUE: increment counter
	DynaBuf_append(GlobalDynaBuf, '\0');
	//printf("[%d, %s]\n", Section_now->zone, GlobalDynaBuf->buffer);
	symbol_set_label(Section_now->zone, *statement_flags, 0, FALSE);
}


// Parse block, beginning with next byte.
// End reason (either CHAR_EOB or CHAR_EOF) can be found in GotByte afterwards
// Has to be re-entrant.
void Parse_until_eob_or_eof(void)
{
	int	statement_flags;

//	// start with next byte, don't care about spaces
//	NEXTANDSKIPSPACE();
	// start with next byte
	GetByte();
	// loop until end of block or end of file
	while ((GotByte != CHAR_EOB) && (GotByte != CHAR_EOF)) {
		// process one statement
		statement_flags = 0;	// no "label = pc" definition yet
		typesystem_force_address_statement(FALSE);
		// Parse until end of statement. Only loops if statement
		// contains "label = pc" definition and something else; or
		// if "!ifdef" is true, or if "!addr" is used without block.
		do {
			switch (GotByte) {
			case CHAR_EOS:	// end of statement
				// Ignore now, act later
				// (stops from being "default")
				break;
			case ' ':	// space
				statement_flags |= SF_FOUND_BLANK;
				/*FALLTHROUGH*/
			case CHAR_SOL:	// start of line
				GetByte();	// skip
				break;
			case '-':
				parse_backward_anon_def(&statement_flags);
				break;
			case '+':
				GetByte();
				if ((GotByte == LOCAL_PREFIX)
				|| (BYTEFLAGS(GotByte) & CONTS_KEYWORD))
					Macro_parse_call();
				else
					parse_forward_anon_def(&statement_flags);
				break;
			case PSEUDO_OPCODE_PREFIX:
				pseudoopcode_parse();
				break;
			case '*':
				parse_pc_def();
				break;
			case LOCAL_PREFIX:
				parse_local_symbol_def(&statement_flags);
				break;
			default:
				if (BYTEFLAGS(GotByte) & STARTS_KEYWORD) {
					parse_mnemo_or_global_symbol_def(&statement_flags);
				} else {
					Throw_error(exception_syntax);
					Input_skip_remainder();
				}
			}
		} while (GotByte != CHAR_EOS);	// until end-of-statement
		vcpu_end_statement();	// adjust program counter
		// go on with next byte
		GetByte();	//NEXTANDSKIPSPACE();
	}
}


// Skip space. If GotByte is CHAR_SOB ('{'), parse block and return TRUE.
// Otherwise (if there is no block), return FALSE.
// Don't forget to call EnsureEOL() afterwards.
int Parse_optional_block(void)
{
	SKIPSPACE();
	if (GotByte != CHAR_SOB)
		return FALSE;
	Parse_until_eob_or_eof();
	if (GotByte != CHAR_EOB)
		Throw_serious_error(exception_no_right_brace);
	GetByte();
	return TRUE;
}


// Error handling

// This function will do the actual output for warnings, errors and serious
// errors. It shows the given message string, as well as the current
// context: file name, line number, source type and source title.
static void throw_message(const char *message, const char *type)
{
	if (format_msvc)
		fprintf(msg_stream, "%s(%d) : %s (%s %s): %s\n",
			Input_now->original_filename, Input_now->line_number,
			type, Section_now->type, Section_now->title, message);
	else
		fprintf(msg_stream, "%s - File %s, line %d (%s %s): %s\n",
			type, Input_now->original_filename, Input_now->line_number,
			Section_now->type, Section_now->title, message);
}


// Output a warning.
// This means the produced code looks as expected. But there has been a
// situation that should be reported to the user, for example ACME may have
// assembled a 16-bit parameter with an 8-bit value.
void Throw_warning(const char *message)
{
	PLATFORM_WARNING(message);
	throw_message(message, "Warning");
}
// Output a warning if in first pass. See above.
void Throw_first_pass_warning(const char *message)
{
	if (pass_count == 0)
		Throw_warning(message);
}


// Output an error.
// This means something went wrong in a way that implies that the output
// almost for sure won't look like expected, for example when there was a
// syntax error. The assembler will try to go on with the assembly though, so
// the user gets to know about more than one of his typos at a time.
void Throw_error(const char *message)
{
	PLATFORM_ERROR(message);
	throw_message(message, "Error");
	++pass_real_errors;
	if (pass_real_errors >= max_errors)
		exit(ACME_finalize(EXIT_FAILURE));
}


// Output a serious error, stopping assembly.
// Serious errors are those that make it impossible to go on with the
// assembly. Example: "!fill" without a parameter - the program counter cannot
// be set correctly in this case, so proceeding would be of no use at all.
void Throw_serious_error(const char *message)
{
	PLATFORM_SERIOUS(message);
	throw_message(message, "Serious error");
	exit(ACME_finalize(EXIT_FAILURE));
}


// Handle bugs
void Bug_found(const char *message, int code)
{
	Throw_warning("Bug in ACME, code follows");
	fprintf(stderr, "(0x%x:)", code);
	Throw_serious_error(message);
}
