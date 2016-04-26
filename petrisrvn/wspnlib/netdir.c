/*
 *  $Id: netdir.c 10972 2012-06-19 01:12:22Z greg $
 *
 *  Check for "nets" dir.
 *
 */

#include <../config.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "wspn.h"

extern char * toolname;			/* Name of program invoking me! */

/*
 * Create a net_dir_name  directory
 */

void
make_net_dir( const char * dirname )
{
	struct stat stat_buf;

	if ( stat( dirname, &stat_buf ) == 0 ) {
		if ( !S_ISDIR( stat_buf.st_mode ) ) {
			errno = ENOTDIR;
			(void) fprintf( stderr, "%s: cannot write to directory -- ", toolname );
			perror( dirname );
			exit( 1 );
		} else if ( !(stat_buf.st_mode & S_IWUSR ) ) {
			errno = EACCES;
			(void) fprintf( stderr, "%s: cannot write to directory -- ", toolname );
			perror( dirname );
			exit( 1 );
		}
		return;
	} else if ( errno == ENOENT ) {
		if ( mkdir( dirname, 0755 ) != 0 ) {
			(void) fprintf( stderr, "%s: cannot create directory -- ", toolname );
			perror( dirname );
			exit( 1 );
		}
	} else {
		(void) fprintf( stderr, "%s: stat error -- ", toolname );
		perror( dirname );
		exit( 1 );
	}
}

