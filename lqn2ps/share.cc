/* share.cc	-- Greg Franks Tue Nov  4 2008
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqn2ps/share.cc $
 *
 * ------------------------------------------------------------------------
 * $Id: share.cc 16969 2024-01-28 22:57:43Z greg $
 * ------------------------------------------------------------------------
 */


#include "share.h"
#include <algorithm>
#include <lqio/error.h>
#include <lqio/glblerr.h>
#include <lqio/dom_group.h>
#include <lqio/dom_processor.h>
#include "processor.h"
#include "task.h"

std::set<Share *,LT<Share> > Share::__share;

Share::Share( const LQIO::DOM::Group* dom, const Processor * aProcessor )
    : _documentObject( dom ), _processor(aProcessor)
{
    if ( aProcessor ) {
	const_cast<Processor *>(aProcessor)->addShare(this);
    }
}

std::ostream& 
Share::print( std::ostream& output ) const
{
    output << "  g " << name()
	   << " " << share()
	   << " " << _processor->name() 
	   << std::endl;
    return output;
}

/* static */
void 
Share::create( const std::pair<std::string,LQIO::DOM::Group *>& p )
{
    const std::string& group_name = p.first;
    const LQIO::DOM::Group* group = p.second;
    const std::string& processor_name = group->getProcessor()->getName();
    Processor* processor = Processor::find( processor_name );

    if ( !processor ) return;

    if ( std::any_of( __share.begin(), __share.end(), [&]( Share * share ){ return share->name() == group_name; } ) ) {
	group->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return;
    } 
    if ( processor->scheduling() != SCHEDULE_CFS ) {
	group->input_error( LQIO::WRN_NON_CFS_PROCESSOR, processor_name.c_str() );
    }

    Share * aShare = new Share( group, processor );
    __share.insert(aShare);
}



Share * 
Share::find( const std::string& name )
{
    std::set<Share *>::const_iterator share = find_if( __share.begin(), __share.end(), [&]( Share * share ){ return share->name() == name; } );
    return share != __share.end() ? *share : nullptr;
}
