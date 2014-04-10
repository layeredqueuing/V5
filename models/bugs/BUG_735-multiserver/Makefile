LQNDIR=../../..
CC = gcc
CXX = g++
CFLAGS = -g -DDEBUG_MVA
CXXFLAGS = $(CFLAGS)
CPPFLAGS = -DTESTMVA -DHAVE_BOOL -DHAVE_STD=1 -DHAVE_NAMESPACES=1 -I. -I$(LQNDIR)
OBJS = dim.o fpgoop.o multserv.o  mva.o  open.o  ph2serv.o  pop.o  prob.o  server.o vector.o
SRCS = dim.cc fpgoop.cc multserv.cc mva.cc open.cc ph2serv.cc pop.cc prob.cc server.cc vector.cc
INCS = dim.h fpgoop.h multserv.h mva.h open.h ph2serv.h pop.h prob.h server.h vector.h

all:	conway reiser fixed test-step 

conway:	$(INCS) $(SRCS) $(OBJS) conway.o
	$(CXX) $(CXXFLAGS) -I. -o conway $(OBJS) conway.o -lm

reiser:	conway
	ln -s conway reiser

fixed:	$(INCS) $(SRCS) $(OBJS) fixed.o
	$(CXX) $(CXXFLAGS) -I. -o fixed $(OBJS) fixed.o -lm

test-step:	$(INCS) $(SRCS) $(OBJS) test-step.o
	$(CXX) $(CXXFLAGS) -I. -o test-step $(OBJS) test-step.o -lm

clean:
	rm -f $(OBJS) *~ reiser reiser.o conway conway.o test-step test-step.o

distclean:	clean
	for i in $(SRCS) $(INCS); do \
	  if test -L $$i; then \
	    rm $$i; \
	  fi \
	done

$(SRCS):
	@if test ! -f $@; then ln -s $(LQNDIR)/lqns/$@ .; fi

$(INCS):
	@if test ! -f $@; then ln -s $(LQNDIR)/lqns/$@ .; fi

dim.cc:	vector.cc vector.h server.h
