# Makefile for lqns regression tests
# Copyright (C) 2003 Greg Franks <greg@sce.carleton.ca>
# ------------------------------------------------------------------------
# $Id: Makefile 14979 2021-09-14 23:03:15Z greg $
# ------------------------------------------------------------------------

SRCS=	err_activity_not_specified.in \
	err_cycle_in_activity_graph.in \
	err_cycle_in_call_graph.in \
	err_duplicate_activity_lvalue.in \
	err_duplicate_activity_rvalue.in \
	err_duplicate_entry-1.in \
	err_duplicate_entry-2.in \
	err_duplicate_entry-3.in \
	err_duplicate_reply.in \
	err_duplicate_start_activity.in \
	err_entry_not_specified.in \
	err_external_sync.in \
	err_infinite_ref_task.in \
	err_invalid_forwarding_probability.in \
	err_invalid_probability.in \
	err_invalid_proc_rate.in \
	err_invalid_reply-1.in \
	err_is_start_activity.in \
	err_join_bad_path-2.in \
	err_join_bad_path.in \
	err_join_path_mismatch.in \
	err_join_repeat.in \
	err_missing_or_branch.in \
	err_mixed_entry_types.in \
	err_no_group_for_cfs.in \
	err_no_reference_tasks.in \
	err_no_start_activities.in \
	err_non_integral_calls.in \
	err_non_ref_think_time.in \
	err_non_unity_replies-1.in \
	err_non_unity_replies-2.in \
	err_open_and_closed_classes.in \
	err_overloaded_open.in \
	err_ref_task_forwarding.in \
	err_reference_task_is_receiver.in \
	err_reference_task_open_arrivals.in \
	err_reference_task_replies.in \
	err_replication_task_proc.in \
	err_replication_task_task-flat.in \
	err_replication_task_task.in \
	err_reply_not_generated-2.in \
	err_src_equals_dst.in \
	err_wrong_task_for_entry.in

lqns:	
#	for i in $(SRCS); do (lqns $$i > /dev/null 2>&1); if test $$? -eq 0; then echo "$$i failed"; fi; done
	for i in $(SRCS); do (lqns $$i > /dev/null 2>&1); j=$$?; if test $$j -eq 0; then echo $$i failed; elif test $$j -ge 128; then echo $$i faulted; else echo $$i passed; fi; done

lqsim:	
	for i in $(SRCS); do (lqsim $$i > /dev/null 2>&1); j=$$?; if test $$j -eq 0; then echo $$i failed; elif test $$j -eq 134; then echo $$i faulted; else echo $$i passed; fi; done
#	for i in $(SRCS); do (lqsim $$i > /dev/null 2>&1); echo $$?; done
