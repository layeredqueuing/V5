/*
 * $Id: makeobj.h 13477 2020-02-08 23:14:37Z greg $
 *
 * Stochastic RendezVous Network header
 */

#if !defined(PETRISRVN_MAKEOBJ_H)
#define PETRISRVN_MAKEOBJ_H

#include <string>
#include <map>
#include <wspnlib/object.h>

void create_arc(LAYER layer, int type, struct trans_object *transition, struct place_object *place);
void create_arc_mult(LAYER layer, int type, struct trans_object *transition, struct place_object *place, short mult );
void groupize(void);
void free_group_store(void);
unsigned no_rpar(const char *format, ...);
void shift_rpars( double offset_x, double offset_y );
struct trans_object * no_trans( const char * format, ... );
struct trans_object * create_trans( double x_pos, double y_pos, LAYER layer, double rate, short enable, short kind, const char * format, ... );
struct trans_object * move_trans( struct trans_object * cur_trans, double x_offset, double y_offset );
struct trans_object * move_trans_tag( struct trans_object * cur_trans, double x_offset, double y_offset );
struct trans_object * orient_trans( struct trans_object * trans, short orientation );
short create_rpar( double curr_x, double curr_y, LAYER layer, double rate, const char * format, ... );
struct res_object * create_res(double curr_x, double curr_y, const char *format_object, const char *format_name, ... );
struct place_object * no_place( const char *format, ...);
struct place_object * create_place(double curr_x, double curr_y, LAYER layer, int tokens, const char *format, ...);
struct place_object * move_place( struct place_object *cur_place, double x_offset, double y_offset );
struct place_object * move_place_tag( struct place_object *cur_place, double x_offset, double y_offset );
const std::string& insert_netobj_name( const std::string& );
char * find_netobj_name( const std::string& );

extern std::map<std::string,std::string> netobj_name_table;
#endif
