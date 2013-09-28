# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# $Header$
# $Log$
# Revision 1.1  2003/04/02 20:45:37  greg
# Add threads.
#

.SUFFIXES:
.SUFFIXES: .p .in


SRVN=	parasrvn -A1e5,0.5,2e4

.in.p:
	$(SRVN) -p $<


SRC = 	case-01-01-01-01.in case-01-01-01-02.in case-01-01-02-01.in case-01-01-02-02.in \
	case-01-02-01-01.in case-01-02-01-02.in case-01-02-02-01.in case-01-02-02-02.in \
	case-01-03-01-01.in case-01-03-01-02.in case-01-03-02-01.in case-01-03-02-02.in \
	case-02-01-01-01.in case-02-01-01-02.in case-02-01-02-01.in case-02-01-02-02.in \
	case-02-02-01-01.in case-02-02-01-02.in case-02-02-02-01.in case-02-02-02-02.in \
	case-02-03-01-01.in case-02-03-01-02.in case-02-03-02-01.in case-02-03-02-02.in


DST = 	case-01-01-01-01.p  case-01-01-01-02.p  case-01-01-02-01.p  case-01-01-02-02.p  \
	case-01-02-01-01.p  case-01-02-01-02.p  case-01-02-02-01.p  case-01-02-02-02.p  \
	case-01-03-01-01.p  case-01-03-01-02.p  case-01-03-02-01.p  case-01-03-02-02.p  \
	case-02-01-01-01.p  case-02-01-01-02.p  case-02-01-02-01.p  case-02-01-02-02.p  \
	case-02-02-01-01.p  case-02-02-01-02.p  case-02-02-02-01.p  case-02-02-02-02.p  \
	case-02-03-01-01.p  case-02-03-01-02.p  case-02-03-02-01.p  case-02-03-02-02.p 


exec:	$(DST)

clean:
	rm $(DST)


case-01-01-01-02.p:	case-01-01-01-02.in
	petrisrvn -p case-01-01-01-02.in

case-01-01-02-02.p:	case-01-01-02-02.in
	petrisrvn -p case-01-01-02-02.in

case-02-01-01-02.p:	case-02-01-01-02.in
	petrisrvn -p case-02-01-01-02.in

case-02-01-02-02.p:	case-02-01-02-02.in
	petrisrvn -p case-02-01-02-02.in


