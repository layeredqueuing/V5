/* -*- C++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* June 2012.								*/
/************************************************************************/

#ifndef _TASK_H
#define _TASK_H

#include <vector>
#include <string>
#include <wspnlib/global.h>
#include "petrisrvn.h"

/*
 * $Id: petrisrvn.h 10943 2012-06-13 20:21:13Z greg $
 *
 * Solve LQN using petrinets.
 */

#include "place.h"

typedef enum {
    SERVER,
    INF_SERV,
    REF_TASK,
    OPEN_SRC,
    SEMAPHORE
} task_type;

#define SERVER_BIT	(1<<SERVER)
#define INF_SERV_BIT	(1<<INF_SERV)
#define REF_TASK_BIT	(1<<REF_TASK)
#define OPEN_SRC_BIT	(1<<OPEN_SRC)
#define SEMAPHORE_BIT	(1<<SEMAPHORE)

#define X_OFFSET( x,delta )	(((double)(((x)*ne)/2.0)+(delta+x_pos))*::x_scaling)
#define	Y_OFFSET( y )		((double)(y)+y_pos)


class Activity;
class ActivityList;
class Entry;
class Processor;
class Phase;
namespace LQIO {
    namespace DOM {
	class Task;
	class Activity;
	class Call;
	class Entry;
	class Document;
    }
}

class Task : public Place {
protected:
    Task( LQIO::DOM::Task* dom, task_type type, Processor * );

private:
    Task( const Task& );
    Task& operator=( const Task& );

public:
    static Task * create( LQIO::DOM::Task * dom );
    bool check();

    task_type type() const { return _type; }
    virtual int priority() const;			/* Priority for this task.	*/
    virtual double think_time() const;			/* Think time for ref. task.	*/

    unsigned int n_phases() const { return _n_phases; }
    Task& set_n_phases( unsigned int n ) { if ( n > _n_phases ) _n_phases = n; return *this; }
    unsigned int n_customers() const;
    unsigned int n_entries() const { return entries.size(); }
    unsigned int n_activities() const { return activities.size(); }
    unsigned int n_act_lists() const { return act_lists.size(); }
    unsigned int n_threads() const { return _n_threads; }
    unsigned int n_open_tokens() const { return _open_tokens; }
    unsigned int max_queue_length() const { return _max_queue_length; }
    unsigned max_k() const { return _max_k; }
    Task& set_max_k( unsigned int k ) { _max_k = k; return *this; }

    virtual bool is_client() const;
    virtual bool is_server() const;
    bool is_single_place_task() const;
    virtual bool is_sync_server() const { return _sync_server; }
    virtual bool inservice_flag() const { return _inservice_flag; }
    virtual bool has_main_thread() const { return _has_main_thread; }

    Processor * processor() const { return _processor; }
    double utilization( const unsigned int m ) const { return _utilization[m]; }

    Activity * add_activity( LQIO::DOM::Activity * activity );
    Activity * find_activity( const std::string& name ) const;
    void  set_start_activity ( LQIO::DOM::Entry* theDOMEntry );

    void initialize();
    void remove_netobj();
    unsigned int set_queue_length();
    void build_forwarding_lists();
    void transmorgrify();
    void make_queue_places();

    void get_results();
    void get_total_throughput( Task * dst, double tot_tput[] );

    virtual void insert_DOM_results();

private:
    double create_instance( double base_x_pos, double base_y_pos, unsigned m, short enabling );
    double create_activity( const double x_pos, const double y_pos, const unsigned m,
			    Activity * curr_act, const Entry * e, const unsigned p_pos, const short enabling,
			    struct place_object * end_place, bool can_reply );
    void create_forwarding_gspn( unsigned e );
    LAYER make_layer_mask( const unsigned m );

private:
    virtual void get_results( unsigned int m );
    double get_throughput( const Entry * d, const Phase * curr_phase, unsigned m  );

public:
    std::vector<Entry *> entries;		/* Entry Lists.			*/
    std::vector<Activity *> activities;		/* Activity list.		*/
    std::vector<ActivityList *> act_lists;	/* Forks-Join lists.		*/
    struct place_object * TX[MAX_MULT];		/* Task place.			*/
#if defined(BUG_163)
    struct place_object * SyX[MAX_MULT];	/* Sync wait place.		*/
#endif
    struct place_object * GdX[MAX_MULT];	/* Guard Place			*/
    struct trans_object * gdX[MAX_MULT];	/* Guard fork transition.	*/
    struct place_object * LX[MAX_MULT];		/* Lock Place	(BUG_164)	*/
#if !defined(BUFFER_BY_ENTRY)
    struct place_object * ZZ;			/* For open requests.		*/
#endif

private:
    Processor * _processor;			/* Processor ID			*/
    const task_type _type;			/* Task types.			*/
    bool _sync_server;				/* true for external sync.	*/
    bool _has_main_thread;			/* true if fork and join.	*/
    bool _inservice_flag;			/* Print inservice probs.	*/
    bool _needs_flush;				/* true if fork and no join.	*/
    bool _queue_made;				/* true if queue made for task.	*/
    unsigned _n_phases;
    unsigned _n_threads;
    unsigned _max_queue_length;
    unsigned _max_k;				/* input queues. 		*/
#if !defined(BUFFER_BY_ENTRY)
    unsigned _open_tokens;			/* Size of queue		*/
#endif
    unsigned _requestor_no;

public:
    double task_tokens[MAX_MULT];		/* Result. 			*/
    double lock_tokens[MAX_MULT];		/* Result.			*/

private:
    double _utilization[MAX_MULT];		/* Result for finding util.	*/

public:
    static double __server_x_offset;		/* Starting offset for next server.	*/
    static double __client_x_offset;		/* Starting offset for next client.	*/
    static double __server_y_offset;
    static double __queue_y_offset;
};

class OpenTask : public Task {
public:
    OpenTask( LQIO::DOM::Document * document, const std::string& name, const Entry * dst ) : Task( 0, OPEN_SRC, 0 ), _document(document), _name(name), _dst(dst) {}

    const char * name() const { return _name.c_str(); }

    virtual unsigned int multiplicity() const { return 1; }
    virtual double think_time() const { return 0.0; }

    virtual bool is_client() const { return true; }
    virtual bool is_server() const { return false; };
    virtual bool is_sync_server() const { return false; }
    virtual bool inservice_flag() const { return false; }
    virtual bool has_main_thread() const { return false; }

    virtual void get_results( unsigned int m );
    virtual void insert_DOM_results();

private:
    LQIO::DOM::Document * _document;
    const std::string _name;
    const Entry * _dst;
};

extern std::vector<Task *> task;

#endif
