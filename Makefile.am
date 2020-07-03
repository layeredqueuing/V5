# -*- Makefile -*-
# ------------------------------------------------------------------------
# $Id: Makefile.am 13447 2020-02-05 22:35:23Z greg $
# ------------------------------------------------------------------------
ACLOCAL_AMFLAGS=-I config
SUBDIRS = lqx lqiolib parasol lqns lqsim srvndiff lqn2ps lqngen doc xml dist
EXTRA_DIST = README NEWS ChangeLog AUTHORS COPYING config/xercesc.m4
RPMDIR = @RPMDIR@

distclean-local:
	rm -f autoconf.cache config.cache

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

