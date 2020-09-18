/* -*- C++ -*-
 * help.h	-- Greg Franks
 *
 * $Id: help.h 13815 2020-09-14 16:30:47Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

#include <config.h>
#include "dim.h"
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
    StringManip( ostream& (*ff)(ostream&, const Help&, const char * ), const Help& h, const char * s ) : f(ff), _h(h), _s(s) {}
private:
    ostream& (*f)( ostream&, const Help&, const char * );
    const Help & _h;
    const char * _s;

    friend ostream& operator<<(ostream & os, const StringManip& m ) { return m.f(os,m._h,m._s); }
};

class StringStringManip {
public:
    StringStringManip( ostream& (*ff)(ostream&, const Help&, const char *, const char * ), const Help& h, const char * s1, const char * s2 ) : f(ff), _h(h), _s1(s1), _s2(s2) {}
private:
    ostream& (*f)( ostream&, const Help&, const char *, const char * );
    const Help & _h;
    const char * _s1;
    const char * _s2;

    friend ostream& operator<<(ostream & os, const StringStringManip& m ) { return m.f(os,m._h,m._s1,m._s2); }
};

class Help
{    
public:
    typedef ostream& (Help::*help_fptr)( ostream&, bool ) const;

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
    
    static ostream& __textbf( ostream& output, const Help & h, const char * s ) { return h.textbf( output, s ); }
    static ostream& __textit( ostream& output, const Help & h, const char * s ) { return h.textit( output, s ); }
    static ostream& __flag( ostream& output, const Help & h, const char * s ) { return h.flag( output, s ); }
    static ostream& __ix( ostream& output, const Help & h, const char * s ) { return h.ix( output, s ); }
    static ostream& __cite( ostream& output, const Help & h, const char * s ) { return h.cite( output, s ); }
    static ostream& __filename( ostream& output, const Help & h, const char * s1, const char * s2 ) { return h.filename( output, s1, s2 ); }

protected:
    static ostream& __tr_( ostream& output, const Help& h, const char * s ) { return h.tr_( output, s ); }

public:
    Help();
    virtual ~Help() {}

    ostream& print( ostream& ) const;

protected:
   static std::map<const int,help_fptr,lt_int> option_table;


protected:
    virtual ostream& preamble( ostream& output ) const = 0;
    virtual ostream& see_also( ostream& output ) const = 0;
    virtual ostream& textbf( ostream& output, const char * s ) const = 0;
    virtual ostream& textit( ostream& output, const char * s ) const = 0;
    virtual ostream& tr_( ostream& output, const char * ) const { return output; }
    virtual ostream& filename( ostream& output, const char * s1, const char * s2 ) const = 0;
    virtual ostream& pp( ostream& ouptut ) const = 0;
    virtual ostream& br( ostream& ouptut ) const = 0;
    virtual ostream& ol_begin( ostream& output ) const = 0;
    virtual ostream& ol_end( ostream& output ) const = 0;
    virtual ostream& dl_begin( ostream& output ) const = 0;
    virtual ostream& dl_end( ostream& output ) const = 0;
    virtual ostream& li( ostream& output, const char * = 0 ) const = 0;
    virtual ostream& flag( ostream& output, const char * s ) const = 0;
    virtual ostream& ix( ostream& output, const char * s ) const { return output; }
    virtual ostream& cite( ostream& output, const char * s ) const { return output; }
    virtual ostream& section( ostream& output, const char * s, const char * ) const = 0;
    virtual ostream& label( ostream& output, const char * s ) const = 0;
    virtual ostream& longopt( ostream& output, const struct option *o ) const = 0;
    virtual ostream& increase_indent( ostream& output ) const = 0;
    virtual ostream& decrease_indent( ostream& output ) const = 0;
    virtual ostream& print_option( ostream&, const char * name, const Options::Option& opt) const = 0;
    virtual ostream& print_pragma( ostream&, const std::string& ) const = 0;
    virtual ostream& table_header( ostream& ) const = 0;
    virtual ostream& table_row( ostream&, const char *, const char *, const char * ix=0 ) const = 0;
    virtual ostream& table_footer( ostream& ) const = 0;
    virtual ostream& trailer( ostream& output ) const { return output; }

private:
    ostream& flagAdvisory( ostream& output, bool verbose ) const;
    ostream& flagBound( ostream& output, bool verbose ) const;
    ostream& flagDebug( ostream& output, bool verbose ) const;
    ostream& flagError( ostream& output, bool verbose ) const;
    ostream& flagGnuplot( ostream& output, bool verbose ) const;
    ostream& flagFast( ostream& output, bool verbose ) const;
    ostream& flagInputFormat( ostream& output, bool verbose ) const;
    ostream& flagNoExecute( ostream& output, bool verbose ) const;
    ostream& flagOutput( ostream& output, bool verbose ) const;
    ostream& flagParseable( ostream& output, bool verbose ) const;
    ostream& flagPragmas( ostream& output, bool verbose ) const;
    ostream& flagRTF( ostream& output, bool verbose ) const;
    ostream& flagTrace( ostream& output, bool verbose ) const;
    ostream& flagVerbose( ostream& output, bool verbose ) const;
    ostream& flagVersion( ostream& output, bool verbose ) const;
    ostream& flagWarning( ostream& output, bool verbose ) const;
    ostream& flagXML( ostream& output, bool verbose ) const;
    ostream& flagSpecial( ostream& output, bool verbose ) const;
    ostream& flagConvergence( ostream& output, bool verbose ) const;
    ostream& flagUnderrelaxation( ostream& output, bool verbose ) const;
    ostream& flagIterationLimit( ostream& output, bool verbose ) const;
    ostream& flagExactMVA( ostream& output, bool verbose ) const;
    ostream& flagSchweitzerMVA( ostream& output, bool verbose ) const;
    ostream& flagHwSwLayering( ostream& output, bool verbose ) const;
    ostream& flagLoose( ostream& output, bool verbose ) const;
    ostream& flagStopOnMessageLoss( ostream& output, bool verbose ) const;
    ostream& flagTraceMVA( ostream& output, bool verbose ) const;
    ostream& flagNoVariance( ostream& output, bool verbose ) const;
    ostream& flagNoHeader( ostream& output, bool verbose ) const;
    ostream& flagReloadLQX( ostream& output, bool verbose ) const;
    ostream& flagRestartLQX( ostream& output, bool verbose ) const;
    ostream& flagDebugLQX( ostream& output, bool verbose ) const;
    ostream& flagDebugXML( ostream& output, bool verbose ) const;
    ostream& flagMethoOfLayers( ostream& output, bool verbose ) const;
    ostream& flagProcessorSharing( ostream& output, bool verbose ) const;
    ostream& flagSquashedLayering( ostream& output, bool verbose ) const;

public:
    ostream& debugAll( ostream & output, bool verbose ) const;
    ostream& debugActivities( ostream & output, bool verbose ) const;
    ostream& debugCalls( ostream & output, bool verbose ) const;
    ostream& debugForks( ostream & output, bool verbose ) const;
    ostream& debugInterlock( ostream & output, bool verbose ) const;
    ostream& debugJoins( ostream & output, bool verbose ) const;
    ostream& debugLQX( ostream & output, bool verbose ) const;
    ostream& debugMVA( ostream & output, bool verbose ) const;
    ostream& debugLayers( ostream & output, bool verbose ) const;
    ostream& debugOvertaking( ostream & output, bool verbose ) const;
    ostream& debugQuorum( ostream & output, bool verbose ) const;
    ostream& debugVariance( ostream & output, bool verbose ) const;
    ostream& debugXML( ostream & output, bool verbose ) const;

    ostream& traceActivities( ostream & output, bool verbose ) const;
    ostream& traceConvergence( ostream & output, bool verbose ) const;
    ostream& traceDeltaWait( ostream & output, bool verbose ) const;
    ostream& traceForks( ostream & output, bool verbose ) const;
    ostream& traceIdleTime( ostream & output, bool verbose ) const;
    ostream& traceInterlock( ostream & output, bool verbose ) const;
    ostream& traceJoins( ostream & output, bool verbose ) const;
    ostream& traceMva( ostream & output, bool verbose ) const;
    ostream& traceOvertaking( ostream & output, bool verbose ) const;
    ostream& traceIntermediate( ostream & output, bool verbose ) const;
    ostream& traceReplication( ostream & output, bool verbose ) const;
    ostream& traceVariance( ostream & output, bool verbose ) const;
    ostream& traceWait( ostream & output, bool verbose ) const;
    ostream& traceThroughput( ostream & output, bool verbose ) const;
    ostream& traceQuorum( ostream & output, bool verbose ) const;

    ostream& specialIterationLimit( ostream & output, bool verbose ) const;
    ostream& specialPrintInterval( ostream & output, bool verbose ) const;
    ostream& specialOvertaking( ostream & output, bool verbose ) const;
    ostream& specialConvergenceValue( ostream & output, bool verbose ) const;
    ostream& specialSingleStep( ostream & output, bool verbose ) const;
    ostream& specialUnderrelaxation( ostream & output, bool verbose ) const;
    ostream& specialGenerateQueueingModel( ostream & output, bool verbose ) const;
    ostream& specialMolMSUnderrelaxation( ostream & output, bool verbose ) const;
    ostream& speicalSkipLayer( ostream & output, bool verbose ) const;
    ostream& specialMakeMan( ostream & output, bool verbose ) const;
    ostream& specialMakeTex( ostream & output, bool verbose ) const;
    ostream& specialMinSteps( ostream & output, bool verbose ) const;
    ostream& specialIgnoreOverhangingThreads( ostream & output, bool verbose ) const;
    ostream& specialFullReinitialize( ostream & output, bool verbose ) const;

    ostream& pragmaCycles( ostream& output, bool verbose ) const;
    ostream& pragmaStopOnMessageLoss( ostream& output, bool verbose ) const;
    ostream& pragmaForceMultiserver( ostream& output, bool verbose ) const;
    ostream& pragmaInterlock( ostream& output, bool verbose ) const;
    ostream& pragmaLayering( ostream& output, bool verbose ) const;
    ostream& pragmaMultiserver( ostream& output, bool verbose ) const;
    ostream& pragmaMVA( ostream& output, bool verbose ) const;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    ostream& pragmaQuorumDistribution( ostream& output, bool verbose ) const;
    ostream& pragmaQuorumDelayedCalls( ostream& output, bool verbose ) const;
    ostream& pragmaIdleTime( ostream& output, bool verbose ) const;
#endif
    ostream& pragmaOvertaking( ostream& output, bool verbose ) const;
    ostream& pragmaProcessor( ostream& output, bool verbose ) const;
#if RESCHEDULE
    ostream& pragmaReschedule( ostream& output, bool verbose ) const;
#endif
    ostream& pragmaTau( ostream& output, bool verbose ) const;
    ostream& pragmaThreads( ostream& output, bool verbose ) const;
    ostream& pragmaVariance( ostream& output, bool verbose ) const;
    ostream& pragmaSeverityLevel( ostream& output, bool verbose ) const;
    ostream& pragmaSpexHeader( ostream& output, bool verbose ) const;
    ostream& pragmaPrune( ostream& output, bool verbose ) const;

    ostream& pragmaCyclesAllow( ostream& output, bool verbose ) const;
    ostream& pragmaCyclesDisallow( ostream& output, bool verbose ) const;

    ostream& pragmaForceMultiserverNone( ostream& output, bool verbose ) const;
    ostream& pragmaForceMultiserverProcessors( ostream& output, bool verbose ) const;
    ostream& pragmaForceMultiserverTasks( ostream& output, bool verbose ) const;
    ostream& pragmaForceMultiserverAll( ostream& output, bool verbose ) const;

    ostream& pragmaStopOnMessageLossFalse( ostream& output, bool verbose ) const;
    ostream& pragmaStopOnMessageLossTrue( ostream& output, bool verbose ) const;

    ostream& pragmaInterlockThroughput( ostream& output, bool verbose ) const;
    ostream& pragmaInterlockNone( ostream& output, bool verbose ) const;

    ostream& pragmaLayeringBatched( ostream& output, bool verbose ) const;
    ostream& pragmaLayeringBatchedBack( ostream& output, bool verbose ) const;
    ostream& pragmaLayeringHwSw( ostream& output, bool verbose ) const;
    ostream& pragmaLayeringMOL( ostream& output, bool verbose ) const;
    ostream& pragmaLayeringMOLBack( ostream& output, bool verbose ) const;
    ostream& pragmaLayeringSquashed( ostream& output, bool verbose ) const;
    ostream& pragmaLayeringSRVN( ostream& output, bool verbose ) const;

    ostream& pragmaMultiServerDefault( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerConway( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerReiser( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerReiserPS( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerRolia( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerRoliaPS( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerBruell( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerSchmidt( ostream& output, bool verbose ) const;
    ostream& pragmaMultiServerSuri( ostream& output, bool verbose ) const;

    ostream& pragmaMVALinearizer( ostream& output, bool verbose ) const;
    ostream& pragmaMVAExact( ostream& output, bool verbose ) const;
    ostream& pragmaMVASchweitzer( ostream& output, bool verbose ) const;
    ostream& pragmaMVAFast( ostream& output, bool verbose ) const;
    ostream& pragmaMVAOneStep( ostream& output, bool verbose ) const;
    ostream& pragmaMVAOneStepLinearizer( ostream& output, bool verbose ) const;

    ostream& pragmaOvertakingMarkov( ostream& output, bool verbose ) const;
    ostream& pragmaOvertakingRolia( ostream& output, bool verbose ) const;
    ostream& pragmaOvertakingSimple( ostream& output, bool verbose ) const;
    ostream& pragmaOvertakingSpecial( ostream& output, bool verbose ) const;
    ostream& pragmaOvertakingNone( ostream& output, bool verbose ) const;

    ostream& pragmaProcessorDefault( ostream& output, bool verbose ) const;
    ostream& pragmaProcessorFCFS( ostream& output, bool verbose ) const;
    ostream& pragmaProcessorHOL( ostream& output, bool verbose ) const;
    ostream& pragmaProcessorPPR( ostream& output, bool verbose ) const;
    ostream& pragmaProcessorPS( ostream& output, bool verbose ) const;

    ostream& pragmaThreadsNone( ostream& output, bool verbose ) const;
    ostream& pragmaThreadsMak( ostream& output, bool verbose ) const;
    ostream& pragmaThreadsHyper( ostream& output, bool verbose ) const;
    ostream& pragmaThreadsExponential( ostream& output, bool verbose ) const;
    ostream& pragmaThreadsDefault( ostream& output, bool verbose ) const;

    ostream& pragmaVarianceDefault( ostream& output, bool verbose ) const;
    ostream& pragmaVarianceNone( ostream& output, bool verbose ) const;
    ostream& pragmaVarianceStochastic( ostream& output, bool verbose ) const;
    ostream& pragmaVarianceMol( ostream& output, bool verbose ) const;
    ostream& pragmaVarianceNoEntry( ostream& output, bool verbose ) const;
    ostream& pragmaVarianceInitOnly( ostream& output, bool verbose ) const;

    ostream& pragmaSeverityLevelWarnings( ostream& output, bool verbose ) const;
    ostream& pragmaSeverityLevelRunTime( ostream& output, bool verbose ) const;

    ostream& pragmaSpexHeaderFalse( ostream& output, bool verbose ) const;
    ostream& pragmaSpexHeaderTrue( ostream& output, bool verbose ) const;

    ostream& pragmaPruneFalse( ostream& output, bool verbose ) const;
    ostream& pragmaPruneTrue( ostream& output, bool verbose ) const;

#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    ostream& pragmaQuorumDistributionDefault( ostream& output, bool verbose ) const;
    ostream& pragmaQuorumDistributionThreepoint( ostream& output, bool verbose ) const;
    ostream& pragmaQuorumDistributionGamma( ostream& output, bool verbose ) const;
    ostream& pragmaQuorumDistributionClosedFormGeo( ostream& output, bool verbose ) const;
    ostream& pragmaQuorumDistributionClosedformDet( ostream& output, bool verbose ) const;

    ostream& pragmaMultiThreadsDefault( ostream& output, bool verbose ) const;
    ostream& pragmaDelayedThreadsKeepAll( ostream& output, bool verbose ) const;
    ostream& pragmaDelayedThreadsAbortAll( ostream& output, bool verbose ) const;
    ostream& pragmaDelayedThreadsAbortLocalOnly( ostream& output, bool verbose ) const;
    ostream& pragmaDelayedThreadsAbortRemoteOnly( ostream& output, bool verbose ) const;

    ostream& pragmaIdleTimeDefault( ostream& output, bool verbose ) const;
    ostream& pragmaIdleTimeJoindelay( ostream& output, bool verbose ) const;
    ostream& pragmaIdleTimeRootentry( ostream& output, bool verbose ) const;
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
    virtual ostream& preamble( ostream& output ) const;
    virtual ostream& see_also( ostream& output ) const;
    virtual ostream& textbf( ostream& output, const char * s ) const;
    virtual ostream& textit( ostream& output, const char * s ) const;
    virtual ostream& filename( ostream& output, const char * s1, const char * s2 ) const;
    virtual ostream& pp( ostream& ouptut ) const;
    virtual ostream& br( ostream& ouptut ) const;
    virtual ostream& ol_begin( ostream& output ) const;
    virtual ostream& ol_end( ostream& output ) const;
    virtual ostream& dl_begin( ostream& output ) const;
    virtual ostream& dl_end( ostream& output ) const;
    virtual ostream& li( ostream& output, const char * s = 0 ) const;
    virtual ostream& section( ostream& output, const char * s1, const char * s2 ) const;
    virtual ostream& label( ostream& output, const char * s ) const;
    virtual ostream& flag( ostream& output, const char * s ) const;
    virtual ostream& longopt( ostream& output, const struct option *o ) const;
    virtual ostream& increase_indent( ostream& output ) const;
    virtual ostream& decrease_indent( ostream& output ) const;
    virtual ostream& print_option( ostream&, const char * name, const Options::Option& opt ) const;
    virtual ostream& print_pragma( ostream&, const std::string& ) const;
    virtual ostream& table_header( ostream& ) const;
    virtual ostream& table_row( ostream&, const char *, const char *, const char * ix=0 ) const;
    virtual ostream& table_footer( ostream& ) const;

private:
    static const char * __comment;
};


class HelpLaTeX : public Help
{
protected:
    virtual ostream& preamble( ostream& output ) const;
    virtual ostream& see_also( ostream& output ) const { return output; }
    virtual ostream& textbf( ostream& output, const char * s ) const;
    virtual ostream& textit( ostream& output, const char * s ) const;
    virtual ostream& tr_( ostream& output, const char * ) const;
    virtual ostream& filename( ostream& output, const char * s1, const char * s2 ) const;
    virtual ostream& pp( ostream& ouptut ) const;
    virtual ostream& br( ostream& ouptut ) const;
    virtual ostream& ol_begin( ostream& output ) const;
    virtual ostream& ol_end( ostream& output ) const;
    virtual ostream& dl_begin( ostream& output ) const;
    virtual ostream& dl_end( ostream& output ) const;
    virtual ostream& li( ostream& output, const char * s = 0 ) const;
    virtual ostream& section( ostream& output, const char * s1, const char * s2 ) const;
    virtual ostream& label( ostream& output, const char * s ) const;
    virtual ostream& flag( ostream& output, const char * s ) const;
    virtual ostream& ix( ostream& output, const char * s ) const;
    virtual ostream& cite( ostream& output, const char * s ) const;
    virtual ostream& longopt( ostream& output, const struct option *o ) const;
    virtual ostream& increase_indent( ostream& output ) const;
    virtual ostream& decrease_indent( ostream& output ) const;
    virtual ostream& print_option( ostream&, const char * name, const Options::Option& opt) const;
    virtual ostream& print_pragma( ostream&, const std::string& ) const;
    virtual ostream& table_header( ostream& ) const;
    virtual ostream& table_row( ostream&, const char *, const char *, const char * ix=0 ) const;
    virtual ostream& table_footer( ostream& ) const;
    virtual ostream& trailer( ostream& output ) const;

    StringManip tr_( const Help& h, const char * s ) const { return StringManip( &Help::__tr_, h, s ); }

private:
    static const char * __comment;
};

class HelpPlain : public Help
{
protected:
    virtual ostream& preamble( ostream& output ) const { return output; }
    virtual ostream& see_also( ostream& output ) const { return output; }
    virtual ostream& textbf( ostream& output, const char * s ) const;
    virtual ostream& textit( ostream& output, const char * s ) const;
    virtual ostream& tr_( ostream& output, const char * ) const { return output; }
    virtual ostream& filename( ostream& output, const char * s1, const char * s2 ) const;
    virtual ostream& pp( ostream& output ) const { return output; }
    virtual ostream& br( ostream& output ) const { return output; }
    virtual ostream& ol_begin( ostream& output ) const { return output; }
    virtual ostream& ol_end( ostream& output ) const { return output; }
    virtual ostream& dl_begin( ostream& output ) const { return output; }
    virtual ostream& dl_end( ostream& output ) const { return output; }
    virtual ostream& li( ostream& output, const char * s = 0 ) const { return output; }
    virtual ostream& section( ostream& output, const char * s1, const char * s2 ) const { return output; }
    virtual ostream& label( ostream& output, const char * s ) const { return output; }
    virtual ostream& flag( ostream& output, const char * s ) const { return output; }
    virtual ostream& ix( ostream& output, const char * s ) const { return output; }
    virtual ostream& cite( ostream& output, const char * s ) const { return output; }
    virtual ostream& longopt( ostream& output, const struct option *o ) const { return output; }
    virtual ostream& increase_indent( ostream& output ) const { return output; }
    virtual ostream& decrease_indent( ostream& output ) const { return output; }
    virtual ostream& print_option( ostream& output, const char * name, const Options::Option& opt) const;
    virtual ostream& print_pragma( ostream& output, const std::string& ) const;
    virtual ostream& table_header( ostream& output ) const { return output; }
    virtual ostream& table_row( ostream& output , const char *, const char *, const char * ix=0 ) const { return output; }
    virtual ostream& table_footer( ostream& output ) const { return output; }
    virtual ostream& trailer( ostream& output ) const { return output; }

    StringManip tr_( const Help& h, const char * s ) const { return StringManip( &Help::__tr_, h, s ); }

public:
    static void print_special( ostream& output );
    static void print_debug( ostream& output );
    static void print_trace( ostream& output );
};

inline ostream& operator<<( ostream& output, const Help& self ) { return self.print( output ); }
#endif
