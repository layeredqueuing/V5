/* -*- c++ -*-
 *  $Id: bcmp_bindings.h 15220 2021-12-15 15:18:47Z greg $
 *
 *  Created by Martin Mroz on 16/04/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __BCMP_BINDINGS__
#define __BCMP_BINDINGS__

/* Forward reference to LQX Environment */
namespace LQX {
    class Environment;
};


/* LQIO DOM Bindings */
namespace BCMP {
    class Model;

    /* Names of the bindings */
    extern const char * __lqx_residence_time;
    extern const char * __lqx_response_time;
    extern const char * __lqx_throughput;
    extern const char * __lqx_utilization;
    extern const char * __lqx_queue_length;

    /* Registration of the bindings for the DOM results */
    void RegisterBindings(LQX::Environment* env, const BCMP::Model*);
}

#endif /* __BCMP_BINDINGS__ */
