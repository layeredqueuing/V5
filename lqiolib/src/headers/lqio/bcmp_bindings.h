/* -*- c++ -*-
 *  $Id: bcmp_bindings.h 14402 2021-01-24 04:20:16Z greg $
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

class BCMPModel;

/* LQIO DOM Bindings */
namespace BCMP {

    /* Names of the bindings */
    extern const char * __lqx_residence_time;
    extern const char * __lqx_throughput;
    extern const char * __lqx_utilization;
    extern const char * __lqx_queue_length;

    /* Registration of the bindings for the DOM results */
    void RegisterBindings(LQX::Environment* env, const Model*);
}

#endif /* __BCMP_BINDINGS__ */
