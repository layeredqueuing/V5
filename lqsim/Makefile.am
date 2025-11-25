#----------------------------------------------------------------------
# $Id: Makefile.am 17600 2025-11-25 20:26:11Z greg $
#----------------------------------------------------------------------
SUBDIRS		= regression

AM_CPPFLAGS	= -I../lqx/libsrc/headers/ -I../lqiolib/src/headers 
if HAVE_PARASOL
AM_CPPFLAGS	+= -I../parasol/include -DHAVE_PARASOL=1
endif
bin_PROGRAMS	= lqsim
lqsim_SOURCES	= activity.cc actlist.cc entry.cc errmsg.cc eventhandler.cc group.cc histogram.cc instance.cc lqsim.cc \
		  message.cc model.cc pragma.cc processor.cc random.cc result.cc runlqx.cc task.cc target.cc
if HAVE_PARASOL
lqsim_SOURCES	+= 
endif
noinst_HEADERS 	= activity.h actlist.h entry.h errmsg.h eventhandler.h group.h histogram.h instance.h lqsim.h message.h \
		  model.h pragma.h processor.h random.h result.h runlqx.h task.h target.h
if HAVE_PARASOL
else
noinst_HEADERS	+= rendezvous.h
endif
EXTRA_DIST	= lqsim.1

lqsim_CXXFLAGS	= -std=gnu++20 -Wall -Wno-parentheses -Wno-unknown-pragmas -Wno-switch -Wno-overloaded-virtual
lqsim_LDADD	= -L../lqx/libsrc/.libs -llqx -L../lqiolib/src/.libs -llqio
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

