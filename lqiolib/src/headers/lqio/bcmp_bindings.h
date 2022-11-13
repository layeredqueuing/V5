/* -*- c++ -*-
 *  $Id: bcmp_bindings.h 16094 2022-11-11 16:53:00Z greg $
 *
 *  Created by Martin Mroz on 16/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __BCMP_BINDINGS__
#define __BCMP_BINDINGS__

#include <lqx/SyntaxTree.h>
#include <lqx/LanguageObject.h>
#include "bcmp_document.h"

/* Forward reference to LQX Environment */
namespace LQX {
    class Environment;
};


/* QNIO DOM Bindings */
namespace BCMP {
    /* Mixin class for QNAP class (chain) and queue (station) attributes. */
    class Attributes {
    public:
	Attributes() : _attributes() {}

	static bool addAttribute( const std::string&, LQX::SyntaxTreeNode * );
	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name);
	
    protected:
	bool add_attribute( const std::string&, LQX::SymbolAutoRef );

    private:
	std::map<const std::string,LQX::SymbolAutoRef> _attributes;		/* Not used internally but for lqx interface. */

	static std::map<const std::string,LQX::SyntaxTreeNode *> __attributes;
    };


    class LQXObject : public LQX::LanguageObject {
    protected:
	typedef double (Model::Result::*get_result_fptr)() const;

	struct attribute_table_t
	{
	    attribute_table_t( get_result_fptr f=nullptr ) : _f(f) {}
	    LQX::SymbolAutoRef operator()( const Model::Result& result ) const;
	    const get_result_fptr _f;
	};

    public:
	LQXObject( uint32_t kLQXobject, const Model::Result * result ) : LQX::LanguageObject(kLQXobject), _result(result) {}

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name);
        const Model::Result* getObject() const { return _result; }

    private:
        const Model::Result * _result;
        static const std::map<const std::string,attribute_table_t> __attributeTable;

    };

    class LQXStation : public LQXObject, public Attributes {
    public:
        const static uint32_t kLQXStationObjectTypeId;
        LQXStation(const Model::Station* station);
        virtual ~LQXStation() {}

        /* Comparison and Operators */
        virtual bool isEqualTo(const LQX::LanguageObject* other) const;
        virtual std::string description() const;
	virtual std::string getTypeName() const { return Model::Station::__typeName; }
	const Model::Station* getStation() const { return dynamic_cast<const Model::Station*>(getObject()); }
	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name);
    };

    class LQXChain : public LQX::LanguageObject, public Attributes {
    protected:
	typedef double (Model::*get_model_fptr)( const std::string& ) const;

	struct attribute_table_t
	{
	    attribute_table_t( get_model_fptr v=nullptr ) : value(v) {}
	    LQX::SymbolAutoRef operator()( const Model& model, const std::string& k ) const { return LQX::Symbol::encodeDouble( (model.*value)(k) ); }
	    const get_model_fptr value;
	};

    public:
        const static uint32_t kLQXChainObjectTypeId = 10+3;

        /* Designated Initializers */
        LQXChain(const BCMP::Model& model, const std::string& chain) : LQX::LanguageObject(kLQXChainObjectTypeId), _model(model), _chain(chain) {}
        virtual ~LQXChain() {}

        /* Comparison and Operators */
        virtual bool isEqualTo(const LQX::LanguageObject* other) const;
        virtual std::string description() const;
	virtual std::string getTypeName() const { return Model::Chain::__typeName; }
        const std::string& getChain() const { return _chain; }
	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name);

    private:
	const BCMP::Model& _model;
        const std::string _chain;
	static const std::map<const std::string,attribute_table_t> __attributeTable;
    };

    /* Names of the bindings */
    extern const char * __lqx_residence_time;
    extern const char * __lqx_response_time;
    extern const char * __lqx_throughput;
    extern const char * __lqx_utilization;
    extern const char * __lqx_queue_length;

    /* Registration of the bindings for the DOM results */
    void RegisterBindings(LQX::Environment* env, const BCMP::Model*);

    /* ------------------------------------------------------------------------ */
    
    /* Output throughput for station (, class). */

    class QNAP2Result : public LQX::Method {
    private:
	typedef double (BCMP::Model::Station::*GetStationResult)() const;
	typedef double (BCMP::Model::Station::Class::*GetClassResult)() const;
	struct Result {
	    Result( const std::string& name, GetStationResult getStationResult, GetClassResult getClassResult ) : 
		_name(name), _getStationResult(getStationResult), _getClassResult(getClassResult) {}

	    const std::string _name;
	    GetStationResult _getStationResult;
	    GetClassResult _getClassResult;
	};
	
    public:
	QNAP2Result(BCMP::Model& model ) : _model(model) {}

    protected:
	LQX::SymbolAutoRef getResult( BCMP::Model::Result::Type type, LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args );

    private:
	const BCMP::Model& model() const { return _model; }
	const BCMP::Model::Station* getStation( const BCMP::LQXStation * ) const;
	const BCMP::Model::Station::Class& getClass( const BCMP::Model::Station *, const BCMP::LQXChain * ) const;
	
    private:
	BCMP::Model& _model;
    };
    
    class Mbusypct : public QNAP2Result {
    public:
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Mbusypct(BCMP::Model& model ) : QNAP2Result(model) {}
	virtual ~Mbusypct() {}
		
	/* All of the glue code to make sure LQX can call solve() */
	virtual std::string getName() const { return "mbusypct"; } 
	virtual const char* getParameterInfo() const { return "o"; } 
	virtual std::string getHelp() const { return "Returns the throughput for the station argument."; } 
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
    };


    class Mcustnb : public QNAP2Result {
    public:
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Mcustnb(BCMP::Model& model ) : QNAP2Result(model) {}
	virtual ~Mcustnb() {}
		
	/* All of the glue code to make sure LQX can call solve() */
	virtual std::string getName() const { return "mcustnb"; } 
	virtual const char* getParameterInfo() const { return "o"; }
	
	virtual std::string getHelp() const { return "Returns the throughput for the station argument."; } 
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
    };


    class Mresponse : public QNAP2Result {
    public:
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Mresponse(BCMP::Model& model ) : QNAP2Result(model) {}
	virtual ~Mresponse() {}
		
	/* All of the glue code to make sure LQX can call solve() */
	virtual std::string getName() const { return "mresponse"; } 
	virtual const char* getParameterInfo() const { return "o"; } 
	virtual std::string getHelp() const { return "Returns the throughput for the station argument."; } 
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
    };

    class Mservice : public QNAP2Result {
    public:
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Mservice(BCMP::Model& model ) : QNAP2Result(model) {}
	virtual ~Mservice() {}
		
	/* All of the glue code to make sure LQX can call solve() */
	virtual std::string getName() const { return "mservice"; } 
	virtual const char* getParameterInfo() const { return "o"; } 
	virtual std::string getHelp() const { return "Returns the throughput for the station argument."; } 
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
    };

    class Mthruput : public QNAP2Result {
    public:
	/* Parameters necessary to call runSolverOnCurrentDOM() */
	Mthruput( BCMP::Model& model ) : QNAP2Result(model) {}
	virtual ~Mthruput() {}
		
	/* All of the glue code to make sure LQX can call solve() */
	virtual std::string getName() const { return "mthruput"; } 
	virtual const char* getParameterInfo() const { return "o+"; } 
	virtual std::string getHelp() const { return "Returns the throughput for the station argument."; } 
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args);
    };
}
#endif /* __BCMP_BINDINGS__ */
