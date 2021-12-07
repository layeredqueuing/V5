/* element.cc	-- Greg Franks Wed Feb 12 2003
 *
 * $Id: key.cc 15170 2021-12-07 23:33:05Z greg $
 */

#include "key.h"
#include "arc.h"
#include "label.h"
#include "model.h"

/*----------------------------------------------------------------------*/
/*                         Helper Functions                             */
/*----------------------------------------------------------------------*/

/*
 * Print all results.
 */

std::ostream&
operator<<( std::ostream& output, const Key& self )
{
    return self.print( output );
}

Key::Key()
{
}


Key::~Key()
{
    for ( std::map<Label *,Arc *>::iterator label = _labels.begin(); label != _labels.end(); ++label ) {
	delete label->first;
	delete label->second;
    }
}


/*
 * Label the key.  Only stick on useful labels.
 */

Key&
Key::label() 
{
    double maxWidth = 0;
    unsigned int i = 0;
    if ( Model::rendezvousCount[0] > 0 ) {
	Arc * arc = Arc::newArc();
	arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::SOLID);
	Label * label = Label::newLabel();
	label->justification( Justification::LEFT );
	*label << "Synchronous request";
	maxWidth = std::max( maxWidth, label->width() );
	_labels[label] = arc;
	i += 1;
    }
    if ( Model::sendNoReplyCount[0] > 0 ) {
	Arc * arc = Arc::newArc();
	arc->arrowhead(Graphic::OPEN_ARROW).linestyle(Graphic::SOLID);
	Label * label = Label::newLabel();
	label->justification( Justification::LEFT );
	*label << "Asynchronous request";
	maxWidth = std::max( maxWidth, label->width() );
	_labels[label] = arc;
	i += 1;
    }
    if ( Model::forwardingCount > 0 ) {
	Arc * arc = Arc::newArc();
	arc->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	Label * label = Label::newLabel();
	label->justification( Justification::LEFT );
	*label << "Forwarded request";
	maxWidth = std::max( maxWidth, label->width() );
	_labels[label] = arc;
	i += 1;
    }

    /* Font size already has magnification */

    const double y_offset = normalized_font_size();
    const double x_offset = Flags::arrow_scaling * 18;
    extent.moveTo( x_offset + y_offset / 2 + maxWidth, i * y_offset );

    return *this;
}

Key& 
Key::moveTo( const double x, const double y )
{
    origin.moveTo( x, y );

    /* Font size already has magnification */

    const double y_offset = normalized_font_size();
    const double x_offset = Flags::arrow_scaling * 18;

    unsigned i = 1;
    for ( std::map<Label *,Arc *>::const_iterator label = _labels.begin(); label != _labels.end(); ++label ) {
	label->second->moveSrc( x+0, (i-0.5) * y_offset + y );
	label->second->moveDst( x+x_offset, (i-0.5) * y_offset + y );
	label->first->moveTo( x+x_offset + y_offset / 2, (i-0.5) * y_offset + y );
	i += 1;
    }

    return *this;
}


Key&
Key::moveBy( const double dx, const double dy )
{
    moveTo( origin.x() + dx, origin.y() + dy );
    return *this;
}


Key&
Key::scaleBy( const double sx, const double sy )
{
    origin.scaleBy( sx, sy );
    extent.scaleBy( sx, sy );
    for ( std::map<Label *,Arc *>::const_iterator label = _labels.begin(); label != _labels.begin(); ++label ) {
	label->first->scaleBy( sx, sy );
	label->second->scaleBy( sx, sy );
    }
    return *this;
}


Key& 
Key::translateY( const double y )
{
    for ( std::map<Label *,Arc *>::const_iterator label = _labels.begin(); label != _labels.begin(); ++label ) {
	label->first->translateY( y );
	label->second->translateY( y );
    }
    return *this;
}

/* 
 * Print a layer.
 */

std::ostream&
Key::print( std::ostream& output ) const
{
    for ( std::map<Label *,Arc *>::const_iterator label = _labels.begin(); label != _labels.end(); ++label ) {
	output << *label->first;
	output << *label->second;
    }
    return output;
}
