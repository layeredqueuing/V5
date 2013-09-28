# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# $Header$
# $Log$
# Revision 1.1  2003/07/10 02:40:15  greg
# Add Dorin's models.
#
#

.SUFFIXES:
.SUFFIXES: .p .in

.in.p:
	$(SRVN) $(OPTS) -p $<

SRC=	b-10.in  b-100.in  b-20.in  b-40.in  b-60.in  b2-10.in  b2-100.in  b2-20.in  b2-40.in  b2-60.in  bstore-mid.in  bstore-pess.in  bstore-slowdb.in  bstore.in  bstore2-mid.in  bstore2-pess.in  bstore2-slowdb.in  bstore2.in  bstorelqns.in  

DST=	b-10.p   b-100.p   b-20.p   b-40.p   b-60.p   b2-10.p   b2-100.p   b2-20.p   b2-40.p   b2-60.p   bstore-mid.p   bstore-pess.p   bstore-slowdb.p   bstore.p   bstore2-mid.p   bstore2-pess.p   bstore2-slowdb.p   bstore2.p   bstorelqns.p   

exec:	$(DST)

clean:
	rm -f $(DST)

