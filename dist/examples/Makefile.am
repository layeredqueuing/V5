# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/dist/examples/Makefile.am $
# $Id: Makefile.am 10904 2012-05-23 17:36:00Z greg $
# Used for make dist.
# ------------------------------------------------------------------------

FILES = cmpdesign.lqn  database.lqn  dbcasea.lqn  dbcaseb.lqn  dbcase.lqn  ex1-1.lqn  ex1-2.lqn  ex1-3.lqn  pipeline.lqn  test.lqn

EXTRA_DIST = 	$(FILES)

install-data-local:	$(FILES)
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/examples
	for i in $(FILES); do \
	  $(INSTALL_DATA) $$i $(DESTDIR)$(pkgdatadir)/examples/$$i; \
	done
