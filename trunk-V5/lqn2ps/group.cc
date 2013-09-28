/* group.cc	-- Greg Franks Thu Mar 24 2005
 *
 * $Id$
 */

#include "group.h"
#include <cstring>
#include <cstdlib>
#if HAVE_FLOAT_H
#include <float.h>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif
#include "node.h"
#include "label.h"
#include "processor.h"
#include "task.h"
#include "share.h"
#include "model.h"

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif


/* ---------------------- Overloaded Operators ------------------------ */

ostream& operator<<( ostream& output, const Group& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_SRVN:
	break;
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
	break;
#endif
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
	break;
#endif
    case FORMAT_XML:
	break;
    default:
	self.draw( output );
	break;
    }

    return output;
}

Group::Group( const string& s ) 
    : myName(s), used(false)
{
    myNode = Node::newNode( 0, 0 );
    myLabel = Label::newLabel();
}


Group::~Group() 
{
    delete myNode;
    delete myLabel;
}

bool
Group::match( const string& s ) const
{
    return myName == s;
}


/* 
 * Locate all tasks associated with the selected processors.
 */

Group&
Group::format( const unsigned MAX_LEVEL )
{
    layer.grow(MAX_LEVEL);

    if ( !populate() ) return *this;

    /* Resort the layers to force tasks making calls right */

    for ( unsigned i = 1; i <= layer.size(); ++i ) {
	layer[i].sort( Entity::compare );
    }	

    /* Now, move all entities for this processor together */

    origin( MAXDOUBLE, MAXDOUBLE ).extent( 0, 0 );
    for ( unsigned i = MAX_LEVEL; i > 0; --i ) {
	if ( layer[i].size() == 0 ) continue;
	layer[i].reformat();
	originMin( layer[i].x(), layer[i].y() );
	extentMax( layer[i].x() + layer[i].width(), layer[i].y() + layer[i].height() );
    }
    extent( width(), height() - y() );

    /* Justify the current "slice", then move it to its column */

    for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
	layer[i].justify( width() );
    }

    return *this;
}


bool
Group::populate()
{
    /* Loop here to go through all processors on a group */

    bool empty = true;
    for ( set<Processor *,ltProcessor>::const_iterator nextProcessor = ::processor.begin(); nextProcessor != ::processor.end(); ++nextProcessor ) {
	Processor * aProcessor = *nextProcessor;
	if ( aProcessor->hasGroup() || !match(aProcessor->name()) ) continue;
	aProcessor->hasGroup( true );

	if ( Flags::print[LAYERING].value.i == LAYERING_PROCESSOR ) {
	    penColour( aProcessor->colour() == Graphic::GREY_10 ? Graphic::BLACK : aProcessor->colour() );
	    fillColour( aProcessor->colour() );
	}
	if ( aProcessor->isSelected() ) {
	    layer[aProcessor->level()] << aProcessor;
	    isUsed( true );
	    empty = false;
	}

	for ( set<Task *,ltTask>::const_iterator nextTask = task.begin(); nextTask != task.end(); ++nextTask ) {
	    Task * aTask = *nextTask;
	    if ( aTask->processor() == aProcessor && aTask->isSelectedIndirectly() ) {
		layer[aTask->level()] << aTask;
		if ( !submodel_output() || !aTask->isSelected() ) {
		    isUsed( true );
		}
		empty = false;
	    }
	}
    }

    return !empty;
}



Group const&
Group::resizeBox() const
{
    /* Now adjust the box (the group) and label */
	
    Point& anOrigin = myNode->origin;
    Point& anExtent = myNode->extent;

    anOrigin.moveBy( -4.5, -(4.5 + Flags::print[FONT_SIZE].value.i * 1.2) );
    anExtent.moveBy( 9.0, 9.0 + Flags::print[FONT_SIZE].value.i * 1.2 );
    return *this;
}


Group const& 
Group::positionLabel() const
{
    Point& anOrigin = myNode->origin;
    Point& anExtent = myNode->extent;
    myLabel->moveTo( anOrigin.x() + anExtent.x() / 2.0, 
		     anOrigin.y() + Flags::print[FONT_SIZE].value.i * 0.6 );
    return *this;
}


Group&
Group::label()
{
    *myLabel << myName;
    return *this;
}


Group& 
Group::origin( const double an_x, const double a_y ) 
{ 
    myNode->origin.moveTo( an_x, a_y ); 
    return *this; 
}


Group& 
Group::extent( const double x, const double y ) 
{ 
    myNode->extent.moveTo( x, y ); 
    return *this; 
}


Group& 
Group::originMin( const double x, const double y )
{
    myNode->origin.min( x, y );
    return *this;
}


Group& 
Group::extentMax( const double x, const double y )
{
    myNode->extent.max( x, y );
    return *this;
}


Group const&
Group::moveBy( const double dx, const double dy )  const
{
    myNode->moveBy( dx, dy );
    myLabel->moveBy( dx, dy );

    return *this;
}

Group const&
Group::moveGroupBy( const double dx, const double dy ) const
{
    const unsigned MAX_LEVEL = layer.size();
    for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
	layer[i].moveBy( dx, dy );
    }
    moveBy( dx, dy );
    return *this;
}


Group const&
Group::scaleBy( const double sx, const double sy ) const
{
    myNode->scaleBy( sx, sy );
    myLabel->scaleBy( sx, sy );
    return *this;
}



Group const&
Group::translateY( const double dy )  const
{
    myNode->translateY( dy );
    myLabel->translateY( dy );
    return *this;
}



/*
 * Draw a box with the name of the group in it.
 */

ostream&
Group::draw( ostream& output ) const
{
    if ( isUsed() ) {
	myNode->penColour( processor()->colour() == Graphic::GREY_10 ? Graphic::BLACK : processor()->colour() ).fillColour( processor()->colour() ).linestyle( linestyle() ).depth( depth() );
	myLabel->depth( depth() );

	myNode->roundedRectangle( output );
	output << *myLabel;
    }

    return output;
}

#if HAVE_REGEX_T
GroupByRegex::GroupByRegex( const string& s ) 
    : Group( s )
{
    myPattern = static_cast<regex_t *>(malloc( sizeof( regex_t ) ));
    if ( myPattern ) {
	myErrorCode = regcomp( myPattern, s.c_str(), REG_EXTENDED );
    }
}


GroupByRegex::~GroupByRegex()
{
    if ( myPattern ) {
	regfree( myPattern );
	::free( myPattern );
    }
}


bool
GroupByRegex::match( const string& s ) const
{
    return regexec( myPattern, const_cast<char *>(s.c_str()), 0, 0, 0 ) != REG_NOMATCH;
}
#endif

GroupByProcessor::GroupByProcessor( const Processor * aProcessor ) 
  : Group( aProcessor->name() ), myProcessor( aProcessor ) 
{
}



GroupByProcessor&
GroupByProcessor::label()
{
    *myLabel << myProcessor->name();
    if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	if ( myProcessor->isMultiServer() ) {
	    *myLabel << " {" << myProcessor->copies() << "}";
	} else if ( myProcessor->isInfinite() ) {
	    *myLabel << " {" << _infty() << "}";
	}
	if ( myProcessor->isReplicated() ) {
	    *myLabel << " <" << myProcessor->replicas() << ">";
	}
    }
    if ( Flags::have_results && Flags::print[PROCESS_UTIL].value.b ) {
	myLabel->newLine() << begin_math( &Label::mu ) << "=" << myProcessor->utilization() << end_math();
	if ( !myProcessor->hasBogusUtilization() ) {
	    myLabel->colour(Graphic::RED);
	}
    }
    return *this;
}


/* 
 * Now adjust the box (the group) and label 
 */

GroupByProcessor const&
GroupByProcessor::resizeBox() const
{
    if ( Flags::print[PROCESS_UTIL].value.b && Flags::have_results ) {
	Point& anOrigin = myNode->origin;
	Point& anExtent = myNode->extent;
	
	anOrigin.moveBy( -4.5, -(4.5 + Flags::print[FONT_SIZE].value.i * 2.2) );
	anExtent.moveBy( 9.0, 9.0 + Flags::print[FONT_SIZE].value.i * 2.2 );
    } else {
	Group::resizeBox();
    }
    return *this;
}


GroupByProcessor const& 
GroupByProcessor::positionLabel() const
{
    if ( Flags::print[PROCESS_UTIL].value.b && Flags::have_results ) {
	Point& anOrigin = myNode->origin;
	Point& anExtent = myNode->extent;
	myLabel->moveTo( anOrigin.x() + anExtent.x() / 2.0, 
			 anOrigin.y() + Flags::print[FONT_SIZE].value.i * 1.4 );

    } else {
	Group::positionLabel();
    }

    return *this;
}


/* 
 * Locate all tasks associated with the selected processors.
 */

bool
GroupByShareDefault::populate()
{

    /* Loop here to go through all shares on a group */

    bool empty = true;
    for ( set<Task *,ltTask>::const_iterator nextTask = processor()->tasks().begin(); nextTask != processor()->tasks().end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( !aTask->share() && aTask->isSelectedIndirectly() ) {
	    layer[aTask->level()] << aTask;
	    empty = false;
	    isUsed( true );
	}
    }

    penColour( processor()->colour() == Graphic::GREY_10 ? Graphic::BLACK : processor()->colour() );
    fillColour( processor()->colour() );

    return !empty;
}



GroupByShareDefault& 
GroupByShareDefault::format( const unsigned MAX_LEVEL )
{
    Group::format( MAX_LEVEL );

    Point oldOrigin(0,0);
    Point oldExtent(0,0);
    if ( isUsed() ) { 
	oldOrigin = myNode->origin;
	oldExtent = myNode->extent;
    }


    /* Now go through all groups with this processor and adjust the
     * origin and extent as necessary.  If we have any tasks as part
     * of the default, then we will have set our own bounds. */

    Sequence<Group *> nextGroup( Model::group );
    Group * aGroup;

    origin( MAXDOUBLE, MAXDOUBLE ).extent( 0., 0.);
    while ( aGroup = nextGroup() ) {
	if ( !dynamic_cast<GroupByShareGroup *>( aGroup ) || aGroup->processor() != processor() ) continue;

	originMin( aGroup->x(), aGroup->y() );
	extentMax( aGroup->x() + aGroup->width(), aGroup->y() + aGroup->height() );
    }

    if ( isUsed() ) {
	const double newX = width() + + Flags::print[X_SPACING].value.f;
	const unsigned MAX_LEVEL = layer.size();
	for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
	    layer[i].moveBy( newX, 0 );
	}
	originMin( newX, oldOrigin.y() );					/* Reset X. */
	extentMax( newX + oldExtent.x(), oldOrigin.y() + oldExtent.y() );	/* Reset Y. */
    } else {
	isUsed( true );		/* always draw this box. */
    }
    extent( width() - x(), height() - y() );

    return *this;
}

/* 
 * Locate all tasks associated with the selected processors.
 */

bool
GroupByShareGroup::populate()
{
    /* Loop here to go through all shares on a group */

    bool empty = true;
    for ( set<Task *,ltTask>::const_iterator nextTask = processor()->tasks().begin(); nextTask != processor()->tasks().end(); ++nextTask ) {
	Task * aTask = *nextTask;

	if ( aTask->share() == share() && aTask->isSelectedIndirectly() ) {
	    layer[aTask->level()] << aTask;
	    if ( !submodel_output() || !aTask->isSelected() ) {
		isUsed( true );
	    }
	    empty = false;
	}
    }

    return !empty;
}


/* 
 * Now adjust the box (the group) and label 
 */

GroupByShareGroup const&
GroupByShareGroup::resizeBox() const
{
    Group::resizeBox();
    return *this;
}


GroupByShareGroup const& 
GroupByShareGroup::positionLabel() const
{
    Group::positionLabel();
    return *this;
}


GroupByShareGroup&
GroupByShareGroup::label()
{
    *myLabel << myShare->name() << " " << share()->share() * 100.0 << "%";
    return *this;
}


GroupSquashed::GroupSquashed( const string& s, const Layer& layer1, const Layer& layer2 ) 
    : Group(s), layer_1(layer1), layer_2(layer2)
{
    penColour( Graphic::DEFAULT_COLOUR );
    fillColour( Graphic::DEFAULT_COLOUR );
    isUsed( true );
}

/* 
 * Look at the stuff in myLevel and myLevel-1 and make a box around
 * it.
 */

GroupSquashed&
GroupSquashed::format( const unsigned MAX_LEVEL )
{
    origin( MAXDOUBLE, MAXDOUBLE ).extent( 0, 0 );
    originMin( min( layer_1.x(), layer_2.x() ), min( layer_1.y(), layer_2.y() ) );
    extentMax( max( layer_1.x() + layer_1.width(), layer_2.x() + layer_2.width() ), 
	       max( layer_1.y() + layer_1.height(), layer_2.y() + layer_2.height() ) );
    extent( width(), height() - y() );
    return *this;
}
