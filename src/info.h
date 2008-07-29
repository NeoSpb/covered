#ifndef __INFO_H__
#define __INFO_H__

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
 \file     info.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     2/12/2003
 \brief    Contains functions for reading/writing info line of CDD file.
*/

#include <stdio.h>


/*! \brief Initializes all information variables. */
void info_initialize();

/*! \brief Writes info line to specified CDD file. */
void info_db_write( FILE* file );

/*! \brief Reads info line from specified line and stores information. */
void info_db_read( char** line );

/*! \brief Reads score args line from specified line and stores information. */
void args_db_read( char** line );

/*! \brief Reads user-specified message from specified line and stores information. */
void message_db_read( char** line );

/*! \brief Reads merged CDD information from specified line and stores information. */
void merged_cdd_db_read( char** line );

/*! \brief Deallocates all memory associated with the information section of a database file. */
void info_dealloc();


/*
 $Log$
 Revision 1.8.4.2  2008/07/25 21:08:35  phase1geo
 Modifying CDD file format to remove the potential for memory allocation assertion
 errors due to a large number of merged CDD files.  Updating IV and Cver regressions per this
 change.

 Revision 1.8.4.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.8.6.1  2008/07/02 23:10:38  phase1geo
 Checking in work on rank function and addition of -m option to score
 function.  Added new diagnostics to verify beginning functionality.
 Checkpointing.

 Revision 1.8  2008/02/09 19:32:45  phase1geo
 Completed first round of modifications for using exception handler.  Regression
 passes with these changes.  Updated regressions per these changes.

 Revision 1.7  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.6  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.5  2006/05/02 21:49:41  phase1geo
 Updating regression files -- all but three diagnostics pass (due to known problems).
 Added SCORE_ARGS line type to CDD format which stores the directory that the score
 command was executed from as well as the command-line arguments to the score
 command.

 Revision 1.4  2006/05/01 22:27:37  phase1geo
 More updates with assertion coverage window.  Still have a ways to go.

 Revision 1.3  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.2  2004/01/31 18:58:43  phase1geo
 Finished reformatting of reports.  Fixed bug where merged reports with
 different leading hierarchies were outputting the leading hierarchy of one
 which lead to confusion when interpreting reports.  Also made modification
 to information line in CDD file for these cases.  Full regression runs clean
 with Icarus Verilog at this point.

 Revision 1.1  2003/02/12 14:56:27  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

*/

#endif

