/* dumpemf.c	-- Greg Franks Thu Dec  2 2004
 *
 * $Id: dumpemf.c 15423 2022-02-03 02:10:02Z greg $
 */

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    NOP,
    HEADER,
    POLYBEZIER,
    POLYGON,
    POLYLINE,
    POLYBEZIERTO,
    POLYLINETO,
    POLYPOLYLINE,
    POLYPOLYGON,
    SETWINDOWEXTEX,
    SETWINDOWORGEX,
    SETVIEWPORTEXTEX,
    SETVIEWPORTORGEX,
    SETBRUSHORGEX,
    EMR_EOF,
    SETPIXELV,
    SETMAPPERFLAGS,
    SETMAPMODE,
    SETBKMODE,
    SETPOLYFILLMODE,
    SETROP2,
    SETSTRETCHBLTMODE,
    SETTEXTALIGN,
    SETCOLORADJUSTMENT,
    SETTEXTCOLOR,
    SETBKCOLOR,
    OFFSETCLIPRGN,
    MOVETOEX,
    SETMETARGN,
    EXCLUDECLIPRECT,
    INTERSECTCLIPRECT,
    SCALEVIEWPORTEXTEX,
    SCALEWINDOWEXTEX,
    SAVEDC,
    RESTOREDC,
    SETWORLDTRANSFORM,
    MODIFYWORLDTRANSFORM,
    SELECTOBJECT,
    CREATEPEN,
    CREATEBRUSHINDIRECT,
    DELETEOBJECT,
    ANGLEARC,
    ELLIPSE,
    RECTANGLE,
    ROUNDRECT,
    ARC,
    CHORD,
    PIE,
    SELECTPALETTE,
    CREATEPALETTE,
    SETPALETTEENTRIES,
    RESIZEPALETTE,
    REALIZEPALETTE,
    EXTFLOODFILL,
    LINETO,
    ARCTO,
    POLYDRAW,
    SETARCDIRECTION,
    SETMITERLIMIT,
    BEGINPATH,
    ENDPATH,
    CLOSEFIGURE,
    FILLPATH,
    STROKEANDFILLPATH,
    STROKEPATH,
    FLATTENPATH,
    WIDENPATH,
    SELECTCLIPPATH,
    ABORTPATH,
    UNUSED1,
    GDICOMMENT,
    FILLRGN,
    FRAMERGN,
    INVERTRGN,
    PAINTRGN,
    EXTSELECTCLIPRGN,
    BITBLT,
    STRETCHBLT,
    MASKBLT,
    PLGBLT,
    SETDIBITSTODEVICE,
    STRETCHDIBITS,
    EXTCREATEFONTINDIRECTW,
    EXTTEXTOUTA,
    EXTTEXTOUTW,
    POLYBEZIER16,
    POLYGON16,
    POLYLINE16,
    POLYBEZIERTO16,
    POLYLINETO16,
    POLYPOLYLINE16,
    POLYPOLYGON16,
    POLYDRAW16,
    CREATEMONOBRUSH,
    CREATEDIBPATTERNBRUSHPT,
    EXTCREATEPEN,
    POLYTEXTOUTA,
    POLYTEXTOUTW
} RECORD_ID;    

char * name[] = {
    0,
    "HEADER",
    "POLYBEZIER",
    "POLYGON",
    "POLYLINE",
    "POLYBEZIERTO",
    "POLYLINETO",
    "POLYPOLYLINE",
    "POLYPOLYGON",
    "SETWINDOWEXTEX",
    "SETWINDOWORGEX",
    "SETVIEWPORTEXTEX",
    "SETVIEWPORTORGEX",
    "SETBRUSHORGEX",
    "EOF",
    "SETPIXELV",
    "SETMAPPERFLAGS",
    "SETMAPMODE",
    "SETBKMODE",
    "SETPOLYFILLMODE",
    "SETROP2",
    "SETSTRETCHBLTMODE",
    "SETTEXTALIGN",
    "SETCOLORADJUSTMENT",
    "SETTEXTCOLOR",
    "SETBKCOLOR",
    "OFFSETCLIPRGN",
    "MOVETOEX",
    "SETMETARGN",
    "EXCLUDECLIPRECT",
    "INTERSECTCLIPRECT",
    "SCALEVIEWPORTEXTEX",
    "SCALEWINDOWEXTEX",
    "SAVEDC",
    "RESTOREDC",
    "SETWORLDTRANSFORM",
    "MODIFYWORLDTRANSFORM",
    "SELECTOBJECT",
    "CREATEPEN",
    "CREATEBRUSHINDIRECT",
    "DELETEOBJECT",
    "ANGLEARC",
    "ELLIPSE",
    "RECTANGLE",
    "ROUNDRECT",
    "ARC",
    "CHORD",
    "PIE",
    "SELECTPALETTE",
    "CREATEPALETTE",
    "SETPALETTEENTRIES",
    "RESIZEPALETTE",
    "REALIZEPALETTE",
    "EXTFLOODFILL",
    "LINETO",
    "ARCTO",
    "POLYDRAW",
    "SETARCDIRECTION",
    "SETMITERLIMIT",
    "BEGINPATH",
    "ENDPATH",
    "CLOSEFIGURE",
    "FILLPATH",
    "STROKEANDFILLPATH",
    "STROKEPATH",
    "FLATTENPATH",
    "WIDENPATH",
    "SELECTCLIPPATH",
    "ABORTPATH",
    "",
    "GDICOMMENT",
    "FILLRGN",
    "FRAMERGN",
    "INVERTRGN",
    "PAINTRGN",
    "EXTSELECTCLIPRGN",
    "BITBLT",
    "STRETCHBLT",
    "MASKBLT",
    "PLGBLT",
    "SETDIBITSTODEVICE",
    "STRETCHDIBITS",
    "EXTCREATEFONTINDIRECTW",
    "EXTTEXTOUTA",
    "EXTTEXTOUTW",
    "POLYBEZIER16",
    "POLYGON16",
    "POLYLINE16",
    "POLYBEZIERTO16",
    "POLYLINETO16",
    "POLYPOLYLINE16",
    "POLYPOLYGON16",
    "POLYDRAW16",
    "CREATEMONOBRUSH",
    "CREATEDIBPATTERNBRUSHPT",
    "EXTCREATEPEN",
    "POLYTEXTOUTA",
    "POLYTEXTOUTW"
};

unsigned long
to_long( unsigned char * buf )
{
    return ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]) & 0xffffffff;
}

unsigned long
to_word( unsigned char * buf )
{
    return (buf[1] << 8) | buf[0];
}

int
main( int argc, char ** argv )
{
    size_t n;
    unsigned char buf[BUFSIZ];
    FILE * input = stdin;

    if ( argc == 2 ) {
	input = fopen( argv[1], "r" );
	if ( !input ) {
	    fprintf( stderr, "Cannot open %s: ", argv[1] );
	    perror( "" );
	    exit( 1 );
	}
    } else if ( argc > 2 ) {
	fprintf( stderr, "Arg count.\n" );
	exit( 1 );
    }
    
    while ( (n = fread( buf, sizeof(char), 8, input )) > 0 ) {
	size_t i = to_long( &buf[4] );
	int code = to_long( &buf[0] );
	if ( 8 <= i && i < BUFSIZ ) {
	    if ( 0 <= code && code < sizeof( name ) ) {
		printf( "%-24s", name[code] );
	    } else {
		printf( "%08x %08lx", code, i );
	    }
	    if ( i > 8 ) {
		n = fread( &buf[8], sizeof(char), i-8, input );
		if ( n == i-8 ) {
		    int j;
		    long k;
		    long l;
		    switch ( code ) {
		    case HEADER:
			printf( "Records: %lu, File Size %lu\n", to_long( &buf[52] ), to_long( &buf[48] ) );
			printf( "\t\t\tBounds: %lu %lu %lu %lu, Frame: %lu %lu %lu %lu, Ref Dev: %lu %lu, MM Dev: %lu %lu\n",
				to_long( &buf[8] ), to_long( &buf[12] ), to_long( &buf[16] ), to_long( &buf[20] ),
				to_long( &buf[24] ), to_long( &buf[28] ), to_long( &buf[32] ), to_long( &buf[36] ),
				to_long( &buf[72] ), to_long( &buf[76] ), to_long( &buf[80] ), to_long( &buf[84] ) );
			k = to_long( &buf[60] );
			l = to_long( &buf[64] );
			printf( "\t\t\tDescription: %ld, String: ", k );
			for ( j = 0; j < k; ++j ) {
			    if ( buf[100+j*2] ) {
				putchar( buf[100+j*2] );
			    } else {
				printf( "<NULL>" );
			    }
			}
			break;

		    case SELECTOBJECT:
		    case DELETEOBJECT:
			printf( "Handle: %08lx", to_long( &buf[8] ) );
			break;

		    case POLYGON:
		    case POLYLINE:
			printf( "rect: %lu,%lu,%lu,%lu, n=%lu ",
				to_long( &buf[8] ), to_long( &buf[12] ), to_long( &buf[16] ), to_long( &buf[20] ),
				to_long( &buf[24] ));
			for ( j = 28; j < i; j += 8 ) {
			    printf( "(%lu, %lu) ", to_long( &buf[j] ), to_long( &buf[j+4] ) );
			}
			break;

		    case SETWINDOWEXTEX:
		    case SETWINDOWORGEX:
		    case SETVIEWPORTEXTEX:
		    case SETVIEWPORTORGEX:
		    case MOVETOEX:
		    case LINETO:
			printf( "(%lu,%lu)", to_long( &buf[8] ), to_long( &buf[12] ) );
			break;

		    case EXTTEXTOUTW:
			l = to_long( &buf[44] );
			k = to_long( &buf[48] );
			printf( "Len: %ld, Offset %ld, ", l, k );
			for ( j = 0; j < l; ++j ) {
			    printf( "%c", buf[k+j*2] );
			}
			break;
			
		    case CREATEPEN:
			printf( "Handle: %lu, Type: %lu, Width: %lu, ?? %lu, Colour: %06lx",
				to_long( &buf[8] ), to_long( &buf[12] ), to_long( &buf[16] ),
				to_long( &buf[20] ), to_long( &buf[24] ) );
			break;
			    
		    case CREATEBRUSHINDIRECT:
			printf( "Handle: %lu, Type: %lu, Colour: %06lx, Hatch: %lu",
				to_long( &buf[8] ), to_long( &buf[12] ), to_long( &buf[16] ),
				to_long( &buf[20] ) );
			break;
			
		    case EXTCREATEFONTINDIRECTW:
			printf( "Handle %lu: Size: %lu Font: ", to_long( &buf[8] ), to_long( &buf[12] ) );
			for ( j = 72; j < 104 && buf[j]; ++j ) {
			    printf( "%c", buf[j] );
			}
			break;
		    default:
			for ( j = 8; j < i; j += 2 ) {
			    printf( " %02x%02x", buf[j+1], buf[j] );
			}
		    }
		} else {
		    fprintf( stderr, "Bogus record!  Bad fread.\n" );
		}
	    }
	    printf( "\n" );
	} else {
	    fprintf( stderr, "Bogus record!  Bad size %lu\n", i );
	    break;
	}
    }
}
