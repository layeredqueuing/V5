/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 1991.								*/
/* December 2008.							*/
/************************************************************************/

/*
 * $Id: symtbl.cc 11963 2014-04-10 14:36:42Z greg $
 *
 * The symbol table and related functions are kept in this file.  The
 * symbol table is comprised of an array of records where each record
 * contains fields for the symbol itself, as well as other data assciated
 * with the symbol.
 *
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <set>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "symtbl.h"

using namespace std;

class st_entry 
{
public:
    st_entry( const char * sym, const int pos ) :  _symbol(sym), _position(pos) {}
    bool operator<( const st_entry& s2 ) const { return _symbol < s2._symbol; }
    bool operator==( const char * s2 ) const { return _symbol == s2; }

    const char * name() const { return _symbol.c_str(); }
    unsigned int position() const { return _position; }

private:
    const string _symbol;
    const int _position;
};

/*
 * Compare two sets by their name.  Used by the set class to insert items
 */

struct ltSymbol
{
    bool operator()(const st_entry * s1, const st_entry * s2) const { return *s1 < *s2; }
};


/*
 * Compare a symbol name to a string.  Used by the find_if (and other algorithm type things.
 */

struct eqSymbol
{
    eqSymbol( const char * s ) : _s(s) {}
    bool operator()(const st_entry * s1 ) const { return *s1 == _s; }

private:
    const char * _s;
};


static set<st_entry *,ltSymbol> symtbl[ST_SYMTBL_TYPES];	/* Primary table.	*/
static vector<st_entry *> postbl[ST_SYMTBL_TYPES];		/* Auxiliary index 	*/

enum {
    FTL_NO_MEMORY,
    ERR_SYMBOL_TABLE_FULL,
    ERR_DUPLICATE_ENTRY
};

static const char * local_error_messages[] = {
    "(fatal error) No more memory.",
    "Symbol table full.",
    "Name \"%s\" previously defined."
};


/*
 * Initialize any global data that may need it.
 */

void 
init_symtbl (void)
{
    for ( unsigned i = 0; i < ST_SYMTBL_TYPES; i++) {
	symtbl[i].clear();
	postbl[i].clear();
	postbl[i].push_back(0);		/* Nothing in slot 0 of this table */
    }
}


/*
 * Given the symbol string, "sym"add_symbol() places a symbol entry into
 * the symbol table and returns a unique positive integer.  Errors will
 * call input_error() and return 0.
 */

unsigned 
add_symbol (
    const char *sym,				/* The name of the symbol.	*/
    const st_etypes type			/* The type of the symbol.	*/
)
{
    if ( !sym ) {
	results_error( local_error_messages[(unsigned)FTL_NO_MEMORY] );
	return 0;
    } else if ( static_cast<unsigned>(type) >= ST_SYMTBL_TYPES) {
	abort();			/* Should not get this one.	*/
    }

    if ( find_if( symtbl[type].begin(), symtbl[type].end(), eqSymbol( sym ) ) != symtbl[type].end() ) {
	results_error( local_error_messages[(unsigned)ERR_DUPLICATE_ENTRY], sym );
	return 0;
    } else {
	st_entry * aSymbol = new st_entry( sym, symtbl[type].size() + 1 );
	symtbl[type].insert( aSymbol );
	postbl[type].push_back( aSymbol );
	return aSymbol->position();
    }
}


/*
 * This function finds the position of the symbol having the name
 * "name".  located anywhere in the symbol table.  If the symbol
 * desired cannot be found, "0" is returned.
 */

unsigned 
find_symbol_name ( const char *name, const st_etypes type)
{
    if ( static_cast<unsigned>(type) >= ST_SYMTBL_TYPES) {
	abort();
    }
    
    set<st_entry *,ltSymbol>::const_iterator nextSymbol = find_if( symtbl[type].begin(), symtbl[type].end(), eqSymbol( name ) );

    if ( nextSymbol != symtbl[type].end() ) {
	return (*nextSymbol)->position();
    } else {
	return 0;
    }
}


/*
 * This function finds the name of the symbol having the type "type" and
 * the position "pos".  The auxiliary index is used.
 */

const char *
find_symbol_pos ( const unsigned pos, const st_etypes type)
{
    if ( static_cast<unsigned>(type) >= ST_SYMTBL_TYPES) {
	abort();
    } else if ( pos == 0 || postbl[static_cast<unsigned>(type)].size() < pos ) {
	abort();	/* slot zero is reserverd and table size is always one bigger than number of entries */
    } 
    return postbl[static_cast<unsigned>(type)][pos]->name();
}
	

/*
 * This function cleans up after the symbol table is no longer needed.
 * Any dynamic memory is returned from whence is came.
 */

void 
erase_symtbl (void)
{
    for ( unsigned i = 0; i < ST_SYMTBL_TYPES; i++) {
	for ( set<st_entry *,ltSymbol>::const_iterator nextEntry = symtbl[i].begin(); nextEntry != symtbl[i].end(); ++nextEntry ) {
	    const st_entry * aSymbol = *nextEntry;
	    delete aSymbol;
	}
    }
}


/*
 * This function dumps the contents of the symbol table to the
 * standard output device.
 */

void 
dump_symtbl (void)
{
    for (unsigned int i = 0; i < ST_SYMTBL_TYPES; i++) {
	(void) printf("Symbol table for type %d\n", i);
	(void) printf("Name\t\t\t\tType\tPos\n");
	for ( set<st_entry *,ltSymbol>::const_iterator nextEntry = symtbl[i].begin(); nextEntry != symtbl[i].end(); ++nextEntry ) {
	    const st_entry * aSymbol = *nextEntry;
	    (void) printf("%30s\t%d\n", aSymbol->name(), aSymbol->position());
	}
    }
}


/*
 * Return the number of entries in a given symbol table.
 */

unsigned 
num_entries (
    const st_etypes type			/* The type of the symbol.	*/
)
{
    return symtbl[static_cast<unsigned>(type)].size();
}
