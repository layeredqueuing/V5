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
 * $Id: model.h 17348 2024-10-09 18:54:22Z greg $
 *
 * Solve LQN using petrinets.
 */

#ifndef PETRISRVN_MODEL_H
#define PETRISRVN_MODEL_H

#include <string>
#include <vector>
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
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
struct place_object;

class Model {
private:
    struct inst_arrival {
	inst_arrival( double _x_pos, double _y_pos, const Phase * _a, const unsigned int _m, const Entry * _b, const unsigned int _n, const place_object * _ab_place ) :
	    x_pos(_x_pos), y_pos(_y_pos), a(_a), m(_m), b(_b), n(_n), ab_place(_ab_place) {}
	const double x_pos;
	const double y_pos;
	const Phase * a;
	const unsigned int m;
	const Entry * b;
	const unsigned int n;
	const struct place_object * ab_place;
    };

public:
    typedef bool (Model::*solve_using)();
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
					std::vector<Model::inst_arrival>& ph2_place );

private:
    explicit Model( LQIO::DOM::Document *, const std::string&, const std::string&, LQIO::DOM::Document::OutputFormat );
    template <typename Type> static inline void Delete( Type x ) { delete x; }

public:
    virtual  ~Model();

    bool operator!() const { return _document == 0; }

    unsigned int n_phases() const { return _n_phases; }

    static int solve( solve_using, const std::string&, LQIO::DOM::Document::InputFormat, const std::string&, LQIO::DOM::Document::OutputFormat, const LQIO::DOM::Pragma& );
    static void recalculateDynamicValues( const LQIO::DOM::Document* );

    bool compute();
    bool restart();
    bool reload();		/* Load results from LQX */

private:
    static LQIO::DOM::Document* load( const std::string& inputFileName, LQIO::DOM::Document::InputFormat input_format );
    bool construct();
    void clear();

    unsigned int set_queue_length() const;

    void insert_DOM_results( const bool valid, const solution_stats_t& ) const;

private:
    bool has_output_file_name() const { return _output_file_name.size() > 0 && _output_file_name != "-"; }

    static void setModelParameters( const LQIO::DOM::Document * );

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
			 queue_fnptr queue_func,
			 std::vector<Model::inst_arrival>& ph2_place );
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
		     std::vector<Model::inst_arrival>& ph2_place );
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
		       std::vector<Model::inst_arrival>& ph2_place );
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
				 std::vector<Model::inst_arrival>& );
    void create_inservice_net( const Phase * c, const Entry * d, unsigned int k, const std::vector<Model::inst_arrival>& ph2_place );
    void build_open_arrivals ();

    void print() const;
    void print_inservice_probability( std::ostream& output ) const;
    unsigned print_inservice_cd( std::ostream& output, const Entry * a, const Entry * b, const Task * j, double tot_tput[], double col_sum[DIMPH+1] ) const;

private:
    LQIO::DOM::Document * _document;
    const std::string _input_file_name;
    const std::string _output_file_name;
    const LQIO::DOM::Document::OutputFormat _output_format;
    unsigned int _n_phases;

public:
    static bool __forwarding_present;
    static bool __open_class_error;

private:
    static LQIO::DOM::CPUTime __start_time;
    static const std::map<const LQIO::DOM::Document::OutputFormat,const std::string> __parseable_output;
};
#endif
