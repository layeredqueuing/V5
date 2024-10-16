/* -*- c++ -*-
 * Generate MVA program for given layer.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: generate.h 17378 2024-10-16 23:25:26Z greg $
 *
 * ------------------------------------------------------------------------
 */

class Entity;
class Task;
class Submodel;
class MVASubmodel;

#include <map>
#include <set>
#include "pragma.h"
#include <lqio/bcmp_document.h>
template <typename Type> class Vector;

class Generate {
    enum class Multiserver { DEFAULT, CONWAY, REISER, REISER_PS, ROLIA, ROLIA_PS, ZHOU };

public:
    enum class Output { NONE, LIBMVA, JMVA, QNAP };

    static void output( const Vector<Submodel *>& submodels );

public:
    static std::string __directory_name;
    static Output __mode;

/* -------------------------------------------------------------------- */
/*                    Output LibMVA source code.                        */
/* -------------------------------------------------------------------- */

private:
    class LibMVA {
	class ArgsManip {
	public:
	    ArgsManip( std::ostream& (*f)(std::ostream&, const unsigned, const unsigned, const unsigned ), const unsigned e, const unsigned k, const unsigned p ) :
		_f(f), _e(e), _k(k), _p(p) {}

	private:
	    std::ostream& (*_f)( std::ostream&, const unsigned, const unsigned, const unsigned );
	    const unsigned _e;
	    const unsigned _k;
	    const unsigned _p;

	    friend std::ostream& operator<<(std::ostream & os, const ArgsManip& m ) { return m._f(os,m._e,m._k,m._p); }
	};

	class SRVNManip {
	public:
	    SRVNManip( std::ostream& (*f)(std::ostream&, const Entity& ), const Entity& s ) : _f(f), _stn(s) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const Entity& );
	    const Entity& _stn;

	    friend std::ostream& operator<<(std::ostream & os, const SRVNManip& m ) { return m._f(os,m._stn); }
	};
    
    public:
	static void program( const Submodel* );
	static void makefile( const unsigned );
	
    private:
	LibMVA( const MVASubmodel& );

	static bool find_libmva( std::string& pathname );
    
	std::ostream& print( std::ostream& ) const;
	std::ostream& printClientStation( std::ostream& output, const Task& aClient ) const;
	std::ostream& printServerStation( std::ostream& output, const Entity& aServer ) const;
	std::ostream& printInterlock( std::ostream& output, const Entity& aServer ) const;

	static ArgsManip station_args( const unsigned e, const unsigned k, const unsigned p ) { return ArgsManip( &print_station_args, e, k, p ); }
	static ArgsManip overtaking_args( const unsigned e, const unsigned k, const unsigned p ) { return ArgsManip( &print_overtaking_args, e, k, p ); }
	static SRVNManip station_name( const Entity& entity ) { return SRVNManip( &print_station_name, entity ); }

	static std::ostream& print_station_name( std::ostream& output, const Entity& anEntity );
	static std::ostream& print_station_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p );
	static std::ostream& print_overtaking_args( std::ostream& output, const unsigned e, const unsigned k, const unsigned p );

	static char remap( char );

	const MVASubmodel& _submodel;
	const unsigned K;			/* Number of chains */
	static const std::vector<std::string> __includes;
	static const std::vector<struct option> __longopts;
	static const std::map<const char, const std::string> __help;
	static const std::map<const int,const std::string> __argument_type;
	static const std::map<const Pragma::MVA,const std::string> __solvers;
	static std::map<const Pragma::Multiserver,std::string> __multiservers;
	static const std::map<const Pragma::Multiserver,const std::pair<const std::string&,const std::string&> > __stations;

    };

/* -------------------------------------------------------------------- */
/*                        Output BCMP Model                             */
/* -------------------------------------------------------------------- */

    class BCMP_Model {
    public:
#if HAVE_LIBEXPAT
	static void serialize_JMVA( const Submodel* );
#endif
	static void serialize_QNAP( const Submodel* );

    protected:
	typedef void (*serialize_fptr)( std::ostream&, const std::string&, const BCMP::Model& );
	static void serialize( const Submodel *, serialize_fptr );
	
    private:
	BCMP_Model( const MVASubmodel& );
	void construct();
	void setComment( const std::string& );

	const std::set<Task *>& clients() const;
	const std::set<Entity *>& servers() const;
	unsigned int number() const;
	double think_time( unsigned int ) const;
	
	void createChain( const Task& );
	void createStation( const Entity& );

	BCMP::Model::Station::Type stationType( const Entity& ) const;

    private:
	const MVASubmodel& _submodel;
	const unsigned int K;		/* Number of chains	*/
	BCMP::Model _model;		/* Output 		*/
	BCMP::Model::Station& _reference;
	std::map<unsigned int,const std::string> _chains;
    };


#if HAVE_LIBEXPAT
    class JMVA_Document : private BCMP_Model {
    public:
	static void serialize( std::ostream& output, const std::string&, const BCMP::Model& model );
    };
#endif
    
    class QNAP_Document : private BCMP_Model {
    public:
	static void serialize( std::ostream& output, const std::string&, const BCMP::Model& model );
    };
    
};
