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
 \file     fsm_arg.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     10/02/2003
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "fsm_arg.h"
#include "util.h"
#include "fsm.h"
#include "fsm_var.h"
#include "vsignal.h"
#include "expr.h"
#include "vector.h"
#include "statement.h"
#include "link.h"
#include "param.h"
#include "obfuscate.h"


extern int  curr_expr_id;
extern char user_msg[USER_MSG_LENGTH];


/*!
 \return Returns pointer to expression tree containing parsed state variable expression.

 \throws anonymous fsm_var_bind_add fsm_var_bind_add fsm_var_bind_add fsm_var_bind_add fsm_var_bind_add Throw Throw expression_create expression_create
                   expression_create expression_create expression_create expression_create expression_create expression_create expression_create
                   expression_create expression_create expression_create expression_create expression_create

 Parses the specified argument value for all information regarding a state variable
 expression.
*/
static expression* fsm_arg_parse_state(
  char** arg,        /*!< Pointer to argument to parse */
  char*  funit_name  /*!< Name of functional unit that this expression belongs to */
) { PROFILE(FSM_ARG_PARSE_STATE);

  bool        error = FALSE;  /* Specifies if a parsing error has been found */
  vsignal*    sig;            /* Pointer to read-in signal */
  expression* expl  = NULL;   /* Pointer to left expression */
  expression* expr  = NULL;   /* Pointer to right expression */
  expression* expt  = NULL;   /* Pointer to temporary expression */
  statement*  stmt;           /* Pointer to statement containing top expression */
  exp_op_type op;             /* Type of operation to decode for this signal */

  /*
   If we are a concatenation, parse arguments of concatenation as signal names
   in a comma-separated list.
  */
  if( **arg == '{' ) {

    /* Get first state */
    while( (**arg != '}') && !error ) {
      if( ((expl != NULL) && (**arg == ',')) || (**arg == '{') ) {
        (*arg)++;
      }
      if( (sig = vsignal_from_string( arg )) != NULL ) {

        Try {

          if( sig->value->width == 0 ) {

            expr = expression_create( NULL, NULL, EXP_OP_SIG, FALSE, curr_expr_id, 0, 0, 0, FALSE );
            curr_expr_id++;
            fsm_var_bind_add( sig->name, expr, funit_name );

          } else if( sig->value->width == 1 ) {

            expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
            curr_expr_id++;
            vector_dealloc( expr->value );
            expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
            vector_from_int( expr->value, sig->dim[0].lsb );

            expr = expression_create( NULL, expr, EXP_OP_SBIT_SEL, FALSE, curr_expr_id, 0, 0, 0, FALSE );
            curr_expr_id++;
            fsm_var_bind_add( sig->name, expr, funit_name );

          } else {

            expt = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
            curr_expr_id++;
            vector_dealloc( expt->value );
            expt->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
            vector_from_int( expt->value, sig->dim[0].lsb );

            expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
            curr_expr_id++;
            vector_dealloc( expr->value );
            expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
            vector_from_int( expr->value, ((sig->value->width - 1) + sig->dim[0].lsb) );

            switch( sig->suppl.part.type ) {
              case SSUPPL_TYPE_IMPLICIT     :  op = EXP_OP_MBIT_SEL;  break;
              case SSUPPL_TYPE_IMPLICIT_POS :  op = EXP_OP_MBIT_POS;  break;
              case SSUPPL_TYPE_IMPLICIT_NEG :  op = EXP_OP_MBIT_NEG;  break;
              default                       :  assert( 0 );           break;
            }
            expr = expression_create( expt, expr, op, FALSE, curr_expr_id, 0, 0, 0, FALSE );
            curr_expr_id++;
            fsm_var_bind_add( sig->name, expr, funit_name );

          }

          if( expl != NULL ) {
            expl = expression_create( expr, expl, EXP_OP_LIST, FALSE, curr_expr_id, 0, 0, 0, FALSE );
            curr_expr_id++;
          } else {
            expl = expr;
          }

          /* Add signal name and expression to FSM var binding list */
          fsm_var_bind_add( sig->name, expr, funit_name );

        } Catch_anonymous {
          vsignal_dealloc( sig );
          expression_dealloc( expr, FALSE );
          // printf( "fsm_arg Throw A\n" ); - HIT
          Throw 0;
        }

        /* Deallocate signal */
        vsignal_dealloc( sig );

      } else {
        expression_dealloc( expl, FALSE );
        error = TRUE;
      }
    }
    if( !error ) {
      (*arg)++;
      expl = expression_create( expl, NULL, EXP_OP_CONCAT, FALSE, curr_expr_id, 0, 0, 0, FALSE );
      curr_expr_id++;
    }

  } else {

    if( (sig = vsignal_from_string( arg )) != NULL ) {

      Try {

        if( sig->value->width == 0 ) {

          expl = expression_create( NULL, NULL, EXP_OP_SIG, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;

        } else if( sig->value->width == 1 ) {

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          vector_from_int( expr->value, sig->dim[0].lsb );

          expl = expression_create( NULL, expr, EXP_OP_SBIT_SEL, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;

        } else {

          expt = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expt->value );
          expt->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          vector_from_int( expt->value, sig->dim[0].lsb );

          expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;
          vector_dealloc( expr->value );
          expr->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
          vector_from_int( expr->value, ((sig->value->width - 1) + sig->dim[0].lsb) );

          switch( sig->suppl.part.type ) {
            case SSUPPL_TYPE_IMPLICIT     :  op = EXP_OP_MBIT_SEL;  break;
            case SSUPPL_TYPE_IMPLICIT_POS :  op = EXP_OP_MBIT_POS;  break;
            case SSUPPL_TYPE_IMPLICIT_NEG :  op = EXP_OP_MBIT_NEG;  break;
            default                       :  assert( 0 );           break;
          }

          expl = expression_create( expt, expr, op, FALSE, curr_expr_id, 0, 0, 0, FALSE );
          curr_expr_id++;

        }

        /* Add signal name and expression to FSM var binding list */
        fsm_var_bind_add( sig->name, expl, funit_name );

      } Catch_anonymous {
        vsignal_dealloc( sig );
        expression_dealloc( expl, FALSE );
        printf( "fsm_arg Throw B\n" );
        Throw 0;
      }

      /* Deallocate signal */
      vsignal_dealloc( sig );

    } else {
      error = TRUE;
    }

  }

  /* Create statement for top-level expression, this statement will work like a continuous assignment */
  if( !error ) {
    stmt = statement_create( expl, NULL );
    stmt->suppl.part.head       = 1;
    stmt->suppl.part.stop_true  = 1;
    stmt->suppl.part.stop_false = 1;
    stmt->suppl.part.cont       = 1;
    stmt->next_true             = stmt;
    stmt->next_false            = stmt;
    fsm_var_stmt_add( stmt, funit_name );
  } else {
    expl = NULL;
  }

  PROFILE_END;

  return( expl );

}

/*!
 \throws anonymous Throw

 Parses specified argument string for FSM information.  If the FSM information
 is considered legal, returns TRUE; otherwise, returns FALSE.
*/
void fsm_arg_parse(
  const char* arg  /*!< Command-line argument following -F specifier */
) { PROFILE(FSM_ARG_PARSE);

  char*       tmp = strdup_safe( arg );  /* Temporary copy of given argument */
  char*       ptr = tmp;                 /* Pointer to current character in arg */
  expression* in_state;                  /* Pointer to input state expression */
  expression* out_state;                 /* Pointer to output state expression */

  Try {

    while( (*ptr != '\0') && (*ptr != '=') ) {
      ptr++;
    }

    if( *ptr == '\0' ) {

      print_output( "Option -F must specify a module/task/function/named block and one or two variables.  See \"covered score -h\" for more information.",
                    FATAL, __FILE__, __LINE__ );
      printf( "fsm_arg Throw C\n" );
      Throw 0;

    } else {

      *ptr = '\0';
      ptr++;

      if( (in_state = fsm_arg_parse_state( &ptr, tmp )) != NULL ) {

        if( *ptr == ',' ) {

          ptr++;

          if( (out_state = fsm_arg_parse_state( &ptr, tmp )) != NULL ) {
            (void)fsm_var_add( arg, in_state, out_state, NULL, FALSE );
          } else {
            print_output( "Illegal FSM command-line output state", FATAL, __FILE__, __LINE__ );
            printf( "fsm_arg Throw D\n" );
            Throw 0;
          }

        } else {

          /* Copy the current expression */
          (void)fsm_var_add( arg, in_state, in_state, NULL, FALSE );

        }

      } else {
  
        print_output( "Illegal FSM command-line input state", FATAL, __FILE__, __LINE__ );
        printf( "fsm_arg Throw E\n" );
        Throw 0;
 
      }

    }

  } Catch_anonymous {
    free_safe( tmp, (strlen( arg ) + 1) );
    printf( "fsm_arg Throw F\n" );
    Throw 0;
  }

  /* Deallocate temporary memory */
  free_safe( tmp, (strlen( arg ) + 1) );

  PROFILE_END;

}

/*!
 \return Returns a pointer to the expression created from the found value; otherwise,
         returns a value of NULL to indicate the this parser was unable to parse the
         specified transition value.

 \throws anonymous Throw expression_create expression_create expression_create expression_create expression_create expression_create
                   expression_create expression_create expression_create expression_create expression_create expression_create expression_create

 Parses specified string value for a parameter or constant value.  If the string
 is parsed correctly, a new expression is created to hold the contents of the
 parsed value and is returned to the calling function.  If the string is not
 parsed correctly, a value of NULL is returned to the calling function.
*/
static expression* fsm_arg_parse_value(
  char**           str,   /*!< Pointer to string containing parameter or constant value */
  const func_unit* funit  /*!< Pointer to functional unit containing this FSM */
) { PROFILE(FSM_ARG_PARSE_VALUE);

  expression* expr = NULL;   /* Pointer to expression containing state value */
  expression* left;          /* Left child expression */
  expression* right = NULL;  /* Right child expression */
  vector*     vec;           /* Pointer to newly allocated vector value */
  int         base;          /* Base of parsed string value */
  char        str_val[256];  /* String version of value parsed */
  int         msb;           /* Most-significant bit position of parameter */
  int         lsb;           /* Least-significant bit position of parameter */
  int         chars_read;    /* Number of characters read from sscanf() */
  mod_parm*   mparm;         /* Pointer to module parameter found */

  vector_from_string( str, FALSE, &vec, &base );

  if( vec != NULL ) {

    /* This value represents a static value, handle as such */
    Try {
      expr = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
    } Catch_anonymous {
      vector_dealloc( vec );
      printf( "fsm_arg Throw G\n" );
      Throw 0;
    }
    curr_expr_id++;

    vector_dealloc( expr->value );
    expr->value = vec;

  } else {

    /* This value should be a parameter value, parse it */
    if( sscanf( *str, "%[a-zA-Z0-9_]\[%d:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        vector_from_int( left->value, msb );

        /* Generate right child expression */
        Try {
          right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          printf( "fsm_arg Throw H\n" );
          Throw 0;
        }
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        vector_from_int( right->value, lsb );

        /* Generate multi-bit parameter expression */
        Try {
          expr = expression_create( right, left, EXP_OP_PARAM_MBIT, FALSE, curr_expr_id, 0, 0, 0, FALSE ); 
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          expression_dealloc( right, FALSE );
          printf( "fsm_arg Throw I\n" );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d+:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        vector_from_int( left->value, msb );

        /* Generate right child expression */
        Try {
          right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          printf( "fsm_arg Throw J\n" );
          Throw 0;
        }
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        vector_from_int( right->value, lsb );

        /* Generate variable positive multi-bit parameter expression */
        Try {
          expr = expression_create( right, left, EXP_OP_PARAM_MBIT_POS, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          expression_dealloc( right, FALSE );
          printf( "fsm_arg Throw K\n" );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d-:%d]%n", str_val, &msb, &lsb, &chars_read ) == 3 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        vector_from_int( left->value, msb );

        /* Generate right child expression */
        Try {
          right = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          printf( "fsm_arg Throw L\n" );
          Throw 0;
        }
        curr_expr_id++;
        vector_dealloc( right->value );
        right->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        vector_from_int( right->value, lsb );

        /* Generate variable positive multi-bit parameter expression */
        Try {
          expr = expression_create( right, left, EXP_OP_PARAM_MBIT_NEG, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          expression_dealloc( right, FALSE );
          printf( "fsm_arg Throw M\n" );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]\[%d]%n", str_val, &lsb, &chars_read ) == 2 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate left child expression */
        left = expression_create( NULL, NULL, EXP_OP_STATIC, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        vector_dealloc( left->value );
        left->value = vector_create( 32, VTYPE_VAL, VDATA_UL, TRUE );
        vector_from_int( left->value, lsb );

        /* Generate single-bit parameter expression */
        Try {
          expr = expression_create( NULL, left, EXP_OP_PARAM_SBIT, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        } Catch_anonymous {
          expression_dealloc( left, FALSE );
          printf( "fsm_arg Throw N\n" );
          Throw 0;
        }
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else if( sscanf( *str, "%[a-zA-Z0-9_]%n", str_val, &chars_read ) == 1 ) {
      *str = *str + chars_read;
      if( (mparm = mod_parm_find( str_val, funit->param_head )) != NULL ) {

        /* Generate parameter expression */
        expr = expression_create( NULL, NULL, EXP_OP_PARAM, FALSE, curr_expr_id, 0, 0, 0, FALSE );
        curr_expr_id++;
        exp_link_add( expr, &(mparm->exp_head), &(mparm->exp_tail) );

      }
    } else {
      expr = NULL;
    }

  }

  PROFILE_END;

  return( expr );

}

/*!
 \throws anonymous Throw

 \par
 Parses a transition string carried in the specified expression argument.  All transitions
 must be in the form of:

 \par
 (\\<parameter\\>|\\<constant_value\\>)->(\\<parameter\\>|\\<constant_value\\>)

 \par
 Each transition is then added to the specified FSM table's arc list which is added to the
 FSM arc transition table when the fsm_create_tables() function is called.
*/
static void fsm_arg_parse_trans(
  expression*      expr,   /*!< Pointer to expression containing string value in vector value array */
  fsm*             table,  /*!< Pointer to FSM table to add the transition arcs to */
  const func_unit* funit   /*!< Pointer to the functional unit that contains the specified FSM */
) { PROFILE(FSM_ARG_PARSE_TRANS);

  expression* from_state;  /* Pointer to from_state value of transition */
  expression* to_state;    /* Pointer to to_state value of transition */
  char*       str;         /* String version of expression value */
  char*       tmp;         /* Temporary pointer to string */

  assert( expr != NULL );

  /* Convert expression value to a string */
  tmp = str = vector_to_string( expr->value, ESUPPL_STATIC_BASE( expr->suppl ), FALSE );

  Try {

    if( (from_state = fsm_arg_parse_value( &str, funit )) == NULL ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Left-hand side FSM transition value must be a constant value or parameter, line: %d, file: %s",
                                  expr->line, obf_file( funit->filename ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      printf( "fsm_arg Throw O\n" );
      Throw 0;
    } else {

      if( (str[0] != '-') || (str[1] != '>') ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "FSM transition values must contain the string '->' between them, line: %d, file: %s",
                                    expr->line, obf_file( funit->filename ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "fsm_arg Throw P\n" );
        Throw 0;
      } else {
        str += 2;
      }

      if( (to_state = fsm_arg_parse_value( &str, funit )) == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Right-hand side FSM transition value must be a constant value or parameter, line: %d, file: %s",
                                    expr->line, obf_file( funit->filename ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "fsm_arg Throw Q\n" );
        Throw 0;
      } else {

        /* Add both expressions to FSM arc list */
        fsm_add_arc( table, from_state, to_state );

      }

    }

  } Catch_anonymous {
    free_safe( tmp, (strlen( tmp ) + 1) );
    printf( "fsm_arg Throw R\n" );
    Throw 0;
  }

  /* Deallocate string */
  free_safe( tmp, (strlen( tmp ) + 1) );

  PROFILE_END;

}

/*!
 \throws anonymous Throw Throw Throw Throw Throw Throw Throw Throw fsm_arg_parse_trans

 Parses the specified attribute parameter for validity and updates FSM structure
 accordingly.
*/
void fsm_arg_parse_attr(
  attr_param*      ap,      /*!< Pointer to attribute parameter list */
  const func_unit* funit,   /*!< Pointer to functional unit containing this attribute */
  bool             exclude  /*!< If TRUE, sets the exclude bits in the FSM */
) { PROFILE(FSM_ARG_PARSE_ATTR);

  attr_param* curr;               /* Pointer to current attribute parameter in list */
  fsm_link*   fsml      = NULL;   /* Pointer to found FSM structure */
  int         index     = 1;      /* Current index number in list */
  bool        ignore    = FALSE;  /* Set to TRUE if we should ignore this attribute */
  expression* in_state  = NULL;   /* Pointer to input state */
  expression* out_state = NULL;   /* Pointer to output state */
  char*       str;                /* Temporary holder for string value */
  char*       tmp;                /* Temporary holder for string value */

  curr = ap;
  while( (curr != NULL) && !ignore ) {

    /* This name is the name of the FSM structure to update */
    if( index == 1 ) {
      if( curr->expr != NULL ) {
        ignore = TRUE;
      } else {
        fsml = fsm_link_find( curr->name, funit->fsm_head );
      }
    } else if( (index == 2) && (strcmp( curr->name, "is" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        int slen;
        tmp = str = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE );
        slen = strlen( tmp );
        Try {
          if( (in_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal input state expression (%s), file: %s", str, obf_file( funit->filename ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "fsm_arg Throw S\n" );
            Throw 0;
          }
        } Catch_anonymous {
          free_safe( tmp, (slen + 1) );
          printf( "fsm_arg Throw S.1\n" );
          Throw 0;
        }
        free_safe( tmp, (slen + 1) );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Input state specified after output state for this FSM has already been specified, file: %s",
                                    obf_file( funit->filename ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "fsm_arg Throw T\n" );
        Throw 0;
      }
    } else if( (index == 2) && (strcmp( curr->name, "os" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        int slen;
        tmp = str = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE );
        slen = strlen( tmp );
        Try {
          if( (out_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, obf_file( funit->filename ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "fsm_arg Throw U\n" );
            Throw 0;
          } else {
            (void)fsm_var_add( funit->name, out_state, out_state, curr->name, exclude );
            fsml = fsm_link_find( curr->name, funit->fsm_head );
          }
        } Catch_anonymous {
          free_safe( tmp, (slen + 1) );
          printf( "fsm_arg Throw U.1\n" );
          Throw 0;
        }
        free_safe( tmp, (slen + 1) );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                                    obf_file( funit->filename ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "fsm_arg Throw V\n" );
        Throw 0;
      }
    } else if( (index == 3) && (strcmp( curr->name, "os" ) == 0) && (out_state == NULL) &&
               (in_state != NULL) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        int slen;
        tmp = str = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE );
        slen = strlen( tmp );
        Try {
          if( (out_state = fsm_arg_parse_state( &str, funit->name )) == NULL ) {
            unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Illegal output state expression (%s), file: %s", str, obf_file( funit->filename ) );
            assert( rv < USER_MSG_LENGTH );
            print_output( user_msg, FATAL, __FILE__, __LINE__ );
            printf( "fsm_arg Throw W\n" );
            Throw 0;
          } else {
            (void)fsm_var_add( funit->name, in_state, out_state, curr->name, exclude );
            fsml = fsm_link_find( curr->name, funit->fsm_head );
          }
        } Catch_anonymous {
          free_safe( tmp, (slen + 1) );
          // printf( "fsm_arg Throw W.1\n" ); - HIT
          Throw 0;
        }
        free_safe( tmp, (slen + 1) );
      } else {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Output state specified after output state for this FSM has already been specified, file: %s",
                                    obf_file( funit->filename ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "fsm_arg Throw X\n" );
        Throw 0;
      }
    } else if( (index > 1) && (strcmp( curr->name, "trans" ) == 0) && (curr->expr != NULL) ) {
      if( fsml == NULL ) {
        unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Attribute FSM name (%s) has not been previously created, file: %s",
                                    obf_sig( curr->name ), obf_file( funit->filename ) );
        assert( rv < USER_MSG_LENGTH );
        print_output( user_msg, FATAL, __FILE__, __LINE__ );
        printf( "fsm_arg Throw Y\n" );
        Throw 0;
      } else {
        fsm_arg_parse_trans( curr->expr, fsml->table, funit );
      }
    } else {
      unsigned int rv;
      tmp = vector_to_string( curr->expr->value, ESUPPL_STATIC_BASE( curr->expr->suppl ), FALSE );
      rv = snprintf( user_msg, USER_MSG_LENGTH, "Invalid covered_fsm attribute parameter (%s=%s), file: %s",
                     curr->name, tmp, obf_file( funit->filename ) );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, FATAL, __FILE__, __LINE__ );
      free_safe( tmp, (strlen( tmp ) + 1) );
      printf( "fsm_arg Throw Z\n" );
      Throw 0;
    }

    /* We need to work backwards in attribute parameter lists */
    curr = curr->prev;
    index++;
    
  }

  PROFILE_END;

}


/*
 $Log$
 Revision 1.52.2.1  2008/07/10 22:43:51  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.53  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.52  2008/05/30 05:38:30  phase1geo
 Updating development tree with development branch.  Also attempting to fix
 bug 1965927.

 Revision 1.51.2.3  2008/05/28 05:57:10  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.51.2.2  2008/05/07 21:09:10  phase1geo
 Added functionality to allow to_string to output full vector bits (even
 non-significant bits) for purposes of reporting for FSMs (matches original
 behavior).

 Revision 1.51.2.1  2008/04/22 23:01:42  phase1geo
 More updates.  Completed initial pass of expr.c and fsm_arg.c.  Working
 on memory.c.  Checkpointing.

 Revision 1.51  2008/04/01 23:08:20  phase1geo
 More updates for error diagnostic cleanup.  Full regression still not
 passing (but is getting close).

 Revision 1.50  2008/03/28 21:11:32  phase1geo
 Fixing memory leak issues with -ep option and embedded FSM attributes.

 Revision 1.49  2008/03/26 21:29:31  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.48  2008/03/18 05:36:04  phase1geo
 More updates (regression still broken).

 Revision 1.47  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.46  2008/03/14 22:00:18  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.45  2008/03/11 22:06:47  phase1geo
 Finishing first round of exception handling code.

 Revision 1.44  2008/03/04 06:46:48  phase1geo
 More exception handling updates.  Still work to go.  Checkpointing.

 Revision 1.43  2008/02/29 00:08:31  phase1geo
 Completed optimization code in simulator.  Still need to verify that code
 changes enhanced performances as desired.  Checkpointing.

 Revision 1.42  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.41  2008/02/22 20:39:22  phase1geo
 More updates for exception handling.

 Revision 1.40  2008/02/09 19:32:44  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.39  2008/02/01 06:37:08  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.38  2008/01/15 23:01:15  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.37  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.36  2008/01/09 05:22:21  phase1geo
 More splint updates using the -standard option.

 Revision 1.35  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.34  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.33  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.32  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.31  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.30  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.29  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.28  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.27  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.26  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.25  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.24  2005/12/23 05:41:52  phase1geo
 Fixing several bugs in score command per bug report #1388339.  Fixed problem
 with race condition checker statement iterator to eliminate infinite looping (this
 was the problem in the original bug).  Also fixed expression assigment when static
 expressions are used in the LHS (caused an assertion failure).  Also fixed the race
 condition checker to properly pay attention to task calls, named blocks and fork
 statements to make sure that these are being handled correctly for race condition
 checking.  Fixed bug for signals that are on the LHS side of an assignment expression
 but is not being assigned (bit selects) so that these are NOT considered for race
 conditions.  Full regression is a bit broken now but the opened bug can now be closed.

 Revision 1.23  2005/12/21 22:30:54  phase1geo
 More updates to memory leak fix list.  We are getting close!  Added some helper
 scripts/rules to more easily debug valgrind memory leak errors.  Also added suppression
 file for valgrind for a memory leak problem that exists in lex-generated code.

 Revision 1.22  2005/12/08 19:47:00  phase1geo
 Fixed repeat2 simulation issues.  Fixed statement_connect algorithm, removed the
 need for a separate set_stop function and reshuffled the positions of esuppl bits.
 Full regression passes.

 Revision 1.21  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.20  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.19  2005/01/07 23:00:10  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.18  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.17  2005/01/06 23:51:17  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.16  2004/04/19 04:54:56  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.15  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.14  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.13  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.12  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.11  2003/11/15 04:25:02  phase1geo
 Fixing syntax error found in Doxygen.

 Revision 1.10  2003/11/07 05:18:40  phase1geo
 Adding working code for inline FSM attribute handling.  Full regression fails
 at this point but the code seems to be working correctly.

 Revision 1.9  2003/10/28 13:28:00  phase1geo
 Updates for more FSM attribute handling.  Not quite there yet but full regression
 still passes.

 Revision 1.8  2003/10/28 01:09:38  phase1geo
 Cleaning up unnecessary output.

 Revision 1.7  2003/10/28 00:18:06  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.6  2003/10/19 05:13:26  phase1geo
 Updating user documentation for changes to FSM specification syntax.  Added
 new fsm5.3 diagnostic to verify concatenation syntax.  Fixing bug in concatenation
 syntax handling.

 Revision 1.5  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.4  2003/10/13 12:27:25  phase1geo
 More fixes to FSM stuff.

 Revision 1.3  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.2  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.1  2003/10/02 12:30:56  phase1geo
 Initial code modifications to handle more robust FSM cases.
*/

