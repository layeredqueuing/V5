/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 2012								*/
/************************************************************************/

#ifndef PETRISRVN_PHASE_H
#define PETRISRVN_PHASE_H

/*
 * $Id: petrisrvn.h 10943 2012-06-13 20:21:13Z greg $
 *
 * Solve LQN using petrinets.
 */

#include "petrisrvn.h"
#include <vector>
#include <map>
#include "wspnlib/global.h"

class Processor;
class Task;
class Entry;
class Phase;

namespace LQIO {
    namespace DOM {
	class Phase;
	class Call;
    }
}

struct slice_info_t {
    slice_info_t();
    double WX_xpos[MAX_MULT];
    double WX_ypos[MAX_MULT];
    struct place_object * WX[MAX_MULT];		/* Wait for proc		*/
    struct place_object * SX[MAX_MULT][MAX_STAGE+1];	/* Service		*/
    struct place_object * ChX[MAX_MULT];	/* Choose next action.		*/
    struct place_object * PrX[MAX_MULT];	/* Processor request.		*/
    struct place_object * PgX[MAX_MULT];	/* Processor grant.		*/
};

struct Call {
    Call() : _dom(NULL), _rpar_y(0), _w(0), _dp(0) {}

    bool is_rendezvous() const;
    bool is_send_no_reply() const;
    double value( const Phase *, double = 0.0 ) const;

    LQIO::DOM::Call * _dom;			/* DOMs for the calls		*/
    short _rpar_y;				/* Rendezvous rate (by phase).	*/
    double _w;					/* Waiting from entry to entry	*/
    double _dp;					/* Drop prob from entry to entry*/
    std::vector<double> _bin;			/* Histogram of customers	*/
};

class Phase {
public:
    typedef double (Phase::*util_fnptr)( unsigned );

public:
    Phase( LQIO::DOM::Phase * phase=nullptr, Task * task=nullptr );
    Phase& set_dom( LQIO::DOM::Phase * phase, Entry * entry );
    virtual ~Phase() {}

    const char * name() const;			/* Name of activity.		*/
    const Entry * entry() const { return _entry; }
    unsigned int entry_id() const;
    const Task * task() const { return _task; }
    const Processor * processor() const;
    LQIO::DOM::Phase * get_dom() const { return _dom; }
    LQIO::DOM::Call * get_call( const Entry * ) const;
    double y( const Entry * ) const;
    double z( const Entry * ) const;
    std::vector<double>* get_histogram( const Entry * entry ) const;
    short rpar_y( const Entry * ) const;	/* Rendezvous rate (by phase).	*/

    double s() const;				/* Service time	.		*/
    double coeff_of_var() const;		/* */
    double think_time() const;			/* Think time.			*/

    bool has_calls() const { return _call.size() > 0; }
    bool has_deterministic_service() const;
    bool has_stochastic_calls() const;
    bool is_hyperexponential() const;
    virtual bool is_activity() const { return false; }
    int is_erlang() const;
    int n_stages() const;

    unsigned int n_slices() const { return _n_slices; }	/* Number of slices for net.	*/
    double mean_processor_calls() const { return _mean_processor_calls; }

    virtual double check();
    Phase& add_call( LQIO::DOM::Call * );
    void build_forwarding_list();

    double transmorgrify( const double x_pos, const double y_pos, const unsigned m,
			  const LAYER layer_mask, const double p_pos, const short enabling );
    void create_spar();
    void create_ypar( Entry * entry );
    void remove_netobj();

    unsigned compute_offset( const Entry * ) const;
    double lambda( unsigned m, const Entry * b, const Phase * src_phase ) const;
    double drop_lambda( unsigned m, const Entry * b, const Phase * src_phase ) const;
    double get_utilization ( unsigned m  );
    double get_processor_utilization ( unsigned m  );
    double compute_queueing_delay( Call& call, const unsigned m, const Entry * b, const unsigned b_n, Phase * src_phase ) const;
    virtual double residence_time() const;
    double response_time( const Entry * dst ) const;

    virtual void insert_DOM_results() const;

    static void inc_par_offsets(void);

private:
    void follow_forwarding_path( const unsigned slice_no, Entry * a, double rate );

    void request_processor( struct trans_object * c_trans,  const unsigned m, const unsigned s ) const;
    void processor_acquired( struct trans_object * c_trans, const unsigned m, const unsigned s ) const;
    void release_processor( struct trans_object * c_trans,  const unsigned m, const unsigned s ) const;

    double service_rate() const;
    double utilization() const;
    double queueing_time( const Entry * dst ) const;
    double drop_probability( const Entry * dst ) const;
    double number_of_calls( const LQIO::DOM::Call * ) const;

    bool simplify_phase() const;

protected:
    LQIO::DOM::Phase * _dom;
    const Entry * _entry;			/* Index to entry.		*/
    const Task * _task;				/* Index to task.	        */

private:
    std::map<const Entry *,Call> _call;
    short _rpar_s[2];	     			/* Service trate parameter.	*/
    double _prob_a;				/* For hyperexpontial		*/
    unsigned _n_slices;				/* Number of slices.		*/
    double _mean_processor_calls;		/* Avg # of calls to processor	*/

public:
    slice_info_t _slice[DIMSLICE];		/* Slices.			*/
    double done_xpos[MAX_MULT];			/* X position			*/
    double done_ypos[MAX_MULT];			/* Y position			*/

    struct place_object * XX[MAX_MULT];		/* Service Time result (BUG_622)*/
    struct place_object * ZX[MAX_MULT];		/* Think Time.			*/
    struct trans_object * doneX[MAX_MULT];	/* Phase is done.		*/
    double task_tokens[MAX_MULT];		/* Results (task util.)		*/
    double proc_tokens[MAX_MULT];		/* Results (proc util.)		*/

public:
    static double __parameter_x;		/* Offset for parameters.	*/
    static double __parameter_y;
};
#endif
