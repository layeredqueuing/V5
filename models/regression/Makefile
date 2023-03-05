# Makefile for Regression Tests
# Copyright (C) 2005 Greg Franks <greg@sce.carleton.ca>
# $Header$
# ------------------------------------------------------------------------
# $Id: Makefile 16469 2023-03-05 14:20:35Z greg $
# ------------------------------------------------------------------------

SUBDIRS = lqns-results  sim-results    
RECURSIVE_TARGETS = all-recursive diff-recursive
DST=	

.PHONY:	.files 

diff:	.files
	srvndiff -IA -S10. -T +tw -@.files petri-results sim-results lqns-results 

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
	-rm -f .files*

.files:	
	@-rm -f .files
	@(cd lqns-results; ls [0-9][0-9]-*.lqxo) > .files-1
	@(cd sim-results; ls [0-9][0-9]-*.lqxo) > .files-2
	@comm -12 .files-1 .files-2 > .files-3
	@(cd petri-results; ls [0-9][0-9]-*.lqxo) > .files-2
	@comm -12 .files-2 .files-3 > .files
	@rm .files-1 .files-2 .files-3
