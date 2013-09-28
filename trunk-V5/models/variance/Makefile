# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# $Header$
# $Log$
# Revision 1.3  2003/03/20 02:05:36  greg
# Add multiserver test cases.
#
# Revision 1.2  2003/03/13 16:14:30  greg
# Add new models.
#
# Revision 1.1  2003/03/12 02:10:53  greg
# Add makefiles.
#

.SUFFIXES:
.SUFFIXES: .p .in

.in.p:
		$(SRVN) $(OPTS) -p $<

SRC=		base.in cv=4.in cv=2.in cv=0.5.in cv=0.25.in p=2.in cv=4,p=2.in cv=2,p=2.in cv=0.5,p=2.in cv=0.25,p=2.in
DST=		base.p cv=4.p cv=2.p cv=0.5.p cv=0.25.p p=2.p cv=4,p=2.p cv=2,p=2.p cv=0.5,p=2.p cv=0.25,p=2.p

exec:		$(DST)

clean:
		rm $(DST)

