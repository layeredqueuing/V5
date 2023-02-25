# $Id: Makefile.am 16429 2023-02-15 16:41:59Z greg $
# Binaries that need to be produced
SUBDIRS = headers/lqx
lib_LTLIBRARIES = liblqx.la
liblqx_la_SOURCES = Array.cpp Environment.cpp Intrinsics.cpp LanguageObject.cpp \
	MethodTable.cpp NonCopyable.cpp Parser.cpp Parser_pre.ypp Program.cpp \
	ReferenceCountedObject.cpp RuntimeException.cpp Scanner.cpp \
	Scanner_pre.lpp Strings.cpp SymbolTable.cpp SyntaxTree.cpp
liblqx_la_LDFLAGS = -no-undefined -version-info 0:1:0
liblqx_la_CFLAGS = -Wno-unused-parameter
liblqx_la_CXXFLAGS = -std=c++11 -Iheaders/lqx -W -Wall -Wno-sign-compare -Wno-unused -Wno-unknown-pragmas -Wno-unused-parameter -fPIC

Parser_pre.cpp: Parser_pre.ypp 
	../preprocessing/lemon Parser_pre.ypp
	mv Parser_pre.c Parser_pre.cpp

Scanner_pre.cpp: Scanner_pre.lpp 
	flex $(YYLINENO) -oScanner_pre.cpp -Pml_scanner_ Scanner_pre.lpp

# Files to remove during cleanup
MOSTLYCLEANFILES = Scanner_pre.cpp Parser_pre.cpp Parser_pre.h Parser_pre.out
