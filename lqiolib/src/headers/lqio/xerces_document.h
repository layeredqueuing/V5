/* -*- C++ -*-
 *  $Id$
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO_XERCES_DOCUMENT__
#define __LQIO_XERCES_DOCUMENT__

#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include "dom_document.h"
#include "dom_phase.h"
#include "confidence_intervals.h"


namespace LQIO {
    class DOMTreeErrorReporter;

    namespace DOM {

	XERCES_CPP_NAMESPACE_USE

	class XercesElementManip {
	public:
	    XercesElementManip( std::ostream& (*f)(std::ostream&, const DOMElement * ), const DOMElement * e ) : _f(f), _e(e) {}
	
	private:
	    std::ostream& (*_f)( std::ostream&, const DOMElement * e );
	    const DOMElement * _e;

	    friend std::ostream& operator<<(std::ostream & os, const XercesElementManip& m ) { return m._f(os,m._e); }
	};

	class Xerces_Document : public Document {
	    friend Document * Document::create( lqio_params_stats*, bool );

	private:
	    struct ltXMLCh {
		bool operator()( const XMLCh * s1, const XMLCh * s2 ) const { return XMLString::compareIStringASCII( s1, s2 ) < 0; }
	    };

	public:
	  static Document * LoadLQNX( const std::string& filename, lqio_params_stats * io_vars, unsigned int & errorCode );		// Factory.

	protected:
	    Xerces_Document( lqio_params_stats* io_vars );

	public:
	    virtual ~Xerces_Document();

	    virtual bool isXMLDOMPresent() const;
	    virtual void serializeDOM( const char * file_name, bool instantiate=false ) const;

	private:
	    Xerces_Document( const Xerces_Document& );
	    Xerces_Document& operator=( const Xerces_Document& );

	private:
	    bool parse( const char *filename );

	    /* Prototypes for internally used functions */

	    void handleModel(const DOMElement *doc);
	    void handleModelParameters(const DOMElement *);
	    void handleProcessor(const DOMElement *);
	    int handleGroup(const DOMElement *procNode, const char *processorName);
	    int handleTask(const DOMElement *curNode, const char *processorName, const char * groupName);
	    void handleFanIn( Task * domTask, const DOMElement * fanin_element );
	    void handleFanOut( Task * domTask, const DOMElement * fanout_element );
	    void handleEntry( const DOMElement * entry_element );
	    void getEntryList(const DOMElement *taskNode, std::vector<DOM::Entry*>& );
	    void handleEntryActivities(DOM::Entry * curEntry, const DOMElement *activities);
	    void handleTaskActivities(DOM::Task * curTask, const DOMElement *activities);
	    void handleActivity(DOM::Task * curTask, const DOMElement *activities);
	    void handlePrecedence(DOM::Task *curTask, const DOMElement *);
	    void handleActivityList( ActivityList * activity_list, Task * domTask, const DOMElement *precedence_element, const ActivityList::ActivityListType type );
	    void handleListEnd( ActivityList * activity_list, Task * domTask, const DOMElement *precedence_element );
	    void handleCalls(const DOMElement *);
	    void handleCallsForEntry(const DOMElement *);
	    void handleForwardingForEntry(const DOMElement *);
	    void handleCallsForActivities(const DOM::Task *, const DOMElement *);
	    void handleCallList(const DOMElement *, const XMLCh *, const XMLCh *, int phase=0 );
	    void handleEntryCall(DOM::Entry * from_entry, int from_phase, const DOM::Call::CallType call_type, const DOMElement * callElement);
	    void handleActivityCall(DOM::Activity * from_activity, const DOM::Call::CallType call_type, const DOMElement * callElement);
	    void handleLQX( const DOMElement * );
	    void handleHistogram( DOM::DocumentObject *, const DOMElement * );
	    void handleHistogramBins( DOM::Histogram * histogram, const DOMNodeList * histogram_bins );

	    void insertDocumentResults() const;
	    void insertProcessorResults( const Processor * ) const;
	    void insertGroupResults( const Group * ) const;
	    void insertTaskResults( const Task * ) const;
	    void insertEntryResults( const Entry * ) const;
	    void insertPhaseResults( const Phase * ) const;
	    void insertActivityResults( const Activity * ) const;
	    void insertCallResults( const Call * ) const;
	    void insertAndJoinActivityListResults( const AndJoinActivityList * ) const;
	    void insertHistogramResults( const Histogram * ) const;

	    bool compareVersionNumber(DOMDocument *doc, const char *attribName, double supportedVersionNum);
	    void initialize_strings();
	    static void delete_strings();
	    static void clearResultList(DOMNodeList *resultList);
	    static void clearExistingDOMResults();   

	    static std::ostream& printStartElement( std::ostream&, const DOMElement * e );
	    static std::ostream& printStartElement2( std::ostream&, const DOMElement * e );
	    static std::ostream& printEndElement( std::ostream&, const DOMElement * e );
	    static std::ostream& printSimpleElement( std::ostream&, const DOMElement * e );

	    static XercesElementManip start_element( const DOMElement * e ) { return XercesElementManip( &printStartElement, e ); }
	    static XercesElementManip start_element2( const DOMElement * e ) { return XercesElementManip( &printStartElement2, e ); }
	    static XercesElementManip end_element( const DOMElement * e ) { return XercesElementManip( &printEndElement, e ); }
	    static XercesElementManip simple_element( const DOMElement * e ) { return XercesElementManip( &printSimpleElement, e ); }
	    

	private:
	    XercesDOMParser *parser;
	    DOMTreeErrorReporter *errReporter;

	    static unsigned __indent;
	    const DOMElement *_root;

        private:
            static XMLCh *XPH1PH2;
            static XMLCh *Xactivity;
            static XMLCh *Xasynch_call;
	    static XMLCh *Xbegin;
            static XMLCh *Xbound_to_entry;
            static XMLCh *Xcall_order;
            static XMLCh *Xcalls_mean;
            static XMLCh *Xcap;
            static XMLCh *Xcomment;
            static XMLCh *Xconf_95;
            static XMLCh *Xconf_99;
            static XMLCh *Xconv_val;
            static XMLCh *Xconv_val_result;
            static XMLCh *Xcore;
            static XMLCh *Xcount;
            static XMLCh *Xdest;
            static XMLCh *Xdeterministic;
            static XMLCh *Xdrop_probability;
            static XMLCh *Xelapsed_time;
            static XMLCh *Xend;
            static XMLCh *Xentry;
            static XMLCh *Xentry_phase_activities;
            static XMLCh *Xfanin;
            static XMLCh *Xfanout;
            static XMLCh *Xfaults;
            static XMLCh *Xforwarding;
            static XMLCh *Xgroup;
            static XMLCh *Xhistogram_bin;
            static XMLCh *Xhost_demand_cvsq;
            static XMLCh *Xhost_demand_mean;
            static XMLCh *Xinitially;
            static XMLCh *Xit_limit;
            static XMLCh *Xiterations;
            static XMLCh *Xjoin_variance;
            static XMLCh *Xjoin_waiting;
            static XMLCh *Xlqn_model;
            static XMLCh *Xlqx;
            static XMLCh *Xmax;
            static XMLCh *Xmax_service_time;
            static XMLCh *Xmin;
            static XMLCh *Xmultiplicity;
            static XMLCh *Xmva_info;
            static XMLCh *Xname;
            static XMLCh *Xnone;
            static XMLCh *Xnumber_bins;
            static XMLCh *Xopen_arrival_rate;
            static XMLCh *Xopen_wait_time;
            static XMLCh *Xoverflow_bin;
            static XMLCh *Xparam;
            static XMLCh *Xphase;
            static XMLCh *Xphase_proc_waiting[DOM::Phase::MAX_PHASE];
            static XMLCh *Xphase_service_time[DOM::Phase::MAX_PHASE];
            static XMLCh *Xphase_service_time_variance[DOM::Phase::MAX_PHASE];
            static XMLCh *Xphase_utilization[DOM::Phase::MAX_PHASE];
            static XMLCh *Xphase_waiting[DOM::Phase::MAX_PHASE];
            static XMLCh *Xphase_waiting_variance[DOM::Phase::MAX_PHASE];
            static XMLCh *Xplatform_info;
            static XMLCh *Xpragma;
            static XMLCh *Xprecedence;
            static XMLCh *Xprint_int;
            static XMLCh *Xpriority;
            static XMLCh *Xprob;
            static XMLCh *Xproc_utilization;
            static XMLCh *Xproc_waiting;
            static XMLCh *Xprocessor;
            static XMLCh *Xquantum;
            static XMLCh *Xqueue_length;
            static XMLCh *Xquorum;
            static XMLCh *Xreplication;
            static XMLCh *Xreply_activity;
            static XMLCh *Xreply_entry;
            static XMLCh *Xresult_activity;
            static XMLCh *Xresult_call;
            static XMLCh *Xresult_conf_95;
            static XMLCh *Xresult_conf_99;
            static XMLCh *Xresult_entry;
            static XMLCh *Xresult_forwarding;
            static XMLCh *Xresult_general;
            static XMLCh *Xresult_group;
            static XMLCh *Xresult_join_delay;
            static XMLCh *Xresult_processor;
            static XMLCh *Xresult_task;
	    static XMLCh *Xr_lock;
	    static XMLCh *Xr_unlock;
	    static XMLCh *Xrwlock;
	    static XMLCh *Xrwlock_reader_waiting;
	    static XMLCh *Xrwlock_reader_waiting_variance;
	    static XMLCh *Xrwlock_reader_holding;
	    static XMLCh *Xrwlock_reader_holding_variance;
	    static XMLCh *Xrwlock_reader_utilization;
	    static XMLCh *Xrwlock_writer_waiting;
	    static XMLCh *Xrwlock_writer_waiting_variance;
	    static XMLCh *Xrwlock_writer_holding;
	    static XMLCh *Xrwlock_writer_holding_variance;
	    static XMLCh *Xrwlock_writer_utilization;
            static XMLCh *Xscheduling;
            static XMLCh *Xsemaphore;
            static XMLCh *Xsemaphore_utilization;
            static XMLCh *Xsemaphore_waiting;
            static XMLCh *Xsemaphore_waiting_variance;
            static XMLCh *Xservice;
            static XMLCh *Xservice_time;
            static XMLCh *Xservice_time_distribution;
            static XMLCh *Xservice_time_variance;
            static XMLCh *Xshare;
            static XMLCh *Xsignal;
            static XMLCh *Xsolver_info;
            static XMLCh *Xsolver_parameters;
	    static XMLCh *Xsource;
            static XMLCh *Xspeed_factor;
            static XMLCh *Xsquared_coeff_variation;
            static XMLCh *Xstep;
            static XMLCh *Xstep_squared;
            static XMLCh *Xsubmodels;
            static XMLCh *Xsynch_call;
            static XMLCh *Xsystem_cpu_time;
            static XMLCh *Xtask;
            static XMLCh *Xtask_activities;
            static XMLCh *Xthink_time;
            static XMLCh *Xthroughput;
            static XMLCh *Xthroughput_bound;
            static XMLCh *Xtype;
            static XMLCh *Xunderflow_bin;
            static XMLCh *Xunderrelax_coeff;
            static XMLCh *Xuser_cpu_time;
            static XMLCh *Xutilization;
            static XMLCh *Xvalid;
            static XMLCh *Xvalue;
            static XMLCh *Xwait;
            static XMLCh *Xwait_squared;
            static XMLCh *Xwaiting;
            static XMLCh *Xwaiting_variance;
	    static XMLCh *Xw_lock;
	    static XMLCh *Xw_unlock;
            static XMLCh *Xxml_debug;

	    static std::map<XMLCh *,ActivityList::ActivityListType,Xerces_Document::ltXMLCh> Xpre_post;
	};

    }
}
#endif /* */
