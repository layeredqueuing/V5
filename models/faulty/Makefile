# Makefile for lqns regression tests
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 15844 2022-08-17 13:48:59Z greg $
# ------------------------------------------------------------------------

SRCS=	err_activity_not_reachable.lqn \
	err_arrival_rate.lqn \
	err_cycle_in_activity_graph.lqn \
	err_cycle_in_call_graph.lqn \
	err_duplicate_activity_lvalue.lqn \
	err_duplicate_activity_rvalue.lqn \
	err_duplicate_reply.lqn \
	err_duplicate_reply.lqnx \
	err_duplicate_start_activity.lqn \
	err_duplicate_symbol-entry.lqn \
	err_duplicate_symbol-processor.lqn \
	err_duplicate_symbol-task.lqn \
	err_entry_count_for_task.lqn \
	err_entry_not_specified.lqn \
	err_external_sync-1.lqn \
	err_external_sync-2.lqn \
	err_external_sync-3.lqn \
	err_fork_join_mismatch.lqn \
	err_invalid_forwarding_probability.lqn \
	err_invalid_from_branch.lqnx \
	err_invalid_or_branch_probability.lqn \
	err_invalid_or_branch_probability.lqnx \
	err_invalid_parameter-calls.lqn \
	err_invalid_parameter-proc.lqnx \
	err_invalid_reply_duplicate.lqn \
	err_invalid_reply_for_snr_entry.lqn \
	err_invalid_reply_from_branch-1.lqn \
	err_invalid_reply_from_branch-2.lqn \
	err_is_start_activity-2.lqn \
	err_is_start_activity.lqn \
	err_join_bad_path-repeat.lqn \
	err_join_bad_path.lqn \
	err_mixed_entry_types.lqn \
	err_mixed_entry_types.lqnx \
	err_no_group_specified.lqn \
	err_no_reference_tasks.lqn \
	err_no_requests_to_entry-1.lqn \
	err_no_start_activities.lqn \
	err_non_ref_think_time.lqn \
	err_non_unity_replies-and.lqn \
	err_non_unity_replies-or.lqn \
	err_not_defined-activity.lqn \
	err_not_defined-activity.lqnx \
	err_open_and_closed_classes.lqn \
	err_or_branch_probabilities-2.lqn \
	err_or_branch_probabilities.lqn \
	err_ref_task_forwarding.lqn \
	err_reference_task_is_infinite.lqn \
	err_reference_task_is_receiver.lqn \
	err_reference_task_is_receiver.lqnx \
	err_reference_task_open_arrivals.lqn \
	err_reference_task_replies.lqn \
	err_replication.lqn \
	err_replication_processor.lqn \
	err_reply_not_generated.lqn \
	err_src_equals_dst.lqn \
	err_src_equals_dst.lqnx \
	err_task_has_no_entries.lqnx \
	err_wrong_task_for_entry.lqn \
	wrn_entry_has_no_requests.lqn \
	wrn_infinite_multi_server.lqn \
	wrn_scheduling_not_supported.lqnx

lqns:	
	for i in $(SRCS); do (lqns $$i > /dev/null 2>&1); j=$$?; if test $$j -eq 0; then echo $$i failed; elif test $$j -ge 128; then echo $$i faulted; else echo $$i passed; fi; done

lqns-verbose:
	for i in $(SRCS); do (lqns $$i > /dev/null); j=$$?; if test $$j -eq 0; then echo $$i failed; elif test $$j -ge 128; then echo $$i faulted; else echo $$i passed; fi; done

lqsim:	
	for i in $(SRCS); do (lqsim $$i > /dev/null 2>&1); j=$$?; if test $$j -eq 0; then echo $$i failed; elif test $$j -ge 128; then echo $$i faulted; else echo $$i passed; fi; done
#	for i in $(SRCS); do (lqsim $$i > /dev/null 2>&1); echo $$?; done


petrisrvn:
	for i in $(SRCS); do (petrisrvn $$i > /dev/null 2>&1); j=$$?; if test $$j -eq 0; then echo $$i failed; elif test $$j -ge 128; then echo $$i faulted; else echo $$i passed; fi; done

lqn2ps:
	for i in $(SRCS); do (lqn2ps -Onull -o/dev/null $$i > /dev/null 2>&1); j=$$?; if test $$j -eq 0; then echo $$i failed; elif test $$j -ge 128; then echo $$i faulted; else echo $$i passed; fi; done

clean:
	rm -rf *.out *.lqxo *.ps *.d nets empty

missing:
	ls err_* | sed -e 's/\..*//' -e 's/-.*//' | sort | uniq > tests
	grep ERR_ ../../lqiolib/src/headers/lqio/glblerr.h | sed -e 's/[ 	]*//g' -e 's/,$$//' -e '/^#/d' -e '/LSTG/d' | tr "[:upper:]" "[:lower:]" | sort > errors
	comm -i -1 -3 tests errors

obsolete:
	grep ERR_ ../../lqiolib/src/headers/lqio/glblerr.h | sed -e 's/[ 	]*//g' -e 's/,$$//' -e '/^#/d' -e '/LSTG/d' | sort > errors
	for i in `cat errors`; do \
	  count=`find ../.. \( -name '*.cc' -o -name '*.cpp' -o -name '*.y' \) -exec grep -nH -e "$i" \{\} + | grep -v 'glblerr.cpp' | wc -l`; \
	  if test $$count -eq 0; then \
	    echo $$i $$count; \
	  fi; \
	done
