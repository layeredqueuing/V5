/* -*- c++ -*-
 * group.h	-- Greg Franks
 *
 * $Id: group.h 17536 2025-04-02 13:42:13Z greg $
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

    double width() const { return _node->width(); }
    double height() const { return _node->height(); }
    double x() const { return _node->left(); }
    double y() const { return _node->bottom(); }

    virtual const Processor * processor() const { return 0; }
    virtual bool isPseudoGroup() const { return false; }

    /* Printing */
    
    virtual std::ostream& draw( std::ostream& output ) const;
    virtual std::ostream& print( std::ostream& output ) const { return output; }
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const { return output; }

protected:
    virtual LineStyle linestyle() const { return Graphic::LineStyle::DASHED; }
    Group& isUsed( const bool yes_or_no ) { used = yes_or_no; return *this; }
    bool isUsed() const { return used; }

    virtual bool populate();

public:
    static std::vector<Group *> __groups;

protected:
    Label * _label;
    Node * _node;
    std::vector<Layer> _layers;

private:
    const std::string myName;
    bool used;
};

inline std::ostream& operator<<( std::ostream& output, const Group& self ) { self.draw( output ); return output; }

class GroupByRegex : public Group
{
public:
    GroupByRegex( unsigned int, const std::string& s );
    virtual ~GroupByRegex();

    virtual bool match( const std::string& ) const;

private:
    std::regex _pattern;
};

class GroupByProcessor : public Group
{
public:
    GroupByProcessor( unsigned int, const Processor * processor );

    GroupByProcessor& label();

    virtual GroupByProcessor & resizeBox();
    virtual GroupByProcessor const& positionLabel() const;

protected:

    virtual const Processor * processor() const { return _processor; }

private:
    const Processor * _processor;
};


class GroupByShare : public GroupByProcessor
{
public:
    GroupByShare( unsigned int nLayers, const Processor * processor ) : GroupByProcessor( nLayers, processor ) {}

protected:
    virtual bool populate() = 0;
};


class GroupByShareDefault : public GroupByShare
{
public:
    GroupByShareDefault( unsigned int nLayers, const Processor * processor ) : GroupByShare( nLayers, processor ) {}

    virtual bool isPseudoGroup() const { return true; }

protected:
    virtual bool populate();
    virtual GroupByShareDefault& format();

};


class GroupByShareGroup : public GroupByShare
{
public:
    GroupByShareGroup( unsigned int nLayers, const Processor * processor, const Share * aShare ) : GroupByShare( nLayers, processor ), _share( aShare ) {}

    virtual GroupByShareGroup & resizeBox();
    virtual GroupByShareGroup const& positionLabel() const;

protected:
    const Share * share() const { return _share; }
    virtual LineStyle linestyle() const { return Graphic::LineStyle::DASHED_DOTTED; }	/* Draw a different box style */

    virtual bool populate();
    virtual GroupByShareGroup& label();

private:
    const Share * _share;
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
