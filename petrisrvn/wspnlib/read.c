/*
 *  $Id: read.c 10972 2012-06-19 01:12:22Z greg $
 *
 *  This file reads in the net definitions and shows them.
 *
 */

#include <../config.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "global.h"
#include "wspn.h"

void
readin()
{
	int             i;

	layer_name[0] = "the_whole_net";
	layer_num = 0;

	if (init_net() == 0)
		exit(1);
	head_net = netobj;
	cur_net = netobj;

	printf("\n---Enter the number of nets:---");
	scanf("%d", &net_num);
	fprintf(fpr, "The number of nets load is  %d.\n", net_num);
	for (i = 1; i <= net_num; i++) {
		printf("\n---Enter netname:---");
		scanf("%s", net_name);
		read_file(net_name);
		fprintf(fpr, "The name of the %d. net is: %s.\n", i, net_name);

		show_net(net_name);
		change_net();
		write_file(net_name);
		put_file();
		if (i != net_num) {
			if (init_net() == 0)
				exit(1);
			cur_net->next = netobj;
			cur_net = netobj;
		}
	}
}
