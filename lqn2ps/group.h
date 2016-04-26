/* -*- c++ -*-
 * group.h	-- Greg Franks
 *
 * $Id: group.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _GROUP_H
#define _GROUP_H

#include "lqn2ps.h"
#include <cstring>
#include "point.h"
#include "node.h"
#include "graphic.h"
#include "vector.h"
#include "layer.h"

class Group;
class Processor;
class Share;

ostream& operator<<( ostream&, const Group& );


class Group : public Graphic
{
private:
    Group( const Group& );
    Group& operator=( const Group& );

public:
    Group( const string& s );
    virtual ~Group();
    virtual bool match( const string& ) const;
    const string& name() const { return myName; }

    Group& origin( const double an_x, const double a_y );
    Group& extent( const double an_x, const double a_y );
    Group& originMin( const double an_x, const double a_y );
    Group& extentMax( const double an_x, const double a_y );
    virtual Group& format( const unsigned MAX_LEVEL );
    virtual Group& label();
    virtual Group const& scaleBy( const double, const double ) const;
    Group const& moveBy( const double, const double ) const;
    Group const& moveGroupBy( const double, const double ) const;
    Group const& translateY( const double ) const;
    virtual Group const& resizeBox() const;
    virtual Group const& positionLabel() const;

    double width() const { return myNode->extent.x(); }
    double height() const { return myNode->extent.y(); }
    double x() const { return myNode->left(); }
    double y() const { return myNode->bottom(); }

    virtual const Processor * processor() const { return 0; }
    virtual bool isPseudoGroup() const { return false; }

    /* Printing */
    
    virtual ostream& draw( ostream& output ) const;
    virtual ostream& print( ostream& output ) const { return output; }
    virtual ostream& comment( ostream& output, const string& ) const { return output; }

protected:
    virtual linestyle_type linestyle() const { return Graphic::DASHED; }
    Group& isUsed( const bool yes_or_no ) { used = yes_or_no; return *this; }
    bool isUsed() const { return used; }

    virtual bool populate();

protected:
    Label * myLabel;
    Node * myNode;
    Vector2<Layer> layer;

private:
    const string myName;
    bool used;
};

#if HAVE_REGEX_T
class GroupByRegex : public Group
{
public:
    GroupByRegex( const string& s );
    virtual ~GroupByRegex();

    virtual bool match( const string& ) const;

private:
    regex_t * myPattern;
    int myErrorCode;

};
#endif

class GroupByProcessor : public Group
{
public:
    GroupByProcessor( const Processor * aProcessor );

    GroupByProcessor& label();

    virtual GroupByProcessor const& resizeBox() const;
    virtual GroupByProcessor const& positionLabel() const;

protected:

    virtual const Processor * processor() const { return myProcessor; }

private:
    const Processor * myProcessor;
};


class GroupByShare : public GroupByProcessor
{
public:
    GroupByShare( const Processor * aProcessor ) : GroupByProcessor( aProcessor ) {}

protected:
    virtual bool populate() = 0;
};


class GroupByShareDefault : public GroupByShare
{
public:
    GroupByShareDefault( const Processor * aProcessor ) : GroupByShare( aProcessor ) {}

    virtual bool isPseudoGroup() const { return true; }

protected:
    virtual bool populate();
    virtual GroupByShareDefault& format( const unsigned MAX_LEVEL );

};


class GroupByShareGroup : public GroupByShare
{
public:
    GroupByShareGroup( const Processor * aProcessor, const Share * aShare ) : GroupByShare( aProcessor ), myShare( aShare ) {}

    virtual GroupByShareGroup const& resizeBox() const;
    virtual GroupByShareGroup const& positionLabel() const;

protected:
    const Share * share() const { return myShare; }
    virtual linestyle_type linestyle() const { return Graphic::DASHED_DOTTED; }	/* Draw a different box style */

    virtual bool populate();
    virtual GroupByShareGroup& label();

private:
    const Share * myShare;
};

class GroupSquashed : public Group
{
public:
    GroupSquashed( const string& s, const Layer& layer1, const Layer& layer2 );

    GroupSquashed& format( const unsigned MAX_LEVEL );

private:
    const Layer & layer_1;
    const Layer & layer_2;
};
#endif
