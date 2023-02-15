# ------------------------------------------------------------------------
# $Id: Makefile.am 16425 2023-02-14 16:53:54Z greg $
# ------------------------------------------------------------------------
ACLOCAL_AMFLAGS=-I config
SUBDIRS = lqx lqiolib libmva parasol lqns lqsim lqn2csv lqn2ps lqngen petrisrvn qnsolver srvndiff doc xml dist
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

dmg:	$(distdir).dmg

$(distdir).dmg:
	$(MAKE) install DESTDIR=/tmp/$(distdir)
	hdiutil create $@ -ov -volname $(distdir) -fs HFS+ -srcfolder "/tmp/$(distdir)/usr/local" -format UDBZ
	-rm -rf /tmp/$(distdir)

winnt-zip:
	zip -uj $(distdir)-winnt.zip ./lqn2csv/lqn2csv.exe ./lqn2ps/lqn2ps.exe ./lqngen/lqngen.exe ./lqns/lqns.exe ./lqsim/lqsim.exe ./qnsolver/qnsolver.exe ./srvndiff/srvndiff.exe ./doc/tutorial/tutorial.pdf ./doc/userman/userman.pdf

zip:
	zip -uj $(distdir).zip ./lqn2csv/lqn2csv ./lqn2ps/lqn2ps ./lqngen/lqngen ./lqns/lqns ./lqsim/lqsim ./qnsolver/qnsolver ./srvndiff/srvndiff ./doc/tutorial/tutorial.pdf ./doc/userman/userman.pdf

