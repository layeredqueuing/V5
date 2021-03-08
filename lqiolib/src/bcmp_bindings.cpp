/*
 *  $Id: bcmp_bindings.cpp 14530 2021-03-08 03:01:26Z greg $
 *
 *  Created by Martin Mroz on 16/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <cstdio>
#include <sstream>
#include <cstring>
#include <lqx/Environment.h>
#include <lqx/SymbolTable.h>
#include <lqx/MethodTable.h>
#include <lqx/LanguageObject.h>
#include <lqx/Array.h>
#include <lqx/RuntimeException.h>

#include "bcmp_document.h"

namespace BCMP {
    const char * __lqx_residence_time           = "residence_time";
    const char * __lqx_response_time            = "response_time";
    const char * __lqx_throughput               = "throughput";
    const char * __lqx_utilization              = "utilization";
    const char * __lqx_queue_length             = "queue_length";
    

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Object] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    class LQXObject : public LQX::LanguageObject {
    protected:
	typedef double (Model::Result::*get_result_fptr)() const;

	struct attribute_table_t
	{
	    attribute_table_t( get_result_fptr m=nullptr ) : mean(m) {}
	    LQX::SymbolAutoRef operator()( const Model::Result& result ) const { return LQX::Symbol::encodeDouble( (result.*mean)() ); }
	    const get_result_fptr mean;
	};

    public:
	LQXObject( uint32_t kLQXobject, const Model::Result * result ) : LQX::LanguageObject(kLQXobject), _result(result)
	    {
	    }

	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name) 
	    {
		std::map<const std::string,attribute_table_t>::const_iterator attribute =  __attributeTable.find( name.c_str() );
		if ( attribute != __attributeTable.end() ) {
		    try {
			if ( _result ) {
			    return attribute->second( *_result );
			}
		    }
		    catch ( const LQIO::should_implement& e ) {
		    }
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

        const Model::Result* getObject() const { return _result; }

    protected:

        const Model::Result * _result;
        static const std::map<const std::string,attribute_table_t> __attributeTable;

    };

    const std::map<const std::string,LQXObject::attribute_table_t> LQXObject::__attributeTable =
    {
	{ __lqx_residence_time,	attribute_table_t( &Model::Result::residence_time ) },
	{ __lqx_throughput,     attribute_table_t( &Model::Result::throughput ) },
	{ __lqx_utilization,    attribute_table_t( &Model::Result::utilization ) },
	{ __lqx_queue_length,   attribute_table_t( &Model::Result::queue_length ) }
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Station] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    /* Note: A station is a subclass of a result */

    class LQXStation : public LQXObject {
    public:

        const static uint32_t kLQXStationObjectTypeId = 10+1;

        /* Designated Initializers */
        LQXStation(const Model::Station* station) : LQXObject(kLQXStationObjectTypeId,station)
            {
            }

        virtual ~LQXStation()
            {
            }

        /* Comparison and Operators */
        virtual bool isEqualTo(const LQX::LanguageObject* other) const
            {
                const LQXObject* object = dynamic_cast<const LQXObject *>(other);
                return object && object->getObject() == getObject();  /* Return a comparison of the types */
            }

        virtual std::string description() const
            {
                /* Return a description of the class */
                std::stringstream ss;
//                ss << getTypeName() << "(" << getStation()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return Model::Station::__typeName;
	    }

	const Model::Station* getStation() const { return dynamic_cast<const Model::Station*>(_result); }
    };

    class LQXGetStation : public LQX::Method {
    public:
	LQXGetStation(const BCMP::Model* model) : _model(model), _symbolCache() {}
	virtual ~LQXGetStation() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return Model::Station::__typeName; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the station associated with a name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the class and look it up in cache */
	    const std::string stationName = decodeString(args, 0);
	    if (_symbolCache.find(stationName) != _symbolCache.end()) {
		return _symbolCache[stationName];
	    }

	    /* Obtain the station reference  */
	    try {
		/* Return an encapsulated reference to the station */
		LQXStation* stationObject = new LQXStation(&_model->stationAt(stationName));
		_symbolCache[stationName] = LQX::Symbol::encodeObject(stationObject, false);
		return _symbolCache[stationName];
	    }
	    catch ( const std::out_of_range& e ) {
		throw LQX::RuntimeException( "No station specified with name ", stationName.c_str() );
		return LQX::Symbol::encodeNull();
	    }
	}

    private:
	const Model* _model;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Class] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    class LQXClass : public LQXObject
    {
    public:

	const static uint32_t kLQXClassObjectTypeId = 10+2;

	/* Designated Initializers */
        LQXClass(const Model::Station::Class* class_) : LQXObject(kLQXClassObjectTypeId,class_)
            {
            }

	virtual ~LQXClass()
	    {
	    }

	/* Comparison and Operators */
	virtual bool isEqualTo(const LQX::LanguageObject* other) const
	    {
		const LQXClass* class_ = dynamic_cast<const LQXClass *>(other);
		return class_ && class_->getClass() == getClass();	/* Return a comparison of the types */
	    }

	virtual std::string description() const
	    {
		/* Return a description of the class */
                std::stringstream ss;
//                ss << getTypeName() << "(" << getClass()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return Model::Station::Class::__typeName;
	    }

        const Model::Station::Class* getClass() const { return dynamic_cast<const Model::Station::Class*>(_result); }

    private:
    };

    class LQXGetClass : public LQX::Method {
    public:
	LQXGetClass() {}
	virtual ~LQXGetClass() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return Model::Station::Class::__typeName; }
	virtual const char* getParameterInfo() const { return "os"; }
	virtual std::string getHelp() const { return "Returns the class associated with a name for station."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the class and look it up in cache */
	    LQX::LanguageObject* lo = decodeObject(args, 0);
	    const std::string className = decodeString(args, 1);
	    LQXStation* lqx_station = dynamic_cast<LQXStation *>(lo);

	    /* Make sure that what we have is a station */
	    if ( !lqx_station ) {
		throw LQX::RuntimeException("No class specified with name `%s'.", className.c_str());
		return LQX::Symbol::encodeNull();
	    }

	    /* Obtain the class from the station */
	    const BCMP::Model::Station* station = lqx_station->getStation();
	    const BCMP::Model::Station::Class* classObject = &station->classAt(className);
	    return LQX::Symbol::encodeObject(new LQXClass(classObject), false);
	}
    };

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Chain] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

    class LQXChain : public LQX::LanguageObject {
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
        LQXChain(const BCMP::Model& model, const std::string& chain) : LQX::LanguageObject(kLQXChainObjectTypeId), _model(model), _chain(chain)
            {
            }

        virtual ~LQXChain()
            {
            }

        /* Comparison and Operators */
        virtual bool isEqualTo(const LQX::LanguageObject* other) const
            {
                const LQXChain* chain = dynamic_cast<const LQXChain *>(other);
                return chain && chain->getChain() == getChain();  /* Return a comparison of the types */
            }

        virtual std::string description() const
            {
                /* Return a description of the class */
                std::stringstream ss;
//                ss << getTypeName() << "(" << getChain()->getName() << ")";
		return ss.str();
	    }

	virtual std::string getTypeName() const
	    {
		return Model::Chain::__typeName;
	    }

        const std::string& getChain() const { return _chain; }


	virtual LQX::SymbolAutoRef getPropertyNamed(LQX::Environment* env, const std::string& name) 
	    {
		/* All the valid properties of classs */
		std::map<const std::string,attribute_table_t>::const_iterator attribute =  __attributeTable.find( name.c_str() );
		if ( attribute != __attributeTable.end() ) {
		    try {
			return attribute->second( _model, _chain );
		    }
		    catch ( const LQIO::should_implement& e ) {
		    }
		}

		/* Anything we don't handle may be handled by our superclass */
		return this->LanguageObject::getPropertyNamed(env, name);
	    }

    private:
	const BCMP::Model& _model;
        const std::string _chain;

	const std::map<const std::string,attribute_table_t> __attributeTable =
	{
	    { __lqx_response_time,  attribute_table_t( &Model::response_time ) },
	    { __lqx_throughput,     attribute_table_t( &Model::throughput ) }
	};
    };

    class LQXGetChain : public LQX::Method {
    public:
	LQXGetChain(const BCMP::Model* model) : _model(model), _symbolCache() {}
	virtual ~LQXGetChain() {}

	/* Basic information for the method itself */
	virtual std::string getName() const { return Model::Chain::__typeName; }
	virtual const char* getParameterInfo() const { return "s"; }
	virtual std::string getHelp() const { return "Returns the chain associated with a name."; }

	/* Invocation of the method from the language */
	virtual LQX::SymbolAutoRef invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args) {

	    /* Decode the name of the class and look it up in cache */
	    const std::string chainName = decodeString(args, 0);
	    if (_symbolCache.find(chainName) != _symbolCache.end()) {
		return _symbolCache[chainName];
	    }

	    /* Obtain the chain reference  */
	    try {
		/* Return an encapsulated reference to the chain */
		LQXChain* chainObject = new LQXChain(*_model,chainName);
		_symbolCache[chainName] = LQX::Symbol::encodeObject(chainObject, false);
		return _symbolCache[chainName];
	    }
	    catch ( const std::out_of_range& e ) {
		throw LQX::RuntimeException( "No chain specified with name ", chainName.c_str() );
		return LQX::Symbol::encodeNull();
	    }
	}

    private:
	const Model* _model;
	std::map<std::string,LQX::SymbolAutoRef> _symbolCache;
    };
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#pragma mark -

namespace BCMP {

    void RegisterBindings(LQX::Environment* env, const Model* model)
    {
	LQX::MethodTable* mt = env->getMethodTable();
	mt->registerMethod(new LQXGetClass());
	mt->registerMethod(new LQXGetStation(model));
	mt->registerMethod(new LQXGetChain(model));
    }
}
