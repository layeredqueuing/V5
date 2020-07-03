/* -*- c++ -*-
 * group.h	-- Greg Franks
 *
 * $Id: group.h 13477 2020-02-08 23:14:37Z greg $
 */

#ifndef _GROUP_H
#define _GROUP_H

#include "lqn2ps.h"
#include <string>
#include "node.h"
#include "graphic.h"
#include "layer.h"

class Processor;
class Share;

class Group : public Graphic
{
private:
    Group( const Group& );
    Group& operator=( const Group& );

public:
    Group( unsigned int, const std::string& s );
    virtual ~Group();
    virtual bool match( const std::string& ) const;
    const std::string& name() const { return myName; }

    Group& origin( const double an_x, const double a_y );
    Group& extent( const double an_x, const double a_y );
    Group& originMin( const double an_x, const double a_y );
    Group& extentMax( const double an_x, const double a_y );
    virtual Group& format();
    virtual Group& label();
    Group& scaleBy( const double, const double );
    Group& moveBy( const double, const double );
    Group& moveGroupBy( const double, const double );
    Group& translateY( const double );
    virtual Group& resizeBox();
    virtual Group const& positionLabel() const;

    double width() const { return myNode->width(); }
    double height() const { return myNode->height(); }
    double x() const { return myNode->left(); }
    double y() const { return myNode->bottom(); }

    virtual const Processor * processor() const { return 0; }
    virtual bool isPseudoGroup() const { return false; }

    /* Printing */
    
    virtual std::ostream& draw( std::ostream& output ) const;
    virtual std::ostream& print( std::ostream& output ) const { return output; }
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const { return output; }

protected:
    virtual linestyle_type linestyle() const { return Graphic::DASHED; }
    Group& isUsed( const bool yes_or_no ) { used = yes_or_no; return *this; }
    bool isUsed() const { return used; }

    virtual bool populate();

public:
    static std::vector<Group *> __groups;

protected:
    Label * myLabel;
    Node * myNode;
    std::vector<Layer> _layers;

private:
    const std::string myName;
    bool used;
};

inline std::ostream& operator<<( std::ostream& output, const Group& self ) { self.draw( output ); return output; }

#if HAVE_REGEX_T
class GroupByRegex : public Group
{
public:
    GroupByRegex( const std::string& s );
    virtual ~GroupByRegex();

    virtual bool match( const std::string& ) const;

private:
    regex_t * myPattern;
    int myErrorCode;

};
#endif

class GroupByProcessor : public Group
{
public:
    GroupByProcessor( unsigned int, const Processor * aProcessor );

    GroupByProcessor& label();

    virtual GroupByProcessor & resizeBox();
    virtual GroupByProcessor const& positionLabel() const;

protected:

    virtual const Processor * processor() const { return myProcessor; }

private:
    const Processor * myProcessor;
};


class GroupByShare : public GroupByProcessor
{
public:
    GroupByShare( unsigned int nLayers, const Processor * aProcessor ) : GroupByProcessor( nLayers, aProcessor ) {}

protected:
    virtual bool populate() = 0;
};


class GroupByShareDefault : public GroupByShare
{
public:
    GroupByShareDefault( unsigned int nLayers, const Processor * aProcessor ) : GroupByShare( nLayers, aProcessor ) {}

    virtual bool isPseudoGroup() const { return true; }

protected:
    virtual bool populate();
    virtual GroupByShareDefault& format();

};


class GroupByShareGroup : public GroupByShare
{
public:
    GroupByShareGroup( unsigned int nLayers, const Processor * aProcessor, const Share * aShare ) : GroupByShare( nLayers, aProcessor ), myShare( aShare ) {}

    virtual GroupByShareGroup & resizeBox();
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
    GroupSquashed( unsigned int nLayers, const std::string& s, const Layer& layer1, const Layer& layer2 );

    GroupSquashed& format();

private:
    const Layer & layer_1;
    const Layer & layer_2;
};
#endif
