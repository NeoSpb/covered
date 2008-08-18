#ifndef __COMB_H__
#define __COMB_H__

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
 \file     comb.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting combinational logic coverage.
*/

#include <stdio.h>

#include "defines.h"


/*! \brief Resets combination counted bits in expression tree */
void combination_reset_counted_expr_tree( expression* exp );

/*! \brief Calculates combination logic statistics for a single expression tree */
void combination_get_tree_stats(
            expression*   exp,
            int*          ulid,
            unsigned int  curr_depth,
            bool          excluded,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excludes,
  /*@out@*/ unsigned int* total );

/*! \brief Calculates combination logic statistics for summary output */
void combination_get_stats(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Collects all toggle expressions that match the specified coverage indication. */
void combination_collect(
            func_unit*    funit,
            int           cov,
  /*@out@*/ expression*** exprs,
  /*@out@*/ unsigned int* exp_cnt,
  /*@out@*/ int**         excludes
);

/*! \brief Gets combinational logic summary statistics for specified functional unit */
void combination_get_funit_summary(
            func_unit*    funit,
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total
);

/*! \brief Gets combinational logic summary statistics for specified functional unit instance */
void combination_get_inst_summary(
            funit_inst*   inst,  
  /*@out@*/ unsigned int* hit,
  /*@out@*/ unsigned int* excluded,
  /*@out@*/ unsigned int* total  
);

/*! \brief Gets output for specified expression including underlines and code */
void combination_get_expression(
            int           expr_id,
  /*@out@*/ char***       code,
  /*@out@*/ int**         uline_groups,
  /*@out@*/ unsigned int* code_size,
  /*@out@*/ char***       ulines,
  /*@out@*/ unsigned int* uline_size,
  /*@out@*/ int**         excludes,
  /*@out@*/ unsigned int* exclude_size
);

/*! \brief Gets output for specified expression including coverage information */
void combination_get_coverage(
            int        exp_id,
            int        uline_id,
  /*@out@*/ char***    info,
  /*@out@*/ int*       info_size
);

/*! \brief Generates report output for combinational logic coverage. */
void combination_report(
  FILE* ofile,
  bool  verbose
);


/*
 $Log$
 Revision 1.25.6.4  2008/08/15 05:11:05  phase1geo
 Converting more old graphics to new style.  Updated documentation.  Cleaned up
 some issues with the build structure per recent documentation changes.  Also fixing
 some issues with the GUI and viewing combination logic coverage that is in an unnamed
 functional unit (more work to do here).  Checkpointing.

 Revision 1.25.6.3  2008/08/07 06:39:10  phase1geo
 Adding "Excluded" column to the summary listbox.

 Revision 1.25.6.2  2008/08/06 20:11:33  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.25.6.1  2008/07/10 22:43:50  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.26  2008/06/27 14:02:00  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.25  2008/01/16 05:01:22  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.24  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.23  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.22  2007/11/20 05:28:57  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.21  2007/04/02 20:19:36  phase1geo
 Checkpointing more work on use of functional iterators.  Not working correctly
 yet.

 Revision 1.20  2007/04/02 04:50:04  phase1geo
 Adding func_iter files to iterate through a functional unit for reporting
 purposes.  Updated affected files.

 Revision 1.19  2006/06/28 04:35:47  phase1geo
 Adding support for line coverage and fixing toggle and combinational coverage
 to redisplay main textbox to reflect exclusion changes.  Also added messageBox
 for close and exit menu options when a CDD has been changed but not saved to
 allow the user to do so before continuing on.

 Revision 1.18  2006/06/27 22:06:26  phase1geo
 Fixing more code related to exclusion.  The detailed combinational expression
 window now works correctly.  I am currently working on getting the main window
 text viewer to display exclusion correctly for all coverage metrics.  Still
 have a ways to go here.

 Revision 1.17  2006/06/26 04:12:55  phase1geo
 More updates for supporting coverage exclusion.  Still a bit more to go
 before this is working properly.

 Revision 1.16  2006/06/23 19:45:26  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.15  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.14  2006/02/06 22:48:34  phase1geo
 Several enhancements to GUI look and feel.  Fixed error in combinational logic
 window.

 Revision 1.13  2005/11/10 19:28:22  phase1geo
 Updates/fixes for tasks/functions.  Also updated Tcl/Tk scripts for these changes.
 Fixed bug with net_decl_assign statements -- the line, start column and end column
 information was incorrect, causing problems with the GUI output.

 Revision 1.12  2004/09/07 03:17:13  phase1geo
 Fixing bug that did not allow combinational logic to be revisited in GUI properly.
 Also removing comments from bgerror function in Tcl code.

 Revision 1.11  2004/08/17 15:23:37  phase1geo
 Added combinational logic coverage output to GUI.  Modified comb.c code to get this
 to work that impacts ASCII coverage output; however, regression is fully passing with
 these changes.  Combinational coverage for GUI is mostly complete regarding information
 and usability.  Possibly some cleanup in output and in code is needed.

 Revision 1.10  2004/08/17 04:43:57  phase1geo
 Updating unary and binary combinational expression output functions to create
 string arrays instead of immediately sending the information to standard output
 to support GUI interface as well as ASCII interface.

 Revision 1.9  2004/08/13 20:45:05  phase1geo
 More added for combinational logic verbose reporting.  At this point, the
 code is being output with underlines that can move up and down the expression
 tree.  No expression reporting is being done at this time, however.

 Revision 1.8  2004/08/11 22:11:39  phase1geo
 Initial beginnings of combinational logic verbose reporting to GUI.

 Revision 1.7  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.6  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.5  2002/10/31 23:13:23  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.2  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.
*/

#endif
