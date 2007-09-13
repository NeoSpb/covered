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
 \file     ovl.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     04/13/2006
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "func_iter.h"
#include "func_unit.h"
#include "instance.h"
#include "iter.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "search.h"
#include "util.h"
#include "vector.h"


/*!
 Specifies the number of assertion modules that need to exist in the ovl_assertions string array.
*/
#define OVL_ASSERT_NUM 27


extern bool       flag_check_ovl_assertions;
extern inst_link* inst_head;
extern bool       report_covered;
extern bool       report_instance;


/*!
 Specifies the module names of all OVL assertions that contain functional coverage task calls.
*/
char* ovl_assertions[OVL_ASSERT_NUM] = { "assert_change",      "assert_cycle_sequence", "assert_decrement",     "assert_delta",
                                         "assert_even_parity", "assert_fifo_index",     "assert_frame",         "assert_handshake",
                                         "assert_implication", "assert_increment",      "assert_never_unknown", "assert_next",
                                         "assert_no_overflow", "assert_no_transition",  "assert_no_underflow",  "assert_odd_parity",
                                         "assert_one_cold",    "assert_one_hot",        "assert_range",         "assert_time",
                                         "assert_transition",  "assert_unchange",       "assert_width",         "assert_win_change",
                                         "assert_window",      "assert_win_unchange",   "assert_zero_one_hot" };

/*!
 \param name  Name of assertion module to check

 \return Returns TRUE if the specified name refers to a supported OVL coverage module; otherwise,
         returns FALSE.
*/
bool ovl_is_assertion_name( const char* name ) {

  int i = OVL_ASSERT_NUM;  /* Loop iterator */

  /* Rule out the possibility if the first 7 characters does not start with "assert" */
  if( strncmp( name, "assert_", 7 ) == 0 ) {

    i = 0;
    while( (i < OVL_ASSERT_NUM) && (strncmp( (name + 7), (ovl_assertions[i] + 7), strlen( ovl_assertions[i] + 7 ) ) != 0) ) {
      i++;
    }

  }

  return( i < OVL_ASSERT_NUM );

}

/*!
 \param funit  Pointer to the functional unit to check

 \return Returns TRUE if the specified module name is a supported OVL assertion; otherwise,
         returns FALSE.
*/
bool ovl_is_assertion_module( const func_unit* funit ) {

  bool        retval = FALSE;  /* Return value for this function */
  funit_link* funitl;          /* Pointer to current functional unit link */

  assert( funit != NULL );

  if( ovl_is_assertion_name( funit->name ) ) {

    /*
     If the module matches in name, check to see if the ovl_cover_t task exists (this
     will tell us if coverage was enabled for this module.
    */
    funitl = funit->tf_head;
    while( (funitl != NULL) && ((strcmp( funitl->funit->name, "ovl_cover_t" ) != 0) || (funitl->funit->type != FUNIT_TASK)) ) {
      funitl = funitl->next;
    }
    retval = (funitl == NULL);

  }

  return( retval );

}

/*!
 \param exp  Pointer to expression to check

 \return Returns TRUE if the specifies expression corresponds to a coverage point; otherwise, returns FALSE.
*/
bool ovl_is_coverage_point( const expression* exp ) {

  return( (exp->op == EXP_OP_TASK_CALL) && (strcmp( exp->name, "ovl_cover_t" ) == 0) );

}

/*!
 \param rm_tasks  Removes extraneous tasks only that are not used for coverage

 Adds all OVL assertions to the no_score list.
*/
void ovl_add_assertions_to_no_score_list( bool rm_tasks ) {

  int  i;          /* Loop iterator */
  char tmp[4096];  /* Temporary string holder */

  for( i=0; i<OVL_ASSERT_NUM; i++ ) {
    if( rm_tasks ) {
      snprintf( tmp, 4096, "%s.ovl_error_t", ovl_assertions[i] );
      search_add_no_score_funit( tmp );
      snprintf( tmp, 4096, "%s.ovl_finish_t", ovl_assertions[i] );
      search_add_no_score_funit( tmp );
      snprintf( tmp, 4096, "%s.ovl_init_msg_t", ovl_assertions[i] );
      search_add_no_score_funit( tmp );
    } else {
      search_add_no_score_funit( ovl_assertions[i] );
    }
  }

}

/*!
 \param funit  Pointer to functional unit to gather OVL assertion coverage information for
 \param total  Pointer to the total number of assertions in the given module
 \param hit    Pointer to the number of assertions hit in the given module during simulation

 Gathers the total and hit assertion coverage information for the specified functional unit and
 stores this information in the total and hit pointers.
*/
void ovl_get_funit_stats( const func_unit* funit, float* total, int* hit ) {

  funit_inst* funiti;      /* Pointer to found functional unit instance containing this functional unit */
  funit_inst* curr_child;  /* Current child of this functional unit's instance */
  int         ignore = 0;  /* Number of functional units to ignore */
  func_iter   fi;          /* Functional unit iterator */
  statement*  stmt;        /* Pointer to current statement */

  /* If the current functional unit is not an OVL module, get the statistics */
  if( !ovl_is_assertion_module( funit ) ) {

    /* Get one instance of this module from the design */
    funiti = inst_link_find_by_funit( funit, inst_head, &ignore );
    assert( funiti != NULL );

    /* Find all child instances of this module that are assertion modules */
    curr_child = funiti->child_head;
    while( curr_child != NULL ) {

      /* If this child instance module type is an assertion module, get its assertion information */
      if( (curr_child->funit->type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

        /* Initialize the functional unit iterator */
        func_iter_init( &fi, curr_child->funit );

        while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {

          /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
          if( ovl_is_coverage_point( stmt->exp ) ) {
            *total = *total + 1;
            if( (stmt->exp->exec_num > 0) || (ESUPPL_EXCLUDED( stmt->exp->suppl ) == 1) ) {
              (*hit)++;
            }
          }

        }

        /* Deallocate functional unit iterator */
        func_iter_dealloc( &fi );

      }

      /* Advance child pointer to next child instance */
      curr_child = curr_child->next;

    }

  }

}

/*!
 \param stmt  Pointer to statement of task call to coverage task

 \return Returns the string passed to the task call

 Finds the task call parameter that specifies the name of the coverage point and returns this
 string value to the calling function.
*/
char* ovl_get_coverage_point( statement* stmt ) {

  /*
   We are going to make a lot of assumptions about the structure of the statement, so just
   do a lot of assertions to make sure that our assumptions are correct.
  */
  assert( stmt != NULL );
  assert( stmt->exp != NULL );
  assert( stmt->exp->left != NULL );
  assert( stmt->exp->left->op == EXP_OP_PASSIGN );
  assert( stmt->exp->left->right != NULL );
  assert( stmt->exp->left->right->op == EXP_OP_STATIC );
  assert( stmt->exp->left->right->value != NULL );
  assert( stmt->exp->left->right->value->suppl.part.base == QSTRING );

  return( vector_to_string( stmt->exp->left->right->value ) );  

}

/*!
 \param ofile  Pointer to output file to display verbose information to
 \param funit  Pointer to module containing assertions that were not covered

 Displays the verbose hit/miss information to the given output file for the given functional
 unit.
*/
void ovl_display_verbose( FILE* ofile, const func_unit* funit ) {

  funit_inst* funiti;      /* Pointer to functional unit instance found for this functional unit */
  int         ignore = 0;  /* Specifies that we do not want to ignore any modules */
  funit_inst* curr_child;  /* Pointer to current child instance of this module */
  char*       cov_point;   /* Pointer to name of current coverage point */
  func_iter   fi;          /* Functional unit iterator */
  statement*  stmt;        /* Pointer to current statement */

  if( report_covered ) {
    fprintf( ofile, "      Instance Name               Assertion Name          Coverage Point                            # of hits\n" );
  } else {
    fprintf( ofile, "      Instance Name               Assertion Name          Coverage Point\n" );
  }
  fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );

  /* Get one instance of this module from the design */
  funiti = inst_link_find_by_funit( funit, inst_head, &ignore );
  assert( funiti != NULL );

  /* Find all child instances of this module that are assertion modules */
  curr_child = funiti->child_head;
  while( curr_child != NULL ) {

    /* If this child instance module type is an assertion module, check its assertion information */
    if( (curr_child->funit->type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

      /* Initialize the functional unit iterator */
      func_iter_init( &fi, curr_child->funit );

      while( (stmt = func_iter_get_next_statement( &fi )) != NULL ) {

        /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
        if( ovl_is_coverage_point( stmt->exp ) ) {

          /* Get coverage point name */
          cov_point = ovl_get_coverage_point( stmt );

          /* Output the coverage verbose results to the specified output file */
          if( (stmt->exp->exec_num == 0) && !report_covered ) {
            fprintf( ofile, "      %-26s  %-22s  \"%-38s\"\n",
                     obf_inst( curr_child->name ), obf_funit( funit_flatten_name( curr_child->funit ) ), cov_point );
          } else if( (stmt->exp->exec_num > 0) && report_covered ) {
            fprintf( ofile, "      %-26s  %-22s  \"%-38s\"  %9d\n",
                     obf_inst( curr_child->name ), obf_funit( funit_flatten_name( curr_child->funit ) ), cov_point, stmt->exp->exec_num );
          }
          
        }

      }

      /* Deallocate functional unit iterator */
      func_iter_dealloc( &fi );

    }

    /* Advance child pointer to next child instance */
    curr_child = curr_child->next;

  }

}

/*!
 \param funit             Pointer to functional unit to gather uncovered/covered assertion instances from
 \param uncov_inst_names  Pointer to array of uncovered instance names in the specified functional unit
 \param excludes          Pointer to array of integers indicating exclusion of associated uncovered instance name
 \param uncov_inst_size   Number of valid elements in the uncov_inst_names array
 \param cov_inst_names    Pointer to array of covered instance names in the specified functional unit
 \param cov_inst_size     Number of valid elements in the cov_inst_names array
 
 Populates the uncovered and covered string arrays with the instance names of child modules that match the
 respective coverage level.
*/
void ovl_collect( func_unit* funit, char*** uncov_inst_names, int** excludes, int* uncov_inst_size,
                  char*** cov_inst_names, int* cov_inst_size ) {
  
  funit_inst* funiti;             /* Pointer to found functional unit instance containing this functional unit */
  funit_inst* curr_child;         /* Current child of this functional unit's instance */
  int         ignore        = 0;  /* Number of functional units to ignore */
  stmt_iter   si;                 /* Statement iterator */
  int         total;              /* Total number of coverage points for a given assertion module */
  int         hit;                /* Number of hit coverage points for a given assertion module */
  int         exclude_found = 0;  /* Set to a value of 1 if at least one excluded coverage point was found */

  /* Get one instance of this module from the design */
  funiti = inst_link_find_by_funit( funit, inst_head, &ignore );
  assert( funiti != NULL );

  /* Find all child instances of this module that are assertion modules */
  curr_child = funiti->child_head;
  while( curr_child != NULL ) {

    /* If this child instance module type is an assertion module, get its assertion information */
    if( (curr_child->funit->type == FUNIT_MODULE) && ovl_is_assertion_module( curr_child->funit ) ) {

      /* Initialize the total and hit values */
      total = 0;
      hit   = 0;

      stmt_iter_reset( &si, curr_child->funit->stmt_head );
      while( si.curr != NULL ) {

        /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
        if( ovl_is_coverage_point( si.curr->stmt->exp ) ) {
          total = total + 1;
          if( (si.curr->stmt->exp->exec_num > 0) || (ESUPPL_EXCLUDED( si.curr->stmt->exp->suppl ) == 1) ) {
            hit++;
            exclude_found |= ESUPPL_EXCLUDED( si.curr->stmt->exp->suppl );
          }
        }

        stmt_iter_next( &si );

      }

    }
    
    /* If there are uncovered coverage points, add this instance to the uncov array */
    if( hit < total ) {
      *uncov_inst_names = (char**)realloc( *uncov_inst_names, (sizeof( char** ) * (*uncov_inst_size + 1)) );
      *excludes         = (int*)  realloc( *excludes,         (sizeof( int )    * (*uncov_inst_size + 1)) );
      (*uncov_inst_names)[*uncov_inst_size] = strdup_safe( curr_child->name, __FILE__, __LINE__ );
      (*excludes)[*uncov_inst_size]         = 0;
      (*uncov_inst_size)++;
      
    /* Otherwise, populate the cov array */
    } else {
      if( exclude_found == 1 ) {
        *uncov_inst_names = (char**)realloc( *uncov_inst_names, (sizeof( char** ) * (*uncov_inst_size + 1)) );
        *excludes         = (int*)  realloc( *excludes,         (sizeof( int )    * (*uncov_inst_size + 1)) );
        (*uncov_inst_names)[*uncov_inst_size] = strdup_safe( curr_child->name, __FILE__, __LINE__ );
        (*excludes)[*uncov_inst_size]         = 1;
        (*uncov_inst_size)++;
      } else {
        *cov_inst_names = (char**)realloc( *cov_inst_names, (sizeof( char** ) * (*cov_inst_size + 1)) );
        (*cov_inst_names)[*cov_inst_size] = strdup_safe( curr_child->name, __FILE__, __LINE__ );
        (*cov_inst_size)++;
      }
    }

    /* Advance child pointer to next child instance */
    curr_child = curr_child->next;

  }

}

/*!
 \param funit       Pointer to functional unit to get assertion information from.
 \param inst_name   Name of assertion instance to get coverage points from.
 \param assert_mod  Pointer to the assertion module name of instance being retrieved
 \param cp_head     Pointer to head of coverage point list.
 \param cp_tail     Pointer to tail of coverage point list.

 Retrieves the coverage point strings and execution counts from the specified assertion module.
*/
void ovl_get_coverage( const func_unit* funit, const char* inst_name, char** assert_mod, str_link** cp_head, str_link** cp_tail ) {

  funit_inst* funiti;      /* Pointer to found functional unit instance */
  funit_inst* curr_child;  /* Pointer to current child functional instance */
  int         ignore = 0;  /* Number of functional units to ignore */
  stmt_iter   si;          /* Statement iterator */

  /* Get one instance of this module from the design */
  funiti = inst_link_find_by_funit( funit, inst_head, &ignore );
  assert( funiti != NULL );

  /* Find child instance */
  curr_child = funiti->child_head;
  while( (curr_child != NULL) && (strcmp( curr_child->name, inst_name ) != 0) ) {
    curr_child = curr_child->next;
  }
  assert( curr_child != NULL );
  
  /* Get the module name and store it in assert_mod */
  *assert_mod = strdup_safe( curr_child->funit->name, __FILE__, __LINE__ );

  /* Gather all missed coverage points */
  stmt_iter_reset( &si, curr_child->funit->stmt_head );
  while( si.curr != NULL ) {

    /* If this statement is a task call to the task "ovl_cover_t", get its total and hit information */
    if( ovl_is_coverage_point( si.curr->stmt->exp ) ) {

      /* Store the coverage point string and execution count */
      str_link_add( ovl_get_coverage_point( si.curr->stmt ), cp_head, cp_tail );
      (*cp_tail)->suppl  = si.curr->stmt->exp->exec_num;
      (*cp_tail)->suppl2 = si.curr->stmt->exp->id;
      (*cp_tail)->suppl3 = ESUPPL_EXCLUDED( si.curr->stmt->exp->suppl );

    }

    stmt_iter_next( &si );

  }

}


/*
 $Log$
 Revision 1.13  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.12  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.11  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.10  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.9  2006/06/23 19:45:27  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.8  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

 Revision 1.7  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.6  2006/05/01 13:19:07  phase1geo
 Enhancing the verbose assertion window.  Still need to fix a few bugs and add
 a few more enhancements.

 Revision 1.5  2006/04/29 05:12:14  phase1geo
 Adding initial version of assertion verbose window.  This is currently working; however,
 I think that I want to enhance this window a bit more before calling it good.

 Revision 1.4  2006/04/28 17:10:19  phase1geo
 Adding GUI support for assertion coverage.  Halfway there.

 Revision 1.3  2006/04/21 22:03:58  phase1geo
 Adding ovl1 and ovl1.1 diagnostics to testsuite.  ovl1 passes while ovl1.1
 currently fails due to a problem with outputting parameters to the CDD file
 (need to look into this further).  Added OVL assertion verbose output support
 which seems to be working for the moment.

 Revision 1.2  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.1  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

*/

