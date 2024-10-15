/* -*- c++ -*-
 * open.h	-- Greg Franks
 *
 * $Id: open.h 17368 2024-10-15 21:03:38Z greg $
 */

#ifndef _OPEN_H
#define _OPEN_H

#include "lqn2ps.h"
#include <vector>
#include "task.h"

class OpenArrival;
class OpenArrivalSource;

/* ------------- Open Arrival Pseudo Tasks (for drawing)  ------------- */

class OpenArrivalSource : public Task {		// "Clients", like reference tasks
public:
    OpenArrivalSource( Entry * source );
    ~OpenArrivalSource();
    virtual OpenArrivalSource * clone( unsigned int, const std::string& aName, const Processor * aProcessor, const Share * aShare ) const { return 0; }

    virtual const std::string& name() const;
    
    virtual OpenArrivalSource& rename() { return *this; }	/* Don't bother */
    virtual OpenArrivalSource& squish( std::map<std::string,unsigned>&, std::map<std::string,std::string>& ) { return *this; }	/* Don't bother */
    virtual OpenArrivalSource& moveSrc( const Point& aPoint );
    double utilization() const { return 0.0; }

    const std::vector<OpenArrival *>& calls() const { return _calls; }

    virtual bool check() const { return true; }
    virtual unsigned referenceTasks( std::vector<Entity *>&, Element * ) const { return 0; }	/* We don't have clients */
    virtual unsigned clients( std::vector<Entity *>&, const callPredicate = 0 ) const { return 0; }	/* We don't have clients */
    virtual unsigned servers( std::vector<Entity *>& ) const;

    void addSrcCall( OpenArrival * );
    void removeSrcCall( OpenArrival * );

    virtual bool isInOpenModel( const std::vector<Entity *>& servers ) const;
    virtual bool isSelectedIndirectly() const;
    
    virtual unsigned setChain( unsigned, callPredicate aFunc ) const;
    virtual OpenArrivalSource& aggregate();

    virtual double radius() const;

    virtual OpenArrivalSource& format() { return *this; }
    virtual OpenArrivalSource& reformat() { return *this; }
    
    virtual OpenArrivalSource& moveBy( const double dx, const double dy ) { Element::moveBy( dx, dy ); return *this; }
    virtual OpenArrivalSource& moveTo( const double x, const double y );
    virtual OpenArrivalSource& scaleBy( const double, const double );
    virtual OpenArrivalSource& translateY( const double );
    virtual OpenArrivalSource& depth( const unsigned );

    virtual OpenArrivalSource& label();

    virtual Graphic::Colour colour() const;


    virtual const OpenArrivalSource& draw( std::ostream& output ) const;
    virtual std::ostream& drawClient( std::ostream&, const bool is_in_open_model, const bool is_in_closed_model ) const;
    virtual std::ostream& print( std::ostream& output ) const;

private:
    const Entry& entry() const { return *_entries.at(0); }

public:
    static std::vector<OpenArrivalSource *> __source;
    
private:
    std::vector<OpenArrival *> _calls;	/* Arc calling entry		*/
};
#endif
