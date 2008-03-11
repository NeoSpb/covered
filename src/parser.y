
%{
/*
 Copyright (c) 2006 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*!
 \file     parser.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/14/2001
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_unit.h"
#include "gen_item.h"
#include "link.h"
#include "obfuscate.h"
#include "parser_misc.h"
#include "profiler.h"
#include "statement.h"
#include "static.h"
#include "util.h"
#include "vector.h"
#include "vsignal.h"

extern char   user_msg[USER_MSG_LENGTH];
extern int    delay_expr_type;
extern int    stmt_conn_id;
extern int    gi_conn_id;
extern isuppl info_suppl;

/* Functions from lexer */
extern void lex_start_udp_table();
extern void lex_end_udp_table();

/*!
 If set to a value > 0, specifies that we are currently parsing code that should be ignored by
 Covered; otherwise, evaluate parsed code.
*/
int ignore_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing a parameter expression
*/
int param_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing an attribute
*/
int attr_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing the control block of a for loop.
*/
int for_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing a generate block
*/
int generate_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing the top-most level of structures in a generate block.
*/
int  generate_top_mode = 0;

/*!
 If set to a value > 0, specifies that we are parsing the body of a generate loop
*/
int  generate_for_mode = 0;

/*!
 If set to a value > 0, specifies that we are creating generate expressions
*/
int  generate_expr_mode = 0;

/*!
 Points to current generate variable name.
*/
char* generate_varname = NULL;

/*!
 Array storing the depth that a given fork block is at.
*/
int* fork_block_depth;

/*!
 Current fork depth position in the fork_block_depth array.
*/
int  fork_depth  = -1;

/*!
 Specifies the current block depth (for embedded blocks).
*/
int  block_depth = 0; 

/*!
 If set to TRUE, specifies that we are currently parsing syntax on the left-hand-side of an
 assignment.
*/
bool lhs_mode    = FALSE;

/*!
 Pointer to head of parameter override list.
*/
param_oride* param_oride_head = NULL;

/*!
 Pointer to tail of parameter override list.
*/
param_oride* param_oride_tail = NULL;

/*!
 Pointer to head of signal list
*/
sig_link* dummy_head    = NULL;

/*!
 Pointer to tail of signal list
*/
sig_link* dummy_tail    = NULL;

/*!
 Contains the last explicitly or implicitly defined packed range information.  This is needed because lists
 of signals/parameters can be made using a previously set range.
*/
sig_range curr_prange;

/*!
 Contains the last explicitly or implicitly defined unpacked range information.  This is needed because lists
 of signals/parameters can be made using a previously set range.
*/
sig_range curr_urange;

/*!
 Contains the last explicitly or implicitly defines signed indication.
*/
bool curr_signed = FALSE;

/*!
 Specifies if the current variable list is handled by Covered or not.
*/
bool curr_handled = TRUE;

/*!
 Specifies if current signal must be assigned or not (by Covered).
*/
bool curr_mba = FALSE;

/*!
 Specifies if current range should be flagged as packed or not.
*/
bool curr_packed = TRUE;

/*!
 Specifies the current signal/port type for a variable list.
*/
int curr_sig_type = SSUPPL_TYPE_DECLARED;

/*!
 Pointer to head of stack structure that stores the contents of generate
 items that we want to refer to later.
*/
gitem_link* save_gi_head = NULL;

/*!
 Pointer to tail of stack structure that stores the contents of generate
 items that we want to refer to later.
*/
gitem_link* save_gi_tail = NULL;

#define YYERROR_VERBOSE 1

/* Uncomment these lines to turn debugging on */
/*
#define YYDEBUG 1
int yydebug = 1; 
*/

/* Recent version of bison expect that the user supply a
   YYLLOC_DEFAULT macro that makes up a yylloc value from existing
   values. I need to supply an explicit version to account for the
   text field, that otherwise won't be copied. */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
          (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
          (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
          (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
          (Current).text         = YYRHSLOC(Rhs, 1).text;               \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC(Rhs, 0).last_line;                                 \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC(Rhs, 0).last_column;                               \
          (Current).text         = YYRHSLOC(Rhs, 0).text;               \
        }                                                               \
    while (0)

%}

%union {
  bool            logical;
  char*           text;
  int	          integer;
  vector*         number;
  double          realtime;
  vsignal*        sig;
  expression*     expr;
  statement*      state;
  static_expr*    statexp;
  str_link*       strlink;
  exp_link*       explink;
  case_statement* case_stmt;
  attr_param*     attr_parm;
  exp_bind*       expbind;
  port_info*      portinfo;
  gen_item*       gitem;
  case_gitem*     case_gi;
  typedef_item*   typdef;
  func_unit*      funit;
};

%token <text>     IDENTIFIER SYSTEM_IDENTIFIER
%token <typdef>   TYPEDEF_IDENTIFIER
%token <text>     PATHPULSE_IDENTIFIER
%token <number>   NUMBER
%token <realtime> REALTIME
%token <number>   STRING
%token IGNORE
%token UNUSED_IDENTIFIER
%token UNUSED_PATHPULSE_IDENTIFIER
%token UNUSED_NUMBER
%token UNUSED_REALTIME
%token UNUSED_STRING UNUSED_SYSTEM_IDENTIFIER
%token K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_LSS K_RS K_RSS K_SG
%token K_ADD_A K_SUB_A K_MLT_A K_DIV_A K_MOD_A K_AND_A K_OR_A K_XOR_A K_LS_A K_RS_A K_ALS_A K_ARS_A K_INC K_DEC K_POW
%token K_PO_POS K_PO_NEG K_STARP K_PSTAR
%token K_LOR K_LAND K_NAND K_NOR K_NXOR K_TRIGGER
%token K_always K_and K_assign K_begin K_buf K_bufif0 K_bufif1 K_case
%token K_casex K_casez K_cmos K_deassign K_default K_defparam K_disable
%token K_edge K_else K_end K_endcase K_endfunction K_endmodule I_endmodule
%token K_endprimitive K_endspecify K_endtable K_endtask K_event K_for
%token K_force K_forever K_fork K_function K_highz0 K_highz1 K_if
%token K_generate K_endgenerate K_genvar
%token K_initial K_inout K_input K_integer K_join K_large K_localparam
%token K_macromodule
%token K_medium K_module K_nand K_negedge K_nmos K_nor K_not K_notif0
%token K_notif1 K_or K_output K_parameter K_pmos K_posedge K_primitive
%token K_pull0 K_pull1 K_pulldown K_pullup K_rcmos K_real K_realtime
%token K_reg K_release K_repeat
%token K_rnmos K_rpmos K_rtran K_rtranif0 K_rtranif1 K_scalered
%token K_signed K_small K_specify
%token K_specparam K_strong0 K_strong1 K_supply0 K_supply1 K_table K_task
%token K_time K_tran K_tranif0 K_tranif1 K_tri K_tri0 K_tri1 K_triand
%token K_trior K_trireg K_vectored K_wait K_wand K_weak0 K_weak1
%token K_while K_wire
%token K_wor K_xnor K_xor
%token K_Shold K_Speriod K_Srecovery K_Ssetup K_Swidth K_Ssetuphold

%token K_automatic K_cell K_use K_library K_config K_endconfig K_design K_liblist K_instance
%token K_showcancelled K_noshowcancelled K_pulsestyle_onevent K_pulsestyle_ondetect

%token K_TU_S K_TU_MS K_TU_US K_TU_NS K_TU_PS K_TU_FS K_TU_STEP
%token K_INCDIR K_DPI
%token K_PP K_PS K_PEQ K_PNE
%token K_bit K_byte K_char K_logic K_shortint K_int K_longint K_unsigned K_shortreal
%token K_unique K_priority K_do
%token K_always_comb K_always_latch K_always_ff
%token K_typedef K_type K_enum K_union K_struct K_packed
%token K_assert K_assume K_property K_endproperty K_cover K_sequence K_endsequence K_expect
%token K_program K_endprogram K_final K_void K_return K_continue K_break K_extern K_interface K_endinterface
%token K_class K_endclass K_extends K_package K_endpackage K_timeunit K_timeprecision K_ref K_bind K_const
%token K_new K_static K_protected K_local K_rand K_randc K_randcase K_constraint K_import K_export K_scalared K_chandle
%token K_context K_pure K_modport K_clocking K_iff K_intersect K_first_match K_throughout K_with K_within
%token K_dist K_covergroup K_endgroup K_coverpoint K_optionDOT K_type_optionDOT K_wildcard
%token K_bins K_illegal_bins K_ignore_bins K_cross K_binsof K_alias K_join_any K_join_none K_matches
%token K_tagged K_foreach K_randsequence K_ifnone K_randomize K_null K_inside K_super K_this K_wait_order
%token K_include K_Sroot K_Sunit K_endclocking K_virtual K_before K_forkjoin K_solve

%token KK_attribute

%type <logical>   automatic_opt block_item_decls_opt
%type <integer>   net_type net_type_sign_range_opt var_type data_type_opt
%type <text>      identifier begin_end_id
%type <text>      udp_port_list
%type <text>      defparam_assign_list defparam_assign
%type <statexp>   static_expr static_expr_primary static_expr_port_list
%type <expr>      expr_primary expression_list expression expression_port_list
%type <expr>      lavalue lpvalue
%type <expr>      event_control event_expression_list event_expression
%type <expr>      delay_value delay_value_simple
%type <expr>      delay1 delay3 delay3_opt
%type <expr>      generate_passign index_expr single_index_expr
%type <state>     statement statement_list statement_or_null 
%type <state>     if_statement_error
%type <state>     passign for_initialization
%type <state>     expression_assignment_list
%type <strlink>   gate_instance gate_instance_list list_of_names
%type <funit>     begin_end_block fork_statement inc_for_depth
%type <attr_parm> attribute attribute_list
%type <portinfo>  port_declaration list_of_port_declarations
%type <gitem>     generate_item generate_item_list generate_item_list_opt
%type <case_stmt> case_items case_item
%type <case_gi>   generate_case_items generate_case_item

%token K_TAND
%right '?' ':'
%left K_LOR
%left K_LAND
%left '|'
%left '^' K_NXOR K_NOR
%left '&' K_NAND
%left K_EQ K_NE K_CEQ K_CNE
%left K_GE K_LE '<' '>'
%left K_LS K_RS K_LSS K_RSS
%left '+' '-'
%left '*' '/' '%' K_POW
%left UNARY_PREC

/* to resolve dangling else ambiguity: */
%nonassoc less_than_K_else
%nonassoc K_else

%%

  /* A degenerate source file can be completely empty. */
main 
  : source_file 
  |
  ;

  /* Verilog-2001 supports attribute lists, which can be attached to a
     variety of different objects. The syntax inside the (* *) is a
     comma separated list of names or names with assigned values. */
attribute_list_opt
  : K_PSTAR
    {
      if( (ignore_mode == 0) && !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Attribute syntax found in block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      }
    }
    attribute_list K_STARP
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        ignore_mode--;
      } else if( ignore_mode == 0 ) {
        Try {
          db_parse_attribute( $3 );
        } Catch_anonymous {
          attribute_dealloc( $3 );
          Throw 0;
        }
      }
    }
  | K_PSTAR
    {
      if( (ignore_mode == 0) && !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Attribute syntax found in block that is specified to not allow Verilog-2001 syntax" );
      }
    }
    K_STARP
  |
  ;

attribute_list
  : attribute_list ',' attribute
    {
      if( (ignore_mode == 0) && ($3 != NULL) && ($1 != NULL) ) {
        $3->next  = $1;
        $1->prev  = $3;
        $3->index = $1->index + 1;
        $$ = $3;
      } else {
        $$ = NULL;
      }
    } 
  | attribute
    {
      $$ = $1;
    }
  ;

attribute
  : IDENTIFIER
    {
      attr_param* ap;
      if( ignore_mode == 0 ) {
        ap = db_create_attr_param( $1, NULL );
        $$ = ap;
      } else {
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | IDENTIFIER '=' {attr_mode++;} expression {attr_mode--;}
    {
      attr_param* ap;
      if( ignore_mode == 0 ) {
        ap = db_create_attr_param( $1, $4 );
        $$ = ap;
      } else {
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '=' expression
    {
      $$ = NULL;
    }
  ;

source_file 
  : description 
  | source_file description
  ;

description
  : module
  | udp_primitive
  | KK_attribute { ignore_mode++; } '(' UNUSED_IDENTIFIER ',' UNUSED_STRING ',' UNUSED_STRING ')' { ignore_mode--; }
  | typedef_decl
  | net_type signed_opt range_opt list_of_variables ';'
  | net_type signed_opt range_opt net_decl_assigns ';'
  | net_type drive_strength
    {
      if( (ignore_mode == 0) && ($1 == 1) ) {
        parser_implicitly_set_curr_range( 0, 0, TRUE );
      }
      curr_signed = FALSE;
    }
    net_decl_assigns ';'
  | K_trireg charge_strength_opt range_opt delay3_opt
    {
      curr_mba      = FALSE;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
    }
    list_of_variables ';'
  | gatetype gate_instance_list ';'
    {
      str_link_delete_list( $2 );
    }
  | gatetype delay3 gate_instance_list ';'
    {
      str_link_delete_list( $3 );
    }
  | gatetype drive_strength gate_instance_list ';'
    {
      str_link_delete_list( $3 );
    }
  | gatetype drive_strength delay3 gate_instance_list ';'
    {
      str_link_delete_list( $4 );
    }
  | K_pullup gate_instance_list ';'
    {
      str_link_delete_list( $2 );
    }
  | K_pulldown gate_instance_list ';'
    {
      str_link_delete_list( $2 );
    }
  | K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      str_link_delete_list( $5 );
    }
  | K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      str_link_delete_list( $5 );
    }
  | block_item_decl
  | K_event
    {
      curr_mba      = TRUE;
      curr_signed   = FALSE;
      curr_sig_type = SSUPPL_TYPE_EVENT;
      curr_handled  = TRUE;
      parser_implicitly_set_curr_range( 0, 0, TRUE );
    }
    list_of_variables ';'
  | K_task automatic_opt IDENTIFIER ';'
    {
      Try {
        if( ignore_mode == 0 ) {
          if( !db_add_function_task_namedblock( ($2 ? FUNIT_ATASK : FUNIT_TASK), $3, @3.text, @3.first_line ) ) {
            ignore_mode++;
          }
        }
      } Catch_anonymous {
        free_safe( $3 );
        Throw 0;
      }
      free_safe( $3 );
    }
    task_item_list_opt statement_or_null
    {
      statement* stmt = $7;
      if( (ignore_mode == 0) && (stmt != NULL) ) {
        stmt->suppl.part.head      = 1;
        stmt->suppl.part.is_called = 1;
        db_add_statement( stmt, stmt );
      }
    }
    K_endtask
    {
      if( ignore_mode == 0 ) {
        db_end_function_task_namedblock( @9.first_line );
      } else {
        ignore_mode--;
      }
    }
  | K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    {
      if( ignore_mode == 0 ) {
        Try {
          if( db_add_function_task_namedblock( ($2 ? FUNIT_AFUNCTION : FUNIT_FUNCTION), $5, @5.text, @5.first_line ) ) {
            db_add_signal( $5, SSUPPL_TYPE_IMPLICIT, &curr_prange, NULL, curr_signed, FALSE, @5.first_line, @5.first_column, TRUE );
          } else {
            ignore_mode++;
          }
        } Catch_anonymous {
          free_safe( $5 );
          Throw 0;
        }
        free_safe( $5 );
      }
    }
    function_item_list statement
    {
      statement* stmt = $9;
      if( (ignore_mode == 0) && (stmt != NULL) ) {
        stmt->suppl.part.head      = 1;
        stmt->suppl.part.is_called = 1;
        db_add_statement( stmt, stmt );
      }
    }
    K_endfunction
    {
      if( ignore_mode == 0 ) {
        db_end_function_task_namedblock( @11.first_line );
      } else {
        ignore_mode--;
      }
    }
  | error ';'
    {
      VLerror( "Invalid $root item" );
    }
  | K_function error K_endfunction
    {
      VLerror( "Syntax error in function description" );
    }
  | enumeration list_of_names ';'
    {
      str_link* strl = $2;
      while( strl != NULL ) {
        db_add_signal( strl->str, SSUPPL_TYPE_DECLARED, &curr_prange, NULL, curr_signed, FALSE, strl->suppl, strl->suppl2, TRUE );
        strl = strl->next;
      }
      str_link_delete_list( $2 );
    }
  ;

module
  : attribute_list_opt module_start IDENTIFIER 
    {
      db_add_module( $3, @2.text, @2.first_line );
      free_safe( $3 );
    }
    module_parameter_port_list_opt
    module_port_list_opt ';'
    module_item_list_opt K_endmodule
    {
      db_end_module( @9.first_line );
    }
  | attribute_list_opt K_module IGNORE I_endmodule
  | attribute_list_opt K_macromodule IGNORE I_endmodule
  ;

module_start
  : K_module
  | K_macromodule
  ;

module_parameter_port_list_opt
  : '#'
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Pre-module parameter declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      }
    }
    '(' module_parameter_port_list ')'
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        ignore_mode--;
      }
    }
  |
  ;

module_parameter_port_list
  : K_parameter signed_opt range_opt parameter_assign
    {
      curr_prange.clear = TRUE;
    }
  | module_parameter_port_list ',' parameter_assign
  | module_parameter_port_list ',' K_parameter signed_opt range_opt parameter_assign
    {
      curr_prange.clear = TRUE;
    }
  ;

module_port_list_opt
  : '(' list_of_ports ')'
  | '(' list_of_port_declarations ')'
    {
      if( $2 != NULL ) {
        parser_dealloc_sig_range( $2->prange, TRUE );
        parser_dealloc_sig_range( $2->urange, TRUE );
        free_safe( $2 );
      }
    }
  |
  ;

  /* Verilog-2001 syntax for declaring all ports within module port list */
list_of_port_declarations
  : port_declaration
    {
      $$ = $1;
    }
  | list_of_port_declarations ',' port_declaration
    {
      if( $1 != NULL ) {
        parser_dealloc_sig_range( $1->prange, TRUE );
        parser_dealloc_sig_range( $1->urange, TRUE );
        free_safe( $1 );
      }
      $$ = $3;
    }
  | list_of_port_declarations ',' IDENTIFIER
    {
      if( $1 != NULL ) {
        db_add_signal( $3, $1->type, $1->prange, $1->urange, curr_signed, curr_mba, @3.first_line, @3.first_column, TRUE );
      }
      free_safe( $3 );
      $$ = $1;
    }
  ;

  /* Handles Verilog-2001 port of type:  input wire [m:l] <list>; */
port_declaration
  : attribute_list_opt port_type net_type_sign_range_opt IDENTIFIER
    { PROFILE(PARSER_PORT_DECLARATION_A);
      port_info* pi;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Inline port declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
        free_safe( $4 );
        $$ = NULL;
      } else {
        if( ignore_mode == 0 ) {
          db_add_signal( $4, curr_sig_type, &curr_prange, NULL, curr_signed, FALSE, @4.first_line, @4.first_column, TRUE );
          pi = (port_info*)malloc_safe( sizeof( port_info ) );
          pi->type      = curr_sig_type;
          pi->is_signed = curr_signed;
          pi->prange    = parser_copy_curr_range( TRUE );
          pi->urange    = parser_copy_curr_range( FALSE );
          free_safe( $4 );
          $$ = pi;
        } else {
          $$ = NULL;
        }
      }
    }
  | attribute_list_opt K_output var_type signed_opt range_opt IDENTIFIER
    { PROFILE(PARSER_PORT_DECLARATION_B);
      port_info* pi;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Inline port declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
        free_safe( $6 );
        $$ = NULL;
      } else {
        if( ignore_mode == 0 ) {
          db_add_signal( $6, SSUPPL_TYPE_OUTPUT, &curr_prange, NULL, curr_signed, FALSE, @6.first_line, @6.first_column, TRUE );
          pi = (port_info*)malloc_safe( sizeof( port_info ) );
          pi->type      = SSUPPL_TYPE_OUTPUT;
          pi->is_signed = curr_signed;
          pi->prange    = parser_copy_curr_range( TRUE );
          pi->urange    = parser_copy_curr_range( FALSE );
          free_safe( $6 );
          $$ = pi;
        } else {
          $$ = NULL;
        }
      }
    }
  /* We just need to parse the static register assignment as this signal will get its value from the dumpfile */
  | attribute_list_opt K_output var_type signed_opt range_opt IDENTIFIER '=' ignore_more static_expr ignore_less
    { PROFILE(PARSER_PORT_DECLARATION_C);
      port_info* pi;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Inline port declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
        free_safe( $6 );
        $$ = NULL;
      } else {
        if( ignore_mode == 0 ) {
          db_add_signal( $6, SSUPPL_TYPE_OUTPUT, &curr_prange, NULL, curr_signed, FALSE, @6.first_line, @6.first_column, TRUE );
          pi = (port_info*)malloc_safe( sizeof( port_info ) );
          pi->type       = SSUPPL_TYPE_OUTPUT;
          pi->is_signed  = curr_signed;
          pi->prange     = parser_copy_curr_range( TRUE );
          pi->urange     = parser_copy_curr_range( FALSE );
          free_safe( $6 );
          $$ = pi;
        } else {
          $$ = NULL;
        }
      }
    }
  | attribute_list_opt port_type net_type_sign_range_opt error
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Inline port declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
      } else if( ignore_mode == 0 ) {
        VLerror( "Invalid variable list in port declaration" );
      }
      $$ = NULL;
    }
  | attribute_list_opt K_output var_type signed_opt range_opt error
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Inline port declaration syntax found in block that is specified to not allow Verilog-2001 syntax" );
      } else if( ignore_mode == 0 ) {
        VLerror( "Invalid variable list in port declaration" );
      }
      $$ = NULL;
    }

  ;

list_of_ports
  : list_of_ports ',' port_opt
  | port_opt
  ;

port_opt
  : port
  |
  ;

  /* Coverage tools should not find port information that interesting.  We will
     handle it for parsing purposes, but ignore its information. */
port
  : port_reference
  | '.' IDENTIFIER '(' port_reference ')'
    {
      free_safe( $2 );
    }
  | '{' port_reference_list '}'
  | '.' IDENTIFIER '(' '{' port_reference_list '}' ')'
    {
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' '{' port_reference_list '}' ')'
  ;

port_reference
  : IDENTIFIER
    {
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER
  | IDENTIFIER '[' ignore_more static_expr ':' static_expr ignore_less ']'
    {
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '[' ignore_more static_expr ':' static_expr ignore_less ']'
  | IDENTIFIER '[' ignore_more static_expr ignore_less ']'
    {
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '[' ignore_more static_expr ignore_less ']'
  | IDENTIFIER '[' error ']'
    {
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '[' error ']'
  ;

port_reference_list
  : port_reference
  | port_reference_list ',' port_reference
  ;

static_expr_port_list
  : static_expr_port_list ',' static_expr
    {
      static_expr* tmp = $3;
      if( ignore_mode == 0 ) {
        if( $3 != NULL ) {
          Try {
            tmp = static_expr_gen_unary( $3, EXP_OP_PASSIGN, @3.first_line, @3.first_column, (@3.last_column - 1) );
          } Catch_anonymous {
            static_expr_dealloc( $3, TRUE );
            Throw 0;
          }
          Try {
            tmp = static_expr_gen( tmp, $1, EXP_OP_LIST, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            static_expr_dealloc( tmp, TRUE );
            static_expr_dealloc( $1, TRUE );
            Throw 0;
          }
          $$ = tmp;
        } else {
          $$ = $1;
        }
      } else {
        $$ = NULL;
      }
    }
  | static_expr
    {
      static_expr* tmp = $1;
      if( ignore_mode == 0 ) {
        if( $1 != NULL ) {
          Try {
            tmp = static_expr_gen_unary( $1, EXP_OP_PASSIGN, @1.first_line, @1.first_column, (@1.last_column - 1) );
          } Catch_anonymous {
            static_expr_dealloc( $1, TRUE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  ;

static_expr
  : static_expr_primary
    {
      $$ = $1;
    }
  | '+' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp = $2;
      if( tmp != NULL ) {
        if( tmp->exp == NULL ) {
          tmp->num = 0 + tmp->num;
        }
      }
      $$ = tmp;
    }
  | '-' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp = $2;
      if( tmp != NULL ) {
        if( tmp->exp == NULL ) {
          tmp->num = 0 - tmp->num;
        }
      }
      $$ = tmp;
    }
  | '~' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UINV, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | '&' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UAND, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | '!' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UNOT, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | '|' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | '^' static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UXOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | K_NAND static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UNAND, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | K_NOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UNOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | K_NXOR static_expr_primary %prec UNARY_PREC
    {
      static_expr* tmp;
      Try {
        tmp = static_expr_gen_unary( $2, EXP_OP_UNXOR, @1.first_line, @1.first_column, (@1.last_column - 1) );
      } Catch_anonymous {
        static_expr_dealloc( $2, TRUE );
        Throw 0;
      }
      $$ = tmp;
    }
  | static_expr '^' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_XOR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '*' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MULTIPLY, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '/' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_DIVIDE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '%' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_MOD, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '+' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_ADD, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '-' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_SUBTRACT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_POW static_expr
    {
      static_expr* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Exponential power operation found in block that is specified to not allow Verilog-2001 syntax" );
        static_expr_dealloc( $1, TRUE );
        static_expr_dealloc( $3, TRUE );
        $$ = NULL;
      } else {
        tmp = static_expr_gen( $3, $1, EXP_OP_EXPONENT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        $$ = tmp;
      }
    }
  | static_expr '&' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_AND, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '|' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_OR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_NOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NOR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_NAND static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NAND, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_NXOR static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_NXOR, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_LS static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_LSHIFT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_RS static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_RSHIFT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_GE static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_GE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_LE static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_LE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr K_EQ static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_EQ, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '>' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_GT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  | static_expr '<' static_expr
    {
      static_expr* tmp;
      tmp = static_expr_gen( $3, $1, EXP_OP_LT, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
      $$ = tmp;
    }
  ;

static_expr_primary
  : NUMBER
    { PROFILE(PARSER_STATIC_EXPR_PRIMARY_A);
      static_expr* tmp;
      if( ignore_mode == 0 ) {
        tmp = (static_expr*)malloc_safe( sizeof( static_expr ) );
        if( vector_is_unknown( $1 ) ) {
          Try {
            tmp->exp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            free_safe( tmp );
            vector_dealloc( $1 );
            Throw 0;
          }
          vector_dealloc( tmp->exp->value );
          tmp->exp->value = $1;
        } else {
          tmp->num = vector_to_int( $1 );
          tmp->exp = NULL;
          vector_dealloc( $1 );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | UNUSED_NUMBER
    {
      $$ = NULL;
    }
  | REALTIME
    {
      $$ = NULL;
    }
  | UNUSED_REALTIME
    {
      $$ = NULL;
    }
  | IDENTIFIER
    { PROFILE(PARSER_STATIC_EXPR_PRIMARY_B);
      static_expr* tmp;
      if( ignore_mode == 0 ) {
        tmp = (static_expr*)malloc_safe( sizeof( static_expr ) );
        tmp->num = -1;
        Try {
          tmp->exp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        } Catch_anonymous {
          free_safe( tmp );
          free_safe( $1 );
          Throw 0;
        }
        free_safe( $1 );
        $$ = tmp;
      } else {
        assert( $1 == NULL );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | '(' static_expr ')'
    {
      $$ = $2;
    }
  | IDENTIFIER '(' static_expr_port_list ')'
    {
      if( ignore_mode == 0 ) {
        $$ = static_expr_gen( NULL, $3, EXP_OP_FUNC_CALL, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
      } else {
        static_expr_dealloc( $3, TRUE );
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '(' static_expr_port_list ')'
    {
      $$ = NULL;
      static_expr_dealloc( $3, TRUE );
    }
  | IDENTIFIER '[' static_expr ']'
    {
      if( ignore_mode == 0 ) { 
        $$ = static_expr_gen( NULL, $3, EXP_OP_SBIT_SEL, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
      } else {
        static_expr_dealloc( $3, TRUE );
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '[' static_expr ']'
    {
      $$ = NULL;
      static_expr_dealloc( $3, TRUE );
    }
  | SYSTEM_IDENTIFIER
    {
      free_safe( $1 );
      $$ = NULL;
    }
  | UNUSED_SYSTEM_IDENTIFIER
    {
      $$ = NULL;
    }
  ;

expression
  : expr_primary
    {
      $$ = $1;
    }
  | '+' expr_primary %prec UNARY_PREC
    {
      if( ignore_mode == 0 ) {
        $2->value->suppl.part.is_signed = 1;
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
  | '-' expr_primary %prec UNARY_PREC
    {
      expression* exp;
      if( ignore_mode == 0 ) {
        Try {
          exp = db_create_expression( $2, NULL, EXP_OP_NEGATE, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          exp->value->suppl.part.is_signed = 1;
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
        $$ = exp;
      } else {
        $$ = NULL;
      }
    }
  | '~' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UINV, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '&' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UAND, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '!' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UNOT, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '|' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '^' expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UXOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NAND expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UNAND, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NOR expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UNOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | K_NXOR expr_primary %prec UNARY_PREC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $2 != NULL ) {
          Try {
            tmp = db_create_expression( $2, NULL, EXP_OP_UNXOR, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $2, FALSE );
            Throw 0;
          }
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | '+' error %prec UNARY_PREC
    {
      VLerror( "Operand of signed positive + is not a primary expression" );
    }
  | '-' error %prec UNARY_PREC
    {
      VLerror( "Operand of signed negative - is not a primary expression" );
    }
  | '~' error %prec UNARY_PREC
    {
      VLerror( "Operand of unary ~ is not a primary expression" );
    }
  | '&' error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction & is not a primary expression" );
    }
  | '!' error %prec UNARY_PREC
    {
      VLerror( "Operand of unary ! is not a primary expression" );
    }
  | '|' error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction | is not a primary expression" );
    }
  | '^' error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ^ is not a primary expression" );
    }
  | K_NAND error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ~& is not a primary expression" );
    }
  | K_NOR error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ~| is not a primary expression" );
    }
  | K_NXOR error %prec UNARY_PREC
    {
      VLerror( "Operand of reduction ~^ is not a primary expression" );
    }
  | expression '^' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_XOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '*' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_MULTIPLY, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '/' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_DIVIDE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '%' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_MOD, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '+' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_ADD, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '-' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_SUBTRACT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_POW expression
    {
      expression* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Exponential power operator found in block that is specified to not allow Verilog-2001 syntax" );
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      } else {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            tmp = db_create_expression( $3, $1, EXP_OP_EXPONENT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          $$ = tmp;
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      }
    }
  | expression '&' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_AND, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '|' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_OR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NAND expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_NAND, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NOR expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_NOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NXOR expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_NXOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '<' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_LT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '>' expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_GT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LS expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_LSHIFT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LSS expression
    {
      expression* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Arithmetic left shift operation found in block that is specified to not allow Verilog-2001 syntax" );
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      } else {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            tmp = db_create_expression( $3, $1, EXP_OP_ALSHIFT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          $$ = tmp;
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      }
    }
  | expression K_RS expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_RSHIFT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_RSS expression
    {
      expression* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Arithmetic right shift operation found in block that is specified to not allow Verilog-2001 syntax" );
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      } else {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            tmp = db_create_expression( $3, $1, EXP_OP_ARSHIFT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          $$ = tmp;
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      }
    }
  | expression K_EQ expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_EQ, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_CEQ expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_CEQ, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_LE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_GE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_GE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_NE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_NE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_CNE expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_CNE, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LOR expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_LOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression K_LAND expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_LAND, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | expression '?' expression ':' expression
    {
      expression* csel;
      expression* cond;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($5 != NULL) ) {
        Try {
          csel = db_create_expression( $5,   $3, EXP_OP_COND_SEL, lhs_mode, $3->line, @1.first_column, (@3.last_column - 1), NULL );
          cond = db_create_expression( csel, $1, EXP_OP_COND,     lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $5, FALSE );
          Throw 0;
        }
        $$ = cond;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        expression_dealloc( $5, FALSE );
        $$ = NULL;
      }
    }
  ;

expr_primary
  : NUMBER
    {
      expression* tmp;
      Try {
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
      } Catch_anonymous {
        vector_dealloc( $1 );
        Throw 0;
      }
      vector_dealloc( tmp->value );
      tmp->value = $1;
      $$ = tmp;
    }
  | UNUSED_NUMBER
    {
      $$ = NULL;
    }
  | REALTIME
    {
      $$ = NULL;
    }
  | UNUSED_REALTIME
    {
      $$ = NULL;
    }
  | STRING
    {
      expression* tmp;
      Try {
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
      } Catch_anonymous {
        vector_dealloc( $1 );
        Throw 0;
      }
      vector_dealloc( tmp->value );
      tmp->value = $1;
      $$ = tmp;
    }
  | UNUSED_STRING
    {
      $$ = NULL;
    }
  | identifier
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
        $$  = tmp;
      } else {
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | identifier K_INC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          tmp = db_create_expression( NULL, tmp,  EXP_OP_PINC, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$  = tmp; 
      } else {
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | identifier K_DEC
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          tmp = db_create_expression( NULL, tmp,  EXP_OP_PDEC, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$  = tmp;
      } else {
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | K_INC identifier
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $2 );
          tmp = db_create_expression( NULL, tmp,  EXP_OP_IINC, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          free_safe( $2 );
          Throw 0;
        }
        $$  = tmp;
      } else {
        $$ = NULL;
      }
      free_safe( $2 );
    }
  | K_DEC identifier
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $2 );
          tmp = db_create_expression( NULL, tmp,  EXP_OP_IDEC, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          free_safe( $2 );
          Throw 0;
        }
        $$  = tmp;
      } else {
        $$ = NULL;
      }
      free_safe( $2 );
    }
  | SYSTEM_IDENTIFIER
    {
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          if( strncmp( $1, "$display", 8 ) == 0 ) {
            $$ = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL );
          } else if( strncmp( $1, "$finish", 7 ) == 0 ) {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SFINISH, lhs_mode, 0, 0, 0, NULL );
          } else if( strncmp( $1, "$stop", 5 ) == 0 ) {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SSTOP, lhs_mode, 0, 0, 0, NULL );
          } else {
            $$ = NULL;
          }
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
      } else {
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | UNUSED_SYSTEM_IDENTIFIER
    {
      $$ = NULL;
    }
  | identifier index_expr
    {
      if( (ignore_mode == 0) && ($1 != NULL) && ($2 != NULL) ) {
        db_bind_expr_tree( $2, $1 );
        $2->line = @1.first_line;
        $2->col  = ((@1.first_column & 0xffff) << 16) | ($2->col & 0xffff);
        $$ = $2;
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | identifier index_expr K_INC
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        db_bind_expr_tree( $2, $1 );
        $2->line = @1.first_line;
        $2->col  = ((@1.first_column & 0xffff) << 16) | ($2->col & 0xffff);
        Try {
          tmp = db_create_expression( NULL, $2, EXP_OP_PINC, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | identifier index_expr K_DEC
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        db_bind_expr_tree( $2, $1 );
        $2->line = @1.first_line;
        $2->col  = ((@1.first_column & 0xffff) << 16) | ($2->col & 0xffff);
        Try {
          tmp = db_create_expression( NULL, $2, EXP_OP_PDEC, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | K_INC identifier index_expr
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        db_bind_expr_tree( $3, $2 );
        $3->line = @2.first_line;
        $3->col  = ((@2.first_column & 0xffff) << 16) | ($3->col & 0xffff);
        Try {
          tmp = db_create_expression( NULL, $3, EXP_OP_IINC, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          free_safe( $2 );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
      free_safe( $2 );
    }
  | K_DEC identifier index_expr
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        db_bind_expr_tree( $3, $2 );
        $3->line = @2.first_line;
        $3->col  = ((@2.first_column & 0xffff) << 16) | ($3->col & 0xffff);
        Try {
          tmp = db_create_expression( NULL, $3, EXP_OP_IDEC, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          free_safe( $2 );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
      free_safe( $2 );
    }
  | identifier '(' expression_port_list ')'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL ) ) {
        Try {
          tmp = db_create_expression( NULL, $3, EXP_OP_FUNC_CALL, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), $1 );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$  = tmp;
      } else {
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | SYSTEM_IDENTIFIER '(' ignore_more expression_port_list ignore_less ')'
    {
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          if( strncmp( $1, "$display", 8 ) == 0 ) {
            $$ = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL );
          } else if( strncmp( $1, "$finish", 7 ) == 0 ) {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SFINISH, lhs_mode, 0, 0, 0, NULL );
          } else if( strncmp( $1, "$stop", 5 ) == 0 ) {
            $$ = db_create_expression( NULL, NULL, EXP_OP_SSTOP, lhs_mode, 0, 0, 0, NULL );
          } else {
            $$ = NULL;
          }
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
      } else {
        $$ = NULL;
      }
      free_safe( $1 );
    }
  | UNUSED_SYSTEM_IDENTIFIER '(' expression_port_list ')'
    {
      $$ = NULL;
    }
  | '(' expression ')'
    {
      if( ignore_mode == 0 ) {
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
  | '{' expression_list '}'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          tmp = db_create_expression( $2, NULL, EXP_OP_CONCAT, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
    }
  | '{' expression '{' expression_list '}' '}'
    {
      expression*  tmp;
      if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
        Try {
          tmp = db_create_expression( $4, $2, EXP_OP_EXPAND, lhs_mode, @1.first_line, @1.first_column, (@6.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          expression_dealloc( $4, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $2, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  ;

  /* Expression lists are used in concatenations and parameter overrides */
expression_list
  : expression_list ',' expression
    { PROFILE(PARSER_EXPRESSION_LIST_A);
      expression*  tmp;
      expression*  exp = $3;
      param_oride* po;
      if( ignore_mode == 0 ) {
        if( param_mode == 0 ) {
          if( $3 != NULL ) {
            Try {
              tmp = db_create_expression( exp, $1, EXP_OP_LIST, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( $1, FALSE );
              expression_dealloc( exp, FALSE );
              Throw 0;
            }
            $$ = tmp;
          } else {
            $$ = $1;
          }
        } else {
          if( $3 != NULL ) {
            po = (param_oride*)malloc_safe( sizeof( param_oride ) );
            po->name = NULL;
            po->expr = $3;
            po->next = NULL;
            if( param_oride_head == NULL ) {
              param_oride_head = param_oride_tail = po;
            } else {
              param_oride_tail->next = po;
              param_oride_tail       = po;
            }
          }
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  | expression
    { PROFILE(PARSER_EXPRESSION_LIST_B);
      expression*  exp = $1;
      param_oride* po;
      if( ignore_mode == 0 ) {
        if( param_mode == 0 ) {
          $$ = exp;
        } else {
          if( $1 != NULL ) {
            po = (param_oride*)malloc_safe( sizeof( param_oride ) );
            po->name = NULL;
            po->expr = $1;
            po->next = NULL;
            if( param_oride_head == NULL ) {
              param_oride_head = param_oride_tail = po;
            } else {
              param_oride_tail->next = po;
              param_oride_tail       = po;
            }
          }
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  |
    { PROFILE(PARSER_EXPRESSION_LIST_C);
      param_oride* po;
      if( (ignore_mode == 0) && (param_mode == 1) ) {
        po = (param_oride*)malloc_safe( sizeof( param_oride ) );
        po->name = NULL;
        po->expr = NULL;
        po->next = NULL;
        if( param_oride_head == NULL ) {
          param_oride_head = param_oride_tail = po;
        } else {
          param_oride_tail->next = po;
          param_oride_tail       = po;
        }
      }
      $$ = NULL;
    }
  | expression_list ','
    { PROFILE(PARSER_EXPRESSION_LIST_D);
      param_oride* po;
      if( (ignore_mode == 0) && (param_mode == 1) ) {
        po = (param_oride*)malloc_safe( sizeof( param_oride ) );
        po->name = NULL;
        po->expr = NULL;
        po->next = NULL;
        if( param_oride_head == NULL ) {
          param_oride_head = param_oride_tail = po;
        } else {
          param_oride_tail->next = po;
          param_oride_tail       = po;
        }
      }
      $$ = $1;
    }
  ;

  /* Expression port lists are used in function/task calls */
expression_port_list
  : expression_port_list ',' expression
    {
      expression* tmp = NULL;
      if( ignore_mode == 0 ) {
        if( $3 != NULL ) {
          Try {
            tmp = db_create_expression( $3, NULL, EXP_OP_PASSIGN, 0, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_LIST, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            expression_dealloc( tmp, FALSE );
            Throw 0;
          }
          $$ = tmp;
        } else {
          $$ = $1;
        }
      } else {
        $$ = NULL;
      }
    }
  | expression
    {
      expression* exp = $1;
      if( ignore_mode == 0 ) {
        Try {
          exp = db_create_expression( $1, NULL, EXP_OP_PASSIGN, 0, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          Throw 0;
        }
        $$ = exp;
      } else {
        $$ = NULL;
      }
    }
  ;

begin_end_id
  : ':' IDENTIFIER        { $$ = $2;   }
  | ':' UNUSED_IDENTIFIER { $$ = NULL; }
  | { $$ = db_create_unnamed_scope(); }
  ;

identifier
  : IDENTIFIER
    {
      $$ = $1;
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | identifier '.' IDENTIFIER
    { PROFILE(PARSER_IDENTIFIER_A);
      int   len = strlen( $1 ) + strlen( $3 ) + 2;
      char* str = (char*)malloc_safe( len );
      unsigned int rv = snprintf( str, len, "%s.%s", $1, $3 );
      assert( rv < len );
      if( $1 != NULL ) {
        free_safe( $1 );
      }
      free_safe( $3 );
      $$ = str;
    }
  | identifier '.' UNUSED_IDENTIFIER
    {
      if( $1 != NULL ) {
        free_safe( $1 );
      }
      $$ = NULL;
    }
  ;

list_of_variables
  : IDENTIFIER
    {
      db_add_signal( $1, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @1.first_line, @1.first_column, curr_handled );
      free_safe( $1 );
    }
  | IDENTIFIER { curr_packed = FALSE; } range
    {
      curr_packed = TRUE;
      if( !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Unpacked array specified for net type in block that was specified to not allow SystemVerilog syntax" );
      } else {
        db_add_signal( $1, curr_sig_type, &curr_prange, &curr_urange, curr_signed, curr_mba, @1.first_line, @1.first_column, curr_handled );
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER
  | UNUSED_IDENTIFIER range
  | list_of_variables ',' IDENTIFIER
    {
      db_add_signal( $3, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @3.first_line, @3.first_column, curr_handled );
      free_safe( $3 );
    }
  | list_of_variables ',' IDENTIFIER range
    {
      if( !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Unpacked array specified for net type in block that was specified to not allow SystemVerilog syntax" );
      } else {
        db_add_signal( $3, curr_sig_type, &curr_prange, &curr_urange, curr_signed, curr_mba, @3.first_line, @3.first_column, curr_handled );
      }
      free_safe( $3 );
    }
  | list_of_variables ',' UNUSED_IDENTIFIER
  | list_of_variables ',' UNUSED_IDENTIFIER range
  ;

  /* I don't know what to do with UDPs.  This is included to allow designs that contain
     UDPs to pass parsing. */
udp_primitive
  : K_primitive IDENTIFIER '(' udp_port_list ')' ';'
      udp_port_decls
      udp_init_opt
      udp_body
    K_endprimitive
    {
      /* We will treat primitives like regular modules */
      db_add_module( $2, @1.text, @1.first_line );
      db_end_module( @10.first_line );
      free_safe( $2 );
    }
  | K_primitive IGNORE K_endprimitive
  ;

udp_port_list
  : IDENTIFIER
    {
      free_safe( $1 );
      $$ = NULL;
    }
  | udp_port_list ',' IDENTIFIER
    {
      free_safe( $3 );
      $$ = NULL;
    }
  ;

udp_port_decls
  : udp_port_decl
  | udp_port_decls udp_port_decl
  ;

udp_port_decl
  : K_input ignore_more list_of_variables ignore_less ';'
  | K_output IDENTIFIER ';'
    {
      free_safe( $2 );
    }
  | K_reg IDENTIFIER ';'
    {
      free_safe( $2 );
    }
  ;

udp_init_opt
  : udp_initial
  |
  ;

udp_initial
  : K_initial IDENTIFIER '=' NUMBER ';'
    {
      free_safe( $2 );
      vector_dealloc( $4 );
    }
  ;

udp_body
  : K_table { lex_start_udp_table(); }
      udp_entry_list
    K_endtable { lex_end_udp_table(); }
  ;

udp_entry_list
  : udp_comb_entry_list
  | udp_sequ_entry_list
  ;

udp_comb_entry_list
  : udp_comb_entry
  | udp_comb_entry_list udp_comb_entry
  ;

udp_comb_entry
  : udp_input_list ':' udp_output_sym ';'
  ;

udp_input_list
  : udp_input_sym
  | udp_input_list udp_input_sym
  ;

udp_input_sym
  : '0'
  | '1'
  | 'x'
  | '?'
  | 'b'
  | '*'
  | '%'
  | 'f'
  | 'F'
  | 'l'
  | 'h'
  | 'B'
  | 'r'
  | 'R'
  | 'M'
  | 'n'
  | 'N'
  | 'p'
  | 'P'
  | 'Q'
  | '_'
  | '+'
  ;

udp_output_sym
  : '0'
  | '1'
  | 'x'
  | '-'
  ;

udp_sequ_entry_list
  : udp_sequ_entry
  | udp_sequ_entry_list udp_sequ_entry
  ;

udp_sequ_entry
  : udp_input_list ':' udp_input_sym ':' udp_output_sym ';'
  ;

 /* This statement type can be found in FOR statements in generate blocks */
generate_passign
  : IDENTIFIER '=' static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER '=' static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_ADD_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_ADD, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_ADD_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_SUB_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_SUBTRACT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_SUB_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_MLT_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_MULTIPLY, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_MLT_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_DIV_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_DIVIDE, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_DIV_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_MOD_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_MOD, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_MOD_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_AND_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_AND, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_AND_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_OR_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_OR, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_OR_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_XOR_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_XOR, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_XOR_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_LS_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_LSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_LS_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_RS_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_RSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_RS_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_ALS_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_ALSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_ALS_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_ARS_A static_expr
    {
      expression* expr = NULL;
      expression* expl = NULL;
      if( $3 != NULL ) {
        Try {
          expl = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
          expr = db_create_expr_from_static( $3, @3.first_line, @3.first_column, (@3.last_column - 1) );
          expr = db_create_expression( expr, expl, EXP_OP_ARSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
          expl = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          expr = db_create_expression( expr, expl, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          if( generate_varname == NULL ) {
            generate_varname = $1;
          }
        } Catch_anonymous {
          static_expr_dealloc( $3, TRUE );
          expression_dealloc( expl, FALSE );
          expression_dealloc( expr, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        $$ = expr;
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_IDENTIFIER K_ARS_A static_expr
    {
      static_expr_dealloc( $3, TRUE );
      $$ = NULL;
    }
  | IDENTIFIER K_INC
    {
      expression* expr = NULL;
      Try {
        expr = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        expr = db_create_expression( NULL, expr, EXP_OP_PINC, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        if( generate_varname == NULL ) {
          generate_varname = $1;
        }
      } Catch_anonymous {
        expression_dealloc( expr, FALSE );
        free_safe( $1 );
        Throw 0;
      }
      $$ = expr;
    }
  | UNUSED_IDENTIFIER K_INC
    {
      $$ = NULL;
    }
  | IDENTIFIER K_DEC
    {
      expression* expr = NULL;
      Try {
        expr = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        expr = db_create_expression( NULL, expr, EXP_OP_PDEC, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        if( generate_varname == NULL ) {
          generate_varname = $1;
        }
      } Catch_anonymous {
        expression_dealloc( expr, FALSE );
        free_safe( $1 );
        Throw 0;
      }
      $$ = expr;
    }
  | UNUSED_IDENTIFIER K_DEC
    {
      $$ = NULL;
    }
  ;

  /* Generate support */
generate_item_list_opt
  : generate_item_list
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

generate_item_list
  : generate_item_list generate_item
    {
      if( $1 == NULL ) {
        $$ = $2;
      } else if( $2 == NULL ) {
        $$ = $1;
      } else {
        db_gen_item_connect( $1, $2 );
        $$ = $1;
      }
    }
  | generate_item
    {
      $$ = $1;
    }
  ;

  /* Similar to module_item but is more restrictive */
generate_item
  : module_item
    {
      $$ = db_get_curr_gen_block();
    }
  | K_begin generate_item_list_opt K_end
    {
      $$ = $2;
    }
  | K_begin ':' IDENTIFIER
    {
      generate_expr_mode++;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $3, @3.text, @3.first_line ) ) {
          ignore_mode++;
        } else {
          gen_item* gi = db_get_curr_gen_block();
          assert( gi != NULL );
          gitem_link_add( gi, &save_gi_head, &save_gi_tail );
        }
      } else {
        ignore_mode++;
      }
      generate_expr_mode--;
    }
    generate_item_list_opt K_end
    {
      if( $3 != NULL ) {
        db_end_function_task_namedblock( @6.first_line );
        free_safe( $3 );
      } else {
        if( ignore_mode > 0 ) {
          ignore_mode--;
        }
      }
      db_gen_item_connect_true( save_gi_tail->gi, $5 );
      $$ = save_gi_tail->gi;
      gitem_link_remove( save_gi_tail->gi, &save_gi_head, &save_gi_tail );
    }
  | K_for
    {
      generate_expr_mode++;
    }
    '(' generate_passign ';' static_expr ';' generate_passign ')' K_begin ':' IDENTIFIER
    {
      generate_for_mode++;
      if( (ignore_mode == 0) && ($12 != NULL) ) {
        if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $12, @12.text, @12.first_line ) ) {
          ignore_mode++;
        } else {
          gen_item* gi = db_get_curr_gen_block();
          assert( gi != NULL );
          gi->varname = generate_varname;
          generate_varname = NULL;
          gitem_link_add( gi, &save_gi_head, &save_gi_tail );
        }
      } else {
        ignore_mode++;
      }
      generate_expr_mode--;
    }
    generate_item_list_opt K_end
    {
      gen_item*   gi1;
      gen_item*   gi2;
      gen_item*   gi3;
      if( $12 != NULL ) {
        db_end_function_task_namedblock( @15.first_line );
      } else {
        if( ignore_mode > 0 ) {
          ignore_mode--;
        }
      }
      generate_for_mode--;
      generate_expr_mode++;
      if( (ignore_mode == 0) && ($4 != NULL) && ($6 != NULL) && ($8 != NULL) && ($14 != NULL) ) {
        block_depth++;
        /* Create first statement */
        db_add_expression( $4 );
        gi1 = db_get_curr_gen_block();
        /* Create second statement and attach it to the first statement */
        db_add_expression( db_create_expr_from_static( $6, @6.first_line, @6.first_column, (@6.last_column - 1)) );
        gi2 = db_get_curr_gen_block();
        db_gen_item_connect( gi1, gi2 );
        /* Add genvar to the new scope */
        /* Connect next_true of gi2 to the new scope */
        db_gen_item_connect_true( gi2, save_gi_tail->gi );
        /* Connect the generate block to the new scope */
        db_gen_item_connect_true( save_gi_tail->gi, $14 );
        /* Create third statement and attach it to the generate block */
        db_add_expression( $8 );
        gi3 = db_get_curr_gen_block();
        db_gen_item_connect_false( save_gi_tail->gi, gi3 );
        gitem_link_remove( save_gi_tail->gi, &save_gi_head, &save_gi_tail );
        gi2->suppl.part.conn_id = gi_conn_id;
        db_gen_item_connect( gi3, gi2 );
        block_depth--;
        $$ = gi1;
      } else {
        expression_dealloc( $4, FALSE );
        static_expr_dealloc( $6, TRUE );
        expression_dealloc( $8, TRUE );
        /* TBD - Deallocate generate block */
        $$ = NULL;
      }
      free_safe( $12 );
      generate_expr_mode--;
    }
  | K_if inc_gen_expr_mode '(' static_expr ')' dec_gen_expr_mode inc_block_depth generate_item dec_block_depth %prec less_than_K_else
    {
      expression* expr;
      gen_item*   gi1;
      if( (ignore_mode == 0) && ($4 != NULL) && ($8 != NULL) ) {
        generate_expr_mode++;
        expr = db_create_expr_from_static( $4, @4.first_line, @4.first_column, (@4.last_column - 1) );
        Try {
          expr = db_create_expression( expr, NULL, EXP_OP_IF, FALSE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          static_expr_dealloc( $4, TRUE );
          expression_dealloc( expr, FALSE );
          gen_item_dealloc( db_get_curr_gen_block(), TRUE );
          gen_item_dealloc( $8, TRUE );
          Throw 0;
        }
        db_add_expression( expr );
        gi1 = db_get_curr_gen_block();
        db_gen_item_connect_true( gi1, $8 );
        generate_expr_mode--;
        $$ = gi1;
      } else {
        static_expr_dealloc( $4, TRUE );
        /* TBD - Deallocate generate block */
        $$ = NULL;
      }
    }
  | K_if inc_gen_expr_mode '(' static_expr ')' dec_gen_expr_mode inc_block_depth generate_item dec_block_depth K_else generate_item
    {
      expression* expr;
      gen_item*   gi1;
      if( (ignore_mode == 0) && ($4 != NULL) && ($8 != NULL) && ($11 != NULL) ) {
        generate_expr_mode++;
        expr = db_create_expr_from_static( $4, @4.first_line, @4.first_column, (@4.last_column - 1) );
        Try {
          expr = db_create_expression( expr, NULL, EXP_OP_IF, FALSE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          static_expr_dealloc( $4, TRUE );
          expression_dealloc( expr, FALSE );
          gen_item_dealloc( db_get_curr_gen_block(), TRUE );
          gen_item_dealloc( $8, TRUE );
          gen_item_dealloc( $11, TRUE );
          Throw 0;
        }
        db_add_expression( expr );
        gi1 = db_get_curr_gen_block();
        db_gen_item_connect_true( gi1, $8 );
        db_gen_item_connect_false( gi1, $11 );
        generate_expr_mode--;
        $$ = gi1;
      } else {
        static_expr_dealloc( $4, TRUE );
        gen_item_dealloc( db_get_curr_gen_block(), TRUE );
        gen_item_dealloc( $8, TRUE );
        gen_item_dealloc( $11, TRUE );
        $$ = NULL;
      }
    }
  | K_case inc_gen_expr_mode '(' static_expr ')' inc_block_depth generate_case_items dec_block_depth dec_gen_expr_mode K_endcase
    { 
      expression* expr;
      expression* c_expr;
      gen_item*   stmt      = NULL;
      gen_item*   last_stmt = NULL;
      case_gitem* c_stmt    = $7;
      case_gitem* tc_stmt;
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        generate_expr_mode++;
        c_expr = db_create_expr_from_static( $4, @4.first_line, @4.first_column, (@4.last_column - 1) );
        while( c_stmt != NULL ) {
          Try {
            if( c_stmt->expr != NULL ) {
              expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASE, lhs_mode, c_stmt->line, 0, 0, NULL );
            } else {
              expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
            }
          } Catch_anonymous {
            static_expr_dealloc( $4, TRUE );
            expression_dealloc( c_expr, FALSE );
            gen_item_dealloc( db_get_curr_gen_block(), TRUE );
            while( c_stmt != NULL ) {
              gen_item_dealloc( c_stmt->gi, TRUE );
              expression_dealloc( c_stmt->expr, FALSE );
              tc_stmt = c_stmt;
              c_stmt  = c_stmt->prev;
              free_safe( tc_stmt );
            }
            Throw 0;
          }
          db_add_expression( expr );
          stmt = db_get_curr_gen_block();
          db_gen_item_connect_true( stmt, c_stmt->gi );
          db_gen_item_connect_false( stmt, last_stmt );
          if( stmt != NULL ) {
            last_stmt = stmt;
          }
          tc_stmt   = c_stmt;
          c_stmt    = c_stmt->prev;
          free_safe( tc_stmt );
        }
        if( stmt != NULL ) {
          stmt->elem.expr->suppl.part.owned = 1;
        }
        generate_expr_mode--;
        $$ = stmt;
      } else {
        static_expr_dealloc( $4, FALSE );
        while( c_stmt != NULL ) {
          expression_dealloc( c_stmt->expr, FALSE );
          // db_remove_statement( c_stmt->stmt );
          /* statement_dealloc_recursive( c_stmt->stmt ); */
          tc_stmt = c_stmt;
          c_stmt  = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = NULL;
      }
    }
  ;

generate_case_item
  : expression_list ':' dec_gen_expr_mode generate_item inc_gen_expr_mode
    { PROFILE(PARSER_GENERATE_CASE_ITEM_A);
      case_gitem* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_gitem*)malloc_safe( sizeof( case_gitem ) );
        cstmt->prev = NULL;
        cstmt->expr = $1;
        cstmt->gi = $4;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | K_default ':' dec_gen_expr_mode generate_item inc_gen_expr_mode
    { PROFILE(PARSER_GENERATE_CASE_ITEM_B);
      case_gitem* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_gitem*)malloc_safe( sizeof( case_gitem ) );
        cstmt->prev = NULL;
        cstmt->expr = NULL;
        cstmt->gi   = $4;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | K_default dec_gen_expr_mode generate_item inc_gen_expr_mode
    { PROFILE(PARSER_GENERATE_CASE_ITEM_C);
      case_gitem* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_gitem*)malloc_safe( sizeof( case_gitem ) );
        cstmt->prev = NULL;
        cstmt->expr = NULL;
        cstmt->gi   = $3;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | error ignore_more dec_gen_expr_mode ':' generate_item ignore_less inc_gen_expr_mode
    {
      VLerror( "Illegal generate case expression" );
    }
  ;

generate_case_items
  : generate_case_items generate_case_item
    {
      case_gitem* list = $1;
      case_gitem* curr = $2;
      if( ignore_mode == 0 ) {
        curr->prev = list;
        $$ = curr;
      } else {
        $$ = NULL;
      }
    }
  | generate_case_item
    {
      $$ = $1;
    }
  ;

  /* This is the start of a module body */
module_item_list_opt
  : module_item_list
  |
  ;

module_item_list
  : module_item_list module_item
  | module_item
  ;

module_item
  : attribute_list_opt
    net_type signed_opt range_opt list_of_variables ';'
  | attribute_list_opt
    net_type signed_opt range_opt net_decl_assigns ';'
  | attribute_list_opt
    net_type drive_strength
    {
      if( (ignore_mode == 0) && ($2 == 1) ) {
        parser_implicitly_set_curr_range( 0, 0, TRUE );
      }
      curr_signed = FALSE;
    }
    net_decl_assigns ';'
  | attribute_list_opt
    TYPEDEF_IDENTIFIER
    {
      curr_mba     = FALSE;
      curr_signed  = $2->is_signed;
      curr_handled = $2->is_handled;
      parser_copy_range_to_curr_range( $2->prange, TRUE );
      parser_copy_range_to_curr_range( $2->urange, FALSE );
    }
    register_variable_list ';'
  | attribute_list_opt port_type range_opt
    {
      curr_mba     = FALSE;
      curr_signed  = FALSE;
      curr_handled = TRUE;
      if( generate_top_mode > 0 ) {
        ignore_mode++;
        VLerror( "Port declaration not allowed within a generate block" );
      }
    }
    list_of_variables ';'
    {
      if( generate_top_mode > 0 ) {
        ignore_mode--;
      }
    }
  /* Handles Verilog-2001 port of type:  input wire [m:l] <list>; */
  | attribute_list_opt port_type net_type signed_opt range_opt
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "ANSI-C port declaration used in a block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      } else {
        curr_mba     = FALSE;
        curr_handled = TRUE;
        if( generate_top_mode > 0 ) {
          ignore_mode++;
          VLerror( "Port declaration not allowed within a generate block" );
        }
      }
    }
    list_of_variables ';'
    {
      if( !parser_check_generation( GENERATION_2001 ) || (generate_top_mode > 0) ) {
        ignore_mode--;
      }
    }
  | attribute_list_opt K_output var_type signed_opt range_opt
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "ANSI-C port declaration used in a block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      } else {
        curr_mba      = FALSE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_OUTPUT;
        if( generate_top_mode > 0 ) {
          ignore_mode++;
          VLerror( "Port declaration not allowed within a generate block" );
        }
      }
    }
    list_of_variables ';'
    {
      if( !parser_check_generation( GENERATION_2001 ) || (generate_top_mode > 0) ) {
        ignore_mode--;
      }
    }
  | attribute_list_opt port_type range_opt error ';'
    {
      if( generate_top_mode > 0 ) {
        VLerror( "Port declaration not allowed within a generate block" );
      } else {
        VLerror( "Invalid variable list in port declaration" );
      }
    }
  | attribute_list_opt K_trireg charge_strength_opt range_opt delay3_opt
    {
      curr_mba      = FALSE;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      if( generate_top_mode > 0 ) {
        ignore_mode++;
        VLerror( "Port declaration not allowed within a generate block" );
      }
    }
    list_of_variables ';'
    {
      if( generate_top_mode > 0 ) {
        ignore_mode--;
      }
    }
  | attribute_list_opt gatetype gate_instance_list ';'
    {
      str_link_delete_list( $3 );
    }
  | attribute_list_opt gatetype delay3 gate_instance_list ';'
    {
      str_link_delete_list( $4 );
    }
  | attribute_list_opt gatetype drive_strength gate_instance_list ';'
    {
      str_link_delete_list( $4 );
    }
  | attribute_list_opt gatetype drive_strength delay3 gate_instance_list ';'
    {
      str_link_delete_list( $5 );
    }
  | attribute_list_opt K_pullup gate_instance_list ';'
    {
      str_link_delete_list( $3 );
    }
  | attribute_list_opt K_pulldown gate_instance_list ';'
    {
      str_link_delete_list( $3 );
    }
  | attribute_list_opt K_pullup '(' dr_strength1 ')' gate_instance_list ';'
    {
      str_link_delete_list( $6 );
    }
  | attribute_list_opt K_pulldown '(' dr_strength0 ')' gate_instance_list ';'
    {
      str_link_delete_list( $6 );
    }
  | block_item_decl
  | attribute_list_opt K_defparam defparam_assign_list ';'
    {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Defparam found but not used, file: %s, line: %u.  Please use -P option to specify",
                                  obf_file( @1.text ), @1.first_line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
    }
  | attribute_list_opt K_event
    {
      curr_mba      = TRUE;
      curr_signed   = FALSE;
      curr_sig_type = SSUPPL_TYPE_EVENT;
      curr_handled  = TRUE;
      parser_implicitly_set_curr_range( 0, 0, TRUE );
    }
    list_of_variables ';'
  /* Handles instantiations of modules and user-defined primitives. */
  | attribute_list_opt IDENTIFIER parameter_value_opt gate_instance_list ';'
    {
      str_link*    tmp  = $4;
      str_link*    curr = tmp;
      param_oride* po;
      Try {
        while( curr != NULL ) {
          while( param_oride_head != NULL ){
            po               = param_oride_head;
            param_oride_head = po->next;
            db_add_override_param( curr->str, po->expr, po->name );
            if( po->name != NULL ) {
              free_safe( po->name );
            }
            free_safe( po );
          }
          (void)db_add_instance( curr->str, $2, FUNIT_MODULE, curr->range );
          curr = curr->next;
        }
      } Catch_anonymous {
        str_link_delete_list( tmp );
        while( param_oride_head != NULL ) {
          po = param_oride_head;
          param_oride_head = po->next;
          free_safe( po->name );
          free_safe( po );
        }
        free_safe( $2 );
        Throw 0;
      }
      str_link_delete_list( tmp );
      param_oride_head = NULL;
      param_oride_tail = NULL;
      free_safe( $2 );
    }
  | attribute_list_opt
    K_assign drive_strength_opt { ignore_mode++; } delay3_opt { ignore_mode--; } assign_list ';'
  | attribute_list_opt
    K_always statement
    {
      statement* stmt = $3;
      if( stmt != NULL ) {
        if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) ) {
          stmt->suppl.part.head = 1;
          db_add_statement( stmt, stmt );
        } else {
          db_remove_statement( stmt );
        }
      }
    }
  | attribute_list_opt
    K_always_comb
    {
      if( !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "always_comb syntax found in block that is specified to not allow SystemVerilog syntax" );
        ignore_mode++;
      }
    }
    statement
    {
      statement*  stmt;
      expression* slist;
      if( !parser_check_generation( GENERATION_SV ) ) {
        ignore_mode--;
      } else {
        if( $4 != NULL ) {
          if( (slist = db_create_sensitivity_list( $4 )) == NULL ) {
            VLerror( "Empty implicit event expression for the specified always_comb statement" );
          } else {
            Try {
              slist = db_create_expression( slist, NULL, EXP_OP_ALWAYS_COMB, lhs_mode, @2.first_line, @2.first_column, (@2.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( slist, FALSE );
              Throw 0;
            }
            stmt = db_create_statement( slist );
            if( !db_statement_connect( stmt, $4 ) ) {
              db_remove_statement( stmt );
              db_remove_statement( $4 );
            } else {
              if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) ) {
                stmt->suppl.part.head = 1;
                db_add_statement( stmt, stmt );
              } else {
                db_remove_statement( stmt );
              }
            }
          }
        }
      }
    }
  | attribute_list_opt
    K_always_latch
    {
      if( !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "always_latch syntax found in block that is specified to not allow SystemVerilog syntax" );
        ignore_mode++;
      }
    }
    statement
    {
      statement*  stmt;
      expression* slist;
      if( !parser_check_generation( GENERATION_SV ) ) {
        ignore_mode--;
      } else {
        if( $4 != NULL ) {
          if( (slist = db_create_sensitivity_list( $4 )) == NULL ) {
            VLerror( "Empty implicit event expression for the specified always_latch statement" );
          } else {
            Try {
              slist = db_create_expression( slist, NULL, EXP_OP_ALWAYS_LATCH, lhs_mode, @2.first_line, @2.first_column, (@2.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( slist, FALSE );
              Throw 0;
            }
            stmt  = db_create_statement( slist );
            if( !db_statement_connect( stmt, $4 ) ) {
              db_remove_statement( stmt );
              db_remove_statement( $4 );
            } else {
              if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) ) {
                stmt->suppl.part.head = 1;
                db_add_statement( stmt, stmt );
              } else {
                db_remove_statement( stmt );
              }
            }
          }
        }
      }
    }
  | attribute_list_opt
    K_always_ff
    {
      if( !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "always_ff syntax found in block that is specified to not allow SystemVerilog syntax" );
        ignore_mode++;
      }
    }
    statement
    {
      statement* stmt = $4;
      if( !parser_check_generation( GENERATION_SV ) ) {
        ignore_mode--;
      } else {
        if( stmt != NULL ) {
          if( db_statement_connect( stmt, stmt ) && (info_suppl.part.excl_always == 0) ) {
            stmt->suppl.part.head = 1;
            db_add_statement( stmt, stmt );
          } else {
            db_remove_statement( stmt );
          }
        }
      }
    }
  | attribute_list_opt
    K_initial statement
    {
      statement* stmt = $3;
      if( stmt != NULL ) {
        if( info_suppl.part.excl_init == 0 ) {
          stmt->suppl.part.head = 1;
          db_add_statement( stmt, stmt );
        } else {
          db_remove_statement( stmt );
        }
      }
    }
  | attribute_list_opt
    K_final statement
    {
      statement* stmt = $3;
      if( stmt != NULL ) {
        if( info_suppl.part.excl_final == 0 ) {
          stmt->suppl.part.head  = 1;
          stmt->suppl.part.final = 1;
          db_add_statement( stmt, stmt );
        } else {
          db_remove_statement( stmt );
        }
      }
    }
  | attribute_list_opt
    K_task automatic_opt IDENTIFIER ';'
    {
      if( generate_for_mode > 0 ) {
        VLerror( "Task definition not allowed within a generate loop" );
        ignore_mode++;
      }
      Try {
        if( ignore_mode == 0 ) {
          if( !db_add_function_task_namedblock( ($3 ? FUNIT_ATASK : FUNIT_TASK), $4, @4.text, @4.first_line ) ) {
            ignore_mode++;
          }
        }
      } Catch_anonymous {
        free_safe( $4 );
        Throw 0;
      }
      free_safe( $4 );
      generate_top_mode--;
    }
    task_item_list_opt statement_or_null
    {
      statement* stmt = $8;
      if( (ignore_mode == 0) && (stmt != NULL) ) {
        stmt->suppl.part.head      = 1;
        stmt->suppl.part.is_called = 1;
        db_add_statement( stmt, stmt );
      }
    }
    K_endtask
    {
      generate_top_mode++;
      if( ignore_mode == 0 ) {
        db_end_function_task_namedblock( @10.first_line );
      } else {
        ignore_mode--;
      }
    }
  | attribute_list_opt
    K_function automatic_opt signed_opt range_or_type_opt IDENTIFIER ';'
    {
      if( generate_for_mode > 0 ) {
        VLerror( "Function definition not allowed within a generate loop" );
        ignore_mode++;
      }
      if( ignore_mode == 0 ) {
        Try {
          if( db_add_function_task_namedblock( ($3 ? FUNIT_AFUNCTION : FUNIT_FUNCTION), $6, @6.text, @6.first_line ) ) {
            generate_top_mode--;
            db_add_signal( $6, SSUPPL_TYPE_IMPLICIT, &curr_prange, NULL, curr_signed, FALSE, @6.first_line, @6.first_column, TRUE );
            generate_top_mode++;
          } else {
            ignore_mode++;
          }
        } Catch_anonymous {
          free_safe( $6 );
          Throw 0;
        }
        free_safe( $6 );
      }
      generate_top_mode--;
    }
    function_item_list statement
    {
      statement* stmt = $10;
      if( (ignore_mode == 0) && (stmt != NULL) ) {
        stmt->suppl.part.head      = 1;
        stmt->suppl.part.is_called = 1;
        db_add_statement( stmt, stmt );
      }
    }
    K_endfunction
    {
      generate_top_mode++;
      if( ignore_mode == 0 ) {
        db_end_function_task_namedblock( @12.first_line );
      } else {
        ignore_mode--;
      }
    }
  | K_generate
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Generate syntax found in block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      } else {
        if( generate_mode == 0 ) {
          generate_mode = 1;
          generate_top_mode = 1;
        } else {
          VLerror( "Found generate keyword inside of a generate block" );
        }
      }
    }
    generate_item_list_opt
    K_endgenerate
    {
      generate_mode = 0;
      generate_top_mode = 0;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        ignore_mode--;
      } else {
        db_add_gen_item_block( $3 );
      }
    }
  | K_genvar
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Genvar syntax found in block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      } else {
        curr_signed   = FALSE;
        curr_mba      = TRUE;
        curr_handled  = TRUE;
        curr_sig_type = SSUPPL_TYPE_GENVAR;
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
    list_of_variables ';'
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        ignore_mode--;
      }
    }
  | attribute_list_opt
    K_specify
    {
      if( generate_top_mode > 0 ) {
        VLerror( "Specify block not allowed within a generate block" );
      }
    }
    ignore_more specify_item_list ignore_less K_endspecify
  | attribute_list_opt
    K_specify
    {
      if( generate_top_mode > 0 ) {
        VLerror( "Specify block not allowed within a generate block" );
      }
    }
    K_endspecify
  | attribute_list_opt
    K_specify
    {
      if( generate_top_mode > 0 ) {
        VLerror( "Specify block not allowed within a generate block" );
      }
    }
    error K_endspecify
  | error ';'
    {
      VLerror( "Invalid module item.  Did you forget an initial or always?" );
    }
  | attribute_list_opt
    K_assign error '=' { ignore_mode++; } expression { ignore_mode--; } ';'
    {
      VLerror( "Syntax error in left side of continuous assignment" );
    }
  | attribute_list_opt
    K_assign error ';'
    {
      VLerror( "Syntax error in continuous assignment" );
    }
  | attribute_list_opt
    K_function error K_endfunction
    {
      if( generate_for_mode > 0 ) {
        VLerror( "Function definition not allowed within generate block" );
      }
      VLerror( "Syntax error in function description" );
    }
  | attribute_list_opt
    enumeration list_of_names ';'
    {
      str_link* strl = $3;
      while( strl != NULL ) {
        db_add_signal( strl->str, SSUPPL_TYPE_DECLARED, &curr_prange, NULL, curr_signed, FALSE, strl->suppl, strl->suppl2, TRUE );
        strl = strl->next;
      }
      str_link_delete_list( $3 );
    }
  | attribute_list_opt
    typedef_decl
  /* SystemVerilog assertion - we don't currently support these and I don't want to worry about how to parse them either */
  | attribute_list_opt
    IDENTIFIER ':' K_assert ';'
    {
      free_safe( $2 );
    }
  /* SystemVerilog property - we don't currently support these but crudely parse them */
  | attribute_list_opt
    K_property K_endproperty

  /* SystemVerilog sequence - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_sequence K_endsequence
  /* SystemVerilog program block - we don't currently support these but crudely will parse them */
  | attribute_list_opt
    K_program K_endprogram
  | KK_attribute '(' { ignore_mode++; } UNUSED_IDENTIFIER ',' UNUSED_STRING ',' UNUSED_STRING { ignore_mode--; }')' ';'
  | KK_attribute '(' error ')' ';'
    {
      VLerror( "Syntax error in $attribute parameter list" );
    }
  ;

for_initialization
  : expression_assignment_list
    {
      $$ = $1;
    }
  ;

  /* TBD - In the SystemVerilog BNF, there are more options available for this rule -- but we don't current support them */
data_type
  : integer_vector_type signed_opt range_opt
  | integer_atom_type signed_opt
  | struct_union '{' struct_union_member_list '}' range_opt
    {
    }
  | struct_union K_packed signed_opt '{' struct_union_member_list '}' range_opt
    {
    }
  | K_event
    {
      curr_mba      = TRUE;
      curr_signed   = FALSE;
      curr_sig_type = SSUPPL_TYPE_EVENT;
      curr_handled  = TRUE;
      parser_implicitly_set_curr_range( 0, 0, TRUE );
    }
  ;

data_type_opt
  : data_type
    {
      $$ = 1;
    }
  |
    {
      $$ = 0;
    }
  ;

  /* TBD - Not sure what this should return at this point */
struct_union
  : K_struct
  | K_union
  | K_union K_tagged
  ;

struct_union_member_list
  : attribute_list_opt data_type_or_void list_of_variables ';'
    {
    }
  | struct_union_member_list ',' attribute_list_opt data_type_or_void list_of_variables ';'
    {
    }
  |
    {
    }
  ;

data_type_or_void
  : data_type
  | K_void 
    {
    }
  ;

integer_vector_type
  : K_bit
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
    }
  | K_logic
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
    }
  | K_reg
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
    }
  ;

integer_atom_type
  : K_byte
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 7, 0, TRUE );
    }
  | K_char
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 7, 0, TRUE );
    }
  | K_shortint
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 15, 0, TRUE );
    }
  | K_int
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 31, 0, TRUE );
    }
  | K_integer
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 31, 0, TRUE );
    }
  | K_longint
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 63, 0, TRUE );
    }
  | K_time
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 63, 0, TRUE );
    }
  ;

expression_assignment_list
  : data_type_opt IDENTIFIER '=' expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      char*       unnamed;
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        Try {
          if( ($1 == 1) && !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "Variables declared in FOR initialization block that is specified to not allow SystemVerilog syntax" );
          } else if( ($1 == 1) || (db_find_signal( $2, TRUE ) == NULL) ) {
            db_add_signal( $2, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @2.first_line, @2.first_column, TRUE );
          }
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @2.first_line, @2.first_column, (@2.last_column - 1), $2 );
        } Catch_anonymous {
          expression_dealloc( $4, FALSE );
          free_safe( $2 );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( $4, tmp, EXP_OP_BASSIGN, FALSE, @2.first_line, @2.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $4, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
      free_safe( $2 );
    }
  | data_type_opt UNUSED_IDENTIFIER '=' expression
    {
      expression_dealloc( $4, FALSE );
      $$ = NULL;
    }
  | expression_assignment_list ',' data_type_opt IDENTIFIER '=' expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($6 != NULL) ) {
        Try {
          if( ($3 == 1) && !parser_check_generation( GENERATION_SV ) ) {
            VLerror( "Variables declared in FOR initialization block that is specified to not allow SystemVerilog syntax" );
          } else if( ($3 == 1) || (db_find_signal( $4, TRUE ) == NULL) ) {
            db_add_signal( $4, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @4.first_line, @4.first_column, TRUE );
          }
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @4.first_line, @4.first_column, (@4.last_column - 1), $4 );
        } Catch_anonymous {
          expression_dealloc( $6, FALSE );
          db_remove_statement( $1 );
          free_safe( $4 );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( $6, tmp, EXP_OP_BASSIGN, FALSE, @4.first_line, @4.first_column, (@6.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $6, FALSE );
          db_remove_statement( $1 );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        if( !db_statement_connect( $1, stmt ) ) {
          db_remove_statement( stmt );
        }
      } else {
        expression_dealloc( $6, FALSE );
      }
      free_safe( $4 );
      $$ = $1;
    }
  | expression_assignment_list ',' data_type_opt UNUSED_IDENTIFIER '=' expression
    {
      db_remove_statement( $1 );      
      expression_dealloc( $6, FALSE );
      $$ = NULL;
    }
  ;

passign
  : lpvalue '=' expression
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_ADD_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "+= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_ADD, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_SUB_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "-= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_SUBTRACT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_MLT_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "*= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_MULTIPLY, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_DIV_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "/= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_DIVIDE, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_MOD_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "%= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_MOD, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_AND_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "&= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_AND, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_OR_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "|= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_OR, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_XOR_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "^= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_XOR, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_LS_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "<<= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_LSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_RS_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( ">>= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_RSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_ALS_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "<<<= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_ALSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_ARS_A expression
    {
      expression* tmp = NULL;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( ">>>= operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_LAST, FALSE, @3.first_line, @3.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( $3, tmp, EXP_OP_ARSHIFT, FALSE, @2.first_line, @2.first_column, (@3.last_column - 1), NULL );
            tmp = db_create_expression( tmp, $1, EXP_OP_BASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_INC
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "Increment (++) operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, $1, EXP_OP_PINC, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue K_DEC
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        if( !parser_check_generation( GENERATION_SV ) ) {
          VLerror( "Decrement (--) operation found in block that is specified to not allow SystemVerilog syntax" );
          expression_dealloc( $1, FALSE );
          $$ = NULL;
        } else {
          Try {
            tmp = db_create_expression( NULL, $1, EXP_OP_PDEC, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          $$ = stmt;
        }
      } else {
        expression_dealloc( $1, FALSE );
        $$ = NULL;
      }
    }
  ;

statement
  : K_assign { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_deassign { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_force { ignore_mode++; } lavalue '=' expression ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
  | K_release { ignore_mode++; } lavalue ';' { ignore_mode--; }
    {
      $$ = NULL;
    }
/*
  | K_begin inc_block_depth statement_list dec_block_depth K_end
    {
      statement* stmt = db_parallelize_statement( $3 );
      $$ = stmt;
    }
*/
  | K_begin inc_block_depth begin_end_block dec_block_depth K_end
    { PROFILE(PARSER_STATEMENT_BEGIN_A);
      expression* exp;
      statement*  stmt;
      char        back[4096];
      char        rest[4096];
      if( ignore_mode == 0 ) {
        db_end_function_task_namedblock( @5.first_line );
        if( $3 != NULL ) {
          scope_extract_back( $3->name, back, rest );
          exp  = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          exp->elem.funit      = $3;
          exp->suppl.part.type = ETYPE_FUNIT;
          exp->name            = strdup_safe( back );
          stmt = db_create_statement( exp );
          $$   = stmt;
        } else {
          if( ignore_mode > 0 ) {
            ignore_mode--;
          }
          $$ = NULL;
        }
      } else {
        if( ignore_mode > 0 ) {
          ignore_mode--;
        }
        $$ = NULL;
      }
    }
/*
  | K_begin inc_block_depth dec_block_depth K_end
    {
      $$ = NULL;
    }
  | K_begin inc_block_depth error dec_block_depth K_end
    {
      VLerror( "Illegal syntax in begin/end block" );
      $$ = NULL;
    }
*/
  | K_fork inc_fork_depth fork_statement K_join
    { PROFILE(PARSER_STATEMENT_FORK_A);
      expression* exp;
      statement*  stmt;
      char        back[4096];
      char        rest[4096];
      if( ignore_mode == 0 ) {
        db_end_function_task_namedblock( @4.first_line );
        if( $3 != NULL ) {
          scope_extract_back( $3->name, back, rest );
          exp  = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          exp->elem.funit      = $3;
          exp->suppl.part.type = ETYPE_FUNIT;
          exp->name            = strdup_safe( back );
          stmt = db_create_statement( exp );
          $$   = stmt;
        } else {
          ignore_mode--;
          $$ = NULL;
        }
      } else {
        ignore_mode--;
        $$ = NULL;
      }
    }
  | K_disable identifier ';'
    {
      expression* exp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          exp = db_create_expression( NULL, NULL, EXP_OP_DISABLE, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), $2 );
        } Catch_anonymous {
          free_safe( $2 );
          Throw 0;
        }
        stmt = db_create_statement( exp );
        free_safe( $2 );
        $$ = stmt;
      } else {
        if( $2 != NULL ) {
          free_safe( $2 );
        }
        $$ = NULL;
      } 
    }
  | K_TRIGGER IDENTIFIER ';'
    {
      expression* expr;
      statement*  stmt;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          expr = db_create_expression( NULL, NULL, EXP_OP_TRIGGER, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), $2 );
        } Catch_anonymous {
          free_safe( $2 );
          Throw 0;
        }
        stmt = db_create_statement( expr );
        free_safe( $2 );
        $$ = stmt;
      } else {
        if( $2 != NULL ) {
          free_safe( $2 );
        }
        $$ = NULL;
      }
    }
  | K_forever inc_block_depth statement dec_block_depth
    {
      if( $3 != NULL ) {
        if( db_statement_connect( $3, $3 ) ) {
          $$ = $3;
        } else {
          db_remove_statement( $3 );
          $$ = NULL;
        }
      }
    }
  | K_repeat '(' expression ')' inc_block_depth statement dec_block_depth
    {
      vector*     vec;
      expression* expr;
      statement*  stmt;
      if( (ignore_mode == 0) && ($3 != NULL) && ($6 != NULL) ) {
        vec = vector_create( 32, VTYPE_VAL, TRUE );
        Try {
          expr = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          vector_from_int( vec, 0x0 );
          assert( expr->value->value == NULL );
          free_safe( expr->value );
          expr->value = vec;
        } Catch_anonymous {
          vector_dealloc( vec );
          expression_dealloc( $3, FALSE );
          db_remove_statement( $6 );
          Throw 0;
        }
        Try {
          expr = db_create_expression( $3, expr, EXP_OP_REPEAT, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          expression_dealloc( expr, FALSE );
          db_remove_statement( $6 );
          Throw 0;
        }
        stmt = db_create_statement( expr );
        db_connect_statement_true( stmt, $6 );
        stmt->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for all statements connecting to stmt */
        assert( db_statement_connect( $6, stmt ) );
        $$ = stmt;
      } else {
        expression_dealloc( $3, FALSE );
        db_remove_statement( $6 );
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_case '(' expression ')' inc_block_depth case_items dec_block_depth K_endcase
    {
      expression*     expr;
      expression*     c_expr    = $4;
      statement*      stmt      = NULL;
      statement*      last_stmt = NULL;
      case_statement* c_stmt    = $7;
      case_statement* tc_stmt;
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        while( c_stmt != NULL ) {
          Try {
            if( c_stmt->expr != NULL ) {
              expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASE, lhs_mode, c_stmt->line, 0, 0, NULL );
            } else {
              expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
            }
          } Catch_anonymous {
            expression_dealloc( $4, FALSE );
            while( c_stmt != NULL ) {
              expression_dealloc( c_stmt->expr, FALSE );
              db_remove_statement( c_stmt->stmt );
              tc_stmt = c_stmt;
              c_stmt  = c_stmt->prev;
              free_safe( tc_stmt );
            }
            Throw 0;
          }
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, c_stmt->stmt );
          db_connect_statement_false( stmt, last_stmt );
          if( stmt != NULL ) {
            last_stmt = stmt;
          }
          tc_stmt   = c_stmt;
          c_stmt    = c_stmt->prev;
          free_safe( tc_stmt );
        }
        if( stmt != NULL ) {
          stmt->exp->suppl.part.owned = 1;
        }
        $$ = stmt;
      } else {
        expression_dealloc( $4, FALSE );
        while( c_stmt != NULL ) {
          expression_dealloc( c_stmt->expr, FALSE );
          db_remove_statement( c_stmt->stmt );
          tc_stmt = c_stmt;
          c_stmt  = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_casex '(' expression ')' inc_block_depth case_items dec_block_depth K_endcase
    {
      expression*     expr;
      expression*     c_expr    = $4;
      statement*      stmt      = NULL;
      statement*      last_stmt = NULL;
      case_statement* c_stmt    = $7;
      case_statement* tc_stmt;
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        while( c_stmt != NULL ) {
          Try {
            if( c_stmt->expr != NULL ) {
              expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEX, lhs_mode, c_stmt->line, 0, 0, NULL );
            } else {
              expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
            }
          } Catch_anonymous {
            expression_dealloc( $4, FALSE );
            while( c_stmt != NULL ) {
              expression_dealloc( c_stmt->expr, FALSE );
              db_remove_statement( c_stmt->stmt );
              tc_stmt = c_stmt;
              c_stmt  = c_stmt->prev;
              free_safe( tc_stmt );
            }
            Throw 0;
          }
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, c_stmt->stmt );
          db_connect_statement_false( stmt, last_stmt );
          if( stmt != NULL ) {
            last_stmt = stmt;
          }
          tc_stmt   = c_stmt;
          c_stmt    = c_stmt->prev;
          free_safe( tc_stmt );
        }
        if( stmt != NULL ) {
          stmt->exp->suppl.part.owned = 1;
        }
        $$ = stmt;
      } else {
        expression_dealloc( $4, FALSE );
        while( c_stmt != NULL ) {
          expression_dealloc( c_stmt->expr, FALSE );
          db_remove_statement( c_stmt->stmt );
          tc_stmt = c_stmt;
          c_stmt  = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_casez '(' expression ')' inc_block_depth case_items dec_block_depth K_endcase
    {
      expression*     expr;
      expression*     c_expr    = $4;
      statement*      stmt      = NULL;
      statement*      last_stmt = NULL;
      case_statement* c_stmt    = $7;
      case_statement* tc_stmt;
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        while( c_stmt != NULL ) {
          Try {
            if( c_stmt->expr != NULL ) {
              expr = db_create_expression( c_stmt->expr, c_expr, EXP_OP_CASEZ, lhs_mode, c_stmt->line, 0, 0, NULL );
            } else {
              expr = db_create_expression( NULL, NULL, EXP_OP_DEFAULT, lhs_mode, c_stmt->line, 0, 0, NULL );
            }
          } Catch_anonymous {
            expression_dealloc( $4, FALSE );
            while( c_stmt != NULL ) {
              expression_dealloc( c_stmt->expr, FALSE );
              db_remove_statement( c_stmt->stmt );
              tc_stmt = c_stmt;
              c_stmt  = c_stmt->prev;
              free_safe( tc_stmt );
            }
            Throw 0;
          }
          stmt = db_create_statement( expr );
          db_connect_statement_true( stmt, c_stmt->stmt );
          db_connect_statement_false( stmt, last_stmt );
          if( stmt != NULL ) {
            last_stmt = stmt;
          }
          tc_stmt   = c_stmt;
          c_stmt    = c_stmt->prev;
          free_safe( tc_stmt );
        }
        if( stmt != NULL ) {
          stmt->exp->suppl.part.owned = 1;
        }
        $$ = stmt;
      } else {
        expression_dealloc( $4, FALSE );
        while( c_stmt != NULL ) {
          expression_dealloc( c_stmt->expr, FALSE );
          db_remove_statement( c_stmt->stmt );
          tc_stmt = c_stmt;
          c_stmt  = c_stmt->prev;
          free_safe( tc_stmt );
        }
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_case '(' expression ')' inc_block_depth error dec_block_depth K_endcase
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $4, FALSE );
      }
      VLerror( "Illegal case expression" );
      $$ = NULL;
    }
  | cond_specifier_opt K_casex '(' expression ')' inc_block_depth error dec_block_depth K_endcase
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $4, FALSE );
      }
      VLerror( "Illegal casex expression" );
      $$ = NULL;
    }
  | cond_specifier_opt K_casez '(' expression ')' inc_block_depth error dec_block_depth K_endcase
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $4, FALSE );
      }
      VLerror( "Illegal casez expression" );
      $$ = NULL;
    }
  | cond_specifier_opt K_if '(' expression ')' inc_block_depth statement_or_null dec_block_depth %prec less_than_K_else
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        Try {
          tmp = db_create_expression( $4, NULL, EXP_OP_IF, FALSE, @2.first_line, @2.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $4, FALSE );
          db_remove_statement( $7 );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        db_connect_statement_true( stmt, $7 );
        $$ = stmt;
      } else {
        db_remove_statement( $7 );
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_if '(' expression ')' inc_block_depth statement_or_null dec_block_depth K_else statement_or_null
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($4 != NULL) ) {
        Try {
          tmp = db_create_expression( $4, NULL, EXP_OP_IF, FALSE, @2.first_line, @2.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $4, FALSE );
          db_remove_statement( $7 );
          db_remove_statement( $10 );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        db_connect_statement_true( stmt, $7 );
        db_connect_statement_false( stmt, $10 );
        $$ = stmt;
      } else {
        db_remove_statement( $7 );
        db_remove_statement( $10 );
        $$ = NULL;
      }
    }
  | cond_specifier_opt K_if '(' error ')' { ignore_mode++; } if_statement_error { ignore_mode--; }
    {
      VLerror( "Illegal conditional if expression" );
      $$ = NULL;
    }
  | K_for inc_for_depth '(' for_initialization ';' expression ';' passign ')' statement dec_for_depth
    { PROFILE(PARSER_STATEMENT_FOR_A);
      expression* exp;
      statement*  stmt;
      statement*  stmt1 = $4;
      statement*  stmt2;
      statement*  stmt3 = $8;
      statement*  stmt4 = $10;
      char        back[4096];
      char        rest[4096];
      if( (ignore_mode == 0) && ($4 != NULL) && ($6 != NULL) && ($8 != NULL) && ($10 != NULL) ) {
        block_depth++;
        stmt2 = db_create_statement( $6 );
        assert( db_statement_connect( stmt1, stmt2 ) );
        db_connect_statement_true( stmt2, stmt4 );
        assert( db_statement_connect( stmt4, stmt3 ) );
        stmt2->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for the stmt3 */
        assert( db_statement_connect( stmt3, stmt2 ) );
        block_depth--;
        stmt1 = db_parallelize_statement( stmt1 );
        stmt1->suppl.part.head      = 1;
        stmt1->suppl.part.is_called = 1;
        db_add_statement( stmt1, stmt1 );
      } else {
        db_remove_statement( stmt1 );
        expression_dealloc( $6, FALSE );
        db_remove_statement( stmt3 );
        db_remove_statement( stmt4 );
        stmt1 = NULL;
      }
      db_end_function_task_namedblock( @11.first_line );
      if( (stmt1 != NULL) && ($2 != NULL) ) {
        scope_extract_back( $2->name, back, rest );
        exp = db_create_expression( NULL, NULL, EXP_OP_NB_CALL, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        exp->elem.funit      = $2;
        exp->suppl.part.type = ETYPE_FUNIT;
        exp->name            = strdup_safe( back );
        stmt = db_create_statement( exp );
        $$ = stmt;
      } else {
        $$ = NULL;
      }
    }
  | K_for inc_for_depth '(' for_initialization ';' expression ';' error ')' statement dec_for_depth
    {
      db_remove_statement( $4 );
      expression_dealloc( $6, FALSE );
      db_remove_statement( $10 );
      db_end_function_task_namedblock( @11.first_line );
      $$ = NULL;
    }
  | K_for inc_for_depth '(' for_initialization ';' error ';' passign ')' statement dec_for_depth
    {
      db_remove_statement( $4 );
      db_remove_statement( $8 );
      db_remove_statement( $10 );
      db_end_function_task_namedblock( @11.first_line );
      $$ = NULL;
    }
  | K_for inc_for_depth '(' error ')' statement dec_for_depth
    {
      db_remove_statement( $6 );
      db_end_function_task_namedblock( @7.first_line );
      $$ = NULL;
    }
  | K_while '(' expression ')' inc_block_depth statement dec_block_depth
    {
      expression* expr;
      statement*  stmt;
      if( (ignore_mode == 0) && ($3 != NULL) && ($6 != NULL) ) {
        Try {
          expr = db_create_expression( $3, NULL, EXP_OP_WHILE, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          db_remove_statement( $6 );
          Throw 0;
        }
        stmt = db_create_statement( expr );
        db_connect_statement_true( stmt, $6 );
        stmt->conn_id = stmt_conn_id;   /* This will cause the STOP bit to be set for all statements connecting to stmt */
        assert( db_statement_connect( $6, stmt ) );
        $$ = stmt;
      } else {
        expression_dealloc( $3, FALSE );
        db_remove_statement( $6 );
        $$ = NULL;
      }
    }
  | K_while '(' error ')' inc_block_depth statement dec_block_depth
    {
      db_remove_statement( $6 );
      $$ = NULL;
    }
  | K_do inc_block_depth statement dec_block_depth K_while '(' expression ')' ';'
    {
      expression* expr;
      statement*  stmt;
      if( (ignore_mode == 0) && ($3 != NULL) && ($7 != NULL) ) {
        Try {
          expr = db_create_expression( $7, NULL, EXP_OP_WHILE, FALSE, @5.first_line, @5.first_column, (@8.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $7, FALSE );
          db_remove_statement( $3 );
          Throw 0;
        }
        stmt = db_create_statement( expr );
        assert( db_statement_connect( $3, stmt ) );
        db_connect_statement_true( stmt, $3 );
        stmt->suppl.part.stop_true = 1;  /* Set STOP bit for the TRUE path */
        $$ = $3;
      } else {
        expression_dealloc( $7, FALSE );
        db_remove_statement( $3 );
        $$ = NULL;
      }
    }
  | delay1 inc_block_depth statement_or_null dec_block_depth
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        stmt = db_create_statement( $1 );
        if( $3 != NULL ) {
          if( !db_statement_connect( stmt, $3 ) ) {
            db_remove_statement( stmt );
            db_remove_statement( $3 );
            stmt = NULL;
          }
        }
        $$ = stmt;
      } else {
        db_remove_statement( $3 );
        $$ = NULL;
      }
    }
  | event_control inc_block_depth statement_or_null dec_block_depth
    {
      statement* stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        stmt = db_create_statement( $1 );
        if( $3 != NULL ) {
          if( !db_statement_connect( stmt, $3 ) ) {
            db_remove_statement( stmt );
            db_remove_statement( $3 );
            stmt = NULL;
          }
        }
        $$ = stmt;
      } else {
        db_remove_statement( $3 );
        $$ = NULL;
      }
    }
  | '@' '*' inc_block_depth statement_or_null dec_block_depth
    {
      expression* expr;
      statement*  stmt;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Wildcard combinational logic sensitivity list found in block that is specified to not allow Verilog-2001 syntax" );
        db_remove_statement( $4 );
        $$ = NULL;
      } else {
        if( (ignore_mode == 0) && ($4 != NULL) ) {
          if( (expr = db_create_sensitivity_list( $4 )) == NULL ) {
            VLerror( "Empty implicit event expression for the specified statement" );
            db_remove_statement( $4 );
            $$ = NULL;
          } else {
            Try {
              expr = db_create_expression( expr, NULL, EXP_OP_SLIST, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL ); 
            } Catch_anonymous {
              expression_dealloc( expr, FALSE );
              db_remove_statement( $4 );
              Throw 0;
            }
            stmt = db_create_statement( expr );
            if( !db_statement_connect( stmt, $4 ) ) {
              db_remove_statement( stmt );
              db_remove_statement( $4 );
              stmt = NULL;
            }
            $$ = stmt;
          }
        } else {
          db_remove_statement( $4 );
          $$ = NULL;
        }
      }
    }
  | passign ';'
    {
      $$ = $1;
    }
  | lpvalue K_LE expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue '=' delay1 expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
        Try {
          tmp = db_create_expression( $4, $3, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $1, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE delay1 expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
        Try {
          tmp = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        expression_dealloc( $3, FALSE );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue '=' event_control expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
        Try {
          tmp = db_create_expression( $4, $3, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $1, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE event_control expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) && ($4 != NULL) ) {
        Try {
          tmp = db_create_expression( $4, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          expression_dealloc( $4, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        expression_dealloc( $3, FALSE );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  | lpvalue '=' K_repeat '(' expression ')' event_control expression ';'
    {
      vector*     vec;
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($5 != NULL) && ($7 != NULL) && ($8 != NULL) ) {
        vec = vector_create( 32, VTYPE_VAL, TRUE );
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          vector_from_int( vec, 0x0 );
          assert( tmp->value->value == NULL );
          free_safe( tmp->value );
          tmp->value = vec;
        } Catch_anonymous {
          vector_dealloc( vec );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $5, FALSE );
          expression_dealloc( $7, FALSE );
          expression_dealloc( $8, FALSE );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( $5, tmp, EXP_OP_REPEAT, FALSE, @3.first_line, @3.first_column, (@6.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $5, FALSE );
          expression_dealloc( $7, FALSE );
          expression_dealloc( $8, FALSE );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( $7, tmp, EXP_OP_RPT_DLY, FALSE, @3.first_line, @3.first_column, (@7.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $7, FALSE );
          expression_dealloc( $8, FALSE );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( $8, tmp, EXP_OP_DLY_OP, FALSE, @3.first_line, @3.first_column, (@8.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $1, FALSE );
          expression_dealloc( $8, FALSE );
          Throw 0;
        }
        Try {
          tmp = db_create_expression( tmp, $1, EXP_OP_DLY_ASSIGN, FALSE, @1.first_line, @1.first_column, (@8.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp, FALSE );
          expression_dealloc( $1, FALSE );
        }
        stmt = db_create_statement( tmp );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $5, FALSE );
        expression_dealloc( $7, FALSE );
        expression_dealloc( $8, FALSE );
        $$ = NULL;
      }
    }
    /* We don't handle the non-blocking assignments ourselves, so we can just ignore the delay here */
  | lpvalue K_LE K_repeat '(' expression ')' event_control expression ';'
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($8 != NULL) ) {
        Try {
          tmp = db_create_expression( $8, $1, EXP_OP_NASSIGN, FALSE, @1.first_line, @1.first_column, (@8.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $5, FALSE );
          expression_dealloc( $7, FALSE );
          expression_dealloc( $8, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        expression_dealloc( $5, FALSE );
        expression_dealloc( $7, FALSE );
        $$ = stmt;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $5, FALSE );
        expression_dealloc( $7, FALSE );
        expression_dealloc( $8, FALSE );
        $$ = NULL;
      }
    }
  | K_wait '(' expression ')' inc_block_depth statement_or_null dec_block_depth
    {
      expression* exp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        Try {
          exp = db_create_expression( $3, NULL, EXP_OP_WAIT, FALSE, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          db_remove_statement( $6 );
          Throw 0;
        }
        stmt = db_create_statement( exp );
        if( $6 != NULL ) {
          if( !db_statement_connect( stmt, $6 ) ) {
            db_remove_statement( stmt );
            db_remove_statement( $6 );
            stmt = NULL;
          }
        }
        $$ = stmt;
      } else {
        db_remove_statement( $6 );
        $$ = NULL;
      }
    }
  | SYSTEM_IDENTIFIER { ignore_mode++; } '(' expression_port_list ')' ';' { ignore_mode--; }
    {
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          if( strncmp( $1, "$display", 8 ) == 0 ) {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL ) );
          } else if( strncmp( $1, "$finish", 7 ) == 0 ) {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SFINISH, lhs_mode, 0, 0, 0, NULL ) );
          } else if( strncmp( $1, "$stop", 5 ) == 0 ) {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SSTOP, lhs_mode, 0, 0, 0, NULL ) );
          } else {
            $$ = NULL;
          }
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
        free_safe( $1 );
      } else {
        if( $1 != NULL ) {
          free_safe( $1 );
        }
        $$ = NULL;
      }
    }
  | UNUSED_SYSTEM_IDENTIFIER '(' expression_port_list ')' ';'
    {
      $$ = NULL;
    }
  | SYSTEM_IDENTIFIER ';'
    {
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          if( strncmp( $1, "$display", 8 ) == 0 ) {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL ) );
          } else if( strncmp( $1, "$finish", 7 ) == 0 ) {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SFINISH, lhs_mode, 0, 0, 0, NULL ) );
          } else if( strncmp( $1, "$stop", 7 ) == 0 ) {
            $$ = db_create_statement( db_create_expression( NULL, NULL, EXP_OP_SSTOP, lhs_mode, 0, 0, 0, NULL ) );
          } else {
            $$  = NULL;
          }
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
        free_safe( $1 );
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | UNUSED_SYSTEM_IDENTIFIER ';'
    {
      $$ = NULL;
    }
  | identifier '(' expression_port_list ')' ';'
    {
      expression* exp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          exp = db_create_expression( NULL, $3, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.first_column, (@5.last_column - 1), $1 );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          free_safe( $1 );
          Throw 0;
        }
        stmt = db_create_statement( exp );
        $$   = stmt;
        free_safe( $1 );
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | identifier ';'
    {
      expression* exp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          exp = db_create_expression( NULL, NULL, EXP_OP_TASK_CALL, FALSE, @1.first_line, @1.first_column, (@2.last_column - 1), $1 );
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
        stmt = db_create_statement( exp );
        $$   = stmt;
        free_safe( $1 );
      } else {
        free_safe( $1 );
        $$ = NULL;
      }
    }
   /* Immediate SystemVerilog assertions are parsed but not performed -- we will not exclude a block that contains one */
  | K_assert ';'
    {
      expression* exp;
      statement*  stmt;
      if( ignore_mode == 0 ) {
        exp  = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL );
        stmt = db_create_statement( exp );
        $$   = stmt;
      }
    }
   /* Inline SystemVerilog assertion -- parsed, not performed and we will not exclude a block that contains one */
  | IDENTIFIER ':' K_assert ';'
    {
      expression* exp;
      statement*  stmt;
      if( ignore_mode == 0 ) {
        Try {
          exp = db_create_expression( NULL, NULL, EXP_OP_NOOP, lhs_mode, 0, 0, 0, NULL );
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
        stmt = db_create_statement( exp );
        $$   = stmt;
      }
      free_safe( $1 );
    }
  | error ';'
    {
      VLerror( "Illegal statement" );
      $$ = NULL;
    }
  ;

fork_statement
  : begin_end_id
    {
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $1, @1.text, @1.first_line ) ) {
          ignore_mode++;
        }
      } else {
        ignore_mode++;
      }
    }
    block_item_decls_opt statement_list dec_fork_depth
    {
      expression* expr;
      statement*  stmt;
      if( $3 && db_is_unnamed_scope( $1 ) && !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Net/variables declared in unnamed fork...join block that is specified to not allow SystemVerilog syntax" );
        ignore_mode++;
      }
      if( ignore_mode == 0 ) {
        if( $4 != NULL ) {
          Try {
            expr = db_create_expression( NULL, NULL, EXP_OP_JOIN, FALSE, @4.first_line, @4.first_column, (@4.last_column - 1), NULL );
          } Catch_anonymous {
            db_remove_statement( $4 );
            free_safe( $1 );
            Throw 0;
          }
          stmt = db_create_statement( expr );
          if( db_statement_connect( $4, stmt ) ) {
            stmt = $4;
            stmt->suppl.part.head      = 1;
            stmt->suppl.part.is_called = 1;
            db_add_statement( stmt, stmt );
            $$ = db_get_curr_funit();
          } else {
            db_remove_statement( $4 );
            db_remove_statement( stmt );
            $$ = NULL;
          }
        } else {
          $$ = NULL;
        }
        free_safe( $1 );
      } else {
        if( $3 && db_is_unnamed_scope( $1 ) ) {
          ignore_mode--;
        }
        free_safe( $1 );
        $$ = NULL;
      }
    }
  | ':' UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  |
    {
      $$ = NULL;
    }
  ;

begin_end_block
  : begin_end_id
    {
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        if( !db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, $1, @1.text, @1.first_line ) ) {
          ignore_mode++;
        }
      } else {
        ignore_mode++;
      }
      generate_top_mode--;
    }
    block_item_decls_opt statement_list
    {
      statement* stmt = $4;
      if( $3 && db_is_unnamed_scope( $1 ) && !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Net/variables declared in unnamed begin...end block that is specified to not allow SystemVerilog syntax" );
        ignore_mode++;
      }
      if( ignore_mode == 0 ) {
        if( stmt != NULL ) {
          stmt->suppl.part.head      = 1;
          stmt->suppl.part.is_called = 1;
          db_add_statement( stmt, stmt );
          $$ = db_get_curr_funit();
        } else {
          $$ = NULL;
        }
      } else {
        if( $3 && db_is_unnamed_scope( $1 ) ) {
          ignore_mode--;
        }
        $$ = NULL;
      }
      free_safe( $1 );
      generate_top_mode++;
    }
  | begin_end_id
    {
      free_safe( $1 );
      ignore_mode++;
      $$ = NULL;
    }
  ;

if_statement_error
  : statement_or_null %prec less_than_K_else
    {
      $$ = NULL;
    }
  | statement_or_null K_else statement_or_null
    {
      $$ = NULL;
    }
  ;

statement_list
  : statement_list statement
    {
      if( ignore_mode == 0 ) {
        if( $1 == NULL ) {
          db_remove_statement( $2 );
          $$ = NULL;
        } else {
          if( $2 != NULL ) {
            if( !db_statement_connect( $1, $2 ) ) {
              db_remove_statement( $2 );
            }
            $$ = $1;
          } else {
            db_remove_statement( $1 );
            $$ = NULL;
          }
        }
      }
    }
  | statement
    {
      $$ = $1;
    }
  ;

statement_or_null
  : statement
    {
      $$ = $1;
    }
  | ';'
    {
      $$ = NULL;
    }
  ;

  /* An lpvalue is the expression that can go on the left side of a procedural assignment.
     This rule handles only procedural assignments. */
lpvalue
  : identifier
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
        free_safe( $1 );
        $$  = tmp;
      } else {
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | identifier start_lhs index_expr end_lhs
    {
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        db_bind_expr_tree( $3, $1 );
        $3->line = @1.first_line;
        $3->col  = ((@1.first_column & 0xffff) << 16) | ($3->col & 0xffff);
        free_safe( $1 );
        $$ = $3;
      } else {
        free_safe( $1 );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | '{' start_lhs expression_list end_lhs '}'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$  = tmp;
      } else {
        expression_dealloc( $3, FALSE );
        $$  = NULL;
      }
    }
  ;

  /* An lavalue is the expression that can go on the left side of a
     continuous assign statement. This checks (where it can) that the
     expression meets the constraints of continuous assignments. */
lavalue
  : identifier
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
        } Catch_anonymous {
          free_safe( $1 );
          Throw 0;
        }
        free_safe( $1 );
        $$  = tmp;
      } else {
        free_safe( $1 );
        $$  = NULL;
      }
    }
  | identifier start_lhs index_expr end_lhs
    {
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        db_bind_expr_tree( $3, $1 );
        $3->line = @1.first_line;
        $3->col  = ((@1.first_column & 0xffff) << 16) | ($3->col & 0xffff);
        free_safe( $1 );
        $$ = $3;
      } else {
        free_safe( $1 );
        expression_dealloc( $3, FALSE );
        $$  = NULL;
      }
    }
  | '{' start_lhs expression_list end_lhs '}'
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, NULL, EXP_OP_CONCAT, TRUE, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$  = tmp;
      } else {
        expression_dealloc( $3, FALSE );
        $$  = NULL;
      }
    }
  ;

block_item_decls_opt
  : block_item_decls { $$ = TRUE; }
  | { $$ = FALSE; }
  ;

block_item_decls
  : block_item_decl
  | block_item_decls block_item_decl
  ;

  /* The block_item_decl is used in function definitions, task
     definitions, module definitions and named blocks. Wherever a new
     scope is entered, the source may declare new registers and
     integers. This rule matches those declarations. The containing
     rule has presumably set up the scope. */
block_item_decl
  : attribute_list_opt K_reg signed_opt range_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
    }
    register_variable_list ';'
  | attribute_list_opt K_bit signed_opt range_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
    }
    register_variable_list ';'
  | attribute_list_opt K_logic signed_opt range_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
    }
    register_variable_list ';'
  | attribute_list_opt K_char unsigned_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 7, 0, TRUE );
    }
    register_variable_list ';'
  | attribute_list_opt K_byte unsigned_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 7, 0, TRUE );
    }
    register_variable_list ';'
  | attribute_list_opt K_shortint unsigned_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 15, 0, TRUE );
    }
    register_variable_list ';'
  | attribute_list_opt K_integer unsigned_opt
    {
      curr_signed   = TRUE;
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 31, 0, TRUE );
    }
    register_variable_list ';'
  | attribute_list_opt K_int unsigned_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 31, 0, TRUE );
    }
    register_variable_list ';'
  | attribute_list_opt K_longint unsigned_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 63, 0, TRUE );
    }
    register_variable_list ';'
  | attribute_list_opt K_time
    {
      curr_signed   = FALSE;
      curr_mba      = TRUE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( 63, 0, TRUE );
    }
    register_variable_list ';'
  | attribute_list_opt TYPEDEF_IDENTIFIER ';'
    {
      curr_signed  = $2->is_signed;
      curr_mba     = TRUE;
      curr_handled = $2->is_handled;
      parser_copy_range_to_curr_range( $2->prange, TRUE );
      parser_copy_range_to_curr_range( $2->urange, FALSE );
    }
    register_variable_list ';'
  | attribute_list_opt K_real
    {
      int real_size = sizeof( double ) * 8;
      curr_signed   = TRUE;
      curr_mba      = FALSE;
      curr_handled  = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( (real_size - 1), 0, TRUE );
    }
    list_of_variables ';'
  | attribute_list_opt K_realtime
    {
      int real_size = sizeof( double ) * 8;
      curr_signed   = TRUE;
      curr_mba      = FALSE;
      curr_handled  = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      parser_implicitly_set_curr_range( (real_size - 1), 0, TRUE );
    }
    list_of_variables ';'
  | attribute_list_opt K_parameter parameter_assign_decl ';'
  | attribute_list_opt K_localparam
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Localparam syntax found in block that is specified to not allow Verilog-2001 syntax" );
        ignore_mode++;
      }
    }
    localparam_assign_decl ';'
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        ignore_mode--;
      }
    }
  | attribute_list_opt K_reg error ';'
    {
      VLerror( "Syntax error in reg variable list" );
    }
  | attribute_list_opt K_bit error ';'
    {
      VLerror( "Syntax error in bit variable list" );
    }
  | attribute_list_opt K_byte error ';'
    {
      VLerror( "Syntax error in byte variable list" );
    }
  | attribute_list_opt K_logic error ';'
    {
      VLerror( "Syntax error in logic variable list" );
    }
  | attribute_list_opt K_char error ';'
    {
      VLerror( "Syntax error in char variable list" );
    }
  | attribute_list_opt K_shortint error ';'
    {
      VLerror( "Syntax error in shortint variable list" );
    }
  | attribute_list_opt K_integer error ';'
    {
      VLerror( "Syntax error in integer variable list" );
    }
  | attribute_list_opt K_int error ';'
    {
      VLerror( "Syntax error in int variable list" );
    }
  | attribute_list_opt K_longint error ';'
    {
      VLerror( "Syntax error in longint variable list" );
    }
  | attribute_list_opt K_time error ';'
    {
      VLerror( "Syntax error in time variable list" );
    }
  | attribute_list_opt K_real error ';'
    {
      VLerror( "Syntax error in real variable list" );
    }
  | attribute_list_opt K_realtime error ';'
    {
      VLerror( "Syntax error in realtime variable list" );
    }
  | attribute_list_opt K_parameter error ';'
    {
      VLerror( "Syntax error in parameter variable list" );
    }
  | attribute_list_opt K_localparam
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Localparam syntax found in block that is specified to not allow Verilog-2001 syntax" );
      }
    }
    error ';'
    {
      if( parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Syntax error in localparam list" );
      }
    }
  ;	

case_item
  : expression_list ':' statement_or_null
    { PROFILE(PARSER_CASE_ITEM_A);
      case_statement* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
        cstmt->prev = NULL;
        cstmt->expr = $1;
        cstmt->stmt = $3;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | K_default ':' statement_or_null
    { PROFILE(PARSER_CASE_ITEM_B);
      case_statement* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
        cstmt->prev = NULL;
        cstmt->expr = NULL;
        cstmt->stmt = $3;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | K_default statement_or_null
    { PROFILE(PARSER_CASE_ITEM_C);
      case_statement* cstmt;
      if( ignore_mode == 0 ) {
        cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
        cstmt->prev = NULL;
        cstmt->expr = NULL;
        cstmt->stmt = $2;
        cstmt->line = @1.first_line;
        $$ = cstmt;
      } else {
        $$ = NULL;
      }
    }
  | error { ignore_mode++; } ':' statement_or_null { ignore_mode--; }
    {
      VLerror( "Illegal case expression" );
    }
  ;

case_items
  : case_items case_item
    {
      case_statement* list = $1;
      case_statement* curr = $2;
      if( ignore_mode == 0 ) {
        curr->prev = list;
        $$ = curr;
      } else {
        $$ = NULL;
      }
    }
  | case_item
    {
      $$ = $1;
    }
  ;

delay1
  : '#' delay_value_simple
    {
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          $$ = db_create_expression( $2, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ')'
    {
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        Try {
          $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  ;

delay3
  : '#' delay_value_simple
    {
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          $$ = db_create_expression( $2, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ')'
    {
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        Try {
          $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@4.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ',' delay_value ')'
    {
      expression_dealloc( $5, FALSE );
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        Try {
          $$ = db_create_expression( $3, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@6.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | '#' '(' delay_value ',' delay_value ',' delay_value ')'
    {
      expression_dealloc( $3, FALSE );
      expression_dealloc( $7, FALSE );
      if( ignore_mode == 0 ) {
        Try {
          $$ = db_create_expression( $5, NULL, EXP_OP_DELAY, lhs_mode, @1.first_line, @1.first_column, (@8.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $5, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $5, FALSE );
        $$ = NULL;
      }
    }
  ;

delay3_opt
  : delay3
    {
      $$ = $1;
    }
  |
    {
      $$ = NULL;
    }
  ;

delay_value
  : static_expr
    { PROFILE(PARSER_DELAY_VALUE_A);
      expression*  tmp;
      static_expr* se = $1;
      if( (ignore_mode == 0) && (se != NULL) ) {
        if( se->exp == NULL ) {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
          } Catch_anonymous {
            static_expr_dealloc( se, TRUE );
            Throw 0;
           }
          vector_init( tmp->value, (vec_data*)malloc_safe( (sizeof( vec_data ) * 32) ), TRUE, 32, VTYPE_VAL );
          vector_from_int( tmp->value, se->num );
          static_expr_dealloc( se, TRUE );
        } else {
          tmp = se->exp;
          static_expr_dealloc( se, FALSE );
        }
        $$ = tmp;
      } else {
        $$ = NULL;
      }
    }
  | static_expr ':' static_expr ':' static_expr
    { PROFILE(PARSER_DELAY_VALUE_B);
      expression*  tmp;
      static_expr* se = NULL;
      if( ignore_mode == 0 ) {
        if( delay_expr_type == DELAY_EXPR_DEFAULT ) {
          unsigned int rv = snprintf( user_msg,
                                      USER_MSG_LENGTH,
                                      "Delay expression type for min:typ:max not specified, using default of 'typ', file %s, line %u",
                                      obf_file( @1.text ),
                                      @1.first_line );
          assert( rv < USER_MSG_LENGTH );
          print_output( user_msg, WARNING, __FILE__, __LINE__ );
        }
        switch( delay_expr_type ) {
          case DELAY_EXPR_MIN :
            se = $1;
            static_expr_dealloc( $3, TRUE );
            static_expr_dealloc( $5, TRUE );
            break;
          case DELAY_EXPR_DEFAULT :
          case DELAY_EXPR_TYP     :
            se = $3;
            static_expr_dealloc( $1, TRUE );
            static_expr_dealloc( $5, TRUE );
            break;
          case DELAY_EXPR_MAX :
            se = $5;
            static_expr_dealloc( $1, TRUE );
            static_expr_dealloc( $3, TRUE );
            break;
          default:
            assert( (delay_expr_type == DELAY_EXPR_MIN) ||
                    (delay_expr_type == DELAY_EXPR_TYP) ||
                    (delay_expr_type == DELAY_EXPR_MAX) );
            break;
        }
        if( se != NULL ) {
          if( se->exp == NULL ) {
            Try {
              tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            } Catch_anonymous {
              static_expr_dealloc( se, TRUE );
              Throw 0;
            }
            vector_init( tmp->value, (vec_data*)malloc_safe( (sizeof( vec_data ) * 32) ), TRUE, 32, VTYPE_VAL );
            vector_from_int( tmp->value, se->num );
            static_expr_dealloc( se, TRUE );
          } else {
            tmp = se->exp;
            free_safe( se );
          }
          $$ = tmp;
        } else {
          $$ = NULL;
        }
      } else {
        $$ = NULL;
      }
    }
  ;

delay_value_simple
  : NUMBER
    {
      expression* tmp;
      Try {
        tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
      } Catch_anonymous {
        vector_dealloc( $1 );
        Throw 0;
      }
      assert( tmp->value->value == NULL );
      free_safe( tmp->value );
      tmp->value = $1;
      $$ = tmp;
    }
  | UNUSED_NUMBER
    {
      $$ = NULL;
    }
  | REALTIME
    {
      $$ = NULL;
    }
  | UNUSED_REALTIME
    {
      $$ = NULL;
    }
  | IDENTIFIER
    {
      expression* tmp;
      Try {
        tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
      } Catch_anonymous {
        free_safe( $1 );
        Throw 0;
      }
      free_safe( $1 );
      $$ = tmp;
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  ;

assign_list
  : assign_list ',' assign
  | assign
  ;

assign
  : lavalue '=' expression
    {
      expression* tmp;
      statement*  stmt;
      if( ($1 != NULL) && ($3 != NULL) && (info_suppl.part.excl_assign == 0) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_ASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        stmt = db_create_statement( tmp );
        stmt->suppl.part.head       = 1;
        stmt->suppl.part.stop_true  = 1;
        stmt->suppl.part.stop_false = 1;
        stmt->suppl.part.cont       = 1;
        /* Statement will be looped back to itself */
        db_connect_statement_true( stmt, stmt );
        db_connect_statement_false( stmt, stmt );
        db_add_statement( stmt, stmt );
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
      }
    }
  ;

range_opt
  : range
  |
    {
      if( ignore_mode == 0 ) {
        parser_implicitly_set_curr_range( 0, 0, curr_packed );
      }
    }
  ;

range
  : '[' static_expr ':' static_expr ']'
    {
      if( ignore_mode == 0 ) {
        parser_explicitly_set_curr_range( $2, $4, curr_packed );
      }
    }
  | range '[' static_expr ':' static_expr ']'
    {
      if( ignore_mode == 0 ) {
        parser_explicitly_set_curr_range( $3, $5, curr_packed );
      }
    }
  ;

range_or_type_opt
  : range      
  | K_integer
    {
      if( ignore_mode == 0 ) {
        parser_implicitly_set_curr_range( 31, 0, curr_packed );
      }
    }
  | K_real
  | K_realtime
  | K_time
    {
      if( ignore_mode == 0 ) {
        parser_implicitly_set_curr_range( 63, 0, curr_packed );
      }
    }
  |
    {
      if( ignore_mode == 0 ) {
        parser_implicitly_set_curr_range( 0, 0, curr_packed );
      }
    }
  ;

  /* The register_variable rule is matched only when I am parsing
     variables in a "reg" definition. I therefore know that I am
     creating registers and I do not need to let the containing rule
     handle it. The register variable list simply packs them together
     so that bit ranges can be assigned. */
register_variable
  : IDENTIFIER
    {
      db_add_signal( $1, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @1.first_line, @1.first_column, TRUE );
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER
  | IDENTIFIER '=' expression
    {
      expression* exp;
      statement*  stmt;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Register declaration with initialization found in block that is specified to not allow Verilog-2001 syntax" );
        free_safe( $1 );
        expression_dealloc( $3, FALSE );
      } else {
        db_add_signal( $1, curr_sig_type, &curr_prange, NULL, curr_signed, curr_mba, @1.first_line, @1.first_column, TRUE );
        if( $3 != NULL ) {
          Try {
            exp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          } Catch_anonymous {
            expression_dealloc( $3, FALSE );
            free_safe( $1 );
            Throw 0;
          }
          Try {
            exp = db_create_expression( $3, exp, EXP_OP_RASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $3, FALSE );
            expression_dealloc( exp, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( exp );
          stmt->suppl.part.head       = 1;
          stmt->suppl.part.stop_true  = 1;
          stmt->suppl.part.stop_false = 1;
          db_add_statement( stmt, stmt );
        }
        free_safe( $1 );
      }
    }
  | UNUSED_IDENTIFIER '=' expression
  | IDENTIFIER { curr_packed = FALSE; } range
    {
      /* Unpacked dimensions are now handled */
      curr_packed = TRUE;
      if( $1 != NULL ) {
        db_add_signal( $1, SSUPPL_TYPE_MEM, &curr_prange, &curr_urange, curr_signed, TRUE, @1.first_line, @1.first_column, TRUE );
        free_safe( $1 );
      }
    }
  | UNUSED_IDENTIFIER range
  ;

register_variable_list
  : register_variable
  | register_variable_list ',' register_variable
  ;

task_item_list_opt
  : task_item_list
  |
  ;

task_item_list
  : task_item_list task_item
  | task_item
  ;

task_item
  : block_item_decl
  | port_type range_opt
    {
      curr_signed  = FALSE;
      curr_mba     = FALSE;
      curr_handled = TRUE;
    }
    list_of_variables ';'
  ;

automatic_opt
  : K_automatic { $$ = TRUE; }
  |             { $$ = FALSE; }
  ;

signed_opt
  : K_signed   { curr_signed = TRUE;  }
  | K_unsigned { curr_signed = FALSE; }
  |            { curr_signed = FALSE; }
  ;

unsigned_opt
  : K_unsigned { curr_signed = FALSE; }
  | K_signed   { curr_signed = TRUE;  }
  |            { curr_signed = TRUE;  }
  ;

  /*
   The net_type_sign_range_opt is only used in port lists so don't set the curr_sig_type field
   as this will already be filled in by the port_type rule.
  */
net_type_sign_range_opt
  : K_wire signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_tri signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_tri1 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_supply0 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_wand signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_triand signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_tri0 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_supply1 signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_wor signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_trior signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | K_logic signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 1;
    }
  | TYPEDEF_IDENTIFIER
    {
      curr_mba     = FALSE;
      curr_signed  = $1->is_signed;
      curr_handled = $1->is_handled;
      parser_copy_range_to_curr_range( $1->prange, TRUE );
      parser_copy_range_to_curr_range( $1->urange, FALSE );
      $$ = 1;
    }
  | signed_opt range_opt
    {
      curr_mba     = FALSE;
      curr_handled = TRUE;
      $$ = 0;
    }
  ;

net_type
  : K_wire
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_tri
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_tri1
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_supply0
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_wand
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_triand
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_tri0
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_supply1
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_wor
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  | K_trior
    {
      curr_mba      = FALSE;
      curr_sig_type = SSUPPL_TYPE_DECLARED;
      curr_signed   = FALSE;
      curr_handled  = TRUE;
      $$ = 1;
    }
  ;

var_type
  : K_reg     { $$ = 1; }
  ;

net_decl_assigns
  : net_decl_assigns ',' net_decl_assign
  | net_decl_assign
  ;

net_decl_assign
  : IDENTIFIER '=' expression
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($1 != NULL) && (info_suppl.part.excl_assign == 0) ) {
        db_add_signal( $1, SSUPPL_TYPE_DECLARED, &curr_prange, NULL, curr_signed, FALSE, @1.first_line, @1.first_column, TRUE );
        if( $3 != NULL ) {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @1.first_line, @1.first_column, (@1.last_column - 1), $1 );
          } Catch_anonymous {
            expression_dealloc( $3, FALSE );
            free_safe( $1 );
            Throw 0;
          }
          Try {
            tmp = db_create_expression( $3, tmp, EXP_OP_DASSIGN, FALSE, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          stmt->suppl.part.head       = 1;
          stmt->suppl.part.stop_true  = 1;
          stmt->suppl.part.stop_false = 1;
          stmt->suppl.part.cont       = 1;
          /* Statement will be looped back to itself */
          db_connect_statement_true( stmt, stmt );
          db_connect_statement_false( stmt, stmt );
          db_add_statement( stmt, stmt );
        }
        free_safe( $1 );
      } else {
        free_safe( $1 );
      }
    }
  | UNUSED_IDENTIFIER '=' expression
  | delay1 IDENTIFIER '=' expression
    {
      expression* tmp;
      statement*  stmt;
      if( (ignore_mode == 0) && ($2 != NULL) && (info_suppl.part.excl_assign == 0) ) {
        db_add_signal( $2, SSUPPL_TYPE_DECLARED, &curr_prange, NULL, FALSE, FALSE, @2.first_line, @2.first_column, TRUE );
        if( $4 != NULL ) {
          Try {
            tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, TRUE, @2.first_line, @2.first_column, (@2.last_column - 1), $2 );
          } Catch_anonymous {
            expression_dealloc( $4, FALSE );
            free_safe( $2 );
            Throw 0;
          }
          Try {
            tmp = db_create_expression( $4, tmp, EXP_OP_DASSIGN, FALSE, @2.first_line, @2.first_column, (@4.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( tmp, FALSE );
            expression_dealloc( $4, FALSE );
            Throw 0;
          }
          stmt = db_create_statement( tmp );
          stmt->suppl.part.head       = 1;
          stmt->suppl.part.stop_true  = 1;
          stmt->suppl.part.stop_false = 1;
          stmt->suppl.part.cont       = 1;
          /* Statement will be looped back to itself */
          db_connect_statement_true( stmt, stmt );
          db_connect_statement_false( stmt, stmt );
          db_add_statement( stmt, stmt );
        }
        free_safe( $2 );
      } else {
        free_safe( $2 );
      }
    }
  ;

drive_strength_opt
  : drive_strength
  |
  ;

drive_strength
  : '(' dr_strength0 ',' dr_strength1 ')'
  | '(' dr_strength1 ',' dr_strength0 ')'
  | '(' dr_strength0 ',' K_highz1 ')'
  | '(' dr_strength1 ',' K_highz0 ')'
  | '(' K_highz1 ',' dr_strength0 ')'
  | '(' K_highz0 ',' dr_strength1 ')'
  ;

dr_strength0
  : K_supply0
  | K_strong0
  | K_pull0
  | K_weak0
  ;

dr_strength1
  : K_supply1
  | K_strong1
  | K_pull1
  | K_weak1
  ;

event_control
  : '@' IDENTIFIER
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), $2 );
        } Catch_anonymous {
          free_safe( $2 );
          Throw 0;
        }
        $$  = tmp;
      } else {
        $$ = NULL;
      }
      free_safe( $2 );
    }
  | '@' UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | '@' '(' event_expression_list ')'
    {
      $$ = $3;
    }
  | '@' '(' error ')'
    {
      VLerror( "Illegal event control expression" );
      $$ = NULL;
    }
  ;

event_expression_list
  : event_expression
    {
      $$ = $1;
    }
  | event_expression_list K_or event_expression
    {
      expression* tmp;
      if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
        Try {
          tmp = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          Throw 0;
        }
        $$ = tmp;
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      }
    }
  | event_expression_list ',' event_expression
    {
      expression* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Comma-separated combinational logic sensitivity list found in block that is specified to not allow Verilog-2001 syntax" );
        expression_dealloc( $1, FALSE );
        expression_dealloc( $3, FALSE );
        $$ = NULL;
      } else {
        if( (ignore_mode == 0) && ($1 != NULL) && ($3 != NULL) ) {
          Try {
            tmp = db_create_expression( $3, $1, EXP_OP_EOR, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
          } Catch_anonymous {
            expression_dealloc( $1, FALSE );
            expression_dealloc( $3, FALSE );
            Throw 0;
          }
          $$ = tmp;
        } else {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $3, FALSE );
          $$ = NULL;
        }
      }
    }
  ;

event_expression
  : K_posedge expression
    {
      expression* tmp1;
      expression* tmp2;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        /* Create 1-bit expression to hold last value of right expression */
        Try {
          tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
        Try {
          tmp2 = db_create_expression( $2, tmp1, EXP_OP_PEDGE, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp1, FALSE );
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
        $$ = tmp2;
      } else {
        $$ = NULL;
      }
    }
  | K_negedge expression
    {
      expression* tmp1;
      expression* tmp2;
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
        Try {
          tmp2 = db_create_expression( $2, tmp1, EXP_OP_NEDGE, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp1, FALSE );
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
        $$ = tmp2;
      } else {
        $$ = NULL;
      }
    }
  | expression
    {
      expression* tmp1;
      expression* tmp2;
      expression* expr = $1;
      if( (ignore_mode == 0) && ($1 != NULL ) ) {
        Try {
          tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( expr, FALSE );
          Throw 0;
        }
        Try {
          tmp2 = db_create_expression( expr, tmp1, EXP_OP_AEDGE, lhs_mode, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( tmp1, FALSE );
          expression_dealloc( expr, FALSE );
          Throw 0;
        }
        $$ = tmp2;
      } else {
        $$ = NULL;
      }
    }
  ;
		
charge_strength_opt
  : charge_strength
  |
  ;

charge_strength
  : '(' K_small ')'
  | '(' K_medium ')'
  | '(' K_large ')'
  ;

port_type
  : K_input  { curr_sig_type = SSUPPL_TYPE_INPUT;  }
  | K_output { curr_sig_type = SSUPPL_TYPE_OUTPUT; }
  | K_inout  { curr_sig_type = SSUPPL_TYPE_INOUT;  }
  ;

defparam_assign_list
  : defparam_assign
    {
      $$ = 0;
    }
  | range defparam_assign
    {
      $$ = 0;
    }
  | defparam_assign_list ',' defparam_assign
  ;

defparam_assign
  : identifier '=' expression
    {
      expression_dealloc( $3, FALSE );
      if( $1 != NULL ) {
        free_safe( $1 );
      }
    }
  ;

 /* Parameter override */
parameter_value_opt
  : '#' '(' { param_mode++; } expression_list { param_mode--; } ')'
    {
      if( ignore_mode == 0 ) {
        expression_dealloc( $4, FALSE );
      }
    }
  | '#' '(' parameter_value_byname_list ')'
  | '#' NUMBER
    {
      vector_dealloc( $2 );
    }
  | '#' UNUSED_NUMBER
  | '#' error
    {
      VLerror( "Syntax error in parameter value assignment list" );
    }
  |
  ;

parameter_value_byname_list
  : parameter_value_byname
  | parameter_value_byname_list ',' parameter_value_byname
  ;

parameter_value_byname
  : '.' IDENTIFIER '(' expression ')'
    { PROFILE(PARSER_PARAMETER_VALUE_BYNAME_A);
      param_oride* po;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Explicit in-line parameter passing syntax found in block that is specified to not allow Verilog-2001 syntax" );
        free_safe( $2 );
        expression_dealloc( $4, FALSE );
      } else {
        po = (param_oride*)malloc_safe( sizeof( param_oride ) );
        po->name = $2;
        po->expr = $4;
        po->next = NULL;
        if( param_oride_head == NULL ) {
          param_oride_head = param_oride_tail = po;
        } else {
          param_oride_tail->next = po;
          param_oride_tail       = po;
        }
      }
    }
  | '.' UNUSED_IDENTIFIER '(' expression ')'
  | '.' IDENTIFIER '(' ')'
    {
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Explicit in-line parameter passing syntax found in block that is specified to not allow Verilog-2001 syntax" );
      }
      free_safe( $2 );
    }
  | '.' UNUSED_IDENTIFIER '(' ')'
  ;

gate_instance_list
  : gate_instance_list ',' gate_instance
    {
      if( (ignore_mode == 0) && ($3 != NULL) ) {
        $3->next = $1;
        $$ = $3;
      } else {
        $$ = NULL;
      }
    }
  | gate_instance
    {
      if( ignore_mode == 0 ) {
        $$ = $1;
      } else {
        $$ = NULL;
      }
    }
  ;

  /* A gate_instance is a module instantiation or a built in part
     type. In any case, the gate has a set of connections to ports. */
gate_instance
  : IDENTIFIER '(' ignore_more expression_list ignore_less ')'
    { PROFILE(PARSER_GATE_INSTANCE_A);
      str_link* tmp;
      tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
      tmp->str   = $1;
      tmp->range = NULL;
      tmp->next  = NULL;
      $$ = tmp;
    }
  | UNUSED_IDENTIFIER '(' ignore_more expression_list ignore_less ')'
    {
      $$ = NULL;
    }
  | IDENTIFIER range '(' ignore_more expression_list ignore_less ')'
    { PROFILE(PARSER_GATE_INSTANCE_B);
      str_link* tmp;
      curr_prange.clear = TRUE;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Arrayed instantiation syntax found in block that is specified to not allow Verilog-2001 syntax" );
        free_safe( $1 );
        $$ = NULL;
      } else {
        tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
        tmp->str   = $1;
        tmp->range = curr_prange.dim;
        tmp->next  = NULL;
        $$ = tmp;
      }
    }
  | UNUSED_IDENTIFIER range '(' ignore_more expression_list ignore_less ')'
    {
      curr_prange.clear = TRUE;
      $$ = NULL;
    }

  | IDENTIFIER '(' port_name_list ')'
    { PROFILE(PARSER_GATE_INSTANCE_C);
      str_link* tmp;
      tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
      tmp->str   = $1;
      tmp->range = NULL;
      tmp->next  = NULL;
      $$ = tmp;
    }
  | UNUSED_IDENTIFIER '(' port_name_list ')'
    {
      $$ = NULL;
    }
  | IDENTIFIER range '(' port_name_list ')'
    { PROFILE(PARSER_GATE_INSTANCE_D);
      str_link* tmp;
      curr_prange.clear = TRUE;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Arrayed instantiation syntax found in block that is specified to not allow Verilog-2001 syntax" );
        free_safe( $1 );
        $$ = NULL;
      } else {
        tmp        = (str_link*)malloc_safe( sizeof( str_link ) );
        tmp->str   = $1;
        tmp->range = curr_prange.dim;
        tmp->next  = NULL;
        $$ = tmp;
      }
    }
  | UNUSED_IDENTIFIER range '(' port_name_list ')'
    {
      $$ = NULL;
    }
  | '(' ignore_more expression_list ignore_less ')'
    {
      $$ = NULL;
    }
  ;

gatetype
  : K_and
  | K_nand
  | K_or
  | K_nor
  | K_xor
  | K_xnor
  | K_buf
  | K_bufif0
  | K_bufif1
  | K_not
  | K_notif0
  | K_notif1
  | K_nmos
  | K_rnmos
  | K_pmos
  | K_rpmos
  | K_cmos
  | K_rcmos
  | K_tran
  | K_rtran
  | K_tranif0
  | K_tranif1
  | K_rtranif0
  | K_rtranif1
  ;

  /* A function_item_list only lists the input/output/inout
     declarations. The integer and reg declarations are handled in
     place, so are not listed. The list builder needs to account for
     the possibility that the various parts may be NULL. */
function_item_list
  : function_item
  | function_item_list function_item
  ;

function_item
  : K_input signed_opt range_opt
    {
      curr_mba      = FALSE;
      curr_handled  = TRUE;
      curr_sig_type = SSUPPL_TYPE_INPUT;
    }
    list_of_variables ';'
  | block_item_decl
  ;

parameter_assign_decl
  : signed_opt range_opt parameter_assign_list
  ;

parameter_assign_list
  : parameter_assign
  | parameter_assign_list ',' parameter_assign
  ;

parameter_assign
  : IDENTIFIER '=' expression
    {
      /* If the size was not set by the user, the left number will be set to 0 but we need to change this to 31 */
      assert( curr_prange.dim != NULL );
      if( curr_prange.dim[0].implicit ) {
        db_add_declared_param( curr_signed, NULL, NULL, $1, $3, FALSE );
      } else {
        db_add_declared_param( curr_signed, curr_prange.dim[0].left, curr_prange.dim[0].right, $1, $3, FALSE );
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '=' expression
  ;

localparam_assign_decl
  : signed_opt range_opt localparam_assign_list
  ;

localparam_assign_list
  : localparam_assign
  | localparam_assign_list ',' localparam_assign
  ;

localparam_assign
  : IDENTIFIER '=' expression
    {
      /* If the size was not set by the user, the left number will be set to 0 but we need to change this to 31 */
      assert( curr_prange.dim != NULL );
      if( curr_prange.dim[0].implicit ) {
        db_add_declared_param( curr_signed, NULL, NULL, $1, $3, TRUE );
      } else {
        db_add_declared_param( curr_signed, curr_prange.dim[0].left, curr_prange.dim[0].right, $1, $3, TRUE );
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '=' expression
  ;

port_name_list
  : port_name_list ',' port_name
  | port_name
  ;

port_name
  : '.' IDENTIFIER '(' ignore_more expression ignore_less ')'
    {
      free_safe( $2 );
    }
  | '.' IDENTIFIER '(' error ')'
    {
      free_safe( $2 );
    }
  | '.' IDENTIFIER '(' ')'
    {
      free_safe( $2 );
    }
  | '.' IDENTIFIER
    {
      if( (ignore_mode == 0) && !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Implicit .name port list item found in block that is specified to not allow SystemVerilog syntax" );
      }
      free_safe( $2 );
    }
  | K_PS
    {
      if( (ignore_mode == 0) && !parser_check_generation( GENERATION_SV ) ) {
        VLerror( "Implicit .* port list item found in block that is specified to not allow SystemVerilog syntax" );
      }
    }
  ;

specify_item_list
  : specify_item
  | specify_item_list specify_item
  ;

specify_item
  : K_specparam specparam_list ';'
  | specify_simple_path_decl ';'
  | specify_edge_path_decl ';'
  | K_if '(' expression ')' specify_simple_path_decl ';'
  | K_if '(' expression ')' specify_edge_path_decl ';'
  | K_Shold '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Speriod '(' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Srecovery '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Ssetup '(' spec_reference_event ',' spec_reference_event ',' expression spec_notifier_opt ')' ';'
  | K_Ssetuphold '(' spec_reference_event ',' spec_reference_event ',' expression ',' expression spec_notifier_opt ')' ';'
  | K_Swidth '(' spec_reference_event ',' expression ',' expression spec_notifier_opt ')' ';'
  | K_Swidth '(' spec_reference_event ',' expression ')' ';'
  ;

specparam_list
  : specparam
  | specparam_list ',' specparam
  ;

specparam
  : UNUSED_IDENTIFIER '=' expression
  | UNUSED_IDENTIFIER '=' expression ':' expression ':' expression
  | UNUSED_PATHPULSE_IDENTIFIER '=' expression
  | UNUSED_PATHPULSE_IDENTIFIER '=' '(' expression ',' expression ')'
  ;
  
specify_simple_path_decl
  : specify_simple_path '=' '(' specify_delay_value_list ')'
  | specify_simple_path '=' delay_value_simple
  | specify_simple_path '=' '(' error ')'
  ;

specify_simple_path
  : '(' specify_path_identifiers spec_polarity K_EG expression ')'
  | '(' specify_path_identifiers spec_polarity K_SG expression ')'
  | '(' error ')'
  ;

specify_path_identifiers
  : UNUSED_IDENTIFIER
  | UNUSED_IDENTIFIER '[' expr_primary ']'
  | specify_path_identifiers ',' UNUSED_IDENTIFIER
  | specify_path_identifiers ',' UNUSED_IDENTIFIER '[' expr_primary ']'
  ;

spec_polarity
  : '+'
  | '-'
  |
  ;

spec_reference_event
  : K_posedge expression
  | K_negedge expression
  | K_posedge expr_primary K_TAND expression
  | K_negedge expr_primary K_TAND expression
  | expr_primary K_TAND expression
    {
      assert( $1 == NULL );
      assert( $3 == NULL );
    }
  | expr_primary
    {
      assert( $1 == NULL );
    }
  ;

spec_notifier_opt
  :
  | spec_notifier
  ;

spec_notifier
  : ','
  | ','  identifier
    {
      if( $2 != NULL ) {
        free_safe( $2 );
      }
    }
  | spec_notifier ',' 
  | spec_notifier ',' identifier
    {
      if( $3 != NULL ) {
        free_safe( $3 );
      }
    }
  | UNUSED_IDENTIFIER
  ;

specify_edge_path_decl
  : specify_edge_path '=' '(' specify_delay_value_list ')'
  | specify_edge_path '=' delay_value_simple
  ;

specify_edge_path
  : '(' K_posedge specify_path_identifiers spec_polarity K_EG UNUSED_IDENTIFIER ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG UNUSED_IDENTIFIER ')'
  | '(' K_posedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG UNUSED_IDENTIFIER ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_EG '(' expr_primary polarity_operator expression ')' ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG UNUSED_IDENTIFIER ')'
  | '(' K_negedge specify_path_identifiers spec_polarity K_SG '(' expr_primary polarity_operator expression ')' ')'
  ;

polarity_operator
  : K_PO_POS
  | K_PO_NEG
  | ':'
  ;

specify_delay_value_list
  : delay_value
    {
      assert( $1 == NULL );
    }
  | specify_delay_value_list ',' delay_value
  ;

 /*
  These are SystemVerilog constructs that are optionally placed before if and case statements.  Covered parses
  them but otherwise ignores them (we will not bother displaying the warning messages).
 */
cond_specifier_opt
  : K_unique
  | K_priority
  |
  ;

 /* SystemVerilog enumeration syntax */
enumeration
  : K_enum enum_var_type_range_opt '{' enum_variable_list '}'
    {
      db_end_enum_list();
    }
  ;

 /* List of valid enumeration variable types */
enum_var_type_range_opt
  : TYPEDEF_IDENTIFIER range_opt
    {
      assert( curr_prange.dim != NULL );
      if( !$1->is_sizeable && !curr_prange.dim[0].implicit ) { 
        VLerror( "Packed dimensions are not allowed for this type" );
      } else {
        if( $1->is_sizeable && !curr_prange.dim[0].implicit ) {
          /* TBD - Need to multiply the size of the typedef with the size of the curr_range */
        } else {
          parser_copy_range_to_curr_range( $1->prange, TRUE );
        }
      }
    }
  | K_reg range_opt
  | K_logic range_opt
  | K_int
    {
      parser_implicitly_set_curr_range( 31, 0, TRUE );
    }
  | K_integer
    {
      parser_implicitly_set_curr_range( 31, 0, TRUE );
    }
  | K_shortint
    {
      parser_implicitly_set_curr_range( 15, 0, TRUE );
    }
  | K_longint
    {
      parser_implicitly_set_curr_range( 63, 0, TRUE );
    }
  | K_byte
    {
      parser_implicitly_set_curr_range( 7, 0, TRUE );
    }
  | range_opt
    {
      assert( curr_prange.dim != NULL );
      if( curr_prange.dim[0].implicit ) {
        parser_implicitly_set_curr_range( 31, 0, TRUE );
      }
    }
  ;

 /* This is a lot like a register_variable but assigns proper value to variables with no assignment */
enum_variable
  : IDENTIFIER
    {
      db_add_signal( $1, SSUPPL_TYPE_ENUM, &curr_prange, NULL, curr_signed, FALSE, @1.first_line, @1.first_column, TRUE );
      Try {
        db_add_enum( db_find_signal( $1, FALSE ), NULL );
      } Catch_anonymous {
        free_safe( $1 );
        Throw 0;
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER
  | IDENTIFIER '=' static_expr
    {
      db_add_signal( $1, SSUPPL_TYPE_ENUM, &curr_prange, NULL, curr_signed, FALSE, @1.first_line, @1.first_column, TRUE );
      Try {
        db_add_enum( db_find_signal( $1, FALSE ), $3 );
      } Catch_anonymous {
        free_safe( $1 );
        Throw 0;
      }
      free_safe( $1 );
    }
  | UNUSED_IDENTIFIER '=' static_expr
    {
      static_expr_dealloc( $3, TRUE );
    }
  ;

enum_variable_list
  : enum_variable_list ',' enum_variable
  | enum_variable
  ;

list_of_names
  : IDENTIFIER
    { PROFILE(PARSER_LIST_OF_NAMES_A);
      str_link* strl = (str_link*)malloc_safe( sizeof( str_link ) );
      strl->str    = $1;
      strl->suppl  = @1.first_line;
      strl->suppl2 = @1.first_column;
      strl->next   = NULL;
      $$ = strl;
    }
  | UNUSED_IDENTIFIER
    {
      $$ = NULL;
    }
  | list_of_names ',' IDENTIFIER
    { PROFILE(PARSER_LIST_OF_NAMES_B);
      str_link* strl = (str_link*)malloc_safe( sizeof( str_link ) );
      str_link* strt = $1;
      strl->str    = $3;
      strl->suppl  = @3.first_line;
      strl->suppl2 = @3.first_column;
      strl->next   = NULL;
      while( strt->next != NULL ) strt = strt->next;
      strt->next = strl;
      $$ = $1; 
    }
  | list_of_names ',' UNUSED_IDENTIFIER
    {
      str_link_delete_list( $1 );
      $$ = NULL;
    }
  ;

 /* Handles typedef declarations (both in module and $root space) */
typedef_decl
  : K_typedef enumeration IDENTIFIER ';'
    {
      if( ignore_mode == 0 ) {
        assert( curr_prange.dim != NULL );
        db_add_typedef( $3, curr_signed, curr_handled, TRUE, parser_copy_curr_range( TRUE ), parser_copy_curr_range( FALSE ) );
        free_safe( $3 );
      }
    }
  | K_typedef net_type_sign_range_opt IDENTIFIER ';'
    {
      if( ignore_mode == 0 ) {
        assert( curr_prange.dim != NULL );
        db_add_typedef( $3, curr_signed, curr_handled, TRUE, parser_copy_curr_range( TRUE ), parser_copy_curr_range( FALSE ) );
        free_safe( $3 );
      }
    }
  ;

single_index_expr
  : '[' expression ']'
    {
      if( (ignore_mode == 0) && ($2 != NULL) ) {
        Try {
          $$ = db_create_expression( NULL, $2, EXP_OP_SBIT_SEL, lhs_mode, @1.first_line, @1.first_column, (@3.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
    }
  | '[' expression ':' expression ']'
    {
      if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
        Try {
          $$ = db_create_expression( $4, $2, EXP_OP_MBIT_SEL, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $2, FALSE );
          expression_dealloc( $4, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $2, FALSE );
        expression_dealloc( $4, FALSE );
        $$ = NULL;
      }
    }
  | '[' expression K_PO_POS static_expr ']'
    {
      expression* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Indexed vector part select found in block that is specified to not allow Verilog-2001 syntax" );
        expression_dealloc( $2, FALSE );
        static_expr_dealloc( $4, FALSE );
        $$ = NULL;
      } else {
        if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
          if( $4->exp == NULL ) {
            Try {
              tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( $2, FALSE );
              static_expr_dealloc( $4, TRUE );
              Throw 0;
            }
            vector_dealloc( tmp->value );
            tmp->value = vector_create( 32, VTYPE_VAL, TRUE );
            vector_from_int( tmp->value, $4->num );
            Try {
              tmp = db_create_expression( tmp, $2, EXP_OP_MBIT_POS, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( tmp, FALSE );
              expression_dealloc( $2, FALSE );
              static_expr_dealloc( $4, TRUE );
              Throw 0;
            }
          } else {
            Try {
              tmp = db_create_expression( $4->exp, $2, EXP_OP_MBIT_POS, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( $2, FALSE );
              static_expr_dealloc( $4, TRUE );
              Throw 0;
            }
          }
          $$ = tmp;
        } else {
          expression_dealloc( $2, FALSE );
          static_expr_dealloc( $4, FALSE );
          $$ = NULL;
        }
      }
    }
  | '[' expression K_PO_NEG static_expr ']'
    {
      expression* tmp;
      if( !parser_check_generation( GENERATION_2001 ) ) {
        VLerror( "Indexed vector part select found in block that is specified to not allow Verilog-2001 syntax" );
        expression_dealloc( $2, FALSE );
        static_expr_dealloc( $4, FALSE );
        $$ = NULL;
      } else {
        if( (ignore_mode == 0) && ($2 != NULL) && ($4 != NULL) ) {
          if( $4->exp == NULL ) {
            Try {
              tmp = db_create_expression( NULL, NULL, EXP_OP_STATIC, FALSE, @1.first_line, @1.first_column, (@1.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( $2, FALSE );
              static_expr_dealloc( $4, TRUE );
              Throw 0;
            }
            vector_dealloc( tmp->value );
            tmp->value = vector_create( 32, VTYPE_VAL, TRUE );
            vector_from_int( tmp->value, $4->num );
            Try {
              tmp = db_create_expression( tmp, $2, EXP_OP_MBIT_NEG, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( tmp, FALSE );
              expression_dealloc( $2, FALSE );
              static_expr_dealloc( $4, TRUE );
              Throw 0;
            }
          } else {
            Try {
              tmp = db_create_expression( $4->exp, $2, EXP_OP_MBIT_NEG, lhs_mode, @1.first_line, @1.first_column, (@5.last_column - 1), NULL );
            } Catch_anonymous {
              expression_dealloc( $2, FALSE );
              static_expr_dealloc( $4, TRUE );
              Throw 0;
            }
          }
          $$ = tmp;
        } else {
          expression_dealloc( $2, FALSE );
          static_expr_dealloc( $4, FALSE );
          $$ = NULL;
        }
      }
    }
  ;

index_expr
  : index_expr single_index_expr
    {
      if( (ignore_mode == 0) && ($1 != NULL) && ($2 != NULL) ) {
        Try {
          $$ = db_create_expression( $2, $1, EXP_OP_DIM, lhs_mode, @1.first_line, @1.first_column, (@2.last_column - 1), NULL );
        } Catch_anonymous {
          expression_dealloc( $1, FALSE );
          expression_dealloc( $2, FALSE );
          Throw 0;
        }
      } else {
        expression_dealloc( $1, FALSE );
        expression_dealloc( $2, FALSE );
        $$ = NULL;
      }
    }
  | single_index_expr
    {
      $$ = $1;
    }
  ;

ignore_more
  :
    {
      ignore_mode++;
    }
  ;
  
ignore_less
  :
    {
      ignore_mode--;
    }
  ;

start_lhs
  :
    {
      lhs_mode = TRUE;
    }
  ;

end_lhs
  :
    {
      lhs_mode = FALSE;
    }
  ;

inc_fork_depth
  :
    {
      fork_depth++;
      fork_block_depth = (int*)realloc( fork_block_depth, ((fork_depth + 1) * sizeof( int )) );
      fork_block_depth[fork_depth] = block_depth;
    }
  ;

dec_fork_depth
  :
    {
      fork_depth--;
      fork_block_depth = (int*)realloc( fork_block_depth, ((fork_depth + 1) * sizeof( int )) );
    }
  ;

inc_block_depth
  :
    {
      block_depth++;
    }
  ;

dec_block_depth
  :
    {
      block_depth--;
    }
  ;

inc_for_depth
  :
    {
      char* scope      = NULL;
      func_unit* funit = db_get_curr_funit();
      if( ignore_mode == 0 ) {
        scope = db_create_unnamed_scope();
        Try {
          assert( db_add_function_task_namedblock( FUNIT_NAMED_BLOCK, scope, funit->filename, 0 ) );
        } Catch_anonymous {
          free_safe( scope );
          Throw 0;
        }
        free_safe( scope );
      }
      block_depth++;
      for_mode++;
      $$ = db_get_curr_funit();
    }
  ;

dec_for_depth
  :
    {
      block_depth--;
      for_mode--;
    }
  ;

inc_gen_expr_mode
  :
    {
      generate_expr_mode++;
    }
  ;

dec_gen_expr_mode
  : 
    {
      generate_expr_mode--;
    }
  ;
