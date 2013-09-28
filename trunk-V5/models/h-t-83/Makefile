# Makefile for
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
#
# $HeadURL$
#
# ------------------------------------------------------------------------
# $Id$
# ------------------------------------------------------------------------


SUBDIRS=	table1 table2

RECURSIVE_TARGETS = all-recursive clean-recursive lqns-recursive new-recursive noml-recursive sim-recursive

SRCS = 	

OBJS = $(SRCS:.fig=.eps)

all:	all-recursive 

lqns:	lqns-recursive

new:	new-recursive

noml:	noml-recursive

sim:	sim-recursive

clean: 	clean-recursive

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

lqns-am:

noml-am:

new-am:

sim-am:

clean-am:


