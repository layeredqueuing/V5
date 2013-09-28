/* result.h	-- Greg Franks
 *
 * $HeadURL$
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#ifndef _RESULT_H
#define _RESULT_H

extern unsigned number_blocks;	/* For block statistics. 	*/

/* Can't be class (because constructors NOT allowed in actlist union */

struct result_t {
    void init( int type, const char * format, ... );
    void init( const long stat_id );
    double accumulate();
    void accumulate_variance ( const double );
    void accumulate_service( const result_t& r_cycle );
    void accumulate_utilization( const result_t& r_cycle, const double service_time );
    
    void reset();		/* Result the raw counter	*/     
    void clear_results();	/* Clear everthing.		*/
    bool has_results() const { return _count > 0.; }

    double mean() const;
    double variance() const;
    double mean_count() const;
    double variance_count() const;
    static double conf95( const unsigned );
    static double conf99( const unsigned );

    FILE * print_raw( FILE * output, const char * format, ... ) const;

public:
    long raw;			/* index to raw value.		*/
     
private:
    double _sum;		/* Sum of values.		*/
    double _sum_sqr;		/* Sum of squares		*/
    double _count;		/* Number of items.		*/
    double _count_sqr;		/* Number of items.		*/
    double _avg_count;		/* Average or count.		*/
    unsigned _n;		/* Number of hits in sample	*/

private:
    int _type;
};


/*
 * Return square.
 */

static inline double square( const double arg )
{
    return arg * arg;
}
#endif
