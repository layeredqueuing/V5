/* group.cc	-- Greg Franks Thu Mar 24 2005
 *
 * $Id: group.cc 14208 2020-12-11 20:44:05Z greg $
 */

#include "group.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#if HAVE_FLOAT_H
#include <float.h>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif
#include "element.h"
#include "node.h"
#include "label.h"
#include "processor.h"
#include "task.h"
#include "share.h"
#include "model.h"

std::vector<Group *> Group::__groups;

Group::Group( unsigned int nLayers, const std::string& s )
    : _layers(nLayers), myName(s), used(false)
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
Group::match( const std::string& s ) const
{
    return myName == s;
}


/*
 * Locate all tasks associated with the selected processors.
 */

Group&
Group::format()
{
    if ( !populate() ) return *this;

    /* Resort the layers to force tasks making calls right */

    for ( std::vector<Layer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer ) {
	layer->sort( (compare_func_ptr)(&Entity::compare) );
    }

    /* Now, move all entities for this processor together */

    origin( MAXDOUBLE, MAXDOUBLE ).extent( 0, 0 );
    for ( std::vector<Layer>::reverse_iterator layer = _layers.rbegin(); layer != _layers.rend(); ++layer ) {
	if ( !*layer ) continue;
	layer->reformat();
	originMin( layer->x(), layer->y() );
	extentMax( layer->x() + layer->width(), layer->y() + layer->height() );
    }
    extent( width(), height() - y() );

    /* Justify the current "slice", then move it to its column */

    for_each ( _layers.begin(), _layers.end(), Exec1<Layer,double>( &Layer::justify, width() ) );
    return *this;
}


bool
Group::populate()
{
    /* Loop here to go through all processors on a group */

    bool empty = true;
    for ( std::set<Processor *>::const_iterator processor = Processor::__processors.begin(); processor != Processor::__processors.end(); ++processor ) {
	if ( (*processor)->hasGroup() || !match((*processor)->name()) ) continue;
	(*processor)->hasGroup( true );

	if ( Flags::print[LAYERING].value.i == LAYERING_PROCESSOR ) {
	    penColour( (*processor)->colour() == Graphic::GREY_10 ? Graphic::BLACK : (*processor)->colour() );
	    fillColour( (*processor)->colour() );
	}
	if ( (*processor)->isSelected() ) {
	    _layers.at((*processor)->level()).append((*processor));
	    isUsed( true );
	    empty = false;
	}

	for ( std::set<Task *>::const_iterator task = Task::__tasks.begin(); task != Task::__tasks.end(); ++task ) {
	    if ( (*task)->hasProcessor(*processor) && (*task)->isSelectedIndirectly() ) {
		_layers.at((*task)->level()).append((*task));
		if ( !submodel_output() || !(*task)->isSelected() ) {
		    isUsed( true );
		}
		empty = false;
	    }
	}
    }

    return !empty;
}


/* 
 * Now adjust the box (the group) and label 
 */

Group&
Group::resizeBox()
{
    myNode->resizeBox( -4.5, -(4.5 + Flags::print[FONT_SIZE].value.i * 1.2), 9.0, 9.0 + Flags::print[FONT_SIZE].value.i * 1.2 );
    return *this;
}


Group const&
Group::positionLabel() const
{
    myLabel->moveTo( myNode->left() + myNode->width() / 2.0,
		     myNode->bottom() + Flags::print[FONT_SIZE].value.i * 0.6 );
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
    myNode->moveTo( an_x, a_y );
    return *this;
}


Group&
Group::extent( const double x, const double y )
{
    myNode->setWidth( x ).setHeight( y );
    return *this;
}


Group&
Group::originMin( const double x, const double y )
{
    myNode->originMin( x, y );
    return *this;
}


Group&
Group::extentMax( const double w, const double h )
{
    myNode->extentMax( w, h );
    return *this;
}


Group&
Group::moveBy( const double dx, const double dy )
{
    myNode->moveBy( dx, dy );
    myLabel->moveBy( dx, dy );
    return *this;
}

Group&
Group::moveGroupBy( const double dx, const double dy )
{
    for_each( _layers.begin(), _layers.end(), ExecXY<Layer>( &Layer::moveBy, dx, dy ) );
    moveBy( dx, dy );
    return *this;
}


Group&
Group::scaleBy( const double sx, const double sy )
{
    myNode->scaleBy( sx, sy );
    myLabel->scaleBy( sx, sy );
    return *this;
}



Group&
Group::translateY( const double dy )
{
    myNode->translateY( dy );
    myLabel->translateY( dy );
    return *this;
}



/*
 * Draw a box with the name of the group in it.
 */

std::ostream&
Group::draw( std::ostream& output ) const
{
    if ( isUsed() ) {
	const colour_type colour = processor() ?  processor()->colour() : Graphic::BLACK;
	myNode->penColour( colour == Graphic::GREY_10 ? Graphic::BLACK : colour ).fillColour( colour ).linestyle( linestyle() ).depth( depth() + 1 );
	myLabel->depth( depth() );

	myNode->roundedRectangle( output );
	output << *myLabel;
    }

    return output;
}

GroupByRegex::GroupByRegex( unsigned int n, const std::string& s )
    : Group( n, s ), _pattern(s)
{
}


GroupByRegex::~GroupByRegex()
{
}


bool
GroupByRegex::match( const std::string& s ) const
{
    return std::regex_match( s, _pattern );
}

GroupByProcessor::GroupByProcessor( const unsigned nLayers, const Processor * processor )
  : Group( nLayers, processor->name() ), myProcessor( processor )
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
    if ( Flags::have_results && Flags::print[PROCESSOR_UTILIZATION].value.b ) {
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

GroupByProcessor&
GroupByProcessor::resizeBox()
{
    if ( Flags::print[PROCESSOR_UTILIZATION].value.b && Flags::have_results ) {
	myNode->resizeBox( -4.5, -(4.5 + Flags::print[FONT_SIZE].value.i * 2.2), 9.0, 9.0 + Flags::print[FONT_SIZE].value.i * 2.2 );
    } else {
	Group::resizeBox();
    }
    return *this;
}


GroupByProcessor const&
GroupByProcessor::positionLabel() const
{
    if ( Flags::print[PROCESSOR_UTILIZATION].value.b && Flags::have_results ) {
	myLabel->moveTo( myNode->left() + myNode->width() / 2.0,
			 myNode->bottom() + Flags::print[FONT_SIZE].value.i * 1.4 );

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
    for ( std::set<Task *>::const_iterator nextTask = processor()->tasks().begin(); nextTask != processor()->tasks().end(); ++nextTask ) {
	Task * aTask = *nextTask;
	if ( !aTask->share() && aTask->isSelectedIndirectly() ) {
	    _layers.at(aTask->level()).append(aTask);
	    empty = false;
	    isUsed( true );
	}
    }

    penColour( processor()->colour() == Graphic::GREY_10 ? Graphic::BLACK : processor()->colour() );
    fillColour( processor()->colour() );

    return !empty;
}



GroupByShareDefault&
GroupByShareDefault::format()
{
    Group::format();

    Point oldOrigin(0,0);
    Point oldExtent(0,0);
    if ( isUsed() ) {
	oldOrigin = myNode->getOrigin();
	oldExtent = myNode->getExtent();
    }

    /* Now go through all groups with this processor and adjust the
     * origin and extent as necessary.  If we have any tasks as part
     * of the default, then we will have set our own bounds. */

    origin( MAXDOUBLE, MAXDOUBLE ).extent( 0., 0.);
    for ( std::vector<Group *>::iterator group = Group::__groups.begin(); group != Group::__groups.end(); ++group ) {
	if ( !dynamic_cast<GroupByShareGroup *>( (*group) ) || (*group)->processor() != processor() ) continue;

	originMin( (*group)->x(), (*group)->y() );
	extentMax( (*group)->x() + (*group)->width(), (*group)->y() + (*group)->height() );
    }

    if ( isUsed() ) {
	const double newX = width() + + Flags::print[X_SPACING].value.f;
	for ( std::vector<Layer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer ) {
	    if ( !*layer ) continue;
	    layer->moveBy( newX, 0 );
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
    for ( std::set<Task *>::const_iterator nextTask = processor()->tasks().begin(); nextTask != processor()->tasks().end(); ++nextTask ) {
	Task * aTask = *nextTask;

	if ( aTask->share() == share() && aTask->isSelectedIndirectly() ) {
	    _layers.at(aTask->level()).append(aTask);
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

GroupByShareGroup&
GroupByShareGroup::resizeBox()
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


GroupSquashed::GroupSquashed( unsigned int nLayers, const std::string& s, const Layer& layer1, const Layer& layer2 )
    : Group(nLayers, s), layer_1(layer1), layer_2(layer2)
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
GroupSquashed::format()
{
    origin( MAXDOUBLE, MAXDOUBLE ).extent( 0, 0 );
    originMin( std::min( layer_1.x(), layer_2.x() ), std::min( layer_1.y(), layer_2.y() ) );
    extentMax( std::max( layer_1.x() + layer_1.width(), layer_2.x() + layer_2.width() ),
	       std::max( layer_1.y() + layer_1.height(), layer_2.y() + layer_2.height() ) );
    extent( width(), height() - y() );
    return *this;
}
