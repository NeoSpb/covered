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
 \file     toggle.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "db.h"
#include "defines.h"
#include "func_unit.h"
#include "link.h"
#include "obfuscate.h"
#include "ovl.h"
#include "toggle.h"
#include "util.h"
#include "vector.h"


extern db**         db_list;
extern unsigned int curr_db;

extern bool   report_covered;
extern bool   report_instance;
extern char** leading_hierarchies;
extern int    leading_hier_num;
extern bool   leading_hiers_differ;
extern isuppl info_suppl;


/*!
 \param sigl   Pointer to signal list to search.
 \param total  Total number of bits in the design/functional unit.
 \param hit01  Number of bits toggling from 0 to 1 during simulation.
 \param hit10  Number of bits toggling from 1 to 0 during simulation.

 Searches specified expression list and signal list, gathering information 
 about toggled bits.  For each bit that is found in the design, the total
 value is incremented by one.  For each bit that toggled from a 0 to a 1,
 the value of hit01 is incremented by one.  For each bit that toggled from
 a 1 to a 0, the value of hit10 is incremented by one.
*/
void toggle_get_stats( sig_link* sigl, int* total, int* hit01, int* hit10 ) { PROFILE(TOGGLE_GET_STATS);

  sig_link* curr_sig = sigl;  /* Current signal being evaluated */
  
  /* Search signal list */
  while( curr_sig != NULL ) {
    if( (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_PARAM) &&
        (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_ENUM)  &&
        (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_MEM)  &&
        (curr_sig->sig->suppl.part.mba == 0) ) {
      *total += curr_sig->sig->value->width;
      if( curr_sig->sig->suppl.part.excluded == 1 ) {
        *hit01 += curr_sig->sig->value->width;
        *hit10 += curr_sig->sig->value->width;
      } else {
        vector_toggle_count( curr_sig->sig->value, hit01, hit10 );
      }
    }
    curr_sig = curr_sig->next;
  }

}

/*!
 \param funit_name  Name of functional unit to gather coverage information from
 \param funit_type  Type of functional unit to gather coverage information from
 \param cov         Specifies to get uncovered (0) or covered (1) signals
 \param sig_head    Pointer to head of list of covered/uncovered signals
 \param sig_tail    Pointer to tail of list of covered/uncovered signals

 \return Returns TRUE if toggle information was found for the specified functional unit; otherwise,
         returns FALSE

 Searches the list of signals for the specified functional unit for signals that are either covered
 or uncovered.  When a signal is found that meets the requirements, signal is added to the signal list.
*/
bool toggle_collect( const char* funit_name, int funit_type, int cov, sig_link** sig_head, sig_link** sig_tail ) { PROFILE(TOGGLE_COLLECT);

  bool        retval = TRUE;  /* Return value for this function */
  funit_link* funitl;         /* Pointer to found functional unit link */
  sig_link*   curr_sig;       /* Pointer to current signal link being evaluated */
  int         hit01;          /* Number of bits that toggled from 0 to 1 */
  int         hit10;          /* Number of bits that toggled from 1 to 0 */
     
  if( (funitl = funit_link_find( funit_name, funit_type, db_list[curr_db]->funit_head )) != NULL ) {

    curr_sig = funitl->funit->sig_head;

    while( curr_sig != NULL ) {

      hit01 = 0;
      hit10 = 0;

      if( (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_PARAM) &&
          (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_ENUM)  &&
          (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_MEM)  &&
          (curr_sig->sig->suppl.part.mba == 0) ) {

        vector_toggle_count( curr_sig->sig->value, &hit01, &hit10 );

        /* If this signal meets the coverage requirement, add it to the signal list */
        if( ((cov == 1) && (hit01 == curr_sig->sig->value->width) && (hit10 == curr_sig->sig->value->width)) ||
	    ((cov == 0) && ((hit01 < curr_sig->sig->value->width) || (hit10 < curr_sig->sig->value->width))) ) {

          sig_link_add( curr_sig->sig, sig_head, sig_tail );
          
        }

      }

      curr_sig = curr_sig->next;

    }

  } else {
 
    retval = FALSE;

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit containing signal to get toggle coverage information for
 \param funit_type  Type of functional unit containing signal to get toggle coverage information for
 \param sig_name    Name of signal within the specified functional unit to get toggle coverage information for
 \param msb         Most-significant bit position of the requested signal
 \param lsb         Least-significant bit position of the requested signal
 \param tog01       Toggle vector of bits transitioning from a 0 to a 1
 \param tog10       Toggle vector of bits transitioning from a 1 to a 0
 \param excluded    Pointer to integer specifying if this signal should be excluded or not

 \return Returns TRUE if the specified functional unit and signal exists; otherwise, returns FALSE.

 Returns toggle coverage information for a specified signal in a specified functional unit.  This
 is needed by the GUI for verbose toggle coverage display.
*/
bool toggle_get_coverage( const char* funit_name, int funit_type, char* sig_name, int* msb, int* lsb, char** tog01, char** tog10, int* excluded ) { PROFILE(TOGGLE_GET_COVERAGE);

  bool        retval = TRUE;  /* Return value for this function */
  funit_link* funitl;         /* Pointer to found functional unit link */
  sig_link*   sigl;           /* Pointer to found signal link */

  if( (funitl = funit_link_find( funit_name, funit_type, db_list[curr_db]->funit_head )) != NULL ) {

    if( (sigl = sig_link_find( sig_name, funitl->funit->sig_head )) != NULL ) {
      assert( sigl->sig->dim != NULL );
      *msb      = sigl->sig->dim[0].msb;
      *lsb      = sigl->sig->dim[0].lsb; 
      *tog01    = vector_get_toggle01_ulong( sigl->sig->value->value.ul, sigl->sig->value->width );
      *tog10    = vector_get_toggle10_ulong( sigl->sig->value->value.ul, sigl->sig->value->width );
      *excluded = sigl->sig->suppl.part.excluded;
    } else {
      retval = FALSE;
    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param funit_name  Name of functional unit to retrieve summary information from.
 \param funit_type  Type of functional unit to retrieve summary information from.
 \param total       Pointer to total number of toggles in this functional unit.
 \param hit         Pointer to total number of toggles hit in this functional unit.

 \return Returns TRUE if specified functional unit was found in design; otherwise,
         returns FALSE.

 Looks up summary information for specified functional unit.  If the functional unit was found,
 the hit and total values for this functional unit are returned to the calling function.
 If the functional unit was not found, a value of FALSE is returned to the calling
 function, indicating that the functional unit was not found in the design and the values
 of total and hit should not be used.
*/
bool toggle_get_funit_summary( const char* funit_name, int funit_type, int* total, int* hit ) { PROFILE(TOGGLE_GET_FUNIT_SUMMARY);

  bool        retval = TRUE;  /* Return value for this function */
  funit_link* funitl;         /* Pointer to found functional unit link */
  sig_link*   curr_sig;       /* Pointer to current signal */
  int         hit01;          /* Number of bits toggling from a 0 to 1 */
  int         hit10;          /* Number of bits toggling from a 1 to 0 */

  /* Initialize total and hit */
  *total = 0;
  *hit   = 0;

  if( (funitl = funit_link_find( funit_name, funit_type, db_list[curr_db]->funit_head )) != NULL ) {

    curr_sig = funitl->funit->sig_head;

    while( curr_sig != NULL ) {

      hit01 = 0;
      hit10 = 0;

      if( (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_PARAM) &&
          (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_ENUM)  &&
          (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_MEM)  &&
          (curr_sig->sig->suppl.part.mba == 0) ) {

        /* We have found a valid signal to look at; therefore, increment the total */
        (*total)++;

        vector_toggle_count( curr_sig->sig->value, &hit01, &hit10 );

        if( ((hit01 == curr_sig->sig->value->width) && (hit10 == curr_sig->sig->value->width)) ||
            (curr_sig->sig->suppl.part.excluded == 1) ) {
          (*hit)++;
        }

      }

      curr_sig = curr_sig->next;

    }

  } else {

    retval = FALSE;

  }

  return( retval );

}

/*!
 \param ofile   Pointer to file to output summary information to
 \param name    Name of instance to output
 \param hits01  Number of bits that toggled from 0 -> 1
 \param hits10  Number of bits that toggled from 1 -> 0
 \param total   Total number of bits in given instance

 \return Returns TRUE if at least one bit was found to not be toggled; otherwise, returns FALSE.

 Displays the toggle instance summary information to the given output file, calculating the miss and
 percentage information.
*/
static bool toggle_display_instance_summary(
  FILE* ofile,
  char* name,
  int   hits01,
  int   hits10,
  int   total
) { PROFILE(TOGGLE_DISPLAY_INSTANCE_SUMMARY);

  float percent01;  /* Percentage of bits toggled from 0 -> 1 */
  float percent10;  /* Percentage of bits toggled from 1 -> 0 */
  int   miss01;     /* Number of bits not toggled from 0 -> 1 */
  int   miss10;     /* Number of bits not toggled from 1 -> 0 */

  calc_miss_percent( hits01, total, &miss01, &percent01 );
  calc_miss_percent( hits10, total, &miss10, &percent10 );

  fprintf( ofile, "  %-43.43s    %5d/%5d/%5d      %3.0f%%         %5d/%5d/%5d      %3.0f%%\n",
           name, hits01, miss01, total, percent01, hits10, miss10, total, percent10 );

  return( (miss01 > 0) || (miss10 > 0) );

}

/*!
 \param ofile        File to output coverage information to.
 \param root         Instance node in the functional unit instance tree being evaluated.
 \param parent_inst  Name of parent instance.
 \param hits01       Pointer to number of 0 -> 1 toggles that were hit in the given instance tree
 \param hits10       Pointer to number of 1 -> 0 toggles that were hit in the given instance tree
 \param total        Pointer to total number of bits that could be toggled in the given instance tree

 \return Returns TRUE if any bits were found to be not toggled; otherwise, returns FALSE.

 Displays the toggle instance summarization to the specified file.  Recursively
 iterates through functional unit instance tree, outputting the toggle information that
 is found at that instance.
*/
static bool toggle_instance_summary(
            FILE*       ofile,
            funit_inst* root,
            char*       parent_inst,
  /*@out@*/ int*        hits01,
  /*@out@*/ int*        hits10,
  /*@out@*/ int*        total
) { PROFILE(TOGGLE_INSTANCE_SUMMARY);

  funit_inst* curr;                /* Pointer to current child functional unit instance of this node */
  char        tmpname[4096];       /* Temporary name holder for instance */
  char*       pname;               /* Printable version of instance name */
  bool        miss_found = FALSE;  /* Specifies if at least one bit was not fully toggled */

  assert( root != NULL );
  assert( root->stat != NULL );

  /* Get printable version of this instance */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( root->stat->show && !funit_is_unnamed( root->funit ) &&
      ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit )) ) {

    miss_found |= toggle_display_instance_summary( ofile, tmpname, root->stat->tog01_hit, root->stat->tog10_hit, root->stat->tog_total );

    /* Update accumulated information */
    *hits01 += root->stat->tog01_hit;
    *hits10 += root->stat->tog10_hit;
    *total  += root->stat->tog_total;

  }

  /* If this is an assertion module, don't output any further */
  if( (info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( root->funit ) ) {

    curr = root->child_head;
    while( curr != NULL ) {
      miss_found |= toggle_instance_summary( ofile, curr, tmpname, hits01, hits10, total );
      curr = curr->next;
    }

  }

  return( miss_found );

}

/*!
 \param ofile   Pointer to file to output summary information to
 \param name    Name of instance to output
 \param fname   Name of file containing instance to output
 \param hits01  Number of bits that toggled from 0 -> 1
 \param hits10  Number of bits that toggled from 1 -> 0
 \param total   Total number of bits in given instance
  
 \return Returns TRUE if at least one bit was found to not be toggled; otherwise, returns FALSE.
  
 Displays the toggle functional unit summary information to the given output file, calculating the miss and
 percentage information.
*/
static bool toggle_display_funit_summary(
  FILE*       ofile,
  const char* name,
  const char* fname,
  int         hits01,
  int         hits10,
  int         total
) { PROFILE(TOGGLE_DISPLAY_FUNIT_SUMMARY);

  float percent01;  /* Percentage of bits that toggled from 0 to 1 */
  float percent10;  /* Percentage of bits that toggled from 1 to 0 */
  int   miss01;     /* Number of bits that did not toggle from 0 to 1 */
  int   miss10;     /* Number of bits that did not toggle from 1 to 0 */

  calc_miss_percent( hits01, total, &miss01, &percent01 );
  calc_miss_percent( hits10, total, &miss10, &percent10 );

  fprintf( ofile, "  %-20.20s    %-20.20s   %5d/%5d/%5d      %3.0f%%         %5d/%5d/%5d      %3.0f%%\n",
           name, fname, hits01, miss01, total, percent01, hits10, miss10, total, percent10 );

  return( (miss01 > 0) || (miss10 > 0) );

}

/*!
 \param ofile   Pointer to file to display coverage results to.
 \param head    Pointer to head of functional unit list to parse.
 \param hits01  Pointer to accumulated hit count for toggles 0 -> 1.
 \param hits10  Pointer to accumulated hit count for toggles 1 -> 0.
 \param total   Pointer to accumulated total of bits.

 \return Returns TRUE if any bits were found to be untoggled; otherwise, returns FALSE.

 Iterates through the functional unit list displaying the toggle coverage for
 each functional unit.
*/
static bool toggle_funit_summary(
            FILE*       ofile,
            funit_link* head,
  /*@out@*/ int*        hits01,
  /*@out@*/ int*        hits10,
  /*@out@*/ int*        total
) { PROFILE(TOGGLE_FUNIT_SUMMARY);

  bool  miss_found = FALSE;  /* Set to TRUE if missing toggles were found */
  char* pname;               /* Printable version of the functional unit name */

  while( head != NULL ) {

    /* If this is an assertion module, don't output any further */
    if( head->funit->stat->show && !funit_is_unnamed( head->funit ) &&
        ((info_suppl.part.assert_ovl == 0) || !ovl_is_assertion_module( head->funit )) ) {

      /* Get printable version of functional unit name */
      pname = scope_gen_printable( funit_flatten_name( head->funit ) );

      miss_found |= toggle_display_funit_summary( ofile, pname, get_basename( obf_file( head->funit->filename ) ),
                                                  head->funit->stat->tog01_hit, head->funit->stat->tog10_hit, head->funit->stat->tog_total );

      free_safe( pname, (strlen( pname ) + 1) );

      /* Update accumulated information */
      *hits01 += head->funit->stat->tog01_hit;
      *hits10 += head->funit->stat->tog10_hit;
      *total  += head->funit->stat->tog_total;

    }

    head = head->next;

  }

  return( miss_found );

}

/*!
 \param ofile  Pointer to file to output results to.
 \param sigl   Pointer to signal list head.

 Displays the signals that did not achieve 100% toggle coverage to standard 
 output from the specified signal list.
*/
static void toggle_display_verbose( FILE* ofile, sig_link* sigl ) { PROFILE(TOGGLE_DISPLAY_VERBOSE);

  sig_link* curr_sig;   /* Pointer to current signal link being evaluated */
  int       hit01;      /* Number of bits that toggled from 0 to 1 */
  int       hit10;      /* Number of bits that toggled from 1 to 0 */
  char*     pname;      /* Printable version of signal name */

  if( report_covered ) {
    fprintf( ofile, "    Signals getting 100%% toggle coverage\n\n" );
  } else {
    fprintf( ofile, "    Signals not getting 100%% toggle coverage\n\n" );
    fprintf( ofile, "      Signal                    Toggle\n" );
  }

  fprintf( ofile, "      ---------------------------------------------------------------------------------------------------------\n" );

  curr_sig = sigl;

  while( curr_sig != NULL ) {

    hit01 = 0;
    hit10 = 0;

    /* Get printable version of the signal name */
    pname = scope_gen_printable( curr_sig->sig->name );

    if( (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_PARAM) &&
        (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_ENUM)  &&
        (curr_sig->sig->suppl.part.type != SSUPPL_TYPE_MEM)  &&
        (curr_sig->sig->suppl.part.mba == 0) &&
        (curr_sig->sig->suppl.part.excluded == 0) ) {

      vector_toggle_count( curr_sig->sig->value, &hit01, &hit10 );

      if( report_covered ) {

        if( (hit01 == curr_sig->sig->value->width) && (hit10 == curr_sig->sig->value->width) ) {
        
          fprintf( ofile, "      %-24s\n", pname );

        }

      } else {

        if( (hit01 < curr_sig->sig->value->width) || (hit10 < curr_sig->sig->value->width) ) {

          fprintf( ofile, "      %-24s  0->1: ", pname );
          vector_display_toggle01_ulong( curr_sig->sig->value->value.ul, curr_sig->sig->value->width, ofile );      
          fprintf( ofile, "\n      ......................... 1->0: " );
          vector_display_toggle10_ulong( curr_sig->sig->value->value.ul, curr_sig->sig->value->width, ofile );      
          fprintf( ofile, " ...\n" );

        }

      }

    }

    free_safe( pname, (strlen( pname ) + 1) );

    curr_sig = curr_sig->next;

  }

}

/*!
 \param ofile        Pointer to file to display coverage results to.
 \param root         Pointer to root of instance functional unit tree to parse.
 \param parent_inst  Name of parent instance.

 Displays the verbose toggle coverage results to the specified output stream on
 an instance basis.  The verbose toggle coverage includes the signal names
 and their bits that did not receive 100% toggle coverage during simulation. 
*/
static void toggle_instance_verbose( FILE* ofile, funit_inst* root, char* parent_inst ) { PROFILE(TOGGLE_INSTANCE_VERBOSE);

  funit_inst* curr_inst;      /* Pointer to current instance being evaluated */
  char        tmpname[4096];  /* Temporary name holder of instance */
  char*       pname;          /* Printable version of the name */

  assert( root != NULL );

  /* Get printable version of the signal */
  pname = scope_gen_printable( root->name );

  if( db_is_unnamed_scope( pname ) ) {
    strcpy( tmpname, parent_inst );
  } else if( strcmp( parent_inst, "*" ) == 0 ) {
    strcpy( tmpname, pname );
  } else {
    unsigned int rv = snprintf( tmpname, 4096, "%s.%s", parent_inst, pname );
    assert( rv < 4096 );
  }

  free_safe( pname, (strlen( pname ) + 1) );

  if( !funit_is_unnamed( root->funit ) &&
      ((root->stat->tog01_hit < root->stat->tog_total) ||
       (root->stat->tog10_hit < root->stat->tog_total)) ) {

    fprintf( ofile, "\n" );
    switch( root->funit->type ) {
      case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
      case FUNIT_ANAMED_BLOCK :
      case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
      case FUNIT_AFUNCTION    :
      case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
      case FUNIT_ATASK        :
      case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
      default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
    }
    pname = scope_gen_printable( funit_flatten_name( root->funit ) );
    fprintf( ofile, "%s, File: %s, Instance: %s\n", pname, obf_file( root->funit->filename ), tmpname );
    fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );
    free_safe( pname, (strlen( pname ) + 1) );

    toggle_display_verbose( ofile, root->funit->sig_head );

  }

  curr_inst = root->child_head;
  while( curr_inst != NULL ) {
    toggle_instance_verbose( ofile, curr_inst, tmpname );
    curr_inst = curr_inst->next;
  }

}

/*!
 \param ofile  Pointer to file to display coverage results to.
 \param head   Pointer to head of functional unit list to parse.

 Displays the verbose toggle coverage results to the specified output stream on
 a functional unit basis (combining functional units that are instantiated multiple times).
 The verbose toggle coverage includes the signal names and their bits that
 did not receive 100% toggle coverage during simulation.
*/
static void toggle_funit_verbose( FILE* ofile, funit_link* head ) { PROFILE(TOGGLE_FUNIT_VERBOSE);

  while( head != NULL ) {

    if( !funit_is_unnamed( head->funit ) &&
        ((head->funit->stat->tog01_hit < head->funit->stat->tog_total) ||
         (head->funit->stat->tog10_hit < head->funit->stat->tog_total)) ) {

      fprintf( ofile, "\n" );
      switch( head->funit->type ) {
        case FUNIT_MODULE       :  fprintf( ofile, "    Module: " );       break;
        case FUNIT_ANAMED_BLOCK :
        case FUNIT_NAMED_BLOCK  :  fprintf( ofile, "    Named Block: " );  break;
        case FUNIT_AFUNCTION    :
        case FUNIT_FUNCTION     :  fprintf( ofile, "    Function: " );     break;
        case FUNIT_ATASK        :
        case FUNIT_TASK         :  fprintf( ofile, "    Task: " );         break;
        default                 :  fprintf( ofile, "    UNKNOWN: " );      break;
      }
      fprintf( ofile, "%s, File: %s\n", obf_funit( funit_flatten_name( head->funit ) ), obf_file( head->funit->filename ) );
      fprintf( ofile, "    -------------------------------------------------------------------------------------------------------------\n" );

      toggle_display_verbose( ofile, head->funit->sig_head );

    }

    head = head->next;

  }

}

/*!
 \param ofile     Pointer to file to output results to.
 \param verbose   Specifies whether or not to provide verbose information

 After the design is read into the functional unit hierarchy, parses the hierarchy by functional unit,
 reporting the toggle coverage for each functional unit encountered.  The parent functional unit will
 specify its own toggle coverage along with a total toggle coverage including its 
 children.
*/
void toggle_report( FILE* ofile, bool verbose ) { PROFILE(TOGGLE_REPORT);

  bool       missed_found = FALSE;  /* If set to TRUE, indicates that untoggled bits were found */
  char       tmp[4096];             /* Temporary string value */
  inst_link* instl;                 /* Pointer to current instance link */
  int        acc_hits01   = 0;      /* Accumulated 0 -> 1 toggle hit count */
  int        acc_hits10   = 0;      /* Accumulated 1 -> 0 toggle hit count */
  int        acc_total    = 0;      /* Accumulated bit count */

  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   TOGGLE COVERAGE RESULTS   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );
  fprintf( ofile, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n" );

  if( report_instance ) {

    if( leading_hiers_differ ) {
      strcpy( tmp, "<NA>" );
    } else {
      assert( leading_hier_num > 0 );
      strcpy( tmp, leading_hierarchies[0] );
    }

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Instance                                           Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    instl = db_list[curr_db]->inst_head;
    while( instl != NULL ) {
      missed_found |= toggle_instance_summary( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*"), &acc_hits01, &acc_hits10, &acc_total );
      instl = instl->next;
    }
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)toggle_display_instance_summary( ofile, "Accumulated", acc_hits01, acc_hits10, acc_total );
    
    if( verbose && missed_found ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      instl = db_list[curr_db]->inst_head;
      while( instl != NULL ) {
        toggle_instance_verbose( ofile, instl->inst, ((instl->next == NULL) ? tmp : "*") );
        instl = instl->next;
      }
    }

  } else {

    fprintf( ofile, "                                                           Toggle 0 -> 1                       Toggle 1 -> 0\n" );
    fprintf( ofile, "Module/Task/Function      Filename                 Hit/ Miss/Total    Percent hit      Hit/ Miss/Total    Percent hit\n" );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );

    missed_found = toggle_funit_summary( ofile, db_list[curr_db]->funit_head, &acc_hits01, &acc_hits10, &acc_total );
    fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
    (void)toggle_display_funit_summary( ofile, "Accumulated", "", acc_hits01, acc_hits10, acc_total );

    if( verbose && missed_found ) {
      fprintf( ofile, "---------------------------------------------------------------------------------------------------------------------\n" );
      toggle_funit_verbose( ofile, db_list[curr_db]->funit_head );
    }

  }

  fprintf( ofile, "\n\n" );

}

/*
 $Log$
 Revision 1.72.2.2  2008/05/28 05:57:12  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.72.2.1  2008/04/23 05:20:45  phase1geo
 Completed initial pass of code updates.  I can now begin testing...  Checkpointing.

 Revision 1.72  2008/04/15 20:37:11  phase1geo
 Fixing database array support.  Full regression passes.

 Revision 1.71  2008/03/17 22:02:32  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.70  2008/02/01 22:10:27  phase1geo
 Adding more diagnostics to regression suite for exclusion testing.
 Also fixed bug in toggle reporter to not output excluded signals in
 verbose output.

 Revision 1.69  2008/02/01 07:03:21  phase1geo
 Fixing bugs in pragma exclusion code.  Added diagnostics to regression suite
 to verify that we correctly exclude/include signals when pragmas are set
 around a register instantiation and the -ep is present/not present, respectively.
 Full regression passes at this point.  Fixed bug in vsignal.c where the excluded
 bit was getting lost when a CDD file was read back in.  Also fixed bug in toggle
 coverage reporting where a 1 -> 0 bit transition was not getting excluded when
 the excluded bit was set for a signal.

 Revision 1.68  2008/01/30 05:51:51  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.67  2008/01/16 23:10:34  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.66  2008/01/16 05:01:23  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.65  2008/01/09 05:22:22  phase1geo
 More splint updates using the -standard option.

 Revision 1.64  2008/01/07 23:59:55  phase1geo
 More splint updates.

 Revision 1.63  2007/12/11 23:19:14  phase1geo
 Fixed compile issues and completed first pass injection of profiling calls.
 Working on ordering the calls from most to least.

 Revision 1.62  2007/11/20 05:29:00  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.61  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.60  2007/07/31 03:36:10  phase1geo
 Fixing last known issue with automatic functions.  Also fixing issue with
 toggle report output (still a problem with the toggle calculation for the
 return value of the function).

 Revision 1.59  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.58  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.57  2007/07/16 12:39:33  phase1geo
 Started to add support for displaying accumulated coverage results for
 each metric.  Finished line and toggle and am half-way done with memory
 coverage (still have combinational logic, FSM and assertion coverage
 to complete before this feature is fully functional).

 Revision 1.56  2007/04/03 18:55:57  phase1geo
 Fixing more bugs in reporting mechanisms for unnamed scopes.  Checking in more
 regression updates per these changes.  Checkpointing.

 Revision 1.55  2007/04/03 04:15:17  phase1geo
 Fixing bugs in func_iter functionality.  Modified functional unit name
 flattening function (though this does not appear to be working correctly
 at this time).  Added calls to funit_flatten_name in all of the reporting
 files.  Checkpointing.

 Revision 1.54  2007/04/02 20:19:37  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

 Revision 1.53  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.52  2006/10/09 17:54:19  phase1geo
 Fixing support for VPI to allow it to properly get linked to the simulator.
 Also fixed inconsistency in generate reports and updated appropriately in
 regressions for this change.  Full regression now passes.

 Revision 1.51  2006/09/25 04:15:04  phase1geo
 Starting to add support for new memory coverage metric.  This includes changes
 for the report command only at this point.

 Revision 1.50  2006/09/20 22:38:10  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.49  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.48  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.47  2006/09/01 04:06:37  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.46  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

 Revision 1.45  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.44  2006/08/16 18:00:03  phase1geo
 Fixing things for good now (after the last submission).  Full regression
 passes with the exception of generate11.2 and generate11.3.

 Revision 1.43  2006/08/16 17:20:52  phase1geo
 Adding support for SystemVerilog data types bit, logic, byte, char, shortint,
 int, and longint.  Added diagnostics to verify correct behavior.  Also added
 generate11.2 and generate11.3 diagnostics which show a shortfall in Covered's
 handling of generate expressions within hierarchy names.

 Revision 1.42  2006/06/29 22:44:57  phase1geo
 Fixing newly introduced bug in FSM report handler.  Also adding pointers back
 to main text window when exclusion properties are changed.  Fixing toggle
 coverage retension.  This is partially working but doesn't seem to want to
 save/restore properly at this point.

 Revision 1.41  2006/06/26 04:12:55  phase1geo
 More updates for supporting coverage exclusion.  Still a bit more to go
 before this is working properly.

 Revision 1.40  2006/06/23 21:43:53  phase1geo
 More updates to include toggle exclusion (this does not work correctly at
 this time).

 Revision 1.39  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

 Revision 1.38  2006/05/25 12:11:02  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.37  2006/04/19 22:21:33  phase1geo
 More updates to properly support assertion coverage.  Removing assertion modules
 from line, toggle, combinational logic, FSM and race condition output so that there
 won't be any overlap of information here.

 Revision 1.36  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.35.8.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.35  2006/03/28 22:28:28  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.34  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.33  2006/02/17 19:50:47  phase1geo
 Added full support for escaped names.  Full regression passes.

 Revision 1.32  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.31  2006/01/19 00:01:09  phase1geo
 Lots of changes/additions.  Summary report window work is now complete (with the
 exception of adding extra features).  Added support for parsing left and right
 shift operators and the exponent operator in static expression scenarios.  Fixed
 issues related to GUI (due to recent changes in the score command).  Things seem
 to be generally working as expected with the GUI now.

 Revision 1.30  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.29  2005/11/10 19:28:23  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.28  2005/11/08 23:12:10  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.27  2004/08/08 12:50:27  phase1geo
 Snapshot of addition of toggle coverage in GUI.  This is not working exactly as
 it will be, but it is getting close.

 Revision 1.26  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.25  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.24  2004/01/30 23:23:27  phase1geo
 More report output improvements.  Still not ready with regressions.

 Revision 1.23  2004/01/30 06:04:45  phase1geo
 More report output format tweaks.  Adjusted lines and spaces to make things
 look more organized.  Still some more to go.  Regression will fail at this
 point.

 Revision 1.22  2004/01/23 14:37:41  phase1geo
 Fixing output of instance line, toggle, comb and fsm to only output module
 name if logic is detected missing in that instance.  Full regression fails
 with this fix.

 Revision 1.21  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.20  2003/10/03 12:31:04  phase1geo
 More report tweaking.

 Revision 1.19  2003/10/03 03:08:44  phase1geo
 Modifying filename in summary output to only specify basename of file instead
 of entire path.  The verbose report contains the full pathname still, however.

 Revision 1.18  2003/08/25 13:02:04  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.17  2003/02/23 23:32:36  phase1geo
 Updates to provide better cross-platform compiler support.

 Revision 1.16  2003/02/17 22:47:20  phase1geo
 Fixing bug with merging same DUTs from different testbenches.  Updated reports
 to display full path instead of instance name and parent instance name.  Added
 merge tests and added merge testing into regression test suite.  Fixing bug with
 -D/-Q option specified with merge command.  Full regression passing.

 Revision 1.15  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.14  2002/10/29 19:57:51  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.13  2002/10/01 13:21:25  phase1geo
 Fixing bug in report output for single and multi-bit selects.  Also modifying
 the way that parameters are dealt with to allow proper handling of run-time
 changing bit selects of parameter values.  Full regression passes again and
 all report generators have been updated for changes.

 Revision 1.12  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.11  2002/08/20 04:48:18  phase1geo
 Adding option to report command that allows the user to display logic that is
 being covered (-c option).  This overrides the default behavior of displaying
 uncovered logic.  This is useful for debugging purposes and understanding what
 logic the tool is capable of handling.

 Revision 1.10  2002/08/19 04:59:49  phase1geo
 Adjusting summary format to allow for larger line, toggle and combination
 counts.

 Revision 1.9  2002/07/20 18:46:38  phase1geo
 Causing fully covered modules to not be output in reports.  Adding
 instance3.v diagnostic to verify this works correctly.

 Revision 1.8  2002/07/17 06:27:18  phase1geo
 Added start for fixes to bit select code starting with single bit selection.
 Full regression passes with addition of sbit_sel1 diagnostic.

 Revision 1.7  2002/07/14 05:27:34  phase1geo
 Fixing report outputting to allow multiple modules/instances to be
 output.

 Revision 1.6  2002/07/09 03:24:48  phase1geo
 Various fixes for module instantiantion handling.  This now works.  Also
 modified report output for toggle, line and combinational information.
 Regression passes.

 Revision 1.5  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

