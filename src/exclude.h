#ifndef __EXCLUDE_H__
#define __EXCLUDE_H__

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
 \file     exclude.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     6/22/2006
 \brief    Contains functions for handling user-specified exclusion of coverage results.
*/

#include "defines.h"


/*! \brief Returns TRUE if the specified line is excluded in the given functional unit. */
bool exclude_is_line_excluded(
  func_unit* funit,
  int        line
);

/*! \brief Sets the excluded bit for all expressions in the given functional unit with the
           specified line number and recalculates the summary coverage information. */
void exclude_set_line_exclude(
            func_unit* funit,
            int        line,
            int        value,
  /*@out@*/ statistic* stat
);

/*! \brief Returns TRUE if the specified signal is excluded in the given functional unit. */
bool exclude_is_toggle_excluded(
  func_unit* funit,
  char*      sig_name
);

/*! \brief Sets the excluded bit for the specified signal in the given functional unit and
           recalculates the summary coverage information. */
void exclude_set_toggle_exclude(
            func_unit*  funit,
            const char* sig_name,
            int         value,
  /*@out@*/ statistic*  stat
);

/*! \return Returns TRUE if the specified expression is excluded in the given functional unit. */
bool exclude_is_comb_excluded(
  func_unit* funit,
  int        expr_id,
  int        uline_id
);

/*! \brief Sets the excluded bit for the specified expression in the given functional unit
           and recalculates the summary coverage information. */
void exclude_set_comb_exclude(
            func_unit* funit,
            int        expr_id,
            int        uline_id,
            int        value,
  /*@out@*/ statistic* stat
);

/*! \brief Returns TRUE if the specified FSM is excluded in the given functional unit. */
bool exclude_is_fsm_excluded(
  func_unit* funit,
  int        expr_id,
  char*      from_state,
  char*      to_state
);

/*! \brief Sets the excluded bit for the specified state transition in the given functional unit
           and recalculates the summary coverage information. */
void exclude_set_fsm_exclude(
            func_unit* funit,
            int        expr_id,
            char*      from_state,
            char*      to_state,
            int        value,
  /*@out@*/ statistic* stat
);

/*! \brief Returns TRUE if given assertion is excluded from coverage. */
bool exclude_is_assert_excluded(
  func_unit* funit,
  char*      inst_name,
  int        expr_id
);

/*! \brief Sets the excluded bit for the specified expression in the given functional unit
           and recalculates the summary coverage information. */
void exclude_set_assert_exclude(
            func_unit* funit,
            char*      inst_name,
            int        expr_id,
            int        value,
  /*@out@*/ statistic* stat
);


/*
 $Log$
 Revision 1.8.6.3  2008/08/07 23:22:49  phase1geo
 Added initial code to synchronize module and instance exclusion information.  Checkpointing.

 Revision 1.8.6.2  2008/08/07 18:03:51  phase1geo
 Fixing instance exclusion segfault issue with GUI.  Also cleaned up function
 documentation in link.c.

 Revision 1.8.6.1  2008/08/06 20:11:33  phase1geo
 Adding support for instance-based coverage reporting in GUI.  Everything seems to be
 working except for proper exclusion handling.  Checkpointing.

 Revision 1.8  2008/01/07 23:59:54  phase1geo
 More splint updates.

 Revision 1.7  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.6  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.5  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.4  2006/06/26 22:49:00  phase1geo
 More updates for exclusion of combinational logic.  Also updates to properly
 support CDD saving; however, this change causes regression errors, currently.

 Revision 1.3  2006/06/23 19:45:27  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.2  2006/06/23 04:03:30  phase1geo
 Updating build files and removing syntax errors in exclude.h and exclude.c
 (though this code doesn't do anything meaningful at this point).

 Revision 1.1  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

*/

#endif

