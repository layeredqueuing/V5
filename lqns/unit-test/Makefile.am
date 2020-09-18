# $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/Makefile.am $
#
# Make file for Unit test files
#
# $Id: Makefile.am 13815 2020-09-14 16:30:47Z greg $
# ------------------------------------------------------------------------

AM_CXXFLAGS	= -DTESTMVA -DTESTDIST -DDEBUG_MVA
AM_CPPFLAGS	= -I..
check_PROGRAMS  = test0 test0a \
		  test1 test1hv test1hvp test1p test1ps \
		  test2 test2nc test2p test2pnc \
		  test3 test3c test3e test3ehv test3ehvp \
		  test4 test4a \
		  test5 \
		  test6 \
		  test7 test7e test7e4 test7ek test7nc \
		  test8 test8p \
		  test9 test9a test9b test9c test9d test9i \
		  test10 \
		  test11 test11a test11b test11f \
		  test12 test12s \
		  test13e test13s \
		  testA \
		  testB \
		  poptest \
		  popitertest \
		  disttest

EXTRA_test_SOURCES	= ../dim.cc ../fpgoop.cc ../multserv.cc ../mva.cc ../ph2serv.cc ../open.cc ../pop.cc ../prob.cc ../server.cc ../vector.cc 

test0_SOURCES		= test0.cc testmva.cc $(EXTRA_test_SOURCES)
test0a_SOURCES		= test0a.cc testmixed.cc $(EXTRA_test_SOURCES)
test1_SOURCES		= test1.cc testmva.cc $(EXTRA_test_SOURCES)
test1hv_SOURCES		= test1hv.cc testmva.cc $(EXTRA_test_SOURCES)
test1hvp_SOURCES	= test1hvp.cc testmva.cc $(EXTRA_test_SOURCES)
test1p_SOURCES		= test1p.cc testmva.cc $(EXTRA_test_SOURCES)
test1ps_SOURCES		= test1ps.cc testmva.cc $(EXTRA_test_SOURCES)
test2_SOURCES		= test2.cc testmva.cc $(EXTRA_test_SOURCES)
test2nc_SOURCES		= test2nc.cc testmva.cc $(EXTRA_test_SOURCES)
test2p_SOURCES		= test2p.cc testmva.cc $(EXTRA_test_SOURCES)
test2pnc_SOURCES	= test2pnc.cc testmva.cc $(EXTRA_test_SOURCES)
test3_SOURCES		= test3.cc testmva.cc $(EXTRA_test_SOURCES)
test3c_SOURCES		= test3c.cc testmva.cc $(EXTRA_test_SOURCES)
test3e_SOURCES		= test3e.cc testmva.cc $(EXTRA_test_SOURCES)
test3ehv_SOURCES	= test3ehv.cc testmva.cc $(EXTRA_test_SOURCES)
test3ehvp_SOURCES	= test3ehvp.cc testmva.cc $(EXTRA_test_SOURCES)
test4_SOURCES		= test4.cc testmva.cc $(EXTRA_test_SOURCES)
test4a_SOURCES		= test4a.cc testmva.cc $(EXTRA_test_SOURCES)
test5_SOURCES		= test5.cc testmva.cc $(EXTRA_test_SOURCES)
test6_SOURCES		= test6.cc testmva.cc $(EXTRA_test_SOURCES)
test7_SOURCES		= test7.cc testmva.cc $(EXTRA_test_SOURCES)
test7e_SOURCES		= test7e.cc testmva.cc $(EXTRA_test_SOURCES)
test7e4_SOURCES		= test7e4.cc testmva.cc $(EXTRA_test_SOURCES)
test7ek_SOURCES		= test7ek.cc testmva.cc $(EXTRA_test_SOURCES)
test7nc_SOURCES		= test7nc.cc testmva.cc $(EXTRA_test_SOURCES)
test8_SOURCES		= test8.cc testmva.cc $(EXTRA_test_SOURCES)
test8p_SOURCES		= test8p.cc testmva.cc $(EXTRA_test_SOURCES)
test9_SOURCES		= test9.cc testmva.cc $(EXTRA_test_SOURCES)
test9a_SOURCES		= test9a.cc testmva.cc $(EXTRA_test_SOURCES)
test9b_SOURCES		= test9b.cc testmva.cc $(EXTRA_test_SOURCES)
test9c_SOURCES		= test9c.cc testmva.cc $(EXTRA_test_SOURCES)
test9d_SOURCES		= test9d.cc testmva.cc $(EXTRA_test_SOURCES)
test9i_SOURCES		= test9i.cc $(EXTRA_test_SOURCES)
test10_SOURCES		= test10.cc testmva.cc $(EXTRA_test_SOURCES)
test11_SOURCES		= test11.cc testmva.cc $(EXTRA_test_SOURCES)
test11a_SOURCES		= test11a.cc testmva.cc $(EXTRA_test_SOURCES)
test11b_SOURCES		= test11b.cc testmva.cc $(EXTRA_test_SOURCES)
test11f_SOURCES		= test11f.cc testmva.cc $(EXTRA_test_SOURCES)
test12_SOURCES		= test12.cc testmva.cc $(EXTRA_test_SOURCES)
test12s_SOURCES		= test12s.cc testmva.cc $(EXTRA_test_SOURCES)
test13e_SOURCES		= test13e.cc testmva.cc $(EXTRA_test_SOURCES)
test13s_SOURCES		= test13s.cc testmva.cc $(EXTRA_test_SOURCES)
testA_SOURCES		= testA.cc testopen.cc $(EXTRA_test_SOURCES)
testB_SOURCES		= testB.cc testopen.cc $(EXTRA_test_SOURCES)
poptest_SOURCES		= poptest.cc $(EXTRA_test_SOURCES)
popitertest_SOURCES	= popitertest.cc $(EXTRA_test_SOURCES)
disttest_SOURCES	= disttest.cc ../randomvar.cc $(EXTRA_test_SOURCES)

run:	check
	@for i in $(check_PROGRAMS); do \
	  if test $$i != "poptest" -a $$i != "popitertest" -a $$i != "disttest"; then \
	    echo $$i; \
	    ./$$i -s; \
	  fi \
	done

run-all:	check
	@for i in $(check_PROGRAMS); do \
	  if test $$i != "poptest" -a -a $$i != "popitertest" $$i != "disttest" -a $$i != "test1ps" -a $$i != "test2" -a $$i != "test12s" -a $$i != "testA" -a $$i != "testB"; then \
	    echo $$i; \
	    ./$$i -a -s; \
	  fi \
	done

run-schweitzer:	check
	@for i in $(check_PROGRAMS); do \
	  if test $$i != "poptest" -a $$i != "popitertest" -a $$i != "disttest" -a $$i != "test1ps" -a $$i != "test2" -a $$i != "test12s" -a $$i != "testA" -a $$i != "testB"; then \
	    echo $$i; \
	    ./$$i -b -s; \
	  fi \
	done

