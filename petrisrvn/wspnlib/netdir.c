/*
 *  $Id: netdir.c 14924 2021-07-19 20:19:58Z greg $
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
	        int rc = 0;
#if defined(__WINNT__)
	        rc = mkdir( dirname );
#else
		rc = mkdir( dirname, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IWOTH|S_IROTH|S_IXOTH );
#endif
		if ( rc != 0 ) {
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

