#ifndef __DB_H__
#define __DB_H__

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
 \file     db.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     12/7/2001
 \brief    Contains functions for writing and reading contents of
           covered database file.
*/

#include "defines.h"


/*! \brief Creates a new database. */
db* db_create();

/*! \brief Deallocates all memory consumed by the database. */
void db_close();

/*! \brief Checks to see if the module specified by the -t option is the top-level module of the simulator. */
bool db_check_for_top_module();

/*! \brief Writes contents of expressions, functional units and vsignals to database file. */
void db_write(
  const char* file,
  bool        parse_mode,
  bool        report_save
);

/*! \brief Reads contents of database file and stores into internal lists. */
void db_read(
  const char* file,
  int         read_mode
);

/*! \brief After functional units have been read, merge the contents of the functional units (used in GUI only). */
void db_merge_funits();

/*! \brief Returns a scaled version of the given value to the timescale for the given functional unit. */
uint64 db_scale_to_precision( uint64 value, func_unit* funit );

/*! \brief Sets the global timescale unit and precision variables. */
void db_set_timescale( int unit, int precision );

/*! \brief Returns a pointer to the current functional unit. */
func_unit* db_get_curr_funit();

/*! \brief Creates a scope name for an unnamed scope.  Called only during parsing. */
char* db_create_unnamed_scope();

/*! Returns TRUE if the given scope is an unnamed scope name; otherwise, returns FALSE. */
bool db_is_unnamed_scope( char* scope );

/*! \brief Adds specified functional unit node to functional unit tree.  Called by parser. */
func_unit* db_add_instance( char* scope, char* name, int type, vector_width* range );

/*! \brief Adds specified module to module list.  Called by parser. */
void db_add_module( char* name, char* file, int start_line );

/*! \brief Adds specified task/function to functional unit list.  Called by parser. */
bool db_add_function_task_namedblock( int type, char* name, char* file, int start_line );

/*! \brief Performs actions necessary when the end of a function/task/named-block is seen.  Called by parser. */
void db_end_function_task_namedblock( int end_line );

/*! \brief Adds specified declared parameter to parameter list.  Called by parser. */
void db_add_declared_param( bool is_signed, static_expr* msb, static_expr* lsb, char* name, expression* expr, bool local );

/*! \brief Adds specified override parameter to parameter list.  Called by parser. */
void db_add_override_param( char* inst_name, expression* expr, char* param_name );

/*! \brief Adds specified defparam to parameter override list.  Called by parser. */
void db_add_defparam( char* name, expression* expr );

/*! \brief Adds specified vsignal to vsignal list.  Called by parser. */
void db_add_signal( char* name, int type, sig_range* prange, sig_range* urange, bool is_signed, bool mba, int line, int col, bool handled );

/*! \brief Creates statement block that acts like a fork join block from a standard statement block */
statement* db_add_fork_join( statement* stmt );

/*! \brief Creates an enumerated list based on the given parameters */
void db_add_enum( vsignal* enum_sig, static_expr* value );

/*! \brief Called after all enumerated values for the current list have been added */
void db_end_enum_list();

/*! \brief Adds given typedefs to the database */
void db_add_typedef( const char* name, bool is_signed, bool is_handled, bool is_sizable, sig_range* prange, sig_range* urange );

/*! \brief Called when the endmodule keyword is parsed. */
void db_end_module( int end_line );

/*! \brief Called when the endfunction or endtask keyword is parsed. */
void db_end_function_task( int end_line );

/*! \brief Finds specified signal in functional unit and returns pointer to the signal structure.  Called by parser. */
vsignal* db_find_signal( char* name, bool okay_if_not_found );

/*! \brief Adds a generate block to the database.  Called by parser. */
void db_add_gen_item_block( gen_item* gi );

/*! \brief Find specified generate item in the current functional unit.  Called by parser. */
gen_item* db_find_gen_item( gen_item* root, gen_item* gi );

/*! \brief Finds specified typedef and returns TRUE if it is found */
typedef_item* db_find_typedef( const char* name );

/*! \brief Returns a pointer to the current implicitly connected generate block.  Called by parser. */
gen_item* db_get_curr_gen_block();

/*! \brief Creates new expression from specified information.  Called by parser and db_add_expression. */
expression* db_create_expression( expression* right, expression* left, exp_op_type op, bool lhs, int line, int first, int last, char* sig_name );

/*! \brief Binds all necessary sub-expressions in the given tree to the given signal name */
void db_bind_expr_tree( expression* root, char* sig_name );

/*! \brief Creates an expression from the specified static expression */
expression* db_create_expr_from_static( static_expr* se, int line, int first_col, int last_col );

/*! \brief Adds specified expression to expression list.  Called by parser. */
void db_add_expression( expression* root );

/*! \brief Creates an expression tree sensitivity list for the given statement block */
expression* db_create_sensitivity_list( statement* stmt );

/*! \brief Checks specified statement for parallelization and if it must be, creates a parallel statement block */
statement* db_parallelize_statement( statement* stmt );

/*! \brief Creates new statement expression from specified information.  Called by parser. */
statement* db_create_statement( expression* exp );

/*! \brief Adds specified statement to current functional unit's statement list.  Called by parser. */
void db_add_statement( statement* stmt, statement* start );

/*! \brief Removes specified statement from current functional unit. */
void db_remove_statement_from_current_funit( statement* stmt );

/*! \brief Removes specified statement and associated expression from list and memory. */
void db_remove_statement( statement* stmt );

/*! \brief Connects gi2 to the true path of gi1 */
void db_gen_item_connect_true( gen_item* gi1, gen_item* gi2 );

/*! \brief Connects gi2 to the false path of gi1 */
void db_gen_item_connect_false( gen_item* gi1, gen_item* gi2 );

/*! \brief Connects one generate item block to another. */
void db_gen_item_connect( gen_item* gi1, gen_item* gi2 );

/*! \brief Connects one statement block to another. */
bool db_statement_connect( statement* curr_stmt, statement* next_stmt );

/*! \brief Connects true statement to specified statement. */
void db_connect_statement_true( statement* stmt, statement* exp_true );

/*! \brief Connects false statement to specified statement. */
void db_connect_statement_false( statement* stmt, statement* exp_false );

/*! \brief Allocates and initializes an attribute parameter. */
attr_param* db_create_attr_param( char* name, expression* expr );

/*! \brief Parses the specified attribute parameter list for Covered attributes */
void db_parse_attribute( attr_param* ap );

/*! \brief Searches entire design for expressions that call the specified statement */
void db_remove_stmt_blks_calling_statement( statement* stmt );

/*! \brief Synchronizes the curr_instance pointer to match the curr_inst_scope hierarchy */
void db_sync_curr_instance();

/*! \brief Sets current VCD scope to specified scope. */
void db_set_vcd_scope( const char* scope );

/*! \brief Moves current VCD hierarchy up one level */
void db_vcd_upscope();

/*! \brief Adds symbol to signal specified by name. */
void db_assign_symbol( const char* name, const char* symbol, int msb, int lsb );

/*! \brief Sets the found symbol value to specified character value.  Called by VCD lexer. */
void db_set_symbol_char( const char* sym, char value );

/*! \brief Sets the found symbol value to specified string value.  Called by VCD lexer. */
void db_set_symbol_string( const char* sym, const char* value );

/*! \brief Performs a timestep for all signal changes during this timestep. */
bool db_do_timestep( uint64 time, bool final ); 


/*
 $Log$
 Revision 1.88.4.1  2008/07/10 22:43:50  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.89  2008/06/28 03:46:28  phase1geo
 More code updates for warning removal.

 Revision 1.88  2008/04/15 13:59:13  phase1geo
 Starting to add support for multiple databases.  Things compile but are
 quite broken at the moment.  Checkpointing.

 Revision 1.87  2008/04/15 06:08:46  phase1geo
 First attempt to get both instance and module coverage calculatable for
 GUI purposes.  This is not quite complete at the moment though it does
 compile.

 Revision 1.86  2008/03/11 22:06:47  phase1geo
 Finishing first round of exception handling code.

 Revision 1.85  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.84  2008/02/10 03:33:13  phase1geo
 More exception handling added and fixed remaining splint errors.

 Revision 1.83  2008/02/08 23:58:06  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.82  2008/01/30 05:51:50  phase1geo
 Fixing doxygen errors.  Updated parameter list syntax to make it more readable.

 Revision 1.81  2008/01/15 23:01:14  phase1geo
 Continuing to make splint updates (not doing any memory checking at this point).

 Revision 1.80  2008/01/10 04:59:04  phase1geo
 More splint updates.  All exportlocal cases are now taken care of.

 Revision 1.79  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.78  2007/09/12 05:40:11  phase1geo
 Adding support for bool and char types in FOR loop initialization blocks.  Adding
 a plethora of new diagnostics to completely verify this new functionality.  These
 new diagnostics have not been run yet so they will fail in the next run of regression
 but should be ready to be checked out.

 Revision 1.77  2006/12/14 23:46:57  phase1geo
 Fixing remaining compile issues with support for functional unit pointers in
 expressions and unnamed scope handling.  Starting to debug run-time issues now.
 Added atask1 diagnostic to begin this verification process.  Checkpointing.

 Revision 1.76  2006/12/11 23:29:16  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.

 Revision 1.75  2006/11/25 04:24:39  phase1geo
 Adding initial code to fully support the timescale directive and its usage.
 Added -vpi_ts score option to allow the user to specify a top-level timescale
 value for the generated VPI file (this code has not been tested at this point,
 however).  Also updated diagnostic Makefile to get the VPI shared object files
 from the current lib directory (instead of the checked in one).

 Revision 1.74  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.73  2006/11/03 23:36:36  phase1geo
 Fixing bug 1590104.  Updating regressions per this change.

 Revision 1.72  2006/10/12 22:48:46  phase1geo
 Updates to remove compiler warnings.  Still some work left to go here.

 Revision 1.71  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.70  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.69  2006/09/08 22:39:50  phase1geo
 Fixes for memory problems.

 Revision 1.68  2006/09/07 21:59:24  phase1geo
 Fixing some bugs related to statement block removal.  Also made some significant
 optimizations to this code.

 Revision 1.67  2006/09/05 21:00:44  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.66  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

 Revision 1.65  2006/08/29 02:51:33  phase1geo
 Adding enumeration parsing support to parser.  No functionality at this point, however.

 Revision 1.64  2006/08/11 15:16:48  phase1geo
 Joining slist3.3 diagnostic to latest development branch.  Adding changes to
 fix memory issues from bug 1535412.

 Revision 1.63  2006/07/29 20:53:43  phase1geo
 Fixing some code related to generate statements; however, generate8.1 is still
 not completely working at this point.  Full regression passes for IV.

 Revision 1.62  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.61  2006/07/20 20:11:08  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.60  2006/07/19 22:30:46  phase1geo
 More work done for generate support.  Still have a ways to go.

 Revision 1.59  2006/07/18 21:52:49  phase1geo
 More work on generate blocks.  Currently working on assembling generate item
 statements in the parser.  Still a lot of work to go here.

 Revision 1.58  2006/06/27 19:34:42  phase1geo
 Permanent fix for the CDD save feature.

 Revision 1.57  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.56  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.55  2006/02/02 22:37:40  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.54  2006/02/01 15:13:10  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.53  2006/01/26 22:40:13  phase1geo
 Fixing last LXT bug.

 Revision 1.52  2006/01/23 03:53:29  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.51  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.50  2006/01/12 22:53:01  phase1geo
 Adding support for localparam construct.  Added tests to regression suite to
 verify correct functionality.  Full regression passes.

 Revision 1.49  2006/01/12 22:14:45  phase1geo
 Completed code for handling parameter value pass by name Verilog-2001 syntax.
 Added diagnostics to regression suite and updated regression files for this
 change.  Full regression now passes.

 Revision 1.48  2006/01/10 05:56:36  phase1geo
 In the middle of adding support for event sensitivity lists to score command.
 Regressions should pass but this code is not complete at this time.

 Revision 1.47  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.46  2005/12/08 19:47:00  phase1geo
 Fixed repeat2 simulation issues.  Fixed statement_connect algorithm, removed the
 need for a separate set_stop function and reshuffled the positions of esuppl bits.
 Full regression passes.

 Revision 1.45  2005/12/07 20:23:38  phase1geo
 Fixing case where statement is unconnectable.  Full regression now passes.

 Revision 1.44  2005/12/05 20:26:55  phase1geo
 Fixing bugs in code to remove statement blocks that are pointed to by expressions
 in NB_CALL and FORK cases.  Fixed bugs in fork code -- this is now working at the
 moment.  Updated regressions which now fully pass.

 Revision 1.43  2005/12/02 19:58:36  phase1geo
 Added initial support for FORK/JOIN expressions.  Code is not working correctly
 yet as we need to determine if a statement should be done in parallel or not.

 Revision 1.42  2005/12/02 12:03:17  phase1geo
 Adding support for excluding functions, tasks and named blocks.  Added tests
 to regression suite to verify this support.  Full regression passes.

 Revision 1.41  2005/11/29 19:04:47  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.40  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.39  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.38  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.37  2004/12/18 16:23:17  phase1geo
 More race condition checking updates.

 Revision 1.36  2004/04/19 04:54:55  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.35  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.34  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.33  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.32  2003/11/26 23:14:41  phase1geo
 Adding code to include left-hand-side expressions of statements for report
 outputting purposes.  Full regression does not yet pass.

 Revision 1.31  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.30  2003/08/09 22:10:41  phase1geo
 Removing wait event signals from CDD file generation in support of another method
 that fixes a bug when multiple wait event statements exist within the same
 statement tree.

 Revision 1.29  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.28  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.27  2003/01/25 22:39:02  phase1geo
 Fixing case where statement is found to be unsupported in middle of statement
 tree.  The entire statement tree is removed from consideration for simulation.

 Revision 1.26  2002/12/11 14:51:57  phase1geo
 Fixes compiler errors from last checkin.

 Revision 1.25  2002/12/03 06:01:16  phase1geo
 Fixing bug where delay statement is the last statement in a statement list.
 Adding diagnostics to verify this functionality.

 Revision 1.24  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.23  2002/11/02 16:16:20  phase1geo
 Cleaned up all compiler warnings in source and header files.

 Revision 1.22  2002/10/31 23:13:30  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.21  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.20  2002/10/29 13:33:21  phase1geo
 Adding patches for 64-bit compatibility.  Reformatted parser.y for easier
 viewing (removed tabs).  Full regression passes.

 Revision 1.19  2002/10/23 03:39:06  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.17  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.16  2002/09/23 01:37:44  phase1geo
 Need to make some changes to the inst_parm structure and some associated
 functionality for efficiency purposes.  This checkin contains most of the
 changes to the parser (with the exception of signal sizing).

 Revision 1.15  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.14  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.13  2002/08/26 12:57:03  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.12  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.11  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.

 Revision 1.10  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.9  2002/07/03 21:30:52  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.8  2002/07/01 15:10:42  phase1geo
 Fixing always loopbacks and setting stop bits correctly.  All verilog diagnostics
 seem to be passing with these fixes.

 Revision 1.7  2002/06/27 12:36:47  phase1geo
 Fixing bugs with scoring.  I think I got it this time.

 Revision 1.6  2002/06/24 12:34:56  phase1geo
 Fixing the set of the STMT_HEAD and STMT_STOP bits.  We are getting close.

 Revision 1.5  2002/06/24 04:54:48  phase1geo
 More fixes and code additions to make statements work properly.  Still not
 there at this point.

 Revision 1.4  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.3  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.

 Revision 1.2  2002/04/30 05:04:25  phase1geo
 Added initial go-round of adding statement handling to parser.  Added simple
 Verilog test to check correct statement handling.  At this point there is a
 bug in the expression write function (we need to display statement trees in
 the proper order since they are unlike normal expression trees.)
*/

#endif

