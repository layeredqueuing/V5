/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/* January 2005.							*/
/************************************************************************/

/*
 * $Id: model.h 13808 2020-09-08 21:16:53Z greg $
 *
 * Solve LQN using petrinets.
 */

#ifndef PETRISRVN_MODEL_H
#define PETRISRVN_MODEL_H

#include <string>
#include <sys/times.h>
#include <lqio/filename.h>
#include <lqio/dom_document.h>
#include <lqio/common_io.h>

namespace LQIO {
    namespace DOM {
	class Document;
	class Entry;
	class Call;
    }
}


class Task;
class Entry;
class Phase;
struct solution_stats_t;

struct debug_place_info {
    struct place_object * place;
    Phase * c;			/* Calling entry		*/
    Phase * d;			/* Called entry			*/
    unsigned m;			/* Instance of caller.		*/
};

#define	DIMDBGPLC	(DIME+1)*2

class Model {
    typedef void (Model::*queue_fnptr)( double x_pos,		/* x coordinate.		*/
					double y_pos,		/* y coordinate.		*/
					double idle_x,
					Phase * a,		/* Source Entry (send from)	*/
					const unsigned s_a,	/* Sending slice number.	*/
					Entry * b,		/* Destination entry.		*/
					const Phase * e,	/* Entry to reply to.		*/
					const unsigned s_e,	/* Slice to reply to.		*/
					const unsigned m,	/* Multiplicity of Src.		*/
					const double prob_fwd,
					const unsigned k,	/* an index.			*/
					const bool async_call,
					struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] );

public:
    static LQIO::DOM::Document* load( const std::string& inputFileName, const std::string& outputFileName );
    explicit Model( LQIO::DOM::Document *, const std::string&, const std::string& );
    virtual  ~Model();

    bool operator!() const { return _document == 0; }

    unsigned int n_phases() const { return _n_phases; }
    
    bool construct();
    static void recalculateDynamicValues( const LQIO::DOM::Document* );
    void initialize();

    int solve();
    int restart();
    int reload();		/* Load results from LQX */

    unsigned int set_queue_length() const;

    void insert_DOM_results( const bool valid, const solution_stats_t& );

private:
    bool has_output_file_name() const { return _output_file_name.size() > 0 && _output_file_name != "-"; }

    void set_comment();
    Model& set_n_phases( const unsigned int );

    void remove_netobj();

    bool transform();
    void trans_rpar();
    void trans_res();

    void make_queues();
    unsigned make_queue( double x_pos,		/* x coordinate.		*/
			 double y_pos,		/* y coordinate.		*/
			 double idle_x,
			 Phase * a,		/* Source Entry (send from)	*/
			 Entry * b,		/* Destination entry.		*/
			 const unsigned ne,
			 const unsigned max_m,	/* Multiplicity of Src.		*/
			 unsigned k,		/* an index.			*/
			 struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT],
			 queue_fnptr queue_func );
    void fifo_queue( double x_pos,		/* x coordinate.		*/
		     double y_pos,		/* y coordinate.		*/
		     double idle_x,
		     Phase * a,			/* Source Entry (send from)	*/
		     const unsigned s_a,	/* Sending slice number.	*/
		     Entry * b,			/* Destination entry.		*/
		     const Phase * e,		/* Entry to reply to.		*/
		     const unsigned s_e,	/* Slice to reply to.		*/
		     const unsigned m,		/* Multiplicity of Src.		*/
		     const double prob_fwd,
		     const unsigned k,		/* an index.			*/
		     const bool async_call,
		     struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] );
    void random_queue( double x_pos,		/* x coordinate.		*/
		       double y_pos,		/* y coordinate.		*/
		       double idle_x,
		       Phase * a,		/* Source Entry (send from)	*/
		       const unsigned s_a,	/* Slice number of a.		*/
		       Entry * b,		/* Destination entry.		*/
		       const Phase * e,		/* Entry to reply to.		*/
		       const unsigned s_e,	/* Slice to reply to.		*/
		       const unsigned m,	/* Multiplicity of Src.		*/
		       const double prob_fwd,
		       const unsigned k,	/* an index.			*/
		       const bool async_call,
		       struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] );
    struct trans_object * queue_prologue( double x_pos,		/* X coordinate.		*/
					  double y_pos,		/* Y coordinate.		*/
					  Phase * a,		/* sending entry.		*/
					  unsigned s_a,		/* Slice number of phase	*/
					  Entry * b,		/* receiving entry.		*/
					  unsigned b_m,		/* instance number of b.	*/
					  const Phase * e,	/* Entry to reply to.		*/
					  const unsigned s_e,	/* Slice to reply to.		*/
					  unsigned m,		/* instance number of a.	*/
					  double prob_fwd,
					  bool async_call,	/* True if z type call.		*/
					  struct trans_object **r_trans );
    void queue_epilogue( double x_pos, 
			 double y_pos,
			 Phase * a, 
			 unsigned s_a,		/* Source Phase 	*/
			 Entry * b, 
			 unsigned b_m,		/* Destination Entry 	*/
			 const Phase * e, 
			 unsigned s_e,
			 unsigned m,		/* Source multiplicity 	*/
			 bool async_call,	/* True if z type call.	*/
			 struct trans_object * q_trans, 
			 struct trans_object * s_trans );
    void create_phase_instr_net( double idle_x, double y_pos, 
				 Phase * a, unsigned m,
				 Entry * b, unsigned n, unsigned k,
				 struct trans_object * r_trans, struct trans_object * q_trans, struct trans_object * s_trans, 
				 struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] );
    void create_inservice_net( double x_pos, double y_pos,
			       Phase * a,	/* Entry of calling task 'i'	*/
			       Entry * b,	/* Entry of server 'j'		*/
			       unsigned m,	/* Instance of task 'i'		*/
			       struct debug_place_info ins_place[DIMPH+1][DIMDBGPLC][MAX_MULT] );

    void build_open_arrivals ();

    void print() const;
    std::string createDirectory() const;
    void print_inservice_probability( std::ostream& output ) const;
    unsigned print_inservice_cd( std::ostream& output, const Entry * a, const Entry * b, const Task * j, double tot_tput[], double col_sum[DIMPH+1] ) const;

private:
    LQIO::DOM::Document * _document;
    std::string _input_file_name;
    std::string _output_file_name;
    static LQIO::DOM::CPUTime __start_time;
    unsigned int _n_phases;

public:
    static bool __forwarding_present;
    static bool __open_class_error;
    static LQIO::DOM::Document::input_format __input_format;
};
#endif
