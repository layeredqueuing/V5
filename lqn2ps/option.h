/* -*- c++ -*-
 * lqn2ps.h	-- Greg Franks
 *
 * $Id: option.h 15434 2022-02-09 00:28:27Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef LQN2PS_OPTION_H
#define LQN2PS_OPTION_H

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <map>
#include <string>

enum class Aggregate {
    NONE,
    SEQUENCES,
    ACTIVITIES,
    PHASES,
    ENTRIES,
    THREADS
};


enum class Colouring {
    NONE,
    INPUT,		/* Input model -- flag errors? */
    RESULTS,		/* Default */
    LAYERS,		/* Each layer gets its own colour */
    CLIENTS,		/* Each client chaing gets its own colour */
    SERVER_TYPE,	/* client, server, etc... */
    CHAINS,		/* Useful for queueing output only */
    DIFFERENCES		/* Results are differences */
};
	

enum class File_Format {
    EEPIC,
#if EMF_OUTPUT
    EMF,
#endif
    FIG,
#if HAVE_GD_H && HAVE_LIBGD && HAVE_GDIMAGEGIFPTR
    GIF,
#endif
#if JMVA_OUTPUT && HAVE_EXPAT_H
    JMVA,
#endif
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBJPEG
    JPEG,
#endif
    JSON,
    LQX,
    NO_OUTPUT,
    OUTPUT,
#if QNAP2_OUTPUT
    QNAP2,
#endif
    PARSEABLE,
#if HAVE_GD_H && HAVE_LIBGD && HAVE_LIBPNG
    PNG,
#endif
    POSTSCRIPT,
    PSTEX,
    RTF,
    SRVN,
#if SVG_OUTPUT
    SVG,
#endif
#if SXD_OUTPUT
    SXD,
#endif
#if TXT_OUTPUT
    TXT,
#endif
#if defined(X11_OUTPUT)
    X11,
#endif
    XML,
    UNKNOWN
};

enum class Justification {
    DEFAULT,
    CENTER,
    LEFT,
    RIGHT,
    ALIGN,		/* For Nodes		*/
    ABOVE		/* For labels on Arcs.	*/
}; 

enum class Key_Position {
    NONE,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    BELOW_LEFT,
    ABOVE_LEFT,
    ON
};

enum class Layering {
    BATCH,
    GROUP,
    HWSW,
    MOL,
    PROCESSOR,
    PROCESSOR_TASK,
    SHARE,
    SQUASHED,
    SRVN,
    TASK_PROCESSOR
};

enum class Processors {
    ALL,
    DEFAULT,
    NONE,
    NONINFINITE
};

enum class Replication {
    EXPAND,
    NONE,
    REMOVE,
    RETURN
};

enum class Special {
    ANNOTATE,
    ARROW_SCALING,
    BCMP,
    CLEAR_LABEL_BACKGROUND,
    EXHAUSTIVE_TOPOLOGICAL_SORT,
    FLATTEN_SUBMODEL,
    FORCE_INFINITE,
    FORWARDING_DEPTH,
    GROUP,
    LAYER_NUMBER,
    NONE,
    NO_ALIGNMENT_BOX,
    NO_ASYNC_TOPOLOGICAL_SORT,
    NO_CV_SQR,
    NO_PHASE_TYPE,
    NO_REF_TASK_CONVERSION,
    PROCESSOR_SCHEDULING,
    PRUNE,
    QUORUM_REPLY,
    RENAME,
    SORT,
    SPEX_HEADER,
    SQUISH_ENTRY_NAMES,
    SUBMODEL_CONTENTS,
    TASKS_ONLY,	
    TASK_SCHEDULING
};


enum class Sorting {
    FORWARD,
    REVERSE,
    TOPILOGICAL,
    NONE
};

class Options
{
public:
    struct Type
    {
	const char * name;
	const int c;
	const char * arg;
	struct opts {
	    union param {		/* Parameter */
		param(const std::map<const Aggregate,const std::string>* _a_) : a(_a_) {}
		param(const std::map<const Colouring,const std::string>* _c_) : c(_c_) {}
		param(const std::map<const File_Format,const std::string>* _f_) : f(_f_) {}
		param(const std::map<const Justification,const std::string>* _j_ ) : j(_j_) {}
		param(const std::multimap<const Key_Position,const std::string>*  _k_ ) : k(_k_) {}
		param(const std::map<const Layering,const std::string>* _l_) : l(_l_) {}
		param(const std::map<const Sorting,const std::string>* _o_ ) : o(_o_) {}
		param(const std::map<const Processors,const std::string>* _p_) : p(_p_) {}
		param(const std::map<const Replication,const std::string>* _r_ ) : r(_r_) {}
		param(const std::string* _s_ ) : s(_s_) {}		/* Strings, integers, etc.	*/
		param(const std::map<const Special,const std::string>* _z_) : z(_z_) {}

		const std::string* s;
		const std::map<const Aggregate,const std::string>* a;
		const std::map<const Colouring,const std::string>* c;
		const std::map<const File_Format,const std::string>* f;
		const std::map<const Justification,const std::string>* j;
		const std::multimap<const Key_Position,const std::string>* k;
		const std::map<const Layering,const std::string>* l;
		const std::map<const Sorting,const std::string>* o;
		const std::map<const Processors,const std::string>* p;
		const std::map<const Replication,const std::string>* r;
		const std::map<const Special,const std::string>* z;
	    } param;
	    union arg {		/* Argument */
		arg() : s(nullptr) {}
		arg(Aggregate _a_) : a(_a_) {}
		arg(bool _b_) : b(_b_) {}
		arg(Colouring _c_) : c(_c_) {}
		arg(double _d_) : d(_d_) {}
		arg(File_Format _f_) : f(_f_) {}
		arg(Justification _j_) : j(_j_) {}
		arg(int _i_) : i(_i_) {}
		arg(Key_Position _k_ ) : k(_k_) {}
		arg(Layering _l_) : l(_l_) {}
		arg(std::regex * _m_) : m(_m_) {}
		arg(Sorting _o_) : o(_o_) {}
		arg(Processors _p_) : p(_p_) {}
		arg(Replication _r_) : r(_r_) {}
		arg(Special _z_) : z(_z_) {}
		arg(const std::string * _s_) : s(_s_) {}

		Aggregate a;
		bool b;
		Colouring c;
		double d;
		File_Format f;
		int i;
		Justification j;
		Key_Position k;
		Layering l;
		std::regex * m;
		Sorting o;
		Processors p;
		Replication r;
		Special z;
		const std::string * s;
	    } value;
	} opts;
    
    const char * msg;
    };

public:
    /* Used to search the Flags::print table for a matching option */
    class find_option {
    public:
	find_option( const int c ) : _c(c) {}
	bool operator()( Options::Type& arg ) const { return (arg.opts.param.s == &Options::result || arg.opts.param.s == &Options::boolean) && arg.c == _c; }
    private:
	const int _c;
    };

private:
    Options();
    Options( const Options& );
    Options& operator=( const Options& );

public:
    /* Discriminators */
    static const std::map<const Aggregate,const std::string> aggregate;
    static const std::map<const Colouring,const std::string> colouring;
    static const std::map<const File_Format,const std::string> file_format;
    static const std::map<const Justification,const std::string> justification;
    static const std::multimap<const Key_Position,const std::string> key_position;
    static const std::map<const Layering,const std::string> layering;
    static const std::map<const Processors,const std::string> processors;
    static const std::map<const Replication,const std::string> replication;
    static const std::map<const Sorting,const std::string> sorting;
    static const std::map<const Special,const std::string> special;
    static const std::string integer;		/* Option arg is an interger	*/
    static const std::string real;		/* Option arg is a real		*/
    static const std::string string;		/* Option arg is a string	*/
    static const std::string boolean;		/* Option is a boolean.		*/
    static const std::string none;		/* No argument			*/
    static const std::string result;		/* Option arg is a result (bool)*/

    static bool set_all_result_options( const bool yesOrNo );
    static Aggregate get_aggregate( const std::string& );
    static Colouring get_colouring( const std::string& );
    static File_Format get_file_format( const std::string& );
    static Justification get_justification( const std::string& );
    static Key_Position get_key_position( const std::string& );
    static Layering get_layering( const std::string&, std::vector<std::string>& );
    static Processors get_processors( const std::string& );
    static Replication get_replication( const std::string& );
    static Sorting get_sorting( const std::string& );
    static Special get_special( const std::string& );
};
#endif
