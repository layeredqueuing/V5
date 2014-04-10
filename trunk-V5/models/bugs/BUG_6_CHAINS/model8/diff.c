#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#define MAX_ARGS	5
#define EXACT_RESULTS	0

#define LAMBDA_c0	0
#define	LAMBDA_c1	1
#define UTIL_p0		2
#define	N_RESULTS	3

#define TEXT_FORMAT	"  %8.2f %8.2f %8.2f "
#define LATEX_FORMAT	" & %6.2f & %6.2f & %6.2f "

typedef struct moment {
    double n;
    double mean;
    double M2;			/* Variance	*/
    double M3;			/* Skew		*/
    double M4;			/* Kurtosis	*/
    double min;
    double max;
} moment;

struct option longopts[] =
    /* name */ /* has arg */ /*flag */ /* val */
{
    { "no-headers", no_argument,        0, 'h' },
    { "no-results", no_argument,        0, 'r' },
    { "no-summary", no_argument,        0, 's' },
    { "infinity",   required_argument,	0, 'i' },
    { "latex",	    no_argument,        0, 'l' },
    { "help",       no_argument,        0, 'H' },
    { 0, 0, 0, 0 }
};

double infinity_is	= 0.0;

const char * help[] = 
{
    "Do not print heading line.",
    "Do not print raw results.",
    "Do not print statistical summary.",
    "Set \"infinity\" to the numeric, non-zero ARG.",
    "Output latex table.",
    "Display this help.",
    0 
};

#define DIFF( j, n ) 	(results[j].result[n] != results[EXACT_RESULTS].result[n] ? ( results[j].result[n] - results[EXACT_RESULTS].result[n] ) * 100. / results[EXACT_RESULTS].result[n] : 0.0)

static inline double skew( double n, double M_2, double M_3 )
{
    return sqrt( n ) * M_3 / pow( M_2, 1.5 );
}

void output_header( bool print_latex, char * argv[], unsigned int n_dirs );


int main( int argc, char * argv[] )
{
    struct {
	unsigned int n;
	float s0a;
	float s0b;
	float s1;
	float s2;
	double result[N_RESULTS];
    } results[MAX_ARGS];

    bool print_headers = true;
    bool print_results = true;
    bool print_summary = true;
    bool print_latex   = false;
    
    /* Option processing */

    for ( ;; ) {
        const int c = getopt_long( argc, argv, "hHlrs", longopts, NULL );
	if ( c == EOF ) break;
	char * endptr;
	
	switch ( c ) {
	case 'H':
	    fprintf( stderr, "Usage:  %s [options] results_1 results_2 ...\n", argv[0] );
	    fprintf( stderr, "Compute percent difference between results_n and result_1\n\nOptions:\n" );
	    for ( unsigned int i = 0; longopts[i].name != 0; ++i ) {
	        fprintf( stderr, " -%c, --%-12s %s\n", longopts[i].val, longopts[i].name, help[i] );
	    }
	    exit( 0 );
	case 'h':
	    print_headers = false;
	    break;
	case 'i':
	    infinity_is = fabs( strtod( optarg, &endptr ) );
	    if ( infinity_is == 0 ) {
		fprintf( stderr, "%s: Invalid argumement to --infinity: %s\n", argv[0], endptr );
		exit( 1 );
	    }
	    break;
	case 'l':
	    print_latex = true;
	    break;
	case 's':
	    print_summary = false;
	    break;
	case 'r':
	    print_results = false;
	    break;
	}
    }

    if ( print_latex ) {
	print_summary = true;
	print_results = false;
    }

    /* Real work */

    const int n_dirs = argc - optind;
    if ( n_dirs >= MAX_ARGS ) {
	fprintf( stderr, "Arg count: %d\n", argc );
	exit( 1 );
    }

    FILE * fd[MAX_ARGS];
    fd[0] = 0;
    for ( unsigned int j = EXACT_RESULTS; j < n_dirs; ++j ) {
	fd[j] = fopen( argv[optind+j], "r" );
	if ( !fd[j] ) { 
	    fprintf( stderr, "Cannot open %s: %s\n", argv[optind+j], strerror( errno ) );
	    exit( 1 );
	}
    }
  
    if ( !print_results && !print_summary ) {
        fprintf( stderr, "Nothing to print.\n" );
    }


    /* Initialize */
    
    moment moment[MAX_ARGS+1][N_RESULTS];		/* Momements for inline calculation */
    for ( unsigned int j = 0; j < n_dirs; ++j ) {
	for ( unsigned int k = 0; k < N_RESULTS; ++k ) {
	    moment[j][k].n    = 0.;
	    moment[j][k].mean = 0.;
	    moment[j][k].M2   = 0.;
	    moment[j][k].M3   = 0.;
	    moment[j][k].M4   = 0.;
	    moment[j][k].min  = MAXFLOAT;
	    moment[j][k].max  = -MAXFLOAT;
	}
    }
    
    if ( print_headers && print_results ) {
	output_header( print_latex, argv, n_dirs );
    }

    /* Load and compute stats */
    
    while ( !feof( fd[EXACT_RESULTS] ) ) {
	for ( unsigned int j = EXACT_RESULTS; j < n_dirs; ++j ) {
	    int count = fscanf( fd[j], "%d, %f, %f, %f, %f, %lf, %lf, %lf\n", &results[j].n, &results[j].s0a, &results[j].s0b, &results[j].s1, &results[j].s2, &results[j].result[LAMBDA_c0], &results[j].result[LAMBDA_c1], &results[j].result[UTIL_p0] );
	    if ( count != N_RESULTS+5 ) {
		fprintf( stderr, "%s: arg count: %d\n", argv[optind+j], count );
		exit( 1 );
	    }
	    if ( j > EXACT_RESULTS && results[j].n != results[EXACT_RESULTS].n ) {
		fprintf( stderr, "%s -> n=%d != %s -> n=%d\n", argv[EXACT_RESULTS], results[EXACT_RESULTS].n, argv[optind+j], results[j].n );
		exit( 1 );
	    }
	}

	if ( print_results ) { 
	    fprintf( stdout, "%2d %4.1f %4.1f %4.1f %4.1f  ", results[EXACT_RESULTS].n, results[EXACT_RESULTS].s0a, results[EXACT_RESULTS].s0b, results[EXACT_RESULTS].s1, results[EXACT_RESULTS].s2 );
	}
	  
	for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
	    double value[N_RESULTS];
	    for ( unsigned int k = 0; k < N_RESULTS; ++k ) {
		double x = DIFF(j,k);

		if ( infinity_is != 0 && (!isfinite(x) || fabs(x) > infinity_is) ) {
		    x = copysign( infinity_is, x );
		} else if ( !isfinite(x) ) {
		    continue;					/* Skip infinities */
		}
		value[k] = x;

		moment[j][k].n += 1.0;
		const double n = moment[j][k].n;
		const double delta = x - moment[j][k].mean;
		const double delta_n = delta / n;
		const double delta_n2 = delta_n * delta_n;
		const double term_1 = delta * delta_n * (n - 1);
		moment[j][k].mean += delta_n;
		moment[j][k].M4   += term_1 * delta_n2 * (n*n - 3*n + 3) + 6 * delta_n2 * moment[j][k].M2 - 4 * delta_n * moment[j][k].M3;
		moment[j][k].M3   += term_1 * delta_n * (n - 2) - 3 * delta_n * moment[j][k].M2;
		moment[j][k].M2   += term_1;
		if ( moment[j][k].min > x ) { moment[j][k].min = x; }
		else if ( moment[j][k].max < x ) { moment[j][k].max = x; }
	    }

	    if ( print_results ) fprintf( stdout, TEXT_FORMAT,
					  value[LAMBDA_c0],
					  value[LAMBDA_c1],
					  value[UTIL_p0] );
	}
	if ( print_results ) fprintf( stdout, "\n" );
    }
    if ( print_results ) fprintf( stdout, "\n" );

    /* */

    if ( print_summary ) {
	if ( print_headers ) {
	    output_header( print_latex, argv, n_dirs );
	} 

	if ( print_latex ) {
#if defined HORIZONTAL
	    fprintf( stdout, "mean" );
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, LATEX_FORMAT,
			 moment[j][LAMBDA_c0].mean,
			 moment[j][LAMBDA_c1].mean,
			 moment[j][UTIL_p0].mean );
	    }
	    fprintf( stdout, "\\\\\n" );
	    fprintf( stdout, "sdev" );
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, LATEX_FORMAT,
			 sqrt( moment[j][LAMBDA_c0].M2 / ( moment[j][LAMBDA_c0].n - 1.) ),
			 sqrt( moment[j][LAMBDA_c1 ].M2 / ( moment[j][LAMBDA_c1 ].n - 1.) ),
			 sqrt( moment[j][UTIL_p0   ].M2 / ( moment[j][UTIL_p0   ].n - 1.) ) );
	    }
	    fprintf( stdout, "\\\\\n\\hline\n" );
#else
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, "\\hline\n\\multicolumn{%d}{|c|}{%s}\\\\\n\\hline\n", N_RESULTS+1, argv[optind+j] );
		fprintf( stdout, "mean " LATEX_FORMAT "\\\\\n",
			 moment[j][LAMBDA_c0].mean,
			 moment[j][LAMBDA_c1].mean,
			 moment[j][UTIL_p0  ].mean );
		fprintf( stdout, "sdev " LATEX_FORMAT "\\\\\n",
			 sqrt( moment[j][LAMBDA_c0].M2 / ( moment[j][LAMBDA_c0].n - 1.) ),
			 sqrt( moment[j][LAMBDA_c1 ].M2 / ( moment[j][LAMBDA_c1 ].n - 1.) ),
			 sqrt( moment[j][UTIL_p0   ].M2 / ( moment[j][UTIL_p0   ].n - 1.) ) );
		fprintf( stdout, "\\hline\n" );
	    }
	    fprintf( stdout, "\\end{tabular}\n" );
#endif

	} else {
	    fprintf( stdout, "%-24s", "mean" );
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, TEXT_FORMAT,
			 moment[j][LAMBDA_c0].mean,
			 moment[j][LAMBDA_c1].mean,
			 moment[j][UTIL_p0  ].mean );
	    }
	    fprintf( stdout, "\n%-24s", "sdev" );
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, TEXT_FORMAT,
			 sqrt( moment[j][LAMBDA_c0].M2 / ( moment[j][LAMBDA_c0].n - 1.) ),
			 sqrt( moment[j][LAMBDA_c1 ].M2 / ( moment[j][LAMBDA_c1 ].n - 1.) ),
			 sqrt( moment[j][UTIL_p0   ].M2 / ( moment[j][UTIL_p0   ].n - 1.) ) );
	    }
	    fprintf( stdout, "\n%-24s", "skew" );
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, TEXT_FORMAT,
			 skew( moment[j][LAMBDA_c0].n, moment[j][LAMBDA_c0].M2, moment[j][LAMBDA_c0].M3 ),
			 skew( moment[j][LAMBDA_c1 ].n, moment[j][LAMBDA_c1].M2,  moment[j][LAMBDA_c1].M3 ),
			 skew( moment[j][UTIL_p0   ].n, moment[j][UTIL_p0  ].M2,  moment[j][UTIL_p0  ].M3 ) );
	    }
	    fprintf( stdout, "\n%-24s", "min" );
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, TEXT_FORMAT,
			 moment[j][LAMBDA_c0].min,
			 moment[j][LAMBDA_c1].min,
			 moment[j][UTIL_p0  ].min );
	    }
	    fprintf( stdout, "\n%-24s", "max" );
	    for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
		fprintf( stdout, TEXT_FORMAT,
			 moment[j][LAMBDA_c0].max,
			 moment[j][LAMBDA_c1].max,
			 moment[j][UTIL_p0  ].max );
	    }
	    fprintf( stdout, "\n" );
	}
    }

    exit( 0 );
}


void
output_header( bool print_latex, char * argv[], unsigned int n_dirs )
{
    if ( print_latex ) {
#ifdef HORIZONTAL
	fprintf( stdout, "\\begin{tabular}{|c|*{%d}{|*{%d}{d{2}|}}}\n\\hline\n", n_dirs-1, N_RESULTS );
	for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
	    fprintf( stdout, "& \\multicolumn{%d}{c||}{%s}\n", N_RESULTS, argv[optind+j] );
        }
	fprintf( stdout, "\\\\\n\\cline{2-%d}\n", (n_dirs-1)*N_RESULTS+1 );
	for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
	    fprintf( stdout, "& \\multicolumn{1}{c|}{$\\lambda$_{c0}} & \\multicolumn{1}{c|}{$\\lambda_{c1}$} & \\multicolumn{1}{c|}{$U_{p0}$} \\\\\n\\hline\n" );
	}
	fprintf( stdout, "\\\\\n\\hline\n" );
#else
	fprintf( stdout, "\\begin{tabular}{|c|*{%d}{d{2}|}}\n\\hline\n", N_RESULTS );
	fprintf( stdout, "$\\%%\\Delta$ & \\multicolumn{1}{c|}{$\\lambda_c0$} & \\multicolumn{1}{c|}{$\\lambda_c1$} & \\multicolumn{1}{c|}{$U_{p0}$} \\\\\n\\hline\n" );
#endif
    } else {
	fprintf( stdout, "%28s", " " );
	for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
	    fprintf( stdout, " %-27s ", argv[optind+j] );
        }
	fprintf( stdout, "\n%28s", " " );
	for ( unsigned int j = EXACT_RESULTS+1; j < n_dirs; ++j ) {
	    fprintf( stdout, " %-8s %-8s %-8s  ", "$f_c0", "$f_c1", "$u_p0" );
	}
	fprintf( stdout, "\n" );
    }
}
