
# @(#)pgfilter.awk	1.2 17:26:48 8/21/95 
#
# File:		pgfilter.awk
# Description:	Awk filter for converting PARASOL trace output into PICL trace
#		 file format suitable for use in ParaGraph.
# Author:	Patrick Morin
# History:	26-05-95 (PRM)	Wrote it.
#
# Notes:	Does not handle shared ports.
#		Doesn't handle timeouts on receives.
#

# Global Variables:
# mm		maps message ids their sending nodes.
# tm		maps tasks to nodes.
# states	The state of each node 0 = executing, 1 = blocked, 2 = idle.
# nodes		The number of nodes in the system.

#
# Initialization routine, called before the first input line is read
#
BEGIN { 
	nodes = 0
	tm[0] = -1	# Grim Reaper Task
}

#
# Handle PARASOL trace output lines
#
$1 == "Time:" {
	node = substr($4,1,length($4)-1)-1
	task = $6+0
	time = substr($2,1,length($2)-1)

	#figure out where the task name ends
	if (index($7,"(") == 1) {
		base = 7
		while (index($(base),")") != length($(base)))
			base++
		base++
	}		
	else
		base = 7

	# Create nodes if necessary
	if (node >= nodes) {
		for (i = nodes; i <= node; i++) 
			print -3,-901,time,i,-1,0 
		nodes = node+1
	}

	# Handle the appropriate input lines
	if ($base == "sending") {
		if (tm[$(base+5)] != -1 && node != -1) { 
			print -3,-21,time,node,-1,3,2,1,$(base+2),tm[$(base+5)]
			print -4,-21,time,node,-1,0
		}
		mm[$(base+2)] = node
	}
	else if ($base == "receiving" && node != -1) {
		print -3,-51,time,node,-1,1,1,2,-1
		states[node] = 1
	}
	else if ($base == "receives" && node != -1) {
		if (states[node] == 1)
			states[node] = 0
		else if (mm[$(base+2)] != -1)
			print -3,-51,time,node,-1,1,1,2,-1
		if (mm[$(base+2)] != -1)
			print -4,-51,time,node,-1,3,2,1,$(base+2),mm[$(base+2)]
	}
	else if ($base == "executing." && node != -1) {
		if (states[node] == 2) {
			states[node] = 0
			print -4,-601,time,node,0,0
		}
		states[node] = 0
	}
	else if ($base == "sleeping." || $base == "suspended." && node != -1) {
		if (states[node] != 2) {
			states[node] = 2
			print -3,-601,time,node,0,0
		}
	}
	else if ($base == "created") {
		tm[task] = node
	}
}

#
# Cleanup routine, cleans up the information for each node.
#
END {
	for (node = 0; node < nodes; node++)
		print -4,-901,time,node,-1,0
}

