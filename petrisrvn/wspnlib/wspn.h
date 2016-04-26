/*
 * $Id: wspn.h 10985 2012-06-21 01:00:51Z greg $
 */

#if !defined(WSPNLIB_WSPN_H)
#define WSPNLIB_WSPN_H

extern int wspn_report_errors;
extern int wspn_accept_input;	/* This may go... */

typedef enum { SOLVE_STEADY_STATE, SOLVE_TRANSIENT } solution_type;

#if defined(__cplusplus)
extern "C" {
#endif

/* change.c */
int change_trate (const char *obj_name, float fvalue);
int change_itrate (const char *obj_name, float fvalue);
int change_pmar (const char *obj_name, int ivalue);
int change_res (const char * obj_name, float fvalue);
int change_rpar (const char * obj_name, float fvalue);

/* value.c */
double value_pmmean (const char *obj_name);
double value_pmvariance (const char *obj_name);
double value_prob (const char *obj_name, int m);
double value_trate (const char *obj_name);
double value_itrate (const char *obj_name);
int value_mpar (const char *obj_name);
int value_pmar (const char *obj_name);
double value_rpar (const char *obj_name);
double value_res (char obj_name[]);
double value_tput (const char *obj_name);
double value_itput (const char *obj_name);
struct place_object * find_place ( const char * obj_name );
struct trans_object * find_trans ( const char * obj_name );
struct trans_object * find_itrans ( const char * obj_name );
struct res_object * find_result ( const char * obj_name );
struct rpar_object * find_rpar ( const char * obj_name );
int solution_stats (int *tangible, int *vanishing, double *precision);

/* solve.c */
void solve (const char *net_name);
int solve2 (const char *net_name, int output, solution_type solver,... );
int remove_result_files (const char *net_name);

/* net.c */
int init_net (void);
struct net_object *create_net (void);
void reset_net_object (struct net_object *net);
void free_netobj (struct net_object *net);
void purge_netobj (struct net_object *net);

/* netdir.c */
void make_net_dir (const char *dirname);

/* save.c */
void save_net_files (const char *toolname, const char *netname);
void write_file(const char *netname);

/* load.c */

int read_file ( const char *netname );

/* res.c */

int collect_res ( int complain, const char * toolname );

/* read.c */

void readin ( void );


/* show.c */

void show_net ( const char *nname );
#if defined(__cplusplus)
}
#endif
#endif
