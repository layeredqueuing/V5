/*
 * $Id: global.h 10985 2012-06-21 01:00:51Z greg $
 *
 */

#include <stdio.h>

#if !defined(WSPN_GLOBAL_H)
#define WSPN_GLOBAL_H

#if defined(__cplusplus)
extern "C" {
#endif
/***************************  global variables   *************************/
typedef unsigned long LAYER;
#define WHOLE_NET_LAYER ((LAYER)1)

#include "const.h"
#include "object.h"

extern struct net_object *netobj;
extern FILE *nfp;
extern FILE *dfp;
extern char edit_file[LINEMAX];

extern int place_num, trans_num, group_num, cset_num, layer_num;

extern LAYER layers_on;

extern char *layer_name[MAX_LAYER];

extern char *emalloc ( unsigned nbytes );

extern int flag_inv;
extern float fix_x, fix_y, cur_x, cur_y;
extern float prev_x, prev_y;
extern float start_x, start_y;

extern int pointmarker_shown;

extern int cur_orient;
extern int cur_command;
extern int cur_object;

/****************************  Variables for Active Drawing  *************/

extern struct place_object *cur_place;
extern struct trans_object *cur_trans;
extern struct arc_object *cur_arc;

extern struct place_object *last_place;
extern struct trans_object *last_trans;
extern struct arc_object *last_arc;

/********************* Variables for loading/saving the net. **********/

extern struct net_object *head_net;
extern struct net_object *cur_net;
extern char    * net_name;
extern FILE    *fpr;
extern int      net_num;

/**************************  Default Variables for Fixed sized objects  ******/

extern float place_radius;
extern float trans_length;
extern float titrans_height;
extern float imtrans_height;
extern float token_diameter;
extern int arrowht;
extern int arrowwid;
extern float not_radius;

extern char *cant_interrupt;

/****************************  MACROS  ****************************************/

#define encode_layer(N) ((LAYER)( ((LAYER)1) << (N) ))
#define test_layer(N,L) ( (int)((encode_layer(N)) & L) )

#define IN_TO_PIX(x)    ((x) * PIX_PER_INCH )	/* inches to pixels */
#define PIX_TO_IN(x)    (((float)(x)) / PIX_PER_INCH)	/* pixels to inches */

#ifndef MIN
#define MIN(A,B)	(((A) < (B)) ?  (A) :  (B))
#endif
#ifndef	MAX
#define ABS(X)		(((X) >= 0 ) ?  (X) : -(X))
#endif
#define sign(X)		(((X) >= 0)  ?   1  :  -1 )

extern char *Gspn;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#if defined(__cplusplus)
}
#endif
#endif
