# Common Makefile stuff
# $HeadURL: svn://franks.dnsalias.com/papers/trunk/mod4sim-11-semaphore/Makefile $
#
# $Id: Makefile 2950 2011-02-14 18:04:52Z greg $
# ------------------------------------------------------------------------

RECURSIVE_TARGETS = all-recursive clean-recursive distclean-recursive
.SUFFIXES:	.eps .sim .exact .new .lqns .none .gnuplot .fig .in .lqn

.fig.eps:
	fig2dev -L eps -m0.7 $< $@

.gnuplot.eps:
	gnuplot $<

.lqn.sim:
	lqsim --no-header -C1.0,1000 -S123456 -xo $@ $<

.lqn.exact:
	petrisrvn --no-header -xo $@ $<

.lqn.none:
	lqns --no-header $(MVA) -Pinterlocking=none -xo $@ $<

.lqn.lqns:
	lqns --no-header $(MVA) -xo $@ $<

.lqn.new:
	$$HOME/usr/src-judy/lqns/lqns --no-header $(MVA) -xo $@ $<

# ------------------------------------------------------------------------

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

