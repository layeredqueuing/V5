/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* March 2013.								*/
/************************************************************************/

/*
 * Comparison of srvn output results.
 * By Greg Franks.  August, 1991.
 *
 * $Id: srvndiff.cc 11320 2013-03-04 20:27:18Z greg $
 */

#include "srvndiff.h"
#include "parseable.h"
#include "symtbl.h"
#include <cmath>
#include <vector>
#include <algorithm>

static double get_diff( result_str_t result, unsigned int i, unsigned int k=0, unsigned int p=0 )
{
    double value[MAX_PASS];
    double conf_value[MAX_PASS];
    (*result_str[(int)result].func)( value, conf_value, i, FILE1, k, p );
    (*result_str[(int)result].func)( value, conf_value, i, FILE2, k, p );
    const double diff = fabs( value[FILE2] - value[FILE1] );
    return diff == 0 ? 0.0 : diff * 100. / value[FILE1];
}

struct HasOpenArrivals {
    bool operator()( const std::pair<int, entry_info>& p ) const { return p.second.open_arrivals; }
};

void print_parseable( FILE * output )
{
    fprintf( output, "# ----- Difference output -----\n" );
    fprintf( output, "# %s\n", command_line.c_str() );
    fprintf( output, "V y\nC 0\nI 0\nPP %ld\nNP %d\n\n", processor_tab[FILE1].size(), phases );

    /* Waiting times */

    fprintf( output, "W 0\n" );
    for ( std::map<int,task_info>::const_iterator t = task_tab[FILE1].begin(); t != task_tab[FILE1].end(); ++t ) {
	const unsigned int task_id = t->first;
	const std::set<int>& entry_list = task_entry_tab[task_id];
	for ( std::set<int>::const_iterator e = entry_list.begin(); e != entry_list.end(); ++e ) {
	    const int from_entry = *e;
	    std::map<int,std::vector<call_info> > calls_by_phase;
	    /* This won't work because I need to find all calls to an entry... */
	    /* need std::map<to_entry,std::vector<double> > */
	    /* go through to here and load up map... */
	    for ( unsigned p = 0; p < phases; ++p ) {
		const std::map<int,call_info>& to = entry_tab[FILE1][from_entry].phase[p].to;
		for ( std::map<int,call_info>::const_iterator k = to.begin(); k != to.end(); ++k ) {
		    const unsigned to_entry = k->first;
		    std::vector<call_info>& calls = calls_by_phase[to_entry];
		    if ( calls.size() < phases ) { 
			calls.resize(phases);
		    }
		    calls[p] = k->second;
		}
	    }
	    if ( calls_by_phase.size() ) {
		/* Now go through new map outputting vectors. */
		/* if map is not empty... */
		for ( std::map<int,std::vector<call_info> >::const_iterator t = calls_by_phase.begin(); t != calls_by_phase.end(); ++t ) {
		    const int to_entry = t->first;
		    if ( e == entry_list.begin() && t == calls_by_phase.begin() ) {
			fprintf( output, "%s : ", find_symbol_pos( task_id, ST_TASK ) );
		    } else {
			fprintf( output, "     " );
		    }
		    fprintf( output, "%s %s ", find_symbol_pos( from_entry, ST_ENTRY ),
			     find_symbol_pos( to_entry, ST_ENTRY ) );
		    for ( unsigned int p = 0; p < phases; ++p ) {
			fprintf( output, " %7.5g", get_diff( P_WAITING, from_entry, to_entry, p ) );				/* Phase service 		*/
		    }
		    fprintf( output, " -1\n" );
		}
		fprintf( output, "     -1\n" );
	    }
	}
    }
    fprintf( output, "-1\n\n" );

    /* Service time */

    fprintf( output, "X %ld\n", entry_tab[FILE1].size() );
    for ( std::map<int,task_info>::const_iterator t = task_tab[FILE1].begin(); t != task_tab[FILE1].end(); ++t ) {
	const unsigned int task_id = t->first;
	const std::set<int>& entry_list = task_entry_tab[task_id];
	for ( std::set<int>::const_iterator e = entry_list.begin(); e != entry_list.end(); ++e ) {
	    const int entry_id = *e;
	    if ( e == entry_list.begin() ) {
		fprintf( output, "%s : ", find_symbol_pos( task_id, ST_TASK ) );
	    } else {
		fprintf( output, "     " );
	    }
	    fprintf( output, "%s", find_symbol_pos( entry_id, ST_ENTRY) );
	    for ( unsigned int p = 0; p < phases; ++p ) {
		fprintf( output, " %7.5g", get_diff( P_SERVICE, entry_id, 0, p ) );					/* Phase service 		*/
	    }
	    fprintf( output, " -1\n" );
	}
	fprintf( output, "     -1\n" );
    }
    fprintf( output, "-1\n\n" );

    /* Task Throughput and Utilization */
    fprintf( output, "FQ %ld\n", task_tab[FILE1].size() );
    for ( std::map<int,task_info>::const_iterator t = task_tab[FILE1].begin(); t != task_tab[FILE1].end(); ++t ) {
	const unsigned int task_id = t->first;
	const std::set<int>& entry_list = task_entry_tab[task_id];
	for ( std::set<int>::const_iterator e = entry_list.begin(); e != entry_list.end(); ++e ) {
	    const int entry_id = *e;
	    if ( e == entry_list.begin() ) {
		fprintf( output, "%s : ", find_symbol_pos( task_id, ST_TASK ) );
	    } else {
		fprintf( output, "     " );
	    }
	    
	    fprintf( output, "%s %7.5g", find_symbol_pos( entry_id, ST_ENTRY), get_diff( P_ENTRY_TPUT, entry_id ) );	/* Throughput 		*/
	    for ( unsigned int p = 0; p < phases; ++p ) {
		fprintf( output, " %7.5g", get_diff( P_PHASE_UTIL, entry_id, 0, p ) );					/* Phase util 		*/
	    }
	    fprintf( output, " -1 %7.5g\n", get_diff( P_ENTRY_UTIL, entry_id ) );					/* Phase Utilization 	*/
	}
	fprintf( output, "     -1\n" );
	if ( entry_list.size() > 1 ) {
	    fprintf( output, "        %7.5g", get_diff( P_THROUGHPUT, task_id ) );
	    for ( unsigned int p = 0; p < phases; ++p ) {
		fprintf( output, " %7.5g", get_diff( P_TASK_UTILIZATION, task_id, 0, p ) );				/* Phase util 		*/
	    }
	    fprintf( output, " -1 %7.5g\n", get_diff( P_UTILIZATION, task_id ) );					/* Total Utilization 	*/
	}
    }
    fprintf( output, "-1\n\n" );

    /* Open arrivals */

    const unsigned count = std::count_if( entry_tab[FILE1].begin(), entry_tab[FILE1].end(), HasOpenArrivals() );
    if ( count > 0 ) {
	fprintf( output, "R %d\n", count );
	for ( std::map<int,task_info>::const_iterator t = task_tab[FILE1].begin(); t != task_tab[FILE1].end(); ++t ) {
	    const unsigned int task_id = t->first;
	    const std::set<int>& entry_list = task_entry_tab[task_id];
	    for ( std::set<int>::const_iterator e = entry_list.begin(); e != entry_list.end(); ++e ) {
		const int entry_id = *e;
		if ( entry_tab[FILE1][entry_id].open_arrivals ) {
		    fprintf( output, "%s : %s %7.5g %7.5g\n", find_symbol_pos( task_id, ST_TASK ), find_symbol_pos( entry_id, ST_ENTRY),
			     0.0, get_diff( P_OPEN_WAIT, entry_id ) );
		}
	    }
	}
	fprintf( output, "-1\n" );
    }
    
    /* Processor utilization and waiting */

    for ( std::map<int,processor_info>::const_iterator p = processor_tab[FILE1].begin(); p != processor_tab[FILE1].end(); ++p ) {
	const unsigned int proc_id = p->first;
	const std::set<int>& task_list = proc_task_tab[proc_id];

	fprintf( output, "P %s %ld\n", find_symbol_pos( proc_id, ST_PROCESSOR ), task_list.size() );
	for ( std::set<int>::const_iterator t = task_list.begin(); t != task_list.end(); ++t ) {
	    const int task_id = *t;
	    const std::set<int>& entry_list = task_entry_tab[task_id];

	    fprintf( output, "  %s %ld 0 0", find_symbol_pos( task_id, ST_TASK ), entry_list.size() );
	    for ( std::set<int>::const_iterator e = entry_list.begin(); e != entry_list.end(); ++e ) {
		const int entry_id = *e;
		if ( e != entry_list.begin() ) {
		    fprintf( output, "          " );
		}
		fprintf( output, " %s %7.5g", find_symbol_pos( entry_id, ST_ENTRY ), get_diff( P_ENTRY_PROC, entry_id ) );
		for ( unsigned int p = 0; p < phases; ++p ) {
		    fprintf( output, " %7.5g", get_diff( P_PROCESSOR_WAIT, entry_id, 0, p ) );				/* Phase waiting 		*/
		}
		fprintf( output, " -1\n" );										/* End of phase list */
	    }
	    fprintf( output, "           -1\n" );									/* End of entry list */
	    if ( entry_list.size() > 1 ) {
		fprintf( output, "              %7.5g\n", get_diff( P_TASK_PROC_UTIL, task_id ) );			/* Proc Task total */
	    }
	}
	if ( task_list.size() > 1 ) {
	    fprintf( output, "  -1\n" );
	    fprintf( output, "              %7.5g\n", get_diff( P_PROCESSOR_UTIL, proc_id ) );	/* Proc total */
	}
	fprintf( output, "-1\n\n" );
    }
    fprintf( output, "-1\n\n" );
}
