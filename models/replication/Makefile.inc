# -*- makefile -*-
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile.inc 6777 2005-11-17 00:15:41Z greg $
# ----------------------------------------------------------------------

.SUFFIXES:
.SUFFIXES: .p .in

.in.p:
	$(SRVN) $(OPTS) -p $<

SRC=	case1.in \
	case2.in \
	case3.in \
	case4.in \
	case5.in 

DST=	case1.p \
	case2.p \
	case3.p \
	case4.p \
	case5.p

exec:	$(DST)

clean:
	rm -f $(DST)

check:	$(DST)
	for i in $(DST); do cvs diff -I'^#' $$i; done

