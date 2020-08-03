/* -*- c++ -*-
 *  $Id: submodel_info.h 13717 2020-08-03 00:04:28Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __LQIO_DOM_MVA_STATISTICS__
#define __LQIO_DOM_MVA_STATISTICS__

namespace LQIO {
    namespace DOM {

	class MVAStatistics {
	public:
	MVAStatistics() : _submodels(0), _core(0), _step(0.0), _step_squared(0.0), _wait(0.0), _wait_squared(0.0), _faults(0) {}

	    void set( const unsigned int submodels,
		      const unsigned long core,
		      const double step,
		      const double step_squared,
		      const double wait,
		      const double wait_squared,
		      const unsigned int faults ) {
		_submodels = submodels;
		_core = core;
		_step = step;
		_step_squared = step_squared;
		_wait = wait;
		_wait_squared = wait_squared;
		_faults = faults;
	    }
		
	    unsigned getNumberOfSubmodels() const { return _submodels; }
	    unsigned long getNumberOfCore() const { return _core; }
	    double getNumberOfStep() const { return _step; }
	    double getNumberOfStepSquared() const { return _step_squared; }
	    double getNumberOfWait() const { return _wait; }
	    double getNumberOfWaitSquared() const { return _wait_squared; }
	    unsigned int getNumberOfFaults() const { return _faults; }
	    
	private:
	    unsigned int _submodels;
	    unsigned long _core;
	    double _step;
	    double _step_squared;
	    double _wait;
	    double _wait_squared;
	    unsigned int _faults;
	};
    }
}
#endif
