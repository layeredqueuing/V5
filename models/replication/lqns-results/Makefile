# Makefile for 
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
#
# ------------------------------------------------------------------------
# $Id: Makefile 6777 2005-11-17 00:15:41Z greg $
# ------------------------------------------------------------------------
#

SRVN=	lqns 
OPTS=	-M

include ../Makefile.inc

diff:	$(DST)
	srvndiff -atask='A[0-9]*' -atask='B[0-9]*' -atask='C[0-9]*' -atask='D[0-9]*' -aentry='A[0-9]*' -aentry='B[0-9]*' -aentry='C[0-9]*' -aentry='D[0-9]*'  -aprocessor='A[0-9]*' -aprocessor='B[0-9]*' -aprocessor='C[0-9]*' -aprocessor='D[0-9]*' ../sim-results ../full-results ../lqns-results
