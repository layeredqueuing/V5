#----------------------------------------------------------------------
# $Id: Makefile.am 15480 2022-03-30 21:44:04Z greg $
#----------------------------------------------------------------------
SUBDIRS		= regression

AM_CPPFLAGS	= -I../lqx/libsrc/headers/ -I../lqiolib/src/headers -I../parasol/include
bin_PROGRAMS	= lqsim
lqsim_SOURCES	= activity.cc actlist.cc entry.cc errmsg.cc group.cc histogram.cc instance.cc model.cc lqsim.cc \
		  message.cc pragma.cc processor.cc result.cc runlqx.cc target.cc task.cc
noinst_HEADERS 	= activity.h actlist.h entry.h errmsg.h group.h histogram.h instance.h model.h lqsim.h \
		  message.h pragma.h processor.h result.h runlqx.h target.h task.h
EXTRA_DIST	= lqsim.1

lqsim_CXXFLAGS	= -std=c++11 -Wall -Wno-parentheses -Wno-unknown-pragmas -Wno-switch -Wno-overloaded-virtual
lqsim_LDADD	= -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio -L../parasol/src -lparasolc
lqsim_LDFLAGS	= @STATIC@
man1_MANS	= lqsim.1

install-exec-hook:

clean-local:

distclean-local:
	rm -f config.cache

if HAVE_PS2PDF
install-data-hook: lqsim.pdf

lqsim.pdf: $(man1_MANS)
	groff -t -man $(man1_MANS) | ps2pdf - $@

endif

lqsim.txt: $(man1_MANS)
	tbl < $(man1_MANS)  | nroff -man | col -b > lqsim.txt

