/*
 *  $Id: bcmp_bindings.cpp 16188 2022-12-20 22:11:21Z greg $
 *
 *  Created by Martin Mroz on 16/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <algorithm>
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
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- [Attributes] */
/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

    std::map<const std::string,LQX::SyntaxTreeNode *> Attributes::__attributes;
    
    /* Mixin */

    bool Attributes::add_attribute( const std::string& name, LQX::SymbolAutoRef value )
    {
	return _attributes.emplace( name, value ).second;
    }

    LQX::SymbolAutoRef Attributes::getPropertyNamed(LQX::Environment* env, const std::string& name) 
    {
	/* Already been used. */
	std::map<const std::string,LQX::SymbolAutoRef>::const_iterator attribute = _attributes.find( name );
	if ( attribute != _attributes.end() ) return attribute->second;

	/* See if it's a valid attribute, but never used */
	std::map<const std::string,LQX::SyntaxTreeNode *>::const_iterator defined = __attributes.find( name );
	if ( defined == __attributes.end() ) return LQX::Symbol::encodeNull();	/* Not defined */

	/* Duplicate the template value and make it writable */
	LQX::SymbolAutoRef src = defined->second->invoke( env );
	LQX::SymbolAutoRef dst;
	if ( src->getType() == LQX::Symbol::SYM_OBJECT && dynamic_cast<LQX::ArrayObject *>(src->getObjectValue()) ) {
	    LQX::ArrayObject * array = dynamic_cast<LQX::ArrayObject *>(src->getObjectValue());
	    dst = LQX::Symbol::encodeObject( std::for_each( array->begin(), array->end(), BCMP::Attributes::initialize( new LQX::ArrayObject() ) ).dst(), false );
	} else {
	    dst = LQX::Symbol::duplicate( src );
	}
	dst->setIsConstant( false );
	return _attributes.emplace( name, dst ).first->second;
    }
    
    /* static */ bool Attributes::addAttribute( const std::string& name, LQX::SyntaxTreeNode * value )
    {
	return __attributes.emplace( name, value ).second;
    }

    /* Copy over the array keys, but create a new array value */
    /* static */ void Attributes::initialize::operator()( std::pair<LQX::SymbolAutoRef,LQX::SymbolAutoRef> item )
    {
	LQX::SymbolAutoRef value = LQX::Symbol::encodeNull();
	value->setIsConstant( false );
	value->copyValue( *item.second );
	_dst->put( item.first, value );
    }

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
	/* Return a description of the class, in this case the station name attribute.  */
	LQX::SymbolAutoRef property = const_cast<LQXStation *>(this)->Attributes::getPropertyNamed(nullptr, "name");
	return property->getStringValue();
    }

    LQX::SymbolAutoRef LQXStation::getPropertyNamed(LQX::Environment* env, const std::string& name) 
    {
	/* Check local attributes for match */

	LQX::SymbolAutoRef property = Attributes::getPropertyNamed(env, name);
	if ( property->getType() != LQX::Symbol::SYM_NULL ) {
	    return property;
	} else {
	    /* Anything we don't handle may be handled by our superclass */
	    return LQXObject::getPropertyNamed(env, name);
	}
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

	    /* Return an encapsulated reference to the station */
	    try {
		if (_symbolCache.find(stationName) == _symbolCache.end()) {
		    _symbolCache[stationName] = LQX::Symbol::encodeObject( new LQXStation(&_model->stationAt(stationName)), false );
		}
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
	LQX::SymbolAutoRef property = Attributes::getPropertyNamed(env, name);
	if ( property->getType() != LQX::Symbol::SYM_NULL ) {
	    return property;
	}

	std::map<const std::string,attribute_table_t>::const_iterator attribute = __attributeTable.find( name );
	if ( attribute != __attributeTable.end() ) {
	    /* All the valid properties of classs */
	    return attribute->second( _model, _chain );
	} else {
	    /* Anything we don't handle may be handled by our superclass */
	    return this->LanguageObject::getPropertyNamed(env, name);
	}
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

	    /* Return an encapsulated reference to the chain */
	    try {
		if (_symbolCache.find(chainName) == _symbolCache.end()) {
		    _symbolCache[chainName] = LQX::Symbol::encodeObject( new LQXChain(*_model,chainName), false );
		}
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

    LQXStation::LQXStation(const Model::Station* station) : LQXObject(kLQXStationObjectTypeId,station), Attributes()
    {
	LQX::SymbolAutoRef value = LQX::Symbol::encodeString("");
	value->setIsConstant(false);
	Attributes::add_attribute( "name", value );
    }

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
		return LQX::Symbol::encodeDouble( (getClass( station, dynamic_cast<BCMP::LQXChain *>( decodeObject(args, 1) ) ).*getResult.at(type)._getClassResult)() );
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
