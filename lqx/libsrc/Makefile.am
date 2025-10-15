# $Id: Makefile.am 17007 2024-02-01 17:30:55Z greg $
# Binaries that need to be produced
SUBDIRS = headers/lqx
lib_LTLIBRARIES = liblqx.la
liblqx_la_SOURCES = Array.cpp Environment.cpp Intrinsics.cpp LanguageObject.cpp \
	MethodTable.cpp Parser.cpp Parser_pre.ypp Program.cpp \
	ReferenceCountedObject.cpp RuntimeException.cpp Scanner.cpp \
	Scanner_pre.lpp Strings.cpp SymbolTable.cpp SyntaxTree.cpp
liblqx_la_LDFLAGS = -no-undefined -version-info 0:1:0
liblqx_la_CFLAGS = -Wno-unused-parameter
liblqx_la_CXXFLAGS = -std=gnu++17 -Iheaders/lqx -W -Wall -Wno-sign-compare -Wno-unused -Wno-unknown-pragmas -Wno-unused-parameter -fPIC

Parser_pre.cpp: Parser_pre.ypp 
	../preprocessing/lemon -T../preprocessing/lempar.c Parser_pre.ypp
	mv Parser_pre.c Parser_pre.cpp

Scanner_pre.cpp: Scanner_pre.lpp 
	flex $(YYLINENO) -oScanner_pre.cpp -Pml_scanner_ Scanner_pre.lpp

# Files to remove during cleanup
MOSTLYCLEANFILES = Scanner_pre.cpp Parser_pre.cpp Parser_pre.h Parser_pre.out
