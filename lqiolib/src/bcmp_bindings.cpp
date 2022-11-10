/*
 *  $Id: bcmp_bindings.cpp 16090 2022-11-10 12:40:49Z greg $
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

#include "bcmp_bindings.h"
#include "bcmp_document.h"
#include "error.h"

namespace BCMP {
    const char * __lqx_residence_time           = "residence_time";
    const char * __lqx_response_time            = "response_time";
    const char * __lqx_throughput               = "throughput";
    const char * __lqx_utilization              = "utilization";
    const char * __lqx_queue_length             = "queue_length";
    

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Object] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    LQX::SymbolAutoRef LQXObject::attribute_table_t::operator()( const Model::Result& result ) const { return LQX::Symbol::encodeDouble( (result.*_f)() ); }

    LQX::SymbolAutoRef LQXObject::getPropertyNamed(LQX::Environment* env, const std::string& name) 
    {
	std::map<const std::string,attribute_table_t>::const_iterator attribute = __attributeTable.find( name );
	if ( attribute != __attributeTable.end() && _result != nullptr ) {
	    return attribute->second( *_result );
	}

	/* Anything we don't handle may be handled by our superclass */
	return this->LanguageObject::getPropertyNamed(env, name);
    }

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

    const uint32_t LQXStation::kLQXStationObjectTypeId = 10+1;

    /* Comparison and Operators */
    bool LQXStation::isEqualTo(const LQX::LanguageObject* other) const
    {
	const LQXStation* station = dynamic_cast<const LQXStation *>(other);
	return station && station->getStation() == getStation();  /* Return a comparison of the types */
    }

    std::string LQXStation::description() const
    {
	/* Return a description of the class */
	std::stringstream ss;
//                ss << getTypeName() << "(" << getStation()->getName() << ")";
	return ss.str();
    }

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

        const Model::Station::Class* getClass() const { return dynamic_cast<const Model::Station::Class*>(getObject()); }

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

    /* Comparison and Operators */
    bool LQXChain::isEqualTo(const LQX::LanguageObject* other) const
    {
	const LQXChain* chain = dynamic_cast<const LQXChain *>(other);
	return chain && chain->getChain() == getChain();  /* Return a comparison of the types */
    }

    std::string LQXChain::description() const
    {
	/* Return a description of the class */
	std::stringstream ss;
//                ss << getTypeName() << "(" << getChain()->getName() << ")";
	return ss.str();
    }

    LQX::SymbolAutoRef LQXChain::getPropertyNamed(LQX::Environment* env, const std::string& name) 
    {
	/* Check local attributes for match */

	std::map<const std::string,LQX::SymbolAutoRef>::iterator local = _attributes.find( name );		/* Not used internally but for lqx interface. */
	if ( local != _attributes.end() ) {
	    return local->second;
	}

	/* All the valid properties of classs */
	std::map<const std::string,attribute_table_t>::const_iterator attribute =  __attributeTable.find( name );
	if ( attribute != __attributeTable.end() ) {
	    return attribute->second( _model, _chain );
	}

	/* Anything we don't handle may be handled by our superclass */
	return this->LanguageObject::getPropertyNamed(env, name);
    }

    const std::map<const std::string,LQXChain::attribute_table_t> LQXChain::__attributeTable =
    {
	{ __lqx_response_time,  LQXChain::attribute_table_t( &Model::response_time ) },
	{ __lqx_throughput,     LQXChain::attribute_table_t( &Model::throughput ) }
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

namespace BCMP
{ 
    /* ------------------------------------------------------------------------ */
    /*                              LQX Methods                                 */
    /* ------------------------------------------------------------------------ */

    const BCMP::Model::Station* QNAP2Result::getStation( const BCMP::LQXStation * lqx_station ) const
    {
	if ( lqx_station == nullptr ) {
	    throw std::logic_error( "undefined station" );
	}
	return lqx_station->getStation();
    }
    
    const BCMP::Model::Station::Class& QNAP2Result::getClass( const BCMP::Model::Station * station, const BCMP::LQXChain * lqx_chain ) const
    {
	if ( lqx_chain == nullptr ) {
	    throw LQX::RuntimeException("Undefined chain");
	}
	return station->classAt( lqx_chain->getChain() );
    }


    LQX::SymbolAutoRef QNAP2Result::getResult( BCMP::Model::Result::Type type, LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args )
    {
	static const std::map<BCMP::Model::Result::Type,Result> getResult = {
	    { BCMP::Model::Result::Type::MEAN_SERVICE,	    { "mservice",   &BCMP::Model::Station::mean_service,    &BCMP::Model::Station::Class::mean_service } },
	    { BCMP::Model::Result::Type::QUEUE_LENGTH,	    { "mcustnb",    &BCMP::Model::Station::queue_length,    &BCMP::Model::Station::Class::queue_length } },
	    { BCMP::Model::Result::Type::RESIDENCE_TIME,    { "mresponse",  &BCMP::Model::Station::residence_time,  &BCMP::Model::Station::Class::residence_time } },
	    { BCMP::Model::Result::Type::THROUGHPUT,	    { "mthruput",   &BCMP::Model::Station::throughput,	    &BCMP::Model::Station::Class::throughput } },
	    { BCMP::Model::Result::Type::UTILIZATION,	    { "mbusypct",   &BCMP::Model::Station::utilization,     &BCMP::Model::Station::Class::utilization } }
	};

	if (args.size() == 0) {
	    return LQX::Symbol::encodeDouble( 0. );
	} 
	try {
	    const BCMP::Model::Station* station = getStation( dynamic_cast<BCMP::LQXStation *>( decodeObject(args, 0) ) );
	    if ( args.size() == 1 ) {
		return LQX::Symbol::encodeDouble( (station->*getResult.at(type)._getStationResult)() );
	    } else if ( args.size() == 2 ) {
		return LQX::Symbol::encodeDouble( (getClass( station, dynamic_cast<BCMP::LQXChain *>( decodeObject(args, 0) ) ).*getResult.at(type)._getClassResult)() );
	    } else {
		throw std::logic_error( "arg count" );
	    }
	}
	catch ( std::logic_error& error ) {
	    throw LQX::RuntimeException( getResult.at(type)._name + ": " + error.what() );
	}
	return LQX::Symbol::encodeDouble( 0. );
    }



    LQX::SymbolAutoRef Mbusypct::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args)
    {
	return getResult( BCMP::Model::Result::Type::UTILIZATION, env, args );
    }

    LQX::SymbolAutoRef Mcustnb::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args)
    {
	return getResult( BCMP::Model::Result::Type::QUEUE_LENGTH, env, args );
    }

    LQX::SymbolAutoRef Mresponse::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args)
    {
	return getResult( BCMP::Model::Result::Type::RESIDENCE_TIME, env, args );
    }

    LQX::SymbolAutoRef Mservice::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args)
    {
	return getResult( BCMP::Model::Result::Type::MEAN_SERVICE, env, args );
    }

    LQX::SymbolAutoRef Mthruput::invoke(LQX::Environment* env, std::vector<LQX::SymbolAutoRef >& args)
    {
	return getResult( BCMP::Model::Result::Type::THROUGHPUT, env, args );
    }
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
