/* group.cc	-- Greg Franks Thu Mar 24 2005
 *
 * $Id: group.cc 16967 2024-01-28 20:33:35Z greg $
 */

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <limits>
#include "group.h"
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
    _node = Node::newNode( 0, 0 );
    _label = Label::newLabel();
}


Group::~Group()
{
    delete _node;
    delete _label;
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

    origin( std::numeric_limits<double>::max(), std::numeric_limits<double>::max() ).extent( 0, 0 );
    for ( std::vector<Layer>::reverse_iterator layer = _layers.rbegin(); layer != _layers.rend(); ++layer ) {
	if ( !*layer ) continue;
	layer->reformat();
	originMin( layer->x(), layer->y() );
	extentMax( layer->x() + layer->width(), layer->y() + layer->height() );
    }
    extent( width(), height() - y() );

    /* Justify the current "slice", then move it to its column */

    std::for_each( _layers.begin(), _layers.end(), [=]( Layer& layer ){ layer.justify( width() ); } );
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

	if ( Flags::layering() == Layering::PROCESSOR ) {
	    penColour( (*processor)->colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : (*processor)->colour() );
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
    _node->resizeBox( -4.5, -(4.5 + Flags::font_size() * 1.2), 9.0, 9.0 + Flags::font_size() * 1.2 );
    return *this;
}


Group const&
Group::positionLabel() const
{
    _label->moveTo( _node->left() + _node->width() / 2.0,
		     _node->bottom() + Flags::font_size() * 0.6 );
    return *this;
}


Group&
Group::label()
{
    *_label << myName;
    return *this;
}


Group&
Group::origin( const double an_x, const double a_y )
{
    _node->moveTo( an_x, a_y );
    return *this;
}


Group&
Group::extent( const double x, const double y )
{
    _node->setWidth( x ).setHeight( y );
    return *this;
}


Group&
Group::originMin( const double x, const double y )
{
    _node->originMin( x, y );
    return *this;
}


Group&
Group::extentMax( const double w, const double h )
{
    _node->extentMax( w, h );
    return *this;
}


Group&
Group::moveBy( const double dx, const double dy )
{
    _node->moveBy( dx, dy );
    _label->moveBy( dx, dy );
    return *this;
}

Group&
Group::moveGroupBy( const double dx, const double dy )
{
    std::for_each( _layers.begin(), _layers.end(), [=]( Layer& layer ){ layer.moveBy( dx, dy ); } );
    moveBy( dx, dy );
    return *this;
}


Group&
Group::scaleBy( const double sx, const double sy )
{
    _node->scaleBy( sx, sy );
    _label->scaleBy( sx, sy );
    return *this;
}



Group&
Group::translateY( const double dy )
{
    _node->translateY( dy );
    _label->translateY( dy );
    return *this;
}



/*
 * Draw a box with the name of the group in it.
 */

std::ostream&
Group::draw( std::ostream& output ) const
{
    if ( isUsed() ) {
	const Colour colour = processor() ?  processor()->colour() : Graphic::Colour::DEFAULT;
	_node->penColour( colour == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour ).fillColour( colour ).linestyle( linestyle() ).depth( depth() + 1 );
	_label->depth( depth() );

	_node->roundedRectangle( output );
	output << *_label;
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
    *_label << myProcessor->name();
    if ( Flags::print_input_parameters() ) {
	if ( myProcessor->isMultiServer() ) {
	    *_label << " {" << myProcessor->copies() << "}";
	} else if ( myProcessor->isInfinite() ) {
	    *_label << " {" << _infty() << "}";
	}
	if ( myProcessor->isReplicated() ) {
	    *_label << " <" << myProcessor->replicas() << ">";
	}
    }
    if ( Flags::have_results && Flags::print[PROCESSOR_UTILIZATION].opts.value.b ) {
	_label->newLine() << begin_math( &Label::mu ) << "=" << myProcessor->utilization() << end_math();
	if ( myProcessor->hasBogusUtilization() && Flags::colouring() != Colouring::NONE ) {
	    _label->colour(Graphic::Colour::RED);
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
    if ( Flags::print[PROCESSOR_UTILIZATION].opts.value.b && Flags::have_results ) {
	_node->resizeBox( -4.5, -(4.5 + Flags::font_size() * 2.2), 9.0, 9.0 + Flags::font_size() * 2.2 );
    } else {
	Group::resizeBox();
    }
    return *this;
}


GroupByProcessor const&
GroupByProcessor::positionLabel() const
{
    if ( Flags::print[PROCESSOR_UTILIZATION].opts.value.b && Flags::have_results ) {
	_label->moveTo( _node->left() + _node->width() / 2.0,
			 _node->bottom() + Flags::font_size() * 1.4 );

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

    penColour( processor()->colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : processor()->colour() );
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
	oldOrigin = _node->getOrigin();
	oldExtent = _node->getExtent();
    }

    /* Now go through all groups with this processor and adjust the
     * origin and extent as necessary.  If we have any tasks as part
     * of the default, then we will have set our own bounds. */

    origin( std::numeric_limits<double>::max(), std::numeric_limits<double>::max() ).extent( 0., 0.);
    for ( std::vector<Group *>::iterator group = Group::__groups.begin(); group != Group::__groups.end(); ++group ) {
	if ( !dynamic_cast<GroupByShareGroup *>( (*group) ) || (*group)->processor() != processor() ) continue;

	originMin( (*group)->x(), (*group)->y() );
	extentMax( (*group)->x() + (*group)->width(), (*group)->y() + (*group)->height() );
    }

    if ( isUsed() ) {
	const double newX = width() + Flags::x_spacing();
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
    *_label << myShare->name() << " " << share()->share() * 100.0 << "%";
    return *this;
}


GroupSquashed::GroupSquashed( unsigned int nLayers, const std::string& s, const Layer& layer1, const Layer& layer2 )
    : Group(nLayers, s), layer_1(layer1), layer_2(layer2)
{
    penColour( Graphic::Colour::DEFAULT );
    fillColour( Graphic::Colour::DEFAULT );
    isUsed( true );
}

/*
 * Look at the stuff in myLevel and myLevel-1 and make a box around
 * it.
 */

GroupSquashed&
GroupSquashed::format()
{
    origin( std::numeric_limits<double>::max(), std::numeric_limits<double>::max() ).extent( 0, 0 );
    originMin( std::min( layer_1.x(), layer_2.x() ), std::min( layer_1.y(), layer_2.y() ) );
    extentMax( std::max( layer_1.x() + layer_1.width(), layer_2.x() + layer_2.width() ),
	       std::max( layer_1.y() + layer_1.height(), layer_2.y() + layer_2.height() ) );
    extent( width(), height() - y() );
    return *this;
}
