
%{
/*!
 \file     parser.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/14/2001
*/

#include <stdio.h>

#include "defines.h"
#include "signal.h"
#include "expr.h"
#include "vector.h"
#include "module.h"
#include "db.h"
#include "link.h"
#include "parser_misc.h"

char err_msg[1000];

int ignore_mode = 0;

/* Uncomment these lines to turn debugging on */
//#define YYDEBUG 1
//#define YYERROR_VERBOSE 1
//int yydebug = 1;
%}

%union {
  char*           text;
  int	          integer;
  vector*         number;
  double          realtime;
  signal*         sig;
  expression*     expr;
  statement*      state;
  signal_width*   sigwidth; 
  str_link*       strlink;
  case_statement* case_stmt;
};

%token <text>   HIDENTIFIER IDENTIFIER
%token <text>   PATHPULSE_IDENTIFIER
%token <number> NUMBER
%token <realtime> REALTIME
%token STRING PORTNAME SYSTEM_IDENTIFIER IGNORE
%token UNUSED_HIDENTIFIER UNUSED_IDENTIFIER
%token UNUSED_PATHPULSE_IDENTIFIER
%token UNUSED_NUMBER
%token UNUSED_REALTIME
%token UNUSED_STRING UNUSED_PORTNAME UNUSED_SYSTEM_IDENTIFIER
%token K_LE K_GE K_EG K_EQ K_NE K_CEQ K_CNE K_LS K_RS K_SG
%token K_PO_POS K_PO_NEG
%token K_LOR K_LAND K_NAND K_NOR K_NXOR K_TRIGGER
%token K_always K_and K_assign K_begin K_buf K_bufif0 K_bufif1 K_case
%token K_casex K_casez K_cmos K_deassign K_default K_defparam K_disable
%token K_edge K_else K_end K_endcase K_endfunction K_endmodule I_endmodule
%token K_endprimitive K_endspecify K_endtable K_endtask K_event K_for
%token K_force K_forever K_fork K_function K_highz0 K_highz1 K_if
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

%token KK_attribute

%type <integer>   net_type
%type <sigwidth>  range_opt range
%type <integer>   static_expr static_expr_primary
%type <text>      identifier port_reference port port_opt port_reference_list
%type <text>      list_of_ports list_of_ports_opt
%type <expr>      expr_primary expression_list expression
%type <expr>      event_control event_expression_list event_expression
%type <text>      udp_port_list
%type <text>      lpvalue lavalue
%type <expr>      delay_value delay_value_simple
%type <text>      range_or_type_opt
%type <text>      defparam_assign_list defparam_assign
%type <text>      gate_instance
%type <text>      parameter_assign_list parameter_assign
%type <text>      localparam_assign_list localparam_assign
%type <strlink>   register_variable_list list_of_variables
%type <strlink>   net_decl_assigns gate_instance_list
%type <text>      register_variable net_decl_assign
%type <state>     statement statement_list statement_opt 
%type <state>     for_statement fork_statement while_statement named_begin_end_block if_statement_error
%type <case_stmt> case_items case_item
%type <expr>      delay1 delay3 delay3_opt

%token K_TAND
%right '?' ':'
%left K_LOR
%left K_LAND
%left '|'
%left '^' K_NXOR K_NOR
%left '&' K_NAND
%left K_EQ K_NE K_CEQ K_CNE
%left K_GE K_LE '<' '>'
%left K_LS K_RS
%left '+' '-'
%left '*' '/' '%'
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

source_file 
	: description 
	| source_file description
	;

description
	: module
	| udp_primitive
	| KK_attribute { ignore_mode++; } '(' UNUSED_IDENTIFIER ',' UNUSED_STRING ',' UNUSED_STRING ')' { ignore_mode--; }
	;

module
	: module_start IDENTIFIER list_of_ports_opt ';'
		{ 
		  db_add_module( $2, @1.text );
		}
	  module_item_list
	  K_endmodule
		{
		  db_end_module();
		}
	| module_start IDENTIFIER list_of_ports_opt ';'
		{
		  db_add_module( $2, @1.text );
		}
	  K_endmodule
		{
		  db_end_module();
		}
	| module_start IGNORE I_endmodule
	;

module_start
	: K_module
	| K_macromodule
	;

list_of_ports_opt
	: '(' list_of_ports ')'
		{
		  $$ = 0;
		}
	|
		{
		  $$ = 0;
		}
	;

list_of_ports
	: port_opt
	| list_of_ports ',' port_opt
	;

port_opt
	: port
	|
		{
		  $$ = 0;
		}
	;

  /* Coverage tools should not find port information that interesting.  We will
     handle it for parsing purposes, but ignore its information. */

port
	: port_reference
	| PORTNAME '(' { ignore_mode++; } port_reference { ignore_mode--; } ')'
		{
		  $$ = 0;
		}
	| '{' { ignore_mode++; } port_reference_list { ignore_mode--; } '}'
		{
		  $$ = 0;
		}
	| PORTNAME '(' '{' { ignore_mode--; } port_reference_list { ignore_mode++; } '}' ')'
		{
		  $$ = 0;
		}
	;

port_reference
        : UNUSED_IDENTIFIER
                {
                  $$ = NULL;
                }
        | UNUSED_IDENTIFIER '[' static_expr ':' static_expr ']'
                {
                  $$ = NULL;
                }
        | UNUSED_IDENTIFIER '[' static_expr ']'
                {
                  $$ = NULL;
                }
        | UNUSED_IDENTIFIER '[' error ']'
                { 
                  $$ = NULL;
                }
	;

port_reference_list
	: port_reference
	| port_reference_list ',' port_reference
	;
		
static_expr
	: static_expr_primary
		{
		  $$ = $1;
		}
	| '+' static_expr_primary %prec UNARY_PREC
		{
		  $$ = +$2;
		}
	| '-' static_expr_primary %prec UNARY_PREC
		{
		  $$ = -$2;
		}
        | '~' static_expr_primary %prec UNARY_PREC
		{
		  $$ = ~$2;
		}
        | '&' static_expr_primary %prec UNARY_PREC
		{
		  int tmp = $2;
		  int uop = tmp & 0x1;
		  int i;
		  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
		    uop = uop & ((tmp >> i) & 0x1);
		  }
		  $$ = uop;
		}
        | '!' static_expr_primary %prec UNARY_PREC
		{
		  $$ = ($2 == 0) ? 1 : 0;
		}
        | '|' static_expr_primary %prec UNARY_PREC
		{
		  int tmp = $2;
		  int uop = tmp & 0x1;
		  int i;
		  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
		    uop = uop | ((tmp >> i) & 0x1);
		  }
		  $$ = uop;
		}
        | '^' static_expr_primary %prec UNARY_PREC
		{
		  int tmp = $2;
		  int uop = uop & 0x1;
		  int i;
		  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
		    uop = uop ^ ((tmp >> i) & 0x1);
		  }
		  $$ = uop;
		}
        | K_NAND static_expr_primary %prec UNARY_PREC
		{
                  int tmp = $2;
                  int uop = tmp & 0x1;
                  int i;
                  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
                    uop = uop & ((tmp >> i) & 0x1);
                  }
                  $$ = (uop == 0) ? 1 : 0;
		}
        | K_NOR static_expr_primary %prec UNARY_PREC
		{
                  int tmp = $2;
                  int uop = tmp & 0x1;
                  int i;
                  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
                    uop = uop | ((tmp >> i) & 0x1);
                  }
                  $$ = (uop == 0) ? 1 : 0;
		}
        | K_NXOR static_expr_primary %prec UNARY_PREC
		{
                  int tmp = $2;
                  int uop = uop & 0x1;
                  int i;
                  for( i=1; i<(SIZEOF_INT * 8); i++ ) {
                    uop = uop ^ ((tmp >> i) & 0x1);
                  }
                  $$ = (uop == 0) ? 1 : 0;
		}

        | static_expr '^' static_expr
		{
		  $$ = $1 ^ $3;
		}
        | static_expr '*' static_expr
		{
		  $$ = $1 * $3;
		}
        | static_expr '/' static_expr
		{
                  if( ignore_mode == 0 ) {
		    $$ = $1 / $3;
                  } else {
                    $$ = 0;
                  }
		}
        | static_expr '%' static_expr
		{
                  if( ignore_mode == 0 ) {
		    $$ = $1 % $3;
                  } else {
                    $$ = 0;
                  }
		}
        | static_expr '+' static_expr
		{
		  $$ = $1 + $3;
		}
        | static_expr '-' static_expr
		{
	  	  $$ = $1 - $3;
		}
        | static_expr '&' static_expr
		{
		  $$ = $1 & $3;
		}
        | static_expr '|' static_expr
		{
		  $$ = $1 | $3;
		}
        | static_expr K_NOR static_expr
		{
		  $$ = ~($1 | $3);
		}
     	| static_expr K_NAND static_expr
		{
		  $$ = ~($1 & $3);
		}
        | static_expr K_NXOR static_expr
		{
		  $$ = ~($1 ^ $3);
		}
	;

static_expr_primary
	: NUMBER
		{
		  $$ = vector_to_int( $1 );
		  vector_dealloc( $1 );
		}
        | UNUSED_NUMBER
                {
                  $$ = 0;
                }
	| '(' static_expr ')'
		{
		  $$ = $2;
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
		    $$ = $2;
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
                  } else {
                    $$ = NULL;
                  }
		}
	| '-' expr_primary %prec UNARY_PREC
		{
                  if( ignore_mode == 0 ) {
		    $$ = $2;
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
                  } else {
                    $$ = NULL;
                  }
		}
	| '~' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UINV, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| '&' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UAND, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| '!' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
			 	@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UNOT, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| '|' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| '^' expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UXOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_NAND expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UNAND, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_NOR expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UNOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_NXOR expr_primary %prec UNARY_PREC
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $2 == NULL ) {
		      snprintf( err_msg, 1000, "%s:%d: Expression signal not declared", 
				@1.text,
				@1.first_line );
		      print_output( err_msg, FATAL );
		      exit( 1 );
		    }
		    tmp = db_create_expression( $2, NULL, EXP_OP_UNXOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| '!' error %prec UNARY_PREC
		{
		  // yyerror( @1, "error: Operand of unary ! is not a primary expression." );
		}
	| '^' error %prec UNARY_PREC
		{
		  // yyerror( @1, "error: Operand of reduction ^ is not a primary expression." );
		}
	| expression '^' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_XOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '*' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_MULTIPLY, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '/' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_DIVIDE, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '%' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_MOD, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '+' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_ADD, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '-' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_SUBTRACT, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '&' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_AND, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '|' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_OR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
        | expression K_NAND expression
                {
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_NAND, @1.first_line, NULL );
                    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
                }
	| expression K_NOR expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_NOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_NXOR expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_NXOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '<' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_LT, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '>' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_GT, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_LS expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_LSHIFT, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_RS expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_RSHIFT, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_EQ expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_EQ, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_CEQ expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_CEQ, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_LE expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_LE, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_GE expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_GE, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_NE expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_NE, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_CNE expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_CNE, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_LOR expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_LOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression K_LAND expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, $1, EXP_OP_LAND, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression '?' expression ':' expression
		{
                  expression* csel;
                  expression* cond;
                  if( ignore_mode == 0 ) {
                    csel = db_create_expression( $5,   $3, EXP_OP_COND_SEL, @1.first_line, NULL );
                    cond = db_create_expression( csel, $1, EXP_OP_COND,     @1.first_line, NULL );
		    $$ = cond;
                  } else {
                    $$ = NULL;
                  }
		}
	;

expr_primary
	: NUMBER
		{
		  expression* tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                  free_safe( tmp->value );
		  tmp->value = $1;
		  $$ = tmp;
		}
        | UNUSED_NUMBER
                {
                  $$ = NULL;
                }
	| STRING
		{
		  $$ = NULL;
		}
        | UNUSED_STRING
                {
                  $$ = NULL;
                }
	| identifier
		{
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, $1 );
                    $$ = tmp;
		    free_safe( $1 );
                  } else {
                    $$ = NULL;
                  }
		}
	| SYSTEM_IDENTIFIER
		{
		  $$ = NULL;
		}
        | UNUSED_SYSTEM_IDENTIFIER
                {
                  $$ = NULL;
                }
	| identifier '[' expression ']'
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $3, NULL, EXP_OP_SBIT_SEL, @1.first_line, $1 );
		    $$ = tmp;
		    free_safe( $1 );
                  } else {
                    $$ = NULL;
                  }
		}
	| identifier '[' expression ':' expression ']'
		{		  
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = db_create_expression( $5, $3, EXP_OP_MBIT_SEL, @1.first_line, $1 );
                    $$ = tmp;
                    free_safe( $1 );
                  } else {
                    $$ = NULL;
                  }
		}
	| identifier '(' expression_list ')'
		{
                  if( ignore_mode == 0 ) {
                    expression_dealloc( $3, FALSE );
                    free_safe( $1 );
                  }
		  $$ = NULL;
		}
	| SYSTEM_IDENTIFIER '(' expression_list ')'
		{
                  expression_dealloc( $3, FALSE );
		  $$ = NULL;
		}
        | UNUSED_SYSTEM_IDENTIFIER '(' expression_list ')'
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
                  if( ignore_mode == 0 ) {
		    $$ = $2;
                  } else {
                    $$ = NULL;
                  }
		}
	| '{' expression '{' expression_list '}' '}'
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    tmp = db_create_expression( $4, $2, EXP_OP_EXPAND, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	;

  /* Expression lists are used in function/task calls and concatenations */
expression_list
	: expression_list ',' expression
		{
		  expression* tmp;
                  if( ignore_mode == 0 ) {
		    if( $3 != NULL ) {
		      tmp = db_create_expression( $3, $1, EXP_OP_CONCAT, @1.first_line, NULL );
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
                  if( ignore_mode == 0 ) {
		    $$ = $1;
                  } else {
                    $$ = NULL;
                  }
		}
	|
		{
		  $$ = NULL;
		}
	| expression_list ','
		{
                  if( ignore_mode == 0 ) {
		    $$ = $1;
                  } else {
                    $$ = NULL;
                  }
		}
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
	| HIDENTIFIER
		{
		  $$ = $1;
		}
        | UNUSED_HIDENTIFIER
                {
                  $$ = NULL;
                }
	;

list_of_variables
	: IDENTIFIER
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $1;
		  tmp->next = NULL;
		  $$ = tmp;
		}
	| list_of_variables ',' IDENTIFIER
		{
		  str_link* tmp = (str_link*)malloc( sizeof( str_link ) );
		  tmp->str  = $3;
		  tmp->next = $1;
		  $$ = tmp;
		}
	;

  /* I don't know what to do with UDPs.  This is included to allow designs that contain
     UDPs to pass parsing. */
udp_primitive
	: K_primitive IDENTIFIER '(' udp_port_list ')' ';'
	    udp_port_decls
	    udp_init_opt
	    udp_body
	  K_endprimitive
	;

udp_port_list
	: IDENTIFIER
		{
		  $$ = 0;
		}
	| udp_port_list ',' IDENTIFIER
	;

udp_port_decls
	: udp_port_decl
	| udp_port_decls udp_port_decl
	;

udp_port_decl
	: K_input list_of_variables ';'
		{
		  str_link_delete_list( $2 );
		}
	| K_output IDENTIFIER ';'
	| K_reg IDENTIFIER ';'
	;

udp_init_opt
	: udp_initial
	|
	;

udp_initial
	: K_initial IDENTIFIER '=' NUMBER ';'
	;

udp_body
	: K_table
	    udp_entry_list
	  K_endtable
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

  /* This is the start of a module body */
module_item_list
	: module_item_list module_item
	| module_item
	;

module_item
	: net_type range_opt list_of_variables ';'
		{
		  str_link* tmp  = $3;
		  str_link* curr = tmp;
		  if( $1 == 1 ) {
		    /* Creating signal(s) */
		    while( curr != NULL ) {
		      db_add_signal( curr->str, $2->width, $2->lsb, 0 );
		      curr = curr->next;
		    }
		  }
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| net_type range_opt net_decl_assigns ';'
		{
		  str_link* tmp  = $3;
		  str_link* curr = tmp;
		  if( $1 == 1 ) {
		    /* Create signal(s) */
		    while( curr != NULL ) {
                      db_add_signal( curr->str, $2->width, $2->lsb, 0 );
                      curr = curr->next;
                    }
		    /* What to do about assignments? */
		  }
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| net_type drive_strength net_decl_assigns ';'
		{
		  str_link* tmp  = $3;
		  str_link* curr = tmp;
		  if( $1 == 1 ) {
		    /* Create signal(s) */
	            while( curr != NULL ) {
                      db_add_signal( curr->str, 1, 0, 0 );
                      curr = curr->next;
                    }
                    /* What to do about assignments? */
		  }
		  str_link_delete_list( $3 );
		}
	| K_trireg { ignore_mode++; } charge_strength_opt range_opt delay3_opt list_of_variables ';' { ignore_mode--; }
		{
		  /* Tri-state signals are not currently supported by covered */
		}
	| port_type range_opt list_of_variables ';'
		{
		  /* Create signal -- implicitly this is a wire which may not be explicitly declared */
		  str_link* tmp  = $3;
                  str_link* curr = tmp;
		  while( curr != NULL ) {
                    db_add_signal( curr->str, $2->width, $2->lsb, 0 );
                    curr = curr->next;
                  }
		  str_link_delete_list( $3 );
		  free_safe( $2 );
		}
	| port_type range_opt error ';'
		{
		  free_safe( $2 );
		  // yyerror( @3, "error: Invalid variable list in port declaration.");
		  // if( $2 ) delete $2;
		  // yyerrok;
		}
	| block_item_decl
	| K_defparam defparam_assign_list ';'
	| K_event list_of_variables ';'
		{
		  str_link_delete_list( $2 );
		}

  /* Handles instantiations of modules and user-defined primitives. */
	| IDENTIFIER parameter_value_opt gate_instance_list ';'
		{
		  str_link* tmp = $3;
		  str_link* curr = tmp;
		  while( curr != NULL ) {
		    db_add_instance( curr->str, $1 );
		    curr = curr->next;
		  }
		  str_link_delete_list( $3 );
		}

	| K_assign drive_strength_opt { ignore_mode++; } delay3_opt { ignore_mode--; } assign_list ';'
		{
		}
	| K_always statement
		{
                  statement* stmt = $2;
                  db_statement_connect( stmt, stmt );
                  db_statement_set_stop( stmt, stmt, TRUE );
                  stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
                  db_add_statement( stmt );
		}
	| K_initial { ignore_mode++; } statement { ignore_mode--; }
		{
                  /*
                  statement* stmt = $2;
                  db_statement_set_stop( stmt, NULL, FALSE );
                  stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);
                  db_add_statement( stmt );
                  */
		}
	| K_task { ignore_mode++; } UNUSED_IDENTIFIER ';'
	    task_item_list_opt statement_opt
	  K_endtask { ignore_mode--; }
	| K_function { ignore_mode++; } range_or_type_opt UNUSED_IDENTIFIER ';'
	    function_item_list statement
	  K_endfunction { ignore_mode--; }
	| K_specify K_endspecify
	| K_specify error K_endspecify
	| error ';'
		{
		  // yyerror( @1, "error: invalid module item.  Did you forget an initial or always?" );
		  // yyerrok;
		}
	| K_assign error '=' { ignore_mode++; } expression { ignore_mode--; } ';'
		{
		  // yyerror( @1, "error: syntax error in left side of continuous assignment." );
		  // yyerrok;
		}
	| K_assign error ';'
		{
		  // yyerror( @1, "error: syntax error in continuous assignment." );
		  // yyerrok;
		}
	| K_function error K_endfunction
		{
		  // yyerrok( @1, "error: I give up on this function definition" );
		  // yyerrok;
		}
	| KK_attribute '(' { ignore_mode++; } UNUSED_IDENTIFIER ',' UNUSED_STRING ',' UNUSED_STRING { ignore_mode--; }')' ';'
	| KK_attribute '(' error ')' ';'
		{
		  // yyerror( @1, "error: Misformed $attribute parameter list.");
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
	| K_begin statement_list K_end
		{
                  $$ = $2;
		}
	| K_begin ':' { ignore_mode++; } named_begin_end_block { ignore_mode--; } K_end
		{
                  $$ = NULL;
		}
	| K_begin K_end
		{
                  $$ = NULL;
		}
	| K_begin error K_end
		{
		  yyerrok;
                  $$ = NULL;
		}
	| K_fork { ignore_mode++; } fork_statement { ignore_mode--; } K_join
		{
                  $$ = NULL;
		}
	| K_disable { ignore_mode++; } identifier ';' { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_TRIGGER { ignore_mode++; } UNUSED_IDENTIFIER ';' { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_forever { ignore_mode++; } statement { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_fork { ignore_mode++; } statement_list K_join { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_repeat { ignore_mode++; } '(' expression ')' statement { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_case '(' expression ')' case_items K_endcase
		{
                  expression*     expr;
                  statement*      stmt;
                  statement*      last_stmt = NULL;
                  case_statement* c_stmt    = $5;
                  case_statement* tc_stmt;
                  if( ignore_mode == 0 ) {
                    while( c_stmt != NULL ) {
                      expr = db_create_expression( $3, c_stmt->expr, EXP_OP_EQ, @1.first_line, NULL );
                      db_add_expression( expr );
                      stmt = db_create_statement( expr );
                      db_connect_statement_true( stmt, c_stmt->stmt );
                      db_connect_statement_false( stmt, last_stmt );
                      db_statement_set_stop( c_stmt->stmt, NULL, FALSE );
                      if( stmt != NULL ) {
                        last_stmt = stmt;
                      }
                      tc_stmt   = c_stmt;
                      c_stmt    = c_stmt->next;
                      free_safe( tc_stmt );
                    }
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_casex '(' expression ')' case_items K_endcase
		{
                  expression*     expr;
                  statement*      stmt;
                  statement*      last_stmt = NULL;
                  case_statement* c_stmt    = $5;
                  case_statement* tc_stmt;
                  if( ignore_mode == 0 ) {
                    while( c_stmt != NULL ) {
                      expr = db_create_expression( $3, c_stmt->expr, EXP_OP_CEQ, @1.first_line, NULL );
                      db_add_expression( expr );
                      stmt = db_create_statement( expr );
                      db_connect_statement_true( stmt, c_stmt->stmt );
                      db_connect_statement_false( stmt, last_stmt );
                      db_statement_set_stop( c_stmt->stmt, NULL, FALSE );
                      if( stmt != NULL ) {
                        last_stmt = stmt;
                      }
                      tc_stmt   = c_stmt;
                      c_stmt    = c_stmt->next;
                      free_safe( tc_stmt );
                    }
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_casez '(' expression ')' case_items K_endcase
		{
                  expression*     expr;
                  statement*      stmt;
                  statement*      last_stmt = NULL;
                  case_statement* c_stmt    = $5;
                  case_statement* tc_stmt;
                  if( ignore_mode == 0 ) {
                    while( c_stmt != NULL ) {
                      expr = db_create_expression( $3, c_stmt->expr, EXP_OP_CEQ, @1.first_line, NULL );
                      db_add_expression( expr );
                      stmt = db_create_statement( expr );
                      db_connect_statement_true( stmt, c_stmt->stmt );
                      db_connect_statement_false( stmt, last_stmt );
                      db_statement_set_stop( c_stmt->stmt, NULL, FALSE );
                      if( stmt != NULL ) {
                        last_stmt = stmt;
                      }
                      tc_stmt   = c_stmt;
                      c_stmt    = c_stmt->next;
                      free_safe( tc_stmt );
                    }
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_case '(' expression ')' error K_endcase
		{
                  if( ignore_mode == 0 ) {
                    expression_dealloc( $3, FALSE );
                  }
                  $$ = NULL;
		}
	| K_casex '(' expression ')' error K_endcase
		{
                  if( ignore_mode == 0 ) {
                    expression_dealloc( $3, FALSE );
                  }
                  $$ = NULL;
		}
	| K_casez '(' expression ')' error K_endcase
		{
                  if( ignore_mode == 0 ) {
                    expression_dealloc( $3, FALSE );
                  }
                  $$ = NULL;
		}
	| K_if '(' expression ')' statement_opt %prec less_than_K_else
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $3 );
		    db_add_expression( $3 );
                    db_connect_statement_true( stmt, $5 );
                    db_statement_set_stop( $5, NULL, FALSE );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_if '(' expression ')' statement_opt K_else statement_opt
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $3 );
		    db_add_expression( $3 );
                    db_connect_statement_true( stmt, $5 );
                    db_connect_statement_false( stmt, $7 );
                    db_statement_set_stop( $5, NULL, FALSE );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_if '(' error ')' { ignore_mode++; } if_statement_error { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_for { ignore_mode++; } for_statement { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_while { ignore_mode++; } while_statement { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| delay1 statement_opt
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $1 );
                    db_add_expression( $1 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| event_control statement_opt
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $1 );
                    db_add_expression( $1 );
                    db_statement_connect( stmt, $2 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| lpvalue '=' expression ';'
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $3 );
		    db_add_expression( $3 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| lpvalue K_LE expression ';'
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $3 );
		    db_add_expression( $3 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| lpvalue '=' { ignore_mode++; } delay1 { ignore_mode--; } expression ';'
		{
		  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $4 );
		    db_add_expression( $4 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| lpvalue K_LE { ignore_mode++; } delay1 { ignore_mode--; } expression ';'
		{
		  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $4 );
		    db_add_expression( $4 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| lpvalue '=' { ignore_mode++; } event_control { ignore_mode--; } expression ';'
		{
		  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $4 );
		    db_add_expression( $4 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| lpvalue '=' K_repeat { ignore_mode++; } '(' expression ')' event_control expression ';' { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| lpvalue K_LE { ignore_mode++; } event_control { ignore_mode--; } expression ';'
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $4 );
		    db_add_expression( $4 );
                    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| lpvalue K_LE K_repeat { ignore_mode++; } '(' expression ')' event_control expression ';' { ignore_mode--; }
		{
                  $$ = NULL;
		}
	| K_wait '(' expression ')' statement_opt
		{
                  statement* stmt;
                  if( ignore_mode == 0 ) {
                    stmt = db_create_statement( $3 );
                    db_add_expression( $3 );
                    db_connect_statement_true( stmt, $5 );
		    $$ = stmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| SYSTEM_IDENTIFIER { ignore_mode++; } '(' expression_list ')' ';' { ignore_mode--; }
		{
                  $$ = NULL;
		}
        | UNUSED_SYSTEM_IDENTIFIER '(' expression_list ')' ';'
                {
                  $$ = NULL;
                }
	| SYSTEM_IDENTIFIER ';'
		{
                  $$ = NULL;
		}
        | UNUSED_SYSTEM_IDENTIFIER ';'
                {
                  $$ = NULL;
                }
	| identifier { ignore_mode++; } '(' expression_list ')' ';' { ignore_mode--; }
		{
                  if( ignore_mode == 0 ) {
		    free_safe( $1 );
                  }
                  $$ = NULL;
		}
	| identifier ';'
		{
                  if( ignore_mode == 0 ) {
		    free_safe( $1 );
                  }
                  $$ = NULL;
		}
	| error ';'
		{
		  // yyerror( @1, "error: Malformed statement." );
		  // yyerrok;
                  $$ = NULL;
		}
	;

for_statement
	: '(' lpvalue '=' expression ';' expression ';' lpvalue '=' expression ')' statement
		{
                  $$ = NULL;
		}
	| '(' lpvalue '=' expression ';' expression ';' error ')' statement
		{
                  $$ = NULL;
		}
	| '(' lpvalue '=' expression ';' error ';' lpvalue '=' expression ')' statement
		{
                  $$ = NULL;
		}
	| '(' error ')' statement
		{
                  $$ = NULL;
		}
        ;

fork_statement
	: ':' UNUSED_IDENTIFIER block_item_decls_opt statement_list
		{
                  $$ = NULL;
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

while_statement
	: '(' expression ')' statement
		{
                  $$ = NULL;
		}
	| '(' error ')' statement
		{
                  $$ = NULL;
		}
        ;

named_begin_end_block
        : UNUSED_IDENTIFIER block_item_decls_opt statement_list
		{
                  $$ = NULL;
		}
	| UNUSED_IDENTIFIER K_end 
		{
                  $$ = NULL;
		}
        ;

if_statement_error
	: statement_opt %prec less_than_K_else
		{
                  $$ = NULL;
		}
	| statement_opt K_else statement_opt
		{
                  $$ = NULL;
		}
        ;

statement_list
	: statement_list statement
                {
                  if( ignore_mode == 0 ) {
                    if( $1 == NULL ) {
                      $$ = $2;
                    } else {
                      if( $2 != NULL ) {
                        db_statement_connect( $1, $2 );
                      }
                      $$ = $1;
                    }
                  }
                }
	| statement
                {
                  $$ = $1;
                }
	;

statement_opt
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
	| identifier '[' static_expr ']'
	| identifier '[' static_expr ':' static_expr ']'
	| '{' expression_list '}'
		{
		  $$ = 0;
		}
	;

  /* An lavalue is the expression that can go on the left side of a
     continuous assign statement. This checks (where it can) that the
     expression meets the constraints of continuous assignments. */
lavalue
	: identifier
	| identifier '[' static_expr ']'
	| identifier range
		{
		  free_safe( $2 );
		}
	| '{' expression_list '}'
		{
		  $$ = 0;
		}
	;

block_item_decls_opt
	: block_item_decls
	|
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
	: K_reg range register_variable_list ';'
		{
		  /* Create new signal */
		  str_link* tmp  = $3;
		  str_link* curr = tmp;
                  if( ignore_mode == 0 ) {
		    while( curr != NULL ) {
		      db_add_signal( curr->str, $2->width, $2->lsb, 0 );
		      curr = curr->next;
		    }
		    str_link_delete_list( tmp );
		    free_safe( $2 );
                  }
		}
	| K_reg register_variable_list ';'
		{
		  /* Create new signal */
		  str_link* tmp  = $2;
		  str_link* curr = tmp;
                  if( ignore_mode == 0 ) {
		    while( curr != NULL ) {
		      db_add_signal( curr->str, 1, 0, 0 );
		      curr = curr->next;
		    }
		    str_link_delete_list( tmp );
                  }
		}
	| K_reg K_signed range register_variable_list ';'
		{
		  /* Create new signal */
		  str_link* tmp  = $4;
                  str_link* curr = tmp;
                  if( ignore_mode == 0 ) {
                    while( curr != NULL ) {
                      db_add_signal( curr->str, $3->width, $3->lsb, 0 );
                      curr = curr->next;
                    }
                    str_link_delete_list( tmp );
		    free_safe( $3 );
                  }
		}
	| K_reg K_signed register_variable_list ';'
		{
		  /* Create new signal */
                  str_link* tmp  = $3;
                  str_link* curr = tmp;
                  if( ignore_mode == 0 ) {
                    while( curr != NULL ) {
                      db_add_signal( curr->str, 1, 0, 0 );
                      curr = curr->next;
                    }
                    str_link_delete_list( tmp );
                  }
		}
	| K_integer register_variable_list ';'
		{
                  if( ignore_mode == 0 ) {
		    str_link_delete_list( $2 );
                  }
		}
	| K_time register_variable_list ';'
		{
                  if( ignore_mode == 0 ) {
		    str_link_delete_list( $2 );
                  }
		}
	| K_real list_of_variables ';'
		{
                  if( ignore_mode == 0 ) {
		    str_link_delete_list( $2 );
                  }
		}
	| K_realtime list_of_variables ';'
		{
                  if( ignore_mode == 0 ) {
		    str_link_delete_list( $2 );
                  }
		}
	| K_parameter parameter_assign_list ';'
	| K_localparam localparam_assign_list ';'
	| K_reg error ';'
		{
		  // yyerror( @1, "error: syntax error in reg variable list." );
		  // yyerrok;
		}
 	;	

case_item
	: expression_list ':' statement_opt
		{
                  case_statement* cstmt;
                  if( ignore_mode == 0 ) {
                    cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
                    cstmt->next = NULL;
                    cstmt->expr = $1;
                    cstmt->stmt = $3;
		    $$ = cstmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_default ':' statement_opt
		{
                  case_statement* cstmt;
                  if( ignore_mode == 0 ) {
                    cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
                    cstmt->next = NULL;
                    cstmt->expr = NULL;
                    cstmt->stmt = $3;
                    $$ = cstmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_default statement_opt
		{
                  case_statement* cstmt;
                  if( ignore_mode == 0 ) {
                    cstmt = (case_statement*)malloc_safe( sizeof( case_statement ) );
                    cstmt->next = NULL;
                    cstmt->expr = NULL;
                    cstmt->stmt = $2;
                    $$ = cstmt;
                  } else {
                    $$ = NULL;
                  }
		}
	| error { ignore_mode++; } ':' statement_opt { ignore_mode--; }
		{
		  // yyerror( @1, "error: Incomprehensible case expression." );
		  // yyerrok;
		}
	;

case_items
	: case_item case_items
                {
                  case_statement* list = $2;
                  case_statement* curr = $1;
                  if( ignore_mode == 0 ) {
                    curr->next = list;
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
                  vector*     vec;
                  expression* exp; 
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, 0xffffffff ); 
                    free_safe( tmp->value );
                    tmp->value = vec;
                    exp = db_create_expression( $2, tmp, EXP_OP_DELAY, @1.first_line, NULL );
                    $$  = exp;
                  } else {
                    $$ = NULL;
                  }
                }
	| '#' '(' delay_value ')'
                {
                  vector*     vec;
                  expression* exp;
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, 0xffffffff );
                    free_safe( tmp->value );
                    tmp->value = vec;
                    exp = db_create_expression( $3, tmp, EXP_OP_DELAY, @1.first_line, NULL );
                    $$  = exp;
                  } else {
                    $$ = NULL;
                  }
                }
	;

delay3
	: '#' delay_value_simple
                {
                  vector*     vec;
                  expression* exp; 
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, 0xffffffff );
                    free_safe( tmp->value );
                    tmp->value = vec;
                    exp = db_create_expression( $2, tmp, EXP_OP_DELAY, @1.first_line, NULL );
                    $$  = exp;
                  } else {
                    $$ = NULL;
                  }
                }
	| '#' '(' delay_value ')'
                {
                  vector*     vec;
                  expression* exp; 
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, 0xffffffff );
                    free_safe( tmp->value );
                    tmp->value = vec;
                    exp = db_create_expression( $3, tmp, EXP_OP_DELAY, @1.first_line, NULL );
                    $$  = exp;
                  } else {
                    $$ = NULL;
                  }
                }
	| '#' '(' delay_value ',' delay_value ')'
                {
                  vector*     vec;
                  expression* exp; 
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, 0xffffffff );
                    free_safe( tmp->value );
                    tmp->value = vec;
                    exp = db_create_expression( $3, tmp, EXP_OP_DELAY, @1.first_line, NULL );
                    $$  = exp;
                  } else {
                    $$ = NULL;
                  }
                }
	| '#' '(' delay_value ',' delay_value ',' delay_value ')'
                {
                  vector*     vec;
                  expression* exp; 
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, 0xffffffff );
                    free_safe( tmp->value );
                    tmp->value = vec;
                    exp = db_create_expression( $3, tmp, EXP_OP_DELAY, @1.first_line, NULL );
                    $$  = exp;
                  } else {
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
                {
                  vector*     vec;
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, $1 );
                    free_safe( tmp->value );
                    tmp->value = vec;
                    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
                }
	| static_expr ':' static_expr ':' static_expr
                {
                  vector*     vec;
                  expression* tmp;
                  if( ignore_mode == 0 ) {
                    vec = vector_create( 32, 0 );
                    tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
                    vector_from_int( vec, $1 );
                    free_safe( tmp->value );
                    tmp->value = vec;
                    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
                }                  
	;

delay_value_simple
	: NUMBER
		{
		  expression* tmp = db_create_expression( NULL, NULL, EXP_OP_NONE, @1.first_line, NULL );
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
                  expression* tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, $1 );
                  $$ = tmp;
		  free_safe( $1 );
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
                  statement* stmt = db_create_statement( $3 );

                  /* Set STMT_HEAD bit */
                  stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_HEAD);

                  /* Set STMT_STOP bit */
                  stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_STOP);

                  /* Set STMT_CONTINUOUS bit */
                  stmt->exp->suppl = stmt->exp->suppl | (0x1 << SUPPL_LSB_STMT_CONTINUOUS);

                  /* Statement will be looped back to itself */
                  db_connect_statement_true( stmt, stmt );
                  db_connect_statement_false( stmt, stmt );
		  db_add_expression( $3 );
            
                  /* Now add statement to current module */
                  db_add_statement( stmt );
		}
	;

range_opt
	: range
		{
		  $$ = $1;
		}
	|
		{
		  signal_width* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (signal_width*)malloc( sizeof( signal_width ) );
		    tmp->width = 1;
		    tmp->lsb   = 0;
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
;

range
	: '[' static_expr ':' static_expr ']'
		{
		  signal_width* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (signal_width*)malloc( sizeof( signal_width ) );
		    if( $2 >= $4 ) {
		      tmp->width = ($2 - $4) + 1;
		      tmp->lsb   = $4;
		    } else {
		      tmp->width = $4 - $2;
		      tmp->lsb   = $2;
		    }
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	;

range_or_type_opt
	: range      
                { 
                  if( ignore_mode == 0 ) {
                    free_safe( $1 );
                  }
                  $$ = NULL;
                }
	| K_integer  { $$ = NULL; }
	| K_real     { $$ = NULL; }
	| K_realtime { $$ = NULL; }
	| K_time     { $$ = NULL; }
	|            { $$ = NULL; }
	;

  /* The register_variable rule is matched only when I am parsing
     variables in a "reg" definition. I therefore know that I am
     creating registers and I do not need to let the containing rule
     handle it. The register variable list simply packs them together
     so that bit ranges can be assigned. */
register_variable
	: IDENTIFIER
		{
		  $$ = $1;
		}
        | UNUSED_IDENTIFIER
                {
                  $$ = NULL;
                }
	| IDENTIFIER '=' expression
		{
		  $$ = $1;
		}
        | UNUSED_IDENTIFIER '=' expression
                {
                  $$ = NULL;
                }
	| IDENTIFIER '[' static_expr ':' static_expr ']'
		{
		  /* We don't support memory coverage */
		  $$ = NULL;
		}
        | UNUSED_IDENTIFIER '[' static_expr ':' static_expr ']'
                {
                  $$ = NULL;
                }
	;

register_variable_list
	: register_variable
		{
		  str_link* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (str_link*)malloc( sizeof( str_link ) );
		    tmp->str  = $1;
		    tmp->next = NULL;
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| register_variable_list ',' register_variable
		{
		  str_link* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (str_link*)malloc( sizeof( str_link ) );
		    tmp->str  = $3;
		    tmp->next = $1;
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
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
	| K_input { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
		{
                  if( ignore_mode == 0 ) {
		    str_link_delete_list( $4 );
                    free_safe( $3 );
                  }
		}
	| K_output { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
		{
                  if( ignore_mode == 0 ) {
		    str_link_delete_list( $4 );
                    free_safe( $3 );
                  }
		}
	| K_inout { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
		{
                  if( ignore_mode == 0 ) {
		    str_link_delete_list( $4 );
                    free_safe( $3 );
                  }
		}
	;

net_type
	: K_wire    { $$ = 1; }
	| K_tri     { $$ = 0; }
	| K_tri1    { $$ = 0; }
	| K_supply0 { $$ = 0; }
	| K_wand    { $$ = 0; }
	| K_triand  { $$ = 0; }
	| K_tri0    { $$ = 0; }
	| K_supply1 { $$ = 0; }
	| K_wor     { $$ = 0; }
	| K_trior   { $$ = 0; }
	;

net_decl_assigns
	: net_decl_assigns ',' net_decl_assign
		{
		  str_link* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (str_link*)malloc( sizeof( str_link ) );
		    tmp->str  = $3;
		    tmp->next = $1;
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| net_decl_assign
		{
		  str_link* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (str_link*)malloc( sizeof( str_link) );
		    tmp->str  = $1;
		    tmp->next = NULL;
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	;

net_decl_assign
	: IDENTIFIER '=' { ignore_mode++; } expression { ignore_mode--; }
		{
                  if( ignore_mode == 0 ) {
		    $$ = $1;
                  } else {
                    $$ = NULL;
                  }
		}
        | UNUSED_IDENTIFIER '=' expression
                {
                  $$ = NULL;
                } 
	| delay1 IDENTIFIER '=' { ignore_mode++; } expression { ignore_mode--; }
		{
		  if( ignore_mode == 0 ) {
                    statement_dealloc( $1 );
		    $$ = $2;
                  } else {
                    $$ = NULL;
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
		  signal*     sig;
		  expression* tmp;
                  if( ignore_mode == 0 ) {
                    sig = db_find_signal( $2 );
		    if( sig != NULL ) {
		      tmp = db_create_expression( NULL, NULL, EXP_OP_SIG, @1.first_line, NULL );
		      vector_dealloc( tmp->value );
		      tmp->value = sig->value;
		      $$ = tmp;
		    } else {
		      $$ = NULL;
		    }
		    free_safe( $2 );
                  } else {
                    $$ = NULL;
                  }
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
                  if( ignore_mode == 0 ) {
		    tmp = db_create_expression( $3, $1, EXP_OP_EOR, @1.first_line, NULL );
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| event_expression_list ',' event_expression
		{
                  if( ignore_mode == 0 ) {
		    expression_dealloc( $1, FALSE );
		    expression_dealloc( $3, FALSE );
                  }
                  $$ = NULL;
		}
	;

event_expression
	: K_posedge expression
		{
                  expression* tmp1;
		  expression* tmp2;
                  nibble      val = 0x2;
                  if( ignore_mode == 0 ) {
                    /* Create 1-bit expression to hold last value of right expression */
                    tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, @1.first_line, NULL );
                    vector_set_value( tmp1->value, &val, 1, 0, 0 );
		    tmp2 = db_create_expression( $2, tmp1, EXP_OP_PEDGE, @1.first_line, NULL );
		    $$ = tmp2;
                  } else {
                    $$ = NULL;
                  }
		}
	| K_negedge expression
		{
		  expression* tmp1;
                  expression* tmp2;
                  nibble      val = 0x2;
                  if( ignore_mode == 0 ) {
                    tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, @1.first_line, NULL );
                    vector_set_value( tmp1->value, &val, 1, 0, 0 );
		    tmp2 = db_create_expression( $2, tmp1, EXP_OP_NEDGE, @1.first_line, NULL );
		    $$ = tmp2;
                  } else {
                    $$ = NULL;
                  }
		}
	| expression
		{
		  expression* tmp1;
                  expression* tmp2;
                  nibble      val = 0x2;
                  if( ignore_mode == 0 ) {
                    tmp1 = db_create_expression( NULL, NULL, EXP_OP_LAST, @1.first_line, NULL );
                    vector_set_value( tmp1->value, &val, 1, 0, 0 );
		    tmp2 = db_create_expression( $1, tmp1, EXP_OP_AEDGE, @1.first_line, NULL );
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
	: K_input
	| K_output
	| K_inout
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
                  if( ignore_mode == 0 ) {
		    expression_dealloc( $3, FALSE );
                  }
		}
	;

parameter_value_opt
	: '#' '(' expression_list ')'
		{
                  if( ignore_mode == 0 ) {
		    expression_dealloc( $3, FALSE );
                  }
		}
	| '#' '(' parameter_value_byname_list ')'
	| '#' NUMBER
                {
                  vector_dealloc( $2 );
                }
        | '#' UNUSED_NUMBER
	| '#' error
	|
	;

parameter_value_byname_list
	: parameter_value_byname
	| parameter_value_byname_list ',' parameter_value_byname
	;

parameter_value_byname
	: PORTNAME '(' expression ')'
		{
                  if( ignore_mode == 0 ) {
		    expression_dealloc( $3, FALSE );
                  }
		}
        | UNUSED_PORTNAME '(' expression ')'
	| PORTNAME '(' ')'
        | UNUSED_PORTNAME '(' ')'
	;

gate_instance_list
	: gate_instance_list ',' gate_instance
		{
		  str_link* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (str_link*)malloc( sizeof( str_link ) );
		    tmp->str  = $3;
		    tmp->next = $1;
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	| gate_instance
		{
		  str_link* tmp;
                  if( ignore_mode == 0 ) {
                    tmp = (str_link*)malloc( sizeof( str_link ) );
		    tmp->str  = $1;
		    tmp->next = NULL;
		    $$ = tmp;
                  } else {
                    $$ = NULL;
                  }
		}
	;

  /* A gate_instance is a module instantiation or a built in part
     type. In any case, the gate has a set of connections to ports. */
gate_instance
	: IDENTIFIER '(' { ignore_mode++; } expression_list { ignore_mode--; } ')'
		{
                  if( ignore_mode == 0 ) {
		    $$ = $1;
                  } else {
                    $$ = NULL;
                  }
		}
        | UNUSED_IDENTIFIER '(' expression_list ')'
                {
                  $$ = NULL;
                }
	| IDENTIFIER { ignore_mode++; } range '(' expression_list { ignore_mode--; } ')'
		{
                  if( ignore_mode == 0 ) {
		    $$ = $1;
                  } else {
                    $$ = NULL;
                  }
		}
	| '(' { ignore_mode++; } expression_list { ignore_mode--; } ')'
		{
		  $$ = NULL;
		}
	| IDENTIFIER '(' port_name_list ')'
		{
		  $$ = $1;
		}
        | UNUSED_IDENTIFIER '(' port_name_list ')'
                {
                  $$ = NULL;
                }
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
	: K_input { ignore_mode++; } range_opt list_of_variables ';' { ignore_mode--; }
	| block_item_decl
	;

parameter_assign_list
	: parameter_assign
	| range parameter_assign
		{
		  $$ = 0;
		}
	| parameter_assign_list ',' parameter_assign

parameter_assign
	: IDENTIFIER '=' { ignore_mode++; } expression { ignore_mode--; }
		{
		  $$ = $1;
		}
        | UNUSED_IDENTIFIER '=' expression
                {
                  $$ = NULL;
                }
	;

localparam_assign_list
	: localparam_assign
                {
                  $$ = NULL;
                }
	| range localparam_assign
		{
                  if( ignore_mode == 0 ) {
                    free_safe( $1 );
                  }
		  $$ = NULL;
		}
	| localparam_assign_list ',' localparam_assign
                {
                  $$ = NULL;
                }
	;

localparam_assign
	: IDENTIFIER '=' { ignore_mode++; } expression { ignore_mode--; }
		{
		  $$ = $1;
		}
        | UNUSED_IDENTIFIER '=' expression
                {
                  $$ = NULL;
                }
	;

port_name_list
	: port_name_list ',' port_name
	| port_name
	;

port_name
	: PORTNAME '(' { ignore_mode++; } expression { ignore_mode--; } ')'
        | UNUSED_PORTNAME '(' expression ')'
	| PORTNAME '(' error ')'
        | UNUSED_PORTNAME '(' error ')'
	| PORTNAME '(' ')'
        | UNUSED_PORTNAME '(' ')'
	;

