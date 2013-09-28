# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# $Header$
# $Log$
# Revision 1.2  2003/03/14 02:21:17  greg
# Update Makefile.
#
# Revision 1.1  2003/03/12 02:10:53  greg
# Add makefiles.
#

.SUFFIXES:
.SUFFIXES: .p .in

.in.p:
	$(SRVN) $(OPTS) -p $<

SRC=	A01.in A02.in A03.in A04.in A05.in A06.in A07.in A08.in A09.in A10.in \
	B01.in B02.in B03.in B04.in B05.in B06.in B07.in B08.in B09.in B10.in B11.in

DST=	A01.p  A02.p  A03.p  A04.p  A05.p  A06.p  A07.p  A08.p  A09.p  A10.p  \
	B01.p  B02.p  B03.p  B04.p  B05.p  B06.p  B07.p  B08.p  B09.p  B10.p  B11.p 


exec:	$(DST)

clean:
	rm $(DST)

