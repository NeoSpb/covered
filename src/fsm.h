#ifndef __FSM_H__
#define __FSM_H__

/*!
 \file     fsm.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     3/31/2002
 \brief    Contains functions for determining/reporting FSM coverage.
*/

#include <stdio.h>

#include "defines.h"

/*! \brief Creates and initializes new FSM structure. */
fsm* fsm_create( expression* from_state, expression* to_state, bool make_table );

/*! \brief Adds new FSM arc structure to specified FSMs arc list. */
void fsm_add_arc( fsm* table, expression* from_state, expression* to_state );

/*! \brief Sets sizes of tables in specified FSM structure. */
void fsm_create_tables( fsm* table );

/*! \brief Outputs contents of specified FSM to CDD file. */
bool fsm_db_write( fsm* table, FILE* file );

/*! \brief Reads in contents of specified FSM. */
bool fsm_db_read( char** line, module* mod );

/*! \brief Reads and merges two FSMs, placing result into base FSM. */
bool fsm_db_merge( fsm* base, char** line, bool same );

/*! \brief Sets the bit in set table based on the values of last and curr. */
void fsm_table_set( fsm* table );

/*! \brief Gathers statistics about the current FSM */
void fsm_get_stats( fsm_link* table, float* state_total, int* state_hit, float* arc_total, int* arc_hit );

/*! \brief Generates report output for FSM coverage. */
void fsm_report( FILE* ofile, bool verbose );

/*! \brief Deallocates specified FSM structure. */
void fsm_dealloc( fsm* table );

/*
 $Log$
 Revision 1.10  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.9  2003/09/22 03:46:24  phase1geo
 Adding support for single state variable FSMs.  Allow two different ways to
 specify FSMs on command-line.  Added diagnostics to verify new functionality.

 Revision 1.8  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.7  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.6  2002/11/05 00:20:07  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.5  2002/10/31 23:13:50  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.4  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.3  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.2  2002/07/03 03:31:11  phase1geo
 Adding RCS Log strings in files that were missing them so that file version
 information is contained in every source and header file.  Reordering src
 Makefile to be alphabetical.  Adding mult1.v diagnostic to regression suite.
*/

#endif

