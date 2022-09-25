/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* August 1991.                                                         */
/* March 2010.								*/
/************************************************************************/

/*
 * $Id: input.h 15895 2022-09-23 17:21:55Z greg $
 */

#if	!defined(LQIO_INPUT_H)
#define	LQIO_INPUT_H

/* Don't forget to fix labels.c */
/* Add a new scheduler type 'CFS' 2008*/
/* Add two new scheduler types for decisions  2016*/
typedef enum {
    SCHEDULE_CUSTOMER,
    SCHEDULE_DELAY,
    SCHEDULE_FIFO,
    SCHEDULE_LIFO,
    SCHEDULE_HOL,
    SCHEDULE_PPR,
    SCHEDULE_RAND,
    SCHEDULE_PS,
    SCHEDULE_POLL,
    SCHEDULE_BURST,
    SCHEDULE_UNIFORM,
    SCHEDULE_SEMAPHORE,
    SCHEDULE_CFS,
    SCHEDULE_RWLOCK
} scheduling_type;

typedef enum { DEFAULT_MATHERR, IGNORE_MATHERR, REPORT_MATHERR, ABORT_MATHERR } matherr_type;

/* Exit codes -- can be ored */
#define NORMAL_TERMINATION      (0)
#define INVALID_OUTPUT          (1 << 0)
#define INVALID_INPUT           (1 << 1)
#define INVALID_ARGUMENT        (1 << 2)
#define FILEIO_ERROR            (1 << 3)
#define EXCEPTION_EXIT	        0xff

#if defined(__cplusplus)
#include "error.h"
#include <map>
#include <string>

namespace LQIO {
    namespace SCHEDULE {
	extern const char * CUSTOMER;
	extern const char * DELAY;
	extern const char * FIFO;
	extern const char * LIFO;
	extern const char * HOL;
	extern const char * PPR;
	extern const char * RAND;
	extern const char * PS;
	extern const char * POLL;
	extern const char * BURST;
	extern const char * UNIFORM;
	extern const char * SEMAPHORE;
	extern const char * CFS;
	extern const char * RWLOCK;

	struct label_t {
	    const std::string str;
	    const std::string XML;
	    const char flag;
	};

    }
    
    typedef struct LQIO::error_message_type ErrorMessageType;

    typedef struct lqio_params_stats
    {
	lqio_params_stats( const char * version, void (*action)(error_severity) );
	void reset() { error_count = 0; }
	bool anError() const { return error_count > 0; }
	const char * toolname() const { return lq_toolname.c_str(); }
	void init( const std::string& version, const std::string& toolname, void (*sa)(error_severity) );

	std::string lq_toolname;                /* I:Name of tool for messages    */
	std::string lq_version;			/* I: version number	          */
	std::string lq_command_line;		/* I:Command line		  */
	void (*severity_action)(LQIO::error_severity);	/* I:Severity action              */

	unsigned max_error;			/* I:Maximum error ID number      */
	mutable unsigned error_count;		/* IO:Number of errors            */
	LQIO::error_severity severity_level;    /* I:Messages < severity_level ignored. */
    } lqio_params_stats;

    extern lqio_params_stats io_vars;
}

extern const std::map<const scheduling_type,const LQIO::SCHEDULE::label_t> scheduling_label; /*  */

#endif
#endif	/* LQIO_INPUT_H */
