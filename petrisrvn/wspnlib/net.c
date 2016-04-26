/*
 * $Id: net.c 11063 2012-07-06 16:15:58Z greg $
 */

#include <../config.h>
#include <stdlib.h>
#include "global.h"
#include "wspn.h"

int
init_net()
{
	int i;

	for (i = 0; i++ < layer_num;)
		free(layer_name[i]);
	place_num = 0;
	trans_num = 0;
	group_num = 0;
	layer_num = 0;
	layers_on = WHOLE_NET_LAYER;
	netobj = create_net();
	return (1);
}

struct net_object *
create_net()
{
	struct net_object *net;

	net = (struct net_object *) emalloc(NETOBJ_SIZE);
	reset_net_object(net);
	return (net);
}

void
reset_net_object(struct net_object *net)
{
	net->comment = NULL;
	net->mpars = NULL;
	net->places = NULL;
	net->rpars = NULL;
	net->trans = NULL;
	net->groups = NULL;
	net->arcs = NULL;
	net->results = NULL;
	net->lisps = NULL;
	net->texts = NULL;
/*	net->boxht = titrans_height;
	net->boxwid = trans_length;
	net->circlerad = place_radius; */
	net->nwcorner.x = 0.0;
	net->nwcorner.y = 0.0;
/*	net->secorner.x = canvas_width;
	net->secorner.y = canvas_height; */
	net->next = NULL;
}

void
free_netobj(struct net_object *net)
{
	purge_netobj(net);
	free((char *) net);
}


void
purge_netobj(struct net_object *net)
{
	struct com_object *com, *com_tmp;
	struct mpar_object *mpar, *mpar_tmp;
	struct probability *prob, *prob_tmp;
	struct place_object *place, *place_tmp;
	struct rpar_object *rpar, *rpar_tmp;
	struct trans_object *trans, *trans_tmp;
	struct group_object *group, *group_tmp;
	struct arc_object *arc, *arc_tmp;
	struct res_object *res, *res_tmp;
	struct lisp_object *lisp, *lisp_tmp;
	struct coordinate *cur_point, *last_point;
	char *cp;

	com = net->comment;
	while (com != NULL) {
		com_tmp = com->next;
		if ((cp = com->line) != NULL)
			free((char *) cp);
		free((char *) com);
		com = com_tmp;
	}
	mpar = net->mpars;
	while (mpar != NULL) {
		mpar_tmp = mpar->next;
		if ((cp = mpar->tag) != NULL)
			free((char *) cp);
		free((char *) mpar);
		mpar = mpar_tmp;
	}
	place = net->places;
	while (place != NULL) {
		place_tmp = place->next;
		if ((cp = place->tag) != NULL)
			free((char *) cp);
		if ((cp = place->color) != NULL)
			free((char *) cp);
		for (prob = place->distr, place->distr = NULL; prob != NULL;
		     prob = prob_tmp) {
			prob_tmp = prob->next;
			free((char *) prob);
		}
		free((char *) place);
		place = place_tmp;
	}
	rpar = net->rpars;
	while (rpar != NULL) {
		rpar_tmp = rpar->next;
		if ((cp = rpar->tag) != NULL)
			free((char *) cp);
		free((char *) rpar);
		rpar = rpar_tmp;
	}
	trans = net->trans;
	while (trans != NULL) {
		trans_tmp = trans->next;
		if ((cp = trans->tag) != NULL)
			free((char *) cp);
		if ((cp = trans->color) != NULL)
			free((char *) cp);
		for (com = trans->mark_dep; com != NULL; com = com_tmp) {
			com_tmp = com->next;
			if (com->line != NULL)
				free((char *) (com->line));
			free((char *) com);
		}
		if (trans->enabl < 0)
			free((char *) (trans->fire_rate.fp));
		free((char *) trans);
		trans = trans_tmp;
	}
	group = net->groups;
	while (group != NULL) {
		group_tmp = group->next;
		trans = group->trans;
		while (trans != NULL) {
			trans_tmp = trans->next;
			if ((cp = trans->tag) != NULL)
				free((char *) cp);
			if ((cp = trans->color) != NULL)
				free((char *) cp);
			for (com = trans->mark_dep; com != NULL; com = com_tmp) {
				com_tmp = com->next;
				if (com->line != NULL)
					free((char *) (com->line));
				free((char *) com);
			}
			free((char *) trans);
			trans = trans_tmp;
		}
		if ((cp = group->tag) != NULL)
			free((char *) cp);
		free((char *) group);
		group = group_tmp;
	}
	arc = net->arcs;
	while (arc != NULL) {
		if ((cp = arc->color) != NULL)
			free((char *) cp);
		arc_tmp = arc->next;
		last_point = arc->point;
		while (last_point != NULL) {
			cur_point = last_point->next;
			free((char *) last_point);
			last_point = cur_point;
		}
		free((char *) arc);
		arc = arc_tmp;
	}
	res = net->results;
	while (res != NULL) {
		res_tmp = res->next;
		if ((cp = res->tag) != NULL)
			free((char *) cp);
		for (com = res->text; com != NULL; com = com_tmp) {
			com_tmp = com->next;
			if ((cp = com->line) != NULL)
				free((char *) cp);
			free((char *) com);
		}
		free((char *) res);
		res = res_tmp;
	}
	lisp = net->lisps;
	while (lisp != NULL) {
		lisp_tmp = lisp->next;
		if ((cp = lisp->tag) != NULL)
			free((char *) cp);
		for (com = lisp->text; com != NULL; com = com_tmp) {
			com_tmp = com->next;
			if ((cp = com->line) != NULL)
				free((char *) cp);
			free((char *) com);
		}
		free((char *) lisp);
		lisp = lisp_tmp;
	}
}
