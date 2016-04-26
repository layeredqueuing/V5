/*
 *  $Id: dom_bindings.h 9005 2009-09-30 13:32:13Z greg $
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
  
  /* Registration of the bindings for the DOM results */
  void RegisterBindings(LQX::Environment* env, DOM::Document* document);
  
};

#endif /* __LQIO_DOM_BINDINGS__ */
