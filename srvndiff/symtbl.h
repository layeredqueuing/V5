/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 1991.								*/
/* December 2008.							*/
/************************************************************************/

/*
 * $Id: symtbl.h 10328 2011-06-13 17:32:52Z greg $
 */

#if	!defined(SYMTBL_H)
#define	SYMTBL_H

#if	defined(__cplusplus)
extern "C" {
#endif

/*----------------------------------------------------------------------*/
/*		 Symbol Table Definitions and Variables			*/
/*----------------------------------------------------------------------*/


#define ST_SYMTBL_TYPES 5	/* Number of unique types of symbols: st_stypes */

typedef enum {
	ST_PROCESSOR,
	ST_GROUP,
	ST_TASK,
	ST_ENTRY,
	ST_ACTIVITY,
} st_etypes;

enum st_stypes {
	ST_NAME,
	ST_POS
};


/* symtbl.c */
extern void init_symtbl (void);
extern unsigned add_symbol (const char *sym, const st_etypes type);
extern unsigned find_symbol_name (const char *name, const st_etypes type);
extern const char *find_symbol_pos (const unsigned pos, const st_etypes type);
extern const char *find_symbol_pos_str (const unsigned pos, const st_etypes type);
extern void erase_symtbl (void);
extern unsigned num_entries (const st_etypes type);
extern void dump_symtbl(void);
extern void results_error( const char * fmt, ... );

#if	defined(__cplusplus)
}
#endif
#endif
