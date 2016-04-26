/*
 *  $Id: show.c 10972 2012-06-19 01:12:22Z greg $
 *
 *  This file shows the net definitions and results.
 *
 *
 */

#include <stdio.h>
#include <math.h>
#include "global.h"
#include "wspn.h"

void
show_net( const char *nname )
{
	struct place_object *place;
	struct trans_object *trans;
	struct res_object *result;

	char            pname[20];

	printf("\n---The net '%s' has %d places and %d trasitions.---",
	       nname, place_num, trans_num);
	printf("\n---The names and marking values of the places are:---");


	for (place = netobj->places; place != NULL; place = place->next) {
		if (place->mpar != NULL)
			printf("\n---%s  (%s)  %d---",
			       place->tag, place->mpar->tag, place->mpar->value);
		else
			printf("\n---%s  %d---", place->tag, place->m0);
	}

	for (trans = netobj->trans; trans != NULL; trans = trans->next) {
		if ((trans->rpar == NULL) && (trans->mark_dep == NULL))
			printf("\n---%s  %2.4f  ", trans->tag, trans->fire_rate.ff);
		else if (trans->mark_dep == NULL)
			printf("\n---%s  (%s)  %2.4f  ",
			       trans->tag, trans->rpar->tag, trans->rpar->value);
		else
			printf("\n---%s  (%s)",
			       trans->tag, trans->mark_dep->line);

		switch (trans->kind) {
		case 0:
			printf("   exp. transition---");
			break;
		case 1:
			printf("   immed. transition---");
			break;
		case 127:
			printf("   determ. transition---");
		}
	}

	if ((netobj->groups) != NULL) {
		for (trans = netobj->groups->trans;
		     trans != NULL; trans = trans->next) {
			if (trans->rpar != NULL)
				printf("\n---%s  (%s)  %2.4f  ",
				       trans->tag, trans->rpar->tag, trans->rpar->value);
			else
				printf("\n---%s  %2.4f  ", trans->tag, trans->fire_rate.ff);

			switch (trans->kind) {
			case 0:
				printf("   exp. transition---");
				break;
			case 1:
				printf("   immed. transition---");
				break;
			case 127:
				printf("   determ. transition---");
			}
		}
	}
	printf("\n---There are following named results:---");

	for (result = netobj->results; result != NULL; result = result->next)
		printf("\n---%s  (%s)  %2.7f---",
		       result->tag, result->text->line, result->value);


/***    printf("\n---See the marking probabilities of places?---");
    scanf("%1s", &yes);
    while (yes == 'y') {
	printf("\n---Enter place name:---");
	scanf("%s", pname);
	for(place=netobj->places;
	       (place != NULL && strcmp(pname, place->tag)!=0);
	       place = place->next);
        if (place != NULL) {
            printf("\n---%s   has mean marking value %2.7f",
	           place->tag, place->distr->prob);
            for (probab=place->distr->next->next;
	           probab != NULL; probab = probab->next)
                printf("\n---marking value = %d has prob = %2.7f---",
		       probab->val, probab->prob);
        }
        else
            printf("\n---No place with this name.---\n");
        printf("\n---Still see marking probabilities?---");
 	scanf("%1s", &yes);
    }
***/
}
