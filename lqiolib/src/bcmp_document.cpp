/* -*- c++ -*-
 * $Id: expat_document.cpp 13764 2020-08-17 19:50:05Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * December 2020.
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <iomanip>
#include <numeric>
#include "bcmp_document.h"

namespace BCMP {

    bool Model::insertClass( const std::string& name, Class::Type type, unsigned int customers, double think_time )
    {
	std::pair<Class_t::iterator,bool> result = _classes.insert( std::pair<const std::string,Class>( name, Class( type, customers, think_time ) ) );
	return result.second;
    }
    
    bool Model::insertStation( const std::string& name, Station::Type type, unsigned int copies)
    {
	std::pair<Station_t::iterator,bool> result = _stations.insert( std::pair<const std::string,Station>( name, Station( type, copies ) ) );
	return result.second;
    }
    
    bool Model::insertDemand( const std::string& station_name, const std::string& class_name, const Station::Demand& demands )
    {
	Station_t::iterator station = _stations.find( station_name );
	if ( station == _stations.end() ) return false;
	return station->second.insertDemand( class_name, demands );
    }
    
    void Model::printJMVA( std::ostream& output ) const
    {
	XML::set_indent(0);
	output << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
	output << XML::start_element( "model" )
	       << XML::attribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" )
	       << XML::attribute( "xsi:noNamespaceSchemaLocation", "JMTmodel.xsd" )
	       << ">" << std::endl;
	      
	output << XML::start_element( "parameters" ) << ">" << std::endl;

	output << XML::start_element( "classes" ) << XML::attribute( "number", nClasses() ) << ">" << std::endl;
	std::for_each( classes().begin(), classes().end(), Class::printJMVAClass( output ) );
	output << XML::end_element( "classes" ) << std::endl;
	
	output << XML::start_element( "stations" ) << XML::attribute( "number", nStations() ) << ">" << std::endl;
	std::for_each( stations().begin(), stations().end(), printJMVAStation( output ) );
	output << XML::end_element( "stations" ) << std::endl;

	output << XML::start_element( "ReferenceStation" ) << XML::attribute( "number", nClasses() ) << ">" << std::endl;
	std::for_each( classes().begin(), classes().end(), Class::printJMVAReference( output ) );
	output << XML::end_element( "ReferenceStation" ) << std::endl;

	output << XML::end_element( "parameters" ) << std::endl;
	output << XML::start_element( "algParams" ) << ">" << std::endl
	       << XML::simple_element( "algType" ) << XML::attribute( "maxSamples", "10000" ) << XML::attribute( "name", "MVA" ) << XML::attribute( "tolerance", "1.0E-7" ) << "/>" << std::endl
	       << XML::simple_element( "compareAlgs" ) << XML::attribute( "value", "false" ) << "/>" << std::endl
	       << XML::end_element( "algParams" ) << std::endl;


	output << XML::end_element( "model" ) << std::endl;
    }

    void
    Model::Class::print_JMVA_class( std::ostream& output, const std::string& name ) const
    {
	if ( isInClosedModel() ) {
	    output << XML::simple_element( "closedclass" )
		   << XML::attribute( "name", name )
		   << XML::attribute( "population", customers() )
		   << "/>" << std::endl;
	}
	if ( isInOpenModel() ) {
	}
    }
    
    void
    Model::Class::print_JMVA_reference( std::ostream& output, const std::string& name ) const
    {
	if ( isInClosedModel() ) {
	    output << XML::simple_element( "Class" )
		   << XML::attribute( "name", name )
		   << XML::attribute( "refStation", "Reference" )
		   << "/>" << std::endl;
	}
	if ( isInOpenModel() ) {
	}
    }


    void
    Model::printQNAP2( std::ostream& output ) const
    {
	output << "/declare/ queue real serv_t		& all stations have this" << std::endl;	
	output << "   ";
	for ( Station_t::const_iterator m = stations().begin(); m != stations().end(); ++m ) {
	    if ( m != stations().begin() ) output << ",";
	    output << m->first;
	}
	output << "	& station identifiers" << std::endl;

	std::for_each( stations().begin(), stations().end(), Model::printQNAP2Station( output, classes() ) );

	output << "/exec/" << std::endl;
	std::for_each( stations().begin(), stations().end(), Model::printQNAP2Variable( output, classes() ) );
	output << "/terminal/" << std::endl;
#if 0
/declare/ integer  n_users;    & global variables

   queue real serv_t;                    & param for every queue  queue.serv_t
   queue choose,server,users,disk,net;   & station identifiers
&
/station/ name = choose;
   service = exp(0.000001);
   transit = server,1,users,1,disk,1, net,1;
&
/station/ name = net;
   service = exp(serv_t);
   type = infinite;
   transit = choose;
&
/station/ name=server;
   service = exp(serv_t);                & in a station, serv_t stands
                                         & for name.serv_t
   transit = choose;
&
/station/ name = disk;
   transit = choose;
   service = exp(serv_t);
&
/station/ name = users;
   init = n_users;                      & params here do not have to be init
   type = infinite;             & standard types infinite, multiple, etc.
                                & default type is single server
   service = exp(serv_t);
   transit = choose,1;
&
/exec/ begin
   &init variables : use the demands
   disk.serv_t:= 0.004;
   server.serv_t := 0.0138;
   users.serv_t:= 2.5;
   net.serv_t:=0.150;
   & loop for multiple solutions
   for n_users := 100,200,300,400,500,600 do
      begin
      & init variables for each if necessary (none here)
      print("no of users = ", n_users);
      solve;       & qnap decides what solver to use (MVA by preference)
                   & take default output variables and format
      end;
   end;
/terminal/
#endif
	std::cerr << "Not implemented" << std::endl;
    }

    /*
     * Prints stations.
     */
    
    void
    Model::printQNAP2Station::operator()( const std::pair<const std::string,Station>& m ) const
    {
	_output << "&" << std::endl
		<< "/station/ name=" << m.first << ";" << std::endl;
	const Station& station = m.second;
	switch ( station.type() ) {
	case Station::REFERENCE: return;
	case Station::DELAY: _output << "   type=infinite;"; break;
	default: break;
	}
	if ( _classes.size() == 1 ) {
	    _output << "   " << "service=exp(serv_t);" << std::endl
		    << "   " << "transit=reference,1;" << std::endl;
	} else {
	} 
    }

    void
    Model::printQNAP2Variable::operator()( const std::pair<const std::string,Station>& m ) const
    {
	/* By class */
	const Station& station = m.second;
	if ( _classes.size() == 1 ) {
	    const Class_t::const_iterator k = _classes.begin();
	    const Station::Demand& demand = station.demandAt(k->first);	// Class name 
	    _output << "   " << m.first << ".serv_t:=" << demand.service_time() << ";" << std::endl;
	}
    }


    bool
    Model::Station::insertDemand( const std::string& class_name, const Demand& demand )
    {
	return _demands.insert( std::pair<const std::string,Demand>( class_name, demand ) ).second;
    }


    /*
     * JMVA insists that service time/visits exist for --all-- classes for --all--stations
     * so pad the demand_map to make it so.
     */

    void
    Model::Station::pad_demand::operator()( const std::pair<const std::string,Station>& m ) const
    {
	for ( Class_t::const_iterator k = _classes.begin(); k != _classes.end(); ++k ) {
	    const std::string& class_name = k->first;
	    Station& station = const_cast<Station&>(m.second);
	    station.insertDemand( class_name, BCMP::Model::Station::Demand() );
	}
    }



    void
    Model::Station::printJMVA( std::ostream& _output, const std::string& name ) const
    {
	std::string element;
	switch ( type() ) {
	case Station::DELAY:
	case Station::REFERENCE:	element = "delaystation"; break;
	case Station::LOAD_INDEPENDENT:	element = "listation"; break;
	default: abort();
	}
	_output << XML::start_element( element ) << XML::attribute( "name", name );
	if ( copies() > 1 ) _output << XML::attribute( "servers", copies() );
	_output << ">" << std::endl;
	_output << XML::start_element( "servicetimes" ) << ">" << std::endl;
	std::for_each( demands().begin(), demands().end(), printJMVAService( _output ) );
	_output << XML::end_element( "servicetimes" ) << std::endl;
	_output << XML::start_element( "visits" ) << ">" << std::endl;
	std::for_each( demands().begin(), demands().end(), printJMVAVisits( _output ) );
	_output << XML::end_element( "visits" ) << std::endl;
	_output << XML::end_element( element ) << std::endl;
    }


    void
    Model::Station::printJMVAService::operator()( const std::pair<const std::string, Demand>& d ) const
    {
	_output << XML::inline_element( "servicetime", "customerClass", d.first, d.second.service_time() ) << std::endl;
    }

    void
    Model::Station::printJMVAVisits::operator()( const std::pair<const std::string, Demand>& d ) const
    {
	_output << XML::inline_element( "servicetime", "customerClass", d.first, d.second.visits() ) << std::endl;
    }

    Model::Station::Demand
    Model::Station::select::operator()( const Model::Station::Demand& augend, const std::pair<const std::string,Model::Station>& station ) const
    {
	const Demand_t& demands = station.second.demands();
	return std::accumulate( demands.begin(), demands.end(), augend, Station::Demand::select( _name ) );
    }

    Model::Station::Demand
    Model::Station::Demand::select::operator()( const Demand& augend, const std::pair<const std::string,Demand>& demand ) const
    {
	if ( demand.first == _name ) return augend + demand.second;
	else return augend;
    }
    
    

    namespace XML {
	static int current_indent = 0;

	int set_indent( int indent )
	{
	    int old_indent = current_indent;
	    current_indent = indent;
	    return old_indent;
	}


	static std::ostream& doIndent( std::ostream& output, int indent )
	{
	    if ( indent < 0 ) {
		if ( current_indent + indent < 0 ) {
		    current_indent = 0;
		} else {
		    current_indent += indent;
		}
	    }
	    if ( current_indent != 0 ) {
		output << std::setw( current_indent * 3 ) << " ";
	    }
	    if ( indent > 0 ) {
		current_indent += indent;
	    }
	    return output;
	}

	static std::ostream& doTempIndent( std::ostream& output, int indent )
	{
	    output << std::setw( (current_indent + indent) * 3 ) << " ";
	    return output;
	}

	static std::ostream& printStartElement( std::ostream& output, const std::string& element, bool complex_element )
	{
	    output << indent( complex_element ? 1 : 0  ) << "<" << element;
	    return output;
	}

	static std::ostream& printEndElement( std::ostream& output, const std::string& element, bool complex_element )
	{
	    if ( complex_element ) {
		output << indent( -1 ) << "</" << element << ">";
	    } else {
		output << "/>";
	    }
	    return output;
	}

	static std::ostream& printInlineElement( std::ostream& output, const std::string& e, const std::string& a, const std::string& v, double d )
	{
	    output << indent( 0 ) << "<" << e << attribute( a, v )  << ">" << d << "</" << e << ">";
	    return output;
	}
    
	static std::ostream& printAttribute( std::ostream& output, const std::string& a, const std::string& v )
	{
	    output << " " << a << "=\"" << v << "\"";
	    return output;
	}
    
	static std::ostream& printAttribute( std::ostream& output, const std::string&  a, double v )
	{
	    output << " " << a << "=\"" << v << "\"";
	    return output;
	}
    
	static std::ostream& printAttribute( std::ostream& output, const std::string&  a, unsigned int v )
	{
	    output << " " << a << "=\"" << v << "\"";
	    return output;
	}

	IntegerManip indent( int i ) { return IntegerManip( &doIndent, i ); }
	IntegerManip temp_indent( int i ) { return IntegerManip( &doTempIndent, i ); }
	BooleanManip start_element( const std::string& e, bool b ) { return BooleanManip( &printStartElement, e, b ); }
	BooleanManip end_element( const std::string& e, bool b ) { return BooleanManip( &printEndElement, e, b ); }
	BooleanManip simple_element( const std::string& e ) { return BooleanManip( &printStartElement, e, false ); }
	InlineElementManip inline_element( const std::string& e, const std::string& a, const std::string& v, double d ) { return InlineElementManip( &printInlineElement, e, a, v, d ); }
	StringManip attribute( const std::string& a, const std::string& v ) { return StringManip( &printAttribute, a, v ); }
	DoubleManip attribute( const std::string&a, double v ) { return DoubleManip( &printAttribute, a, v ); }
	UnsignedManip attribute( const std::string&a, unsigned v ) { return UnsignedManip( &printAttribute, a, v ); }
    }
}
