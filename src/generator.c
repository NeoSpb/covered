/*
 Copyright (c) 2006-2009 Trevor Williams

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program;
 if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "codegen.h"
#include "db.h"
#include "defines.h"
#include "expr.h"
#include "func_iter.h"
#include "func_unit.h"
#include "generator.h"
#include "gen_item.h"
#include "link.h"
#include "ovl.h"
#include "param.h"
#include "profiler.h"
#include "util.h"


extern void reset_lexer_for_generation(
  const char* in_fname,  /*!< Name of file to read */
  const char* out_dir    /*!< Output directory name */
);
extern int VLparse();

extern db**           db_list;
extern unsigned int   curr_db;
extern char           user_msg[USER_MSG_LENGTH];
extern str_link*      modlist_head;
extern str_link*      modlist_tail;
extern const exp_info exp_op_info[EXP_OP_NUM];
extern bool           debug_mode;
extern func_unit*     curr_funit;
extern int            generate_mode;
extern isuppl         info_suppl;
extern unsigned int   inline_comb_depth;


struct fname_link_s;
typedef struct fname_link_s fname_link;
struct fname_link_s {
  char*       filename;    /*!< Filename associated with all functional units */
  func_unit*  next_funit;  /*!< Pointer to the next/current functional unit that will be parsed */
  funit_link* head;        /*!< Pointer to head of functional unit list */
  funit_link* tail;        /*!< Pointer to tail of functional unit list */
  fname_link* next;        /*!< Pointer to next filename list */
};

struct replace_info_s;
typedef struct replace_info_s replace_info;
struct replace_info_s {
  char*     word_ptr;
  str_link* list_ptr;
};

struct reg_insert_s;
typedef struct reg_insert_s reg_insert;
struct reg_insert_s {
  str_link*   ptr;         /*!< Pointer to string link to insert register information after */
  reg_insert* next;        /*!< Pointer to the next reg_insert structure in the stack */
};


/*!
 Pointer to functional unit stack.
*/
funit_link* funit_top = NULL;

/*!
 Pointer to the current output file.
*/
FILE* curr_ofile = NULL;

/*!
 Temporary holding buffer for code to be output.
*/
char work_buffer[4096];

/*!
 Pointer to head of hold buffer list.
*/
str_link* work_head = NULL;

/*!
 Pointer to tail of hold buffer list.
*/
str_link* work_tail = NULL;

/*!
 Temporary holding buffer for code to be output.
*/
char hold_buffer[4096];

/*!
 Pointer to head of hold buffer list.
*/
str_link* hold_head = NULL;

/*!
 Pointer to tail of hold buffer list.
*/
str_link* hold_tail = NULL;

/*!
 Pointer to the top of the register insertion stack.
*/
reg_insert* reg_top = NULL;

/*!
 Pointer to head of list for combinational coverage lines.
*/
str_link* comb_head = NULL;

/*!
 Pointer to tail of list for combinational coverage lines.
*/
str_link* comb_tail = NULL;

/*!
 Specifies that we should handle the current functional unit as an assertion.
*/
bool handle_funit_as_assert = FALSE;

/*!
 Iterator for current functional unit.
*/
static func_iter fiter;

/*!
 Pointer to current statement (needs to be set to NULL at the beginning of each module).
*/
static statement* curr_stmt;

/*!
 Pointer to top of statement stack (statement stack's are used for saving found statements that need to
 be reused at a later time.  We are reusing the stmt_loop_link structure which does not match our needs
 exactly but is close enough.
*/
static stmt_loop_link* stmt_stack = NULL;

/*!
 Stores replacement string information for the first character of the potential replacement string.
*/
static replace_info replace_first;

/*!
 Stores replacement string information for the last character of the potential replacement string.
*/
static replace_info replace_last;

/*!
 If replace_ptr is not set to NULL, this value contains the first line of the replacement string.
*/
static unsigned int replace_first_line;

/*!
 If replace_ptr is not set to NULL, this value contains the first column of the replacement string.
*/
static unsigned int replace_first_col;


static char* generator_gen_size(
  expression* exp,
  func_unit*  funit,
  int*        number
);

/*!
 Outputs the current state of the code lists to standard output (for debugging purposes only).
*/
void generator_display() { PROFILE(GENERATOR_DISPLAY);

  str_link* strl;

  printf( "----------------------------------------------------------------\n" );
  printf( "Holding code list (%p %p):\n", hold_head, hold_tail );
  strl = hold_head;
  while( strl != NULL ) {
    printf( "    %s\n", strl->str );
    strl = strl->next;
  }
  printf( "Holding buffer:\n  %s\n", hold_buffer );

  printf( "Working code list (%p %p):\n", work_head, work_tail );
  strl = work_head;
  while( strl != NULL ) {
    printf( "    %s\n", strl->str );
    strl = strl->next;
  }
  printf( "Working buffer:\n  %s\n", work_buffer );

  PROFILE_END;

}

/*!
 \return Returns allocated string containing the difference in scope between the current functional unit
         and the specified child functional unit.
*/
static char* generator_get_relative_scope(
  func_unit* child  /*!< Pointer to child functional unit */
) { PROFILE(GENERATOR_GET_RELATIVE_SCOPE);

  char* back = strdup_safe( child->name );
  char* relative_scope;

  scope_extract_scope( child->name, funit_top->funit->name, back );
  relative_scope = strdup_safe( back );

  free_safe( back, (strlen( child->name ) + 1) );

  PROFILE_END;

  return( relative_scope );

}

/*!
 Clears the first and last replace information pointers.
*/
void generator_clear_replace_ptrs() { PROFILE(GENERATOR_CLEAR_REPLACE_PTRS);

  /* Clear the first pointers */
  replace_first.word_ptr = NULL;
  replace_first.list_ptr = NULL;

  /* Clear the last pointers */
  replace_last.word_ptr = NULL;
  replace_last.list_ptr = NULL;

  PROFILE_END;

}

/*!
 \return Returns TRUE if the given functional unit is within a static-only function; otherwise, returns FALSE.
*/
static bool generator_is_static_function_only(
  func_unit* funit  /*!< Pointer to functional unit to check */
) { PROFILE(GENERATOR_IS_STATIC_FUNCTION_ONLY);

  func_unit* func = funit_get_curr_function( funit );

  PROFILE_END;

  return( (func != NULL) && (func->suppl.part.staticf == 1) && (func->suppl.part.normalf == 0) );

}

/*!
 \return Returns TRUE if the given functional unit is within a static function; otherwise, returns FALSE.
*/
bool generator_is_static_function(
  func_unit* funit  /*!< Pointer to functional unit to check */
) { PROFILE(GENERATOR_IS_STATIC_FUNCTION);

  func_unit* func = funit_get_curr_function( funit );

  PROFILE_END;

  return( (func != NULL) && (func->suppl.part.staticf == 1) );

}

/*!
 Replaces a portion (as specified by the line and column inputs) of the currently
 marked code segment (marking occurs automatically) with the given string.
*/
void generator_replace(
  const char*  str,           /*!< String to replace the original */
  unsigned int first_line,    /*!< Line number of start of code to replace */
  unsigned int first_column,  /*!< Column number of start of code to replace */
  unsigned int last_line,     /*!< Line number of end of code to replace */
  unsigned int last_column    /*!< Column number of end of code to replace */
) { PROFILE(GENERATOR_REPLACE);

  /* We can only perform the replacement if something has been previously marked for replacement */
  if( replace_first.word_ptr != NULL ) {

    /* Go to starting line */
    while( first_line > replace_first_line ) {
      replace_first.list_ptr = replace_first.list_ptr->next;
      if( replace_first.list_ptr == NULL ) {
        assert( first_line == (replace_first_line + 1) );
        replace_first.word_ptr = work_buffer;
      } else {
        replace_first.word_ptr = replace_first.list_ptr->str;
      }
      replace_first_col = 0;
      replace_first_line++;
    }

    /* Remove original code */
    if( first_line == last_line ) {

      /* If the line exists in the work buffer, replace the working buffer */
      if( replace_first.list_ptr == NULL ) {

        char keep_end[4096];
        strcpy( keep_end, (replace_first.word_ptr + (last_column - replace_first_col) + 1) );

        /* Chop the working buffer */
        *(replace_first.word_ptr + (first_column - replace_first_col)) = '\0';

        /* Append the new string */
        if( (strlen( work_buffer ) + strlen( str )) < 4095 ) {
          strcat( work_buffer, str );
        } else {
          (void)str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );
          strcpy( work_buffer, str );
        }

        /* Now append the end of the working buffer */
        if( (strlen( work_buffer ) + strlen( keep_end )) < 4095 ) {
          replace_first.word_ptr = work_buffer + strlen( work_buffer );
          strcat( work_buffer, keep_end );
        } else {
          (void)str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );
          strcpy( work_buffer, keep_end );
          replace_first.word_ptr = work_buffer;
        }

        /* Adjust the first replacement column to match the end of the newly inserted string */
        replace_first_col += (first_column - replace_first_col) + ((last_column - first_column) + 1);

      /* Otherwise, the line exists in the working list, so replace it there */
      } else {

        unsigned int keep_begin_len = (replace_first.word_ptr + (first_column - replace_first_col)) - replace_first.list_ptr->str;
        char*        keep_begin_str = (char*)malloc_safe( keep_begin_len + 1 );
        char*        keep_end_str   = strdup_safe( replace_first.word_ptr + (last_column - replace_first_col) + 1 );

        strncpy( keep_begin_str, replace_first.list_ptr->str, keep_begin_len );
        keep_begin_str[keep_begin_len] = '\0';
        free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
        replace_first.list_ptr->str = (char*)malloc_safe( keep_begin_len + strlen( str ) + strlen( keep_end_str ) + 1 );
        strcpy( replace_first.list_ptr->str, keep_begin_str );
        strcat( replace_first.list_ptr->str, str );
        strcat( replace_first.list_ptr->str, keep_end_str );

        free_safe( keep_begin_str, (strlen( keep_begin_str ) + 1) );
        free_safe( keep_end_str,   (strlen( keep_end_str ) + 1) );

      }
    
    } else {

      unsigned int keep_len       = (replace_first.word_ptr + (first_column - replace_first_col)) - replace_first.list_ptr->str;
      char*        keep_str       = (char*)malloc_safe( keep_len + strlen( str ) + 1 );
      str_link*    first_list_ptr = replace_first.list_ptr; 

      /*
       First, let's concat the kept code on the current line with the new code and replace
       the current line with the new string.
      */
      strncpy( keep_str, replace_first.list_ptr->str, keep_len );
      keep_str[keep_len] = '\0';
      strcat( keep_str, str );
      free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
      replace_first.list_ptr->str = keep_str;

      /* Now remove the rest of the replaced code and adjust the replacement pointers as needed */
      first_list_ptr         = replace_first.list_ptr;
      replace_first.list_ptr = replace_first.list_ptr->next;
      replace_first_line++;
      while( replace_first_line < last_line ) {
        str_link* next = replace_first.list_ptr->next;
        free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
        free_safe( replace_first.list_ptr, sizeof( str_link ) );
        replace_first.list_ptr = next;
        replace_first_line++;
      }
      first_list_ptr->next = replace_first.list_ptr;

      /* Remove the last line portion from the buffer if the last line is there */
      if( replace_first.list_ptr == NULL ) {
        char tmp_buffer[4096];
        strcpy( tmp_buffer, (work_buffer + last_column + 1) );
        strcpy( work_buffer, tmp_buffer );
        replace_first.word_ptr = work_buffer;
        replace_first_col      = last_column + 1;
        work_tail              = first_list_ptr;

      /* Otherwise, remove the last line portion from the list */
      } else {
        char* tmp_str = strdup_safe( replace_first.list_ptr->str + last_column + 1 );
        free_safe( replace_first.list_ptr->str, (strlen( replace_first.list_ptr->str ) + 1) );
        replace_first.list_ptr->str = tmp_str;
        replace_first.word_ptr      = tmp_str;
        replace_first_col           = last_column + 1;

      }

    }

  }

  PROFILE_END;

}

/*!
 Allocates and initializes a new structure for the purposes of coverage register insertion.
 Called by parser. 
*/
void generator_push_reg_insert() { PROFILE(GENERATOR_PUSH_REG_INSERT);

  reg_insert* ri;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    print_output( "In generator_push_reg_insert", DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Make sure that the hold buffer is added to the hold list */
  if( hold_buffer[0] != '\0' ) {
    strcat( hold_buffer, "\n" );
    (void)str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
    hold_buffer[0] = '\0';
  }

  /* Allocate and initialize the new register insertion structure */
  ri       = (reg_insert*)malloc_safe( sizeof( reg_insert ) );
  ri->ptr  = hold_tail;
  ri->next = reg_top;

  /* Point the register stack top to the newly created register insert structure */
  reg_top = ri;

  PROFILE_END;

}

/*!
 Pops the head of the register insertion stack.  Called by parser.
*/
void generator_pop_reg_insert() { PROFILE(GENERATOR_POP_REG_INSERT);

  reg_insert* ri;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    print_output( "In generator_pop_reg_insert", DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* Save pointer to the top reg_insert structure and adjust reg_top */
  ri      = reg_top;
  reg_top = ri->next;

  /* Deallocate the reg_insert structure */
  free_safe( ri, sizeof( reg_insert ) );

}

#ifdef OBSOLETE
/*!
 \return Returns TRUE if the reg_top stack contains exactly one element.
*/
static bool generator_is_reg_top_one_deep() { PROFILE(GENERATOR_IS_BASE_REG_INSERT);

  bool retval = (reg_top != NULL) && (reg_top->next == NULL);

  PROFILE_END;

  return( retval );

}
#endif

/*!
 Inserts the given register instantiation string into the appropriate spot in the hold buffer.
*/
static void generator_insert_reg(
  const char* str  /*!< Register instantiation string to insert */
) { PROFILE(GENERATOR_INSERT_REG);

  str_link* tmp_head = NULL;
  str_link* tmp_tail = NULL;

  assert( reg_top != NULL );

  /* Create string link */
  (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );

  /* Insert it at the insertion point */
  if( reg_top->ptr == NULL ) {
    tmp_head->next = hold_head;
    if( hold_head == NULL ) {
      hold_head = hold_tail = tmp_head;
    } else {
      hold_head = tmp_head;
    }
  } else {
    tmp_head->next     = reg_top->ptr->next;
    reg_top->ptr->next = tmp_head;
    if( hold_tail == reg_top->ptr ) {
      hold_tail = tmp_head;
    }
  }
 
  PROFILE_END;

}

/*!
 Pushes the given functional unit to the top of the functional unit stack.
*/
void generator_push_funit(
  func_unit* funit  /*!< Pointer to the functional unit to push onto the stack */
) { PROFILE(GENERATOR_PUSH_FUNIT);

  funit_link* tmp_head = NULL;
  funit_link* tmp_tail = NULL;

  /* Create a functional unit link */
  funit_link_add( funit, &tmp_head, &tmp_tail );

  /* Set the new functional unit link to the top of the stack */
  tmp_head->next = funit_top;
  funit_top      = tmp_head;

  PROFILE_END;

}

/*!
 Pops the top of the functional unit stack and deallocates it.
*/
void generator_pop_funit() { PROFILE(GENERATOR_POP_FUNIT);

  funit_link* tmp = funit_top;

  assert( tmp != NULL );

  funit_top = funit_top->next;

  free_safe( tmp, sizeof( funit_link ) );

  PROFILE_END;

}
  

/*!
 \return Returns TRUE if the specified expression is one that requires a substitution.
*/
bool generator_expr_needs_to_be_substituted(
  expression* exp  /*!< Pointer to expression to evaluate */
) { PROFILE(GENERATOR_EXPR_NEEDS_TO_BE_SUBSTITUTED);

  bool retval = (exp->op == EXP_OP_SRANDOM)      ||
                (exp->op == EXP_OP_SURANDOM)     ||
                (exp->op == EXP_OP_SURAND_RANGE) ||
                (exp->op == EXP_OP_SVALARGS);

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the given expression requires coverage output to be generated.
*/
static bool generator_expr_cov_needed(
  expression*  exp,   /*!< Pointer to expression to evaluate */
  unsigned int depth  /*!< Expression depth of the given expression */
) { PROFILE(GENERATOR_EXPR_COV_NEEDED);

  bool retval = (depth < inline_comb_depth) && (EXPR_IS_MEASURABLE( exp ) == 1) && !expression_is_static_only( exp );

  PROFILE_END;

  return( retval );

}

/*!
 \return Returns TRUE if the current expression needs to be substituted with an expression name.
*/
bool generator_expr_name_needed(
  expression* exp  /*!< Pointer to expression to evaluate */
) {

  return( exp->suppl.part.comb_cntd == 1 );

}

/*!
 Recursively clears the comb_cntd bits in the given expression tree for all expressions that do not require
 substitution.
*/
void generator_clear_comb_cntd(
  expression* exp  /*!< Pointer to expression to clear */
) { PROFILE(GENERATOR_CLEAR_COMB_CNTD);

  if( exp != NULL ) {

    generator_clear_comb_cntd( exp->left );
    generator_clear_comb_cntd( exp->right );

    /* Only clear the comb_cntd bit if it does not require substitution */
    if( exp->suppl.part.eval_t ) {
      exp->suppl.part.eval_t    = 0;
      exp->suppl.part.comb_cntd = 0;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns the name of the given expression in a string form that represents the physical attributes
         of the given expression for the purposes of lookup.  Each name is guaranteed to be unique.  The
         returned string is allocated and must be deallocated by the calling function.
*/
char* generator_create_expr_name(
  expression*  exp  /*!< Pointer to expression */
) { PROFILE(GENERATOR_CREATE_EXPR_NAME);

  char*        name;
  unsigned int slen;
  unsigned int rv;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char         op[30];
  char         fline[30];
  char         lline[30];
  char         col[30];

  assert( exp != NULL );

  /* Create string versions of op, first line, last line and column information */
  rv = snprintf( op,    30, "%x", exp->op );           assert( rv < 30 );
  rv = snprintf( fline, 30, "%u", exp->ppline );       assert( rv < 30 );
  rv = snprintf( lline, 30, "%u", last_exp->ppline );  assert( rv < 30 );
  rv = snprintf( col,   30, "%x", exp->col );          assert( rv < 30 );

  /* Allocate and create the unique name */
  slen = 11 + strlen( op ) + 1 + strlen( fline ) + 1 + strlen( lline ) + 1 + strlen( col ) + 2;
  name = (char*)malloc_safe( slen );
  rv   = snprintf( name, slen, " \\covered$X%s_%s_%s_%s ", op, fline, lline, col );
  assert( rv < slen );

  PROFILE_END;

  return( name );

}

/*!
 Populates the specified filename list with the functional unit list, sorting all functional units with the
 same filename together in the same link.
*/
static void generator_create_filename_list(
  funit_link*  funitl,  /*!< Pointer to the head of the functional unit linked list */
  fname_link** head,    /*!< Pointer to the head of the filename list to populate */
  fname_link** tail     /*!< Pointer to the tail of the filename list to populate */
) { PROFILE(GENERATOR_SORT_FUNIT_BY_FILENAME);

  while( funitl != NULL ) {

    /* Only add modules that are not the $root "module" */
    if( (funitl->funit->suppl.part.type == FUNIT_MODULE) && (strncmp( "$root", funitl->funit->name, 5 ) != 0) ) {

      fname_link* fnamel      = *head;
      const char* funit_fname = (funitl->funit->incl_fname != NULL) ? funitl->funit->incl_fname : funitl->funit->orig_fname;

      /* Search for functional unit filename in our filename list */
      while( (fnamel != NULL) && (strcmp( fnamel->filename, funit_fname ) != 0) ) {
        fnamel = fnamel->next;
      }

      /* If the filename link was not found, create a new link */
      if( fnamel == NULL ) {

        /* Allocate and initialize the filename link */
        fnamel             = (fname_link*)malloc_safe( sizeof( fname_link ) );
        fnamel->filename   = strdup_safe( funit_fname );
        fnamel->next_funit = funitl->funit;
        fnamel->head       = NULL;
        fnamel->tail       = NULL;
        fnamel->next       = NULL;

        /* Add the new link to the list */
        if( *head == NULL ) {
          *head = *tail = fnamel;
        } else {
          (*tail)->next = fnamel;
          *tail         = fnamel;
        }

      /*
       Otherwise, if the filename was found, compare the start_line of the next_funit to this functional unit's first_line.
       If the current functional unit's start_line is less, set next_funit to point to this functional unit.
      */
      } else {

        if( fnamel->next_funit->start_line > funitl->funit->start_line ) {
          fnamel->next_funit = funitl->funit;
        }

      }

      /* Add the current functional unit to the filename functional unit list */
      funit_link_add( funitl->funit, &(fnamel->head), &(fnamel->tail) );

    }

    /* Advance to the next functional unit */
    funitl = funitl->next;

  }
 
  PROFILE_END;

}

#ifdef OBSOLETE
/*!
 \return Returns TRUE if next functional unit exists; otherwise, returns FALSE.

 This function should be called whenever we see an endmodule of a covered module.
*/
static bool generator_set_next_funit(
  fname_link* curr  /*!< Pointer to the current filename link being worked on */
) { PROFILE(GENERATOR_SET_NEXT_FUNIT);

  func_unit*  next_funit = NULL;
  funit_link* funitl     = curr->head;

  /* Find the next functional unit */
  while( funitl != NULL ) {
    if( (funitl->funit->start_line >= curr->next_funit->end_line) &&
        ((next_funit == NULL) || (funitl->funit->start_line < next_funit->start_line)) ) {
      next_funit = funitl->funit;
    }
    funitl = funitl->next;
  }

  /* Only change the next_funit if we found a next functional unit (we don't want this value to ever be NULL) */
  if( next_funit != NULL ) {
    curr->next_funit = next_funit;
  }

  PROFILE_END;

  return( next_funit != NULL );

}
#endif

/*!
 Deallocates all memory allocated for the given filename linked list.
*/
static void generator_dealloc_filename_list(
  fname_link* head  /*!< Pointer to filename list to deallocate */
) { PROFILE(GENERATOR_DEALLOC_FNAME_LIST);

  fname_link* tmp;

  while( head != NULL ) {
    
    tmp  = head;
    head = head->next;

    /* Deallocate the filename */
    free_safe( tmp->filename, (strlen( tmp->filename ) + 1) );

    /* Deallocate the functional unit list (without deallocating the functional units themselves) */
    funit_link_delete_list( &(tmp->head), &(tmp->tail), FALSE );

    /* Deallocate the link itself */
    free_safe( tmp, sizeof( fname_link ) );

  }

  PROFILE_END;

}

/*!
 Generates an instrumented version of a given functional unit.
*/
static void generator_output_funits(
  fname_link* head  /*!< Pointer to the head of the filename list */
) { PROFILE(GENERATOR_OUTPUT_FUNIT);

  while( head != NULL ) {

    char         filename[4096];
    unsigned int rv;
    funit_link*  funitl;

    /* Create the output file pathname */
    rv = snprintf( filename, 4096, "covered/verilog/%s", get_basename( head->filename ) );
    assert( rv < 4096 );

    /* Output the name to standard output */
    rv = snprintf( user_msg, USER_MSG_LENGTH, "Generating inlined coverage file \"%s\"", filename );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, NORMAL, __FILE__, __LINE__ );

    /* Populate the modlist list */
    funitl = head->head;
    while( funitl != NULL ) {
      (void)str_link_add( strdup_safe( funitl->funit->name ), &modlist_head, &modlist_tail );
      funitl = funitl->next;
    }

    /* Open the output file for writing */
    if( (curr_ofile = fopen( filename, "w" )) != NULL ) {

      /* Parse the original code and output inline coverage code */
      reset_lexer_for_generation( head->filename, "covered/verilog" );
      (void)VLparse();

      /* Flush the work and hold buffers */
      generator_flush_all;

      /* Close the output file */
      rv = fclose( curr_ofile );
      assert( rv == 0 );

    } else {

      rv = snprintf( user_msg, USER_MSG_LENGTH, "Unable to create generated Verilog file \"%s\"", filename );
      assert( rv < USER_MSG_LENGTH );
      Throw 0;

    }

    /* Reset the modlist list */
    str_link_delete_list( modlist_head );
    modlist_head = modlist_tail = NULL;

    head = head->next;

  }

  PROFILE_END;

}

/*!
 Outputs the covered portion of the design to the covered/verilog directory.
*/
void generator_output() { PROFILE(GENERATOR_OUTPUT);

  fname_link* fname_head = NULL;  /* Pointer to head of filename linked list */
  fname_link* fname_tail = NULL;  /* Pointer to tail of filename linked list */
  fname_link* fnamel;             /* Pointer to current filename link */

  /* Create the initial "covered" directory - TBD - this should be done prior to this function call */
  if( !directory_exists( "covered" ) ) {
    /*@-shiftimplementation@*/
    if( mkdir( "covered", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
    /*@=shiftimplementation@*/
      print_output( "Unable to create \"covered\" directory", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* If the "covered/verilog" directory exists, remove it */
  if( directory_exists( "covered/verilog" ) ) {
    if( system( "rm -rf covered/verilog" ) != 0 ) {
      print_output( "Unable to remove \"covered/verilog\" directory", FATAL, __FILE__, __LINE__ );
      Throw 0;
    }
  }

  /* Create "covered/verilog" directory */
  /*@-shiftimplementation@*/
  if( mkdir( "covered/verilog", (S_IRWXU | S_IRWXG | S_IRWXO) ) != 0 ) {
  /*@=shiftimplementation@*/
    print_output( "Unable to create \"covered/verilog\" directory", FATAL, __FILE__, __LINE__ );
    Throw 0;
  }

  /* Initialize the work_buffer and hold_buffer arrays */
  work_buffer[0] = '\0';
  hold_buffer[0] = '\0';

  /* Initialize the functional unit iter */
  fiter.sls  = NULL;
  fiter.sigs = NULL;

  /* Create the filename list from the functional unit list */
  generator_create_filename_list( db_list[curr_db]->funit_head, &fname_head, &fname_tail );

  /* Iterate through the covered files, generating coverage output along with the design information */
  generator_output_funits( fname_head );

  /* Deallocate memory from filename list */
  generator_dealloc_filename_list( fname_head );

  /* Deallocate the functional unit iterator */
  func_iter_dealloc( &fiter );

  PROFILE_END;

}

/*!
 Initializes and resets the functional unit iterator.
*/
void generator_init_funit(
  func_unit* funit  /*!< Pointer to current functional unit */
) { PROFILE(GENERATOR_INIT_FUNIT);

  /* Deallocate the functional unit iterator */
  func_iter_dealloc( &fiter );

  /* Initializes the functional unit iterator */
  func_iter_init( &fiter, funit, TRUE, FALSE, TRUE );

  /* Clear the current statement pointer */
  curr_stmt = NULL;

  /* Reset the replacement string information */
  generator_clear_replace_ptrs();

  /* Calculate if we need to handle this functional unit as an assertion or not */
  handle_funit_as_assert = (info_suppl.part.scored_assert == 1) && ovl_is_assertion_module( funit );

  PROFILE_END;

}

/*!
 Prepends the given string to the working code list/buffer.  This function is used by the parser
 to add code prior to the current working buffer without updating the holding buffer.
*/
void generator_prepend_to_work_code(
  const char* str  /*!< String to write */
) { PROFILE(GENERATOR_PREPEND_TO_WORK_CODE);

  char         tmpstr[4096];
  unsigned int rv;

  /* If the work list is empty, prepend to the work buffer */
  if( work_head == NULL ) {

    if( (strlen( work_buffer ) + strlen( str )) < 4095 ) {
      strcpy( tmpstr, work_buffer );
      rv = snprintf( work_buffer, 4096, "%s %s", str, tmpstr );
      assert( rv < 4096 );
    } else {
      (void)str_link_add( strdup_safe( str ), &work_head, &work_tail );
    }

  /* Otherwise, prepend the string to the head string of the work list */
  } else {

    if( (strlen( work_head->str ) + strlen( str )) < 4095 ) {
      strcpy( tmpstr, work_head->str );
      rv = snprintf( tmpstr, 4096, "%s %s", str, work_head->str );
      assert( rv < 4096 );
      free_safe( work_head->str, (strlen( work_head->str ) + 1) );
      work_head->str = strdup_safe( tmpstr );
    } else {
      str_link* tmp_head = NULL;
      str_link* tmp_tail = NULL;
      (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      if( work_head == NULL ) {
        work_head = work_tail = tmp_head;
      } else {
        tmp_tail->next = work_head;
        work_head      = tmp_head;
      }
    }

  }

  PROFILE_END;

}

/*!
 Adds the given string to the working code buffer.
*/
void generator_add_to_work_code(
               const char*  str,           /*!< String to write */
               unsigned int first_line,    /*!< First line of string from file */
               unsigned int first_column,  /*!< First column of string from file */
               bool         from_code,     /*!< Specifies if the string came from the code directly */
  /*@unused@*/ const char*  file,          /*!< Filename that called this function */
  /*@unused@*/ unsigned int line           /*!< Line number where this function call was performed from */
) { PROFILE(GENERATOR_ADD_TO_WORK_CODE);

  static bool semi_from_code_just_seen  = FALSE;
  static bool semi_inject_just_seen     = FALSE;
  static bool begin_from_code_just_seen = FALSE;
  static bool begin_inject_just_seen    = FALSE;
  static bool default_just_seen         = FALSE;
  bool        add                       = TRUE;

  /* Make sure that we remove back-to-back semicolons */
  if( strcmp( str, ";" ) == 0 ) {
    if( ((semi_from_code_just_seen || begin_from_code_just_seen) && !from_code) ||
        ((semi_inject_just_seen || begin_inject_just_seen || default_just_seen) && from_code) ) {
      add = FALSE;
    }
    if( from_code ) {
      semi_from_code_just_seen  = TRUE;
      semi_inject_just_seen     = FALSE;
    } else {
      semi_inject_just_seen     = TRUE;
      semi_from_code_just_seen  = FALSE;
    }
    begin_from_code_just_seen = FALSE;
    begin_inject_just_seen    = FALSE;
    default_just_seen        = FALSE;
  } else if( strcmp( str, " begin" ) == 0 ) {
    if( from_code ) {
      begin_from_code_just_seen = TRUE;
      begin_inject_just_seen    = FALSE;
    } else {
      begin_inject_just_seen    = TRUE;
      begin_from_code_just_seen = FALSE;
    }
    semi_from_code_just_seen = FALSE;
    semi_inject_just_seen    = FALSE;
    default_just_seen        = FALSE;
  } else if( strcmp( str, "default" ) == 0 ) {
    default_just_seen = TRUE;
  } else if( (str[0] != ' ') && (str[0] != '\n') && (str[0] != '\t') && (str[0] != '\r') && (str[0] != '\b') ) {
    semi_from_code_just_seen  = FALSE;
    semi_inject_just_seen     = FALSE;
    begin_from_code_just_seen = FALSE;
    begin_inject_just_seen    = FALSE;
    default_just_seen         = FALSE;
  }

  if( add ) {

    long replace_offset = strlen( work_buffer );

    /* I don't believe that a line will ever exceed 4K chars */
    assert( (strlen( work_buffer ) + strlen( str )) < 4095 );
    strcat( work_buffer, str );

#ifdef DEBUG_MODE
    if( debug_mode ) {
      unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Adding to work code [%s] (fline: %u, fcol: %u, from_code: %d, file: %s, line: %u)",
                                  str, first_line, first_column, from_code, file, line );
      assert( rv < USER_MSG_LENGTH );
      print_output( user_msg, DEBUG, __FILE__, __LINE__ );
      generator_display();
    }
#endif

    /* If we have hit a newline, add the working buffer to the working list and clear the working buffer */
    if( strcmp( str, "\n" ) == 0 ) {
      str_link* tmp_tail = work_tail;
      str_link* strl     = str_link_add( strdup_safe( work_buffer ), &work_head, &work_tail );

      /*
       If the current word pointer is pointing at the buffer, we need to adjust it to point at the associated
       character in the new work list entry.
      */
      if( (replace_first.word_ptr != NULL) && (replace_first.list_ptr == NULL) ) {
        replace_first.word_ptr = strl->str + (replace_first.word_ptr - work_buffer); 
        replace_first.list_ptr = strl;
      }

      /* Set the replace_first or replace_last strucutures if they have not been setup previously */
      if( from_code ) {
        if( replace_first.word_ptr == NULL ) {
          replace_first.word_ptr = strl->str + replace_offset;
          replace_first.list_ptr = strl;
          replace_first_line     = first_line;
          replace_first_col      = first_column;
        }
      } else {
        if( (replace_first.word_ptr != NULL) && (replace_last.word_ptr == NULL) ) {
          if( replace_offset == 0 ) {
            replace_last.list_ptr = tmp_tail;
            replace_last.word_ptr = tmp_tail->str + (strlen( tmp_tail->str ) - 1);
          } else {
            replace_last.list_ptr = strl;
            replace_last.word_ptr = strl->str + (replace_offset - 1);
          }
        }
      }
      work_buffer[0] = '\0';
    } else {
      if( from_code ) {
        if( replace_first.word_ptr == NULL ) {
          replace_first.word_ptr = work_buffer + replace_offset;
          replace_first_line     = first_line;
          replace_first_col      = first_column;
        }
      } else {
        if( (replace_first.word_ptr != NULL) && (replace_last.word_ptr == NULL) ) {
          replace_last.word_ptr = work_buffer + (replace_offset - 1);
        }
      }
    }

  }

  PROFILE_END;

}

/*!
 Flushes the current working code to the holding code buffers.
*/
void generator_flush_work_code1(
  /*@unused@*/ const char*  file,  /*!< Filename that calls this function */
  /*@unused@*/ unsigned int line   /*!< Line number that calls this function */
) { PROFILE(GENERATOR_FLUSH_WORK_CODE1);

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing work code (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
//    generator_display();
  }
#endif

  /* If the hold_buffer is not empty, move it to the hold list */
  if( strlen( hold_buffer ) > 0 ) {
    (void)str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
  }

  /* If the working list is not empty, append it to the holding code */
  if( work_head != NULL ) {

    /* Move the working code lists to the holding code lists */
    if( hold_head == NULL ) {
      hold_head = work_head;
    } else {
      hold_tail->next = work_head;
    }
    hold_tail = work_tail;
    work_head = work_tail = NULL;

  }

  /* Copy the working buffer to the holding buffer */
  strcpy( hold_buffer, work_buffer );

  /* Clear the work buffer */
  work_buffer[0] = '\0';

  /* Clear replacement pointers */
  generator_clear_replace_ptrs();

  PROFILE_END;

}

/*!
 Adds the given code string to the "immediate" code generator.  This code can be output to the file
 immediately if there is no code in the sig_list and exp_list arrays.  If it cannot be output immediately,
 the code is added to the exp_list array.
*/
void generator_add_to_hold_code(
               const char*  str,   /*!< String to write */
  /*@unused@*/ const char*  file,  /*!< Filename of caller of this function */
  /*@unused@*/ unsigned int line   /*!< Line number of caller of this function */
) { PROFILE(GENERATOR_ADD_TO_HOLD_CODE);
 
  /* I don't believe that a line will ever exceed 4K chars */
  assert( (strlen( hold_buffer ) + strlen( str )) < 4095 );
  strcat( hold_buffer, str );

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Adding to hold code [%s] (file: %s, line: %u)", str, file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
//    generator_display();
  }
#endif

  /* If we have hit a newline, add it to the hold list and clear the hold buffer */
  if( strcmp( str, "\n" ) == 0 ) {
    (void)str_link_add( strdup_safe( hold_buffer ), &hold_head, &hold_tail );
    hold_buffer[0] = '\0';
  }

  PROFILE_END;

}

/*!
 Outputs all held code to the output file.
*/
void generator_flush_hold_code1(
  /*@unused@*/ const char*  file,  /*!< Filename that calls this function */
  /*@unused@*/ unsigned int line   /*!< Line number that calls this function */
) { PROFILE(GENERATOR_FLUSH_HOLD_CODE1);

  str_link* strl = hold_head;

#ifdef DEBUG_MODE
  if( debug_mode ) {
    unsigned int rv = snprintf( user_msg, USER_MSG_LENGTH, "Flushing hold code (file: %s, line: %u)", file, line );
    assert( rv < USER_MSG_LENGTH );
    print_output( user_msg, DEBUG, __FILE__, __LINE__ );
  }
#endif

  /* We shouldn't ever be flushing the hold code if the reg_top is more than one entry deep */
  assert( (reg_top == NULL) || (reg_top->next == NULL) );

  fprintf( curr_ofile, "\n" );

  /* Output the hold buffer list to the output file */
  strl = hold_head;
  while( strl != NULL ) {
    fprintf( curr_ofile, "%s", strl->str );
    strl = strl->next;
  }
  str_link_delete_list( hold_head );
  hold_head = hold_tail = NULL;

  /* If the hold buffer is not empty, output it as well */
  if( strlen( hold_buffer ) > 0 ) {
    fprintf( curr_ofile, "%s", hold_buffer );
    hold_buffer[0] = '\0';
  }

  /* Reset the register insertion pointer (no need to push/pop) */
  if( reg_top != NULL ) {
    reg_top->ptr = NULL;
  }

  PROFILE_END;

}

/*!
 Shortcut for calling generator_flush_work_code() followed by generator_flush_hold_code().
*/
void generator_flush_all1(
  const char*  file,  /*!< Filename where function is called */
  unsigned int line   /*!< Line number where function is called */
) { PROFILE(GENERATOR_FLUSH_ALL1);

  generator_flush_work_code1( file, line );
  generator_flush_hold_code1( file, line );

  PROFILE_END;

}

/*!
 \return Returns a pointer to the found statement (or if no statement was found, returns NULL).

 Searches the current functional unit for a statement that matches the given positional information.
*/
statement* generator_find_statement(
  unsigned int first_line,   /*!< First line of statement to find */
  unsigned int first_column  /*!< First column of statement to find */
) { PROFILE(GENERATOR_FIND_STATEMENT);

//  printf( "In generator_find_statement, line: %d, column: %d\n", first_line, first_column );

  if( (curr_stmt == NULL) || (curr_stmt->exp->ppline < first_line) ||
      ((curr_stmt->exp->ppline == first_line) && (curr_stmt->exp->col.part.first < first_column)) ) {

//    func_iter_display( &fiter );

    /* Attempt to find the expression with the given position */
    while( ((curr_stmt = func_iter_get_next_statement( &fiter )) != NULL) &&
//           printf( "  statement %s %d %u\n", expression_string( curr_stmt->exp ), ((curr_stmt->exp->col >> 16) & 0xffff), curr_stmt->exp->ppline ) &&
           ((curr_stmt->exp->ppline < first_line) || 
            ((curr_stmt->exp->ppline == first_line) && (curr_stmt->exp->col.part.first < first_column)) ||
            (curr_stmt->exp->op == EXP_OP_FORK)) );

    /* If we couldn't find it in the func_iter, look for it in the generate list */
    if( curr_stmt == NULL ) {
      statement* gen_stmt = generate_find_stmt_by_position( curr_funit, first_line, first_column );
      if( gen_stmt != NULL ) {
        curr_stmt = gen_stmt;
      }
    }

  }

//  if( (curr_stmt != NULL) && (curr_stmt->exp->ppline == first_line) && (curr_stmt->exp->col.part.first == first_column) && (curr_stmt->exp->op != EXP_OP_FORK) ) {
//    printf( "  FOUND (%s %x)!\n", expression_string( curr_stmt->exp ), curr_stmt->exp->col.part.first );
//  } else {
//    printf( "  NOT FOUND!\n" );
//  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->ppline != first_line) ||
          (curr_stmt->exp->col.part.first != first_column) || (curr_stmt->exp->op == EXP_OP_FORK)) ? NULL : curr_stmt );

}

/*!
 \return Returns a pointer to the found statement (or if no statement was found, returns NULL).

 Searches the current functional unit for a statement that contains the case expression matches the
 given positional information.
*/
static statement* generator_find_case_statement(
  unsigned int first_line,   /*!< First line of case expression to find */
  unsigned int first_column  /*!< First column of case expression to find */
) { PROFILE(GENERATOR_FIND_CASE_STATEMENT);

//  printf( "In generator_find_case_statement, line: %d, column: %d\n", first_line, first_column );

  if( (curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->ppline < first_line) ||
      ((curr_stmt->exp->left->ppline == first_line) && (curr_stmt->exp->left->col.part.first < first_column)) ) {

//    if( curr_stmt->exp->left != NULL ) {
//      printf( "curr_stmt->exp->left: %s\n", expression_string( curr_stmt->exp->left ) );
//    }
//    func_iter_display( &fiter );


    /* Attempt to find the expression with the given position */
    while( ((curr_stmt = func_iter_get_next_statement( &fiter )) != NULL) && 
//           printf( "  statement %s %d %u\n", expression_string( curr_stmt->exp ), curr_stmt->exp->col.part.first, curr_stmt->exp->ppline ) &&
           ((curr_stmt->exp->left == NULL) ||
            (curr_stmt->exp->left->ppline < first_line) ||
            ((curr_stmt->exp->left->ppline == first_line) && (curr_stmt->exp->left->col.part.first < first_column))) );

  }

//  if( (curr_stmt != NULL) && (curr_stmt->exp->left != NULL) && (curr_stmt->exp->left->ppline == first_line) && (curr_stmt->exp->left->col.part.first == first_column) ) {
//    printf( "  FOUND (%s %x)!\n", expression_string( curr_stmt->exp->left ), curr_stmt->exp->left->col.part.first );
//  } else {
//    printf( "  NOT FOUND!\n" );
//  }

  PROFILE_END;

  return( ((curr_stmt == NULL) || (curr_stmt->exp->left == NULL) || (curr_stmt->exp->left->ppline != first_line) ||
          (curr_stmt->exp->left->col.part.first != first_column)) ? NULL : curr_stmt );

}

/*!
 Inserts line coverage information for the given statement.
*/
void generator_insert_line_cov_with_stmt(
  statement* stmt,      /*!< Pointer to statement to generate line coverage for */
  bool       semicolon  /*!< Specifies if a semicolon (TRUE) or a comma (FALSE) should be appended to the created line */
) { PROFILE(GENERATOR_INSERT_LINE_COV_WITH_STMT);

  if( (stmt != NULL) && ((info_suppl.part.scored_line && !handle_funit_as_assert) || (handle_funit_as_assert && ovl_is_coverage_point( stmt->exp ))) ) {

    expression*  last_exp = expression_get_last_line_expr( stmt->exp );
    char         str[4096];
    char         sig[4096];
    unsigned int rv;
    str_link*    tmp_head = NULL;
    str_link*    tmp_tail = NULL;
    char*        scope    = generator_get_relative_scope( stmt->funit ); 

    if( scope[0] == '\0' ) {
      rv = snprintf( sig, 4096, " \\covered$L%u_%u_%x ", stmt->exp->ppline, last_exp->ppline, stmt->exp->col );
    } else {
      rv = snprintf( sig, 4096, " \\covered$L%u_%u_%x$%s ", stmt->exp->ppline, last_exp->ppline, stmt->exp->col, scope );
    }
    assert( rv < 4096 );
    free_safe( scope, (strlen( scope ) + 1) );

    /* Create the register */
    rv = snprintf( str, 4096, "reg %s;\n", sig );
    assert( rv < 4096 );
    generator_insert_reg( str );

    /* Prepend the line coverage assignment to the working buffer */
    rv = snprintf( str, 4096, " %s = 1'b1%c", sig, (semicolon ? ';' : ',') );
    assert( rv < 4096 );
    (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
    if( work_head == NULL ) {
      work_head = work_tail = tmp_head;
    } else {
      tmp_tail->next = work_head;
      work_head      = tmp_head;
    }

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the statement inserted (or NULL if no statement was inserted).

 Inserts line coverage information in string queues.
*/
statement* generator_insert_line_cov(
               unsigned int first_line,    /*!< First line to create line coverage for */
  /*@unused@*/ unsigned int last_line,     /*!< Last line to create line coverage for */
               unsigned int first_column,  /*!< First column of statement */
  /*@unused@*/ unsigned int last_column,   /*!< Last column of statement */
               bool         semicolon      /*!< Set to TRUE to create a semicolon after the line assignment; otherwise, adds a comma */
) { PROFILE(GENERATOR_INSERT_LINE_COV);

  statement* stmt = NULL;

  if( ((stmt = generator_find_statement( first_line, first_column )) != NULL) && !generator_is_static_function_only( stmt->funit ) &&
      ((info_suppl.part.scored_line && !handle_funit_as_assert) || (handle_funit_as_assert && ovl_is_coverage_point( stmt->exp ))) ) {

    generator_insert_line_cov_with_stmt( stmt, semicolon );

  }

  PROFILE_END;

  return( stmt );

}

/*!
 Handles the insertion of event-type combinational logic code.
*/
void generator_insert_event_comb_cov(
  expression* exp,        /*!< Pointer to expression to output */
  func_unit*  funit,      /*!< Pointer to functional unit containing the expression */
  bool        reg_needed  /*!< If set to TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_EVENT_COMB_COV);

  char         name[4096];
  char         str[4096];
  unsigned int rv;
  expression*  last_exp = expression_get_last_line_expr( exp );
  expression*  root_exp = exp;
  char*        scope    = generator_get_relative_scope( funit );

  /* Find the root event of this expression tree */
  while( (ESUPPL_IS_ROOT( root_exp->suppl ) == 0) && (EXPR_IS_EVENT( root_exp->parent->expr ) == 1) ) {
    root_exp = root_exp->parent->expr;
  }

  /* Create signal name */
  if( scope[0] == '\0' ) {
    rv = snprintf( name, 4096, " \\covered$E%u_%u_%x ", exp->ppline, last_exp->ppline, exp->col );
  } else {
    rv = snprintf( name, 4096, " \\covered$E%u_%u_%x$%s ", exp->ppline, last_exp->ppline, exp->col, scope );
  }
  assert( rv < 4096 );
  free_safe( scope, (strlen( scope ) + 1) );

  /* Create register */
  if( reg_needed ) {
    rv = snprintf( str, 4096, "reg %s;\n", name );
    assert( rv < 4096 );
    generator_insert_reg( str );
  }

  /*
   If the expression is also the root of its expression tree, it is the only event in the statement; therefore,
   the coverage string should just set the coverage variable to a value of 1 to indicate that the event was hit.
  */
  if( exp == root_exp ) {

    /* Create assignment and append it to the working code list */
    rv = snprintf( str, 4096, "%s = 1'b1;", name );
    assert( rv < 4096 );
    generator_add_cov_to_work_code( str );
    generator_add_cov_to_work_code( "\n" );

  /*
   Otherwise, we need to save off the state of the temporary event variable and compare it after the event statement
   has triggered to see which events where hit in the event statement.
  */
  } else {

    char* tname     = generator_create_expr_name( exp );
    char* event_str = codegen_gen_expr_one_line( exp->right, funit, FALSE );
    bool  stmt_head = (root_exp->parent->stmt->suppl.part.head == 1);

    /* Handle the event */
    switch( exp->op ) {
      case EXP_OP_PEDGE :
        {
          if( reg_needed && (exp->suppl.part.eval_t == 0) ) {
            rv = snprintf( str, 4096, "reg %s;\n", tname );
            assert( rv < 4096 );
            generator_insert_reg( str );
            exp->suppl.part.eval_t = 1;
          }
          rv = snprintf( str, 4096, " %s = (%s!==1'b1) & ((%s)===1'b1);", name, tname, event_str );
          assert( rv < 4096 );
          generator_add_cov_to_work_code( str );
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          if( stmt_head ) {
            generator_add_cov_to_work_code( str );
          } else {
            generator_prepend_to_work_code( str );
          }
          generator_add_cov_to_work_code( "\n" );
        }
        break;

      case EXP_OP_NEDGE :
        {
          if( reg_needed && (exp->suppl.part.eval_t == 0) ) {
            rv = snprintf( str, 4096, "reg %s;\n", tname );
            assert( rv < 4096 );
            generator_insert_reg( str );
            exp->suppl.part.eval_t = 1;
          }
          rv = snprintf( str, 4096, " %s = (%s!==1'b0) & ((%s)===1'b0);", name, tname, event_str );
          assert( rv < 4096 );
          generator_add_cov_to_work_code( str );
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          if( stmt_head ) {
            generator_add_cov_to_work_code( str );
          } else {
            generator_prepend_to_work_code( str );
          }
          generator_add_cov_to_work_code( "\n" );
        }
        break;

      case EXP_OP_AEDGE :
        {
          if( reg_needed && (exp->suppl.part.eval_t == 0) ) {
            int   number;
            char* size = generator_gen_size( exp->right, funit, &number );
            if( number >= 0 ) {
              rv = snprintf( str, 4096, "reg [%d:0] %s;\n", (number - 1), tname );
            } else {
              rv = snprintf( str, 4096, "reg [((%s)-1):0] %s;\n", size, tname );
            }
            assert( rv < 4096 );
            generator_insert_reg( str );
            free_safe( size, (strlen( size ) + 1) );
            exp->suppl.part.eval_t = 1;
          }
          rv = snprintf( str, 4096, " %s = (%s!==(%s));", name, tname, event_str );
          assert( rv < 4096 );
          generator_add_cov_to_work_code( str );
          rv = snprintf( str, 4096, " %s = %s;", tname, event_str );
          assert( rv < 4096 );
          generator_add_cov_to_work_code( str );
          generator_add_cov_to_work_code( "\n" );
        }
        break;

      default :
        break;
    }

    /* Deallocate memory */
    free_safe( event_str, (strlen( event_str ) + 1) );
    free_safe( tname,     (strlen( tname )     + 1) );

  }

  PROFILE_END;

}

/*!
 Inserts combinational logic coverage code for a unary expression.
*/
static void generator_insert_unary_comb_cov(
  expression*  exp,        /*!< Pointer to expression to output */
  func_unit*   funit,      /*!< Pointer to functional unit containing this expression */
  bool         net,        /*!< Set to TRUE if this expression is a net */
  bool         reg_needed  /*!< If TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_UNARY_COMB_COV);

  char         prefix[16];
  char         sig[4096];
  char*        sigr;
  char         str[4096];
  unsigned int rv;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char*        scope    = generator_get_relative_scope( funit );

  /* Create signal */
  if( scope[0] == '\0' ) {
    rv = snprintf( sig,  4096, " \\covered$%c%u_%u_%x ", (net ? 'u' : 'U'), exp->ppline, last_exp->ppline, exp->col );
  } else {
    rv = snprintf( sig,  4096, " \\covered$U%u_%u_%x$%s ", exp->ppline, last_exp->ppline, exp->col, scope );
  }
  assert( rv < 4096 );
  free_safe( scope, (strlen( scope ) + 1) );

  /* Create right signal name */
  sigr = generator_create_expr_name( exp );

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire " );
  } else {
    prefix[0] = '\0';
    if( reg_needed ) {
      rv = snprintf( str, 4096, "reg %s;\n", sig );
      assert( rv < 4096 );
      generator_insert_reg( str );
    }
  }

  /* Prepend the coverage assignment to the working buffer */
  if( exp->value->suppl.part.is_signed == 1 ) {
    rv = snprintf( str, 4096, "%s%s = (%s != 0);", prefix, sig, sigr );
  } else {
    rv = snprintf( str, 4096, "%s%s = (%s > 0);", prefix, sig, sigr );
  }
  assert( rv < 4096 );
  (void)str_link_add( strdup_safe( str ), &comb_head, &comb_tail );

  /* Deallocate temporary memory */
  free_safe( sigr, (strlen( sigr ) + 1) );

  PROFILE_END;

}

/*!
 Inserts AND/OR/OTHER-style combinational logic coverage code.
*/
static void generator_insert_comb_comb_cov(
  expression* exp,        /*!< Pointer to expression to output */
  func_unit*  funit,      /*!< Pointer to functional unit containing this expression */
  bool        net,        /*!< Set to TRUE if this expression is a net */
  bool        reg_needed  /*!< If set to TRUE, instantiates needed registers */
) { PROFILE(GENERATOR_INSERT_AND_COMB_COV);

  char         prefix[16];
  char         sig[4096];
  char*        sigl;
  char*        sigr;
  char         str[4096];
  unsigned int rv;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char*        scope    = generator_get_relative_scope( funit );

  /* Create signal */
  if( scope[0] == '\0' ) {
    rv = snprintf( sig, 4096, " \\covered$%c%u_%u_%x ", (net ? 'c' : 'C'), exp->ppline, last_exp->ppline, exp->col );
  } else {
    rv = snprintf( sig, 4096, " \\covered$C%u_%u_%x$%s ", exp->ppline, last_exp->ppline, exp->col, scope );
  }
  assert( rv < 4096 );
  free_safe( scope, (strlen( scope ) + 1) );

  /* Create left and right expression names */
  sigl = generator_create_expr_name( exp->left );
  sigr = generator_create_expr_name( exp->right );

  /* Create prefix */
  if( net ) {
    strcpy( prefix, "wire [1:0] " );
  } else if( reg_needed ) {
    prefix[0] = '\0';
    rv = snprintf( str, 4096, "reg [1:0] %s;\n", sig );
    assert( rv < 4096 );
    generator_insert_reg( str );
  }

  /* Prepend the coverage assignment to the working buffer */
  if( exp->left->value->suppl.part.is_signed == 1 ) {
    if( exp->right->value->suppl.part.is_signed == 1 ) {
      rv = snprintf( str, 4096, "%s%s = {(%s != 0),(%s != 0)};", prefix, sig, sigl, sigr );
    } else {
      rv = snprintf( str, 4096, "%s%s = {(%s != 0),(%s > 0)};", prefix, sig, sigl, sigr );
    }
  } else {
    if( exp->right->value->suppl.part.is_signed == 1 ) {
      rv = snprintf( str, 4096, "%s%s = {(%s > 0),(%s != 0)};", prefix, sig, sigl, sigr );
    } else {
      rv = snprintf( str, 4096, "%s%s = {(%s > 0),(%s > 0)};", prefix, sig, sigl, sigr );
    }
  }
  assert( rv < 4096 );
  (void)str_link_add( strdup_safe( str ), &comb_head, &comb_tail );

  /* Deallocate memory */
  free_safe( sigl, (strlen( sigl ) + 1) );
  free_safe( sigr, (strlen( sigr ) + 1) );

  PROFILE_END;

}

static char* generator_mbit_gen_value(
  expression* exp,
  func_unit*  funit,
  int*        number
) { PROFILE(GENERATOR_MBIT_GEN_VALUE);

  char* value = NULL;

  if( exp != NULL ) {

    if( exp->op == EXP_OP_STATIC ) {
      *number = vector_to_int( exp->value );
    } else { 
      value = codegen_gen_expr_one_line( exp, funit, FALSE );
    }
    
  }

  PROFILE_END;

  return( value );

}

/*!
 Generates MSB string to use for sizing subexpression values.
*/
static char* generator_gen_size(
  expression* exp,    /*!< Pointer to subexpression to generate MSB value for */
  func_unit*  funit,  /*!< Pointer to functional unit containing this subexpression */
  int*        number  /*!< Pointer to value that is set to the number of the returned string represents numbers only */
) { PROFILE(GENERATOR_GEN_SIZE);

  char* size = NULL;

  *number = -1;

  if( exp != NULL ) {

    char*        lexp = NULL;
    char*        rexp = NULL;
    unsigned int rv;
    int          lnumber;
    int          rnumber;

    switch( exp->op ) {
      case EXP_OP_STATIC :
        *number = exp->value->width;
        break;
      case EXP_OP_LIST     :
      case EXP_OP_MULTIPLY :
        lexp = generator_gen_size( exp->left,  funit, &lnumber );
        rexp = generator_gen_size( exp->right, funit, &rnumber );
        if( (lexp == NULL) && (rexp == NULL) ) {
          *number = lnumber + rnumber;
        } else {
          unsigned int slen;
          if( lexp == NULL ) {
            lexp = convert_int_to_str( lnumber );
          } else if( rexp == NULL ) {
            rexp = convert_int_to_str( rnumber );
          }
          slen = 1 + strlen( lexp ) + 3 + strlen( rexp ) + 2;
          size = (char*)malloc_safe( slen );
          rv   = snprintf( size, slen, "(%s)+(%s)", lexp, rexp );
          assert( rv < slen );
          free_safe( lexp, (strlen( lexp ) + 1) );
          free_safe( rexp, (strlen( rexp ) + 1) );
        }
        break;
      case EXP_OP_CONCAT         :
      case EXP_OP_NEGATE         :
      case EXP_OP_COND           :
        size = generator_gen_size( exp->right, funit, number );
        break;
      case EXP_OP_MBIT_POS       :
      case EXP_OP_MBIT_NEG       :
      case EXP_OP_PARAM_MBIT_POS :
      case EXP_OP_PARAM_MBIT_NEG :
        size = generator_mbit_gen_value( exp->right, funit, number );
        break;
      case EXP_OP_LSHIFT  :
      case EXP_OP_RSHIFT  :
      case EXP_OP_ALSHIFT :
      case EXP_OP_ARSHIFT :
        size = generator_gen_size( exp->left, funit, number );
        break;
      case EXP_OP_EXPAND :
        lexp = generator_mbit_gen_value( exp->left, funit, &lnumber );
        rexp = generator_gen_size( exp->right, funit, &rnumber );
        if( (lexp == NULL) && (rexp == NULL) ) {
          *number = lnumber * rnumber;
        } else {
          unsigned int slen;
          if( lexp == NULL ) {
            lexp = convert_int_to_str( lnumber );
          } else if( rexp == NULL ) {
            rexp = convert_int_to_str( rnumber );
          }
          slen = 1 + strlen( lexp ) + 3 + strlen( rexp ) + 2;
          size = (char*)malloc_safe( slen );
          rv   = snprintf( size, slen, "(%s)*(%s)", lexp, rexp );
          assert( rv < slen );
          free_safe( lexp, (strlen( lexp ) + 1) );
          free_safe( rexp, (strlen( rexp ) + 1) );
        }
        break;
      case EXP_OP_STIME :
      case EXP_OP_SR2B  :
      case EXP_OP_SR2I  :
        *number = 64;
        break;
      case EXP_OP_SSR2B        :
      case EXP_OP_SRANDOM      :
      case EXP_OP_SURANDOM     :
      case EXP_OP_SURAND_RANGE :
        *number = 32;
        break;
      case EXP_OP_LT        :
      case EXP_OP_GT        :
      case EXP_OP_EQ        :
      case EXP_OP_CEQ       :
      case EXP_OP_LE        :
      case EXP_OP_GE        :
      case EXP_OP_NE        :
      case EXP_OP_CNE       :
      case EXP_OP_LOR       :
      case EXP_OP_LAND      :
      case EXP_OP_UAND      :
      case EXP_OP_UNOT      :
      case EXP_OP_UOR       :
      case EXP_OP_UXOR      :
      case EXP_OP_UNAND     :
      case EXP_OP_UNOR      :
      case EXP_OP_UNXOR     :
      case EXP_OP_EOR       :
      case EXP_OP_NEDGE     :
      case EXP_OP_PEDGE     :
      case EXP_OP_AEDGE     :
      case EXP_OP_CASE      :
      case EXP_OP_CASEX     :
      case EXP_OP_CASEZ     :
      case EXP_OP_DEFAULT   :
      case EXP_OP_REPEAT    :
      case EXP_OP_RPT_DLY   :
      case EXP_OP_WAIT      :
      case EXP_OP_SFINISH   :
      case EXP_OP_SSTOP     :
      case EXP_OP_SSRANDOM  :
      case EXP_OP_STESTARGS :
      case EXP_OP_SVALARGS  :
      case EXP_OP_PARAM_SBIT :
        *number = 1;
        break;
      case EXP_OP_SBIT_SEL  :
        {
          unsigned int dimension = expression_get_curr_dimension( exp );
          if( (exp->sig->suppl.part.type == SSUPPL_TYPE_MEM) && ((dimension + 1) < (exp->sig->udim_num + exp->sig->pdim_num)) ) {
            size = mod_parm_gen_size_code( exp->sig, (dimension + 1), funit_get_curr_module( funit ), number );
          } else {
            *number = 1;
          }
        }
        break;
      case EXP_OP_MBIT_SEL   :
      case EXP_OP_PARAM_MBIT :
        lexp = generator_mbit_gen_value( exp->left,  funit, &lnumber );
        rexp = generator_mbit_gen_value( exp->right, funit, &rnumber );
        if( (lexp == NULL) && (rexp == NULL) ) {
          *number = ((exp->sig->suppl.part.big_endian == 1) ? (rnumber - lnumber) : (lnumber - rnumber)) + 1;
        } else {
          unsigned int slen;
          if( lexp == NULL ) {
            lexp = convert_int_to_str( lnumber );
          } else if( rexp == NULL ) {
            rexp = convert_int_to_str( rnumber );
          }
          slen = 2 + strlen( lexp ) + 3 + strlen( rexp ) + 5;
          size = (char*)malloc_safe( slen );
          if( exp->sig->suppl.part.big_endian == 1 ) {
            rv = snprintf( size, slen, "((%s)-(%s))+1", rexp, lexp );
          } else {
            rv = snprintf( size, slen, "((%s)-(%s))+1", lexp, rexp );
          }
          assert( rv < slen );
          free_safe( lexp, (strlen( lexp ) + 1) );
          free_safe( rexp, (strlen( rexp ) + 1) );
        }
        break;
      case EXP_OP_SIG       :
      case EXP_OP_PARAM     :
      case EXP_OP_FUNC_CALL :
        if( (exp->sig->suppl.part.type == SSUPPL_TYPE_GENVAR) || (exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_SREAL) ) {
          *number = 32;
        } else if( exp->sig->suppl.part.type == SSUPPL_TYPE_DECL_REAL ) {
          *number = 64;
        } else {
          size = mod_parm_gen_size_code( exp->sig, expression_get_curr_dimension( exp ), funit_get_curr_module( funit ), number );
        }
        break;
      default :
        {
          bool set = FALSE;
          if( exp->left != NULL ) {
            lexp = generator_gen_size( exp->left, funit, &lnumber );
            if( exp->right == NULL ) {
              if( lexp == NULL ) {
                *number = lnumber;
              } else {
                size = lexp;
              }
              set = TRUE;
            }
          }
          if( exp->right != NULL ) {
            rexp = generator_gen_size( exp->right, funit, &rnumber );
            if( exp->left == NULL ) {
              if( rexp == NULL ) {
                *number = rnumber;
              } else {
                size = rexp;
              }
              set = TRUE;
            }
          }
          if( !set ) {
            if( (lexp == NULL) && (rexp == NULL) ) {
              *number = (lnumber > rnumber) ? lnumber : rnumber;
            } else {
              unsigned int slen;
              if( lexp == NULL ) {
                lexp = convert_int_to_str( lnumber );
              } else if( rexp == NULL ) {
                rexp = convert_int_to_str( rnumber );
              }
              slen = (strlen( lexp ) * 2) + (strlen( rexp ) * 2) + 14;
              size = (char*)malloc_safe( slen );
              rv   = snprintf( size, slen, "((%s)>(%s))?(%s):(%s)", lexp, rexp, lexp, rexp );
              assert( rv < slen );
              free_safe( lexp, (strlen( lexp ) + 1) );
              free_safe( rexp, (strlen( rexp ) + 1) );
            }
          }
        }
        break;
    }

  } 

  PROFILE_END;

  return( size );

}

/*!
 Generates the strings needed for the left-hand-side of the temporary expression.
*/
static char* generator_create_lhs(
  expression* exp,        /*!< Pointer to subexpression to create LHS string for */
  func_unit*  funit,      /*!< Functional unit containing the expression */
  bool        net,        /*!< If set to TRUE, generate code for a net; otherwise, generate for a register */
  bool        reg_needed  /*!< If TRUE, instantiates the needed registers */
) { PROFILE(GENERATOR_CREATE_LHS);

  unsigned int rv;
  char*        name = generator_create_expr_name( exp );
  char*        size;
  char*        code;
  int          number;

  /* Generate MSB string */
  size = generator_gen_size( exp, funit, &number );

  if( net ) {

    unsigned int slen;

    /* Create sized wire string */
    if( size == NULL ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", (number - 1) );
      assert( rv < 50 );
      slen = 6 + strlen( tmp ) + 4 + strlen( name ) + 1;
      code = (char*)malloc_safe( slen );
      rv   = snprintf( code, slen, "wire [%s:0] %s", tmp, name );
    } else {
      slen = 7 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 1;
      code = (char*)malloc_safe_nolimit( slen );
      rv   = snprintf( code, slen, "wire [(%s)-1:0] %s", ((size != NULL) ? size : "1"), name );
    }

    assert( rv < slen );

  } else {

    /* Create sized register string */
    if( reg_needed ) {  // && (exp->suppl.part.eval_t == 0) ) {

      unsigned int slen;
      char*        str;
      
      if( size == NULL ) {
        char tmp[50];
        rv = snprintf( tmp, 50, "%d", (number - 1) );
        assert( rv < 50 );
        if( exp->value->suppl.part.is_signed == 1 ) {
          slen = 30 + strlen( name ) + 20 + strlen( tmp ) + 4 + strlen( name ) + 10;
          str  = (char*)malloc_safe( slen );
          rv   = snprintf( str, slen, "`ifdef V1995_COV_MODE\ninteger %s;\n`else\nreg signed [%s:0] %s;\n`endif\n", name, tmp, name );
        } else {
          slen = 5 + strlen( tmp ) + 4 + strlen( name ) + 3;
          str  = (char*)malloc_safe( slen );
          rv   = snprintf( str, slen, "reg [%s:0] %s;\n", tmp, name );
        }
      } else {
        if( exp->value->suppl.part.is_signed == 1 ) {
          slen = 30 + strlen( name ) + 21 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 10;
          str  = (char*)malloc_safe_nolimit( slen );
          rv   = snprintf( str, slen, "`ifdef V1995_COV_MODE\ninteger %s;\n`else\nreg signed [(%s-1):0] %s;\n`endif\n", name, ((size != NULL) ? size : "1"), name );
        } else {
          slen = 6 + ((size != NULL) ? strlen( size ) : 1) + 7 + strlen( name ) + 3;
          str  = (char*)malloc_safe_nolimit( slen );
          rv   = snprintf( str, slen, "reg [(%s)-1:0] %s;\n", ((size != NULL) ? size : "1"), name );
        }
      }

      assert( rv < slen );
      generator_insert_reg( str );
      free_safe( str, (strlen( str ) + 1) );

      exp->suppl.part.eval_t = 1;

    }

    /* Set the name to the value of code */
    code = strdup_safe( name );

  }

  /* Deallocate memory */
  free_safe( name, (strlen( name ) + 1) );
  free_safe( size, (strlen( size ) + 1) );

  PROFILE_END;

  return( code );

}

#ifdef OBSOLETE
/*!
 Concatenates the given string values and appends them to the working code buffer.
*/
static void generator_concat_code(
               char* lhs,     /*!< LHS string */
  /*@null@*/   char* before,  /*!< Optional string that precedes the left subexpression string array */
  /*@null@*/   char* lstr,    /*!< String array from left subexpression */
  /*@null@*/   char* middle,  /*!< Optional string that is placed in-between the left and right subexpression array strings */
  /*@null@*/   char* rstr,    /*!< String array from right subexpression */
  /*@null@*/   char* after,   /*!< Optional string that is placed after the right subpexression string array */
  /*@unused@*/ bool  net      /*!< If TRUE, specifies that this subexpression exists in net logic */
) { PROFILE(GENERATOR_CONCAT_CODE);

  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;
  char         str[4096];
  unsigned int i;
  unsigned int rv;

  /* Prepend the coverage assignment to the working buffer */
  rv = snprintf( str, 4096, "%s = ", lhs );
  assert( rv < 4096 );
  if( before != NULL ) {
    if( (strlen( str ) + strlen( before )) < 4095 ) {
      strcat( str, before );
    } else {
      (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      strcpy( str, before );
    }
  }
  if( lstr != NULL ) {
    if( (strlen( str ) + strlen( lstr )) < 4095 ) {
      strcat( str, lstr );
    } else {
      (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      if( strlen( lstr ) < 4095 ) {
        strcpy( str, lstr );
      } else {
        (void)str_link_add( strdup_safe( lstr ), &tmp_head, &tmp_tail );
        str[0] = '\0';
      }
    }
  }
  if( middle != NULL ) {
    if( (strlen( str ) + strlen( middle )) < 4095 ) {
      strcat( str, middle );
    } else {
      (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      strcpy( str, middle );
    }
  }
  if( rstr != NULL ) {
    if( (strlen( str ) + strlen( rstr )) < 4095 ) {
      strcat( str, rstr );
    } else {
      (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      if( strlen( rstr ) < 4095 ) {
        strcpy( str, lstr );
      } else {
        (void)str_link_add( strdup_safe( rstr ), &tmp_head, &tmp_tail );
        str[0] = '\0';
      }
    }
  }
  if( after != NULL ) {
    if( (strlen( str ) + strlen( after )) < 4095 ) {
      strcat( str, after );
    } else {
      (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      strcpy( str, after );
    }
  }
  if( (strlen( str ) + 1) < 4095 ) {
    strcat( str, ";" );
  } else {
    (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
    strcpy( str, ";" );
  }
  (void)str_link_add( strdup_safe( str ), &tmp_head, &tmp_tail );
      
  /* Prepend to the working list */
  if( work_head == NULL ) {
    work_head = work_tail = tmp_head;
  } else {
    tmp_tail->next = work_head;
    work_head      = tmp_head;
  }

  PROFILE_END;

}
#endif

#ifdef OBSOLETE
/*!
 Generates the combinational logic temporary expression string for the given expression.
*/
static void generator_create_exp(
  expression* exp,   /*!< Pointer to the expression to generation the expression for */
  char*       lhs,   /*!< String for left-hand-side of temporary expression */
  char*       lstr,  /*!< String for left side of RHS expression */
  char*       rstr,  /*!< String for right side of RHS expression */
  bool        net    /*!< Set to TRUE if we are generating for a net; set to FALSE for a register */
) { PROFILE(GENERATOR_CREATE_EXP);

  switch( exp->op ) {
    case EXP_OP_XOR        :  generator_concat_code( lhs, NULL, lstr, " ^ ", rstr, NULL, net );  break;
    case EXP_OP_MULTIPLY   :  generator_concat_code( lhs, NULL, lstr, " * ", rstr, NULL, net );  break;
    case EXP_OP_DIVIDE     :  generator_concat_code( lhs, NULL, lstr, " / ", rstr, NULL, net );  break;
    case EXP_OP_MOD        :  generator_concat_code( lhs, NULL, lstr, " % ", rstr, NULL, net );  break;
    case EXP_OP_ADD        :  generator_concat_code( lhs, NULL, lstr, " + ", rstr, NULL, net );  break;
    case EXP_OP_SUBTRACT   :  generator_concat_code( lhs, NULL, lstr, " - ", rstr, NULL, net );  break;
    case EXP_OP_AND        :  generator_concat_code( lhs, NULL, lstr, " & ", rstr, NULL, net );  break;
    case EXP_OP_OR         :  generator_concat_code( lhs, NULL, lstr, " | ", rstr, NULL, net );  break;
    case EXP_OP_NAND       :  generator_concat_code( lhs, NULL, lstr, " ~& ", rstr, NULL, net );  break;
    case EXP_OP_NOR        :  generator_concat_code( lhs, NULL, lstr, " ~| ", rstr, NULL, net );  break;
    case EXP_OP_NXOR       :  generator_concat_code( lhs, NULL, lstr, " ~^ ", rstr, NULL, net );  break;
    case EXP_OP_LT         :  generator_concat_code( lhs, NULL, lstr, " < ", rstr, NULL, net );  break;
    case EXP_OP_GT         :  generator_concat_code( lhs, NULL, lstr, " > ", rstr, NULL, net );  break;
    case EXP_OP_LSHIFT     :  generator_concat_code( lhs, NULL, lstr, " << ", rstr, NULL, net );  break;
    case EXP_OP_RSHIFT     :  generator_concat_code( lhs, NULL, lstr, " >> ", rstr, NULL, net );  break;
    case EXP_OP_EQ         :  generator_concat_code( lhs, NULL, lstr, " == ", rstr, NULL, net );  break;
    case EXP_OP_CEQ        :  generator_concat_code( lhs, NULL, lstr, " === ", rstr, NULL, net );  break;
    case EXP_OP_LE         :  generator_concat_code( lhs, NULL, lstr, " <= ", rstr, NULL, net );  break;
    case EXP_OP_GE         :  generator_concat_code( lhs, NULL, lstr, " >= ", rstr, NULL, net );  break;
    case EXP_OP_NE         :  generator_concat_code( lhs, NULL, lstr, " != ", rstr, NULL, net );  break;
    case EXP_OP_CNE        :  generator_concat_code( lhs, NULL, lstr, " !== ", rstr, NULL, net );  break;
    case EXP_OP_LOR        :  generator_concat_code( lhs, NULL, lstr, " || ", rstr, NULL, net );  break;
    case EXP_OP_LAND       :  generator_concat_code( lhs, NULL, lstr, " && ", rstr, NULL, net );  break;
    case EXP_OP_UINV       :  generator_concat_code( lhs, NULL, NULL, "~", rstr, NULL, net );  break;
    case EXP_OP_UAND       :  generator_concat_code( lhs, NULL, NULL, "&", rstr, NULL, net );  break;
    case EXP_OP_UNOT       :  generator_concat_code( lhs, NULL, NULL, "!", rstr, NULL, net );  break;
    case EXP_OP_UOR        :  generator_concat_code( lhs, NULL, NULL, "|", rstr, NULL, net );  break;
    case EXP_OP_UXOR       :  generator_concat_code( lhs, NULL, NULL, "^", rstr, NULL, net );  break;
    case EXP_OP_UNAND      :  generator_concat_code( lhs, NULL, NULL, "~&", rstr, NULL, net );  break;
    case EXP_OP_UNOR       :  generator_concat_code( lhs, NULL, NULL, "~|", rstr, NULL, net );  break;
    case EXP_OP_UNXOR      :  generator_concat_code( lhs, NULL, NULL, "~^", rstr, NULL, net );  break;
    case EXP_OP_ALSHIFT    :  generator_concat_code( lhs, NULL, lstr, " <<< ", rstr, NULL, net );  break;
    case EXP_OP_ARSHIFT    :  generator_concat_code( lhs, NULL, lstr, " >>> ", rstr, NULL, net );  break;
    case EXP_OP_EXPONENT   :  generator_concat_code( lhs, NULL, lstr, " ** ", rstr, NULL, net );  break;
    case EXP_OP_NEGATE     :  generator_concat_code( lhs, NULL, NULL, "-", rstr, NULL, net );  break;
    case EXP_OP_CASE       :  generator_concat_code( lhs, NULL, lstr, " == ", rstr, NULL, net );  break;
    case EXP_OP_CASEX      :  generator_concat_code( lhs, NULL, lstr, " === ", rstr, NULL, net );  break;
    case EXP_OP_CASEZ      :  generator_concat_code( lhs, NULL, lstr, " === ", rstr, NULL, net );  break;  /* TBD */
    case EXP_OP_CONCAT     :  generator_concat_code( lhs, NULL, NULL, "{", rstr, "}", net );  break;
    case EXP_OP_EXPAND     :  generator_concat_code( lhs, "{", lstr, "{", rstr, "}}", net );  break;
    case EXP_OP_LIST       :  generator_concat_code( lhs, NULL, lstr, ",", rstr, NULL, net );  break;
    case EXP_OP_COND       :  generator_concat_code( lhs, NULL, lstr, " ? ", rstr, NULL, net );  break;
    case EXP_OP_COND_SEL   :  generator_concat_code( lhs, NULL, lstr, " : ", rstr, NULL, net );  break;
    case EXP_OP_SIG        :
    case EXP_OP_PARAM      :  generator_concat_code( lhs, exp->name, NULL, NULL, NULL, NULL, net );  break;
    case EXP_OP_DIM        :  generator_concat_code( lhs, NULL, lstr, NULL, rstr, NULL, net );  break;
    case EXP_OP_FUNC_CALL  :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s(", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, ")", NULL, NULL, net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_SBIT_SEL   :
    case EXP_OP_PARAM_SBIT :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, "]", NULL, NULL, net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_MBIT_SEL   :
    case EXP_OP_PARAM_MBIT :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, ":", rstr, "]", net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_MBIT_POS       :
    case EXP_OP_PARAM_MBIT_POS :
      {
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, "+:", rstr, "]", net );
        free_safe( str, slen );
      }
      break;
    case EXP_OP_MBIT_NEG       :
    case EXP_OP_PARAM_MBIT_NEG :
      { 
        unsigned int slen = strlen( exp->name ) + 2;
        char*        str  = (char*)malloc_safe( slen );
        unsigned int rv   = snprintf( str, slen, "%s[", exp->name );
        assert( rv < slen );
        generator_concat_code( lhs, str, lstr, "-:", rstr, "]", net );
        free_safe( str, slen );
      }
      break;
    default :
      break;
  }

  PROFILE_END;

}
#endif

/*!
 Generates temporary subexpression for the given expression (not recursively)
*/
static void generator_insert_subexp(
  expression* exp,         /*!< Pointer to the current expression */
  func_unit*  funit,       /*!< Pointer to the functional unit that exp exists in */
  bool        net,         /*!< If TRUE, specifies that we are generating for a net */
  bool        reg_needed,  /*!< If TRUE, instantiates needed registers */
  bool        replace_exp  /*!< If TRUE, replaces the actual logic with this subexpression */
) { PROFILE(GENERATOR_INSERT_SUBEXP);

  char*        lhs_str  = NULL;
  char*        val_str;
  char*        str;
  unsigned int slen;
  unsigned int rv;
  unsigned int i;
  str_link*    tmp_head = NULL;
  str_link*    tmp_tail = NULL;

  /* Create LHS portion of assignment */
  lhs_str = generator_create_lhs( exp, funit, net, reg_needed );

  /* Generate value string */
  if( EXPR_IS_OP_AND_ASSIGN( exp ) == 1 ) {
    char  op_str[4];
    char* lval_str = codegen_gen_expr_one_line( exp->left,  funit, !generator_expr_needs_to_be_substituted( exp->left ) );
    char* rval_str = codegen_gen_expr_one_line( exp->right, funit, !generator_expr_needs_to_be_substituted( exp->right ) );

    switch( exp->op ) {
      case EXP_OP_MLT_A :  strcpy( op_str, "*"  );   break;
      case EXP_OP_DIV_A :  strcpy( op_str, "/"  );   break;
      case EXP_OP_MOD_A :  strcpy( op_str, "%"  );   break;
      case EXP_OP_LS_A  :  strcpy( op_str, "<<" );   break;
      case EXP_OP_RS_A  :  strcpy( op_str, ">>" );   break;
      case EXP_OP_ALS_A :  strcpy( op_str, "<<<" );  break;
      case EXP_OP_ARS_A :  strcpy( op_str, ">>>" );  break;
      default :
        assert( 0 );
        break;
    }

    /* Construct the string */
    slen    = 1 + strlen( lval_str ) + 2 + strlen( op_str ) + 2 + strlen( rval_str ) + 2;
    val_str = (char*)malloc_safe( slen );
    rv      = snprintf( val_str, slen, "(%s) %s (%s)", lval_str, op_str, rval_str );
    assert( rv < slen );

    free_safe( lval_str, (strlen( lval_str ) + 1) );
    free_safe( rval_str, (strlen( rval_str ) + 1) );

  /* Otherwise, the value string is just the expression itself */
  } else {
    val_str = codegen_gen_expr_one_line( exp, funit, !generator_expr_needs_to_be_substituted( exp ) );

  }

  /* If this expression needs to be substituted, do it with the lhs_str value */
  if( replace_exp && !net ) {
    expression* last_exp = expression_get_last_line_expr( exp );
    generator_replace( lhs_str, exp->ppline, exp->col.part.first, last_exp->ppline, exp->col.part.last );
  }

  /* Create expression string */
  slen = strlen( lhs_str ) + 3 + strlen( val_str ) + 2;
  str  = (char*)malloc_safe_nolimit( slen );
  rv   = snprintf( str, slen, "%s = %s;", lhs_str, val_str );
  assert( rv < slen );

  /* Prepend the string to the register/working code */
  (void)str_link_add( str, &comb_head, &comb_tail );

  /* Deallocate memory */
  free_safe( lhs_str, (strlen( lhs_str ) + 1) );
  free_safe( val_str, (strlen( val_str ) + 1) );

  /* Specify that this expression has an intermediate value assigned to it */
  exp->suppl.part.comb_cntd = 1;

  PROFILE_END;

}

/*!
 Recursively inserts the combinational logic coverage code for the given expression tree.
*/
static void generator_insert_comb_cov_helper2(
  expression*  exp,           /*!< Pointer to expression tree to operate on */
  func_unit*   funit,         /*!< Pointer to current functional unit */
  exp_op_type  parent_op,     /*!< Parent expression operation (originally should be set to the same operation as exp) */
  int          parent_depth,  /*!< Current expression depth (originally set to 0) */
  bool         force_subexp,  /*!< Set to TRUE if a expression subexpression is required needed (originally set to FALSE) */
  bool         net,           /*!< If set to TRUE generate code for a net */
  bool         root,          /*!< Set to TRUE only for the "root" expression in the tree */
  bool         reg_needed,    /*!< If set to TRUE, registers are created as needed; otherwise, they are omitted */
  bool         replace_exp    /*!< If set to TRUE, will allow this expression to replace the original */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER2);

  if( exp != NULL ) {

    int  depth             = parent_depth + ((exp->op != parent_op) ? 1 : 0);
    bool expr_cov_needed   = generator_expr_cov_needed( exp, depth );
    bool child_replace_exp = replace_exp &&
                             !(force_subexp ||
                               generator_expr_needs_to_be_substituted( exp ) ||
                               (EXPR_IS_COMB( exp ) && !root && expr_cov_needed) ||
                               (!EXPR_IS_EVENT( exp ) && !EXPR_IS_COMB( exp ) && expr_cov_needed));

    /* Generate children expression trees (depth first search) */
    generator_insert_comb_cov_helper2( exp->left,  funit, exp->op, depth, (expr_cov_needed & EXPR_IS_COMB( exp )), net, FALSE, reg_needed, child_replace_exp );
    generator_insert_comb_cov_helper2( exp->right, funit, exp->op, depth, (expr_cov_needed & EXPR_IS_COMB( exp )), net, FALSE, reg_needed, child_replace_exp );

    /* Generate event combinational logic type */
    if( EXPR_IS_EVENT( exp ) ) {
      if( expr_cov_needed ) {
        generator_insert_event_comb_cov( exp, funit, reg_needed );
      }
      if( force_subexp || generator_expr_needs_to_be_substituted( exp ) ) {
        generator_insert_subexp( exp, funit, net, reg_needed, replace_exp );
      }

    /* Otherwise, generate binary combinational logic type */
    } else if( EXPR_IS_COMB( exp ) ) {
      if( !root && (expr_cov_needed || force_subexp || generator_expr_needs_to_be_substituted( exp )) ) {
        generator_insert_subexp( exp, funit, net, reg_needed, replace_exp );
      }
      if( expr_cov_needed ) {
        generator_insert_comb_comb_cov( exp, funit, net, reg_needed );
      }

    /* Generate unary combinational logic type */
    } else {
      if( expr_cov_needed || force_subexp || generator_expr_needs_to_be_substituted( exp ) ) {
        generator_insert_subexp( exp, funit, net, reg_needed, replace_exp );
      }
      if( expr_cov_needed ) {
        generator_insert_unary_comb_cov( exp, funit, net, reg_needed );
      }

    }

  }

  PROFILE_END;

}

/*!
 Recursively inserts the combinational logic coverage code for the given expression tree.
*/
static void generator_insert_comb_cov_helper(
  expression*  exp,        /*!< Pointer to expression tree to operate on */
  func_unit*   funit,      /*!< Pointer to current functional unit */
  exp_op_type  parent_op,  /*!< Parent expression operation (originally should be set to the same operation as exp) */
  bool         net,        /*!< If set to TRUE generate code for a net */
  bool         root,       /*!< Set to TRUE only for the "root" expression in the tree */
  bool         reg_needed  /*!< If set to TRUE, registers are created as needed; otherwise, they are omitted */
) { PROFILE(GENERATOR_INSERT_COMB_COV_HELPER);

  /* Generate the code */
  generator_insert_comb_cov_helper2( exp, funit, parent_op, 0, FALSE, net, root, reg_needed, TRUE );

  /* Output the generated code */
  if( comb_head != NULL ) {

    /* Prepend to the working list */
    if( work_head == NULL ) {
      work_head = comb_head;
      work_tail = comb_tail;
    } else {
      comb_tail->next = work_head;
      work_head       = comb_head;
    }

    /* Clear the comb head/tail pointers for reuse */
    comb_head = comb_tail = NULL;

  }

  /* Clear the comb_cntd bits in the expression tree */
  generator_clear_comb_cntd( exp );

}

/*!
 \return Returns a string containing the memory index value to use for memory write/read coverage.

 Generates a memory index value for a given memory expression.
*/
static char* generator_gen_mem_index_helper(
  expression* exp,        /*!< Pointer to expression accessing memory signal */
  func_unit*  funit,      /*!< Pointer to functional unit containing exp */
  int         dimension,  /*!< Current memory dimension (should be initially set to expression_get_curr_dimension( exp ) */
  char*       ldim_width  /*!< Bit width of the lower dimension in string form (should be NULL in first call) */
) { PROFILE(GENERATOR_GEN_MEM_INDEX_HELPER);

  char*        index;
  char*        str;
  char*        num;
  unsigned int slen;
  unsigned int rv;
  int          number;

  /* Calculate the index value */
  switch( exp->op ) {
    case EXP_OP_SBIT_SEL :
      index = codegen_gen_expr_one_line( exp->left, funit, FALSE );
      break;
    case EXP_OP_MBIT_SEL :
      {
        char* lstr = codegen_gen_expr_one_line( exp->left,  funit, FALSE );
        char* rstr = codegen_gen_expr_one_line( exp->right, funit, FALSE );
        slen  = (strlen( lstr ) * 3) + (strlen( rstr ) * 3) + 24;
        index = (char*)malloc_safe( slen );
        rv    = snprintf( index, slen, "((%s)>(%s))?((%s)-(%s)):((%s)-(%s))", lstr, rstr, lstr, rstr, rstr, lstr );
        assert( rv < slen );
        free_safe( lstr, (strlen( lstr ) + 1) );
        free_safe( rstr, (strlen( rstr ) + 1) );
      }
      break;
    case EXP_OP_MBIT_POS :
      index = codegen_gen_expr_one_line( exp->left, funit, FALSE );
      break;
    case EXP_OP_MBIT_NEG :
      {
        char* lstr = codegen_gen_expr_one_line( exp->left,  funit, FALSE );
        char* rstr = codegen_gen_expr_one_line( exp->right, funit, FALSE );
        slen  = 2 + strlen( lstr ) + 3 + strlen( rstr ) + 5;
        index = (char*)malloc_safe( slen );
        rv    = snprintf( index, slen, "((%s)-(%s))+1", lstr, rstr );
        assert( rv < slen );
        free_safe( lstr, (strlen( lstr ) + 1) );
        free_safe( rstr, (strlen( rstr ) + 1) );
      }
      break;
    default :
      assert( 0 );
      break;
  }

  /* Get the LSB of the current dimension */
  num = mod_parm_gen_lsb_code( exp->sig, dimension, funit_get_curr_module( funit ), &number );

  /* Adjust the index to get the true index */
  {
    char* tmp_index = index;
    if( num == NULL ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", number );
      assert( rv < 50 );
      slen  = 1 + strlen( tmp_index ) + 3 + strlen( tmp ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "(%s)-(%s)", tmp_index, tmp );
      assert( rv < slen );
    } else {
      slen  = 1 + strlen( tmp_index ) + 3 + strlen( num ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "(%s)-(%s)", tmp_index, num );
      assert( rv < slen );
      free_safe( num, (strlen( num ) + 1) );
    }
    free_safe( tmp_index, (strlen( tmp_index ) + 1) );
  }

  /* Get the dimensional width for the current expression */
  num = mod_parm_gen_size_code( exp->sig, dimension, funit_get_curr_module( funit ), &number );

  /* If the current dimension is big endian, recalculate the index value */
  if( exp->elem.dim->dim_be ) {
    char* tmp_index = index;
    if( num == NULL ) {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", (number - 1) );
      assert( rv < 50 );
      slen  = 1 + strlen( tmp ) + 3 + strlen( tmp_index ) + 2;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "(%s)-(%s)", tmp, tmp_index );
    } else {
      slen  = 2 + strlen( num ) + 5 + strlen( index ) + 1;
      index = (char*)malloc_safe( slen );
      rv    = snprintf( index, slen, "((%s)-1)-%s", num, tmp_index );
    }
    assert( rv < slen );
    free_safe( tmp_index, (strlen( tmp_index ) + 1) );
  }

  /* Create the full string for this dimension */
  if( ldim_width != NULL ) {
    slen = 1 + strlen( index ) + 3 + strlen( ldim_width ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, "(%s)*(%s)", index, ldim_width );
    assert( rv < slen );
  } else {
    str = strdup_safe( index );
  }

  if( dimension != 0 ) {

    char* width;

    /* Create the width of this dimension */
    if( num == NULL ) {
      num = convert_int_to_str( number );
    }

    if( ldim_width != NULL ) {
      slen  = 1 + strlen( ldim_width ) + 3 + strlen( num ) + 2;
      width = (char*)malloc_safe( slen );
      rv    = snprintf( width, slen, "(%s)*(%s)", ldim_width, num );
      assert( rv < slen );
    } else {
      width = strdup_safe( num );
    }

    /* Adding our generated value to the other dimensional information */
    {
      char* tmpstr = str;
      char* rest   = generator_gen_mem_index_helper( ((dimension == 1) ? exp->parent->expr->left : exp->parent->expr->left->right), funit, (dimension - 1), width );

      slen = 1 + strlen( tmpstr ) + 3 + strlen( rest ) + 2;
      str  = (char*)malloc_safe( slen );
      rv   = snprintf( str, slen, "(%s)+(%s)", tmpstr, rest );
      assert( rv < slen ); 

      free_safe( rest,   (strlen( rest )   + 1) );
      free_safe( tmpstr, (strlen( tmpstr ) + 1) );
    }

    /* Deallocate memory */
    free_safe( width, (strlen( width ) + 1) );

  }

  /* Deallocate memory */
  free_safe( num,   (strlen( num )   + 1) );
  free_safe( index, (strlen( index ) + 1) );

  PROFILE_END;

  return( str );

}

/*!
 \return Returns a string containing the memory index value to use for memory write/read coverage.

 Generates a memory index value for a given memory expression.
*/
static char* generator_gen_mem_index(
  expression* exp,       /*!< Pointer to expression accessing memory signal */
  func_unit*  funit,     /*!< Pointer to functional unit containing exp */
  int         dimension  /*!< Current memory dimension (should be initially set to expression_get_curr_dimension( exp ) */
) { PROFILE(GENERATOR_GEN_MEM_INDEX);

  char* ldim_width = NULL;
  char* str;

  /* If the dimension has not been set and its not the last dimension, calculate a last dimension value */
  if( dimension < ((exp->sig->udim_num + exp->sig->pdim_num) - 1) ) {
    int          number;
    int          slen;
    unsigned int rv;
    int          dim = (exp->sig->udim_num + exp->sig->pdim_num) - 1;
    char*        num = mod_parm_gen_size_code( exp->sig, dim--, funit_get_curr_module( funit ), &number );
    if( num == NULL ) {
      ldim_width = convert_int_to_str( number );
    } else {
      ldim_width = num;
    }
    for( ; dim>dimension; dim-- ) {
      char* tmp_str = ldim_width;
      num        = mod_parm_gen_size_code( exp->sig, dim, funit_get_curr_module( funit ), &number );
      if( num == NULL ) {
        num = convert_int_to_str( number );
      }
      slen       = 1 + strlen( tmp_str ) + 3 + strlen( num ) + 2;
      ldim_width = (char*)malloc_safe( slen );
      rv         = snprintf( ldim_width, slen, "(%s)*(%s)", tmp_str, num );
      assert( rv < slen );
      free_safe( tmp_str, (strlen( tmp_str ) + 1) );
      free_safe( num,     (strlen( num )     + 1) );
    }
  }

  /* Call the helper function to calculate the string */
  str = generator_gen_mem_index_helper( exp, funit, dimension, ldim_width );

  /* Deallocate memory */
  free_safe( ldim_width, (strlen( ldim_width ) + 1) );

  PROFILE_END;

  return( str );

}

#ifdef OBSOLETE
/*!
 \return Returns the string form of the overall size of the given memory.
*/
static char* generator_gen_mem_size(
  vsignal*   sig,  /*!< Pointer to signal to generate memory size information for */
  func_unit* mod   /*!< Pointer to module containing the given signal parameter sizers (module that signal exists in) */
) { PROFILE(GENERATOR_GEN_MEM_SIZE);

  unsigned int i;
  char*        curr_size;
  char*        size = NULL;
  unsigned int slen = 0;
  unsigned int rv;

  for( i=0; i<(sig->udim_num + sig->pdim_num); i++ ) {

    char* tmpsize = size;
    int   number;

    curr_size = mod_parm_gen_size_code( sig, i, mod, &number );

    if( number >= 0 )  {
      char tmp[50];
      rv = snprintf( tmp, 50, "%d", number );
      assert( rv < 50 );
      slen += strlen( tmp ) + 6;
      size  = (char*)malloc_safe( slen );
      rv    = snprintf( size, slen, "(%s)*(%s)", tmpsize, tmp );
    } else {
      slen += strlen( curr_size ) + 6;
      size  = (char*)malloc_safe( slen );
      rv    = snprintf( size, slen, "(%s)*(%s)", tmpsize, curr_size );
    }
    assert( rv < slen );

    free_safe( curr_size, (strlen( curr_size ) + 1) );
    free_safe( tmpsize,   (strlen( tmpsize ) + 1) );

  }

  PROFILE_END;

  return( size );

}
#endif

/*!
 \return Returns a string containing the LSB of the RHS to use to assign to this LHS expression.
*/
static char* generator_get_lhs_lsb_helper(
  expression* exp,   /*!< Pointer to LHS expression to get LSB information for */
  func_unit*  funit  /*!< Functional unit containing the given expression */
) { PROFILE(GENERATOR_GET_LHS_LSB_HELPER);

  char* lsb;

  if( exp != NULL ) {

    char*        right;
    char*        size;
    int          number;
    unsigned int rv;
    unsigned int slen;

    /* Get the LSB information for the right expression */
    if( (ESUPPL_IS_ROOT( exp->parent->expr->parent->expr->suppl ) == 0) && (exp->parent->expr->parent->expr->op != EXP_OP_CONCAT) ) {
      right = generator_get_lhs_lsb_helper( exp->parent->expr->parent->expr->right, funit );
    } else {
      right = strdup_safe( "0" );
    }

    /* Calculate our width */
    size = generator_gen_size( exp, funit, &number );

    /* Add our size to the size of the right expression */
    if( number >= 0 ) {
      char num[50];
      rv = snprintf( num, 50, "%d", number );
      assert( rv < 50 );
      slen = 1 + strlen( num ) + 3 + strlen( right ) + 2;
      lsb  = (char*)malloc_safe( slen );
      rv   = snprintf( lsb, slen, "(%s)+(%s)", num, right );
      assert( rv < slen );
    } else {
      slen = 1 + strlen( size ) + 3 + strlen( right ) + 2;
      lsb  = (char*)malloc_safe( slen );
      rv   = snprintf( lsb, slen, "(%s)+(%s)", size, right );
      assert( rv < slen );
    }

    free_safe( size,  (strlen( size ) + 1) );
    free_safe( right, (strlen( right ) + 1) );

  } else {

    lsb = strdup_safe( "0" );

  }

  PROFILE_END;

  return( lsb );

}

/*!
 \return Returns a string containing the LSB of the RHS to use to assign to this LHS expression.
*/
static char* generator_get_lhs_lsb(
  expression* exp,   /*!< Pointer to LHS expression to get LSB information for */
  func_unit*  funit  /*!< Pointer to functional unit containing this expression */
) { PROFILE(GENERATOR_GET_LHS_LSB);

  char* lsb;

  if( (exp != NULL) && (ESUPPL_IS_ROOT( exp->parent->expr->suppl ) == 0) && (exp->parent->expr->op != EXP_OP_NASSIGN) ) {

    if( exp->parent->expr->left == exp ) {
      lsb = generator_get_lhs_lsb_helper( exp->parent->expr->right, funit );
    } else if( exp->parent->expr->parent->expr->op != EXP_OP_CONCAT ) {
      lsb = generator_get_lhs_lsb_helper( exp->parent->expr->parent->expr->right, funit );
    } else {
      lsb = strdup_safe( "0" );
    }

  } else {
    
    lsb = strdup_safe( "0" );

  }

  PROFILE_END;

  return( lsb );

}

/*!
 Inserts memory coverage for the given expression.
*/
static void generator_insert_mem_cov(
  expression* exp,    /*!< Pointer to expression accessing memory signal */
  func_unit*  funit,  /*!< Pointer to functional unit containing the given expression */
  bool        net,    /*!< If TRUE, creates the signal name for a net; otherwise, creates the signal name for a register */
  bool        write,  /*!< If TRUE, creates write logic; otherwise, creates read logic */
  expression* rhs     /*!< If the root expression is a non-blocking assignment, this pointer will point to the RHS
                           expression that is required to extract memory coverage.  If this pointer is NULL, handle
                           memory coverage normally. */
) { PROFILE(GENERATOR_INSERT_MEM_COV);

  char         name[4096];
  char         range[4096];
  unsigned int rv;
  char*        idxstr   = generator_gen_mem_index( exp, funit, expression_get_curr_dimension( exp ) );
  char*        value;
  char*        str;
  expression*  last_exp = expression_get_last_line_expr( exp );
  char         num[50];
  char*        scope    = generator_get_relative_scope( funit );

  /* Figure out the size to store the number of dimensions */
  strcpy( num, "32" );

  /* Now insert the code to store the index and memory */
  if( write ) {

    char*        size;
    char*        memstr;
    unsigned int vlen;
    char         iname[4096];
    str_link*    tmp_head  = NULL;
    str_link*    tmp_tail  = NULL;
    int          number;
    expression*  first_exp = expression_get_first_select( exp );

    /* First, create the wire/register to hold the index */
    if( scope[0] == '\0' ) {
      rv = snprintf( iname, 4096, " \\covered$%c%u_%u_%x$%s ", (net ? 'i' : 'I'), exp->ppline, last_exp->ppline, exp->col, exp->name );
    } else {
      rv = snprintf( iname, 4096, " \\covered$%c%u_%u_%x$%s$%s ", (net ? 'i' : 'I'), exp->ppline, last_exp->ppline, exp->col, exp->name, scope );
    }
    assert( rv < 4096 );

    if( net ) {

      unsigned int slen = 7 + strlen( num ) + 7 + strlen( name ) + 3 + strlen( idxstr ) + 2;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "wire [(%s)-1:0] %s = %s;", num, iname, idxstr );
      assert( rv < slen );

    } else {

      unsigned int slen = 6 + strlen( num ) + 7 + strlen( iname ) + 3;

      str = (char*)malloc_safe( slen );
      rv  = snprintf( str, slen, "reg [(%s)-1:0] %s;\n", num, iname );
      assert( rv < slen );
      generator_insert_reg( str );
      free_safe( str, (strlen( str ) + 1) );

      slen = 1 + strlen( iname ) + 3 + strlen( idxstr ) + 2;
      str  = (char*)malloc_safe( slen );
      rv   = snprintf( str, slen, " %s = %s;", iname, idxstr );
      assert( rv < slen );

    }

    /* Prepend the index */
    (void)str_link_add( str, &tmp_head, &tmp_tail );

    /* Generate size needed to store memory element */
    size = generator_gen_size( exp, funit, &number );

    /*
     If the rhs expression is not NULL, we are within a non-blocking assignment so calculate the value that will be stored
     in the memory element.
    */
    if( rhs != NULL ) {

      char*       ename    = generator_create_expr_name( rhs );
      char        rhs_reg[4096];
      expression* last_rhs = expression_get_last_line_expr( rhs );
      char*       rhs_str;
      char*       lsb_str;
      char*       msb_str;

      /*
       We are reusing the comb_cntd bit in the expression supplemental field to indicate that this expression
       has or has not been created.
      */
      if( rhs->suppl.part.eval_t == 0 ) {

        char* size;
        int   number;

        /* Generate size needed to store memory element */
        size = generator_gen_size( rhs, funit, &number );

        if( number >= 0 ) {
          rv = snprintf( rhs_reg, 4096, "reg [%d:0] %s;\n", (number - 1), ename );
        } else {
          rv = snprintf( rhs_reg, 4096, "reg [(%s)-1:0] %s;\n", size, ename );
        }
        assert( rv < 4096 );
        generator_insert_reg( rhs_reg );

        /* Add the expression that will be assigned to the memory element */
        rhs_str = codegen_gen_expr_one_line( rhs, funit, FALSE );
        vlen    = strlen( ename ) + 3 + strlen( rhs_str ) + 2;
        value   = (char*)malloc_safe( vlen );
        rv      = snprintf( value, vlen, "%s = %s;", ename, rhs_str );
        assert( rv < vlen );
        free_safe( rhs_str, (strlen( rhs_str ) + 1) );

        /* Prepend the expression */
        (void)str_link_add( value, &tmp_head, &tmp_tail );

        /* Specify that the expression has been placed */
        rhs->suppl.part.eval_t = 1;

        free_safe( size, (strlen( size ) + 1) );

      }

      /* Generate the LSB of the RHS expression that needs to be assigned to this memory element */
      lsb_str = generator_get_lhs_lsb( exp, funit );

      /* Generate the MSB of the RHS expression that needs to be assigned to this memory element */
      if( number >= 0 ) {
        char num[50];
        rv = snprintf( num, 50, "%d", number );
        assert( rv < 50 );
        vlen    = 2 + strlen( num ) + 6 + strlen( lsb_str ) + 2;
        msb_str = (char*)malloc_safe( vlen );
        rv      = snprintf( msb_str, vlen, "((%s)-1)+(%s)", num, lsb_str );
        assert( rv < vlen );
      } else {
        vlen    = 2 + strlen( size ) + 6 + strlen( lsb_str ) + 2;
        msb_str = (char*)malloc_safe( vlen );
        rv      = snprintf( msb_str, vlen, "((%s)-1)+(%s)", size, lsb_str );
        assert( rv < vlen );
      }

      /* Generate the part select of the RHS expression to assign to this memory element */
      vlen   = strlen( ename ) + 1 + strlen( msb_str ) + 1 + strlen( lsb_str ) + 2;
      memstr = (char*)malloc_safe( vlen );
      rv     = snprintf( memstr, vlen, "%s[%s:%s]", ename, msb_str, lsb_str );

      free_safe( lsb_str, (strlen( lsb_str ) + 1) );
      free_safe( msb_str, (strlen( msb_str ) + 1) );
      free_safe( ename,   (strlen( ename )   + 1) );

    } else {

      memstr = codegen_gen_expr_one_line( first_exp, funit, FALSE );

    }

    /* Prepend the temporary list to the working list */
    if( work_head == NULL ) {
      work_head = work_tail = tmp_head;
    } else {
      tmp_tail->next = work_head;
      work_head      = tmp_head;
    }

    /* Create name */
    if( scope[0] == '\0' ) {
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s ", (net ? 'w' : 'W'), exp->ppline, last_exp->ppline, exp->col, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s$%s ", (net ? 'w' : 'W'), exp->ppline, last_exp->ppline, exp->col, exp->name, scope );
    }
    assert( rv < 4096 );

    /* Create the range information for the write */
    if( number >= 0 ) {
      rv = snprintf( range, 4096, "[%d+((%s)-1):0]", number, num );
    } else {
      rv = snprintf( range, 4096, "[(%s)+((%s)-1):0]", size, num );
    }
    assert( rv < 4096 );

    /* Create the value to assign */
    vlen   = 1 + strlen( memstr ) + 1 + strlen( iname ) + 2;
    value  = (char*)malloc_safe( vlen );
    rv = snprintf( value, vlen, "{%s,%s}", memstr, iname );
    assert( rv < vlen );

    /* Deallocate temporary strings */
    free_safe( size, (strlen( size ) + 1) );
    free_safe( memstr, (strlen( memstr ) + 1) );

  } else {

    /* Create name */
    if( scope[0] == '\0' ) {
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s ", (net ? 'r' : 'R'), exp->ppline, last_exp->ppline, exp->col, exp->name );
    } else {
      rv = snprintf( name, 4096, " \\covered$%c%u_%u_%x$%s$%s ", (net ? 'r' : 'R'), exp->ppline, last_exp->ppline, exp->col, exp->name, scope );
    }
    assert( rv < 4096 );

    /* Create the range information for the read */
    rv = snprintf( range, 4096, "[(%s)-1:0]", num );
    assert( rv < 4096 );

    /* Create the value to assign */
    value  = idxstr;
    idxstr = NULL;

  }

  /* Create the assignment string for a net */
  if( net ) {

    unsigned int slen = 4 + 1 + strlen( range ) + 1 + strlen( name ) + 3 + strlen( value ) + 2;

    str = (char*)malloc_safe( slen );
    rv  = snprintf( str, slen, "wire %s %s = %s;", range, name, value );
    assert( rv < slen );

  /* Otherwise, create the assignment string for a register and create the register */
  } else {

    unsigned int slen = 3 + 1 + strlen( range ) + 1 + strlen( name ) + 3;

    str = (char*)malloc_safe( slen );
    rv  = snprintf( str, slen, "reg %s %s;\n", range, name );
    assert( rv < slen );

    /* Add the register to the register list */
    generator_insert_reg( str );
    free_safe( str, (strlen( str ) + 1) );

    /* Now create the assignment string */
    slen = 1 + strlen( name ) + 3 + strlen( value ) + 2;
    str  = (char*)malloc_safe( slen );
    rv   = snprintf( str, slen, " %s = %s;", name, value );
    assert( rv < slen );

  }

  /* Append the line coverage assignment to the working buffer */
  generator_add_cov_to_work_code( str );
  generator_add_cov_to_work_code( "\n" );

  /* Deallocate temporary memory */
  free_safe( idxstr, (strlen( idxstr ) + 1) );
  free_safe( value,  (strlen( value )  + 1) );
  free_safe( str,    (strlen( str )    + 1) );
  free_safe( scope,  (strlen( scope )  + 1) );

  PROFILE_END;

}

/*!
 Traverses the specified expression tree searching for memory accesses.  If any are found, the appropriate
 coverage code is inserted into the output buffers.
*/
static void generator_insert_mem_cov_helper(
  expression* exp,           /*!< Pointer to current expression */
  func_unit*  funit,         /*!< Pointer to functional unit containing the expression */
  bool        net,           /*!< Specifies if the code generator should produce code for a net or a register */
  bool        do_read,       /*!< If TRUE, performs memory read access for any memories found in the expression tree (by default, set it to FALSE) */
  bool        do_write,      /*!< If TRUE, performs memory write access for any memories found in the expression tree (by default, set it to FALSE) */
  expression* rhs            /*!< Set to the RHS expression if the root expression was a non-blocking assignment */
) { PROFILE(GENERATOR_INSERT_MEM_COV_HELPER);

  if( exp != NULL ) {

    /* Generate code to perform memory write/read access */
    if( (exp->sig != NULL) && (exp->sig->suppl.part.type == SSUPPL_TYPE_MEM) && (exp->elem.dim != NULL) && exp->elem.dim->last ) {
      if( ((exp->suppl.part.lhs == 1) || do_write) && !do_read ) {
        generator_insert_mem_cov( exp, funit, net, TRUE, rhs );
      }
      if( (exp->suppl.part.lhs == 0) || do_read ) {
        generator_insert_mem_cov( exp, funit, net, FALSE, rhs );
      }
    }

    /* Get memory coverage for left expression */
    generator_insert_mem_cov_helper( exp->left,
                                     funit,
                                     net,
                                     ((exp->op == EXP_OP_SBIT_SEL) || (exp->op == EXP_OP_MBIT_SEL) || (exp->op == EXP_OP_MBIT_POS) ||
                                      (exp->op == EXP_OP_MBIT_NEG) || do_read),
                                     FALSE,
                                     rhs );

    /* Get memory coverage for right expression */
    generator_insert_mem_cov_helper( exp->right,
                                     funit,
                                     net,
                                     ((exp->op == EXP_OP_MBIT_SEL) || do_read),
                                     ((exp->op == EXP_OP_SASSIGN) && (exp->parent->expr != NULL) && ((exp->parent->expr->op == EXP_OP_SRANDOM) || (exp->parent->expr->op == EXP_OP_SURANDOM))),
                                     rhs );

  }

  PROFILE_END;

}

/*!
 \return Returns a pointer to the statement inserted (or NULL if no statement was inserted).

 Insert combinational logic coverage code for the given expression (by file position).
*/
statement* generator_insert_comb_cov(
  unsigned int first_line,    /*!< First line of expression to generate for */
  unsigned int first_column,  /*!< First column of expression to generate for */
  bool         net,           /*!< If set to TRUE, generate code for a net; otherwise, generate code for a variable */
  bool         use_right,     /*!< If set to TRUE, use right-hand expression */
  bool         save_stmt      /*!< If set to TRUE, saves the found statement to the statement stack */
) { PROFILE(GENERATOR_INSERT_COMB_COV);

  statement* stmt = NULL;

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( ((info_suppl.part.scored_comb == 1) || (info_suppl.part.scored_memory == 1)) && !handle_funit_as_assert &&
      ((stmt = generator_find_statement( first_line, first_column )) != NULL) && !generator_is_static_function_only( stmt->funit ) ) {

    /* Generate combinational coverage */
    if( info_suppl.part.scored_comb == 1 ) {
      generator_insert_comb_cov_helper( (use_right ? stmt->exp->right : stmt->exp), stmt->funit, (use_right ? stmt->exp->right->op : stmt->exp->op), net, TRUE, TRUE );
    }

    /* Generate memory coverage */
    if( info_suppl.part.scored_memory == 1 ) {
      generator_insert_mem_cov_helper( stmt->exp, stmt->funit, net, FALSE, FALSE, ((stmt->exp->op == EXP_OP_NASSIGN) ? stmt->exp->right : NULL) );
    }

  }

  /* If we need to save the found statement, do so now */
  if( save_stmt ) {

    stmt_loop_link* new_stmtl;

    assert( stmt != NULL );

    new_stmtl       = (stmt_loop_link*)malloc_safe( sizeof( stmt_loop_link ) );
    new_stmtl->stmt = stmt;
    new_stmtl->next = stmt_stack;
    new_stmtl->type = use_right ? 0 : 1;
    stmt_stack      = new_stmtl;

  }

  PROFILE_END;

  return( stmt );

}

/*!
 \return Returns a pointer to the inserted statement (or NULL if no statement was inserted).

 Inserts combinational coverage information from statement stack (and pop stack).
*/
statement* generator_insert_comb_cov_from_stmt_stack() { PROFILE(GENERATOR_INSERT_COMB_COV_FROM_STMT_STACK);

  statement* stmt = NULL;

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert ) {

    stmt_loop_link* sll;
    expression*     exp;

    assert( stmt_stack != NULL );

    stmt = stmt_stack->stmt;
    sll  = stmt_stack;
    exp  = stmt_stack->type ? stmt->exp->right : stmt->exp;

    /* Generate combinational coverage information */
    if( !generator_is_static_function_only( stmt->funit ) ) {
      generator_insert_comb_cov_helper( exp, stmt->funit, exp->op, FALSE, TRUE, FALSE );
    }

    /* Now pop the statement stack */
    stmt_stack = sll->next;
    free_safe( sll, sizeof( stmt_loop_link ) );

  }

  PROFILE_END;

  return( stmt );

}

/*!
 Inserts combinational coverage information for the given statement.
*/
void generator_insert_comb_cov_with_stmt(
  statement* stmt,       /*!< Pointer to statement to generate combinational logic coverage for */
  bool       use_right,  /*!< Specifies if the right expression should be used in the statement */
  bool       reg_needed  /*!< If TRUE, instantiate necessary registers */
) { PROFILE(GENERATOR_INSERT_COMB_COV_WITH_STMT);

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert && (stmt != NULL) && !generator_is_static_function_only( stmt->funit ) ) {

    expression* exp = use_right ? stmt->exp->right : stmt->exp;

    /* Insert combinational coverage */
    generator_insert_comb_cov_helper( exp, stmt->funit, exp->op, FALSE, TRUE, reg_needed );

  }

  PROFILE_END;

}

/*!
 Handles combinational logic for an entire case block (and its case items -- not the case item logic blocks).
*/
void generator_insert_case_comb_cov(
  unsigned int first_line,   /*!< First line number of first statement in case block */
  unsigned int first_column  /*!< First column of first statement in case block */
) { PROFILE(GENERATOR_INSERT_CASE_COMB_COV);

  statement* stmt;

  /* Insert combinational logic code coverage if it is specified on the command-line to do so and the statement exists */
  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert && ((stmt = generator_find_case_statement( first_line, first_column )) != NULL) &&
      !generator_is_static_function_only( stmt->funit ) ) {

    generator_insert_comb_cov_helper( stmt->exp->left, stmt->funit, stmt->exp->left->op, FALSE, TRUE, TRUE );

#ifdef FUTURE_ENHANCEMENT
    /* Generate covered for the current case item */
    generator_insert_comb_cov_helper( stmt->exp, stmt->funit, stmt->exp->op, FALSE, TRUE, TRUE );

    /* If the current statement is a case item type, handle it; otherwise, we are done */
    while( !stmt->suppl.part.stop_false &&
           ((stmt->next_false->exp->op == EXP_OP_CASE) || (stmt->next_false->exp->op == EXP_OP_CASEX) || (stmt->next_false->exp->op == EXP_OP_CASEZ)) ) {

      /* Move statement to next false statement */
      stmt = stmt->next_false;

      /* Generate covered for the current case item */
      generator_insert_comb_cov_helper( stmt->exp, stmt->funit, stmt->exp->op, FALSE, TRUE, TRUE );

    }
#endif

  }

  PROFILE_END;

}

/*!
 Inserts FSM coverage at the end of the module for the current module.
*/
void generator_insert_fsm_covs() { PROFILE(GENERATOR_INSERT_FSM_COVS);

  if( (info_suppl.part.scored_fsm == 1) && !handle_funit_as_assert && !generator_is_static_function_only( curr_funit ) ) {

    fsm_link*    fsml = curr_funit->fsm_head;
    unsigned int id   = 1;

    while( fsml != NULL ) {

      if( fsml->table->from_state->id == fsml->table->to_state->id ) {

        int   number;
        char* size = generator_gen_size( fsml->table->from_state, curr_funit, &number );
        char* exp  = codegen_gen_expr_one_line( fsml->table->from_state, curr_funit, FALSE );
        if( number >= 0 ) {
          fprintf( curr_ofile, "wire [%d:0] \\covered$F%u = %s;\n", (number - 1), id, exp );
        } else {
          fprintf( curr_ofile, "wire [(%s)-1:0] \\covered$F%u = %s;\n", ((size != NULL) ? size : "1"), id, exp );
        }
        free_safe( size, (strlen( size ) + 1) );
        free_safe( exp, (strlen( exp ) + 1) );

      } else {

        int   from_number;
        int   to_number;
        char* fsize = generator_gen_size( fsml->table->from_state, curr_funit, &from_number );
        char* fexp  = codegen_gen_expr_one_line( fsml->table->from_state, curr_funit, FALSE );
        char* tsize = generator_gen_size( fsml->table->to_state, curr_funit, &to_number );
        char* texp  = codegen_gen_expr_one_line( fsml->table->to_state, curr_funit, FALSE );
        if( from_number >= 0 ) {
          if( to_number >= 0 ) {
            fprintf( curr_ofile, "wire [%d:0] \\covered$F%u = {%s,%s};\n", ((from_number + to_number) - 1), id, fexp, texp );
          } else {
            fprintf( curr_ofile, "wire [(%d+(%s))-1:0] \\covered$F%u = {%s,%s};\n", from_number, ((tsize != NULL) ? tsize : "1"), id, fexp, texp );
          }
        } else {
          if( to_number >= 0 ) {
            fprintf( curr_ofile, "wire [((%s)+%d)-1:0] \\covered$F%u = {%s,%s};\n", ((fsize != NULL) ? fsize : "1"), to_number, id, fexp, texp );
          } else {
            fprintf( curr_ofile, "wire [((%s)+(%s))-1:0] \\covered$F%u = {%s,%s};\n",
                     ((fsize != NULL) ? fsize : "1"), ((tsize != NULL) ? tsize : "1"), id, fexp, texp );
          }
        }
        free_safe( fsize, (strlen( fsize ) + 1) );
        free_safe( fexp,  (strlen( fexp )  + 1) );
        free_safe( tsize, (strlen( tsize ) + 1) );
        free_safe( texp,  (strlen( texp )  + 1) );

      }

      fsml = fsml->next;
      id++;

    }

  }

  PROFILE_END;

}

/*!
 Replaces "event" types with "reg" types when performing combinational logic coverage.  This behavior
 is needed because events are impossible to discern coverage from when multiple events are used within a wait
 statement.
*/
void generator_handle_event_type( 
  unsigned int first_line,   /*!< First line of "event" type specifier */
  unsigned int first_column  /*!< First column of "event" type specifier */
) { PROFILE(GENERATOR_HANDLE_EVENT_TYPE);

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert ) {
    generator_replace( "reg", first_line, first_column, first_line, (first_column + 4) );
  }

  PROFILE_END;

}

/*!
 Transforms the event trigger to a register inversion (converts X to 0 if the identifier has not been initialized).
 This behavior is needed because events are impossible to discern coverage from when multiple events are used within a wait
 statement.
*/
void generator_handle_event_trigger(
  const char*  identifier,    /*!< Name of trigger identifier */
  unsigned int first_line,    /*!< First line of the '->' specifier */
  unsigned int first_column,  /*!< First column of the '->' specifier */
  unsigned int last_line,     /*!< Last line which contains the trigger identifier (not including the semicolon) */
  unsigned int last_column    /*!< Last column which contains the trigger identifier (not including the semicolon) */
) { PROFILE(GENERATOR_HANDLE_EVENT_TRIGGER);

  if( (info_suppl.part.scored_comb == 1) && !handle_funit_as_assert ) {

    char         str[4096];
    unsigned int rv = snprintf( str, 4096, "%s = (%s === 1'bx) ? 1'b0 : ~%s", identifier, identifier, identifier ); 
   
    /* Perform the replacement */
    generator_replace( str, first_line, first_column, last_line, last_column );

  }

  PROFILE_END;

}

