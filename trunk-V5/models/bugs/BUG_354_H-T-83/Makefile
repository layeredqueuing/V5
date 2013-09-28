# Makefile for lqns
# Copyright (C) 2004 Greg Franks <greg@sce.carleton.ca>
#
# $HeadURL: svn://franks.dnsalias.com/papers/trunk/ieeetse-qest/Makefile $
#
# ------------------------------------------------------------------------
# $Id$
# ------------------------------------------------------------------------

SUBDIRS = petri lqns new
RECURSIVE_TARGETS = all-recursive clean-recursive distclean-recursive

DST = 
SRCS =	

OBJS = 

.SUFFIXES:	.pdf .ps .txt .dvi .bbl .aux .eps .fig .tex

all:	all-recursive 

clean: 	clean-recursive

distclean: distclean-recursive

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

all-am:	$(OBJS) 
	-srvndiff -TA +jsb $(SUBDIRS)

clean-am:

distclean-am:

