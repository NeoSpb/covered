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
 \file     info.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/12/2003
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "info.h"
#include "util.h"


extern char** merge_in;
extern int    merge_in_num;


/*!
 Informational line for the CDD file.
*/
isuppl info_suppl = {0};

/*!
 This string specifies the Verilog hierarchy leading up to the DUT.  This value is
 taken from the -i value (or is a value of '*' if the -t option is only specified).
*/
char** leading_hierarchies;

/*!
 Specifies the number of hierarchies stored in the leading_hierarchies array.
*/
int leading_hier_num;

/*!
 Set to TRUE if more than one leading hierarchy exists and it differs with the first leading hierarchy.
*/
bool leading_hiers_differ;

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
int cdd_version = CDD_VERSION;

/*!
 Specifes the pathname where the score command was originally run from.
*/
char score_run_path[4096];

/*!
 Array containing all of the score arguments.
*/
/*@null@*/ char** score_args = NULL;

/*!
 Number of valid elements in the score args array.
*/
int score_arg_num = 0;


/*!
 Initializes all variables used for information.
*/
void info_initialize() { PROFILE(INFO_INITIALIZE);

  leading_hier_num     = 0;
  leading_hiers_differ = FALSE;

}

/*!
 Sets the vector element size in the global info_suppl structure based on the current machine
 unsigned long byte size.
*/
void info_set_vector_elem_size() { PROFILE(INFO_SET_VECTOR_ELEM_SIZE);

  switch( sizeof( ulong ) ) {
    case 1 :  info_suppl.part.vec_ul_size = 0;  break;
    case 2 :  info_suppl.part.vec_ul_size = 1;  break;
    case 4 :  info_suppl.part.vec_ul_size = 2;  break;
    case 8 :  info_suppl.part.vec_ul_size = 3;  break;
    default:
      print_output( "Unsupported unsigned long size", FATAL, __FILE__, __LINE__ );
      Throw 0;
      break;
  }

}

/*!
 \param file  Pointer to file to write information to.
 
 Writes information line to specified file.
*/
void info_db_write( FILE* file ) { PROFILE(INFO_DB_WRITE);

  int i;  /* Loop iterator */

  assert( leading_hier_num > 0 );

  /* Calculate vector element size */
  info_set_vector_elem_size();

  fprintf( file, "%d %x %x %s %d",
           DB_TYPE_INFO,
           CDD_VERSION,
           info_suppl.all,
           leading_hierarchies[0],
           merge_in_num );

  /* Display any merge filename information */
  if( leading_hier_num == merge_in_num ) {
    for( i=0; i<merge_in_num; i++ ) {
      fprintf( file, " %s %s", merge_in[i], leading_hierarchies[i] );
    }
  } else {
    assert( (leading_hier_num - 1) == merge_in_num );
    for( i=0; i<merge_in_num; i++ ) {
      fprintf( file, " %s %s", merge_in[i], leading_hierarchies[i+1] );
    }
  }

  fprintf( file, "\n" );

  /* Display score arguments */
  fprintf( file, "%d %s", DB_TYPE_SCORE_ARGS, score_run_path );

  for( i=0; i<score_arg_num; i++ ) {
    fprintf( file, " %s", score_args[i] );
  }

  fprintf( file, "\n" );

}

/*!
 \param line  Pointer to string containing information line to parse.

 \throws anonymous Throw Throw Throw

 Reads information line from specified string and stores its information.
*/
void info_db_read( char** line ) { PROFILE(INFO_DB_READ);

  int          chars_read;     /* Number of characters scanned in from this line */
  control      scored;         /* Indicates if this file contains scored data */
  unsigned int version;        /* Contains CDD version from file */
  int          mnum;           /* Temporary merge num */
  char         tmp1[4096];     /* Temporary string */
  char         tmp2[4096];     /* Temporary string */
  int          i;              /* Loop iterator */

  /* Save off original scored value */
  scored = info_suppl.part.scored;

  if( sscanf( *line, "%x %x %s %d%n", &version, &(info_suppl.all), tmp1, &mnum, &chars_read ) == 4 ) {

    *line = *line + chars_read;

    if( version != CDD_VERSION ) {
      print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
      printf( "info Throw A\n" );
      Throw 0;
    }

    /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
    if( (leading_hier_num > 0) && (strcmp( leading_hierarchies[0], tmp1 ) != 0) ) {
      leading_hiers_differ = TRUE;
    }

    /* Assign this hierarchy to the leading hierarchies array */
    leading_hierarchies = (char**)realloc_safe( leading_hierarchies, (sizeof( char* ) * leading_hier_num), (sizeof( char* ) * (leading_hier_num + 1)) );
    leading_hierarchies[leading_hier_num] = strdup_safe( tmp1 );
    leading_hier_num++;

    for( i=0; i<mnum; i++ ) {

      if( sscanf( *line, "%s %s%n", tmp1, tmp2, &chars_read ) == 2 ) {

        *line = *line + chars_read;

        /* Add merged file */
        merge_in = (char**)realloc_safe( merge_in, (sizeof( char* ) * merge_in_num), (sizeof( char* ) * (merge_in_num + 1)) );
        merge_in[merge_in_num] = strdup_safe( tmp1 );
        merge_in_num++;

        /* Set leading_hiers_differ to TRUE if this is not the first hierarchy and it differs from the first */
        if( strcmp( leading_hierarchies[0], tmp2 ) != 0 ) {
          leading_hiers_differ = TRUE;
        }

        /* Add its hierarchy */
        leading_hierarchies = (char**)realloc_safe( leading_hierarchies, (sizeof( char* ) * leading_hier_num), (sizeof( char* ) * (leading_hier_num + 1)) );
        leading_hierarchies[leading_hier_num] = strdup_safe( tmp2 );
        leading_hier_num++;

      } else {
        print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
        printf( "info Throw B\n" );
        Throw 0;
      }

    }

    /* Set scored flag to correct value */
    if( info_suppl.part.scored == 0 ) {
      info_suppl.part.scored = scored;
    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    printf( "info Throw C\n" );
    Throw 0;

  }

}

/*!
 \param line  Pointer to string containing information line to parse.
 
 \throws anonymous Throw

 Reads score command-line args line from specified string and stores its information.
*/
void args_db_read( char** line ) { PROFILE(ARGS_DB_READ);

  int  chars_read;     /* Number of characters scanned in from this line */
  char tmp1[4096];     /* Temporary string */

  if( sscanf( *line, "%s%n", score_run_path, &chars_read ) == 1 ) {

    *line = *line + chars_read;

    /* Store score command-line arguments */
    while( sscanf( *line, "%s%n", tmp1, &chars_read ) == 1 ) {
      *line                     = *line + chars_read;
      score_args                = (char**)realloc_safe( score_args, (sizeof( char* ) * score_arg_num), (sizeof( char* ) * (score_arg_num + 1)) );
      score_args[score_arg_num] = strdup_safe( tmp1 );
      score_arg_num++;
    }

  } else {

    print_output( "CDD file being read is incompatible with this version of Covered", FATAL, __FILE__, __LINE__ );
    printf( "info Throw D\n" );
    Throw 0;

  }

}

/*!
 Deallocates all memory associated with the database information section.  Needs to be called
 when the database is closed.
*/
void info_dealloc() { PROFILE(INFO_DEALLOC);

  int i;  /* Loop iterator */

  /* Deallocate all information regarding hierarchies */
  for( i=0; i<leading_hier_num; i++ ) {
    free_safe( leading_hierarchies[i], (strlen( leading_hierarchies[i] ) + 1) );
  }
  free_safe( leading_hierarchies, (sizeof( char* ) * leading_hier_num) );

  leading_hierarchies = NULL;
  leading_hier_num    = 0;

  /* Free score arguments */
  for( i=0; i<score_arg_num; i++ ) {
    free_safe( score_args[i], (strlen( score_args[i] ) + 1) );
  }
  free_safe( score_args, (sizeof( char* ) * score_arg_num) );

  score_args    = NULL;
  score_arg_num = 0;

  /* Free merged arguments */
  for( i=0; i<merge_in_num; i++ ) {
    free_safe( merge_in[i], (strlen( merge_in[i] ) + 1) );
  }
  free_safe( merge_in, (sizeof( char* ) * merge_in_num) );

}

/*
 $Log$
 Revision 1.31.2.1  2008/05/28 22:12:31  phase1geo
 Adding further support for 32-/64-bit support.  Checkpointing.

 Revision 1.31  2008/03/31 21:40:23  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.30  2008/03/17 22:02:31  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.29  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.28  2008/03/14 22:00:19  phase1geo
 Beginning to instrument code for exception handling verification.  Still have
 a ways to go before we have anything that is self-checking at this point, though.

 Revision 1.27  2008/03/11 22:06:48  phase1geo
 Finishing first round of exception handling code.

 Revision 1.26  2008/03/10 22:00:31  phase1geo
 Working on more exception handling (script is finished now).  Starting to work
 on code enhancements again :)  Checkpointing.

 Revision 1.25  2008/03/04 22:46:08  phase1geo
 Working on adding check_exceptions.pl script to help me make sure that all
 exceptions being thrown are being caught and handled appropriately.  Other
 code adjustments are made in regards to exception handling.

 Revision 1.24  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.23  2008/01/16 06:40:37  phase1geo
 More splint updates.

 Revision 1.22  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.21  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.20  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.19  2006/10/13 15:56:02  phase1geo
 Updating rest of source files for compiler warnings.

 Revision 1.18  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.17  2006/05/02 21:49:41  phase1geo
 Updating regression files -- all but three diagnostics pass (due to known problems).
 Added SCORE_ARGS line type to CDD format which stores the directory that the score
 command was executed from as well as the command-line arguments to the score
 command.

 Revision 1.16  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.15  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

 Revision 1.14  2006/04/12 21:22:51  phase1geo
 Fixing problems with multi-file merging.  This now seems to be working
 as needed.  We just need to document this new feature.

 Revision 1.13  2006/04/12 18:06:24  phase1geo
 Updating regressions for changes that were made to support multi-file merging.
 Also fixing output of FSM state transitions to be what they were.
 Regressions now pass; however, the support for multi-file merging (beyond two
 files) has not been tested to this point.

 Revision 1.12  2006/04/12 13:28:37  phase1geo
 Fixing problem with memory allocation for merged files.

 Revision 1.11  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.10  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.9  2005/12/12 03:46:14  phase1geo
 Adding exclusion to score command to improve performance.  Updated regression
 which now fully passes.

 Revision 1.8  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.7  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.6  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.5  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.4  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.3  2003/10/17 02:12:38  phase1geo
 Adding CDD version information to info line of CDD file.  Updating regression
 for this change.

 Revision 1.2  2003/02/18 20:17:02  phase1geo
 Making use of scored flag in CDD file.  Causing report command to exit early
 if it is working on a CDD file which has not been scored.  Updated testsuite
 for these changes.

 Revision 1.1  2003/02/12 14:56:26  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

*/

