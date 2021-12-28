# ------------------------------------------------------------------------
# $Id: Makefile.am 15281 2021-12-28 13:29:59Z greg $
# ------------------------------------------------------------------------
ACLOCAL_AMFLAGS=-I config
SUBDIRS = lqx lqiolib libmva petrisrvn parasol lqns lqsim lqn2csv lqn2ps lqngen qnsolver srvndiff doc xml dist
RPMDIR = @RPMDIR@
EXTRA_DIST = README NEWS ChangeLog AUTHORS COPYING 

distclean-local: svnclean
	rm -f autoconf.cache config.cache

if HAVE_SVN
svnclean:
	svn stat | grep '^?' | sed -e s'/^?       //' | xargs rm -rf
endif

rpm: $(RPMDIR)/SOURCES/$(distdir).tar.gz dist/linux/lqns.spec
	if test -d /usr/local/man; then \
	  sed -e "s/product_version VERSION/product_version $(VERSION)/" -e 's:/share/man:/man:' < dist/linux/lqns.spec > $(RPMDIR)/SPECS/lqns.spec; \
	else \
	  sed -e "s/product_version VERSION/product_version $(VERSION)/" < dist/linux/lqns.spec > $(RPMDIR)/SPECS/lqns.spec; \
	fi
	rpmbuild -bb $(RPMDIR)/SPECS/lqns.spec

$(RPMDIR)/SOURCES/$(PACKAGE)-$(VERSION).tar.gz: $(distdir).tar.gz
	cp $(distdir).tar.gz $(RPMDIR)/SOURCES/$(distdir).tar.gz 

$(distdir).tar.gz:
	$(MAKE) dist
