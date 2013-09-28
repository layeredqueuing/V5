LQNDIR=$(HOME)/usr/src-V5
CC = gcc
CXX = g++
CFLAGS = -g
CXXFLAGS = -g -DDEBUG_STEP
CPPFLAGS = -DTESTMVA -DHAVE_BOOL -DHAVE_STD=1 -DHAVE_NAMESPACES=1 -I. -I$(LQNDIR)
OBJS = dim.o fpgoop.o multserv.o  mva.o  open.o  ph2serv.o  pop.o  prob.o  server.o vector.o
SRCS = dim.cc fpgoop.cc multserv.cc mva.cc open.cc ph2serv.cc pop.cc prob.cc server.cc vector.cc
INCS = dim.h fpgoop.h multserv.h mva.h open.h ph2serv.h pop.h prob.h server.h vector.h

all:	exact linearizer

exact:	$(INCS) $(SRCS) $(OBJS) exact-1.o
	$(CXX) $(CXXFLAGS) -I. -o exact $(OBJS) exact-1.o -lm

linearizer:	$(INCS) $(SRCS) $(OBJS) linearizer-1.o
	$(CXX) $(CXXFLAGS) -I. -o linearizer $(OBJS) linearizer-1.o -lm

clean:
	rm -f $(OBJS) *~ exact exact-1.o linearizer linearizer-1.o

really-clean:	clean
	for i in $(SRCS) $(INCS); do \
	  if test -L $$i; then \
	    rm $$i; \
	  fi \
	done

$(SRCS):
	ln -s $(LQNDIR)/lqns/$@ .

$(INCS):
	ln -s $(LQNDIR)/lqns/$@ .

dim.cc:	vector.cc vector.h server.h
