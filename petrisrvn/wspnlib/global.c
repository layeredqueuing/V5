/*
 *  $Id: global.c 10972 2012-06-19 01:12:22Z greg $
 *
 */

#include <../config.h>
#include <stdio.h>
#include <errno.h>


#define WHOLE_NET_LAYER ((LAYER)1)

#include "const.h"
#include "object.h"
#include "global.h"

/***************************  global variables   *************************/
struct net_object *netobj = NULL;

FILE    *nfp;
FILE    *dfp;
char    edit_file[LINEMAX];

int     place_num, trans_num, group_num, cset_num, layer_num;

LAYER   layers_on;

char    *layer_name[MAX_LAYER];


/**************************  Default Variables for Fixed sized objects  ******/

float	place_radius = 7;
float	trans_length = 15;
float	titrans_height = 7;
float	imtrans_height = 3;
float	token_diameter = 3;
int	arrowht = 6;
int	arrowwid = 4;
float	not_radius = 2;

