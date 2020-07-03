/*
 *  $Id: dom_bindings.h 13499 2020-02-24 01:57:22Z greg $
 *
 *  Created by Martin Mroz on 16/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_BINDINGS__
#define __LQIO_DOM_BINDINGS__

/* Forward reference to LQX Environment */
namespace LQX {
    class Environment;
};

/* LQIO DOM Bindings */
namespace LQIO {
    namespace DOM {
	class Document;
    }

    /* Names of the bindings */
    extern const char * __lqx_elapsed_time;
    extern const char * __lqx_exceeded_time;
    extern const char * __lqx_iterations;
    extern const char * __lqx_pr_exceeded;
    extern const char * __lqx_processor_utilization;
    extern const char * __lqx_processor_waiting;
    extern const char * __lqx_service_time;
    extern const char * __lqx_system_time;
    extern const char * __lqx_throughput;
    extern const char * __lqx_throughput_bound;
    extern const char * __lqx_user_time;
    extern const char * __lqx_utilization;
    extern const char * __lqx_variance;
    extern const char * __lqx_waiting;
    extern const char * __lqx_waiting_variance;

    /* Registration of the bindings for the DOM results */
    void RegisterBindings(LQX::Environment* env, const DOM::Document* document);
}

#endif /* __LQIO_DOM_BINDINGS__ */
