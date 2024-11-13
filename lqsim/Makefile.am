#----------------------------------------------------------------------
# $Id: Makefile.am 17467 2024-11-13 14:57:26Z greg $
#----------------------------------------------------------------------
SUBDIRS		= regression

AM_CPPFLAGS	= -I../lqx/libsrc/headers/ -I../lqiolib/src/headers -I../parasol/include
bin_PROGRAMS	= lqsim
lqsim_SOURCES	= activity.cc actlist.cc entry.cc errmsg.cc group.cc histogram.cc instance.cc model.cc lqsim.cc \
		  message.cc pragma.cc processor.cc random.cc result.cc runlqx.cc target.cc task.cc
noinst_HEADERS 	= activity.h actlist.h entry.h errmsg.h group.h histogram.h instance.h model.h lqsim.h \
		  message.h pragma.h processor.h random.h result.h runlqx.h target.h task.h
EXTRA_DIST	= lqsim.1

lqsim_CXXFLAGS	= -std=c++17 -Wall -Wno-parentheses -Wno-unknown-pragmas -Wno-switch -Wno-overloaded-virtual
lqsim_LDADD	= -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio
lqsim_LDADD	+= -L../parasol/src -lparasolc
if HAVE_PARASOL
lqsim_LDADD	+= -L../parasol/src -lparasolc
endif
lqsim_LDFLAGS	= @STATIC@
man1_MANS	= lqsim.1

install-exec-hook:

clean-local:

distclean-local:
	rm -f config.cache

if HAVE_PS2PDF
all-local:	lqsim.pdf

install-data-hook: lqsim.pdf

lqsim.pdf: $(man1_MANS)
	groff -t -man $(man1_MANS) > $<.ps
	ps2pdf $<.ps $@
	rm $<.ps

endif

lqsim.txt: $(man1_MANS)
	tbl < $(man1_MANS)  | nroff -man | col -b > lqsim.txt

