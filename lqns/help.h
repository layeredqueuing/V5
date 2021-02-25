/* -*- C++ -*-
 * help.h	-- Greg Franks
 *
 * $Id: help.h 14489 2021-02-24 22:44:45Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

//#include "dim.h"
#include <config.h>
#include <map>
#include "lqns.h"

namespace Options {
    class Option;
}

void usage( const char * = 0 );
void usage( const char );
void usage( const char c, const char * s );

class Help;

class StringManip {
public:
    StringManip( std::ostream& (*ff)(std::ostream&, const Help&, const char * ), const Help& h, const char * s ) : f(ff), _h(h), _s(s) {}
private:
    std::ostream& (*f)( std::ostream&, const Help&, const char * );
    const Help & _h;
    const char * _s;

    friend std::ostream& operator<<(std::ostream & os, const StringManip& m ) { return m.f(os,m._h,m._s); }
};

class StringStringManip {
public:
    StringStringManip( std::ostream& (*ff)(std::ostream&, const Help&, const char *, const char * ), const Help& h, const char * s1, const char * s2 ) : f(ff), _h(h), _s1(s1), _s2(s2) {}
private:
    std::ostream& (*f)( std::ostream&, const Help&, const char *, const char * );
    const Help & _h;
    const char * _s1;
    const char * _s2;

    friend std::ostream& operator<<(std::ostream & os, const StringStringManip& m ) { return m.f(os,m._h,m._s1,m._s2); }
};

class Help
{    
public:
    typedef std::ostream& (Help::*help_fptr)( std::ostream&, bool ) const;

    struct parameter_info {
	parameter_info() : _help(nullptr), _default(false) {}
	parameter_info( Help::help_fptr h, bool d=false) : _help(h), _default(d) {}
	Help::help_fptr _help;
	bool _default;
    };
    typedef std::map<std::string,parameter_info> parameter_map_t;
    
    struct pragma_info {
	pragma_info() : _help(nullptr), _value(nullptr) {}
	pragma_info( Help::help_fptr h, std::map<std::string,parameter_info>* v=nullptr ) : _help(h), _value(v) {}
	Help::help_fptr _help;
	const parameter_map_t* _value;
    };

    typedef std::map<std::string,Help::pragma_info> pragma_map_t;

public:
    static void initialize();

    StringManip bold( const Help& h, const char * s ) const { return StringManip( &Help::__textbf, h, s ); }
    StringManip emph( const Help& h, const char * s ) const { return StringManip( &Help::__textit, h, s ); }
    StringManip flag( const Help& h, const char * s ) const { return StringManip( &Help::__flag, h, s ); }
    StringManip ix( const Help& h, const char * s ) const { return StringManip( &Help::__ix, h, s ); }
    StringManip cite( const Help& h, const char * s ) const { return StringManip( &Help::__cite, h, s ); }
    StringStringManip filename( const Help& h, const char * s1, const char * s2 = 0 ) const { return StringStringManip( &Help::__filename, h, s1, s2 ); }

private:
    Help( const Help& );
    Help& operator=( const Help& );
    
    static std::ostream& __textbf( std::ostream& output, const Help & h, const char * s ) { return h.textbf( output, s ); }
    static std::ostream& __textit( std::ostream& output, const Help & h, const char * s ) { return h.textit( output, s ); }
    static std::ostream& __flag( std::ostream& output, const Help & h, const char * s ) { return h.flag( output, s ); }
    static std::ostream& __ix( std::ostream& output, const Help & h, const char * s ) { return h.ix( output, s ); }
    static std::ostream& __cite( std::ostream& output, const Help & h, const char * s ) { return h.cite( output, s ); }
    static std::ostream& __filename( std::ostream& output, const Help & h, const char * s1, const char * s2 ) { return h.filename( output, s1, s2 ); }

protected:
    static std::ostream& __tr_( std::ostream& output, const Help& h, const char * s ) { return h.tr_( output, s ); }

public:
    Help();
    virtual ~Help() {}

    std::ostream& print( std::ostream& ) const;

protected:
   static std::map<const int,help_fptr,lt_int> option_table;


protected:
    virtual std::ostream& preamble( std::ostream& output ) const = 0;
    virtual std::ostream& see_also( std::ostream& output ) const = 0;
    virtual std::ostream& textbf( std::ostream& output, const char * s ) const = 0;
    virtual std::ostream& textit( std::ostream& output, const char * s ) const = 0;
    virtual std::ostream& tr_( std::ostream& output, const char * ) const { return output; }
    virtual std::ostream& filename( std::ostream& output, const char * s1, const char * s2 ) const = 0;
    virtual std::ostream& pp( std::ostream& ouptut ) const = 0;
    virtual std::ostream& br( std::ostream& ouptut ) const = 0;
    virtual std::ostream& ol_begin( std::ostream& output ) const = 0;
    virtual std::ostream& ol_end( std::ostream& output ) const = 0;
    virtual std::ostream& dl_begin( std::ostream& output ) const = 0;
    virtual std::ostream& dl_end( std::ostream& output ) const = 0;
    virtual std::ostream& li( std::ostream& output, const char * = 0 ) const = 0;
    virtual std::ostream& flag( std::ostream& output, const char * s ) const = 0;
    virtual std::ostream& ix( std::ostream& output, const char * s ) const { return output; }
    virtual std::ostream& cite( std::ostream& output, const char * s ) const { return output; }
    virtual std::ostream& section( std::ostream& output, const char * s, const char * ) const = 0;
    virtual std::ostream& label( std::ostream& output, const char * s ) const = 0;
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const = 0;
    virtual std::ostream& increase_indent( std::ostream& output ) const = 0;
    virtual std::ostream& decrease_indent( std::ostream& output ) const = 0;
    virtual std::ostream& print_option( std::ostream&, const char * name, const Options::Option& opt) const = 0;
    virtual std::ostream& print_pragma( std::ostream&, const std::string& ) const = 0;
    virtual std::ostream& table_header( std::ostream& ) const = 0;
    virtual std::ostream& table_row( std::ostream&, const char *, const char *, const char * ix=0 ) const = 0;
    virtual std::ostream& table_footer( std::ostream& ) const = 0;
    virtual std::ostream& trailer( std::ostream& output ) const { return output; }

private:
    std::ostream& flagAdvisory( std::ostream& output, bool verbose ) const;
    std::ostream& flagBound( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebug( std::ostream& output, bool verbose ) const;
    std::ostream& flagError( std::ostream& output, bool verbose ) const;
    std::ostream& flagFast( std::ostream& output, bool verbose ) const;
    std::ostream& flagInputFormat( std::ostream& output, bool verbose ) const;
    std::ostream& flagNoExecute( std::ostream& output, bool verbose ) const;
    std::ostream& flagOutput( std::ostream& output, bool verbose ) const;
    std::ostream& flagParseable( std::ostream& output, bool verbose ) const;
    std::ostream& flagPragmas( std::ostream& output, bool verbose ) const;
    std::ostream& flagRTF( std::ostream& output, bool verbose ) const;
    std::ostream& flagTrace( std::ostream& output, bool verbose ) const;
    std::ostream& flagVerbose( std::ostream& output, bool verbose ) const;
    std::ostream& flagVersion( std::ostream& output, bool verbose ) const;
    std::ostream& flagWarning( std::ostream& output, bool verbose ) const;
    std::ostream& flagXML( std::ostream& output, bool verbose ) const;
    std::ostream& flagSpecial( std::ostream& output, bool verbose ) const;
    std::ostream& flagConvergence( std::ostream& output, bool verbose ) const;
    std::ostream& flagUnderrelaxation( std::ostream& output, bool verbose ) const;
    std::ostream& flagIterationLimit( std::ostream& output, bool verbose ) const;
    std::ostream& flagExactMVA( std::ostream& output, bool verbose ) const;
    std::ostream& flagSchweitzerMVA( std::ostream& output, bool verbose ) const;
    std::ostream& flagHwSwLayering( std::ostream& output, bool verbose ) const;
    std::ostream& flagLoose( std::ostream& output, bool verbose ) const;
    std::ostream& flagStopOnMessageLoss( std::ostream& output, bool verbose ) const;
    std::ostream& flagTraceMVA( std::ostream& output, bool verbose ) const;
    std::ostream& flagNoVariance( std::ostream& output, bool verbose ) const;
    std::ostream& flagNoHeader( std::ostream& output, bool verbose ) const;
    std::ostream& flagReloadLQX( std::ostream& output, bool verbose ) const;
    std::ostream& flagRestartLQX( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebugLQX( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebugXML( std::ostream& output, bool verbose ) const;
    std::ostream& flagMethoOfLayers( std::ostream& output, bool verbose ) const;
    std::ostream& flagProcessorSharing( std::ostream& output, bool verbose ) const;
    std::ostream& flagSquashedLayering( std::ostream& output, bool verbose ) const;

public:
    std::ostream& debugAll( std::ostream & output, bool verbose ) const;
    std::ostream& debugActivities( std::ostream & output, bool verbose ) const;
    std::ostream& debugCalls( std::ostream & output, bool verbose ) const;
    std::ostream& debugForks( std::ostream & output, bool verbose ) const;
    std::ostream& debugInterlock( std::ostream & output, bool verbose ) const;
    std::ostream& debugJoins( std::ostream & output, bool verbose ) const;
    std::ostream& debugLQX( std::ostream & output, bool verbose ) const;
    std::ostream& debugMVA( std::ostream & output, bool verbose ) const;
    std::ostream& debugLayers( std::ostream & output, bool verbose ) const;
    std::ostream& debugOvertaking( std::ostream & output, bool verbose ) const;
    std::ostream& debugQuorum( std::ostream & output, bool verbose ) const;
    std::ostream& debugVariance( std::ostream & output, bool verbose ) const;
    std::ostream& debugXML( std::ostream & output, bool verbose ) const;

    std::ostream& traceActivities( std::ostream & output, bool verbose ) const;
    std::ostream& traceConvergence( std::ostream & output, bool verbose ) const;
    std::ostream& traceDeltaWait( std::ostream & output, bool verbose ) const;
    std::ostream& traceForks( std::ostream & output, bool verbose ) const;
    std::ostream& traceIdleTime( std::ostream & output, bool verbose ) const;
    std::ostream& traceInterlock( std::ostream & output, bool verbose ) const;
    std::ostream& traceIntermediate( std::ostream & output, bool verbose ) const;
    std::ostream& traceJoins( std::ostream & output, bool verbose ) const;
    std::ostream& traceMva( std::ostream & output, bool verbose ) const;
    std::ostream& traceOvertaking( std::ostream & output, bool verbose ) const;
    std::ostream& traceQuorum( std::ostream & output, bool verbose ) const;
    std::ostream& traceReplication( std::ostream & output, bool verbose ) const;
    std::ostream& traceThroughput( std::ostream & output, bool verbose ) const;
    std::ostream& traceVariance( std::ostream & output, bool verbose ) const;
    std::ostream& traceVirtualEntry( std::ostream & output, bool verbose ) const;
    std::ostream& traceWait( std::ostream & output, bool verbose ) const;

    std::ostream& specialIterationLimit( std::ostream & output, bool verbose ) const;
    std::ostream& specialPrintInterval( std::ostream & output, bool verbose ) const;
    std::ostream& specialOvertaking( std::ostream & output, bool verbose ) const;
    std::ostream& specialConvergenceValue( std::ostream & output, bool verbose ) const;
    std::ostream& specialSingleStep( std::ostream & output, bool verbose ) const;
    std::ostream& specialUnderrelaxation( std::ostream & output, bool verbose ) const;
    std::ostream& specialGenerateQueueingModel( std::ostream & output, bool verbose ) const;
    std::ostream& specialMolMSUnderrelaxation( std::ostream & output, bool verbose ) const;
    std::ostream& speicalSkipLayer( std::ostream & output, bool verbose ) const;
    std::ostream& specialMakeMan( std::ostream & output, bool verbose ) const;
    std::ostream& specialMakeTex( std::ostream & output, bool verbose ) const;
    std::ostream& specialMinSteps( std::ostream & output, bool verbose ) const;
    std::ostream& specialIgnoreOverhangingThreads( std::ostream & output, bool verbose ) const;
    std::ostream& specialFullReinitialize( std::ostream & output, bool verbose ) const;

    std::ostream& pragmaCycles( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaStopOnMessageLoss( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceMultiserver( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaInterlock( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaLayering( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiserver( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMVA( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaOvertaking( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaProcessor( std::ostream& output, bool verbose ) const;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    std::ostream& pragmaQuorumDistribution( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaQuorumDelayedCalls( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaIdleTime( std::ostream& output, bool verbose ) const;
#endif
#if RESCHEDULE
    std::ostream& pragmaReschedule( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaRescheduleTrue( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaRescheduleFalse( std::ostream& output, bool verbose ) const;
#endif
    std::ostream& pragmaTau( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaThreads( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaVariance( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSeverityLevel( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexHeader( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaPrune( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaCyclesAllow( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaCyclesDisallow( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaForceMultiserverNone( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceMultiserverProcessors( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceMultiserverTasks( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceMultiserverAll( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaStopOnMessageLossFalse( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaStopOnMessageLossTrue( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaInterlockThroughput( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaInterlockNone( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaLayeringBatched( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaLayeringBatchedBack( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaLayeringHwSw( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaLayeringMOL( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaLayeringMOLBack( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaLayeringSquashed( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaLayeringSRVN( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaMultiServerDefault( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerConway( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerReiser( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerReiserPS( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerRolia( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerRoliaPS( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerBruell( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerSchmidt( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiServerSuri( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaMVALinearizer( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMVAExact( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMVASchweitzer( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMVAFast( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMVAOneStep( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMVAOneStepLinearizer( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaOvertakingMarkov( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaOvertakingRolia( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaOvertakingSimple( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaOvertakingSpecial( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaOvertakingNone( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaProcessorDefault( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaProcessorFCFS( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaProcessorHOL( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaProcessorPPR( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaProcessorPS( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaThreadsNone( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaThreadsMak( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaThreadsHyper( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaThreadsExponential( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaThreadsDefault( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaVarianceDefault( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaVarianceNone( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaVarianceStochastic( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaVarianceMol( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaVarianceNoEntry( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaVarianceInitOnly( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaSeverityLevelWarnings( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSeverityLevelRunTime( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaSpexHeaderFalse( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexHeaderTrue( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaPruneFalse( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaPruneTrue( std::ostream& output, bool verbose ) const;

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    std::ostream& pragmaQuorumDistributionDefault( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaQuorumDistributionThreepoint( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaQuorumDistributionGamma( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaQuorumDistributionClosedFormGeo( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaQuorumDistributionClosedformDet( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaMultiThreadsDefault( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaDelayedThreadsKeepAll( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaDelayedThreadsAbortAll( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaDelayedThreadsAbortLocalOnly( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaDelayedThreadsAbortRemoteOnly( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaIdleTimeDefault( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaIdleTimeJoindelay( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaIdleTimeRootentry( std::ostream& output, bool verbose ) const;
#endif

protected:
    static pragma_map_t __pragmas;
    
private:
    static parameter_map_t  __cycles_args;
    static parameter_map_t  __force_multiserver_args;
    static parameter_map_t  __interlock_args;
    static parameter_map_t  __layering_args;
    static parameter_map_t  __multiserver_args;
    static parameter_map_t  __mva_args;
    static parameter_map_t  __overtaking_args;
    static parameter_map_t  __processor_args;
    static parameter_map_t  __prune_args;
#if RESCHEDULE
    static parameter_map_t  __reschedule_args;
#endif
    static parameter_map_t  __spex_header_args;
    static parameter_map_t  __stop_on_message_loss_args;
    static parameter_map_t  __threads_args;
    static parameter_map_t  __variance_args;
    static parameter_map_t  __warning_args;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    static parameter_map_t  __quorum_distribution_args;
    static parameter_map_t  __quorum_delayed_calls_args;
    static parameter_map_t  __idle_time_args;
#endif
};

class HelpTroff : public Help
{
protected:
    virtual std::ostream& preamble( std::ostream& output ) const;
    virtual std::ostream& see_also( std::ostream& output ) const;
    virtual std::ostream& textbf( std::ostream& output, const char * s ) const;
    virtual std::ostream& textit( std::ostream& output, const char * s ) const;
    virtual std::ostream& filename( std::ostream& output, const char * s1, const char * s2 ) const;
    virtual std::ostream& pp( std::ostream& ouptut ) const;
    virtual std::ostream& br( std::ostream& ouptut ) const;
    virtual std::ostream& ol_begin( std::ostream& output ) const;
    virtual std::ostream& ol_end( std::ostream& output ) const;
    virtual std::ostream& dl_begin( std::ostream& output ) const;
    virtual std::ostream& dl_end( std::ostream& output ) const;
    virtual std::ostream& li( std::ostream& output, const char * s = 0 ) const;
    virtual std::ostream& section( std::ostream& output, const char * s1, const char * s2 ) const;
    virtual std::ostream& label( std::ostream& output, const char * s ) const;
    virtual std::ostream& flag( std::ostream& output, const char * s ) const;
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const;
    virtual std::ostream& increase_indent( std::ostream& output ) const;
    virtual std::ostream& decrease_indent( std::ostream& output ) const;
    virtual std::ostream& print_option( std::ostream&, const char * name, const Options::Option& opt ) const;
    virtual std::ostream& print_pragma( std::ostream&, const std::string& ) const;
    virtual std::ostream& table_header( std::ostream& ) const;
    virtual std::ostream& table_row( std::ostream&, const char *, const char *, const char * ix=0 ) const;
    virtual std::ostream& table_footer( std::ostream& ) const;

private:
    static const char * __comment;
};


class HelpLaTeX : public Help
{
protected:
    virtual std::ostream& preamble( std::ostream& output ) const;
    virtual std::ostream& see_also( std::ostream& output ) const { return output; }
    virtual std::ostream& textbf( std::ostream& output, const char * s ) const;
    virtual std::ostream& textit( std::ostream& output, const char * s ) const;
    virtual std::ostream& tr_( std::ostream& output, const char * ) const;
    virtual std::ostream& filename( std::ostream& output, const char * s1, const char * s2 ) const;
    virtual std::ostream& pp( std::ostream& ouptut ) const;
    virtual std::ostream& br( std::ostream& ouptut ) const;
    virtual std::ostream& ol_begin( std::ostream& output ) const;
    virtual std::ostream& ol_end( std::ostream& output ) const;
    virtual std::ostream& dl_begin( std::ostream& output ) const;
    virtual std::ostream& dl_end( std::ostream& output ) const;
    virtual std::ostream& li( std::ostream& output, const char * s = 0 ) const;
    virtual std::ostream& section( std::ostream& output, const char * s1, const char * s2 ) const;
    virtual std::ostream& label( std::ostream& output, const char * s ) const;
    virtual std::ostream& flag( std::ostream& output, const char * s ) const;
    virtual std::ostream& ix( std::ostream& output, const char * s ) const;
    virtual std::ostream& cite( std::ostream& output, const char * s ) const;
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const;
    virtual std::ostream& increase_indent( std::ostream& output ) const;
    virtual std::ostream& decrease_indent( std::ostream& output ) const;
    virtual std::ostream& print_option( std::ostream&, const char * name, const Options::Option& opt) const;
    virtual std::ostream& print_pragma( std::ostream&, const std::string& ) const;
    virtual std::ostream& table_header( std::ostream& ) const;
    virtual std::ostream& table_row( std::ostream&, const char *, const char *, const char * ix=0 ) const;
    virtual std::ostream& table_footer( std::ostream& ) const;
    virtual std::ostream& trailer( std::ostream& output ) const;

    StringManip tr_( const Help& h, const char * s ) const { return StringManip( &Help::__tr_, h, s ); }

private:
    static const char * __comment;
};

class HelpPlain : public Help
{
protected:
    virtual std::ostream& preamble( std::ostream& output ) const { return output; }
    virtual std::ostream& see_also( std::ostream& output ) const { return output; }
    virtual std::ostream& textbf( std::ostream& output, const char * s ) const;
    virtual std::ostream& textit( std::ostream& output, const char * s ) const;
    virtual std::ostream& tr_( std::ostream& output, const char * ) const { return output; }
    virtual std::ostream& filename( std::ostream& output, const char * s1, const char * s2 ) const;
    virtual std::ostream& pp( std::ostream& output ) const { return output; }
    virtual std::ostream& br( std::ostream& output ) const { return output; }
    virtual std::ostream& ol_begin( std::ostream& output ) const { return output; }
    virtual std::ostream& ol_end( std::ostream& output ) const { return output; }
    virtual std::ostream& dl_begin( std::ostream& output ) const { return output; }
    virtual std::ostream& dl_end( std::ostream& output ) const { return output; }
    virtual std::ostream& li( std::ostream& output, const char * s = 0 ) const { return output; }
    virtual std::ostream& section( std::ostream& output, const char * s1, const char * s2 ) const { return output; }
    virtual std::ostream& label( std::ostream& output, const char * s ) const { return output; }
    virtual std::ostream& flag( std::ostream& output, const char * s ) const { return output; }
    virtual std::ostream& ix( std::ostream& output, const char * s ) const { return output; }
    virtual std::ostream& cite( std::ostream& output, const char * s ) const { return output; }
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const { return output; }
    virtual std::ostream& increase_indent( std::ostream& output ) const { return output; }
    virtual std::ostream& decrease_indent( std::ostream& output ) const { return output; }
    virtual std::ostream& print_option( std::ostream& output, const char * name, const Options::Option& opt) const;
    virtual std::ostream& print_pragma( std::ostream& output, const std::string& ) const;
    virtual std::ostream& table_header( std::ostream& output ) const { return output; }
    virtual std::ostream& table_row( std::ostream& output , const char *, const char *, const char * ix=0 ) const { return output; }
    virtual std::ostream& table_footer( std::ostream& output ) const { return output; }
    virtual std::ostream& trailer( std::ostream& output ) const { return output; }

    StringManip tr_( const Help& h, const char * s ) const { return StringManip( &Help::__tr_, h, s ); }

public:
    static void print_special( std::ostream& output );
    static void print_debug( std::ostream& output );
    static void print_trace( std::ostream& output );
};

inline std::ostream& operator<<( std::ostream& output, const Help& self ) { return self.print( output ); }
#endif
