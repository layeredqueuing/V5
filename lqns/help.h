/* -*- C++ -*-
 * help.h	-- Greg Franks
 *
 * $Id: help.h 17099 2024-03-04 22:02:11Z greg $
 */

#ifndef _HELP_H
#define _HELP_H

#include <map>

namespace Options {
    class Option;
}

void usage( const char * = 0 );
void usage( const char );
void usage( const char c, const std::string& s );

class Help;

extern const char opts[];
#if HAVE_GETOPT_LONG
extern const struct option longopts[];
#endif

class StringManip {
public:
    StringManip( std::ostream& (*ff)(std::ostream&, const Help&, const std::string& ), const Help& h, const std::string& s ) : f(ff), _h(h), _s(s) {}
private:
    std::ostream& (*f)( std::ostream&, const Help&, const std::string& );
    const Help & _h;
    const std::string& _s;

    friend std::ostream& operator<<(std::ostream & os, const StringManip& m ) { return m.f(os,m._h,m._s); }
};

class StringStringManip {
public:
    StringStringManip( std::ostream& (*ff)(std::ostream&, const Help&, const std::string&, const std::string& ), const Help& h, const std::string& s1, const std::string& s2 ) : f(ff), _h(h), _s1(s1), _s2(s2) {}
private:
    std::ostream& (*f)( std::ostream&, const Help&, const std::string&, const std::string& );
    const Help & _h;
    const std::string& _s1;
    const std::string& _s2;

    friend std::ostream& operator<<(std::ostream & os, const StringStringManip& m ) { return m.f(os,m._h,m._s1,m._s2); }
};

class Help
{    
public:
    typedef std::ostream& (Help::*help_fptr)( std::ostream&, bool ) const;

    struct parameter_info {
	parameter_info() : _help(nullptr), _default(false) {}
	parameter_info( Help::help_fptr h, bool d=false) : _help(h), _default(d) {}
	const Help::help_fptr _help;
	const bool _default;
    };
    typedef std::map<const std::string,const parameter_info> parameter_map_t;
    
    struct pragma_info {
	pragma_info() : _help(nullptr), _value(nullptr) {}
	pragma_info( Help::help_fptr h, const std::map<const std::string,const parameter_info>* v=nullptr ) : _help(h), _value(v) {}
	const Help::help_fptr _help;
	const parameter_map_t* _value;
    };

    typedef std::map<const std::string,const Help::pragma_info> pragma_map_t;

public:
    static void initialize();

    static StringManip bold( const Help& h, const std::string& s ) { return StringManip( &Help::__textbf, h, s ); }
    static StringManip emph( const Help& h, const std::string& s ) { return StringManip( &Help::__textit, h, s ); }
    static StringManip flag( const Help& h, const std::string& s ) { return StringManip( &Help::__flag, h, s ); }
    static StringManip ix( const Help& h, const std::string& s ) { return StringManip( &Help::__ix, h, s ); }
    static StringManip cite( const Help& h, const std::string& s ) { return StringManip( &Help::__cite, h, s ); }
    static StringManip tr( const Help& h, const std::string& s ) { return StringManip( &Help::__tr, h, s ); }
    static StringStringManip filename( const Help& h, const std::string& s1, const std::string& s2 = "" ) { return StringStringManip( &Help::__filename, h, s1, s2 ); }

private:
    Help( const Help& ) = delete;
    Help& operator=( const Help& ) = delete;
    
    static std::ostream& __textbf( std::ostream& output, const Help & h, const std::string& s ) { return h.textbf( output, s ); }
    static std::ostream& __textit( std::ostream& output, const Help & h, const std::string& s ) { return h.textit( output, s ); }
    static std::ostream& __flag( std::ostream& output, const Help & h, const std::string& s ) { return h.flag( output, s ); }
    static std::ostream& __ix( std::ostream& output, const Help & h, const std::string& s ) { return h.ix( output, s ); }
    static std::ostream& __cite( std::ostream& output, const Help & h, const std::string& s ) { return h.cite( output, s ); }
    static std::ostream& __filename( std::ostream& output, const Help & h, const std::string& s1, const std::string& s2 ) { return h.filename( output, s1, s2 ); }
    static std::ostream& __tr( std::ostream& output, const Help& h, const std::string& s ) { return h.tr( output, s ); }

public:
    Help() {}
    virtual ~Help() {}

    std::ostream& print( std::ostream& ) const;

protected:
    static const std::map<const int,const help_fptr> __option_table;

protected:
    virtual std::ostream& preamble( std::ostream& output ) const = 0;
    virtual std::ostream& see_also( std::ostream& output ) const = 0;
    virtual std::ostream& textbf( std::ostream& output, const std::string& s ) const = 0;
    virtual std::ostream& textit( std::ostream& output, const std::string& s ) const = 0;
    virtual std::ostream& tr( std::ostream& output, const std::string& ) const { return output; }
    virtual std::ostream& filename( std::ostream& output, const std::string& s1, const std::string& s2 ) const = 0;
    virtual std::ostream& pp( std::ostream& ouptut ) const = 0;
    virtual std::ostream& br( std::ostream& ouptut ) const = 0;
    virtual std::ostream& ol_begin( std::ostream& output ) const = 0;
    virtual std::ostream& ol_end( std::ostream& output ) const = 0;
    virtual std::ostream& dl_begin( std::ostream& output ) const = 0;
    virtual std::ostream& dl_end( std::ostream& output ) const = 0;
    virtual std::ostream& li( std::ostream& output, const std::string& = "" ) const = 0;
    virtual std::ostream& flag( std::ostream& output, const std::string& s ) const = 0;
    virtual std::ostream& ix( std::ostream& output, const std::string& s ) const { return output; }
    virtual std::ostream& cite( std::ostream& output, const std::string& s ) const { return output; }
    virtual std::ostream& section( std::ostream& output, const std::string& s, const std::string& ) const = 0;
    virtual std::ostream& label( std::ostream& output, const std::string& s ) const = 0;
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const = 0;
    virtual std::ostream& increase_indent( std::ostream& output ) const = 0;
    virtual std::ostream& decrease_indent( std::ostream& output ) const = 0;
    virtual std::ostream& print_option( std::ostream&, const std::string& name, const Options::Option& opt) const = 0;
    virtual std::ostream& print_pragma( std::ostream&, const std::string& ) const = 0;
    virtual std::ostream& table_header( std::ostream& ) const = 0;
    virtual std::ostream& table_row( std::ostream&, const std::string&, const std::string&, const std::string& ix = "" ) const = 0;
    virtual std::ostream& table_footer( std::ostream& ) const = 0;
    virtual std::ostream& trailer( std::ostream& output ) const { return output; }

private:
    std::ostream& flagAdvisory( std::ostream& output, bool verbose ) const;
    std::ostream& flagBatch( std::ostream& output, bool verbose ) const;
    std::ostream& flagBound( std::ostream& output, bool verbose ) const;
    std::ostream& flagConvergence( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebug( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebugJSON( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebugLQX( std::ostream& output, bool verbose ) const;
    std::ostream& flagPrintSPEX( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebugSRVN( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebugSubmodels( std::ostream& output, bool verbose ) const;
    std::ostream& flagDebugXML( std::ostream& output, bool verbose ) const;
    std::ostream& flagError( std::ostream& output, bool verbose ) const;
    std::ostream& flagExactMVA( std::ostream& output, bool verbose ) const;
    std::ostream& flagFast( std::ostream& output, bool verbose ) const;
    std::ostream& flagHelp( std::ostream& output, bool verbose ) const;
    std::ostream& flagHuge( std::ostream& output, bool verbose ) const;
    std::ostream& flagHwSwLayering( std::ostream& output, bool verbose ) const;
    std::ostream& flagInputFormat( std::ostream& output, bool verbose ) const;
    std::ostream& flagIterationLimit( std::ostream& output, bool verbose ) const;
    std::ostream& flagJSON( std::ostream& output, bool verbose ) const;
    std::ostream& flagMOLUnderrelaxation( std::ostream& output, bool verbose ) const;
    std::ostream& flagMethoOfLayers( std::ostream& output, bool verbose ) const;
    std::ostream& flagNoExecute( std::ostream& output, bool verbose ) const;
    std::ostream& flagNoHeader( std::ostream& output, bool verbose ) const;
    std::ostream& flagNoVariance( std::ostream& output, bool verbose ) const;
    std::ostream& flagOutput( std::ostream& output, bool verbose ) const;
    std::ostream& flagParseable( std::ostream& output, bool verbose ) const;
    std::ostream& flagPragmas( std::ostream& output, bool verbose ) const;
    std::ostream& flagPrintComment( std::ostream& output, bool verbose ) const;
    std::ostream& flagPrintInterval( std::ostream& output, bool verbose ) const;
    std::ostream& flagProcessorSharing( std::ostream& output, bool verbose ) const;
    std::ostream& flagRTF( std::ostream& output, bool verbose ) const;
    std::ostream& flagReloadLQX( std::ostream& output, bool verbose ) const;
    std::ostream& flagResetMVA( std::ostream& output, bool verbose ) const;
    std::ostream& flagRestartLQX( std::ostream& output, bool verbose ) const;
    std::ostream& flagSRVNLayering( std::ostream& output, bool verbose ) const;
    std::ostream& flagSchweitzerMVA( std::ostream& output, bool verbose ) const;
    std::ostream& flagSpecial( std::ostream& output, bool verbose ) const;
    std::ostream& flagSPEXConvergence( std::ostream& output, bool verbose ) const;
    std::ostream& flagSquashedLayering( std::ostream& output, bool verbose ) const;
    std::ostream& flagStopOnMessageLoss( std::ostream& output, bool verbose ) const;
    std::ostream& flagTrace( std::ostream& output, bool verbose ) const;
    std::ostream& flagTraceMVA( std::ostream& output, bool verbose ) const;
    std::ostream& flagUnderrelaxation( std::ostream& output, bool verbose ) const;
    std::ostream& flagVerbose( std::ostream& output, bool verbose ) const;
    std::ostream& flagVersion( std::ostream& output, bool verbose ) const;
    std::ostream& flagWarning( std::ostream& output, bool verbose ) const;
    std::ostream& flagXML( std::ostream& output, bool verbose ) const;

public:
    std::ostream& debugActivities( std::ostream & output, bool verbose ) const;
    std::ostream& debugCalls( std::ostream & output, bool verbose ) const;
    std::ostream& debugForks( std::ostream & output, bool verbose ) const;
    std::ostream& debugInterlock( std::ostream & output, bool verbose ) const;
    std::ostream& debugJSON( std::ostream& output, bool verbose ) const;
    std::ostream& debugJoins( std::ostream & output, bool verbose ) const;
    std::ostream& debugLQX( std::ostream & output, bool verbose ) const;
    std::ostream& debugMVA( std::ostream & output, bool verbose ) const;
    std::ostream& debugOvertaking( std::ostream & output, bool verbose ) const;
    std::ostream& debugQuorum( std::ostream & output, bool verbose ) const;
    std::ostream& debugReplication( std::ostream & output, bool verbose ) const;
    std::ostream& printSPEX( std::ostream& output, bool verbose ) const;
    std::ostream& debugSRVN( std::ostream& output, bool verbose ) const;
    std::ostream& debugSubmodels( std::ostream & output, bool verbose ) const;
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
    std::ostream& traceMVA( std::ostream & output, bool verbose ) const;
    std::ostream& traceOvertaking( std::ostream & output, bool verbose ) const;
    std::ostream& traceQuorum( std::ostream & output, bool verbose ) const;
    std::ostream& traceReplication( std::ostream & output, bool verbose ) const;
    std::ostream& traceThroughput( std::ostream & output, bool verbose ) const;
    std::ostream& traceVariance( std::ostream & output, bool verbose ) const;
    std::ostream& traceVirtualEntry( std::ostream & output, bool verbose ) const;
    std::ostream& traceWait( std::ostream & output, bool verbose ) const;

    std::ostream& specialFullReinitialize( std::ostream & output, bool verbose ) const;
    std::ostream& specialGenerateJMVAOutput( std::ostream & output, bool verbose ) const;
    std::ostream& specialGenerateQNAPOutput( std::ostream & output, bool verbose ) const;
    std::ostream& specialGenerateQueueingModel( std::ostream & output, bool verbose ) const;
    std::ostream& specialIgnoreOverhangingThreads( std::ostream & output, bool verbose ) const;
    std::ostream& specialMakeMan( std::ostream & output, bool verbose ) const;
    std::ostream& specialMakeTex( std::ostream & output, bool verbose ) const;
    std::ostream& specialMinSteps( std::ostream & output, bool verbose ) const;
    std::ostream& specialPrintInterval( std::ostream & output, bool verbose ) const;
    std::ostream& specialSingleStep( std::ostream & output, bool verbose ) const;
    std::ostream& speicalSkipLayer( std::ostream & output, bool verbose ) const;

    std::ostream& pragmaConvergenceValue( std::ostream & output, bool verbose ) const;
    std::ostream& pragmaCycles( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceInfinite( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceMultiserver( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaInterlock( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaIterationLimit( std::ostream & output, bool verbose ) const;
    std::ostream& pragmaLayering( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMOLUnderrelaxation( std::ostream & output, bool verbose ) const;
    std::ostream& pragmaMVA( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaMultiserver( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaOvertaking( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaProcessor( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaPrune( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaStopOnMessageLoss( std::ostream& output, bool verbose ) const;
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
    std::ostream& pragmaSaveMarginalProbabilities( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSeverityLevel( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexComment( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexConvergence( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexHeader( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexIterationLimit( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexUnderrelaxation( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaTau( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaThreads( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaUnderrelaxation( std::ostream & output, bool verbose ) const;
    std::ostream& pragmaVariance( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaCyclesAllow( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaCyclesDisallow( std::ostream& output, bool verbose ) const;

    std::ostream& pragmaForceInfiniteNone( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceInfiniteFixedRate( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceInfiniteMultiServers( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaForceInfiniteAll( std::ostream& output, bool verbose ) const;

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

    std::ostream& pragmaSpexCommentFalse( std::ostream& output, bool verbose ) const;
    std::ostream& pragmaSpexCommentTrue( std::ostream& output, bool verbose ) const;

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
    static const pragma_map_t __pragmas;
    
private:
    static const parameter_map_t  __cycles_args;
    static const parameter_map_t  __force_infinite_args;
    static const parameter_map_t  __force_multiserver_args;
    static const parameter_map_t  __interlock_args;
    static const parameter_map_t  __layering_args;
    static const parameter_map_t  __multiserver_args;
    static const parameter_map_t  __mva_args;
    static const parameter_map_t  __overtaking_args;
    static const parameter_map_t  __processor_args;
    static const parameter_map_t  __prune_args;
#if RESCHEDULE
    static const parameter_map_t  __reschedule_args;
#endif
    static const parameter_map_t  __spex_comment_args;
    static const parameter_map_t  __spex_header_args;
    static const parameter_map_t  __stop_on_message_loss_args;
    static const parameter_map_t  __threads_args;
    static const parameter_map_t  __variance_args;
    static const parameter_map_t  __warning_args;
#if HAVE_LIBGSL && HAVE_LIBGSLCBLAS
    static const parameter_map_t  __quorum_distribution_args;
    static const parameter_map_t  __quorum_delayed_calls_args;
    static const parameter_map_t  __idle_time_args;
#endif
};

class HelpTroff : public Help
{
protected:
    virtual std::ostream& preamble( std::ostream& output ) const;
    virtual std::ostream& see_also( std::ostream& output ) const;
    virtual std::ostream& textbf( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& textit( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& filename( std::ostream& output, const std::string& s1, const std::string& s2 ) const;
    virtual std::ostream& pp( std::ostream& ouptut ) const;
    virtual std::ostream& br( std::ostream& ouptut ) const;
    virtual std::ostream& ol_begin( std::ostream& output ) const;
    virtual std::ostream& ol_end( std::ostream& output ) const;
    virtual std::ostream& dl_begin( std::ostream& output ) const;
    virtual std::ostream& dl_end( std::ostream& output ) const;
    virtual std::ostream& li( std::ostream& output, const std::string& s = 0 ) const;
    virtual std::ostream& section( std::ostream& output, const std::string& s1, const std::string& s2 ) const;
    virtual std::ostream& label( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& flag( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const;
    virtual std::ostream& increase_indent( std::ostream& output ) const;
    virtual std::ostream& decrease_indent( std::ostream& output ) const;
    virtual std::ostream& print_option( std::ostream&, const std::string& name, const Options::Option& opt ) const;
    virtual std::ostream& print_pragma( std::ostream&, const std::string& ) const;
    virtual std::ostream& table_header( std::ostream& ) const;
    virtual std::ostream& table_row( std::ostream&, const std::string&, const std::string&, const std::string& ix = "" ) const;
    virtual std::ostream& table_footer( std::ostream& ) const;

private:
    static const std::string __comment;
};


class HelpLaTeX : public Help
{
protected:
    virtual std::ostream& preamble( std::ostream& output ) const;
    virtual std::ostream& see_also( std::ostream& output ) const { return output; }
    virtual std::ostream& textbf( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& textit( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& tr( std::ostream& output, const std::string& ) const;
    virtual std::ostream& filename( std::ostream& output, const std::string& s1, const std::string& s2 ) const;
    virtual std::ostream& pp( std::ostream& ouptut ) const;
    virtual std::ostream& br( std::ostream& ouptut ) const;
    virtual std::ostream& ol_begin( std::ostream& output ) const;
    virtual std::ostream& ol_end( std::ostream& output ) const;
    virtual std::ostream& dl_begin( std::ostream& output ) const;
    virtual std::ostream& dl_end( std::ostream& output ) const;
    virtual std::ostream& li( std::ostream& output, const std::string& s = 0 ) const;
    virtual std::ostream& section( std::ostream& output, const std::string& s1, const std::string& s2 ) const;
    virtual std::ostream& label( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& flag( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& ix( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& cite( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const;
    virtual std::ostream& increase_indent( std::ostream& output ) const;
    virtual std::ostream& decrease_indent( std::ostream& output ) const;
    virtual std::ostream& print_option( std::ostream&, const std::string& name, const Options::Option& opt) const;
    virtual std::ostream& print_pragma( std::ostream&, const std::string& ) const;
    virtual std::ostream& table_header( std::ostream& ) const;
    virtual std::ostream& table_row( std::ostream&, const std::string&, const std::string&, const std::string& ix = "" ) const;
    virtual std::ostream& table_footer( std::ostream& ) const;
    virtual std::ostream& trailer( std::ostream& output ) const;

private:
    static const std::string __comment;
};

class HelpPlain : public Help
{
protected:
    virtual std::ostream& preamble( std::ostream& output ) const { return output; }
    virtual std::ostream& see_also( std::ostream& output ) const { return output; }
    virtual std::ostream& textbf( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& textit( std::ostream& output, const std::string& s ) const;
    virtual std::ostream& tr( std::ostream& output, const std::string& ) const { return output; }
    virtual std::ostream& filename( std::ostream& output, const std::string& s1, const std::string& s2 ) const;
    virtual std::ostream& pp( std::ostream& output ) const { return output; }
    virtual std::ostream& br( std::ostream& output ) const { return output; }
    virtual std::ostream& ol_begin( std::ostream& output ) const { return output; }
    virtual std::ostream& ol_end( std::ostream& output ) const { return output; }
    virtual std::ostream& dl_begin( std::ostream& output ) const { return output; }
    virtual std::ostream& dl_end( std::ostream& output ) const { return output; }
    virtual std::ostream& li( std::ostream& output, const std::string& s = 0 ) const { return output; }
    virtual std::ostream& section( std::ostream& output, const std::string& s1, const std::string& s2 ) const { return output; }
    virtual std::ostream& label( std::ostream& output, const std::string& s ) const { return output; }
    virtual std::ostream& flag( std::ostream& output, const std::string& s ) const { return output; }
    virtual std::ostream& ix( std::ostream& output, const std::string& s ) const { return output; }
    virtual std::ostream& cite( std::ostream& output, const std::string& s ) const { return output; }
    virtual std::ostream& longopt( std::ostream& output, const struct option *o ) const { return output; }
    virtual std::ostream& increase_indent( std::ostream& output ) const { return output; }
    virtual std::ostream& decrease_indent( std::ostream& output ) const { return output; }
    virtual std::ostream& print_option( std::ostream& output, const std::string& name, const Options::Option& opt) const;
    virtual std::ostream& print_pragma( std::ostream& output, const std::string& ) const;
    virtual std::ostream& table_header( std::ostream& output ) const { return output; }
    virtual std::ostream& table_row( std::ostream& output , const std::string&, const std::string&, const std::string& ix = "" ) const { return output; }
    virtual std::ostream& table_footer( std::ostream& output ) const { return output; }
    virtual std::ostream& trailer( std::ostream& output ) const { return output; }

public:
    static void print_special( std::ostream& output );
    static void print_debug( std::ostream& output );
    static void print_trace( std::ostream& output );
};

inline std::ostream& operator<<( std::ostream& output, const Help& self ) { return self.print( output ); }
#endif
