# $HeadURL$
# $Id$
# Used for make dist.
# ------------------------------------------------------------------------

FILES = cmpdesign.lqn  database.lqn  dbcasea.lqn  dbcaseb.lqn  dbcase.lqn  ex1-1.lqn  ex1-2.lqn  ex1-3.lqn  pipeline.lqn  test.lqn

EXTRA_DIST = 	$(FILES)

install-data-local:	$(FILES)
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/examples
	for i in $(FILES); do \
	  $(INSTALL_DATA) $$i $(DESTDIR)$(pkgdatadir)/examples/$$i; \
	done
