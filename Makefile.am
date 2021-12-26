# 
# ------------------------------------------------------------------------
# $Id: Makefile.am 15261 2021-12-25 19:29:06Z greg $
# ------------------------------------------------------------------------
ACLOCAL_AMFLAGS=-I config
SUBDIRS = lqx lqiolib libmva petrisrvn parasol lqns lqsim lqn2csv lqn2ps lqngen qnsolver srvndiff doc xml dist
RPMDIR = @RPMDIR@

distclean-local:
	rm -f autoconf.cache config.cache
if HAVE_SVN
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
