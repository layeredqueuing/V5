/* -*- c++ -*-
 * $Id: filename.h 12112 2014-09-12 13:51:38Z greg $
 *
 * MVA solvers: Exact, Bard-Schweitzer, Linearizer and Linearizer2.
 * Abstract superclass does no operation by itself.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 * ------------------------------------------------------------------------
 */

#if	!defined(SRVNIOLIB_FILENAME_H)
#define	SRVNIOLIB_FILENAME_H

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#if defined(__cplusplus)
#include <string>
#include <stdexcept>

/* Autconf botches inline sometimes. */

#if defined(inline)
#undef inline
#endif

using namespace std;

namespace LQIO {

    class Filename
    {
    public:
	Filename() {};
	Filename( const char * base, const char * extension = 0, const char * directory = 0, const char * suffix = 0 );
	Filename( const Filename& );
	Filename& operator=( const Filename& );
	Filename& operator=( const char * );

	const char * operator()() const;
	Filename& operator<<( const char * );
	Filename& operator<<( const unsigned );

	const char * generate( const char * base, const char * extension = 0, const char * directory = 0, const char * suffix = 0 );
	Filename& backup() { Filename::backup( (*this)() ); return *this; }

	int mtimeCmp( const char * fileName ) throw( std::invalid_argument );

	unsigned rfind( const string& s ) const;
	unsigned  find( const string& s ) const;
	Filename& insert( unsigned pos, const char * s );

	static int isRegularFile( const char * fileName );
	static int isRegularFile( int fileno );
	static int isDirectory( const char * fileName );
	static int isWriteableFile( int fileno );
	static void backup( const char * filename );

    private:
	string aString;
    };


    char * make_file_name ( const char * dir_name, const char *file_name, const char *extension);
    int is_regular_file( const char * fileName );
    int is_directory( const char * fileName );
    int is_writeable_file( int fileno );
    int mtime_cmp( const char * src, const char * dst );
    void backup_file( const char * filename );

#if !HAVE_BASENAME
    char *basename(const char *path);
    char *dirname(const char *path);
#endif

};

#endif
#endif
