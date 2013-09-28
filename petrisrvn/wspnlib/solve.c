/*
 *  $Id$
 *
 * Solve the petri net "net_name".  The actual work is performed by a subprocess.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include "global.h"
#include "wspn.h"

char * toolname = "wspn";
static char pathname[MAXPATHLEN];

void
solve( const char *net_name )
{
	(void) solve2( net_name, 2, SOLVE_STEADY_STATE );
	collect_res( TRUE, toolname );
}


int
solve2( const char * net_name, int output_fd, solution_type solver, ... )
{
	char command[256];
	char arg1[32];
	int status;
	int retval;
	int child;
	int maxfds = sysconf( _SC_OPEN_MAX );
	unsigned i;
	va_list args;
	int nullfd;
	char path_name[MAXPATHLEN];

	va_start( args, solver );
	
	if ( !remove_result_files( net_name ) ) return 0;
	
	/*
	 * Now solve the net.
	 */
	
	child = fork();
	
	switch( child ) {
	case -1:
		va_end( args );
		(void) fprintf( stderr, "%s: cannot fork -- ", toolname );
		perror( (char *)0 );
		return 0;
 
	default:
		va_end( args );

		do {
			retval = wait( &status );
		} while ( retval != -1 && retval != child );

		if ( retval == -1 ) {
			(void) fprintf( stderr, "%s: wait error -- ", toolname );
			perror( (char *)0 );
			return 0;
		} else if ( status & 0x003f ) {
			(void) fprintf( stderr, "%s: petri-net solution failed due to signal %d\n", toolname, status & 0x7f );
			return 0;
		} else if ( status & 0xff00 ) {
			(void) fprintf( stderr, "%s: petri-net solution failed, exit value %d\n", toolname, status >> 8 & 0xff );
			return 0;
		} else {		/* KaPlah! */
			return 1;
		}
		break;

	case 0:
		if ( output_fd > 2 ) {
			dup2( output_fd, 1 );		/* Catch stderr.	*/
			dup2( output_fd, 2 );		/* Catch stderr.	*/
		}
		nullfd = open( "/dev/null", O_RDONLY );
		if ( !nullfd ) {
			(void) fprintf( stderr, "Cannot open " );
			perror( "/dev/null" );
			exit ( 1 );
			
		}
		dup2( nullfd, 0 );			/* No need for input.	*/

		for ( i = 3; i < maxfds; ++i ) {
			close( i );
		}
			
		/* See gspn_gs_proc( m, mi ) in "greatspn1.5/greatsrc1.5/command.c" */
		
		(void) sprintf( path_name, "nets/%s", net_name );
		
		switch ( solver ) {
		case SOLVE_STEADY_STATE:
			(void) sprintf( command, "%s", "newSO" );
			execl( "/bin/bash", "bash", command, path_name, "-s10", (char *)0 );
			break;
			
		case SOLVE_TRANSIENT:
			(void) sprintf( command, "%s", "newTR" );
			(void) sprintf( arg1, "%d", va_arg( args, int ) );
			execl( "/bin/bash", "bash", command, path_name, arg1, (char *)0 );
			break;
			
		default:
			(void) fprintf( stderr, "%s: Invalid solver - code %d", toolname,
				       (int)solver );
			exit( 1 );
		}
		
		(void) fprintf( stderr, "%s: Cannot exec ", toolname );
		perror( command );
		exit( 1 );

	}
	return 0;
}



/*
 * Remove all result files for net_name.
 */

int
remove_result_files( const char * net_name )
{
	char path_name[256];
	int status;
	int retval;
	int child;
	int maxfds = sysconf( _SC_OPEN_MAX );
	unsigned i;
	char * home_dir = getenv( "HOME" );
	if ( !home_dir ) {
	    home_dir = ".";
	}

	if ( !getcwd( pathname, MAXPATHLEN ) ) {
		(void) fprintf( stderr, "%s: cwd = '%s'\n", toolname, pathname );
		(void) strcpy( pathname, "." );	/* Punt on error */
	}

	/* First, get rid of all result files from previous run. */
	
	child = fork();
	
	switch( child ) {
	case -1:
		(void) fprintf( stderr, "%s: cannot fork -- ", toolname );
		perror( (char *)0 );
		return 0;
 
	default:
		do {
			retval = wait( &status );
		} while ( retval != child && ( retval != -1 || errno == EINTR ) );

		if ( retval == -1 ) {
			(void) fprintf( stderr, "%s: wait error -- ", toolname );
			perror( (char *)0 );
			return 0;
		} else if ( status & 0x003f ) {
			(void) fprintf( stderr, "%s: petri-net cleanup failed due to signal %d\n", toolname, status & 0x7f );
			return 0;
		} else if ( status & 0xff00 ) {
			(void) fprintf( stderr, "%s: petri-net cleanup failed, exit value %d\n", toolname, status >> 8 & 0xff );
			return 0;
		}
		break;

	case 0:
		for ( i = 3; i < maxfds; ++i ) {	/* close all non-essential descriptors */
			close( i );
		}
		close( 0 );				/* No need for input either. */

		/* See save_proc( item, event ) in "greatspn1.5/greatsrc1.5/command.c" */
		
		sprintf( path_name, "nets/%s", net_name );
		execl( "/bin/bash", "bash", "--norc", "RMNET", path_name, (char *)0 );
		(void) fprintf( stderr, "%s: Cannot exec %s %s", toolname, "RMNET", path_name );
		perror( "" );
		exit( 1 );

	}
	return 1;
}
