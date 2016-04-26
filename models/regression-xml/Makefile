# Makefile for Regression Tests
# Copyright (C) 2005 Greg Franks <greg@sce.carleton.ca>
# $Header$
# ------------------------------------------------------------------------
# $Id: Makefile 11963 2014-04-10 14:36:42Z greg $
# ------------------------------------------------------------------------

SUBDIRS = lqns-results  sim-results    
RECURSIVE_TARGETS = all-recursive diff-recursive
DST=	

.PHONY:	.files 

diff:	.files
	srvndiff -IA +t -@.files sim-results lqns-results 

all:	all-recursive 

$(RECURSIVE_TARGETS):
	@set fnord $$MAKEFLAGS; amf=$$2; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"

all-am:

clean:
	-rm -f .files .sync-files .fork-files

.files:	
	@-rm .files
	for i in $(DST); do echo $$i >> .files; done

