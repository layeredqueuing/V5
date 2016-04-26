/*
 *  $Id: wspn.c 11063 2012-07-06 16:15:58Z greg $
 *
 *  This is the main program modeling the decomposition of complex nets.
 *  ctiny
 *
 */

#include <../config.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "global.h"
#include "wspn.h"

struct net_object *head_net;
struct net_object *cur_net;

char            filename[LINEMAX];
char            * net_name;
char            obj_name[LINEMAX];

char            yes;
int             ivalue;
float           fvalue;
int             net_num;
FILE           *fpr;

int
main()
{
	char * place_name;

	printf("\n---Enter the name of the result file.---");
	scanf("\n%s", filename);
	fpr = fopen(filename, "w");

	readin();

#ifdef	WSPN
	printf("---Enter the initial value.\n");
	scanf("%e", &mu);

	netobj = head_net;
	i = 1;
	while (i <= 6) {
		net_name = strdup( netobj->comment->next->line);
		if (strcmp(net_name, "tiny/firstsubnet") == 0)
			change_trate("T14", mu);
		else
			change_trate("T13", mu);

		write_file(net_name);
		put_file();

		solve(net_name);

		if (strcmp(net_name, "tiny/firstsubnet") == 0) {
			fprintf(fpr, "\n %d  First:    %f", i, mu);
			p1 = value_pmmean("P1");
			p13 = value_pmmean("P13");
			mu1 = value_trate("T1");
			mu = (p1 * mu1) / (1 - p13);
		} else {
			fprintf(fpr, "\n %d  Second:   %f", i, mu);
			p2 = value_pmmean("P2");
			p16 = value_pmmean("P16");
			mu2 = value_trate("T2");
			mu = (p2 * mu2) / (1 - p16);
		}
		if (netobj == head_net)
			netobj = head_net->next;
		else
			netobj = head_net;
		i++;
	}
#else
	net_name = strdup( netobj->comment->next->line);
	solve( net_name );
	place_name = "P1";
	printf( "info for %s, pmmean = %f, P(0) = %f, P(1) = %f, P(2) = %f, P(3) = %f, P(4) = %f\n",
	       place_name,
	       value_pmmean( place_name ),
	       value_prob( place_name, 0 ),
	       value_prob( place_name, 1 ),
	       value_prob( place_name, 2 ),
	       value_prob( place_name, 3 ),
	       value_prob( place_name, 4 ) );
	place_name = "P7";
	printf( "info for %s, pmmean = %f, P(0) = %f, P(1) = %f, P(2) = %f, P(3) = %f, P(4) = %f\n",
	       place_name,
	       value_pmmean( place_name ),
	       value_prob( place_name, 0 ),
	       value_prob( place_name, 1 ),
	       value_prob( place_name, 2 ),
	       value_prob( place_name, 3 ),
	       value_prob( place_name, 4 ) );
	place_name = "P11";
	printf( "info for %s, pmmean = %f, P(0) = %f, P(1) = %f, P(2) = %f, P(3) = %f, P(4) = %f\n",
	       place_name,
	       value_pmmean( place_name ),
	       value_prob( place_name, 0 ),
	       value_prob( place_name, 1 ),
	       value_prob( place_name, 2 ),
	       value_prob( place_name, 3 ),
	       value_prob( place_name, 4 ) );
#endif
	(void) fclose(fpr);
	printf("\n\n");
	return 0;
}



/*
 * this function may not last very long...
 */

void
change_net()
{
	float fvalue;
	struct place_object *place;
	struct trans_object *trans;
	char yes, obj_name[40];
	int ivalue;


	printf("\n---Change the marking values?---");
	scanf("%1s", &yes);
	while (yes == 'y') {
		printf("\n---Enter place name and new marking value:---");
		scanf("%s %d", obj_name, &ivalue);

		for (place = netobj->places;
		     (place != NULL && strcmp(obj_name, place->tag) != 0);
		     place = place->next);
		if (place != NULL)
			if (place->mpar != NULL)
				place->mpar->value = ivalue;
			else
				place->tokens = place->m0 = ivalue;
		else
			printf("\n---No place with this name.---\n");

		printf("\n---Still change marking values?---");
		scanf("%1s", &yes);
	}

	printf("\n---Change the transition rates?---");
	scanf("%1s", &yes);
	while (yes == 'y') {
		printf("\n---Enter transition name and new rate:---");
		scanf("%s %f", obj_name, &fvalue);

		for (trans = netobj->trans;
		     (trans != NULL && strcmp(obj_name, trans->tag) != 0);
		     trans = trans->next);
		if (trans != NULL)
			if ((trans->rpar == NULL) && (trans->mark_dep == NULL))
				trans->fire_rate.ff = fvalue;
			else if (trans->mark_dep == NULL)
				trans->rpar->value = fvalue;
			else {
				printf("\n---Marking dependence!---");
				printf("\n---Enter marking dependence expression.---");
				scanf("%s", trans->mark_dep->line);
			}
		else
			printf("\n---No transition with this name.---\n");

		printf("\n---Still change transition rates?---");
		scanf("%1s", &yes);
	}
}

