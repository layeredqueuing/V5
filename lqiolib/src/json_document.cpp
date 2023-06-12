/* -*- c++ -*-
 * $Id: json_document.cpp 16736 2023-06-08 16:11:47Z greg $
 *
 * Read in JSON input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * September 2013
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <cassert>
#include <cmath>
#include <regex>
#include <fcntl.h>
#include <sys/stat.h>
#if HAVE_SYS_MMAN_H
#include <sys/types.h>
#include <sys/mman.h>
#endif
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include <lqx/SyntaxTree.h>
#include <lqx/Program.h>
#include "dom_actlist.h"
#include "dom_entry.h"
#include "dom_extvar.h"
#include "dom_group.h"
#include "dom_histogram.h"
#include "dom_object.h"
#include "dom_phase.h"
#include "dom_processor.h"
#include "dom_task.h"
#include "error.h"
#include "filename.h"
#include "glblerr.h"
#include "input.h"
#include "json_document.h"
#include "srvn_results.h"
#include "srvn_spex.h"
#include "xml_input.h"

extern "C" {
#include "srvn_gram.h"
    int srvnparse_string( int start_token, const char * s );
}

namespace LQIO {
    namespace DOM {
	int JSON_Document::Import::__indent = 0;
	int JSON_Document::Export::__indent = -1;		/* suppress initial newline */

	/* ---------------------------------------------------------------- */
	/* Function objects for for_each.				    */
	/* ---------------------------------------------------------------- */

	struct AddPragma {
	    AddPragma( Document& document ) : _document(document) {}
	    void operator()( const std::pair<std::string, picojson::value>& o )
		{
		    if ( Document::__debugJSON ) std::cerr << JSON_Document::Import::indent(0) << "{ \"" << o.first << "\", \"" << o.second.to_str() << "\" }" << std::endl;
		    _document.addPragma( o.first, o.second.to_str() );
		}
	private:
	    Document& _document;
	};

	static void setPhaseName( Phase * phase, const Entry * entry, unsigned int p )
	{
	    if ( phase->getName().size() > 0 ) return;
	    assert( 0 < p && p <= 3 );
	    std::string name = entry->getName();
	    name += "_ph";
	    name += "0123"[p];
	    phase->setName( name );
	}

	static void setCallName( Call * call, const std::string src, const std::string dst )
	{
	    if ( call->getName().size() > 0 ) return;
	    std::string name = src;
	    name += '_';
	    name += dst;
	    call->setName(name);
	}

    }
}

/* -------------------------------------------------------------------- */
/* DOM input.								*/
/* -------------------------------------------------------------------- */

namespace LQIO {
    namespace DOM {

	JSON_Document::JSON_Document( Document& document, const std::string& input_file_name, bool createObjects, bool loadResults )
	    : _dom(), _document( document ), _input_file_name(input_file_name), _createObjects(createObjects), _loadResults(loadResults)
	{
	    Import::__indent = 0;
	    Export::__indent = -1;
	}


	JSON_Document::~JSON_Document()
	{
	}


	bool
	JSON_Document::load( Document& document, const std::string& input_file_name, unsigned& errorCode, const bool load_results )
	{
	    JSON_Document input( document, input_file_name, true, load_results );

	    if ( !input.parse() ) {
		return false;
	    } else {
		const std::string& program_text = document.getLQXProgramText();
		if ( program_text.size() ) {
		    /* If we have an LQX program, then we need to compute */
		    LQX::Program* program = LQX::Program::loadFromText(input_file_name.c_str(), document.getLQXProgramLineNumber(), program_text.c_str());
		    if (program == nullptr) {
			LQIO::runtime_error( LQIO::ERR_LQX_COMPILATION, input_file_name.c_str() );
		    }
		    document.setLQXProgram( program );
		}
	    }

	    return true;
	}

	/*
	 * Load results (only) from filename.  The input DOM must be present (and match iff LQNX input).
	 */

	bool
	JSON_Document::loadResults( Document& document, const std::string& input_file_name )
	{
	    JSON_Document input( document, input_file_name, false, true );
	    return input.parse();
	}


	/*
	 * Parse the XML file, catching any XML exceptions that might

	 * propogate out of it.
	 */

	bool
	JSON_Document::parse()
	{
	    struct stat statbuf;
	    bool rc = true;
	    int input_fd = -1;

	    if ( !Filename::isFileName( _input_file_name ) ) {
		input_fd = fileno( stdin );
	    } else if ( ( input_fd = open( _input_file_name.c_str(), O_RDONLY ) ) < 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot open input file " << _input_file_name << " - " << strerror( errno ) << std::endl;
		return false;
	    }

	    if ( isatty( input_fd ) ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Input from terminal is not allowed." << std::endl;
		return false;
	    } else if ( fstat( input_fd, &statbuf ) != 0 ) {
		std::cerr << LQIO::io_vars.lq_toolname << ": Cannot stat " << _input_file_name << " - " << strerror( errno ) << std::endl;
		return false;
#if defined(S_ISSOCK)
	    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) && !S_ISSOCK(statbuf.st_mode) ) {
#else
	    } else if ( !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode) ) {
#endif
		std::cerr << LQIO::io_vars.lq_toolname << ": Input from " << _input_file_name << " is not allowed." << std::endl;
		return false;
	    }

#if HAVE_MMAP
	    char *buffer = static_cast<char *>(mmap( 0, statbuf.st_size, PROT_READ, MAP_PRIVATE|MAP_FILE, input_fd, 0 ));
	    if ( buffer != MAP_FAILED ) {
		std::string err = picojson::parse( _dom, buffer, buffer + statbuf.st_size );
		if ( err.empty() ) {
		    try {
			handleModel();
		    }
		    catch ( const std::runtime_error& e ) {
			input_error( e.what() );
			rc = false;
		    }
		} else {
		    input_error( err.c_str() );
		    rc = false;
		}
		munmap( buffer, statbuf.st_size );
	    } else {
		std::cerr << LQIO::io_vars.lq_toolname << ": Read error on " << _input_file_name << " - " << strerror( errno ) << std::endl;
	    }
#else
	    size_t size = statbuf.st_size;
	    char * buffer = static_cast<char *>(malloc( size ) );
	    char * p = buffer;
	    size_t len = 0;
	    do {
	        len = read( input_fd, p, std::min( size, static_cast<size_t>(BUFSIZ) ) );	/* BUG 400 -- windows issue */
		if ( static_cast<int>(len) < 0 ) {
		    std::cerr << LQIO::io_vars.lq_toolname << ": Read error on " << _input_file_name << " - " << strerror( errno ) << std::endl;
		    rc = false;
		    break;
		}
		p += len;
		size -= len;
	    } while ( len > 0 );
	    if ( len == 0 ) {
		std::string err = picojson::parse( _dom, buffer, buffer + statbuf.st_size );
		if ( err.empty() ) {
		    try {
			handleModel();
		    }
		    catch ( const std::runtime_error& e ) {
			input_error( e.what() );
			rc = false;
		    }
		} else {
		    input_error( err.c_str() );
		    rc = false;
		}
	    }
	    free( buffer );
#endif
	    close( input_fd );
	    return rc;
	}

	void
	JSON_Document::input_error( const char * fmt, ... ) const
	{
	    va_list args;
	    va_start( args, fmt );
	    verrprintf( stderr, LQIO::error_severity::ERROR, _input_file_name.c_str(),  0, 0, fmt, args );
	    va_end( args );
	}
 
	/* ---------------------------------------------------------------- */
	/* DOM importation						    */
	/* ---------------------------------------------------------------- */

	const std::map<const char*,const JSON_Document::ImportModel,JSON_Document::ImportModel>	 JSON_Document::model_table =
	{
	    { Xcomment,		ImportModel() },	// Nop. Discard
	    { Xparameters,	ImportModel( &JSON_Document::handleParameters  ) },
	    { Xgeneral,		ImportModel( &JSON_Document::handleGeneral ) },
	    { Xprocessor,	ImportModel( &JSON_Document::handleProcessor ) },
	    { Xresults,		ImportModel( &JSON_Document::handleResults ) },
	    { Xconvergence,	ImportModel( &JSON_Document::handleConvergence ) },
	    { "#",		ImportModel() }		// Nop. Discard
	};


	void
	JSON_Document::handleModel()
	{
	    if ( !_dom.is<picojson::object>() ) throw std::runtime_error( "JSON object expected" );
	    const picojson::value::object& obj = _dom.get<picojson::object>();
	    Spex::__parameter_list = spex_list( nullptr, nullptr );

	    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		const std::string& attr = i->first;
		const std::map<const char *,const ImportModel>::const_iterator j = model_table.find( attr.c_str() );
		if ( j == model_table.end() ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, "lqn-model", attr.c_str() );
		} else {
		    j->second( attr, *this, i->second );
		}
	    }

	    /*
	     * Parameters, Results and Convergence call the SRVN parser.  For Json, these variables are set
	     * so we need to set up the program after the model is loaded.
	     */

	    spex_set_program( Spex::__parameter_list, Spex::__result_list, Spex::__convergence_list );
	}

	void
	JSON_Document::handleComment( DocumentObject *, const picojson::value& value )
	{
	}

	void
	JSON_Document::handleParameters( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		std::string program;
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) std::cerr << Import::indent(0) << i->to_str() << std::endl;
		    if ( i->is<std::string>() ) {
			program += i->get<std::string>();
			program += "\n";
		    } else {
			XML::invalid_argument( Xparameters, i->to_str() );
		    }
		}
		srvnparse_string( SPEX_PARAMETER, program.c_str() );
	    } else {
		XML::invalid_argument( Xparameters, value.to_str() );
	    }
	}

	void
	JSON_Document::handlePragma( DocumentObject *, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handlePragma( 0, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const picojson::value::object& obj = value.get<picojson::object>();
		std::for_each( obj.begin(), obj.end(), AddPragma( _document ) );
	    } else {
		XML::invalid_argument( Xpragma, value.to_str() );
	    }
	}

	const std::map<const char*,const JSON_Document::ImportGeneral,JSON_Document::ImportGeneral>  JSON_Document::general_table =
	{
	    { Xconv_val,	    ImportGeneral( &Document::setModelConvergence ) },
	    { Xcomment,		    ImportGeneral( &Document::setModelComment ) },
	    { Xit_limit,	    ImportGeneral( &Document::setModelIterationLimit ) },
	    { Xunderrelax_coeff,    ImportGeneral( &Document::setModelUnderrelaxationCoefficient ) },
	    { Xprint_int,	    ImportGeneral( &Document::setModelPrintInterval ) },
	    { Xobserve,		    ImportGeneral( &JSON_Document::handleGeneralObservation ) },	// SPEX
	    { Xpragma,		    ImportGeneral( &JSON_Document::handlePragma ) },
	    { Xresults,		    ImportGeneral( &JSON_Document::handleGeneralResult ) },
	    { "#",		    ImportGeneral( &Document::setModelComment ) }
	};

	void
	JSON_Document::handleGeneral( DocumentObject *, const picojson::value& value )
	{
	    if ( !value.is<picojson::object>() ) {
		XML::invalid_argument( Xgeneral, value.to_str() );
	    }
	    const picojson::value::object& obj = value.get<picojson::object>();
	    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		const std::string& attr = i->first;
		const std::map<const char *,const ImportGeneral>::const_iterator j = general_table.find(attr.c_str());
		if ( j != general_table.end() ) {
		    j->second(attr,*this,_document,i->second);		/* Handle attribute */
		} else {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xgeneral, attr.c_str() );
		}
	    }
	    const int iterations = _document.getResultIterations();
	    if ( iterations > 0 ) {
		const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( iterations );
	    }
	}

	const std::map<const char*,const JSON_Document::ImportGeneralObservation,JSON_Document::ImportGeneralObservation>  JSON_Document::general_observation_table =
	{
	    { Xiterations,	ImportGeneralObservation( KEY_ITERATIONS ) },
	    { Xwaiting,		ImportGeneralObservation( KEY_WAITING ) },		// Waits (overloaded)
	    { Xservice_time,	ImportGeneralObservation( KEY_SERVICE_TIME ) },		// Steps (overloaded)
	    { Xsystem_cpu_time,	ImportGeneralObservation( KEY_SYSTEM_TIME ) },
	    { Xuser_cpu_time,	ImportGeneralObservation( KEY_USER_TIME ) },
	    { Xelapsed_time,	ImportGeneralObservation( KEY_ELAPSED_TIME ) }
	};

	void
	JSON_Document::handleGeneralObservation( DocumentObject *, const picojson::value& value )
	{
	    if ( !value.is<picojson::object>() ) {
		XML::invalid_argument( Xgeneral, value.to_str() );
	    }
	    const picojson::value::object& obj = value.get<picojson::object>();
	    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		const std::string& attr = i->first;
		const std::map<const char *,const ImportGeneralObservation>::const_iterator j = general_observation_table.find(attr.c_str());
		if ( j != general_observation_table.end() ) {
		    j->second(attr,*this,_document,i->second);		/* Handle attribute */
		} else {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xgeneral, attr.c_str() );
		}
	    }
	}

	const std::map<const char*,const JSON_Document::ImportProcessor,JSON_Document::ImportProcessor>	 JSON_Document::processor_table =
	{
	    { Xname,		ImportProcessor() },		// Nop.	 Handled specially
	    { Xscheduling,	ImportProcessor() },		// Nop.	 Handled specially
	    { Xcomment,		ImportProcessor( &DocumentObject::setComment ) },
	    { Xquantum,		ImportProcessor( &Processor::setQuantum ) },
	    { Xmultiplicity,	ImportProcessor( &Processor::setCopies ) },
	    { Xreplication,	ImportProcessor( &Processor::setReplicas ) },
	    { Xspeed_factor,	ImportProcessor( &Processor::setRate ) },
	    { Xobserve,		ImportProcessor( &JSON_Document::handleObservation ) },	// SPEX
	    { Xresults,		ImportProcessor( &JSON_Document::handleResult ) },
	    { Xtask,		ImportProcessor( &JSON_Document::handleTask ) },
	    { Xgroup,		ImportProcessor( &JSON_Document::handleGroup ) },
	    { "#",		ImportProcessor( &DocumentObject::setComment ) }
	};

	void
	JSON_Document::handleProcessor( DocumentObject *, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handleProcessor( 0, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& processor_name = get_string_attribute( Xname, obj );
		    Processor * processor = _document.getProcessorByName( processor_name );
		    if ( _createObjects ) {
			if ( processor ) {
			    throw duplicate_symbol( processor_name );
			}
			processor = new Processor( &_document, processor_name.c_str(), get_scheduling_attribute( obj, SCHEDULE_PS ) );
			_document.addProcessorEntity( processor );

			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    const std::map<const char *,const ImportProcessor>::const_iterator j = processor_table.find(attr.c_str());
			    if ( j == processor_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprocessor, attr.c_str() );
			    } else {
				j->second(attr,*this,*processor,i->second);	/* Handle attribtue */
			    }
			}

			const scheduling_type scheduling_flag = processor->getSchedulingType();
			if ( !processor->hasQuantum() ) {
			    if ( scheduling_flag == SCHEDULE_CFS 
				 || scheduling_flag == SCHEDULE_PS ) {
				processor->runtime_error( LQIO::ERR_NO_QUANTUM_SCHEDULING, scheduling_label.at(scheduling_flag).str.c_str() );
			    }
			} else if ( scheduling_flag == SCHEDULE_DELAY
				    || scheduling_flag == SCHEDULE_FIFO
				    || scheduling_flag == SCHEDULE_HOL
				    || scheduling_flag == SCHEDULE_PPR
				    || scheduling_flag == SCHEDULE_RAND ) {
			    processor->runtime_error( LQIO::WRN_QUANTUM_SCHEDULING, scheduling_label.at(scheduling_flag).str.c_str() );
			}
		    } else if ( !processor ) {
			LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, processor_name.c_str() );
		    } else {
			handleResults( processor, obj );
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xprocessor, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xprocessor, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprocessor, arg.what() );
		}
	    } else {
		XML::invalid_argument( Xprocessor, value.to_str() );
	    }
	}


	const std::map<const char*,const JSON_Document::ImportGroup,JSON_Document::ImportGroup>	 JSON_Document::group_table =
	{
	    { Xname,		ImportGroup() },		// Nop.	 Handled specially
	    { Xcomment,		ImportGroup( &DocumentObject::setComment ) },
	    { Xprocessor,	ImportGroup( &Group::setProcessor ) },
	    { Xshare,		ImportGroup( &Group::setGroupShare ) },
	    { Xcap,		ImportGroup( &Group::setCap ) },
	    { Xobserve,		ImportGroup( &JSON_Document::handleObservation ) },	// SPEX
	    { Xresults,		ImportGroup( &JSON_Document::handleResult ) },
	    { Xtask,		ImportGroup( &JSON_Document::handleTask ) },
	    { "#",		ImportGroup( &DocumentObject::setComment ) },
	};

	void
	JSON_Document::handleGroup( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handleGroup( parent, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& group_name = get_string_attribute( Xname, obj );
		    Group * group = _document.getGroupByName( group_name );
		    if ( _createObjects ) {
			if ( group ) {
			    throw duplicate_symbol( group_name );
			}
			Processor * processor = dynamic_cast<Processor *>(parent);
			assert( processor != nullptr );
			group = new Group( &_document, group_name.c_str(), processor );
			_document.addGroup( group );
			processor->addGroup( group );

			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    const std::map<const char *,const ImportGroup>::const_iterator j = group_table.find(attr.c_str());
			    if ( j == group_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xgroup, attr.c_str() );
			    } else {
				j->second(attr,*this,*group,i->second);	/* Handle attribtue */
			    }
			}
		    } else if ( !group ) {
			LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, group_name.c_str() );
		    } else {
			handleResults( group, obj );
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xgroup, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xgroup, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xgroup, arg.what() );
		}
	    } else {
		XML::invalid_argument( Xgroup, value.to_str() );
	    }
	}

	const std::map<const char*,const JSON_Document::ImportTask,JSON_Document::ImportTask>  JSON_Document::task_table =
	{
	    { Xname,		ImportTask() },			// Nop.	 Handled specially
	    { Xscheduling,	ImportTask() },			// Nop.	 Handled specially
	    { Xcomment,		ImportTask( &DocumentObject::setComment ) },
	    { Xfanin,		ImportTask( &JSON_Document::handleFanInOut, &Task::setFanIn ) },
	    { Xfanout,		ImportTask( &JSON_Document::handleFanInOut, &Task::setFanOut ) },
	    { Xprocessor,	ImportTask( &Task::setProcessor ) },
	    { Xgroup,		ImportTask( &Task::setGroup ) },
	    { Xmultiplicity,	ImportTask( &Task::setCopies ) },
	    { Xreplication,	ImportTask( &Task::setReplicas ) },
	    { Xthink_time,	ImportTask( &Task::setThinkTime ) },
	    { Xpriority,	ImportTask( &Task::setPriority ) },
	    { Xactivity,	ImportTask() },			// Nop.	 Handled specially
	    { Xentry,		ImportTask( &JSON_Document::handleEntry ) },
	    { Xprecedence,	ImportTask() },			// Nop.	 Handled specially
	    { Xobserve,		ImportTask( &JSON_Document::handleObservation ) },	// SPEX
	    { Xresults,		ImportTask( &JSON_Document::handleResult ) },
	    { "#",		ImportTask( &DocumentObject::setComment ) },
	};
	void
	JSON_Document::handleTask( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handleTask( parent, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& task_name = get_string_attribute( Xname, obj );
		    Task * task = _document.getTaskByName( task_name );
		    if ( _createObjects ) {
			if ( task ) {
			    throw duplicate_symbol( task_name );
			}
			std::vector<Entry *> entries;		/* Add list later */
			Processor * processor = 0;
			Group * group = dynamic_cast<Group *>(parent);
			if ( group ) {
			    processor = const_cast<Processor *>(group->getProcessor());
			} else {
			    processor = dynamic_cast<Processor *>(parent);
			}
			assert( processor );
			task = new Task( &_document, task_name.c_str(), get_scheduling_attribute( obj, SCHEDULE_FIFO ), entries, processor );
			processor->addTask(task);
			if ( group ) {
			    group->addTask( task );
			    task->setGroup( group );
			}

			_document.addTaskEntity(task);

			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    const std::map<const char *,const ImportTask>::const_iterator j = task_table.find(attr.c_str());
			    if ( j == task_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xtask, attr.c_str() );
			    } else {
				j->second(attr,*this,*task,i->second);	/* Handle attribtue */
			    }
			}
			const scheduling_type scheduling_flag = task->getSchedulingType();
			if ( task->hasThinkTime() && scheduling_flag != SCHEDULE_CUSTOMER ) {
			    task->runtime_error( LQIO::ERR_NON_REF_THINK_TIME );
			}

			/* Do activities and precedences last so that entries are defined */
			if ( has_attribute( Xactivity, obj ) ) {
			    handleActivity( task, get_value_attribute( Xactivity, obj ) );
			}
			if ( has_attribute( Xprecedence, obj ) ) {
			    handlePrecedence( task, get_value_attribute( Xprecedence, obj ) );
			}

		    } else if ( !task ) {
			LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, task_name.c_str() );
		    } else {
			handleResults( task, obj );
		    }
		}
		catch ( const undefined_symbol& attr ) {
		    LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, attr.what() );
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xtask, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xtask, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xtask, arg.what() );
		}
	    } else {
		XML::invalid_argument( Xtask, value.to_str() );
	    }
	}


	void
	JSON_Document::handleFanInOut( DocumentObject * parent, const picojson::value& value, fan_in_out_fptr fan_in_out )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handleFanInOut( parent, *i, fan_in_out );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const picojson::value::object& obj = value.get<picojson::object>();
		picojson::value::object::const_iterator i = obj.begin();
		if ( obj.size() == 1 ) {
		    if ( Document::__debugJSON ) Import::printIndent(std::cerr,0) << "{ " << i->first << ": " << i->second.to_str() << " }" << std::endl;
		    Task * task = dynamic_cast<Task *>(parent);
		    if ( task ) {
			(task->*fan_in_out)( i->first, get_external_variable( i->second ) );
		    } else {
			XML::invalid_argument( Xfanin, value.to_str() );
		    }
		} else if ( obj.size() > 1 ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprecedence, (++i)->first.c_str() );
		}
	    } else {
		XML::invalid_argument( Xentry, value.to_str() );
	    }
	}

	const std::map<const char*,const JSON_Document::ImportEntry,JSON_Document::ImportEntry> JSON_Document::entry_table =
	{
	    { Xname,		    ImportEntry() },
	    { Xstart_activity,	    ImportEntry() },		// Nop.	 Handled specially
	    { Xcomment,		    ImportEntry( &DocumentObject::setComment ) },
	    { Xphase,		    ImportEntry( &JSON_Document::handlePhase ) },
	    { Xcoeff_of_var_sq,	    ImportEntry( &Phase::setCoeffOfVariationSquared ) },
	    { Xdeterministic,	    ImportEntry( &Phase::setPhaseTypeFlag ) },
	    { Xopen_arrival_rate,   ImportEntry( &Entry::setOpenArrivalRate ) },
	    { Xservice_time,	    ImportEntry( &Phase::setServiceTime ) },
	    { Xthink_time,	    ImportEntry( &Phase::setThinkTime ) },
	    { Xforwarding,	    ImportEntry( &JSON_Document::handleCall, Call::Type::FORWARD ) },
	    { Xhistogram,	    ImportEntry( &JSON_Document::handleHistogram ) },
	    { Xmax_service_time,    ImportEntry( &JSON_Document::handleMaxServiceTime ) },
	    { Xobserve,		    ImportEntry( &JSON_Document::handleObservation ) },	// SPEX
	    { Xresults,		    ImportEntry( &JSON_Document::handleResult ) },
	    { "#",		    ImportEntry( &DocumentObject::setComment ) }
	};

	void
	JSON_Document::handleEntry( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handleEntry( parent, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& entry_name = get_string_attribute( Xname, obj );
		    Entry * entry = _document.getEntryByName( entry_name.c_str() );
		    if ( _createObjects ) {
			if ( !entry ) {
			    entry = new Entry( &_document, entry_name );
			    _document.addEntry(entry);		  /* Add to global table */
			    connectEntry( entry, dynamic_cast<Task *>(parent), entry_name );
			} else if ( entry->getEntryType() == Entry::Type::NOT_DEFINED ) {
			    connectEntry( entry, dynamic_cast<Task *>(parent), entry_name );
			} else {
			    throw duplicate_symbol( entry_name );
			}

			/* If I have a start activity, then set it.  Do this first for properly handling the remaining attributes. */

			picojson::value::object::const_iterator i = obj.find(Xstart_activity);
			if ( i != obj.end() ) {
			    if ( i->second.is<std::string>() ) {
				const std::string& name = i->second.get<std::string>();
				Task * task = const_cast<Task *>(entry->getTask());
				assert( task != nullptr );
				Activity * activity = task->getActivity( name.c_str(), true );
				entry->setStartActivity( activity );
				_document.db_check_set_entry(entry, Entry::Type::ACTIVITY);
			    } else {
				XML::invalid_argument( Xstart_activity, i->second.to_str() );
			    }
			} else {
			    _document.db_check_set_entry(entry, Entry::Type::STANDARD);
			}

			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    const std::map<const char *,const ImportEntry>::const_iterator j = entry_table.find(attr.c_str());
			    if ( j == entry_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xentry, attr.c_str() );
			    } else if ( i->second.is<picojson::array>() ) {		/* Phase or forwarding list */
				if ( attr == Xphase ) {
				    /* If I have Xphase, then I am a standard entry.  Forwarding is also an array. */
				    _document.db_check_set_entry(entry, Entry::Type::STANDARD);
				}
				if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr, i->second );
				const picojson::value::array& arr = i->second.get<picojson::array>();
				unsigned int p_j = 0;		/* Dest array index */
				for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x ) {
				    if ( j->second.getType() == dom_type::JSON_OBJECT ) {
					j->second(attr,true,*this,*entry,*x);					/* Handle array of attributes such as calls or phases */
				    } else {
					p_j += 1;
					if ( x->is<picojson::null>() || (x->is<double>() && x->get<double>() == 0) ) continue;		/* Don't create null phases */
					Phase* phase = entry->getPhase(p_j);
					setPhaseName( phase, entry, p_j );
					j->second(attr,*this,*phase,*x);					/* Handle parameter argument */
				    }
				}
				if ( Document::__debugJSON ) Import::endAttribute( std::cerr, attr, i->second );
			    } else {
				j->second(attr,false,*this,*entry,i->second);					/* Handle attribute */
			    }
			}
		    } else if ( !entry ) {
			throw undefined_symbol( entry_name );
		    } else {
			if ( entry->getTask() == nullptr )  {
			    entry->setTask(dynamic_cast<Task *>(parent));
			}
			handleResults( entry, obj );
		    }
		}
		catch ( const undefined_symbol& attr ) {
		    LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, attr.what() );
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xentry, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xentry, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xentry, arg.what() );
		}
	    } else {
		XML::invalid_argument( Xentry, value.to_str() );
	    }
	}


	const std::map<const char*,const JSON_Document::ImportPhase,JSON_Document::ImportPhase> JSON_Document::phase_table =
	{
	    { Xphase,		ImportPhase() },
	    { Xmax_service_time,ImportPhase( &Phase::setMaxServiceTime ) },
	    { Xcomment,		ImportPhase( &DocumentObject::setComment ) },
	    { Xservice_time,	ImportPhase( &Phase::setServiceTime ) },
	    { Xcoeff_of_var_sq,	ImportPhase( &Phase::setCoeffOfVariationSquared ) },
	    { Xthink_time,	ImportPhase( &Phase::setThinkTime ) },
	    { Xdeterministic,	ImportPhase( &Phase::setPhaseTypeFlag ) },
	    { Xsynch_call,	ImportPhase( &JSON_Document::handleCall, Call::Type::RENDEZVOUS ) },
	    { Xasynch_call,	ImportPhase( &JSON_Document::handleCall, Call::Type::SEND_NO_REPLY ) },
	    { Xhistogram,	ImportPhase( &JSON_Document::handleHistogram ) },
	    { Xobserve,		ImportPhase( &JSON_Document::handleObservation ) },	// SPEX
	    { Xresults,		ImportPhase( &JSON_Document::handleResult ) },
	    { "#",		ImportPhase( &DocumentObject::setComment ) }
	};

	void
	JSON_Document::handlePhase( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handlePhase( parent, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		Entry& entry = *dynamic_cast<Entry *>(parent);
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    // Need to get the phase number, and create the phase if necessary.
		    if ( _createObjects ) {
			const unsigned int p = get_opt_phase( obj );
			if ( p == 0 ) throw XML::missing_attribute( Xphase );
			Phase * phase = entry.getPhase(p);
			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    const std::map<const char *,const ImportPhase>::const_iterator j = phase_table.find(attr.c_str());
			    if ( j == phase_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xphase, attr.c_str() );
			    } else if ( i->second.is<picojson::array>() ) {
				const picojson::value::array& arr = i->second.get<picojson::array>();
				for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x ) {
				    j->second(attr,*this,*phase,*x);		/* Handle attribute */
				}
			    } else {
				j->second(attr,*this,*phase,i->second);		/* Handle attribute */
			    }
			}
			/* Set a default name (for petrisrvn...) */
			if ( phase->getName().size() == 0 ) {
			    std::string name = entry.getName();
			    name += "_ph";
			    name += "0123"[p];
			    phase->setName( name );
			}
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xphase, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xphase, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xphase, arg.what() );
		}
	    } else {
		XML::invalid_argument( Xphase, value.to_str() );
	    }
	}

	const std::map<const char*,const JSON_Document::ImportTaskActivity,JSON_Document::ImportTaskActivity> JSON_Document::task_activity_table =
	{
	    { Xtask,		ImportTaskActivity() },
	    { Xactivity,	ImportTaskActivity( &JSON_Document::handleActivity ) },
	    { Xprecedence,	ImportTaskActivity( &JSON_Document::handlePrecedence ) }
	};

	void
	JSON_Document::handleTaskActivity( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handleTaskActivity( parent, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		/* Find task... */
		try {
		    const std::string& attr = get_string_attribute( Xtask, obj );
		    Task * task = _document.getTaskByName( attr.c_str() );
		    if ( !task ) {
			LQIO::input_error( LQIO::ERR_NOT_DEFINED, attr.c_str() );
		    } else {
			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    const std::map<const char *,const ImportTaskActivity>::const_iterator j = task_activity_table.find(attr.c_str());
			    if ( j == task_activity_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xactivity, attr.c_str() );
			    } else {
				j->second(attr,*this,task,i->second);		/* Handle attribute */
			    }
			}
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xactivity, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xactivity, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xactivity, arg.what() );
		}
	    } else {
		XML::invalid_argument( Xactivity, value.to_str() );
	    }
	}


	const std::map<const char*,const JSON_Document::ImportActivity,JSON_Document::ImportActivity> JSON_Document::activity_table =
	{
	    { Xname,		ImportActivity() },
	    { Xmax_service_time,ImportActivity( &Phase::setMaxServiceTime ) },
	    { Xcomment,		ImportActivity( &DocumentObject::setComment ) },
	    { Xservice_time,	ImportActivity( &Phase::setServiceTime ) },
	    { Xcoeff_of_var_sq,	ImportActivity( &Phase::setCoeffOfVariationSquared ) },
	    { Xthink_time,	ImportActivity( &Phase::setThinkTime ) },
	    { Xdeterministic,	ImportActivity( &Phase::setPhaseTypeFlag ) },
	    { Xsynch_call,	ImportActivity( &JSON_Document::handleCall, Call::Type::RENDEZVOUS ) },
	    { Xasynch_call,	ImportActivity( &JSON_Document::handleCall, Call::Type::SEND_NO_REPLY ) },
	    { Xreply_to,	ImportActivity( &JSON_Document::handleReplyList ) },
	    { Xhistogram,	ImportActivity( &JSON_Document::handleHistogram ) },
	    { Xobserve,		ImportActivity( &JSON_Document::handleObservation ) },	// SPEX
	    { Xresults,		ImportActivity( &JSON_Document::handleResult ) },
	    { "#",		ImportActivity( &DocumentObject::setComment ) }
	};

	void
	JSON_Document::handleActivity( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handleActivity( parent, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() && dynamic_cast<Task *>(parent) ) {
		Task& task = *dynamic_cast<Task *>(parent);
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& activity_name = get_string_attribute( Xname, obj );
		    Activity * activity = task.getActivity( activity_name, _createObjects );
		    if ( _createObjects ) {
			if ( activity->isSpecified() ) {
			    throw duplicate_symbol( activity_name );
			}
			activity->setIsSpecified(true);
			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    const std::map<const char *,const ImportActivity>::const_iterator j = activity_table.find(attr.c_str());
			    if ( j == activity_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xactivity, attr.c_str() );
			    } else if ( i->second.is<picojson::array>() ) {
				const picojson::value::array& arr = i->second.get<picojson::array>();
				for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x ) {
				    j->second(attr,*this,*activity,*x);			/* Handle attribute */
				}
			    } else {
				j->second(attr,*this,*activity,i->second);		/* Handle attribute */
			    }
			}
		    } else if ( !activity ) {
			LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, activity_name.c_str() );
		    } else {
			handleResults( activity, obj );
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xactivity, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xactivity, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xactivity, arg.what() );
		}
	    } else {
		XML::invalid_argument( Xactivity, value.to_str() );
	    }
	}


	const std::map<const char*,const JSON_Document::ImportPrecedence,JSON_Document::ImportPrecedence> JSON_Document::precedence_table =
	{
	    { Xpre,		ImportPrecedence(ActivityList::Type::JOIN) },
	    { Xpost,		ImportPrecedence(ActivityList::Type::FORK) },
	    { Xand_join,	ImportPrecedence(ActivityList::Type::AND_JOIN) },
	    { Xand_fork,	ImportPrecedence(ActivityList::Type::AND_FORK) },
	    { Xor_join,		ImportPrecedence(ActivityList::Type::OR_JOIN) },
	    { Xor_fork,		ImportPrecedence(ActivityList::Type::OR_FORK) },
	    { Xloop,		ImportPrecedence(ActivityList::Type::REPEAT) },
	    { Xquorum,		ImportPrecedence() },
	    { Xresults,		ImportPrecedence() },
	    { Xhistogram,	ImportPrecedence() }
	};

	void
	JSON_Document::handlePrecedence( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *i );
		    handlePrecedence( parent, *i );
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *i );
		}
	    } else if ( value.is<picojson::object>() && dynamic_cast<Task *>(parent) ) {
		Task& task = *dynamic_cast<Task *>(parent);
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		ActivityList * pre_list = 0;		/* Need this for quorum/results of join */
		ActivityList * post_list = 0;
		std::map<const char *,picojson::value> deferred;	/* For deferred attributes */
		try {
		    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			const std::string& attr = i->first;
			const std::map<const char *,const ImportPrecedence>::const_iterator j = precedence_table.find(attr.c_str());
			if ( j == precedence_table.end() ) {
			    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprecedence, attr.c_str() );
			} else if ( i->second.is<picojson::array>() ) {
			    if ( _createObjects ) {
				if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, i->first, i->second );
				ActivityList * precedence;
				if ( j->second.precedence_type() == ActivityList::Type::AND_JOIN ) {
				    precedence = new AndJoinActivityList( &_document, &task, /* quorum count */ 0 );		// set by Import
				} else {
				    precedence = new ActivityList( &_document, &task, j->second.precedence_type() );
				}
				if ( precedence->isJoinList() ) {
				    if ( pre_list ) {
					LQIO::runtime_error( LQIO::ERR_DUPLICATE_SYMBOL, "Precedence", attr.c_str() );
				    } else {
					pre_list = precedence;
				    }
				} else {
				    if ( post_list ) {
					LQIO::runtime_error( LQIO::ERR_DUPLICATE_SYMBOL, "Precedence", attr.c_str() );
				    } else {
					post_list = precedence;
				    }
				}
				const picojson::value::array& arr = i->second.get<picojson::array>();
				for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x) {
				    j->second(attr,*this,*precedence,*x);		/* Handle attribute */
				}
				if ( Document::__debugJSON ) Import::endAttribute( std::cerr, i->first, i->second );
			    }
			} else if ( i->second.is<double>() || i->second.is<std::string>() || i->second.is<picojson::object>() ) {
			    deferred[j->first] = i->second;
			} else {
			    XML::invalid_argument( attr, i->second.to_str() );
			}
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xprecedence, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xprecedence, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprecedence, arg.what() );
		}

		/* Deferred op -- set quorum */
		for ( std::map<const char *,picojson::value>::const_iterator i = deferred.begin(); i != deferred.end(); ++i ) {
		    if ( dynamic_cast<AndJoinActivityList *>(pre_list) == nullptr ) {
			LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprecedence, i->first );
		    } else if ( i->first == Xquorum && i->second.is<double>() && _createObjects ) {
			dynamic_cast<AndJoinActivityList *>(pre_list)->setQuorumCountValue( static_cast<int>(i->second.get<double>()) );
		    } else if ( i->first == Xquorum && i->second.is<std::string>() && _createObjects ) {
			dynamic_cast<AndJoinActivityList *>(pre_list)->setQuorumCount( _document.db_build_parameter_variable( i->second.get<std::string>().c_str(), 0 ) );
		    } else if ( i->first == Xresults && i->second.is<picojson::object>() ) {
			handleResult( pre_list, i->second );	/* Result goes with join */
		    } else if ( i->first == Xhistogram && i->second.is<picojson::object>() ) {
			handleHistogram( pre_list, i->second );
		    } else {
			LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprecedence, i->first );
		    }
		}

		/* Connect the pre-list (first) to the post-list (second) */
		if ( pre_list ) {
		    pre_list->setNext( post_list );
		}
		if ( post_list ) {
		    post_list->setPrevious( pre_list );
		}
	    } else {
		XML::invalid_argument( Xprecedence, value.to_str() );
	    }
	}

	const std::map<const char*,const JSON_Document::ImportCall,JSON_Document::ImportCall> JSON_Document::call_table =
	{
	    { Xdestination,	ImportCall() },
	    { Xmean_calls,	ImportCall( &Call::setCallMean ) },
	    { Xresults,		ImportCall( &JSON_Document::handleResult ) },
	    { Xobserve,		ImportCall( &JSON_Document::handleObservation ) },	// SPEX
	};

	const std::map<const Call::Type,const std::string> JSON_Document::call_type_table =
	{
	    { Call::Type::NULL_CALL,		"Null" },
	    { Call::Type::SEND_NO_REPLY,	JSON_Document::Xasynch_call },
	    { Call::Type::RENDEZVOUS,		JSON_Document::Xsynch_call },
	    { Call::Type::FORWARD,		JSON_Document::Xforwarding }
	};

	void
	JSON_Document::handleCall( DocumentObject * parent, const picojson::value& value, Call::Type call_type )
	{
	    if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    const std::string& destination_name = get_string_attribute( Xdestination, obj );
		    Entry * destination = _document.getEntryByName( destination_name );
		    if ( !destination ) {
			if ( _createObjects ) {
			    destination = new Entry( &_document, destination_name.c_str() );
			    _document.addEntry( destination );
			} else {
			    throw undefined_symbol( destination_name );
			}
		    }
		    _document.db_check_set_entry( destination );

		    /* Get the call */
		    Call * call = 0;
		    if ( dynamic_cast<Phase *>(parent) ) {
			Phase * source = dynamic_cast<Phase *>(parent);
			call = source->getCallToTarget( destination );
			if ( !call &&  _createObjects ) {		/* Skip nulls as we don't want to create things */
			    call = new Call( &_document, call_type, source, destination );
			    setCallName( call, source->getName(), destination_name );
			    source->addCall( call );
			}
		    } else if ( dynamic_cast<Entry *>(parent) ) {
			Entry * source = dynamic_cast<Entry *>(parent);
			call = source->getForwardingToTarget( destination );
			if ( !call &&  _createObjects ) {
			    call = new Call( &_document, source, destination );
			    setCallName( call, source->getName(), destination_name );
			    source->addForwardingCall( call );
			}
		    } else {
			abort();
		    }

		    /* Get remainder of attributes */

		    for ( picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i ) {
			const std::string& attr = i->first;
			if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr, i->second );
			const std::map<const char *,const ImportCall>::const_iterator j = call_table.find(attr.c_str());
			if ( j == call_table.end() ) {
			    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, call_type_table.at(call_type).c_str(), attr.c_str() );
			} else {
			    j->second( attr, *this, *call, i->second );
			}
			if ( Document::__debugJSON ) Import::endAttribute( std::cerr, attr, i->second );
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, call_type_table.at(call_type).c_str(), attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, call_type_table.at(call_type).c_str(), arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, call_type_table.at(call_type).c_str(), arg.what() );
		}
	    } else {
		XML::invalid_argument( call_type_table.at(call_type).c_str(), value.to_str() );
	    }
	}

	void
	JSON_Document::handleReplyList( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    handleReplyList( parent, *i );
		}
	    } else if ( value.is<std::string>() && dynamic_cast<Activity *>(parent) ) {
		Activity * activity = dynamic_cast<Activity *>(parent);
		const std::string& name = value.get<std::string>();
		if ( Document::__debugJSON ) Import::printIndent(std::cerr, 0) << name << std::endl;
		Entry * entry = _document.getEntryByName( name );
		if ( !entry ) {
		    throw undefined_symbol( name );
		}
		if ( _createObjects ) {
		    activity->getReplyList().push_back(entry);
		}
	    } else {
		XML::invalid_argument( Xreply_to, value.to_str() );
	    }
	}


	void
	JSON_Document::handleResults( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		std::string program;
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) {
			std::cerr << Import::indent(0) << i->to_str() << std::endl;
		    }
		    if ( i->is<std::string>() ) {
			program += i->get<std::string>();
			program += "\n";
		    } else {
			XML::invalid_argument( Xresults, i->to_str() );
		    }
		}
		srvnparse_string( SPEX_RESULT, program.c_str() );
	    } else {
		XML::invalid_argument( Xresults, value.to_str() );
	    }
	}

	void
	JSON_Document::handleConvergence( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() ) {
		std::string program;
		const picojson::value::array& arr = value.get<picojson::array>();
		for (picojson::value::array::const_iterator i = arr.begin(); i != arr.end(); ++i) {
		    if ( Document::__debugJSON ) {
			std::cerr << Import::indent(0) << i->to_str() << std::endl;
		    }
		    if ( i->is<std::string>() ) {
			program += i->get<std::string>();
			program += "\n";
		    } else {
			XML::invalid_argument( Xconvergence, i->to_str() );
		    }
		}
		srvnparse_string( SPEX_CONVERGENCE, program.c_str() );
	    } else {
		XML::invalid_argument( Xconvergence, value.to_str() );
	    }
	}

	/* -------------------------- results ------------------------- */

	const std::map<const char*,const JSON_Document::ImportGeneralResult,JSON_Document::ImportGeneralResult> JSON_Document::general_result_table =
	{
	    { Xconv_val_result, ImportGeneralResult( &Document::setResultConvergenceValue ) },
	    { Xelapsed_time,	ImportGeneralResult( dom_type::CLOCK, &Document::setResultElapsedTime) },
	    { Xiterations,	ImportGeneralResult( &Document::setResultIterations ) },
	    { Xmax_rss,		ImportGeneralResult( &Document::setResultMaxRSS ) },
	    { Xmva_info,	ImportGeneralResult( &JSON_Document::handleMvaInfo ) },
	    { Xplatform_info,	ImportGeneralResult( &Document::setResultPlatformInformation ) },
	    { Xsolver_info,	ImportGeneralResult( &Document::setResultSolverInformation ) },
	    { Xsystem_cpu_time, ImportGeneralResult( dom_type::CLOCK, &Document::setResultSysTime) },
	    { Xuser_cpu_time,	ImportGeneralResult( dom_type::CLOCK, &Document::setResultUserTime) },
	    { Xvalid,		ImportGeneralResult( &Document::setResultValid ) }
	};

	void
	JSON_Document::handleGeneralResult( DocumentObject * parent, const picojson::value& value )
	{
	    _document.setInstantiated( true );		/* Set true even if we aren't loading results */

	    if ( !_loadResults ) return;

	    if ( !value.is<picojson::object>() ) {
		XML::invalid_argument( Xresults, value.to_str() );	/* throws... */
	    }
	    const picojson::value::object& obj = value.get<picojson::object>();
	    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		const std::string& attr = i->first;
		try {
		    const std::map<const char *,const ImportGeneralResult>::const_iterator j = general_result_table.find(attr.c_str());
		    if ( j != general_result_table.end() ) {
			j->second(attr,*this,_document,i->second);		/* Handle attribute */
		    } else {
			LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xresults, attr.c_str() );
		    }
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xresults, arg.what() );
		}
	    }
	}

	void
	JSON_Document::handleMvaInfo( Document * document, const picojson::value& value )
	{
	    if ( !value.is<picojson::object>() ) {
		XML::invalid_argument( Xresults, value.to_str() );	/* throws... */
	    }
	    const picojson::value::object& obj = value.get<picojson::object>();

	    try {
		document->setMVAStatistics( get_long_attribute( Xsubmodels, obj ),
					    get_long_attribute( Xcore, obj ),
					    get_double_attribute( Xstep, obj ),
					    get_double_attribute( Xstep_squared, obj ),
					    get_double_attribute( Xwait, obj ),
					    get_double_attribute( Xwait_squared, obj ),
					    get_long_attribute( Xfaults, obj ) );
	    }
	    catch ( const XML::missing_attribute& e ) {
	    }
	}

	const std::map<const char*,const JSON_Document::ImportObservation,JSON_Document::ImportObservation>  JSON_Document::observation_table =
	{
	    { Xthroughput,		ImportObservation( KEY_THROUGHPUT ) },
	    { Xproc_utilization,	ImportObservation( KEY_PROCESSOR_UTILIZATION ) },
	    { Xproc_waiting,		ImportObservation( KEY_PROCESSOR_WAITING ) },
	    { Xservice_time,		ImportObservation( KEY_SERVICE_TIME ) },
	    { Xthroughput,		ImportObservation( KEY_THROUGHPUT ) },
	    { Xthroughput_bound,	ImportObservation( KEY_THROUGHPUT_BOUND ) },
	    { Xutilization,		ImportObservation( KEY_UTILIZATION ) },
	    { Xservice_time_variance,	ImportObservation( KEY_VARIANCE ) },
	    { Xwaiting,			ImportObservation( KEY_WAITING ) },
	    { Xwaiting_variance,	ImportObservation( KEY_WAITING_VARIANCE ) },
	    { Xprob_exceed_max,		ImportObservation( KEY_EXCEEDED_TIME ) },
	    { Xphase,			ImportObservation() }
	};

	void
	JSON_Document::handleObservation( DocumentObject * parent, const picojson::value& value )
	{
	    if ( !value.is<picojson::object>() ) {
		XML::invalid_argument( Xresults, value.to_str() );	/* throws... */
	    }
	    const picojson::value::object& obj = value.get<picojson::object>();

	    const unsigned int p = get_opt_phase( obj );
	    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		const std::string& attr = i->first;
		try {
		    const std::map<const char *,const ImportObservation>::const_iterator j = observation_table.find(attr.c_str());
		    if ( j != observation_table.end() ) {
			j->second(attr,*this,*parent,p,i->second);		/* Handle attribute */
		    } else {
			LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xobserve, attr.c_str() );
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xobserve, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xobserve, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xobserve, arg.what() );
		}
	    }
	}


	const std::map<const char*,const JSON_Document::ImportResult,JSON_Document::ImportResult>  JSON_Document::result_table =
	{
	    { Xbottleneck_strength,	ImportResult( &DocumentObject::setResultBottleneckStrength ) },
	    { Xdrop_probability,	ImportResult( &DocumentObject::setResultDropProbability,	&DocumentObject::setResultDropProbabilityVariance ) },
	    { Xjoin_variance,		ImportResult( &DocumentObject::setResultVarianceJoinDelay,	&DocumentObject::setResultVarianceJoinDelayVariance ) },
	    { Xjoin_waiting,		ImportResult( &DocumentObject::setResultJoinDelay,		&DocumentObject::setResultJoinDelayVariance ) },
	    { Xmarginal_queue_probabilities, ImportResult() },
	    { Xopen_wait_time,		ImportResult( &DocumentObject::setResultWaitingTime,		&DocumentObject::setResultWaitingTimeVariance ) },
	    { Xphase_utilization,	ImportResult( &DocumentObject::setResultUtilization,		&DocumentObject::setResultUtilizationVariance,		&DocumentObject::setResultPhasePUtilization,	    &DocumentObject::setResultPhasePUtilizationVariance ) },
	    { Xproc_utilization,	ImportResult( &DocumentObject::setResultProcessorUtilization,	&DocumentObject::setResultProcessorUtilizationVariance ) },
	    { Xproc_waiting,		ImportResult( &DocumentObject::setResultProcessorWaiting,	&DocumentObject::setResultProcessorWaitingVariance,	&DocumentObject::setResultPhasePProcessorWaiting,   &DocumentObject::setResultPhasePProcessorWaitingVariance ) },
	    { Xservice_time,		ImportResult( &DocumentObject::setResultServiceTime,		&DocumentObject::setResultServiceTimeVariance,		&DocumentObject::setResultPhasePServiceTime,	    &DocumentObject::setResultPhasePServiceTimeVariance ) },
	    { Xservice_time_variance,	ImportResult( &DocumentObject::setResultVarianceServiceTime,	&DocumentObject::setResultVarianceServiceTimeVariance,	&DocumentObject::setResultPhasePVarianceServiceTime,&DocumentObject::setResultPhasePVarianceServiceTimeVariance ) },
	    { Xsquared_coeff_variation, ImportResult( &DocumentObject::setResultSquaredCoeffVariation,	&DocumentObject::setResultSquaredCoeffVariationVariance) },
	    { Xthroughput,		ImportResult( &DocumentObject::setResultThroughput,		&DocumentObject::setResultThroughputVariance ) },
	    { Xthroughput_bound,	ImportResult( &DocumentObject::setResultThroughputBound ) },
	    { Xutilization,		ImportResult( &DocumentObject::setResultUtilization,		&DocumentObject::setResultUtilizationVariance,		&DocumentObject::setResultPhasePUtilization,	    &DocumentObject::setResultPhasePUtilizationVariance ) },
	    { Xwaiting,			ImportResult( &DocumentObject::setResultWaitingTime,		&DocumentObject::setResultWaitingTimeVariance ) },
	    { Xwaiting_variance,	ImportResult( &DocumentObject::setResultVarianceWaitingTime,	&DocumentObject::setResultVarianceWaitingTimeVariance ) }
	};

	void
	JSON_Document::handleResult( DocumentObject * parent, const picojson::value& value )
	{
	    if ( !_loadResults ) return;

	    if ( !value.is<picojson::object>() ) {
		XML::invalid_argument( Xresults, value.to_str() );	/* throws... */
	    }
	    const picojson::value::object& obj = value.get<picojson::object>();
	    for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
		const std::string& attr = i->first;
		try {
		    const std::map<const char *,const ImportResult>::const_iterator j = result_table.find(attr.c_str());
		    if ( j != result_table.end() ) {
			j->second(attr,*this,*parent,i->second);		/* Handle attribute */
		    } else {
			LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xresults, attr.c_str() );
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xresults, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xresults, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xresults, arg.what() );
		}
	    }
	}

	const std::map<const char*,const JSON_Document::ImportHistogram,JSON_Document::ImportHistogram> JSON_Document::histogram_table =
	{
	    { Xnumber_bins,	ImportHistogram() },		// Nop must be done first.
	    { Xmin,		ImportHistogram() },
	    { Xmax,		ImportHistogram() },
	    { Xhistogram_bin,	ImportHistogram() }
	};

	void
	JSON_Document::handleHistogram( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() && dynamic_cast<Entry *>(parent) ) {
		Entry * entry = dynamic_cast<Entry *>(parent);
		const picojson::value::array& arr = value.get<picojson::array>();
		unsigned int p = 1;
		for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x, ++p ) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *x );
		    if ( x->is<picojson::object>() ) {
			handleHistogram( entry->getPhase( p ), *x );
		    } else {
			XML::invalid_argument( Xhistogram, value.to_str() );
		    }
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *x );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    Histogram * hist = new Histogram( &_document,
						      Histogram::Type::CONTINUOUS,
						      get_long_attribute( Xnumber_bins, obj ),
						      get_double_attribute( Xmin, obj ),
						      get_double_attribute( Xmax, obj ) );
		    parent->setHistogram( hist );

		    /* Now do remaining attributes with hist (results) */
		    if ( _loadResults ) {
			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
			    const std::string& attr = i->first;
			    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr, i->second );
			    const std::map<const char *,const ImportHistogram>::const_iterator j = histogram_table.find(attr.c_str());
			    if ( j == histogram_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xtask, attr.c_str() );
			    } else if ( i->second.is<picojson::array>() ) {
				const picojson::value::array& arr = i->second.get<picojson::array>();
				unsigned int n = 0;
				for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x, ++n ) {
				    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *x );
				    handleHistogramBin( hist, n, *x );		/* Need the bin :-( */
				    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *x );
				}
			    } else if ( j->second.getType() != dom_type::DOM_NULL ) {
				XML::invalid_argument( Xhistogram, value.to_str() );
			    }
			    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, attr, i->second );
			}
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xhistogram, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xhistogram, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xhistogram, arg.what() );
		}
	    } else if ( value.is<double>() ) {
		/* For prob_exceed_max */
		Histogram * hist = new Histogram( &_document, Histogram::Type::CONTINUOUS, 0, value.get<double>(), value.get<double>() );
		parent->setHistogram( hist );
	    } else {
		XML::invalid_argument( Xhistogram, value.to_str() );
	    }
	}

	void
	JSON_Document::handleHistogramBin( DocumentObject * parent, unsigned int index, const picojson::value& value )
	{
	    Histogram * histogram = dynamic_cast<Histogram *>(parent);
	    if ( value.is<picojson::object>() && histogram ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		double mean = 0.;
		double variance = 0.;
		for ( picojson::value::object::const_iterator k = obj.begin(); k != obj.end(); ++k ) {
		    const std::string& attr = k->first;
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr, k->second );
		    if ( attr == Xbegin && k->second.is<double>() ) {
		    } else if ( attr == Xprob && k->second.is<double>() ) {
			mean  = k->second.get<double>();
		    } else if ( attr == Xconf_95 && k->second.is<double>() ) {
			variance = invert( k->second.get<double>() );
		    } else if ( attr != Xend || !k->second.is<double>() ) {
			XML::invalid_argument( attr, k->second.to_str() );
		    }
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, attr, k->second );
		}
		histogram->setBinMeanVariance( index, mean, variance );
	    } else {
		XML::invalid_argument( Xhistogram_bin, value.to_str() );
	    }
	}


	void
	JSON_Document::handleMaxServiceTime( DocumentObject * parent, const picojson::value& value )
	{
	    if ( value.is<picojson::array>() && dynamic_cast<Entry *>(parent) ) {
		Entry * entry = dynamic_cast<Entry *>(parent);
		const picojson::value::array& arr = value.get<picojson::array>();
		unsigned int p = 1;
		for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x, ++p ) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *x );
		    if ( x->is<picojson::object>() ) {
			handleMaxServiceTime( entry->getPhase( p ), *x );
		    } else {
			XML::invalid_argument( Xmax_service_time, value.to_str() );
		    }
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *x );
		}
	    } else if ( value.is<picojson::object>() ) {
		const std::map<std::string, picojson::value> obj = value.get<picojson::object>();
		try {
		    double service_time = get_double_attribute( Xmax_service_time, obj );
		    Histogram * hist = new Histogram( &_document,
						      Histogram::Type::CONTINUOUS,
						      0,
						      service_time,
						      service_time );
		    parent->setHistogram( hist );

		    /* Now do remaining attributes with hist (results) */
		    if ( _loadResults ) {
			for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i) {
#if 0
			    const std::string& attr = i->first;
			    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr, i->second );
			    const std::map<const char *,const ImportHistogram>::const_iterator j = histogram_table.find(attr.c_str());
			    if ( j == histogram_table.end() ) {
				LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xtask, attr.c_str() );
			    } else if ( i->second.is<picojson::array>() ) {
				const picojson::value::array& arr = i->second.get<picojson::array>();
				unsigned int n = 0;
				for (picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x, ++n ) {
				    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *x );
				    handleHistogramBin( hist, n, *x );		/* Need the bin :-( */
				    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *x );
				}
			    } else if ( j->second.getType() != dom_type::DOM_NULL ) {
				XML::invalid_argument( Xmax_service_time, value.to_str() );
			    }
			    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, attr, i->second );
#endif
			}
		    }
		}
		catch ( const XML::missing_attribute& attr ) {
		    LQIO::runtime_error( LQIO::ERR_MISSING_ATTRIBUTE, Xmax_service_time, attr.what() );
		}
		catch ( const std::invalid_argument& arg ) {
		    LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, Xmax_service_time, arg.what() );
		}
		catch ( const should_implement& arg ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xmax_service_time, arg.what() );
		}
	    }
	}

	/* Find the results for an element and handle them.  (used when reloading the model). */

	void
	JSON_Document::handleResults( DocumentObject * parent, const std::map<std::string, picojson::value>& obj )
	{
	    std::map<std::string, picojson::value>::const_iterator attr = obj.find( Xresults );
	    if ( attr != obj.end() ) {
		handleResult( parent, attr->second );
	    }
	}

	void
	JSON_Document::connectEntry( Entry * entry, Task * task, const std::string& name )
	{
	    if ( !task ) {
		LQIO::runtime_error( ERR_NOT_DEFINED, name.c_str() );
	    } else {
		const std::vector<Entry*>& entries = task->getEntryList();
		const_cast<std::vector<Entry*>&>(entries).push_back(entry);
		entry->setTask(task);
		Entry * old_entry = _document.getEntryByName( name );
		if ( !old_entry ) {
		    _document.addEntry(entry);
		} else {
		    assert( old_entry == entry );
		}
	    }
	}


	/* static */ bool
	JSON_Document::has_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    const std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    return attr != obj.end();
	}

	/* static */ const std::string
	JSON_Document::get_string_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    const std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr != obj.end() ) {
		return attr->second.to_str();
	    } else {
		throw XML::missing_attribute( name );
	    }
	}

	/* static */ long
	JSON_Document::get_long_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    const std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr == obj.end() ) {
		throw XML::missing_attribute( name );
	    } else if ( attr->second.is<double>() ) {
		const double value = attr->second.get<double>();
		if ( value >= 0 && rint(value) == value ) {
		    return static_cast<long>(value);
		}
	    } else if ( attr->second.is<std::string>() ) {
		char * end_ptr = 0;
		const double value = strtod( attr->second.get<std::string>().c_str(), &end_ptr );
		if ( value >= 0 && rint(value) == value && ( !end_ptr || *end_ptr == '\0' ) ) {
		    return static_cast<long>(value);
		}
	    }
	    XML::invalid_argument( name, attr->second.to_str() );		// throws.
	    return 0;
	}

	/* static */ double
	JSON_Document::get_double_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    const std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr->first, attr->second );
	    if ( attr == obj.end() ) {
		throw XML::missing_attribute( name );
	    } else if ( attr->second.is<double>() ) {
		return attr->second.get<double>();
	    } else if ( attr->second.is<std::string>() ) {
		char * end_ptr = 0;
		const double value = strtod( attr->second.get<std::string>().c_str(), &end_ptr );
		if ( !end_ptr || *end_ptr == '\0' ) {
		    return value;
		}
	    }
	    XML::invalid_argument( name, attr->second.to_str() );		// throws.
	    return 0.;
	}

	/* static */ const picojson::value&
	JSON_Document::get_value_attribute( const char * name, const std::map<std::string, picojson::value>& obj )
	{
	    const std::map<std::string, picojson::value>::const_iterator attr = obj.find( name );
	    if ( attr != obj.end() ) {
		return attr->second;
	    } else {
		throw XML::missing_attribute( name );
	    }
	}

	/* static */  scheduling_type
	JSON_Document::get_scheduling_attribute( const std::map<std::string, picojson::value>& obj, const scheduling_type default_value )
	{
	    const std::map<std::string, picojson::value>::const_iterator attr = obj.find( Xscheduling );
	    if ( attr != obj.end() ) {
		const std::string attribute = attr->second.to_str();
		std::map<const std::string, const scheduling_type>::const_iterator i = scheduling_table.find( attribute.c_str() );
		if ( i == scheduling_table.end() ) {
		    XML::invalid_argument( Xscheduling, attribute );	/* throws */
		} else {
		    return i->second;
		}
	    }
	    return default_value;
	}

	/* static */ unsigned int
	JSON_Document::get_unsigned_int( const picojson::value& value )
	{
	    char * end_ptr = 0;
	    double i = -1;
	    if ( value.is<double>() ) {
		i = value.get<double>();
	    } else if ( value.is<std::string>() ) {
		i = strtod( value.get<std::string>().c_str(), &end_ptr );
	    }
	    if ( i > 0 && rint(i) == i && ( !end_ptr || *end_ptr == '\0' ) ) {
		return static_cast<unsigned int>(i);
	    }
	    throw std::invalid_argument( value.to_str() );
	}

	/* static */ unsigned int
	JSON_Document::get_opt_phase( const picojson::value::object& obj )
	{
	    const std::map<std::string, picojson::value>::const_iterator phase = obj.find( Xphase );
	    if ( phase != obj.end() ) {
		const picojson::value& value = phase->second;
		char * end_ptr = 0;
		double i = -1;
		if ( value.is<double>() ) {
		    i = value.get<double>();
		} else if ( value.is<std::string>() ) {
		    i = strtod( value.get<std::string>().c_str(), &end_ptr );
		} else if ( value.is<picojson::array>() ) {
		    return 0;
		}
		if ( 0 < i && rint(i) <= Phase::MAX_PHASE && rint(i) == i && ( !end_ptr || *end_ptr == '\0' ) ) {
		    return rint(i);
		}
		throw std::invalid_argument( value.to_str() );
	    }
	    return 0;
	}

	ExternalVariable *
	JSON_Document::get_external_variable( const picojson::value& value ) const
	{
	    static const std::regex e( "^\\$[_$A-Za-z0-9]+$" );
	    if ( value.is<std::string>() ) {
		const std::string& s = value.get<std::string>();
		if ( std::regex_match( s, e ) ) {
		    bool is_symbol = false;
		    return getDocument().db_build_parameter_variable( s.c_str(), &is_symbol );
		} else {
		    /* The parser only returns success/fail... so we use an global variable. */
		    if ( srvnparse_string( SPEX_EXPRESSION, s.c_str() ) == 0 ) {
			return static_cast<ExternalVariable *>(Spex::__temp_variable);
		    }
		    /* Parse errors will throw invalid argument */
		}
	    } else if ( value.is<double>() ) {
		return new ConstantExternalVariable( value.get<double>() );
	    }
	    throw std::invalid_argument( value.to_str() );
	}

	/* static */ std::ostream&
	JSON_Document::Import::printIndent( std::ostream& output, int i )
	{
	    if ( i < 0 ) {
		__indent += i;
		if ( __indent < 0 ) {
		    __indent = 0;
		}
	    }
	    output << std::setw( __indent * 4 ) << " ";
	    if ( i >= 0 ) {
		__indent += i;
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Import::beginAttribute( std::ostream& output, const std::string& attribute, const picojson::value& value )
	{
	    if ( value.is<picojson::object>() ) {
		std::cerr << indent( +1 ) << attribute << ": {" << std::endl;
	    } else if ( value.is<picojson::array>() ) {
		std::cerr << indent( +1 ) << attribute << ": [" << std::endl;
	    } else {
		std::cerr << indent( 0 ) << attribute << ": " << value.to_str() << std::endl;
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Import::beginAttribute( std::ostream& output, const picojson::value& value )
	{
	    if ( value.is<picojson::object>() ) {
		std::cerr << indent( +1 ) << "{" << std::endl;
	    } else if ( value.is<picojson::array>() ) {
		std::cerr << indent( +1 ) << "[" << std::endl;
	    } else {
		std::cerr << indent( 0 ) << value.to_str() << std::endl;
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Import::endAttribute( std::ostream& output, const std::string& attribute, const picojson::value& value )
	{
	    return endAttribute( output, value );

	}
	/* static */ std::ostream&
	JSON_Document::Import::endAttribute( std::ostream& output, const picojson::value& value )
	{
	    if ( value.is<picojson::object>() ) {
		std::cerr << indent( -1 ) << "}" << std::endl;
	    } else if ( value.is<picojson::array>() ) {
		std::cerr << indent( -1 ) << "]" << std::endl;
	    }

	    return output;
	}

	/*
	 * This method corresponds to the "set" method used in the constructor.
	 */

	void
	JSON_Document::ImportModel::operator()( const std::string& attribute, JSON_Document& document, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    switch ( getType() ) {
	    case dom_type::JSON_OBJECT:
		if ( value.is<picojson::object>() || value.is<picojson::array>() || value.is<std::string>() ) {
		    (document.*getFptr().o)( 0, value );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::DOM_NULL:
		break;

	    default:
		XML::invalid_argument( attribute, value.to_str() );
		break;
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}

	/*
	 * This method corresponds to the "set" method used in the constructor.
	 */

	void
	JSON_Document::ImportGeneral::operator()( const std::string& attribute, JSON_Document& input, Document& document, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    switch ( getType()	) {
	    case dom_type::DOM_EXTVAR:
		if ( value.is<std::string>() ) {
		    const std::string& str = value.get<std::string>();
		    if ( str[0] == '$' ) {
			bool is_symbol = false;
			(document.*getFptr().de)( document.db_build_parameter_variable( str.c_str(), &is_symbol ) );
		    } else {
			(document.*getFptr().de)( new ConstantExternalVariable( str.c_str() ) );
		    }
		} else if ( value.is<double>() ) {
		    (document.*getFptr().de)( new ConstantExternalVariable( value.get<double>() ) );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::UNSIGNED:
		if ( value.is<double>() ) {
		    (document.*getFptr().du)( static_cast<unsigned int>(value.get<double>()) );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (document.*getFptr().ds)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::JSON_OBJECT:
		if ( value.is<picojson::object>() || value.is<picojson::array>() ) {
		    (input.*getFptr().o)( nullptr, value );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    default:
		abort();
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}


	/*
	 * This method corresponds to the "set" method used in the constructor.
	 */

	void
	JSON_Document::ImportProcessor::operator()( const std::string& attribute, JSON_Document& input, Processor& processor, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    switch ( getType() ) {
	    case dom_type::DOM_EXTVAR:
		(processor.*getFptr().pr_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::UNSIGNED:
		if ( value.is<double>() ) {
		    (processor.*getFptr().et_u)( static_cast<unsigned int>(value.get<double>()) );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::DOUBLE:
		if ( value.is<double>() ) {
		    (processor.*getFptr().et_d)( value.get<double>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::JSON_OBJECT:
		if ( value.is<picojson::object>() || value.is<picojson::array>() ) {
		    (input.*getFptr().o)( &processor, value );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (processor.*getFptr().os)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::DOM_NULL:
		break;

	    default:
		abort();
		break;
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}


	/*
	 * This method corresponds to the "set" method used in the constructor.
	 */

	void
	JSON_Document::ImportGroup::operator()( const std::string& attribute, JSON_Document& input, Group& group, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );
	    switch ( getType() ) {
	    case dom_type::DOM_EXTVAR:
		(group.*getFptr().gr_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::BOOLEAN:
		if ( value.is<bool>() ) {
		    (group.*getFptr().gr_b)( value.get<bool>() );
		} else if ( value.is<std::string>() && ::strcasecmp( value.get<std::string>().c_str(), "true" ) == 0 ) {
		    (group.*getFptr().gr_b)( true );
		} else if ( value.is<std::string>() && ::strcasecmp( value.get<std::string>().c_str(), "false" ) == 0 ) {
		    (group.*getFptr().gr_b)( false );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::JSON_OBJECT:
		if ( value.is<picojson::object>() || value.is<picojson::array>() ) {
		    (input.*getFptr().o)( &group, value );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (group.*getFptr().os)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::DOM_PROCESSOR:
		if ( value.is<std::string>() ) {
		    Processor * processor = input.getDocument().getProcessorByName( value.get<std::string>().c_str() );
		    if ( processor ) {
			(group.*getFptr().gr_p)( processor );
		    } else {
			LQIO::input_error( LQIO::ERR_NOT_DEFINED, value.get<std::string>().c_str() );
		    }
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::DOM_NULL:
		break;

	    default:
		abort();
	    }
	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}


	/*
	 * This method corresponds to the "set" method used in the constructor.
	 */

	void
	JSON_Document::ImportTask::operator()( const std::string& attribute, JSON_Document& input, Task& task, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    switch ( getType() ) {
	    case dom_type::DOM_EXTVAR:
		(task.*getFptr().ta_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::UNSIGNED:
		if ( value.is<double>() ) {
		    (task.*getFptr().ta_u)( static_cast<unsigned int>(value.get<double>()) );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::JSON_OBJECT:
		if ( value.is<picojson::object>() || value.is<picojson::array>() ) {
		    if ( getGptr() ) {
			(input.*getFptr().ta_f)( &task, value, getGptr() );
		    } else {
			(input.*getFptr().o)( &task, value );
		    }
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (task.*getFptr().os)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::DOM_GROUP:
		if ( value.is<std::string>() ) {
		    Group * group = input.getDocument().getGroupByName( value.get<std::string>().c_str() );
		    if ( group ) {
			(task.*getFptr().ta_g)( group );
			group->addTask( &task );
		    } else {
			LQIO::input_error( LQIO::ERR_NOT_DEFINED, value.get<std::string>().c_str() );
		    }
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::DOM_PROCESSOR:
		if ( value.is<std::string>() ) {
		    Processor * processor = input.getDocument().getProcessorByName( value.get<std::string>().c_str() );
		    if ( processor ) {
			(task.*getFptr().ta_p)( processor );
			processor->addTask( &task );
		    } else {
			LQIO::input_error( LQIO::ERR_NOT_DEFINED, value.get<std::string>().c_str() );
		    }
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::DOM_NULL:
		break;

	    default:
		abort();
		break;
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}


	/*
	 * Simple entry parameters
	 */

	void
	JSON_Document::ImportEntry::operator()( const std::string& attribute, bool is_array, JSON_Document& input, Entry& entry, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) {
		if ( is_array ) {
		    Import::beginAttribute( std::cerr, value );
		} else {
		    Import::beginAttribute( std::cerr, attribute, value );
		}
	    }

	    switch ( getType() ) {
	    case dom_type::DOM_ACTIVITY:
		if ( value.is<std::string>() ) {
		    /* Activity name... we have to find it. */
		    Task * task = const_cast<Task *>(entry.getTask());
		    if ( task ) {
			const std::string& name = value.get<std::string>();
			Activity* activity = task->getActivity( name, true);
			(entry.*getFptr().en_a)( activity );
		    }
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::DOM_EXTVAR:
		(entry.*getFptr().en_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::DOM_NULL:
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (entry.*getFptr().os)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::JSON_OBJECT:
		if ( getCallType() == Call::Type::NULL_CALL ) {
		    (input.*getFptr().o)( &entry, value );
		} else {
		    (input.*getFptr().ca_t)( &entry, value, getCallType() );		/* For forwarding. */
		}
		break;

	    default:
		abort();
		break;
	    }

	    if ( Document::__debugJSON ) {
		if ( is_array ) {
		    Import::endAttribute( std::cerr, value );
		} else {
		    Import::endAttribute( std::cerr, attribute, value );
		}
	    }
	}


	/*
	 * Phase parameters specified by the entry.
	 */

	void
	JSON_Document::ImportEntry::operator()( const std::string& attribute, JSON_Document& input, Phase& phase, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, value );

	    switch ( getType() ) {
	    case dom_type::DOM_EXTVAR:
		(phase.*getFptr().ph_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::JSON_OBJECT:
		if ( getCallType() == Call::Type::NULL_CALL ) {
		    (input.*getFptr().o)( &phase, value );
		} else {
		    (input.*getFptr().ca_t)( &phase, value, getCallType() );		/* For Sync/async calls */
		}
		break;

	    case dom_type::BOOLEAN:
		if ( value.is<bool>() ) {
		    (phase.*getFptr().ph_t)( value.get<bool>() ? Phase::Type::DETERMINISTIC : Phase::Type::STOCHASTIC );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    default:
		abort();
		break;
	    }

	    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, value );
	}


	void
	JSON_Document::ImportPhase::operator()( const std::string& attribute, JSON_Document& input, Phase& phase, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    switch ( getType() ) {
	    case dom_type::DOM_EXTVAR:
		(phase.*getFptr().ph_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::BOOLEAN:
		if ( value.is<bool>() ) {
		    (phase.*getFptr().ph_t)( value.get<bool>() ? Phase::Type::DETERMINISTIC : Phase::Type::STOCHASTIC );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::JSON_OBJECT:
		(input.*getFptr().ca_t)( &phase, value, getCallType() );
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (phase.*getFptr().os)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::DOM_NULL:
		break;

	    default:
		abort();
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}



	/*
	 * Called for extvar and doubles (and strings) to set parameters for a call
	 */

	void
	JSON_Document::ImportCall::operator()( const std::string& attribute, JSON_Document& input, Call& call, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, value );

	    switch ( getType() ) {
	    case dom_type::DOM_EXTVAR:
		(call.*getFptr().ca_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::DOM_NULL:
		break;

	    case dom_type::JSON_OBJECT:
		(input.*getFptr().o)( &call, value );
		break;

	    default:
		abort();
		break;
	    }

	    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, value );
	}

	/*
	 * Called for objects (results, observations).
	 */

	void
	JSON_Document::ImportCall::operator()( const std::string& attribute, JSON_Document& input, Entry& entry, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );
	    switch ( getType() ) {
	    case dom_type::JSON_OBJECT:
		(input.*getFptr().o)( &entry, value );
		break;

	    case dom_type::DOM_NULL:
		break;

	    default:
		abort();
		break;
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}

	void
	JSON_Document::ImportTaskActivity::operator()( const std::string& attribute, JSON_Document& input, Task *task, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );
	    switch ( getType() ) {
	    case dom_type::JSON_OBJECT:
		if ( value.is<picojson::object>() || value.is<picojson::array>() ) {
		    (input.*getFptr().o)( task, value );
		}
		break;

	    case dom_type::DOM_NULL:
		break;

	    default:
		abort();
	    }
	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}

	void
	JSON_Document::ImportActivity::operator()( const std::string& attribute, JSON_Document& input, Activity& activity, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    switch ( getType() ) {
	    case dom_type::DOM_EXTVAR:
		(activity.*getFptr().ph_e)( input.get_external_variable( value ) );
		break;

	    case dom_type::BOOLEAN:
		if ( value.is<bool>() ) {
		    (activity.*getFptr().ph_t)( value.get<bool>() ? Phase::Type::DETERMINISTIC : Phase::Type::STOCHASTIC );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::JSON_OBJECT:
		(input.*getFptr().ca_t)( &activity, value, getCallType() );
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (activity.*getFptr().os)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}

	    case dom_type::DOM_NULL:
		break;

	    default:
		abort();
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}



	void
	JSON_Document::ImportPrecedence::operator()( const std::string& attribute, JSON_Document& input, ActivityList& list, const picojson::value& value ) const
	{
	    Activity * activity = 0;
	    const Task * task = list.getTask();
	    if ( value.is<std::string>() ) {

		/* Just an activity reference */

		if ( Document::__debugJSON ) std::cerr << indent(0) << value.get<std::string>() << std::endl;
		activity = task->getActivity( value.get<std::string>() );
		if ( !activity ) {
		    throw undefined_symbol( value.get<std::string>() );
		}
		list.add( activity );
	    } else if ( value.is<picojson::object>() ) {
		const picojson::value::object& obj = value.get<picojson::object>();
		picojson::value::object::const_iterator i = obj.begin();
		if ( obj.size() == 1 ) {
		    if ( Document::__debugJSON ) std::cerr << indent(0) << "{ " << i->first << ": " << i->second.to_str() << " }" << std::endl;
		    const std::string& name = i->first;
		    activity = task->getActivity( name );
		    if ( !activity ) {
			throw undefined_symbol( value.get<std::string>() );
		    }
		    const picojson::value& value = i->second;
		    if ( value.is<std::string>() ) {
			bool is_symbol = false;
			list.add( activity, input.getDocument().db_build_parameter_variable( value.get<std::string>().c_str(), &is_symbol ) );
		    } else if ( value.is<picojson::null>() ) {
			list.add( activity );
		    } else if ( value.is<double>() ) {
			list.addValue( activity, value.get<double>() );
		    }
		} else if ( obj.size() > 1 ) {
		    LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xprecedence, (++i)->first.c_str() );
		}
	    } else {
		XML::invalid_argument( attribute, value.to_str() );			// throws
	    }

	    if ( activity ) {
		if ( list.isJoinList() ) {
		    activity->outputTo( &list );
		} else {
		    activity->inputFrom( &list );
		}
	    }
	}


	void
	JSON_Document::ImportGeneralObservation::operator()( const std::string& attribute, JSON_Document& input, LQIO::DOM::Document& document, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );
	    const int key = getKey();
	    try {
		if ( value.is<std::string>() ) {
		    const std::string& var1 = value.get<std::string>();
		    LQIO::spex.observation( LQIO::Spex::ObservationInfo( key, 0, var1.c_str() ) );
		} else {
		    throw std::invalid_argument( value.to_str() );
		}
	    }
	    catch ( const std::invalid_argument& arg ) {
		LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, attribute.c_str(), arg.what() );
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}

	void
	JSON_Document::ImportObservation::operator()( const std::string& attribute, JSON_Document& input, DocumentObject& object, unsigned int phase, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );

	    const int key = getKey();
	    try {
		if ( value.is<picojson::array>() ) {
		    const picojson::value::array& array = value.get<picojson::array>();
		    for (picojson::value::array::const_iterator j = array.begin(); j != array.end(); ++j ) {
			if ( j->is<picojson::object>() ) {
			    input.handleObservation( &object, *j );
			} else {
			    throw std::invalid_argument( j->to_str() );
			}
		    }
		} else if ( key != 0 ) {
		    if ( value.is<picojson::object>() ) {
			const picojson::value::object& obj = value.get<picojson::object>();
			const std::string& var1 = get_string_attribute( Xmean, obj );
			const std::string& var2 = get_string_attribute( Xconf_95, obj );
			addObservation( &object, phase, key, var1.c_str(), 95, var2.c_str() );	/* string */
		    } else if ( value.is<std::string>() ) {
			const std::string& var1 = value.get<std::string>();
			addObservation( &object, phase, key, var1.c_str() );
		    } else {
			throw std::invalid_argument( value.to_str() );
		    }
		}
	    }
	    catch ( const std::invalid_argument& arg ) {
		LQIO::runtime_error( LQIO::ERR_INVALID_ARGUMENT, attribute.c_str(), arg.what() );
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}

	void
	JSON_Document::ImportObservation::addObservation( DocumentObject * object, unsigned int p, int key, const char * var1, unsigned int conf, const char * var2 ) const
	{
	    if ( dynamic_cast<Processor *>(object)
		 || dynamic_cast<Group *>(object)
		 || dynamic_cast<Task *>(object)
		 || dynamic_cast<Entry *>(object) ) {
		LQIO::spex.observation( object, LQIO::Spex::ObservationInfo( key, p, var1, conf, var2 ) );
	    } else if ( dynamic_cast<Activity *>(object) ) {
		/* Handle before phase because an activity is a phase */
		LQIO::DOM::Activity * activity = dynamic_cast<Activity *>(object);
		const LQIO::DOM::Task * task = activity->getTask();
		LQIO::spex.observation( task, activity, LQIO::Spex::ObservationInfo( key, 0, var1, conf, var2 ) );
	    } else if ( dynamic_cast<Phase *>(object) ) {
		/* A bit more complicated because phases are referenced via entries. */
		const Phase * phase = dynamic_cast<Phase *>(object);
		const Entry * entry = phase->getSourceEntry();
		LQIO::spex.observation( entry, LQIO::Spex::ObservationInfo( key, get_phase( phase ), var1, conf, var2 ) );
	    } else if ( dynamic_cast<Call *>(object) ) {
		/* Even more complicated becauses I need to get to the entry to find the call in LQX */
		const LQIO::DOM::Call * call = dynamic_cast<LQIO::DOM::Call *>(object);
		const DocumentObject * source = call->getSourceObject();
		const LQIO::DOM::Entry * destination = call->getDestinationEntry();
		if ( dynamic_cast<const Activity *>(source) ) {
		    const LQIO::DOM::Activity * activity = dynamic_cast<const Activity *>(source);
		    const LQIO::DOM::Task * task = activity->getTask();
		    LQIO::spex.observation( task, activity, destination, LQIO::Spex::ObservationInfo( key, 0, var1, conf, var2 ) );
		} else if ( dynamic_cast<const Phase *>(source) ) {
		    const Phase * phase = dynamic_cast<const Phase *>(source);
		    const Entry * entry = phase->getSourceEntry();
		    const unsigned int p = get_phase( phase );
		    LQIO::spex.observation( entry, p, destination, LQIO::Spex::ObservationInfo( key, p, var1, conf, var2 ) );
		} else {	/* forwarding */
		    const Entry * entry = dynamic_cast<const Entry *>(source);
		    assert( entry != nullptr );
		    LQIO::spex.observation( entry, destination, LQIO::Spex::ObservationInfo( key, 0, var1, conf, var2 ) );
		}
	    } else {
		abort();
	    }
	}

	void
	JSON_Document::ImportGeneralResult::operator()( const std::string& attribute, JSON_Document& input, Document& document, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );
	    unsigned long hrs	= 0;
	    unsigned long mins	= 0;
	    double secs	 = 0;

	    switch ( getType() ) {
	    case dom_type::BOOLEAN:
		if ( value.is<bool>() ) {
		    (document.*getFptr().db)( value.get<bool>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::CLOCK:
		if ( value.is<std::string>() && ::sscanf( value.get<std::string>().c_str(), "%ld:%ld:%lf", &hrs, &mins, &secs ) == 3 ) {
		    (document.*getFptr().dd)( hrs * 3600 + mins * 60 + secs );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::DOUBLE:
		if ( value.is<double>() ) {
		    (document.*getFptr().dd)( value.get<double>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::LONG:
		if ( value.is<double>() ) {
		    (document.*getFptr().dl)( static_cast<long>(value.get<double>()) );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::UNSIGNED:
		if ( value.is<double>() ) {
		    (document.*getFptr().du)( static_cast<unsigned int>(value.get<double>()) );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::STRING:
		if ( value.is<std::string>() ) {
		    (document.*getFptr().ds)( value.get<std::string>() );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::JSON_OBJECT:
		if ( value.is<picojson::object>() ) {
		    (input.*getFptr().doc)( &document, value );
		} else {
		    XML::invalid_argument( attribute, value.to_str() );
		}
		break;

	    case dom_type::DOM_NULL:
		break;

	    default:
		XML::invalid_argument( attribute, value.to_str() );
		break;
	    }
	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}

	void
	JSON_Document::ImportResult::operator()( const std::string& attribute, JSON_Document& input, DocumentObject& dom_obj, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );
	    if ( value.is<double>() && getType() == dom_type::DOUBLE ) {
		(dom_obj.*getFptr().re_d)( value.get<double>() );
	    } else if ( value.is<picojson::array>() ) {		/* phase results */
		/* List of results (except for phases!) */
		const picojson::value::array& arr = value.get<picojson::array>();
		unsigned int p = 1;
		for ( picojson::value::array::const_iterator x = arr.begin(); x != arr.end(); ++x, ++p ) {
		    if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, *x );
		    if ( ::strcasecmp( attribute.c_str(), Xmarginal_queue_probabilities ) == 0 ) {
			if ( dynamic_cast<Entity*>(&dom_obj) != nullptr ) {
			    std::vector<double>& marginals = dynamic_cast<Entity&>(dom_obj).getResultMarginalQueueProbabilities();
			    marginals.push_back( x->get<double>() );
			}
		    } else if ( x->is<double>() ) {
			(dom_obj.*getF2ptr())( p, x->get<double>() );
		    } else if ( x->is<picojson::object>() ) {
			picojson::object obj = x->get<picojson::object>();
			(dom_obj.*getF2ptr())( p, input.get_double_attribute( Xmean, obj ) );				/* Mean */
			(dom_obj.*getG2ptr())( p, input.invert( input.get_double_attribute( Xconf_95, obj ) ) );	/* Variance */
		    } else {
			XML::invalid_argument( attribute, value.to_str() );
		    }
		    if ( Document::__debugJSON ) Import::endAttribute( std::cerr, *x );
		}
	    } else if ( value.is<picojson::object>() ) {
		const picojson::value::object& obj = value.get<picojson::object>();
		for ( picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); ++i ) {
		    const std::string& attr = i->first;
		    const picojson::value& arg = i->second;
		    if ( attr == Xmean && arg.is<double>() ) {
			if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr, arg );
			(dom_obj.*getFptr().re_d)( arg.get<double>() );				/* Mean */
		    } else if ( attr == Xconf_95 ) {
			if ( Document::__debugJSON ) Import::beginAttribute( std::cerr, attr, arg );
			(dom_obj.*getGptr())( input.invert( arg.get<double>() ) );		/* Variance */
		    } else {
			LQIO::runtime_error( LQIO::ERR_UNEXPECTED_ATTRIBUTE, Xresults, attr.c_str() );
		    }
		}
	    } else {
		XML::invalid_argument( attribute, value.to_str() );
	    }
	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}


	void
	JSON_Document::ImportHistogram::operator()( const std::string& attribute, JSON_Document& input, Histogram& hist, const picojson::value& value ) const
	{
	    if ( Document::__debugJSON ) std::cerr << begin_attribute( attribute, value );
	    switch ( getType() ) {
	    case dom_type::JSON_OBJECT:		/* Bin number */
		if ( value.is<picojson::object>() || value.is<picojson::array>() ) {
		    (input.*getFptr().o)( &hist, value );  // handle bin.
		}
		break;

	    case dom_type::DOM_NULL:
		break;

	    default:
		XML::invalid_argument( attribute, value.to_str() );
		break;
	    }

	    if ( Document::__debugJSON ) std::cerr << end_attribute( attribute, value );
	}
    }
}

/* -------------------------------------------------------------------- */
/* DOM serialization - write results to output				*/
/* -------------------------------------------------------------------- */
namespace LQIO {
    namespace DOM {

	/* Export the DOM. */
	void
	JSON_Document::serializeDOM( std::ostream& output ) const
	{
	    if ( _document.hasConfidenceIntervals() ) {
		const_cast<ConfidenceIntervals *>(&_conf_95)->set_blocks( _document.getResultNumberOfBlocks() );
		const_cast<ConfidenceIntervals *>(&_conf_99)->set_blocks( _document.getResultNumberOfBlocks() );
	    }

	    Model( output, _conf_95 ).print( _document );
	}

	void
	JSON_Document::Model::print( const Document& document ) const
	{
	    _output << begin_object();
	    _output << begin_array( Xcomment )
		    << indent() << "\"Generated by: " << LQIO::io_vars.lq_toolname << ", version " << LQIO::io_vars.lq_version << "\",";
	    const GetLogin login;
	    _output << indent() << "\"For: " << login << "\",";
#if HAVE_STRFTIME
	    time_t tloc;
	    char tbuf[32];
	    time( &tloc );
	    strftime( tbuf, 32, "%a %b %H:%M:%S %Y", localtime( &tloc ) );
	    _output << indent() << "\"" << tbuf << "\",";
#endif
	    _output << indent() << "\"Invoked as: " << LQIO::io_vars.lq_command_line << ' ' << LQIO::DOM::Document::__input_file_name << "\",";
	    _output << indent() << "\"" << Common_IO::svn_id() << "\"";
	    _output << end_array() << ",";

	    if ( !document.instantiated() && !Spex::input_variables().empty() ) {
		_output << begin_array( Xparameters );
		const std::map<std::string,LQX::SyntaxTreeNode *>& vars = Spex::input_variables();
		std::for_each( vars.begin(), vars.end(), ExportParameters( _output, _conf_95 ) );
		_output << end_array() << ",";
	    }

	    ExportGeneral( _output, _conf_95 ).print( document );

	    const std::map<std::string,Processor *>& processors = document.getProcessors();
	    _output << next_begin_array( Xprocessor );
	    std::for_each( processors.begin(), processors.end(), ExportProcessor( _output, _conf_95 ) );
	    _output << end_array();

	    if ( !document.instantiated() ) {
		printResultOrConvergenceVariables( Spex::result_variables(), Xresults );
		printResultOrConvergenceVariables( Spex::convergence_variables(), Xconvergence );
	    }
	    
	    _output << end_object();
	}

	
	void
	JSON_Document::Model::printResultOrConvergenceVariables( const std::vector<std::pair<const std::string,LQX::SyntaxTreeNode *>>& variables, const std::string& name ) const
	{
	    if ( variables.empty() ) return;
	    LQX::SyntaxTreeNode::setVariablePrefix( "$" );
	    _output << next_begin_array( name );
	    std::for_each( variables.begin(), variables.end(), ExportResultOrConvergenceVariable( _output, _conf_95 ) );
	    _output << end_array();
	}

	void
	JSON_Document::ExportPragma::operator()( const std::pair<std::string,std::string>& pragma ) const
	{
	    _output << separator() << indent() << "{ \"" << pragma.first << "\": \"" << pragma.second << "\" }";
	}


	void
	JSON_Document::ExportParameters::operator()( const std::pair<std::string,LQX::SyntaxTreeNode *>& var ) const
	{
	    _output << separator() << indent() << "\"" << Spex::print_input_variable( var ) << "\"";
	}


	void
	JSON_Document::ExportGeneral::print( const Document& document ) const
	{
	    _output << begin_object( Xgeneral );
	    const std::string& comment = document.getModelComment();
	    if ( !comment.empty() ) {
		_output << attribute( Xcomment, comment ) << ",";
	    }
	    _output << attribute( Xconv_val, *document.getModelConvergence() )
		    << next_attribute( Xit_limit, *document.getModelIterationLimit() )
		    << next_attribute( Xunderrelax_coeff, *document.getModelUnderrelaxationCoefficient() )
		    << next_attribute( Xprint_int, *document.getModelPrintInterval() );

	    const std::map<std::string,std::string>& pragmas = document.getPragmaList();
	    if ( !pragmas.empty() ) {
		_output << next_begin_array( Xpragma );
		std::for_each( pragmas.begin(), pragmas.end(), ExportPragma( _output, _conf_95 ) );
		_output << end_array();
	    }

	    const std::vector<LQIO::Spex::ObservationInfo> doc_vars = LQIO::Spex::document_variables();
	    if ( doc_vars.size() > 0 ) {
		_output << next_begin_object( Xobserve );
		std::for_each ( doc_vars.begin(), doc_vars.end(), ExportObservation( _output, _conf_95 ) );
		_output << end_object();
	    }
	    if ( document.hasResults() ) {
		_output << next_begin_object( Xresults )
			<< attribute( Xsolver_info, document.getResultSolverInformation() )
			<< next_attribute( Xvalid, document.getResultValid() )
			<< next_attribute( Xconv_val_result, document.getResultConvergenceValue() )
			<< next_attribute( Xiterations, static_cast<double>(document.getResultIterations()) )
			<< next_attribute( Xplatform_info, document.getResultPlatformInformation() )
			<< time_attribute( Xuser_cpu_time, document.getResultUserTime() )
			<< time_attribute( Xsystem_cpu_time, document.getResultSysTime() )
			<< time_attribute( Xelapsed_time, document.getResultElapsedTime() );
		if ( document.getResultMaxRSS() > 0 ) {
		    _output << next_attribute( Xmax_rss, static_cast<long>(document.getResultMaxRSS()) );
		}
		const MVAStatistics& mva_info = document.getResultMVAStatistics();
		if ( mva_info.getNumberOfSubmodels() > 0 ) {
		    _output << next_begin_object( Xmva_info )
			    << attribute( Xsubmodels, static_cast<long>(mva_info.getNumberOfSubmodels()) )
			    << next_attribute( Xcore, static_cast<double>(mva_info.getNumberOfCore() ) )
			    << next_attribute( Xstep, mva_info.getNumberOfStep() )
			    << next_attribute( Xstep_squared, mva_info.getNumberOfStepSquared() )
			    << next_attribute( Xwait, mva_info.getNumberOfWait() )
			    << next_attribute( Xwait_squared, mva_info.getNumberOfWaitSquared() )
			    << next_attribute( Xfaults, static_cast<double>(mva_info.getNumberOfFaults() ) );
		    _output << end_object();
		}

		_output << end_object();
	    }

	    _output << end_object();

	}


	void
	JSON_Document::ExportProcessor::print( const Processor& processor ) const
	{
	    if ( processor.getTaskList().size() == 0 ) return;

	    _output << separator() << begin_object()
		    << attribute( Xname, processor.getName() );
	    if ( processor.getComment().size() > 0 ) {
		_output << next_attribute( Xcomment,  processor.getComment() );
	    }

	    if ( processor.isInfinite() ) {
		_output << next_attribute( Xscheduling, std::string( scheduling_label.at(SCHEDULE_DELAY).XML ) );		 // see labels.cpp
		/* All other attributes don't matter */
	    } else {
		const scheduling_type scheduling = processor.getSchedulingType();
		_output << next_attribute( Xscheduling, std::string( scheduling_label.at(scheduling).XML ) );	      // see labels.cpp
		if ( processor.hasQuantumScheduling() ) {
		    if ( processor.hasQuantum() ) {
			_output << next_attribute( Xquantum, *processor.getQuantum() );
		    } else {
			_output << next_attribute( Xquantum, 0. );
		    }
		}
		if ( processor.isMultiserver() ) {
		    _output << next_attribute( Xmultiplicity, *processor.getCopies() );
		}
		if ( processor.hasRate() ) {
		    _output << next_attribute( Xspeed_factor, *processor.getRate() );
		}
	    }
	    if ( processor.hasReplicas() ) {
		_output << next_attribute( Xreplication, *processor.getReplicas() );
	    }

/*+ JSON-SPEX */
	    if ( !processor.getDocument()->instantiated() ) {
		std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::observations().equal_range( &processor );
		if ( range.first != range.second ) {
		    _output << next_begin_object( Xobserve );
		    std::for_each( range.first, range.second, ExportObservation( _output, _conf_95 ) );
		    _output << end_object();
		}
	    }
/*- JSON-SPEX */

	    if ( processor.hasResults() ) {
		const std::vector<double>& marginals = processor.getResultMarginalQueueProbabilities();
		_output << next_begin_object( Xresults )
			<< attribute( Xutilization, processor.getResultUtilization(), _conf_95, processor.getResultUtilizationVariance() );
		if ( !marginals.empty() ) {
		    _output << next_begin_array( Xmarginal_queue_probabilities, true );
		    for ( std::vector<double>::const_iterator p = marginals.begin(); p != marginals.end(); ++p ) {
			if ( p != marginals.begin() ) _output << ", ";
			_output << *p;
		    }
		    _output << end_array( true );
		}
		_output << end_object();
	    }

	    const std::set<Group*>& groups = processor.getGroupList();
	    if ( groups.size() > 0 ) {
		_output << next_begin_array( Xgroup );
		std::for_each( groups.begin(), groups.end(), ExportGroup( _output, _conf_95 ) );
		_output << end_array();
	    } else {
		const std::set<Task *>& tasks = processor.getTaskList();
		_output << next_begin_array( Xtask );
		std::for_each( tasks.begin(), tasks.end(), ExportTask( _output, _conf_95 ) );
		_output << end_array();
	    }
	    _output << end_object();
	}

	void
	JSON_Document::ExportGroup::print(Group const& group ) const
	{
	    if ( group.getTaskList().size() == 0 ) return;

	    _output << separator() << begin_object()
		    << attribute( Xname, group.getName() );
	    if ( group.getComment().size() > 0 ) {
		_output << next_attribute( Xcomment,  group.getComment() );
	    }
	    _output << next_attribute( Xshare, *group.getGroupShare() )
		    << next_attribute( Xcap, group.getCap() );

/*+ JSON-SPEX */
	    if ( !group.getDocument()->instantiated() ) {
		std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::observations().equal_range( &group );
		if ( range.first != range.second ) {
		    _output << next_begin_object( Xobserve );
		    std::for_each( range.first, range.second, ExportObservation( _output, _conf_95 ) );
		    _output << end_object();
		}
	    }
/*- JSON-SPEX */

	    if ( group.hasResults() ) {
		_output << next_begin_object( Xresults )
			<< attribute( Xutilization, group.getResultUtilization(), _conf_95, group.getResultUtilizationVariance() )
			<< end_object();
	    }

	    const std::set<Task *>& tasks = group.getTaskList();
	    _output << next_begin_array( Xtask );
	    std::for_each( tasks.begin(), tasks.end(), ExportTask( _output, _conf_95 ) );
	    _output << end_array();
	    _output << end_object();
	}


	void
	JSON_Document::ExportTask::print(Task const& task ) const
	{
	    _output << separator() << begin_object()
		    << attribute( Xname, task.getName() );
	    if ( task.getComment().size() > 0 ) {
		_output << next_attribute( Xcomment,  task.getComment() );
	    }
	    if ( task.isInfinite() ) {
		_output << next_attribute( Xscheduling, std::string( scheduling_label.at(SCHEDULE_DELAY).XML ) );		   // see lqio/labels.c
	    } else {
		_output << next_attribute( Xscheduling, std::string( scheduling_label.at(task.getSchedulingType()).XML ) );	// see lqio/labels.c
		if ( task.isMultiserver() ) {
		    _output << next_attribute( Xmultiplicity, *task.getCopies() );
		}
	    }
	    if ( task.hasReplicas() ) {
		_output << next_attribute( Xreplication, *task.getReplicas() );
	    }
	    if ( task.getSchedulingType() == SCHEDULE_CUSTOMER && task.hasThinkTime() ) {
		_output << next_attribute( Xthink_time, *task.getThinkTime() );
	    }
	    if ( task.hasPriority() ) {
		_output << next_attribute( Xpriority, *task.getPriority() );
	    }
	    if ( task.hasQueueLength() ) {
		_output << next_attribute( Xqueue_length, *task.getQueueLength() );
	    }
	    if ( task.getSchedulingType() == SCHEDULE_SEMAPHORE && dynamic_cast<const SemaphoreTask&>(task).getInitialState() == SemaphoreTask::InitialState::EMPTY ) {
		_output << next_attribute( Xinitially, 0.0 );
	    }
	    if ( task.hasHistogram() ) {
		_output << "," << indent() << "\"" << Xhistogram << "\": ";
		ExportHistogram( _output, _conf_95 ).print( *task.getHistogram() );
	    }
	    if ( task.getFanIns().size() > 0 ) {
		_output << next_begin_array( Xfanin );
		std::for_each( task.getFanIns().begin(), task.getFanIns().end(), ExportFanInOut( _output, _conf_95 ) );
		_output << end_array();
	    }
	    if ( task.getFanOuts().size() > 0 ) {
		_output << next_begin_array( Xfanout );
		std::for_each( task.getFanOuts().begin(), task.getFanOuts().end(), ExportFanInOut( _output, _conf_95 ) );
		_output << end_array();
	    }

/*+ JSON-SPEX */
	    if ( !task.getDocument()->instantiated() ) {
		std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::observations().equal_range( &task );
		if ( range.first != range.second ) {
		    const unsigned int n_phases = task.getDocument()->getMaximumPhase();
		    _output << next_begin_object( Xobserve );
		    std::vector< std::vector<Spex::ObservationInfo> > obs( n_phases + 1 );
		    bool has_phases = false;
		    for ( unsigned int i = 0; i <= n_phases; ++i ) {
			std::for_each( range.first, range.second, CollectPhase( obs[i], i ) );
			has_phases |= (i > 0 && obs[i].size() > 0);
		    }
		    /* Only want phase 0 (total) */
		    if ( obs[0].size() > 0 ) {
			std::for_each( obs[0].begin(), obs[0].end(), ExportObservation( _output, _conf_95 ) );
		    }
		    if ( has_phases ) {
			if ( obs[0].size() > 0 ) _output << ",";
			_output << begin_array( Xphase );
			for ( unsigned p = 1; p <= n_phases; ++p ) {
			    if ( p != 1 ) _output << ", ";
			    _output << begin_object()
				    << attribute( Xphase, static_cast<long>( p )) << ",";
			    std::for_each( obs[p].begin(), obs[p].end(), ExportObservation( _output, _conf_95 ) );
			    _output << end_object();
			}
			_output << end_array();
		    }
		    _output << end_object();
		}
	    }
/*- JSON-SPEX */

	    if ( task.hasResults() ) {
		_output << next_begin_object( Xresults )
			<< attribute( Xthroughput, task.getResultThroughput(), _conf_95, task.getResultThroughputVariance() )
			<< next_attribute( Xutilization, task.getResultUtilization(), _conf_95, task.getResultUtilizationVariance() )
			<< next_attribute( Xproc_utilization, task.getResultProcessorUtilization(), _conf_95, task.getResultProcessorUtilizationVariance() );
		if ( task.getResultBottleneckStrength() > 0.0 ) {
		    _output << next_attribute( Xbottleneck_strength, task.getResultBottleneckStrength() );
		}
		if ( dynamic_cast<const SemaphoreTask *>(&task) ) {
		    _output << next_attribute( Xsemaphore_waiting, task.getResultHoldingTime(), _conf_95, task.getResultHoldingTimeVariance() )
			    << next_attribute( Xsemaphore_waiting_variance, task.getResultVarianceHoldingTime(), _conf_95, task.getResultVarianceHoldingTimeVariance() )
			    << next_attribute( Xsemaphore_utilization, task.getResultHoldingUtilization(), _conf_95, task.getResultHoldingUtilizationVariance() );
		} else if ( dynamic_cast<const RWLockTask *>(&task) ) {
		    _output << next_attribute( Xrwlock_reader_waiting, task.getResultReaderBlockedTime(), _conf_95, task.getResultReaderBlockedTime() )
			    << next_attribute( Xrwlock_reader_waiting_variance, task.getResultVarianceReaderBlockedTime(), _conf_95, task.getResultVarianceReaderBlockedTime() )
			    << next_attribute( Xrwlock_reader_holding, task.getResultReaderHoldingTime(), _conf_95, task.getResultReaderHoldingTime() )
			    << next_attribute( Xrwlock_reader_holding_variance, task.getResultVarianceReaderHoldingTime(), _conf_95,  task.getResultVarianceReaderHoldingTime() )
			    << next_attribute( Xrwlock_reader_utilization, task.getResultReaderHoldingUtilization(), _conf_95, task.getResultReaderHoldingUtilization() )
			    << next_attribute( Xrwlock_writer_waiting, task.getResultWriterBlockedTime(), _conf_95, task.getResultWriterBlockedTime() )
			    << next_attribute( Xrwlock_writer_waiting_variance, task.getResultVarianceWriterBlockedTime(), _conf_95, task.getResultVarianceWriterBlockedTime() )
			    << next_attribute( Xrwlock_writer_holding, task.getResultWriterHoldingTime(), _conf_95, task.getResultWriterHoldingTime() )
			    << next_attribute( Xrwlock_writer_holding_variance, task.getResultVarianceWriterHoldingTime(), _conf_95, task.getResultVarianceWriterHoldingTime() )
			    << next_attribute( Xrwlock_writer_utilization, task.getResultWriterHoldingUtilization(), _conf_95, task.getResultWriterHoldingUtilization() );
		}

		_output << next_begin_array( Xphase_utilization, !_conf_95.is_set() );
		for ( unsigned p = 1; p <= task.getResultPhaseCount(); ++p ) {
		    if ( p != 1 ) _output << ", ";
		    _output << print_value( task.getResultPhasePUtilization(p), _conf_95, task.getResultPhasePUtilizationVariance(p) );
		}
		_output << end_array( !_conf_95.is_set() );
		_output << end_object();

	    }

	    const std::vector<Entry *>& entries = task.getEntryList();

	    /* Entry definition */
	    _output << next_begin_array( Xentry );
	    std::for_each ( entries.begin(), entries.end(), ExportEntry( _output, _conf_95 ) );
	    _output << end_array();

	    /* Activity definitions */
	    const std::map<std::string,Activity*>& activities = task.getActivities();
	    if ( activities.size() ) {
		_output << next_begin_array( Xactivity );
		std::for_each( activities.begin(), activities.end(), ExportActivity( _output, _conf_95 ) );
		_output << end_array();
	    }

	    /* Precedence defintions */
	    const std::set<ActivityList*>& precedences = task.getActivityLists();
	    if ( precedences.size() > 1 ) {
		_output << next_begin_array( Xprecedence );
		std::for_each( precedences.begin(), precedences.end(), ExportPrecedence( _output, _conf_95 ) );
		_output << end_array();
	    }
	    _output << end_object();
	}

	void
	JSON_Document::ExportEntry::print(Entry const& entry ) const
	{
	    _output << separator() << begin_object()
		    << attribute( Xname, entry.getName() );
	    if ( entry.getComment().size() > 0 ) {
		_output << next_attribute( Xcomment,  entry.getComment() );
	    }
	    if ( entry.getOpenArrivalRate() ) {
		_output << next_attribute( Xopen_arrival_rate, *entry.getOpenArrivalRate() );
	    }
	    if ( entry.getEntryPriority() ) {
		_output << next_attribute( Xpriority, *entry.getEntryPriority() );
	    }

	    switch ( entry.getSemaphoreFlag() ) {
	    case Entry::Semaphore::SIGNAL: _output << next_attribute( Xsemaphore, Xsignal ); break;
	    case Entry::Semaphore::WAIT:   _output << next_attribute( Xsemaphore, Xwait ); break;
	    default: break;
	    }

	    switch ( entry.getRWLockFlag() ) {
	    case Entry::RWLock::READ_UNLOCK:  _output << next_attribute( Xrwlock, Xr_unlock ); break;
	    case Entry::RWLock::READ_LOCK:    _output << next_attribute( Xrwlock, Xr_lock ); break;
	    case Entry::RWLock::WRITE_UNLOCK: _output << next_attribute( Xrwlock, Xw_unlock ); break;
	    case Entry::RWLock::WRITE_LOCK:   _output << next_attribute( Xrwlock, Xw_lock ); break;
	    default: break;
	    }

	    const std::vector<Call *>& forwarding = entry.getForwarding();
	    if ( forwarding.size() > 0 ) {
		_output << next_begin_array( Xforwarding );
		std::for_each( forwarding.begin(), forwarding.end(), ExportCall( _output, Call::Type::FORWARD, _conf_95 ) );
		_output << end_array();
	    }


	    const std::map<unsigned, Phase*>& phases = entry.getPhaseList();
	    if ( entry.getStartActivity() != 0 ) {
		_output << next_attribute( Xstart_activity, entry.getStartActivity()->getName() );
	    } else {
		_output << next_begin_array( Xphase );
		std::for_each( phases.begin(), phases.end(), ExportPhase( _output, _conf_95 ) );
		_output << end_array();
	    }

	    if ( entry.getStartActivity() != NULL && entry.hasHistogram() ) {
		/* Output histogram data for entries specified by activities.  Otherwise, it's output at the phase level. */
		_output << next_begin_array( Xhistogram );
		printPhase( entry, Xdeterministic, ExportHistogram( _output, _conf_95) );
		_output << end_array();
	    }

/*+ JSON-SPEX */
	    if ( !entry.getDocument()->instantiated() ) {
		std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::observations().equal_range( &entry );
		if ( range.first != range.second ) {
		    _output << next_begin_object( Xobserve );
		    std::for_each( range.first, range.second, ExportObservation( _output, _conf_95 ) );
		    _output << end_object();
		}
	    }
/*- JSON-SPEX */

	    if ( entry.hasResults() ) {
		_output << next_begin_object( Xresults );
		_output << attribute( Xthroughput, entry.getResultThroughput(), _conf_95, entry.getResultThroughputVariance() )
			<< next_attribute( Xutilization, entry.getResultUtilization(), _conf_95, entry.getResultUtilizationVariance() )
			<< next_attribute( Xsquared_coeff_variation, entry.getResultSquaredCoeffVariation(), _conf_95, entry.getResultSquaredCoeffVariationVariance() )
			<< next_attribute( Xproc_utilization, entry.getResultProcessorUtilization(), _conf_95, entry.getResultProcessorUtilizationVariance() );
		if ( entry.hasResultsForThroughputBound() ) {
		    _output << next_attribute( Xthroughput_bound, entry.getResultThroughputBound() );
		}
		if ( entry.hasResultsForOpenWait() ) {
		    _output << next_attribute( Xopen_wait_time, entry.getResultWaitingTime(), _conf_95, entry.getResultWaitingTimeVariance() );
		}
		if ( entry.getStartActivity() != NULL ) {
		    _output << entry_phase_results( entry, Xservice_time, &Entry::getResultPhasePServiceTime, _conf_95, &Entry::getResultPhasePServiceTimeVariance )
			    << entry_phase_results( entry, Xservice_time_variance, &Entry::getResultPhasePVarianceServiceTime, _conf_95, &Entry::getResultPhasePVarianceServiceTimeVariance )
			    << entry_phase_results( entry, Xproc_waiting, &Entry::getResultPhasePProcessorWaiting, _conf_95, &Entry::getResultPhasePProcessorWaitingVariance )
			    << entry_phase_results( entry, Xphase_utilization, &Entry::getResultPhasePUtilization, _conf_95, &Entry::getResultPhasePUtilizationVariance );
		}
		_output << end_object();
	    }

	    _output << end_object();
	}

	/*
	 * print out the phases.  don't use std::for_each because the phases are a map, so phase 1 may not exist.  however,
	 * the json version uses arrays, so use null if the phase is not there.
	 */

	void
	JSON_Document::ExportEntry::printPhase( const Entry& entry, const char * name, extvar_fptr get ) const
	{
	    _output << next_begin_array( name, true );
	    const unsigned int n_phases = entry.getMaximumPhase();
	    for ( unsigned int p = 1; p <= n_phases; ++p ) {
		if ( p > 1 ) {
		    _output << ", ";
		}
		if ( entry.hasPhase( p ) ) {
		    Phase * phase = entry.getPhase( p );
		    const ExternalVariable * var = (phase->*get)();
		    if ( var ) {
			_output << print_value( *var );
		    } else {
			_output << 0;
		    }
		} else {
		    _output << 0;
		}
	    }
	    _output << end_array( true );
	}

	void
	JSON_Document::ExportEntry::printPhase( const Entry& entry, const char * name, phtype_fptr get ) const
	{
	    _output << next_begin_array( name, true );
	    const unsigned int n_phases = entry.getMaximumPhase();
	    for ( unsigned int p = 1; p <= n_phases; ++p ) {
		if ( p > 1 ) {
		    _output << ", ";
		}
		if ( entry.hasPhase( p ) ) {
		    Phase * phase = entry.getPhase( p );
		    const Phase::Type type = (phase->*get)();
		    _output << (type == Phase::Type::DETERMINISTIC ? "true" : "false");
		} else {
		    _output << 0;
		}
	    }
	    _output << end_array( true );
	}

	void
	JSON_Document::ExportEntry::printPhase( const Entry& entry, const char * name, const ConfidenceIntervals * conf, double_phase_fptr get ) const
	{
	    _output << next_begin_array( name, true );
	    const unsigned int n_phases = entry.getMaximumPhase();
	    for ( unsigned int p = 1; p <= n_phases; ++p ) {
		if ( p > 1 ) {
		    _output << ", ";
		}
		if ( entry.hasPhase( p ) ) {
		    Phase * phase = entry.getPhase( p );
		    const double value = (phase->*get)();
		    _output << (conf ? (*conf)( value ) : value );
		} else {
		    _output << 0;
		}
	    }
	    _output << end_array( true );
	}

	void
	JSON_Document::ExportEntry::printPhase( const Entry& entry, const char * name, double_entry_fptr get_mean, const ConfidenceIntervals& conf_95, double_entry_fptr get_variance ) const
	{
	    _output << next_begin_array( name, conf_95.is_set() );
	    const unsigned int n_phases = entry.isStandardEntry() ? entry.getMaximumPhase() : 2;
	    for ( unsigned int p = 1; p <= n_phases; ++p ) {
		if ( p > 1 ) {
		    _output << ", ";
		}
		if ( conf_95.is_set() ) {
		    _output << begin_object()
			    << attribute( Xmean, (entry.*get_mean)( p ) )
			    << next_attribute( Xconf_95, conf_95( (entry.*get_variance)( p ) ) )
			    << end_object();
		} else {
		    _output << (entry.*get_mean)( p );
		}
	    }
	    _output << end_array( conf_95.is_set() );
	}

	void
	JSON_Document::ExportEntry::printPhase( const Entry& entry, const char * name, const ExportHistogram& f ) const
	{
	    for ( unsigned int p = 1; p <= entry.getMaximumPhase(); ++p ) {
		if ( p > 1 ) {
		    _output << ", ";
		}
		if ( entry.hasPhase( p ) && entry.getPhase( p )->hasHistogram() ) {
		    f.print( *entry.getPhase( p )->getHistogram() );
		} else {
		    _output << "null";
		}
	    }
	}


	/*
	 * Print out results of the form phase1-utilization="value"* ...
	 * If the ConfidenceInvervals object is present, its
	 * operator()() function is used.
	 */

	std::ostream&
	JSON_Document::ExportEntry::printEntryPhaseResults( std::ostream& output, const Entry & entry, const char * attribute, const double_entry_fptr mean, const LQIO::ConfidenceIntervals& conf_95, const double_entry_fptr variance	 )
	{
	    output << next_begin_array( attribute, !conf_95.is_set() );
	    for ( unsigned int p = 1; p <= entry.getResultPhaseCount(); ++p ) {
		if ( p != 1 ) output << ", ";
		output << print_value( (entry.*mean)(p), conf_95, (entry.*variance)(p) );
	    }
	    output << end_array( !conf_95.is_set() );
	    return output;
	}

	void
	JSON_Document::ExportPhase::print( unsigned int p, const Phase& phase ) const
	{
	    if ( !phase.isPresent() ) return;

	    _output << separator() << begin_object()
		    << attribute( Xphase, static_cast<long>( p ));
	    ExportPhaseActivity::printParameters( phase );
	    ExportPhaseActivity::printResults( phase );
	    _output << end_object();
	}

	/*
	 * Print out information about a call.
	 */

	void
	JSON_Document::ExportCall::operator()( const Call* call ) const
	{
	    if ( call->getCallType() != _type ) return;

	    const Entry& destination = *call->getDestinationEntry();
	    _output << separator() << begin_object()
		    << attribute( Xdestination, destination.getName() )
		    << next_attribute( Xmean_calls, *call->getCallMean() );
/*+ JSON-SPEX */
	    if ( !call->getDocument()->instantiated() ) {
		/* Observations are stored by entry in spex, so, we have to collect them all */
		std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::observations().equal_range( call );
		if ( range.first != range.second ) {
		    _output << next_begin_object( Xobserve );
		    std::for_each( range.first, range.second, ExportObservation( _output, _conf_95 ) );
		    _output << end_object();
		}
	    }
/*- JSON-SPEX */
	    if ( call->hasResults() ) {
		_output << next_begin_object( Xresults );
		print( Xwaiting, *call, &Call::getResultWaitingTime, &Call::getResultWaitingTimeVariance );
		if ( call->hasResultVarianceWaitingTime() ) {
		    _output << ",";
		    print( Xwaiting_variance, *call, &Call::getResultVarianceWaitingTime, &Call::getResultVarianceWaitingTimeVariance );
		}
		if ( call->hasResultDropProbability() ) {
		    _output << ",";
		    print( Xdrop_probability, *call, &Call::getResultDropProbability, &Call::getResultDropProbabilityVariance );
		}
		_output << end_object();
	    }
	    _output << end_object();
	}

	void
	JSON_Document::ExportCall::print( const char * attr, const Call& call, double_call_fptr mean, double_call_fptr variance ) const
	{
	    if ( _conf_95.is_set() ) {
		_output << begin_object( attr )
			<< attribute( Xmean, (call.*mean)() )
			<< next_attribute( Xconf_95, _conf_95( (call.*variance)() ) )
			<< end_object();
	    } else {
		_output << attribute( attr, (call.*mean)() );
	    }
	}


	void
	JSON_Document::ExportPhaseActivity::printParameters( const Phase& phase ) const
	{
	    if ( phase.getComment().size() > 0 ) {
		_output << next_attribute( Xcomment,  phase.getComment() );
	    }
	    if ( phase.getServiceTime() ) {
		_output << next_attribute( Xservice_time, *phase.getServiceTime() );
	    }
	    if ( phase.isNonExponential() ) {
		_output << next_attribute( Xcoeff_of_var_sq, *phase.getCoeffOfVariationSquared() );
	    }
	    if ( phase.hasThinkTime() ) {
		_output << next_attribute( Xthink_time,	 *phase.getThinkTime() );
	    }
	    if ( phase.hasMaxServiceTimeExceeded() ) {
		_output << next_attribute( Xmax_service_time, phase.getHistogram()->getMax() );
	    }
	    if ( phase.hasDeterministicCalls() ) {
		_output << next_attribute( Xdeterministic, true );
	    }

	    const std::vector<Call*>& calls = phase.getCalls();
	    if ( std::any_of( calls.begin(), calls.end(), has_synch_call ) ) {
		_output << next_begin_array( Xsynch_call );
		std::for_each( calls.begin(), calls.end(), ExportCall( _output, Call::Type::RENDEZVOUS, _conf_95 ) );
		_output << end_array();
	    }
	    if ( std::any_of( calls.begin(), calls.end(), has_asynch_call ) ) {
		_output << next_begin_array( Xasynch_call );
		std::for_each( calls.begin(), calls.end(), ExportCall( _output, Call::Type::SEND_NO_REPLY, _conf_95 ) );
		_output << end_array();
	    }
	}

	void
	JSON_Document::ExportPhaseActivity::printResults( const Phase& phase ) const
	{
/*+ JSON-SPEX */
	    if ( !phase.getDocument()->instantiated() ) {
		std::pair<Spex::obs_var_tab_t::const_iterator, Spex::obs_var_tab_t::const_iterator> range = Spex::observations().equal_range( &phase );
		if ( range.first != range.second ) {
		    _output << next_begin_object( Xobserve );
		    std::for_each( range.first, range.second, ExportObservation( _output, _conf_95 ) );
		    _output << end_object();
		}
	    }
/*- JSON-SPEX */

	    if ( phase.hasResults() ) {
		_output << next_begin_object( Xresults );
		_output << attribute( Xutilization, phase.getResultUtilization(), _conf_95, phase.getResultUtilizationVariance() )
			<< next_attribute( Xservice_time, phase.getResultServiceTime(), _conf_95, phase.getResultServiceTimeVariance() )
			<< next_attribute( Xproc_waiting, phase.getResultProcessorWaiting(), _conf_95, phase.getResultProcessorWaitingVariance() );
		if ( dynamic_cast<const Activity*>( &phase ) ) {
		    _output << next_attribute( Xthroughput, phase.getResultThroughput(), _conf_95, phase.getResultThroughputVariance() )
			    << next_attribute( Xproc_utilization, phase.getResultProcessorUtilization(), _conf_95, phase.getResultProcessorUtilizationVariance() );
		}
		if ( phase.getResultVarianceServiceTime() ) {
		    _output << next_attribute( Xservice_time_variance, phase.getResultVarianceServiceTime(), _conf_95, phase.getResultVarianceServiceTimeVariance() );	// optional attribute.
		}
		if ( phase.hasMaxServiceTimeExceeded() ) {
		    _output << next_attribute( Xprob_exceed_max, phase.getResultMaxServiceTimeExceeded(), _conf_95, phase.getResultMaxServiceTimeExceededVariance() );
		}
		if ( phase.hasHistogram() ) {
		    _output << "," << indent() << "\"" << Xhistogram << "\": ";
		    ExportHistogram( _output, _conf_95 ).print( *phase.getHistogram() );
		}
		_output << end_object();
	    }
	}

	void
	JSON_Document::ExportActivity::print( const Activity& activity ) const
	{
	    _output << separator() << begin_object()
		    << attribute( Xname, activity.getName() );
	    ExportPhaseActivity::printParameters( activity );

	    const std::vector<Entry*>& replies = activity.getReplyList();
	    if ( replies.size() > 0 ) {
		_output << next_begin_array( Xreply_to, true );
		for ( std::vector<Entry*>::const_iterator i = replies.begin(); i != replies.end(); ++i ) {
		    if ( i != replies.begin() ) {
			_output << ", ";
		    }
		    _output << "\"" << (*i)->getName() << "\"";
		}
		_output << end_array( true );
	    }

	    ExportPhaseActivity::printResults( activity );
	    _output << end_object();
	}


	const std::map<const ActivityList::Type,const std::string>  JSON_Document::precedence_type_table =
	{
	    { ActivityList::Type::JOIN,		Xpre },
	    { ActivityList::Type::FORK,		Xpost },
	    { ActivityList::Type::AND_FORK,	Xand_fork },
	    { ActivityList::Type::AND_JOIN,	Xand_join },
	    { ActivityList::Type::OR_FORK,	Xor_fork },
	    { ActivityList::Type::OR_JOIN,	Xor_join },
	    { ActivityList::Type::REPEAT,	Xloop }
	};

	void
	JSON_Document::ExportPrecedence::print( const ActivityList& join ) const
	{
	    /* Print the Join (pre) list here... */

	    if ( join.isForkList() ) return;

	    _output << separator()
		    << begin_object()
		    << indent() << "\"" << precedence_type_table.at(join.getListType()) << "\": ";
	    const std::vector<const Activity*>& join_list = join.getList();
	    _output << "[ ";
	    for ( std::vector<const Activity*>::const_iterator activity = join_list.begin(); activity != join_list.end(); ++activity ) {
		if ( activity != join_list.begin() ) {
		    _output << ", ";
		}
		_output <<"\"" << (*activity)->getName() << "\"";
	    }
	    _output << end_array( true );

	    if ( join.getListType() == ActivityList::Type::AND_JOIN ) {
		const AndJoinActivityList& and_join = dynamic_cast<const AndJoinActivityList&>(join);
		const bool quorum = and_join.hasQuorumCount();
		if ( quorum ) {
		    _output << next_attribute( Xquorum, *dynamic_cast<const AndJoinActivityList&>(join).getQuorumCount() );
		}
		if ( and_join.hasHistogram() ) {
		    _output << "," << indent() << "\"" << Xhistogram << "\": ";
		    ExportHistogram( _output, _conf_95 ).print( *join.getHistogram() );
		}
		if ( join.hasResults() ) {
		    _output << next_begin_object( Xresults )
			    << attribute( Xjoin_waiting, and_join.getResultJoinDelay(), _conf_95, and_join.getResultJoinDelayVariance() );
		    if ( and_join.hasResultVarianceJoinDelay() ) {
			_output << next_attribute( Xjoin_variance, and_join.getResultVarianceJoinDelay(), _conf_95, and_join.getResultVarianceJoinDelayVariance() );
		    }
		    _output << end_object();
		}
	    }

	    /* Print the fork (post) list */

	    if ( join.getNext() ) {
		const ActivityList& fork = *join.getNext();
		_output << "," << indent() << "\"" << precedence_type_table.at(fork.getListType()) << "\": [";
		const std::vector<const Activity*>& fork_list = fork.getList();
		for ( std::vector<const Activity*>::const_iterator activity = fork_list.begin(); activity != fork_list.end(); ++activity ) {
		    if ( activity != fork_list.begin() ) {
			_output << ", ";
		    }
		    if ( fork.getListType() == ActivityList::Type::OR_FORK || fork.getListType() == ActivityList::Type::REPEAT ) {
			_output << "{ \"" << (*activity)->getName() << "\": ";
			const ExternalVariable * value = fork.getParameter( *activity );
			if ( value ) {
			    _output << "\"" << *value << "\"";
			} else {
			    _output << "null";
			}
			_output << " }";
		    } else {
			_output <<"\"" << (*activity)->getName() << "\"";
		    }
		}
		_output << end_array( true );
	    }

	    _output << end_object();
	}


	void
	JSON_Document::ExportInputVariables::operator()( const Spex::var_name_and_expr& var ) const
	{
	    _output << separator() << indent() << "\"" << Spex::print_input_variable( var ) << "\"";
	}

	void
	JSON_Document::ExportResultOrConvergenceVariable::operator()( const Spex::var_name_and_expr& var ) const
	{
	    _output << separator() << indent() << "\"" << Spex::print_var_name_and_expr( var ) << "\"";
	}


	void
	JSON_Document::ExportHistogram::print( const Histogram& histogram ) const
	{
	    _output << separator()
		    << begin_object()
		    << attribute( Xnumber_bins, static_cast<double>(histogram.getBins()) )
		    << next_attribute( Xmin, histogram.getMin() )
		    << next_attribute( Xmax, histogram.getMax() );

	    _output << next_begin_array( Xhistogram_bin );
	    const unsigned int n_bins = histogram.getBins() + 2;
	    for ( unsigned int i = 0; i < n_bins; ++i ) {
		if ( i > 0 ) {
		    _output << ",";
		}
		_output << indent()
			<< "{ \"" << Xbegin << "\": " << histogram.getBinBegin(i)
			<< ", \"" << Xend   << "\": " << histogram.getBinEnd(i)
			<< ", \"" << Xprob  << "\": " << histogram.getBinMean(i);
		const double variance = histogram.getBinVariance(i);
		if ( variance ) {
		    _output << ", \"" << Xconf_95 << "\": " << _conf_95( variance );
		}
		_output << " }";
	    }
	    _output << end_array();
	    _output << end_object();
	}


	void
	JSON_Document::ExportFanInOut::print( const std::pair<const std::string, const ExternalVariable *>& fan_in_out ) const
	{
	    _output << separator() << indent() << "{ \"" << fan_in_out.first << "\": " << print_value( *fan_in_out.second ) << " }";
	}

	void
	JSON_Document::ExportObservation::print( const Spex::ObservationInfo& obs ) const
	{
	    _output << separator() << indent() << "\"" << __key_lqx_function_map.at(obs.getKey()) << "\": " << print_value( obs );		/* Should only be one for a processors */
	}
    }
}

namespace LQIO {
    namespace DOM {
	bool
	JSON_Document::has_activities( const std::pair<std::string,Task *>& tp )
	{
	    return tp.second->getActivities().size() > 0;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printIndent( std::ostream& output )
	{
	    if ( __indent < 0 ) {
		__indent = 0;
	    } else {
		output << std::endl;
	    }
	    if ( __indent > 0 ) {
		output << std::setw( __indent * 4 ) << " ";
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printSeparator( std::ostream& output, const Export& self )
	{
	    if ( self._count > 0 ) {
		output << ",";
	    }
	    self._count += 1;
	    return output;
	}


	/* static */ std::ostream&
	JSON_Document::Export::printValue( std::ostream& output, const ExternalVariable& value )
	{
	    std::map<const LQIO::DOM::ExternalVariable *, const LQX::SyntaxTreeNode *>::const_iterator vp;
	    if ( !value.wasSet() ) {
		output << "\"";
		if ( (vp = LQIO::Spex::__inline_expression.find( &value )) != LQIO::Spex::__inline_expression.end() ) {
		    LQX::SyntaxTreeNode::setVariablePrefix( "$" );
		    vp->second->print( output );
		} else {
		    output << value;			/* Should be a string :-) */
		}
		output << "\"";
	    } else if ( value.getType() == ExternalVariable::Type::STRING ) {
		output << "\"" << value << "\"";	/* Should be a string or a variable name */
	    } else {
		output << value;			/* Should be a number :-) */
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printValue( std::ostream& output, const Spex::ObservationInfo& obs )
	{
	    if ( obs.getConfLevel() == 95 ) {
		output << begin_object()
		       << attribute( Xmean, obs.getVariableName() )
		       << next_attribute( Xconf_95, obs.getConfVariableName() )
		       << end_object();
	    } else {
		output << "\"" << obs.getVariableName() << "\"";
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printValue( std::ostream& output, const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance )
	{
	    if ( conf_95.is_set() ) {
		output << begin_object()
		       << attribute( Xmean, mean )
		       << next_attribute( Xconf_95, conf_95( variance ) )
		       << end_object();
	    } else {
		output << mean;
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printAttribute( std::ostream& output, const std::string& name, const double value )
	{
	    output << indent() << "\"" << name << "\": " << value;
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printAttribute( std::ostream& output, const std::string& name, const ExternalVariable& value )
	{
	    output << indent() << "\"" << name << "\": " << print_value( value );
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printAttribute( std::ostream& output, const std::string& name, const long value )
	{
	    output << indent() << "\"" << name << "\": " << value;
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printAttribute( std::ostream& output, const std::string& name, const std::string& value )
	{
	    output << indent() << "\"" << name << "\": \"" << escape_string( value ) << "\"";
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printAttribute( std::ostream& output, const std::string& name, const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance )
	{
	    if ( conf_95.is_set() ) {
		output << begin_object( name )
		       << attribute( Xmean, mean )
		       << next_attribute( Xconf_95, conf_95( variance ) )
		       << end_object();
	    } else {
		output << attribute( name, mean );
	    }
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printAttribute( std::ostream& output, const std::string& name, const Spex::ObservationInfo& obs )
	{
	    output << indent() << "\"" << name << "\":" << print_value( obs );
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printNextAttribute(std::ostream& output, const std::string& name, bool value )
	{
	    output << "," << indent() << "\"" << name << "\": " << (value ? "true" : "false");
	    return output;
	}

	/* static */ std::ostream&
	JSON_Document::Export::printNextAttribute( std::ostream& output, const std::string& name, const double value )
	{
	    output << ",";
	    return printAttribute( output, name, value );
	}

	/* static */ std::ostream&
	JSON_Document::Export::printNextAttribute( std::ostream& output, const std::string& name, const ExternalVariable& value )
	{
	    output << ",";
	    return printAttribute( output, name, value );
	}

	/* static */ std::ostream&
	JSON_Document::Export::printNextAttribute( std::ostream& output, const std::string& name, const long value )
	{
	    output << ",";
	    return printAttribute( output, name, value );
	}

	/* static */ std::ostream&
	JSON_Document::Export::printNextAttribute( std::ostream& output, const std::string& name, const std::string& value )
	{
	    output << ",";
	    return printAttribute( output, name, value );
	}

	/* static */ std::ostream&
	JSON_Document::Export::printNextAttribute( std::ostream& output, const std::string& name, const double mean, const LQIO::ConfidenceIntervals& conf_95, const double variance )
	{
	    output << ",";
	    return printAttribute( output, name, mean, conf_95, variance );
	}

	/* static */ std::ostream&
	JSON_Document::Export::printTimeAttribute( std::ostream& output, const std::string& name, const double time )
	{
	    output << "," << indent() << "\"" << name << "\": \"" << CPUTime::print( time ) <<	"\"";
	    return output;
	}

	/* static */
	std::ostream&
	JSON_Document::Export::escapeString( std::ostream& output, const std::string& s )
	{
	    for ( std::string::const_iterator p = s.begin(); p != s.end() ; ++p ) {	/* Handle special characters in strings */
		switch ( *p ) {
		case '\n':	output << "\\n"; break;
		case '\t':	output << "\\t"; break;
		case '\r':	output << "\\r"; break;
		case '\b':	output << "\\b"; break;
		case '\f':	output << "\\f"; break;
		case '\\':	output << "\\\\"; break;
		case '"':	output << "\\\""; break;
		case ' ':	output << " "; break;
		default:
		    if ( isgraph( *p ) ) {
			output << *p;
		    } else {
			output << "\\u" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(*p) << std::setfill( ' ' );
		    }
		    break;
		}
	    }
	    return output;
	}

	/* static */
	std::ostream& JSON_Document::Export::printArrayBegin( std::ostream& output, const std::string& name, bool inline_array )
	{
	    output << indent() << "\"" << name << "\": [";
	    if ( inline_array ) {
		output << " ";
	    } else {
		__indent += 1;
	    }
	    return output;
	}

	/* static */
	std::ostream& JSON_Document::Export::printNextArrayBegin( std::ostream& output, const std::string& name, bool inline_array )
	{
	    output << ",";
	    return printArrayBegin( output, name, inline_array );
	}

	/* static */
	std::ostream& JSON_Document::Export::printArrayEnd( std::ostream& output, bool inline_array )
	{
	    if ( inline_array ) {
		output << " ";
	    } else {
		if ( __indent > 0 ) {
		    __indent -= 1;
		}
		output << indent();
	    }
	    output << "]";
	    return output;
	}

	/* static */
	std::ostream& JSON_Document::Export::printObjectBegin( std::ostream& output )
	{
	    output << indent() << "{";
	    __indent += 1;
	    return output;
	}

	/* static */
	std::ostream& JSON_Document::Export::printObjectBegin( std::ostream& output, const std::string& name )
	{
	    output << indent() << "\"" << name << "\": {";
	    __indent += 1;
	    return output;
	}

	/* static */
	std::ostream& JSON_Document::Export::printNextObjectBegin( std::ostream& output, const std::string& name )
	{
	    output << ",";
	    return printObjectBegin( output, name );
	}

	/* static */
	std::ostream& JSON_Document::Export::printObjectEnd( std::ostream& output )
	{
	    if ( __indent > 0 ) {
		__indent -= 1;
	    }
	    output << indent() << "}";
	    if ( __indent == 0 ) {
		output << std::endl;
	    }
	    return output;
	}

    }
}

namespace LQIO {
    namespace DOM {

	/* ---------------------------------------------------------------- */
	/* Data.							    */
	/* ---------------------------------------------------------------- */

	const char * JSON_Document::Xactivity				= "activity";
	const char * JSON_Document::Xand_fork				= "and-fork";
	const char * JSON_Document::Xand_join				= "and-join";
	const char * JSON_Document::Xasynch_call			= "asynch-call";
	const char * JSON_Document::Xbegin				= "begin";
	const char * JSON_Document::Xbottleneck_strength		= "bottleneck-strength";
	const char * JSON_Document::Xcap				= "cap";
	const char * JSON_Document::Xcoeff_of_var_sq			= "host-demand-cvsq";
	const char * JSON_Document::Xcomment				= "comment";
	const char * JSON_Document::Xconf_95				= "conf-95";
	const char * JSON_Document::Xconf_99				= "conf-99";
	const char * JSON_Document::Xconvergence			= "convergence";
	const char * JSON_Document::Xconv_val				= "conv-val";
	const char * JSON_Document::Xconv_val_result			= "convergence-value";
	const char * JSON_Document::Xcore				= "core";
	const char * JSON_Document::Xdestination			= "destination";
	const char * JSON_Document::Xdeterministic			= "determinstic-calls";
	const char * JSON_Document::Xdrop_probability			= "drop-probability";
	const char * JSON_Document::Xelapsed_time			= "elapsed-time";
	const char * JSON_Document::Xend				= "end";
	const char * JSON_Document::Xentry				= "entry";
	const char * JSON_Document::Xfanin				= "fan-in";
	const char * JSON_Document::Xfanout				= "fan-out";
	const char * JSON_Document::Xfaults				= "faults";
	const char * JSON_Document::Xforwarding				= "forwarding";
	const char * JSON_Document::Xgeneral				= "general";
	const char * JSON_Document::Xgroup				= "group";
	const char * JSON_Document::Xhistogram				= "histogram";
	const char * JSON_Document::Xhistogram_bin			= "bin";
	const char * JSON_Document::Xinitially				= "initially";
	const char * JSON_Document::Xit_limit				= "it-limit";
	const char * JSON_Document::Xiterations				= "iterations";
	const char * JSON_Document::Xjoin_variance			= "join-variance";
	const char * JSON_Document::Xjoin_waiting			= "join-waiting";
	const char * JSON_Document::Xloop				= "loop";
	const char * JSON_Document::Xmarginal_queue_probabilities	= "marginal-queue-probabilities";
	const char * JSON_Document::Xmax				= "max";
	const char * JSON_Document::Xmax_rss				= "max-rss";
	const char * JSON_Document::Xmax_service_time			= "max-service-time";
	const char * JSON_Document::Xmean				= "mean";
	const char * JSON_Document::Xmean_calls				= "mean-calls";
	const char * JSON_Document::Xmin				= "min";
	const char * JSON_Document::Xmultiplicity			= "multiplicity";
	const char * JSON_Document::Xmva_info				= "mva-info";
	const char * JSON_Document::Xname				= "name";
	const char * JSON_Document::Xnumber_bins			= "bins";
	const char * JSON_Document::Xobserve				= "observe";
	const char * JSON_Document::Xopen_arrival_rate			= "open-arrival-rate";
	const char * JSON_Document::Xopen_wait_time			= "open-wait-time";
	const char * JSON_Document::Xor_fork				= "or-fork";
	const char * JSON_Document::Xor_join				= "or-join";
	const char * JSON_Document::Xoverflow_bin			= "overflow";
	const char * JSON_Document::Xparameters				= "parameters";
	const char * JSON_Document::Xphase				= "phase";
	const char * JSON_Document::Xphase_type_flag			= "phase-type";
	const char * JSON_Document::Xphase_utilization			= "phase-utilization";
	const char * JSON_Document::Xplatform_info			= "platform-info";
	const char * JSON_Document::Xpost				= "post";
	const char * JSON_Document::Xpragma				= "pragma";
	const char * JSON_Document::Xpre				= "pre";
	const char * JSON_Document::Xprecedence				= "precedence";
	const char * JSON_Document::Xprint_int				= "print-int";
	const char * JSON_Document::Xpriority				= "priority";
	const char * JSON_Document::Xprob				= "prob";
	const char * JSON_Document::Xprob_exceed_max			= "prob_exceed_max_service_time";
	const char * JSON_Document::Xproc_utilization			= "proc-utilization";
	const char * JSON_Document::Xproc_waiting			= "proc-waiting";
	const char * JSON_Document::Xprocessor				= "processor";
	const char * JSON_Document::Xquantum				= "quantum";
	const char * JSON_Document::Xqueue_length			= "queue-length";
	const char * JSON_Document::Xquorum				= "quorum";
	const char * JSON_Document::Xr_lock				= "read-lock";
	const char * JSON_Document::Xr_unlock				= "read-unlock";
	const char * JSON_Document::Xreplication			= "replication";
	const char * JSON_Document::Xreply_to				= "reply-to";
	const char * JSON_Document::Xresults				= "results";
	const char * JSON_Document::Xrwlock				= "rwlock";
	const char * JSON_Document::Xrwlock_reader_holding		= "rwlock-reader-holding";
	const char * JSON_Document::Xrwlock_reader_holding_variance	= "rwlock-reader-holding-variance";
	const char * JSON_Document::Xrwlock_reader_utilization		= "rwlock-reader-utilization";
	const char * JSON_Document::Xrwlock_reader_waiting		= "rwlock-reader-waiting";
	const char * JSON_Document::Xrwlock_reader_waiting_variance	= "rwlock-reader-waiting-variance";
	const char * JSON_Document::Xrwlock_writer_holding		= "rwlock-writer-holding";
	const char * JSON_Document::Xrwlock_writer_holding_variance	= "rwlock-writer-holding-variance";
	const char * JSON_Document::Xrwlock_writer_utilization		= "rwlock-writer-utilization";
	const char * JSON_Document::Xrwlock_writer_waiting		= "rwlock-writer-waiting";
	const char * JSON_Document::Xrwlock_writer_waiting_variance	= "rwlock-writer-waiting-variance";
	const char * JSON_Document::Xscheduling				= "scheduling";
	const char * JSON_Document::Xsemaphore				= "semaphore";
	const char * JSON_Document::Xsemaphore_utilization		= "semaphore-waiting";
	const char * JSON_Document::Xsemaphore_waiting			= "semaphore-waiting-variance";
	const char * JSON_Document::Xsemaphore_waiting_variance		= "semaphore-utilization";
	const char * JSON_Document::Xservice_time			= "service-time";
	const char * JSON_Document::Xservice_time_variance		= "service-time-variance";
	const char * JSON_Document::Xservice_type			= "service-time";
	const char * JSON_Document::Xshare				= "share";
	const char * JSON_Document::Xsignal				= "signal";
	const char * JSON_Document::Xsolver_info			= "solver-info";
	const char * JSON_Document::Xspeed_factor			= "rate";
	const char * JSON_Document::Xsquared_coeff_variation		= "squared-coeff-variation";
	const char * JSON_Document::Xstart_activity			= "start-activity";
	const char * JSON_Document::Xstep				= "step";
	const char * JSON_Document::Xstep_squared			= "step-squared";
	const char * JSON_Document::Xsubmodels				= "submodels";
	const char * JSON_Document::Xsynch_call				= "synch-call";
	const char * JSON_Document::Xsystem_cpu_time			= "system-cpu-time";
	const char * JSON_Document::Xtask				= "task";
	const char * JSON_Document::Xthink_time				= "think-time";
	const char * JSON_Document::Xthroughput				= "throughput";
	const char * JSON_Document::Xthroughput_bound			= "throughput-bound";
	const char * JSON_Document::Xtotal				= "total";
	const char * JSON_Document::Xunderflow_bin			= "underflow";
	const char * JSON_Document::Xunderrelax_coeff			= "underrelax-coeff";
	const char * JSON_Document::Xuser_cpu_time			= "user-cpu-time";
	const char * JSON_Document::Xutilization			= "utilization";
	const char * JSON_Document::Xvalid				= "valid";
	const char * JSON_Document::Xw_lock				= "write-lock";
	const char * JSON_Document::Xw_unlock				= "write-unlock";
	const char * JSON_Document::Xwait				= "wait";
	const char * JSON_Document::Xwait_squared			= "wait-squared";
	const char * JSON_Document::Xwaiting				= "waiting";
	const char * JSON_Document::Xwaiting_variance			= "waiting-variance";


	/* Maps srvn_gram.h KEY_XXX to lqx function name */
	const std::map<const int,const char *> JSON_Document::__key_lqx_function_map = {
	    { KEY_ELAPSED_TIME,		Xelapsed_time },
	    { KEY_EXCEEDED_TIME,	Xprob_exceed_max },
	    { KEY_ITERATIONS,		Xiterations },
	    { KEY_PROCESSOR_UTILIZATION,Xproc_utilization },
	    { KEY_PROCESSOR_WAITING,	Xproc_waiting },
	    { KEY_SERVICE_TIME,		Xservice_time },
	    { KEY_SYSTEM_TIME,		Xsystem_cpu_time },
	    { KEY_THROUGHPUT,		Xthroughput },
	    { KEY_THROUGHPUT_BOUND,	Xthroughput_bound },
	    { KEY_USER_TIME,		Xuser_cpu_time },
	    { KEY_UTILIZATION,		Xutilization },
	    { KEY_VARIANCE,		Xservice_time_variance },
	    { KEY_WAITING,		Xwaiting },
	    { KEY_WAITING_VARIANCE,	Xwaiting_variance }
	};


    }
}
