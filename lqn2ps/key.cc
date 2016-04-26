/* element.cc	-- Greg Franks Wed Feb 12 2003
 *
 * $Id: key.cc 11963 2014-04-10 14:36:42Z greg $
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

ostream&
operator<<( ostream& output, const Key& self )
{
    return self.print( output );
}

Key::Key()
{
}


Key::~Key()
{
    myArcs.deleteContents();
    myLabels.deleteContents();
}


/*
 * Label the key.  Only stick on useful labels.
 */

Key&
Key::label() 
{
    unsigned i;
    const unsigned n = (Model::forwardingCount > 0 ? 1 : 0)
	+ (Model::rendezvousCount[0] > 0 ? 1 : 0)
	+ (Model::sendNoReplyCount[0] > 0 ? 1 : 0);
    myArcs.grow(n);
    myLabels.grow(n);

    for ( i = 1; i <= n; ++i ) {
	myArcs[i] = Arc::newArc();
	myLabels[i] = Label::newLabel();
	myLabels[i]->justification( LEFT_JUSTIFY );
    }

    double maxWidth = 0;
    i = 1;
    if ( Model::rendezvousCount[0] > 0 ) {
	myArcs[i]->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::SOLID);
	(*myLabels[i]) << "Synchronous request";
	maxWidth = max( maxWidth, myLabels[i]->width() );
	i += 1;
    }
    if ( Model::sendNoReplyCount[0] > 0 ) {
	myArcs[i]->arrowhead(Graphic::OPEN_ARROW).linestyle(Graphic::SOLID);
	(*myLabels[i]) << "Asynchronous request";
	maxWidth = max( maxWidth, myLabels[i]->width() );
	i += 1;
    }
    if ( Model::forwardingCount > 0 ) {
	myArcs[i]->arrowhead(Graphic::CLOSED_ARROW).linestyle(Graphic::DASHED);
	(*myLabels[i]) << "Forwarded request";
	maxWidth = max( maxWidth, myLabels[i]->width() );
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

    for ( unsigned i = 1; i <= myArcs.size(); ++i ) {
	myArcs[i]->moveSrc( x+0, (i-0.5) * y_offset + y );
	myArcs[i]->moveDst( x+x_offset, (i-0.5) * y_offset + y );
	myLabels[i]->moveTo( x+x_offset + y_offset / 2, (i-0.5) * y_offset + y );
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
    Sequence<Arc *> nextArc(myArcs);
    Sequence<Label *> nextLabel(myLabels);

    Arc * anArc;
    Label * aLabel;

    while ( anArc = nextArc() ) {
	anArc->scaleBy( sx, sy );
    }
    while ( aLabel = nextLabel() ) {
	aLabel->scaleBy( sx, sy );
    }
    return *this;
}


Key& 
Key::translateY( const double y )
{
    Sequence<Arc *> nextArc(myArcs);
    Sequence<Label *> nextLabel(myLabels);

    Arc * anArc;
    Label * aLabel;

    while ( anArc = nextArc() ) {
	anArc->translateY( y );
    }
    while ( aLabel = nextLabel() ) {
	aLabel->translateY( y );
    }
    return *this;
}

/* 
 * Print a layer.
 */

ostream&
Key::print( ostream& output ) const
{
    BackwardsSequence<Arc *> nextArc(myArcs);
    BackwardsSequence<Label *> nextLabel(myLabels);

    const Arc * anArc;
    const Label * aLabel;

    while ( anArc = nextArc() ) {
	output << *anArc;
    }
    while ( aLabel = nextLabel() ) {
	output << *aLabel;
    }
    return output;
}
