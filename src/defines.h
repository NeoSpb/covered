#ifndef __DEFINES_H__
#define __DEFINES_H__

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
 \file     defines.h
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     11/27/2001
 \brief    Contains definitions/structures used in the Covered utility.
 
 \par
 This file is included by all files within Covered.  All defines, macros, structures, enums,
 and typedefs in the tool are specified in this file.
*/

#include "../config.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cexcept.h"

/*!
 Specifies current version of the Covered utility.
*/
#define COVERED_VERSION    VERSION

/*!
 Contains the CDD version number of all CDD files that this version of Covered can write
 and read.
*/
#define CDD_VERSION        14

/*!
 This contains the header information specified when executing this tool.
*/
#define COVERED_HEADER     "\nCovered %s -- Verilog Code Coverage Utility\nWritten by Trevor Williams  (phase1geo@gmail.com)\nFreely distributable under the GPL license\n", COVERED_VERSION

/*!
 Default database filename if not specified on command-line.
*/
#define DFLT_OUTPUT_CDD    "cov.cdd"

/*!
 Default profiling output filename.
*/
#define PROFILING_OUTPUT_NAME "covered.prof"

/*!
 Default filename that will contain the code necessary to attach Covered as a VPI to the Verilog
 simulator.
*/
#define DFLT_VPI_NAME      "covered_vpi.v"

/*!
 Determine size of integer in bits.
*/
#define INTEGER_WIDTH	   (SIZEOF_INT * 8)

/*!
 Specifies the maximum number of bits that a vector can hold.
*/
#define MAX_BIT_WIDTH      65536

/*!
 Specifies the maximum number of bytes that can be allocated via the safe allocation functions
 in util.c.
*/
#define MAX_MALLOC_SIZE    (MAX_BIT_WIDTH * 2)

/*!
 Length of user_msg global string (used for inputs to snprintf calls).
*/
#define USER_MSG_LENGTH    (MAX_BIT_WIDTH * 2)

/*!
 If -w option is specified to report command, specifies number of characters of width
 we will output.
*/
#define DEFAULT_LINE_WIDTH 105

/*!
 \addtogroup generations Supported Generations

 The following defines are used by Covered to specify the supported Verilog generations

 @{
*/

/*! Verilog-1995 */
#define GENERATION_1995		0

/*! Verilog-2001 */
#define GENERATION_2001 	1

/*! SystemVerilog */
#define GENERATION_SV		2

/*! @} */

/*!
 \addtogroup dumpfile_fmt Dumpfile Format

 The following defines are used by Covered to determine what dumpfile parsing mode we are in

 @{
*/

/*! No dumpfile was specified */
#define DUMP_FMT_NONE      0

/*! VCD dumpfile was specified */
#define DUMP_FMT_VCD       1

/*! LXT dumpfile was specified */
#define DUMP_FMT_LXT       2

/*! @} */

/*!
 \addtogroup output_type Output type

 The following defines are used by the print_output function to
 determine what type of output to generate and whether the output
 should or should not be suppressed.

 @{
*/

/*!
 Indicates that the specified error is fatal and will cause the
 program to immediately stop.  Cannot be suppressed.
*/
#define FATAL        1

/*!
 Indicates that the specified error is fatal but that the ERROR prefix
 to the error message should not be output.  Used for an error that needs
 multiple lines of output.
*/
#define FATAL_WRAP   2

/*!
 Indicate that the specified error is only a warning that something
 may be wrong but will not cause the program to immediately terminate.
 Suppressed if output_suppressed boolean variable in util.c is set to TRUE.
*/
#define WARNING      3

/*!
 Indicates that the specified error is only a warning but that the
 WARNING prefix should not be output.  Used for warning messages that
 need multiple lines of output.
*/
#define WARNING_WRAP 4

/*!
 Indicate that the specified message is not an error but some type of
 necessary output.  Suppressed if output_suppressed boolean variable in
 util.c is set to TRUE.
*/
#define NORMAL       5

/*!
 Indicates that the specified message is debug information that should
 only be displayed to the screen when the -D flag is specified.
*/
#define DEBUG        6

/*! @} */

/*!
 \addtogroup db_types Database line types

 The following defines are used in the coverage database to indicate
 the type for parsing purposes.

 @{
*/

/*!
 Specifies that the current coverage database line describes a signal.
*/
#define DB_TYPE_SIGNAL       1

/*!
 Specifies that the current coverage database line describes an expression.
*/
#define DB_TYPE_EXPRESSION   2

/*!
 Specifies that the current coverage database line describes a module.
*/
#define DB_TYPE_FUNIT        3

/*!
 Specifies that the current coverage database line describes a statement.
*/
#define DB_TYPE_STATEMENT    4

/*!
 Specifies that the current coverage database line describes general information.
*/
#define DB_TYPE_INFO         5

/*!
 Specifies that the current coverage database line describes a finite state machine.
*/
#define DB_TYPE_FSM          6

/*!
 Specifies that the current coverage database line describes a race condition block.
*/
#define DB_TYPE_RACE         7

/*!
 Specifies the arguments specified to the score command.
*/
#define DB_TYPE_SCORE_ARGS   8

/*!
 Specifies that the current coverage database line describes a new struct/union
 (all signals and struct/unions below this are members of this struct/union)
*/
#define DB_TYPE_SU_START     9

/*!
 Specifies that the current coverage database line ends the currently populated struct/union.
*/
#define DB_TYPE_SU_END       10

/*! @} */

/*!
 \addtogroup func_unit_types Functional Unit Types

 The following defines specify the type of functional unit being represented in the \ref func_unit
 structure.  A functional unit is defined to be any Verilog structure that contains scope.
 
 @{
*/

/*! Represents a Verilog module (syntax "module <name> ... endmodule") */
#define FUNIT_MODULE         0

/*! Represents a Verilog named block (syntax "begin : <name> ... end") */
#define FUNIT_NAMED_BLOCK    1

/*! Represents a Verilog function (syntax "function <name> ... endfunction") */
#define FUNIT_FUNCTION       2

/*! Represents a Verilog task (syntax "task <name> ... endtask") */
#define FUNIT_TASK           3

/*! Represents a scope that is considered a "no score" functional unit */
#define FUNIT_NO_SCORE       4

/*! Represents a re-entrant Verilog function (syntax "function automatic <name> ... endfunction") */
#define FUNIT_AFUNCTION      5

/*! Represents a re-entrant Verilog task (syntax "task <name> ... endtask") */
#define FUNIT_ATASK          6

/*! Represents a named block inside of a re-entrant Verilog task or function */
#define FUNIT_ANAMED_BLOCK   7

/*! The number of valid functional unit types */
#define FUNIT_TYPES          8

/*! @} */

/*!
 \addtogroup report_detail Detailedness of reports.

 The following defines specify the amount of detail to include in a generated report.

 @{
*/

/*!
 Specifies that only summary information should be included in report.
*/
#define REPORT_SUMMARY       0x0

/*!
 Includes summary information as well as verbose output for line and toggle coverage.
 Provides high-level view of combinational logic holes.  This information is useful
 for understanding where uncovered logic exists but not necessarily why it was not
 covered.
*/
#define REPORT_DETAILED      0x2

/*!
 Includes summary information and all verbose information for line, toggle and
 combinational logic.  Shows why combinational logic was not considered covered by
 exposing all missed expressions in each expression tree.  This information is most
 useful in debugging but may be a bit to much to easily discern where logic is not
 covered.
*/
#define REPORT_VERBOSE       0xffffffff

/*! @} */

/*!
 Used for merging two vector nibbles from two vectors.  Both vector nibble
 fields are ANDed with this mask and ORed together to perform the merge.  
 Fields that are merged are:
 - TOGGLE0->1
 - TOGGLE1->0
 - FALSE
 - TRUE
*/
#define VECTOR_MERGE_MASK    0x6c

/*!
 \addtogroup expr_suppl Expression supplemental field defines and macros.

 @{
*/

/*!
 Used for merging two supplemental fields from two expressions.  Both expression
 supplemental fields are ANDed with this mask and ORed together to perform the
 merge.  See esuppl_u for information on which bits are masked.
*/
#define ESUPPL_MERGE_MASK            0xfffff

/*!
 Specifies the number of bits to store for a given expression for reentrant purposes.
 Contains the following expression supplemental field bits:
 -# left_changed
 -# right_changed
 -# eval_t
 -# eval_f
 -# prev_called
*/
#define ESUPPL_BITS_TO_STORE         5

/*!
 Returns a value of 1 if the specified supplemental value has the SWAPPED
 bit set indicating that the children of the current expression were
 swapped positions during the scoring phase.
*/
#define ESUPPL_WAS_SWAPPED(x)        x.part.swapped

/*!
 Returns a value of 1 if the specified supplemental value has the ROOT bit
 set indicating that the current expression is the root expression of its
 expression tree.
*/
#define ESUPPL_IS_ROOT(x)            x.part.root

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 who was just evaluated to TRUE.
*/
#define ESUPPL_IS_TRUE(x)            x.part.eval_t

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 who was just evaluated to FALSE.
*/
#define ESUPPL_IS_FALSE(x)           x.part.eval_f

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 that has evaluated to a value of TRUE (1) during simulation.
*/
#define ESUPPL_WAS_TRUE(x)           x.part.true

/*!
 Returns a value of 1 if the specified supplemental belongs to an expression
 that has evaluated to a value of FALSE (0) during simulation.
*/
#define ESUPPL_WAS_FALSE(x)          x.part.false

/*!
 Returns a value of 1 if the left child expression was changed during this
 timestamp.
*/
#define ESUPPL_IS_LEFT_CHANGED(x)    x.part.left_changed

/*!
 Returns a value of 1 if the right child expression was changed during this
 timestamp.
*/
#define ESUPPL_IS_RIGHT_CHANGED(x)   x.part.right_changed

/*!
 Returns a value of 1 if the specified expression has already been counted
 for combinational coverage.
*/
#define ESUPPL_WAS_COMB_COUNTED(x)   x.part.comb_cntd

/*!
 Returns a value of 1 if the specified expression exists on the left-hand side
 of an assignment operation.
*/
#define ESUPPL_IS_LHS(x)             x.part.lhs

/*!
 Returns a value of 1 if the specified expression is contained in a function.
*/
#define ESUPPL_IS_IN_FUNC(x)         x.part.in_func

/*!
 Returns a value of 1 if the specified expression owns its vector structure or 0 if it
 shares it with someone else.
*/
#define ESUPPL_OWNS_VEC(x)           x.part.owns_vec

/*!
 Returns a value of 1 if the specified expression should be excluded from coverage
 consideration.
*/
#define ESUPPL_EXCLUDED(x)           x.part.excluded

/*!
 Returns the type of expression that this expression should be treated as.  Legal values
 are specified \ref expression_types.
*/
#define ESUPPL_TYPE(x)               x.part.type

/*!
 Returns the base type of the vector value if the expression is an EXP_OP_STATIC.  Legal
 values are DECIMAL, HEXIDECIMAL, OCTAL, BINARY, and QSTRING.
*/
#define ESUPPL_STATIC_BASE(x)        x.part.base

/*! @} */

/*!
 \addtogroup ssuppl_type Signal Supplemental Field Types

 The following defines represent the various types of signals that can be parsed.

 @{
*/

/*! This is an input port net signal */
#define SSUPPL_TYPE_INPUT_NET     0

/*! This is an input port registered signal */
#define SSUPPL_TYPE_INPUT_REG     1

/*! This is an output port net signal */
#define SSUPPL_TYPE_OUTPUT_NET    2

/*! This is an output port registered signal */
#define SSUPPL_TYPE_OUTPUT_REG    3

/*! This is an inout port net signal */
#define SSUPPL_TYPE_INOUT_NET     4

/*! This is an inout port registered signal */
#define SSUPPL_TYPE_INOUT_REG     5

/*! This is a declared net signal (i.e., not a port) */
#define SSUPPL_TYPE_DECL_NET      6

/*! This is a declared registered signal (i.e., not a port) */
#define SSUPPL_TYPE_DECL_REG      7

/*! This is an event */
#define SSUPPL_TYPE_EVENT         8

/*! This signal was implicitly created */
#define SSUPPL_TYPE_IMPLICIT      9

/*! This signal was implicitly created (this signal was created from a positive variable multi-bit expression) */
#define SSUPPL_TYPE_IMPLICIT_POS  10

/*! This signal was implicitly created (this signal was created from a negative variable multi-bit expression) */
#define SSUPPL_TYPE_IMPLICIT_NEG  11

/*! This signal is a parameter */
#define SSUPPL_TYPE_PARAM         12

/*! This signal is a genvar */
#define SSUPPL_TYPE_GENVAR        13

/*! This signal is an enumerated value */
#define SSUPPL_TYPE_ENUM          14

/*! This signal is a memory */
#define SSUPPL_TYPE_MEM           15

/*! @} */

/*!
 Returns TRUE if the given vsignal is a net type.
*/
#define SIGNAL_IS_NET(x)          ((x->suppl.part.type == SSUPPL_TYPE_INPUT_NET)    || \
                                   (x->suppl.part.type == SSUPPL_TYPE_OUTPUT_NET)   || \
                                   (x->suppl.part.type == SSUPPL_TYPE_INOUT_NET)    || \
                                   (x->suppl.part.type == SSUPPL_TYPE_EVENT)        || \
                                   (x->suppl.part.type == SSUPPL_TYPE_DECL_NET)     || \
                                   (x->suppl.part.type == SSUPPL_TYPE_IMPLICIT)     || \
                                   (x->suppl.part.type == SSUPPL_TYPE_IMPLICIT_POS) || \
                                   (x->suppl.part.type == SSUPPL_TYPE_IMPLICIT_NEG))
     
/*!
 \addtogroup read_modes Modes for reading database file

 Specify how to integrate read data from database file into memory structures.

 @{
*/

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when reading after parsing source files.
*/
#define READ_MODE_NO_MERGE                0

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when performing a MERGE command on first file.
*/
#define READ_MODE_MERGE_NO_MERGE          1

/*!
 When new functional unit is read from database file, it is placed in the functional
 unit list and is placed in the correct hierarchical position in the instance tree.
 Used when performing a REPORT command.
*/
#define READ_MODE_REPORT_NO_MERGE         2

/*!
 When functional unit is completely read in (including module, signals, expressions), the
 functional unit is looked up in the current instance tree.  If the instance exists, the
 functional unit is merged with the instance's functional unit; otherwise, we are attempting to
 merge two databases that were created from different designs.
*/
#define READ_MODE_MERGE_INST_MERGE        3

/*!
 When functional unit is completely read in (including module, signals, expressions), the
 functional unit is looked up in the functional unit list.  If the functional unit is found,
 it is merged with the existing functional unit; otherwise, it is added to the functional
 unit list.
*/
#define READ_MODE_REPORT_MOD_MERGE        4

/*! @} */

/*!
 \addtogroup param_suppl_types Instance/module parameter supplemental type definitions.

 @{
*/

/*!
 Specifies that the current module parameter is a declared type (belongs to current 
 module).
*/
#define PARAM_TYPE_DECLARED             0

/*!
 Specifies that the current module parameter is an override type (overrides the value
 of an instantiated modules parameter declaration).
*/
#define PARAM_TYPE_OVERRIDE             1

/*!
 Specifies that the current module parameter specifies the value for a signal LSB
 value.
*/
#define PARAM_TYPE_SIG_LSB              2

/*!
 Specifies that the current module parameter specifies the value for a signal MSB
 value.
*/
#define PARAM_TYPE_SIG_MSB              3

/*!
 Specifies that the current module parameter specifies the value for an instance
 instantiation LSB value.
*/
#define PARAM_TYPE_INST_LSB             4

/*!
 Specifies that the current module parameter specifies the value for an instance
 instantiation MSB value.
*/
#define PARAM_TYPE_INST_MSB             5

/*!
 Specifies that the current module parameter is a declared type (belongs to current
 module) and is local (cannot be overridden by defparams and inline parameter overrides).
*/
#define PARAM_TYPE_DECLARED_LOCAL       6

/*! @} */

/*!
 \addtogroup generate_item_types Generate Block Item Types

 The following defines specify the different types of elements that can be stored in
 a generate item structure.

 @{
*/

/*! Holds an expression that is used in either a generate FOR, IF or CASE statement */
#define GI_TYPE_EXPR            0

/*! Holds a signal instantiation */
#define GI_TYPE_SIG             1

/*! Holds a statement block (initial, assign, always) */
#define GI_TYPE_STMT            2

/*! Holds an module instantiation */
#define GI_TYPE_INST            3

/*! Holds a task, function, named begin/end block */
#define GI_TYPE_TFN             4

/*! Holds information for a signal/expression bind */
#define GI_TYPE_BIND            5

/*! @} */

/*!
 \addtogroup delay_expr_types  Delay expression types.
 
 The following defines are used select a delay expression type in the form min:typ:max
 when encountered during parsing.
 
 @{
*/

/*!
 Default value of delay expression selector to indicate that the user has not
 specified a value to use.
*/
#define DELAY_EXPR_DEFAULT      0

/*!
 Causes parser to always use min value in min:typ:max delay expressions.
*/
#define DELAY_EXPR_MIN          1

/*!
 Causes parser to always use typ value in min:typ:max delay expressions.
*/
#define DELAY_EXPR_TYP          2

/*!
 Causes parser to always use max value in min:typ:max delay expressions.
*/
#define DELAY_EXPR_MAX          3

/*! @} */

/*!
 Enumeration of the various expression operations that Covered supports.
*/
typedef enum exp_op_type_e {
  EXP_OP_STATIC = 0,      /*!<   0:0x00.  Specifies constant value. */
  EXP_OP_SIG,             /*!<   1:0x01.  Specifies signal value. */
  EXP_OP_XOR,             /*!<   2:0x02.  Specifies '^' operator. */
  EXP_OP_MULTIPLY,        /*!<   3:0x03.  Specifies '*' operator. */
  EXP_OP_DIVIDE,          /*!<   4:0x04.  Specifies '/' operator. */
  EXP_OP_MOD,             /*!<   5:0x05.  Specifies '%' operator. */
  EXP_OP_ADD,             /*!<   6:0x06.  Specifies '+' operator. */
  EXP_OP_SUBTRACT,        /*!<   7:0x07.  Specifies '-' operator. */
  EXP_OP_AND,             /*!<   8:0x08.  Specifies '&' operator. */
  EXP_OP_OR,              /*!<   9:0x09.  Specifies '|' operator. */
  EXP_OP_NAND,            /*!<  10:0x0a.  Specifies '~&' operator. */
  EXP_OP_NOR,             /*!<  11:0x0b.  Specifies '~|' operator. */
  EXP_OP_NXOR,            /*!<  12:0x0c.  Specifies '~^' operator. */
  EXP_OP_LT,              /*!<  13:0x0d.  Specifies '<' operator. */
  EXP_OP_GT,              /*!<  14:0x0e.  Specifies '>' operator. */
  EXP_OP_LSHIFT,          /*!<  15:0x0f.  Specifies '<<' operator. */
  EXP_OP_RSHIFT,          /*!<  16:0x10.  Specifies '>>' operator. */
  EXP_OP_EQ,              /*!<  17:0x11.  Specifies '==' operator. */
  EXP_OP_CEQ,             /*!<  18:0x12.  Specifies '===' operator. */
  EXP_OP_LE,              /*!<  19:0x13.  Specifies '<=' operator. */
  EXP_OP_GE,              /*!<  20:0x14.  Specifies '>=' operator. */
  EXP_OP_NE,              /*!<  21:0x15.  Specifies '!=' operator. */
  EXP_OP_CNE,             /*!<  22:0x16.  Specifies '!==' operator. */
  EXP_OP_LOR,             /*!<  23:0x17.  Specifies '||' operator. */
  EXP_OP_LAND,            /*!<  24:0x18.  Specifies '&&' operator. */
  EXP_OP_COND,            /*!<  25:0x19.  Specifies '?:' conditional operator. */
  EXP_OP_COND_SEL,        /*!<  26:0x1a.  Specifies '?:' conditional select. */
  EXP_OP_UINV,            /*!<  27:0x1b.  Specifies '~' unary operator. */
  EXP_OP_UAND,            /*!<  28:0x1c.  Specifies '&' unary operator. */
  EXP_OP_UNOT,            /*!<  29:0x1d.  Specifies '!' unary operator. */
  EXP_OP_UOR,             /*!<  30:0x1e.  Specifies '|' unary operator. */
  EXP_OP_UXOR,            /*!<  31:0x1f.  Specifies '^' unary operator. */
  EXP_OP_UNAND,           /*!<  32:0x20.  Specifies '~&' unary operator. */
  EXP_OP_UNOR,            /*!<  33:0x21.  Specifies '~|' unary operator. */
  EXP_OP_UNXOR,           /*!<  34:0x22.  Specifies '~^' unary operator. */
  EXP_OP_SBIT_SEL,        /*!<  35:0x23.  Specifies single-bit signal select (i.e., [x]). */
  EXP_OP_MBIT_SEL,        /*!<  36:0x24.  Specifies multi-bit signal select (i.e., [x:y]). */
  EXP_OP_EXPAND,          /*!<  37:0x25.  Specifies bit expansion operator (i.e., {x{y}}). */
  EXP_OP_CONCAT,          /*!<  38:0x26.  Specifies signal concatenation operator (i.e., {x,y}). */
  EXP_OP_PEDGE,           /*!<  39:0x27.  Specifies posedge operator (i.e., \@posedge x). */
  EXP_OP_NEDGE,           /*!<  40:0x28.  Specifies negedge operator (i.e., \@negedge x). */
  EXP_OP_AEDGE,           /*!<  41:0x29.  Specifies anyedge operator (i.e., \@x). */
  EXP_OP_LAST,            /*!<  42:0x2a.  Specifies 1-bit holder for parent. */
  EXP_OP_EOR,             /*!<  43:0x2b.  Specifies 'or' event operator. */
  EXP_OP_DELAY,           /*!<  44:0x2c.  Specifies delay operator (i.e., #(x)). */
  EXP_OP_CASE,            /*!<  45:0x2d.  Specifies case equality expression. */
  EXP_OP_CASEX,           /*!<  46:0x2e.  Specifies casex equality expression. */
  EXP_OP_CASEZ,           /*!<  47:0x2f.  Specifies casez equality expression. */
  EXP_OP_DEFAULT,         /*!<  48:0x30.  Specifies case/casex/casez default expression. */
  EXP_OP_LIST,            /*!<  49:0x31.  Specifies comma separated expression list. */
  EXP_OP_PARAM,           /*!<  50:0x32.  Specifies full parameter. */
  EXP_OP_PARAM_SBIT,      /*!<  51:0x33.  Specifies single-bit select parameter. */
  EXP_OP_PARAM_MBIT,      /*!<  52:0x34.  Specifies multi-bit select parameter. */
  EXP_OP_ASSIGN,          /*!<  53:0x35.  Specifies an assign assignment operator. */
  EXP_OP_DASSIGN,         /*!<  54:0x36.  Specifies a wire declaration assignment operator. */
  EXP_OP_BASSIGN,         /*!<  55:0x37.  Specifies a blocking assignment operator. */
  EXP_OP_NASSIGN,         /*!<  56:0x38.  Specifies a non-blocking assignment operator. */
  EXP_OP_IF,              /*!<  57:0x39.  Specifies an if statement operator. */
  EXP_OP_FUNC_CALL,       /*!<  58:0x3a.  Specifies a function call. */
  EXP_OP_TASK_CALL,       /*!<  59:0x3b.  Specifies a task call (note: this operator MUST be the root of the expression tree) */
  EXP_OP_TRIGGER,         /*!<  60:0x3c.  Specifies an event trigger (->). */
  EXP_OP_NB_CALL,         /*!<  61:0x3d.  Specifies a "call" to a named block */
  EXP_OP_FORK,            /*!<  62:0x3e.  Specifies a fork command */
  EXP_OP_JOIN,            /*!<  63:0x3f.  Specifies a join command */
  EXP_OP_DISABLE,         /*!<  64:0x40.  Specifies a disable command */
  EXP_OP_REPEAT,          /*!<  65:0x41.  Specifies a repeat loop test expression */
  EXP_OP_WHILE,           /*!<  66:0x42.  Specifies a while loop test expression */
  EXP_OP_ALSHIFT,         /*!<  67:0x43.  Specifies arithmetic left shift (<<<) */
  EXP_OP_ARSHIFT,         /*!<  68:0x44.  Specifies arithmetic right shift (>>>) */
  EXP_OP_SLIST,           /*!<  69:0x45.  Specifies sensitivity list (*) */
  EXP_OP_EXPONENT,        /*!<  70:0x46.  Specifies the exponential operator "**" */
  EXP_OP_PASSIGN,         /*!<  71:0x47.  Specifies a port assignment */
  EXP_OP_RASSIGN,         /*!<  72:0x48.  Specifies register assignment (reg a = 1'b0) */
  EXP_OP_MBIT_POS,        /*!<  73:0x49.  Specifies positive variable multi-bit select (a[b+:3]) */
  EXP_OP_MBIT_NEG,        /*!<  74:0x4a.  Specifies negative variable multi-bit select (a[b-:3]) */
  EXP_OP_PARAM_MBIT_POS,  /*!<  75:0x4b.  Specifies positive variable multi-bit parameter select */
  EXP_OP_PARAM_MBIT_NEG,  /*!<  76:0x4c.  Specifies negative variable multi-bit parameter select */
  EXP_OP_NEGATE,          /*!<  77:0x4d.  Specifies the unary negate operator (-) */
  EXP_OP_NOOP,            /*!<  78:0x4e.  Specifies no operation is to be performed (placeholder) */
  EXP_OP_ALWAYS_COMB,     /*!<  79:0x4f.  Specifies an always_comb statement (implicit event expression - similar to SLIST) */
  EXP_OP_ALWAYS_LATCH,    /*!<  80:0x50.  Specifies an always_latch statement (implicit event expression - similar to SLIST) */
  EXP_OP_IINC,            /*!<  81:0x51.  Specifies the immediate increment SystemVerilog operator (++a) */
  EXP_OP_PINC,            /*!<  82:0x52.  Specifies the postponed increment SystemVerilog operator (a++) */
  EXP_OP_IDEC,            /*!<  83:0x53.  Specifies the immediate decrement SystemVerilog operator (--a) */
  EXP_OP_PDEC,            /*!<  84:0x54.  Specifies the postponed decrement SystemVerilog operator (a--) */
  EXP_OP_DLY_ASSIGN,      /*!<  85:0x55.  Specifies a delayed assignment (i.e., a = #5 b; or a = @(c) b;) */
  EXP_OP_DLY_OP,          /*!<  86:0x56.  Child expression of DLY_ASSIGN, points to the delay expr and the op expr */
  EXP_OP_RPT_DLY,         /*!<  87:0x57.  Child expression of DLY_OP, points to the delay expr and the repeat expr */
  EXP_OP_DIM,             /*!<  88:0x58.  Specifies a selection dimension (right expression points to a selection expr) */
  EXP_OP_WAIT,            /*!<  89:0x59.  Specifies a wait statement */
  EXP_OP_SFINISH,         /*!<  90:0x5a.  Specifies a $finish call */
  EXP_OP_SSTOP,           /*!<  91:0x5b.  Specifies a $stop call */
  EXP_OP_ADD_A,           /*!<  92:0x5c.  Specifies the '+=' operator */
  EXP_OP_SUB_A,           /*!<  93:0x5d.  Specifies the '-=' operator */
  EXP_OP_MLT_A,           /*!<  94:0x5e.  Specifies the '*=' operator */
  EXP_OP_DIV_A,           /*!<  95:0x5f.  Specifies the '/=' operator */
  EXP_OP_MOD_A,           /*!<  96:0x60.  Specifies the '%=' operator */
  EXP_OP_AND_A,           /*!<  97:0x61.  Specifies the '&=' operator */
  EXP_OP_OR_A,            /*!<  98:0x62.  Specifies the '|=' operator */
  EXP_OP_XOR_A,           /*!<  99:0x63.  Specifies the '^=' operator */
  EXP_OP_LS_A,            /*!< 100:0x64.  Specifies the '<<=' operator */
  EXP_OP_RS_A,            /*!< 101:0x65.  Specifies the '>>=' operator */
  EXP_OP_ALS_A,           /*!< 102:0x66.  Specifies the '<<<=' operator */
  EXP_OP_ARS_A,           /*!< 103:0x67.  Specifies the '>>>=' operator */
  EXP_OP_NUM              /*!< The total number of defines for expression values */
} exp_op_type;

/*!
 Returns a value of 1 if the specified expression is considered to be measurable.
*/
#define EXPR_IS_MEASURABLE(x)      (((exp_op_info[x->op].suppl.measurable == 1) && \
                                     (ESUPPL_IS_LHS( x->suppl ) == 0) && \
                                     !((ESUPPL_IS_ROOT( x->suppl ) == 0) && \
                                       ((x->op == EXP_OP_SIG) || \
                                        (x->op == EXP_OP_SBIT_SEL) || \
                                        (x->op == EXP_OP_MBIT_SEL) || \
                                        (x->op == EXP_OP_MBIT_POS) || \
                                        (x->op == EXP_OP_MBIT_NEG)) && \
                                       (x->parent->expr->op != EXP_OP_ASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_DASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_BASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_NASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_RASSIGN) && \
                                       (x->parent->expr->op != EXP_OP_DLY_OP) && \
                                       (x->parent->expr->op != EXP_OP_IF) && \
                                       (x->parent->expr->op != EXP_OP_WHILE) && \
                                       (x->parent->expr->op != EXP_OP_COND)) && \
                                     (x->line != 0)) ? 1 : 0)

/*!
 Returns a value of TRUE if the specified expression is a STATIC, PARAM, PARAM_SBIT, PARAM_MBIT,
 PARAM_MBIT_POS or PARAM_MBIT_NEG operation type.
*/
#define EXPR_IS_STATIC(x)        exp_op_info[x->op].suppl.is_static

/*!
 Returns a value of true if the specified expression is considered a combination expression by
 the combinational logic report generator.
*/
#define EXPR_IS_COMB(x)          ((exp_op_info[x->op].suppl.is_comb > 0) && \
                                  (EXPR_IS_OP_AND_ASSIGN(x) || \
                                  (!expression_is_static_only( x->left ) && \
                                   !expression_is_static_only( x->right))))

/*!
 Returns a value of true if the specified expression is considered to be an event type
 (i.e., it either occurred or did not occur -- WAS_FALSE is not important).
*/
#define EXPR_IS_EVENT(x)         exp_op_info[x->op].suppl.is_event

/*!
 These expressions will only allow a statement block to advance when they evaluate to a value of true (i.e.,
 their statement's false path is NULL).  They allow context switching to occur if they evaluate to false.
*/
#define EXPR_IS_CONTEXT_SWITCH(x)	((exp_op_info[x->op].suppl.is_context_switch == 1) || \
                                         ((x->op == EXP_OP_NB_CALL) && !ESUPPL_IS_IN_FUNC(x->suppl)))

/*!
 These expressions all use someone else's vectors instead of their own.
*/
#define EXPR_OWNS_VEC(o)                ((o != EXP_OP_SIG)            && \
                                         (o != EXP_OP_SBIT_SEL)       && \
                                         (o != EXP_OP_MBIT_SEL)       && \
                                         (o != EXP_OP_MBIT_POS)       && \
                                         (o != EXP_OP_MBIT_NEG)       && \
                                         (o != EXP_OP_TRIGGER)        && \
                                         (o != EXP_OP_PARAM)          && \
                                         (o != EXP_OP_PARAM_SBIT)     && \
                                         (o != EXP_OP_PARAM_MBIT)     && \
                                         (o != EXP_OP_PARAM_MBIT_POS) && \
                                         (o != EXP_OP_PARAM_MBIT_NEG) && \
                                         (o != EXP_OP_ASSIGN)         && \
                                         (o != EXP_OP_DASSIGN)        && \
                                         (o != EXP_OP_BASSIGN)        && \
                                         (o != EXP_OP_NASSIGN)        && \
                                         (o != EXP_OP_RASSIGN)        && \
                                         (o != EXP_OP_IF)             && \
                                         (o != EXP_OP_WHILE)          && \
					 (o != EXP_OP_PASSIGN)        && \
                                         (o != EXP_OP_DLY_ASSIGN)     && \
                                         (o != EXP_OP_DIM))

/*!
 Returns a value of true if the specified expression is considered a unary expression by
 the combinational logic report generator.
*/
#define EXPR_IS_UNARY(x)        exp_op_info[x->op].suppl.is_unary

/*!
 Returns a value of 1 if the specified expression was measurable for combinational 
 coverage but not fully covered during simulation.
*/
#define EXPR_COMB_MISSED(x)     (EXPR_IS_MEASURABLE( x ) && (x->ulid != -1))

/*!
 Returns a value of 1 if the specified expression's left child may be deallocated using the
 expression_dealloc function.  We can deallocate any left expression that doesn't belong to
 a case statement or has the owned bit set to 1.
*/
#define EXPR_LEFT_DEALLOCABLE(x)     (((x->op != EXP_OP_CASE)  && \
                                       (x->op != EXP_OP_CASEX) && \
                                       (x->op != EXP_OP_CASEZ)) || \
                                      (x->suppl.part.owned == 1))

/*!
 Specifies if the right expression can be deallocated (currently there is no reason why a
 right expression can't be deallocated.
*/
#define EXPR_RIGHT_DEALLOCABLE(x)    (TRUE)

/*!
 Specifies if the expression is an op-and-assign operation (i.e., +=, -=, &=, etc.)
*/
#define EXPR_IS_OP_AND_ASSIGN(x)     ((x->op == EXP_OP_ADD_A) || \
                                      (x->op == EXP_OP_SUB_A) || \
                                      (x->op == EXP_OP_MLT_A) || \
                                      (x->op == EXP_OP_DIV_A) || \
                                      (x->op == EXP_OP_MOD_A) || \
                                      (x->op == EXP_OP_AND_A) || \
                                      (x->op == EXP_OP_OR_A)  || \
                                      (x->op == EXP_OP_XOR_A) || \
                                      (x->op == EXP_OP_LS_A)  || \
                                      (x->op == EXP_OP_RS_A)  || \
                                      (x->op == EXP_OP_ALS_A) || \
                                      (x->op == EXP_OP_ARS_A))

/*!
 Specifies the number of temporary vectors required by the given expression operation.
*/
#define EXPR_TMP_VECS(x)             exp_op_info[x].suppl.tmp_vecs

/*!
 Returns TRUE if this expression should have its dim element populated.
*/
#define EXPR_OP_HAS_DIM(x)          ((x == EXP_OP_DIM)            || \
                                     (x == EXP_OP_SBIT_SEL)       || \
                                     (x == EXP_OP_PARAM_SBIT)     || \
                                     (x == EXP_OP_MBIT_SEL)       || \
                                     (x == EXP_OP_PARAM_MBIT)     || \
                                     (x == EXP_OP_MBIT_POS)       || \
                                     (x == EXP_OP_MBIT_NEG)       || \
                                     (x == EXP_OP_PARAM_MBIT_POS) || \
                                     (x == EXP_OP_PARAM_MBIT_NEG))
 

/*!
 \addtogroup op_tables

 The following describe the operation table values for AND, OR, XOR, NAND, NOR and
 NXOR operations.

 @{
*/

/*!
 Specifies the number of entries in an optab array.
*/
#define OPTAB_SIZE        17
 
/*                        00  01  02  03  10  11  12  13  20  21  22  23  30  31  32  33  NOT*/
#define AND_OP_TABLE      0,  0,  0,  0,  0,  1,  2,  2,  0,  2,  2,  2,  0,  2,  2,  2,  0
#define OR_OP_TABLE       0,  1,  2,  2,  1,  1,  1,  1,  2,  1,  2,  2,  2,  1,  2,  2,  0
#define XOR_OP_TABLE      0,  1,  2,  2,  1,  0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  0
#define NAND_OP_TABLE     1,  1,  1,  1,  1,  0,  2,  2,  1,  2,  2,  2,  1,  2,  2,  2,  1
#define NOR_OP_TABLE      1,  0,  2,  2,  0,  0,  0,  0,  2,  0,  2,  2,  2,  0,  2,  2,  1
#define NXOR_OP_TABLE     1,  0,  2,  2,  0,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  1

/*! @} */

/*!
 \addtogroup comparison_types

 The following are comparison types used in the vector_op_compare function in vector.c:

 @{
*/

#define COMP_LT         0       /*!< Less than */
#define COMP_GT         1       /*!< Greater than */
#define COMP_LE         2       /*!< Less than or equal to */
#define COMP_GE         3       /*!< Greater than or equal to */
#define COMP_EQ         4       /*!< Equal to */
#define COMP_NE         5       /*!< Not equal to */
#define COMP_CEQ        6       /*!< Case equality */
#define COMP_CNE        7       /*!< Case inequality */
#define COMP_CXEQ       8       /*!< Casex equality */
#define COMP_CZEQ       9       /*!< Casez equality */

/*! @} */

/*!
 \addtogroup lexer_val_types

 The following defines specify what type of vector value can be extracted from a value
 string.  These defines are used by vector.c and the lexer.
 
 @{
*/

#define DECIMAL			0	/*!< String in format [dD][0-9]+ */
#define BINARY			1	/*!< String in format [bB][01xXzZ_\?]+ */
#define OCTAL			2	/*!< String in format [oO][0-7xXzZ_\?]+ */
#define HEXIDECIMAL		3	/*!< String in format [hH][0-9a-fA-FxXzZ_\?]+ */
#define QSTRING                 4       /*!< Quoted string */

/*! @} */

/*!
 \addtogroup attribute_types Attribute Types

 The following defines specify the attribute types that are parseable by Covered.

 @{
*/

#define ATTRIBUTE_UNKNOWN       0       /*!< This attribute is not recognized by Covered */
#define ATTRIBUTE_FSM           1       /*!< FSM attribute */

/*! @} */

/*!
 \addtogroup race_condition_types Race Condition Violation Types

 The following group of defines specify all of the types of race conditions that Covered is
 currently capable of detecting:

 @{
*/

/*! All sequential logic uses non-blocking assignments */
#define RACE_TYPE_SEQ_USES_NON_BLOCK         0

/*! All combinational logic in an always block uses blocking assignments */
#define RACE_TYPE_CMB_USES_BLOCK             1

/*! All mixed sequential and combinational logic in the same always block uses non-blocking assignments */
#define RACE_TYPE_MIX_USES_NON_BLOCK         2

/*! Blocking and non-blocking assignments should not be used in the same always block */
#define RACE_TYPE_HOMOGENOUS                 3

/*! Assignments made to a variable should only be done within one always block */
#define RACE_TYPE_ASSIGN_IN_ONE_BLOCK1       4

/*! Signal assigned both in statement block and via input/inout port */
#define RACE_TYPE_ASSIGN_IN_ONE_BLOCK2       5

/*! The $strobe system call should only be used to display variables that were assigned using non-blocking assignments */
#define RACE_TYPE_STROBE_DISPLAY_NON_BLOCK   6

/*! No #0 procedural assignments should exist */
#define RACE_TYPE_NO_POUND_0_PROC_ASSIGNS    7

/*! Total number of race condition checks in this list */
#define RACE_TYPE_NUM                        8

/*! @} */

/*!
 \addtogroup comb_types Combinational Logic Output Types

 The following group of defines specify the combinational logic output types (these are stored in the is_comb
 variable in the exp_info structure.

 @{
*/

/*! Specifies that the expression is not a two variable combinational expression */
#define NOT_COMB	0

/*! Specifies that the expression is an AND type two variable combinational expression */
#define AND_COMB	1

/*! Specifies that the expression is an OR type two variable combinational expression */
#define OR_COMB		2

/*! Specifies that the expression is a two variable combinational expression that is neither an AND or OR type */
#define OTHER_COMB	3

/*! @} */

/*!
 \addtogroup vector_types Vector Types

 The following defines specify the various flavors of vector data that we can store.

 @{
*/

/*! Used for storing 2 or 4-state value-only values (no coverage information stored) */
#define VTYPE_VAL   0

/*! Used for storing 2 or 4-state signal values */
#define VTYPE_SIG   1

/*! Used for storing 2 or 4-state expression values */
#define VTYPE_EXP   2

/*! Used for storing 2 or 4-state memory information */
#define VTYPE_MEM   3

/*! @} */

/*!
 \addtogroup vector_type_sizes Vector Type Data Sizes

 The following defines specify that various types of data types that can be stored in a vector.  This
 value determines which value pointer is used in the vector structure for holding data.

 @{
*/

/*! unsigned long */
#define VDATA_UL      0

/*! @} */

/*!
 \addtogroup vector_type_indices Vector Type Information Indices
*/

/*! Provides value indices for VTYPE_VAL vectors */
typedef enum vtype_val_indices_e {
  VTYPE_INDEX_VAL_VALL,
  VTYPE_INDEX_VAL_VALH,
  VTYPE_INDEX_VAL_NUM
} vtype_val_indices;

/*! Provides value indices for VTYPE_SIG vectors */
typedef enum vtype_sig_indices_e {
  VTYPE_INDEX_SIG_VALL,
  VTYPE_INDEX_SIG_VALH,
  VTYPE_INDEX_SIG_TOG01,
  VTYPE_INDEX_SIG_TOG10,
  VTYPE_INDEX_SIG_MISC,
  VTYPE_INDEX_SIG_NUM
} vtype_sig_indices;

/*! Provides value indices for VTYPE_MEM vectors */
typedef enum vtype_mem_indices_e {
  VTYPE_INDEX_MEM_VALL,
  VTYPE_INDEX_MEM_VALH,
  VTYPE_INDEX_MEM_TOG01,
  VTYPE_INDEX_MEM_TOG10,
  VTYPE_INDEX_MEM_WR,
  VTYPE_INDEX_MEM_RD,
  VTYPE_INDEX_MEM_NUM
} vtype_mem_indices;

/*! Provides value indices for VTYPE_EXP vectors */
typedef enum vtype_exp_indices_e {
  VTYPE_INDEX_EXP_VALL,
  VTYPE_INDEX_EXP_VALH,
  VTYPE_INDEX_EXP_EVAL_A,
  VTYPE_INDEX_EXP_EVAL_B,
  VTYPE_INDEX_EXP_EVAL_C,
  VTYPE_INDEX_EXP_EVAL_D,
  VTYPE_INDEX_EXP_NUM
} vtype_exp_indices;

/*! @} */

/*!
 Mask for signal supplemental field when writing to CDD file.
*/
#define VSUPPL_MASK               0x7f

/*!
 \addtogroup expression_types Expression Types

 The following defines specify the various flavors of expression types that can be stored in its elem_ptr
 union.

 @{
*/

/*! Specifies that the element pointer is not valid */
#define ETYPE_NONE      0

/*! Specifies that the element pointer points to a functional unit */
#define ETYPE_FUNIT     1

/*! Specifies that the element pointer points to a delay */
#define ETYPE_DELAY     2

/*! Specifies that the element pointer points to a thread */
#define ETYPE_THREAD    3

/*! Specifies that the element pointer points to a single temporary vector */
#define ETYPE_VEC1      4

/*! Specifies that the element pointer points to a vector2 block */
#define ETYPE_VEC2      5

/*! @} */

/*!
 \addtogroup thread_states Thread States

 The following defines specify the various states that a thread can be in.

 @{
*/

/*! Specifies that this thread is current inactive and can be reused */
#define THR_ST_NONE     0

/*! Specifies that this thread is currently in the active queue */
#define THR_ST_ACTIVE   1

/*! Specifies that this thread is currently in the delayed queue */
#define THR_ST_DELAYED  2

/*! Specifies that this thread is currently in the waiting queue */
#define THR_ST_WAITING  3

/*! @} */

/*!
 \addtogroup struct_union_types Struct/Union Types

 The following defines specify the various types of struct/union structures that can exist.

 @{
*/

/*! Specifies a struct */
#define SU_TYPE_STRUCT        0

/*! Specifies a union */
#define SU_TYPE_UNION         1

/*! Specifies a tagged union */
#define SU_TYPE_TAGGED_UNION  2

/*! @} */

/*!
 \addtogroup struct_union_member_types Struct/Union Member Types

 The following defines specify the various types of struct/union member types that can exist.

 @{
*/

/*! Specifies the member is a void type */
#define SU_MEMTYPE_VOID      0

/*! Specifies the member is a signal type */
#define SU_MEMTYPE_SIG       1

/*! Specifies the member is a typedef */
#define SU_MEMTYPE_TYPEDEF   2

/*! Specifies the member is an enumeration */
#define SU_MEMTYPE_ENUM      3

/*! Specifies the member is a struct, union or tagged union */
#define SU_MEMTYPE_SU        4

/*! @} */

/*! Overload for the snprintf function which verifies that we don't overrun character arrays */
//#define snprintf(x,y,...)	assert( snprintf( x, y, __VA_ARGS__ ) < (y) );

/*! Performs time comparison with the sim_time structure */
#define TIME_CMP_LE(x,y)         ((((x).lo <= (y).lo) && ((x).hi <= (y).hi)) || ((x).hi < (y).hi))
#define TIME_CMP_GT(x,y)         (((x).lo > (y).lo) || ((x).hi > (y).hi))
#define TIME_CMP_GE(x,y)         ((((x).lo >= (y).lo) && ((x).hi >= (y).hi)) || ((x).hi > (y).hi))
#define TIME_CMP_NE(x,y)         (((x).lo ^ (y).lo) || ((x).hi ^ (y).hi))

/*! Performs time increment where x is the sim_time structure to increment and y is a 64-bit value to increment to */
#define TIME_INC(x,y)           (x).hi+=((0xffffffff-(x).lo)<(y).lo)?((y).hi+1):(y).hi; (x).lo+=(y).lo;

/*!
 Defines boolean variables used in most functions.
*/
typedef enum {
  FALSE,   /*!< Boolean false value */
  TRUE     /*!< Boolean true value */
} bool;

/*!
 Create an 8-bit unsigned value.
*/
#if SIZEOF_CHAR == 1
typedef unsigned char uint8;
#define UINT8(x) x
#define ato8(x)  atoi(x)
#elif SIZEOF_SHORT == 1
typedef unsigned short uint8;
#define UINT8(x) x
#define ato8(x)  atoi(x)
#elif SIZEOF_INT == 1
typedef unsigned int uint8;
#define UINT8(x) x
#define ato8(x)  atoi(x)
#elif SIZEOF_LONG == 1
typedef unsigned long uint8;
#define UINT8(x) x
#define ato8(x)  atol(x)
#elif SIZEOF_LONG_LONG == 1
typedef unsigned long long uint8;
#define UINT8(x) x ## LL
#define ato8(x)  atoll(x)
#else
#error "Unable to find an 8-bit data type"
#endif

/*!
 Create an 16-bit unsigned value.
*/
#if SIZEOF_CHAR == 2
typedef unsigned char uint16;
#define UINT16(x) x
#define ato16(x)  atoi(x)
#elif SIZEOF_SHORT == 2
typedef unsigned short uint16;
#define UINT16(x) x
#define ato16(x)  atoi(x)
#elif SIZEOF_INT == 2
typedef unsigned int uint16;
#define UINT16(x) x
#define ato16(x)  atoi(x)
#elif SIZEOF_LONG == 2
typedef unsigned long uint16;
#define UINT16(x) x
#define ato16(x)  atol(x)
#elif SIZEOF_LONG_LONG == 2
typedef unsigned long long uint16;
#define UINT16(x) x ## LL
#define ato16(x)  atoll(x)
#else
#error "Unable to find a 16-bit data type"
#endif

/*!
 Create a 32-bit unsigned value.
*/
#if SIZEOF_CHAR == 4
typedef unsigned char uint32;
#define UINT32(x) x
#define ato32(x)  atoi(x)
#elif SIZEOF_SHORT == 4
typedef unsigned short uint32;
#define UINT32(x) x
#define ato32(x)  atoi(x)
#elif SIZEOF_INT == 4
typedef unsigned int uint32;
#define UINT32(x) x
#define ato32(x)  atoi(x)
#elif SIZEOF_LONG == 4
typedef unsigned long uint32;
#define UINT32(x) x
#define ato32(x)  atol(x)
#elif SIZEOF_LONG_LONG == 4
typedef unsigned long long uint32;
#define UINT32(x) x ## LL
#define ato32(x)  atoll(x)
#else
#error "Unable to find a 32-bit data type"
#endif

/*!
 Create a 32-bit real value.
*/
#if SIZEOF_FLOAT == 4
typedef float real32;
#elif SIZEOF_DOUBLE == 4
typedef double real32;
#else
#error "Unable to find a 32-bit real data type"
#endif

/*!
 Create a 64-bit unsigned value.
*/
#if SIZEOF_CHAR == 8
typedef char int64;
typedef unsigned char uint64;
#define UINT64(x) x
#define ato64(x)  atoi(x)
#elif SIZEOF_SHORT == 8
typedef short int64;
typedef unsigned short uint64;
#define UINT64(x) x
#define ato64(x)  atoi(x)
#elif SIZEOF_INT == 8
typedef int int64;
typedef unsigned int uint64;
#define UINT64(x) x
#define ato64(x)  atoi(x)
#elif SIZEOF_LONG == 8
typedef long int64;
typedef unsigned long uint64;
#define UINT64(x) x
#define ato64(x)  atol(x)
#elif SIZEOF_LONG_LONG == 8
typedef long long int64;
typedef unsigned long long uint64;
#define UINT64(x) x ## LL
#define ato64(x)  atoll(x)
#else
#error "Unable to find a 64-bit data type"
#endif

/*!
 Create a 64-bit real value.
*/
#if SIZEOF_FLOAT == 8
typedef float real64;
#elif SIZEOF_DOUBLE == 8
typedef double real64;
#else
#error "Unable to find a 64-bit real data type"
#endif

/*!
 A nibble is a 8-bit value.
*/
typedef uint8 nibble;

/*!
 A control is a 32-bit value.
*/
typedef uint32 control;

/*!
 Machine-dependent value.
*/
typedef unsigned long ulong;

/*!
 Create defines for unsigned long.
*/
#if SIZEOF_LONG == 1
#define UL_SET     0xff
#define UL_DIV_VAL 3
#define UL_MOD_VAL 0x7
#define UL_BITS    8
#elif SIZEOF_LONG == 2
#define UL_SET     0xffff
#define UL_DIV_VAL 4
#define UL_MOD_VAL 0xf
#define UL_BITS    16
#elif SIZEOF_LONG == 4
#define UL_SET     0xffffffff
#define UL_DIV_VAL 5
#define UL_MOD_VAL 0x1f
#define UL_BITS    32
#elif SIZEOF_LONG == 8
#define UL_SET     0xffffffffffffffff
#define UL_DIV_VAL 6
#define UL_MOD_VAL 0x3f
#define UL_BITS    64
#else
#error "Unsigned long is of an unsupported size"
#endif

/*! Divides a bit position by an unsigned long */
#define UL_DIV(x)  ((x) >> UL_DIV_VAL)

/*! Mods a bit position by an unsigned long */
#define UL_MOD(x)  ((x) &  UL_MOD_VAL)

/*------------------------------------------------------------------------------*/

union esuppl_u;

/*!
 Renaming esuppl_u for naming convenience.
*/
typedef union esuppl_u esuppl;

/*!
 A esuppl is a 32-bit value that represents the supplemental field of an expression.
*/
union esuppl_u {
  uint32   all;               /*!< Controls all bits within this union */
  struct {
 
    /* MASKED BITS */
    uint32 swapped        :1;  /*!< Bit 0.  Mask bit = 1.  Indicates that the children of this expression have been
                                    swapped.  The swapping of the positions is performed by the score command (for
                                    multi-bit selects) and this bit indicates to the report code to swap them back
                                    when displaying them in. */
    uint32 root           :1;  /*!< Bit 1.  Mask bit = 1.  Indicates that this expression is a root expression.
                                    Traversing to the parent pointer will take you to a statement type. */
    uint32 false          :1;  /*!< Bit 2.  Mask bit = 1.  Indicates that this expression has evaluated to a value
                                    of FALSE during the lifetime of the simulation. */
    uint32 true           :1;  /*!< Bit 3.  Mask bit = 1.  Indicates that this expression has evaluated to a value
                                    of TRUE during the lifetime of the simulation. */
    uint32 left_changed   :1;  /*!< Bit 4.  Mask bit = 1.  Indicates that this expression has its left child
                                    expression in a changed state during this timestamp. */
    uint32 right_changed  :1;  /*!< Bit 5.  Mask bit = 1.  Indicates that this expression has its right child
                                    expression in a changed state during this timestamp. */
    uint32 eval_00        :1;  /*!< Bit 6.  Mask bit = 1.  Indicates that the value of the left child expression
                                    evaluated to FALSE and the right child expression evaluated to FALSE. */
    uint32 eval_01        :1;  /*!< Bit 7.  Mask bit = 1.  Indicates that the value of the left child expression
                                    evaluated to FALSE and the right child expression evaluated to TRUE. */
    uint32 eval_10        :1;  /*!< Bit 8.  Mask bit = 1.  Indicates that the value of the left child expression
                                    evaluated to TRUE and the right child expression evaluated to FALSE. */
    uint32 eval_11        :1;  /*!< Bit 9.  Mask bit = 1.  Indicates that the value of the left child expression
                                    evaluated to TRUE and the right child expression evaluated to TRUE. */
    uint32 lhs            :1;  /*!< Bit 10.  Mask bit = 1.  Indicates that this expression exists on the left-hand
                                    side of an assignment operation. */
    uint32 in_func        :1;  /*!< Bit 11.  Mask bit = 1.  Indicates that this expression exists in a function */
    uint32 owns_vec       :1;  /*!< Bit 12.  Mask bit = 1.  Indicates that this expression either owns its vector
                                    structure or shares it with someone else. */
    uint32 excluded       :1;  /*!< Bit 13.  Mask bit = 1.  Indicates that this expression should be excluded from
                                    coverage results.  If a parent expression has been excluded, all children expressions
                                    within its tree are also considered excluded (even if their excluded bits are not
                                    set. */
    uint32 type           :3;  /*!< Bits 16:14.  Mask bit = 1.  Indicates how the pointer element should be treated as */
    uint32 base           :3;  /*!< Bits 19:17.  Mask bit = 1.  When the expression op is a STATIC, specifies the base
                                    type of the value (DECIMAL, HEXIDECIMAL, OCTAL, BINARY, QSTRING). */
 
    /* UNMASKED BITS */
    uint32 eval_t         :1;  /*!< Bit 20.  Mask bit = 0.  Indicates that the value of the current expression is
                                    currently set to TRUE (temporary value). */
    uint32 eval_f         :1;  /*!< Bit 21.  Mask bit = 0.  Indicates that the value of the current expression is
                                    currently set to FALSE (temporary value). */
    uint32 comb_cntd      :1;  /*!< Bit 22.  Mask bit = 0.  Indicates that the current expression has been previously
                                    counted for combinational coverage.  Only set by report command (therefore this bit
                                    will always be a zero when written to CDD file. */
    uint32 exp_added      :1;  /*!< Bit 23.  Mask bit = 0.  Temporary bit value used by the score command but not
                                    displayed to the CDD file.  When this bit is set to a one, it indicates to the
                                    db_add_expression function that this expression and all children expressions have
                                    already been added to the functional unit expression list and should not be added again. */
    uint32 owned          :1;  /*!< Bit 24.  Mask bit = 0.  Temporary value used by the score command to indicate
                                    if this expression is already owned by a mod_parm structure. */
    uint32 gen_expr       :1;  /*!< Bit 25.  Mask bit = 0.  Temporary value used by the score command to indicate
                                    that this expression is a part of a generate expression. */
    uint32 prev_called    :1;  /*!< Bit 26.  Mask bit = 0.  Temporary value used by named block and task expression
                                    functions to indicate if we are in the middle of executing a named block or task
                                    expression (since these cause a context switch to occur. */
    uint32 for_cntrl      :1;  /*!< Bit 27.  Mask bit = 0.  Temporary value used by the score command which sets to true
                                    if this expression exists within the control portion of a for loop. */
  } part;
};

/*------------------------------------------------------------------------------*/

union ssuppl_u;

/*!
 Renaming ssuppl_u field for convenience.
*/
typedef union ssuppl_u ssuppl;

/*!
 Supplemental signal information.
*/
union ssuppl_u {
  uint32 all;
  struct {
    uint32 col            :16; /*!< Specifies the starting column this signal is declared on */
    uint32 type           :4;  /*!< Specifies signal type (see \ref ssuppl_type for legal values) */
    uint32 big_endian     :1;  /*!< Specifies if this signal is in big or little endianness */
    uint32 excluded       :1;  /*!< Specifies if this signal should be excluded for toggle coverage */
    uint32 not_handled    :1;  /*!< Specifies if this signal is handled by Covered or not */
    uint32 assigned       :1;  /*!< Specifies that this signal will be assigned from simulated results (instead of dumpfile) */
    uint32 mba            :1;  /*!< Specifies that this signal MUST be assigned from simulated results because this information
                                     is NOT provided in the dumpfile */
    uint32 implicit_size  :1;  /*!< Specifies that this signal has not been given a specified size by the user at the current time */
  } part;
};

/*------------------------------------------------------------------------------*/

union psuppl_u;

/*!
 Renaming psuppl_u field for convenience.
*/
typedef union psuppl_u psuppl;

/*!
 Supplemental module parameter information.
*/
union psuppl_u {
  uint32 all;
  struct {
    uint32 order    : 16;      /*!< Specifies the parameter order number in its module */
    uint32 type     : 3;       /*!< Specifies the parameter type (see \ref param_suppl_types for legal value) */
    uint32 owns_expr: 1;       /*!< Specifies the parameter is the owner of the associated expression */
    uint32 dimension: 10;      /*!< Specifies the signal dimension that this module parameter resolves */
  } part;
};

/*------------------------------------------------------------------------------*/

union isuppl_u;

/*!
 Renaming isuppl_u field for convenience.
*/
typedef union isuppl_u isuppl;

/*!
 Supplemental field for information line in CDD file.
*/
union isuppl_u {
  uint32 all;
  struct {
    uint32 scored      : 1;    /*!< Specifies if the design has been scored yet */
    uint32 excl_assign : 1;    /*!< Specifies if assign statements are being excluded from coverage */
    uint32 excl_always : 1;    /*!< Specifies if always statements are being excluded from coverage */
    uint32 excl_init   : 1;    /*!< Specifies if initial statements are being excluded from coverage */
    uint32 excl_final  : 1;    /*!< Specifies if final statements are being excluded from coverage */
    uint32 excl_pragma : 1;    /*!< Specifies if code encapsulated in coverage pragmas should be excluded from coverage */
    uint32 assert_ovl  : 1;    /*!< Specifies that OVL assertions should be included for coverage */
    uint32 vec_ul_size : 2;    /*!< Specifies the bit size of a vector element (0=8 bits, 1=16-bits, 2=32-bits, 3=64-bits) */
  } part;
};

/*------------------------------------------------------------------------------*/

union vsuppl_u;

/*!
 Renaming vsuppl_u field for convenience.
*/
typedef union vsuppl_u vsuppl;

/*!
 Supplemental field for vector structure.
*/
union vsuppl_u {
  nibble   all;                    /*!< Allows us to set all bits in the suppl field */
  struct {
    nibble type      :2;           /*!< Specifies what type of information is stored in this vector
                                        (see \ref vector_types for legal values) */
    nibble data_type :2;           /*!< Specifies what the size/type of a single value entry is */
    nibble owns_data :1;           /*!< Specifies if this vector owns its data array or not */
    nibble is_signed :1;           /*!< Specifies that this vector should be treated as a signed value */
    nibble is_2state :1;           /*!< Specifies that this vector should be treated as a 2-state value */
    nibble set       :1;           /*!< Specifies if this vector's data has been set previously */
  } part;
};

/*------------------------------------------------------------------------------*/

union fsuppl_u;

/*!
 Renaming fsuppl_u field for convenience.
*/
typedef union fsuppl_u fsuppl;

/*!
 Supplemental field for FSM table structure.
*/
union fsuppl_u {
  nibble all;                      /*!< Allows us to set all bits in the suppl field */
  struct {
    nibble known : 1;              /*!< Specifies if the possible state transitions are known */
  } part;
};

/*------------------------------------------------------------------------------*/

union asuppl_u;

/*!
 Renaming asuppl_u field for convenience.
*/
typedef union asuppl_u asuppl;

/*!
 Supplemental field for FSM table structure.
*/
union asuppl_u {
  nibble all;                      /*!< Allows us to set all bits in the suppl field */
  struct {
    nibble hit      : 1;           /*!< Specifies if from->to arc was hit */
    nibble excluded : 1;           /*!< Specifies if from->to transition should be excluded from coverage consideration */
  } part;
};

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION DECLARATIONS  */

union  expr_stmt_u;
struct exp_info_s;
struct str_link_s;
struct vector_s;
struct const_value_s;
struct vecblk_s;
struct exp_dim_s;
struct expression_s;
struct vsignal_s;
struct fsm_s;
struct fsm_table_arc_s;
struct fsm_table_s;
struct statement_s;
struct sig_link_s;
struct stmt_iter_s;
struct exp_link_s;
struct stmt_link_s;
struct stmt_loop_link_s;
struct statistic_s;
struct mod_parm_s;
struct inst_parm_s;
struct fsm_arc_s;
struct fsm_link_s;
struct race_blk_s;
struct func_unit_s;
struct funit_link_s;
struct inst_link_s;
struct sym_sig_s;
struct symtable_s;
struct static_expr_s;
struct vector_width_s;
struct exp_bind_s;
struct case_stmt_s;
struct case_gitem_s;
struct funit_inst_s;
struct tnode_s;

#ifdef HAVE_SYS_TIME_H
struct timer_s;
#endif

struct fsm_var_s;
struct fv_bind_s;
struct attr_param_s;
struct stmt_blk_s;
struct thread_s;
struct thr_link_s;
struct thr_list_s;
struct perf_stat_s;
struct port_info_s;
struct param_oride_s;
struct gen_item_s;
struct gitem_link_s;
struct typedef_item_s;
struct enum_item_s;
struct sig_range_s;
struct dim_range_s;
struct reentrant_s;
struct rstack_entry_s;
struct struct_union_s;
struct su_member_s;
struct profiler_s;
struct db_s;
struct sim_time_s;

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION TYPEDEFS  */

/*!
 Renaming expression operation information structure for convenience.
*/
typedef struct exp_info_s exp_info;

/*!
 Renaming string link structure for convenience.
*/
typedef struct str_link_s str_link;

/*!
 Renaming vector structure for convenience.
*/
typedef struct vector_s vector;

/*!
 Renaming vector structure for convenience.
*/
typedef struct const_value_s const_value;

/*!
 Renaming vector structure for convenience.
*/
typedef struct vecblk_s vecblk;

/*!
 Renaming expression dimension structure for convenience.
*/
typedef struct exp_dim_s exp_dim;

/*!
 Renaming expression statement union for convenience.
*/
typedef union expr_stmt_u expr_stmt;

/*!
 Renaming expression structure for convenience.
*/
typedef struct expression_s expression;

/*!
 Renaming signal structure for convenience.
*/
typedef struct vsignal_s     vsignal;

/*!
 Renaming FSM structure for convenience.
*/
typedef struct fsm_s fsm;

/*!
 Renaming FSM table arc structure for convenience.
*/
typedef struct fsm_table_arc_s fsm_table_arc;

/*!
 Renaming FSM table structure for convenience.
*/
typedef struct fsm_table_s fsm_table;

/*!
 Renaming statement structure for convenience.
*/
typedef struct statement_s statement;

/*!
 Renaming signal link structure for convenience.
*/
typedef struct sig_link_s sig_link;

/*!
 Renaming statement iterator structure for convenience.
*/
typedef struct stmt_iter_s stmt_iter;

/*!
 Renaming expression link structure for convenience.
*/
typedef struct exp_link_s exp_link;

/*!
 Renaming statement link structure for convenience.
*/
typedef struct stmt_link_s stmt_link;

/*!
 Renaming statement loop link structure for convenience.
*/
typedef struct stmt_loop_link_s stmt_loop_link;

/*!
 Renaming statistic structure for convenience.
*/
typedef struct statistic_s statistic;

/*!
 Renaming module parameter structure for convenience.
*/
typedef struct mod_parm_s mod_parm;

/*!
 Renaming instance parameter structure for convenience.
*/
typedef struct inst_parm_s inst_parm;

/*!
 Renaming FSM arc structure for convenience.
*/
typedef struct fsm_arc_s fsm_arc;

/*!
 Renaming fsm_link structure for convenience.
*/
typedef struct fsm_link_s fsm_link;

/*!
 Renaming race_blk structure for convenience.
*/
typedef struct race_blk_s race_blk;

/*!
 Renaming functional unit structure for convenience.
*/
typedef struct func_unit_s func_unit;

/*!
 Renaming functional unit link structure for convenience.
*/
typedef struct funit_link_s funit_link;

/*!
 Renaming functional unit instance link structure for convenience.
*/
typedef struct inst_link_s inst_link;

/*!
 Renaming symbol signal structure for convenience.
*/
typedef struct sym_sig_s sym_sig;

/*!
 Renaming symbol table structure for convenience.
*/
typedef struct symtable_s symtable;

/*!
 Renaming static expression structure for convenience.
*/
typedef struct static_expr_s static_expr;

/*!
 Renaming vector width structure for convenience.
*/
typedef struct vector_width_s vector_width;

/*!
 Renaming signal/functional unit to expression binding structure for convenience.
*/
typedef struct exp_bind_s exp_bind;

/*!
 Renaming case statement structure for convenience.
*/
typedef struct case_stmt_s case_statement;

/*!
 Renaming case generate item structure for convenience.
*/
typedef struct case_gitem_s case_gitem;

/*!
 Renaming functional unit instance structure for convenience.
*/
typedef struct funit_inst_s funit_inst;

/*!
 Renaming tree node structure for convenience.
*/
typedef struct tnode_s tnode;

#ifdef HAVE_SYS_TIME_H
/*!
 Renaming timer structure for convenience.
*/
typedef struct timer_s timer;
#endif

/*!
 Renaming FSM variable structure for convenience.
*/
typedef struct fsm_var_s fsm_var;

/*!
 Renaming FSM bind structure for convenience.
*/
typedef struct fv_bind_s fv_bind;

/*!
 Renaming attribute parameter structure for convenience.
*/
typedef struct attr_param_s attr_param;

/*!
 Renaming statement-signal structure for convenience.
*/
typedef struct stmt_blk_s stmt_blk;

/*!
 Renaming thread structure for convenience.
*/
typedef struct thread_s thread;

/*!
 Renaming thr_link structure for convenience.
*/
typedef struct thr_link_s thr_link;

/*!
 Renaming thr_list structure for convenience.
*/
typedef struct thr_list_s thr_list;

/*!
 Renaming perf_stat structure for convenience.
*/
typedef struct perf_stat_s perf_stat;

/*!
 Renaming port_info_s structure for convenience.
*/
typedef struct port_info_s port_info;

/*!
 Renaming param_orides_s structure for convenience.
*/
typedef struct param_oride_s param_oride;

/*!
 Renaming gen_item_s structure for convenience.
*/
typedef struct gen_item_s gen_item;

/*!
 Renaming gitem_link_s structure for convenience.
*/
typedef struct gitem_link_s gitem_link;

/*!
 Renaming typedef_item_s structure for convenience.
*/
typedef struct typedef_item_s typedef_item;

/*!
 Renaming enum_item_s structure for convenience.
*/
typedef struct enum_item_s enum_item;

/*!
 Renaming sig_range_s structure for convenience.
*/
typedef struct sig_range_s sig_range;

/*!
 Renaming dim_range_s structure for convenience.
*/
typedef struct dim_range_s dim_range;

/*!
 Renaming reentrant_s structure for convenience.
*/
typedef struct reentrant_s reentrant;

/*!
 Renaming rstack_entry_s structure for convenience.
*/
typedef struct rstack_entry_s rstack_entry;

/*!
 Renaming struct_unions_s structure for convenience.
*/
typedef struct struct_union_s struct_union;

/*!
 Renaming su_member_s structure for convenience.
*/
typedef struct su_member_s su_member;

/*!
 Renaming profiler_s structure for convenience.
*/
typedef struct profiler_s profiler;

/*!
 Renaming db_s structure for convenience.
*/
typedef struct db_s db;

/*!
 Renaming sim_time_s structure for convenience.
*/
typedef struct sim_time_s sim_time;

/*------------------------------------------------------------------------------*/
/*  STRUCTURE/UNION DEFINITIONS  */

/*!
 Representation of simulation time -- used for performance enhancement purposes.
*/
struct sim_time_s {
  unsigned int lo;                   /*!< Lower 32-bits of the current time */
  unsigned int hi;                   /*!< Upper 32-bits of the current time */
  uint64       full;                 /*!< Full 64 bits of the current time - for displaying purposes only */
  bool         final;                /*!< Specifies if this is the final simulation timestep */
};

/*!
 Contains static information about each expression operation type.
*/
struct exp_info_s {
  char* name;                             /*!< Operation name */
  char* op_str;                           /*!< Operation string name for report output purposes */
  bool  (*func)( expression*, thread*, const sim_time* );  /*!< Operation function to call */
  struct {
    uint32 is_event:1;                   /*!< Specifies if operation is an event */
    uint32 is_static:1;                  /*!< Specifies if operation is a static value (does not change during simulation) */
    uint32 is_comb:2;                    /*!< Specifies if operation is combinational (both left/right expressions valid) */
    uint32 is_unary:1;                   /*!< Specifies if operation is unary (left expression valid only) */
    uint32 measurable:1;                 /*!< Specifies if this operation type can be measured */
    uint32 is_context_switch:1;          /*!< Specifies if this operation will cause a context switch */
    uint32 assignable:1;                 /*!< Specifies if this operation can be immediately assigned (i.e., +=) */
    uint32 tmp_vecs:3;                   /*!< Number of temporary vectors used by this expression */
  } suppl;                                /*!< Supplemental information about this expression */
};

/*!
 Specifies an element in a linked list containing string values.  This data
 structure allows us to add new elements to the list without resizing, this
 optimizes performance with small amount of overhead.
*/
struct str_link_s {
  char*         str;                 /*!< String to store */
  uint32        suppl;               /*!< 32-bit additional information */
  uint32        suppl2;              /*!< 32-bit additional information */
  nibble        suppl3;              /*!< 8-bit additional information */
  vector_width* range;               /*!< Pointer to optional range information */
  str_link*     next;                /*!< Pointer to next str_link element */
};

/*!
 Contains information for signal value.  This value is represented as
 a generic vector.  The vector.h/.c files contain the functions that
 manipulate this information.
*/
struct vector_s {
  int        width;                  /*!< Bit width of this vector */
  vsuppl     suppl;                  /*!< Supplemental field */
  union {
    ulong** ul;                      /*!< Machine sized unsigned integer array for value, signal, expression and memory types */
//    real32** r32;                    /*!< 32-bit real value (float) */
//    real64** r64;                    /*!< 64-bit real value (double) */
  } value;
};

/*!
 Contains information about a parsed constant value including its data and base type.
*/
struct const_value_s {
  vector* vec;                       /*!< Pointer to vector containing constant value */
  int     base;                      /*!< Base type of constant value */
};

/*!
 Contains temporary storage vectors used by certain expressions for performance purposes.
*/
struct vecblk_s {
  vector vec[5];                     /*!< Vector array */
  int    index;                      /*!< Specifies to the called function which vector may be accessed */
}; 

/*!
 Specifies the current dimensional LSB of the associated expression.  This is used by the DIM, SBIT, MBIT,
 MBIT_POS and MBIT_NEG expressions types for proper bit selection of given signal.
*/
struct exp_dim_s {
  int  curr_lsb;                     /*!< Calculated LSB of this expression dimension (if -1, LSB was an X) */
  int  dim_lsb;                      /*!< Dimensional LSB (static value) */
  bool dim_be;                       /*!< Dimensional big endianness (static value) */
  int  dim_width;                    /*!< Dimensional width of current expression (static value) */
  bool last;                         /*!< Specifies if this is the dimension that should handle the signal interaction */
  bool set_mem_rd;                   /*!< Set to TRUE if the MEM_RD bit should be set for this entry */
};

/*!
 Allows the parent pointer of an expression to point to either another expression
 or a statement.
*/
union expr_stmt_u {
  expression* expr;                  /*!< Pointer to expression */
  statement*  stmt;                  /*!< Pointer to statement */
};

/*!
 An expression is defined to be a logical combination of signals/values.  Expressions may
 contain subexpressions (which are expressions in and of themselves).  An measurable expression
 may only evaluate to TRUE (1) or FALSE (0).  If the parent expression of this expression is
 NULL, then this expression is considered a root expression.  The nibble suppl contains the
 run-time information for its expression.
*/
struct expression_s {
  vector*      value;              /*!< Current value and toggle information of this expression */
  exp_op_type  op;                 /*!< Expression operation type */
  esuppl       suppl;              /*!< Supplemental information for the expression */
  int          id;                 /*!< Specifies unique ID for this expression in the parent */
  int          ulid;               /*!< Specifies underline ID for reporting purposes */
  int          line;               /*!< Specified line in file that this expression is found on */
  uint32       exec_num;           /*!< Specifies the number of times this expression was executed during simulation */
  uint32       col;                /*!< Specifies column location of beginning/ending of expression */
  vsignal*     sig;                /*!< Pointer to signal.  If NULL then no signal is attached */
  char*        name;               /*!< Name of signal/function/task for output purposes (only valid if we are binding
                                        to a signal, task or function */
  expr_stmt*   parent;             /*!< Parent expression/statement */
  expression*  right;              /*!< Pointer to expression on right */
  expression*  left;               /*!< Pointer to expression on left */
  fsm*         table;              /*!< Pointer to FSM table associated with this expression */
  union {
    func_unit* funit;              /*!< Pointer to task/function to be called by this expression */
    thread*    thr;                /*!< Pointer to next thread to be called */
    uint64*    scale;              /*!< Pointer to parent functional unit's timescale value */
    vecblk*    tvecs;              /*!< Temporary vectors that are sized to match value */   
    exp_dim*   dim;                /*!< Current dimensional LSB of this expression (valid for DIM, SBIT_SEL, MBIT_SEL, MBIT_NEG and MBIT_POS) */
  } elem;
};

/*!
 Stores all information needed to represent a signal.  If value of value element is non-zero at the
 end of the run, this signal has been simulated.
*/
struct vsignal_s {
  char*      name;                   /*!< Full hierarchical name of signal in design */
  int        line;                   /*!< Specifies line number that this signal was declared on */
  ssuppl     suppl;                  /*!< Supplemental information for this signal */
  vector*    value;                  /*!< Pointer to vector value of this signal */
  int        pdim_num;               /*!< Number of packed dimensions in pdim array */
  int        udim_num;               /*!< Number of unpacked dimensions in pdim array */
  dim_range* dim;                    /*!< Unpacked/packed dimension array */
  exp_link*  exp_head;               /*!< Head pointer to list of expressions */
  exp_link*  exp_tail;               /*!< Tail pointer to list of expressions */
};

/*!
 Stores information for an FSM within the design.
*/
struct fsm_s {
  char*       name;                  /*!< User-defined name that this FSM pertains to */
  expression* from_state;            /*!< Pointer to from_state expression */
  expression* to_state;              /*!< Pointer to to_state expression */
  fsm_arc*    arc_head;              /*!< Pointer to head of list of expression pairs that describe the valid FSM arcs */
  fsm_arc*    arc_tail;              /*!< Pointer to tail of list of expression pairs that describe the valid FSM arcs */
  fsm_table*  table;                 /*!< FSM arc traversal table */
  bool        exclude;               /*!< Set to TRUE if the states/transitions of this table should be excluded as determined by pragmas */
};

/*!
 Stores information for a uni-/bi-directional state transition.
*/
struct fsm_table_arc_s {
  asuppl       suppl;                /*!< Supplemental field for this state transition entry */
  unsigned int from;                 /*!< Index to from_state vector value in fsm_table vector array */
  unsigned int to;                   /*!< Index to to_state vector value in fsm_table vector array */
};

/*!
 Stores information for an FSM table (including states and state transitions).
*/
struct fsm_table_s {
  fsuppl          suppl;             /*!< Supplemental field for FSM table */
  vector**        fr_states;         /*!< List of FSM from state vectors that are valid for this FSM (VTYPE_VAL) */
  unsigned int    num_fr_states;     /*!< Contains the number of from states stored in this table */
  vector**        to_states;         /*!< List of FSM to state vectors that are valid for this FSM (VTYPE_VAL) */
  unsigned int    num_to_states;     /*!< Contains the number of to states stored in this table */
  fsm_table_arc** arcs;              /*!< List of FSM state transitions */
  unsigned int    num_arcs;          /*!< Contains the number of arcs stored in this table */
};

/*!
 A statement is defined to be the structure connected to the root of an expression tree.
 Statements are sequentially run in the run-time engine, starting at the root statement.
 After a statements expression tree has been checked for changes and possibly placed into
 the run-time expression queue, the statements calls the next statement to be run.
 If the value of the root expression of the associated expression tree is a non-zero value,
 the next_true statement will be executed (if next_true is not NULL); otherwise, the
 next_false statement is run.  If next_true and next_false point to the same structure, we
 have hit the end of the statement sequence; executing the next statement will be executing
 the first statement of the statement sequence (next_true and next_false should both point
 to the first statement in the sequence).
*/
struct statement_s {
  expression* exp;                   /*!< Pointer to associated expression tree */
  statement*  next_true;             /*!< Pointer to next statement to run if expression tree non-zero */
  statement*  next_false;            /*!< Pointer to next statement to run if next_true not picked */
  int         conn_id;               /*!< Current connection ID (used to make sure that we do not infinitely loop
                                          in connecting statements together) */
  func_unit*  funit;                 /*!< Pointer to statement's functional unit that it belongs to */
  union {
    uint32  all;
    struct {
      /* Masked bits */
      uint32  head      :1;          /*!< Bit 0.  Mask bit = 1.  Indicates the statement is a head statement */
      uint32  stop_true :1;          /*!< Bit 1.  Mask bit = 1.  Indicates the statement which this expression belongs
                                          should write itself to the CDD and not continue to traverse its next_true pointer. */
      uint32  stop_false:1;          /*!< Bit 2.  Mask bit = 1.  Indicates the statement which this expression belongs
                                          should write itself to the CDD and not continue to traverse its next_false pointer. */
      uint32  cont      :1;          /*!< Bit 3.  Mask bit = 1.  Indicates the statement which this expression belongs is
                                          part of a continuous assignment.  As such, stop simulating this statement tree
                                          after this expression tree is evaluated. */
      uint32  is_called :1;          /*!< Bit 4.  Mask bit = 1.  Indicates that this statement is called by a FUNC_CALL,
                                          TASK_CALL, NB_CALL or FORK statement.  If a statement has this bit set, it will NOT
                                          be automatically placed in the thread queue at time 0. */
      uint32  excluded  :1;          /*!< Bit 5.  Mask bit = 1.  Indicates that this statement (and its associated expression
                                          tree) should be excluded from coverage results. */
      uint32  final     :1;          /*!< Bit 6.  Mask bit = 1.  Indicates that this statement block should only be executed
                                          during the final simulation step. */
      uint32  ignore_rc :1;          /*!< Bit 7.  Mask bit = 1.  Specifies that we should ignore race condition checking for
                                          this statement. */

      /* Unmasked bits */
      uint32  added     :1;          /*!< Bit 8.  Mask bit = 0.  Temporary bit value used by the score command but not
                                          displayed to the CDD file.  When this bit is set to a one, it indicates to the
                                          db_add_statement function that this statement and all children statements have
                                          already been added to the functional unit statement list and should not be added again. */
    } part;
  } suppl;                           /*!< Supplemental bits for statements */
};

/*!
 Linked list element that stores a signal.
*/
struct sig_link_s {
  vsignal*  sig;                     /*!< Pointer to signal in list */
  sig_link* next;                    /*!< Pointer to next signal link element in list */
};

/*!
 Statement link iterator.
*/
struct stmt_iter_s {
  stmt_link* curr;                   /*!< Pointer to current statement link */
  stmt_link* last;                   /*!< Two-way pointer to next/previous statement link */
};

/*!
 Expression link element.  Stores pointer to an expression.
*/
struct exp_link_s {
  expression* exp;                   /*!< Pointer to expression */
  exp_link*   next;                  /*!< Pointer to next expression element in list */
};

/*!
 Statement link element.  Stores pointer to a statement.
*/
struct stmt_link_s {
  statement* stmt;                   /*!< Pointer to statement */
  stmt_link* ptr;                    /*!< Pointer to next statement element in list */
};

/*!
 Special statement link that stores the ID of the statement that the specified
 statement pointer needs to traverse when it has completed.  These structure types
 are used by the statement CDD reader.  When a statement is read that points to a
 statement that hasn't been read out of the CDD, the read statement is stored into
 one of these link types that is linked like a stack (pushed/popped at the head).
 The head of this stack is interrogated by future statements being read out.  When
 a statement's ID matches the ID at the head of the stack, the element is popped and
 the two statements are linked accordingly.  This sequence is used to handle statement
 looping.
*/
struct stmt_loop_link_s {
  statement*      stmt;              /*!< Pointer to last statement in loop */
  int             id;                /*!< ID of next statement after last */
  bool            next_true;         /*!< Specifies if the ID is for next_true or next_false */
  stmt_loop_link* next;              /*!< Pointer to next statement in stack */
};

/*!
 Contains statistics for coverage results which is stored in a functional unit instance.
*/
struct statistic_s {
  int   line_total;                  /*!< Total number of lines parsed */
  int   line_hit;                    /*!< Number of lines executed during simulation */
  int   tog_total;                   /*!< Total number of bits to toggle */
  int   tog01_hit;                   /*!< Number of bits toggling from 0 to 1 */
  int   tog10_hit;                   /*!< Number of bits toggling from 1 to 0 */
  int   comb_total;                  /*!< Total number of expression combinations */
  int   comb_hit;                    /*!< Number of logic combinations hit */
  int   state_total;                 /*!< Total number of FSM states */
  int   state_hit;                   /*!< Number of FSM states reached */
  int   arc_total;                   /*!< Total number of FSM arcs */
  int   arc_hit;                     /*!< Number of FSM arcs traversed */
  int   race_total;                  /*!< Total number of race conditions found */
  int   rtype_total[RACE_TYPE_NUM];  /*!< Total number of each race condition type found */
  int   assert_total;                /*!< Total number of assertions */
  int   assert_hit;                  /*!< Number of assertions covered during simulation */
  int   mem_ae_total;                /*!< Total number of addressable memory elements */
  int   mem_wr_hit;                  /*!< Total number of addressable memory elements written */
  int   mem_rd_hit;                  /*!< Total number of addressable memory elements read */
  int   mem_tog_total;               /*!< Total number of bits in memories */
  int   mem_tog01_hit;               /*!< Total number of bits toggling from 0 to 1 in memories */
  int   mem_tog10_hit;               /*!< Total number of bits toggling from 1 to 0 in memories */
  bool  show;                        /*!< Set to TRUE if this module should be output to the report */
};

/*!
 Structure containing parts for a module parameter definition.
*/
struct mod_parm_s {
  char*        name;                 /*!< Name of parameter */
  static_expr* msb;                  /*!< Static expression containing the MSB of the module parameter */
  static_expr* lsb;                  /*!< Static expression containing the LSB of the module parameter */
  bool         is_signed;            /*!< Specifies if the module parameter was labeled as signed */
  expression*  expr;                 /*!< Expression tree containing value of parameter */
  psuppl       suppl;                /*!< Supplemental field */
  exp_link*    exp_head;             /*!< Pointer to head of expression list for dependents */
  exp_link*    exp_tail;             /*!< Pointer to tail of expression list for dependents */
  vsignal*     sig;                  /*!< Pointer to associated signal (if one is available) */
  char*        inst_name;            /*!< Stores name of instance that will have this parameter overriden (only valid for parameter overridding) */
  mod_parm*    next;                 /*!< Pointer to next module parameter in list */
};

/*!
 Structure containing parts for an instance parameter.
*/
struct inst_parm_s {
  vsignal*     sig;                  /*!< Name, MSB, LSB and vector value for this instance parameter */
  char*        inst_name;            /*!< Name of instance to which this structure belongs to */
  mod_parm*    mparm;                /*!< Pointer to base module parameter */
  inst_parm*   next;                 /*!< Pointer to next instance parameter in list */
};

/*!
 Contains information for a single state transition arc in an FSM within the design.
*/
struct fsm_arc_s {
  expression* from_state;            /*!< Pointer to expression that represents the state we are transferring from */
  expression* to_state;              /*!< Pointer to expression that represents the state we are transferring to */
  fsm_arc*    next;                  /*!< Pointer to next fsm_arc in this list */
};

/*!
 Linked list element that stores an FSM structure.
*/
struct fsm_link_s {
  fsm*      table;                   /*!< Pointer to FSM structure to store */
  fsm_link* next;                    /*!< Pointer to next element in fsm_link list */
};

/*!
 Contains information for storing race condition information
*/
struct race_blk_s {
  int       start_line;              /*!< Starting line number of statement block that was found to be a race condition */
  int       end_line;                /*!< Ending line number of statement block that was found to be a race condition */
  int       reason;                  /*!< Numerical reason for why this statement block was found to be a race condition */
  race_blk* next;                    /*!< Pointer to next race block in list */
};

/*!
 Contains information for a functional unit (i.e., module, named block, function or task).
*/
struct func_unit_s {
  int           type;                /*!< Specifies the type of functional unit this structure represents.
                                          Legal values are defined in \ref func_unit_types */
  char*         name;                /*!< Functional unit name */
  char*         filename;            /*!< File name where functional unit exists */
  int           start_line;          /*!< Starting line number of functional unit in its file */
  int           end_line;            /*!< Ending line number of functional unit in its file */
  int           ts_unit;             /*!< Timescale unit value */
  uint64        timescale;           /*!< Timescale for this functional unit contents */
  statistic*    stat;                /*!< Pointer to functional unit coverage statistics structure */
  sig_link*     sig_head;            /*!< Head pointer to list of signals in this functional unit */
  sig_link*     sig_tail;            /*!< Tail pointer to list of signals in this functional unit */
  exp_link*     exp_head;            /*!< Head pointer to list of expressions in this functional unit */
  exp_link*     exp_tail;            /*!< Tail pointer to list of expressions in this functional unit */
  statement*    first_stmt;          /*!< Pointer to first head statement in this functional unit (for tasks/functions only) */
  stmt_link*    stmt_head;           /*!< Head pointer to list of statements in this functional unit */
  stmt_link*    stmt_tail;           /*!< Tail pointer to list of statements in this functional unit */
  fsm_link*     fsm_head;            /*!< Head pointer to list of FSMs in this functional unit */
  fsm_link*     fsm_tail;            /*!< Tail pointer to list of FSMs in this functional unit */
  race_blk*     race_head;           /*!< Head pointer to list of race condition blocks in this functional unit if we are a module */
  race_blk*     race_tail;           /*!< Tail pointer to list of race condition blocks in this functional unit if we are a module */
  mod_parm*     param_head;          /*!< Head pointer to list of parameters in this functional unit if we are a module */
  mod_parm*     param_tail;          /*!< Tail pointer to list of parameters in this functional unit if we are a module */
  gitem_link*   gitem_head;          /*!< Head pointer to list of generate items within this module */
  gitem_link*   gitem_tail;          /*!< Tail pointer to list of generate items within this module */
  func_unit*    parent;              /*!< Pointer to parent functional unit (only valid for functions, tasks and named blocks */
  funit_link*   tf_head;             /*!< Head pointer to list of task/function functional units if we are a module */
  funit_link*   tf_tail;             /*!< Tail pointer to list of task/function functional units if we are a module */
  typedef_item* tdi_head;            /*!< Head pointer to list of typedef types for this functional unit */
  typedef_item* tdi_tail;            /*!< Tail pointer to list of typedef types for this functional unit */
  enum_item*    ei_head;             /*!< Head pointer to list of enumerated values for this functional unit */
  enum_item*    ei_tail;             /*!< Tail pointer to list of enumerated values for this functional unit */
  struct_union* su_head;             /*!< Head pointer to list of struct/unions for this functional unit */
  struct_union* su_tail;             /*!< Tail pointer to list of struct/unions for this functional unit */
  int           elem_type;           /*!< Set to 0 if elem should be treated as a thread pointer; set to 1 if elem should be treated
                                          as a thread list pointer. */
  union {
    thread*   thr;                   /*!< Pointer to a single thread that this statement is associated with */
    thr_list* tlist;                 /*!< Pointer to a list of threads that this statement is currently associated with */
  } elem;                            /*!< Pointer element */
};

/*!
 Linked list element that stores a functional unit (no scope).
*/
struct funit_link_s {
  func_unit*  funit;                 /*!< Pointer to functional unit in list */
  funit_link* next;                  /*!< Next functional unit link in list */
};

/*!
 Linked list element that stores a functional unit instance.
*/
struct inst_link_s {
  funit_inst* inst;                  /*!< Pointer to functional unit instance in list */
  inst_link*  next;                  /*!< Next functional unit instance link in list */
};

/*!
 For each signal within a symtable entry, an independent MSB and LSB needs to be
 stored along with the signal pointer that it references to properly assign the
 VCD signal value to the appropriate signal.  This structure is setup to hold these
 three key pieces of information in a list-style data structure.
*/
struct sym_sig_s {
  vsignal* sig;                      /*!< Pointer to signal that this symtable entry refers to */
  int      msb;                      /*!< Most significant bit of value to set */
  int      lsb;                      /*!< Least significant bit of value to set */
  sym_sig* next;                     /*!< Pointer to next sym_sig link in list */
};

/*!
 Stores symbol name of signal along with pointer to signal itself into a lookup table
*/
struct symtable_s {
  sym_sig*  sig_head;                /*!< Pointer to head of sym_sig list */
  sym_sig*  sig_tail;                /*!< Pointer to tail of sym_sig list */
  char*     value;                   /*!< String representation of last current value */
  int       size;                    /*!< Number of bytes allowed storage for value */
  symtable* table[256];              /*!< Array of symbol tables for next level */
};

/*!
 Specifies possible values for a static expression (constant value).
*/
struct static_expr_s {
  expression* exp;                   /*!< Specifies if static value is an expression */
  int         num;                   /*!< Specifies if static value is a numeric value */
};

/*!
 Specifies bit range of a signal or expression.
*/
struct vector_width_s {
  static_expr*  left;                /*!< Specifies left bit value of bit range */
  static_expr*  right;               /*!< Specifies right bit value of bit range */
  bool          implicit;            /*!< Specifies if vector width was explicitly set by user or implicitly set by parser */
};

/*!
 Binds a signal to an expression.
*/
struct exp_bind_s {
  int              type;             /*!< Specifies if name refers to a signal (0), function (FUNIT_FUNCTION) or task (FUNIT_TASK) */
  char*            name;             /*!< Name of Verilog scoped signal/functional unit to bind */
  bool             clear_assigned;   /*!< If TRUE, clears the signal assigned supplemental field without binding */
  int              line;             /*!< Specifies line of expression -- used when expression is deallocated and we are clearing */
  expression*      exp;              /*!< Expression to bind. */
  expression*      fsm;              /*!< FSM expression to create value for when this expression is bound */
  func_unit*       funit;            /*!< Pointer to functional unit containing expression */
  exp_bind*        next;             /*!< Pointer to next binding in list */
};

/*!
 Binds an expression to a statement.  This is used when constructing a case
 structure.
*/
struct case_stmt_s {
  expression*     expr;              /*!< Pointer to case equality expression */
  statement*      stmt;              /*!< Pointer to first statement in case statement */
  int             line;              /*!< Line number of case statement */
  case_statement* prev;              /*!< Pointer to previous case statement in list */
};

/*!
 Binds an expression to a generate item.  This is used when constructing a case structure
 in a generate block.
*/
struct case_gitem_s {
  expression*     expr;              /*!< Pointer to case equality expression */
  gen_item*       gi;                /*!< Pointer to first generate item in case generate item */
  int             line;              /*!< Line number of case generate item */
  case_gitem*     prev;              /*!< Pointer to previous case generate item in list */
};

/*!
 A functional unit instance element in the functional unit instance tree.
*/
struct funit_inst_s {
  char*         name;                /*!< Instance name of this functional unit instance */
  func_unit*    funit;               /*!< Pointer to functional unit this instance represents */
  statistic*    stat;                /*!< Pointer to statistic holder */
  vector_width* range;               /*!< Used to create an array of instances */
  inst_parm*    param_head;          /*!< Head pointer to list of parameter overrides in this functional unit if it is a module */
  inst_parm*    param_tail;          /*!< Tail pointer to list of parameter overrides in this functional unit if it is a module */
  gitem_link*   gitem_head;          /*!< Head pointer to list of generate items used for this instance */
  gitem_link*   gitem_tail;          /*!< Tail pointer to list of generate items used for this instance */
  funit_inst*   parent;              /*!< Pointer to parent instance -- used for convenience only */
  funit_inst*   child_head;          /*!< Pointer to head of child list */
  funit_inst*   child_tail;          /*!< Pointer to tail of child list */
  funit_inst*   next;                /*!< Pointer to next child in parents list */
};

/*!
 Node for a tree that carries two strings:  a key and a value.  The tree is a binary
 tree that is sorted by key.
*/
struct tnode_s {
  char*  name;                       /*!< Key value for tree node */
  char*  value;                      /*!< Value of node */
  tnode* left;                       /*!< Pointer to left child node */
  tnode* right;                      /*!< Pointer to right child node */
  tnode* up;                         /*!< Pointer to parent node */
};

#ifdef HAVE_SYS_TIME_H
/*!
 Structure for holding code timing data.  This information can be useful for optimizing
 code segments.  To use a timer, create a pointer to a timer structure in global
 memory and assign the pointer to the value NULL.  Then simply call timer_start to
 start timing and timer_stop to stop timing.  You may call as many timer_start/timer_stop
 pairs as needed and the total timed time will be kept in the structure's total variable.
 To clear the total for a timer, call timer_clear.
*/
struct timer_s {
  struct timeval start;              /*!< Contains start time of a particular timer */
  time_t         total;              /*!< Contains the total amount of user time accrued for this timer */
};
#endif

/*!
 Contains information for an FSM state variable.
*/
struct fsm_var_s {
  char*       funit;                 /*!< Name of functional unit containing FSM variable */
  char*       name;                  /*!< Name associated with this FSM variable */
  expression* ivar;                  /*!< Pointer to input state expression */
  expression* ovar;                  /*!< Pointer to output state expression */
  vsignal*    iexp;                  /*!< Pointer to input signal matching ovar name */
  fsm*        table;                 /*!< Pointer to FSM containing signal from ovar */
  bool        exclude;               /*!< Set to TRUE if the associated FSM needs to be excluded from coverage consideration */
  fsm_var*    next;                  /*!< Pointer to next fsm_var element in list */
};

/*!
 Binding structure for FSM variables to their expressions.
*/
struct fv_bind_s {
  char*       sig_name;              /*!< Name of signal to bind to expression */
  expression* expr;                  /*!< Pointer to expression to bind to signal */
  char*       funit_name;            /*!< Name of functional unit to find sig_name and expression */
  statement*  stmt;                  /*!< Pointer to statement which contains root of expr */
  fv_bind*    next;                  /*!< Pointer to next FSM variable bind element in list */
};

/*!
 Container for a Verilog-2001 attribute.
*/
struct attr_param_s {
  char*       name;                  /*!< Name of attribute parameter identifier */
  expression* expr;                  /*!< Pointer to expression assigned to the attribute parameter identifier */
  int         index;                 /*!< Index position in the array that this parameter is located at */
  attr_param* next;                  /*!< Pointer to next attribute parameter in list */
  attr_param* prev;                  /*!< Pointer to previous attribute parameter in list */
};

/*!
 The stmt_blk structure contains extra information for a given signal that is only used during
 the parsing stage (therefore, the information does not need to be specified in the vsignal
 structure itself) and is used to keep track of information about that signal's use in the current
 functional unit.  It is used for race condition checking.
*/
struct stmt_blk_s {
  statement* stmt;                   /*!< Pointer to top-level statement in statement tree that this signal is first found in */
  bool       remove;                 /*!< Specifies if this statement block should be removed after checking is complete */
  bool       seq;                    /*!< If true, this statement block is considered to include sequential logic */
  bool       cmb;                    /*!< If true, this statement block is considered to include combinational logic */
  bool       bassign;                /*!< If true, at least one expression in statement block is a blocking assignment */
  bool       nassign;                /*!< If true, at least one expression in statement block is a non-blocking assignment */
};

/*!
 Simulator feature that keeps track of head pointer for a given thread along with pointers to the
 parent and children thread of this thread.  Threads allow us to handle task calls and fork/join
 statements.
*/
struct thread_s {
  func_unit* funit;                  /*!< Pointer to functional unit that this thread is running for */
  thread*    parent;                 /*!< Pointer to parent thread that spawned this thread */
  statement* curr;                   /*!< Pointer to current statement running in this thread */
  reentrant* ren;                    /*!< Pointer to re-entrant structure to use for this thread */
  union {
    uint8 all;
    struct {
      uint8 state      : 2;          /*!< Set to current state of this thread (see \ref thread_states for legal values) */
      uint8 kill       : 1;          /*!< Set to TRUE if this thread should be killed */
      uint8 exec_first : 1;          /*!< Set to TRUE when the first statement is being executed */
    } part;
  } suppl;
  unsigned   active_children;        /*!< Set to the number of children threads in active thread queue */
  thread*    queue_prev;             /*!< Pointer to previous thread in active/delayed queue */
  thread*    queue_next;             /*!< Pointer to next thread in active/delayed queue */
  thread*    all_prev;               /*!< Pointer to previous thread in all pool */
  thread*    all_next;               /*!< Pointer to next thread in all pool */
  sim_time   curr_time;              /*!< Set to the current simulation time for this thread */
};

/*!
 Linked list structure for a thread list.
*/
struct thr_link_s {
  thread*   thr;                     /*!< Pointer to thread */
  thr_link* next;                    /*!< Pointer to next thread link in list */
};

/*!
 Linked list structure for a thread list.
*/
struct thr_list_s {
  thr_link* head;                    /*!< Pointer to the head link of a thread list */
  thr_link* tail;                    /*!< Pointer to the tail link of a thread list */
  thr_link* next;                    /*!< Pointer to next thread link in list to use for newly added threads */
};

/*!
 Performance statistic container used for simulation-time performance characteristics.
*/
struct perf_stat_s {
  uint32  op_exec_cnt[EXP_OP_NUM];   /*!< Specifies the number of times that the associated operation was executed */
  float   op_cnt[EXP_OP_NUM];        /*!< Specifies the number of expressions containing the associated operation */
};

/*!
 Container for port-specific information.
*/
struct port_info_s {
  int           type;                /*!< Type of port (see \ref ssuppl_type for legal values) */
  bool          is_signed;           /*!< Set to TRUE if this port is signed */
  sig_range*    prange;              /*!< Contains packed range information for this port */
  sig_range*    urange;              /*!< Contains unpacked range information for this port */
};

/*!
 Container for holding information pertinent for parameter overriding.
*/
struct param_oride_s {
  char*        name;                 /*!< Specifies name of parameter being overridden (only valid for by_name syntax) */
  expression*  expr;                 /*!< Expression to override parameter with */
  param_oride* next;                 /*!< Pointer to next parameter override structure in list */
};

/*!
 Container holding a generate block item.
*/
struct gen_item_s {
  union {
    expression* expr;                /*!< Pointer to an expression */
    vsignal*    sig;                 /*!< Pointer to signal */
    statement*  stmt;                /*!< Pointer to statement */
    funit_inst* inst;                /*!< Pointer to instance */
  } elem;                            /*!< Union of various pointers this generate item is pointing at */
  union {
    uint32      all;                 /*!< Specifies the entire supplemental field */
    struct {
      uint32    type       : 3;      /*!< Specifies which element pointer is valid */
      uint32    conn_id    : 16;     /*!< Connection ID (used for connecting) */
      uint32    stop_true  : 1;      /*!< Specifies that we should stop traversing the true path */
      uint32    stop_false : 1;      /*!< Specifies that we should stop traversing the false path */
      uint32    resolved   : 1;      /*!< Specifies if this generate item has been resolved */
      uint32    removed    : 1;      /*!< Specifies if this generate item has been "removed" from the design */
    } part;
  } suppl;
  char*         varname;             /*!< Specifies genvar name (for TFN) or signal/TFN name to bind to (BIND) */
  gen_item*     next_true;           /*!< Pointer to the next generate item if expr is true */
  gen_item*     next_false;          /*!< Pointer to the next generate item if expr is false */
};

/*!
 Generate item link element.  Stores pointer to a generate item.
*/
struct gitem_link_s {
  gen_item*   gi;                    /*!< Pointer to a generate item */
  gitem_link* next;                  /*!< Pointer to next generate item element in list */
};

/*!
 Structure to hold information for a typedef'ed type.  This information is stored within a module but is
 not written to the CDD file.
*/
struct typedef_item_s {
  char*         name;                /*!< Name of this typedef */
  bool          is_signed;           /*!< Specifies if this typedef type is signed */
  bool          is_handled;          /*!< Specifies if this typedef type is handled by Covered */
  bool          is_sizeable;         /*!< Specifies if a range can be later placed on this typedef */
  sig_range*    prange;              /*!< Dimensional packed range information for this type */
  sig_range*    urange;              /*!< Dimensional unpacked range information for this type */
  typedef_item* next;                /*!< Pointer to next item in typedef list */
};

/*!
 Binds a signal to a static expression for an enumerated value.
*/
struct enum_item_s {
  vsignal*      sig;                 /*!< Enumerated value in the form of a signal */
  static_expr*  value;               /*!< Value to assign to enumerated value */
  bool          last;                /*!< Set to TRUE if this is the last enumerated value in the list */
  enum_item*    next;                /*!< Pointer to next enumeration item in the list */
};

/*!
 Represents a multi-dimensional memory structure in the design.
*/
struct sig_range_s {
  int           dim_num;             /*!< Specifies the number of dimensions in dim array */
  vector_width* dim;                 /*!< Pointer to array of unpacked or packed dimensions */
  bool          clear;               /*!< Set to TRUE when this range has been fully consumed */
};

/*!
 Represents a dimension in a multi-dimensional array/signal.
*/
struct dim_range_s {
  int        msb;                    /*!< MSB of range */
  int        lsb;                    /*!< LSB of range */
};

/*!
 Represents a reentrant stack and control information.
*/
struct reentrant_s {
  nibble*       data;                /*!< Packed bit data stored from signals */
  int           data_size;           /*!< Number of nibbles stored in a single rstack_entry data */
};

/*!
 Represents a SystemVerilog structure/union.
*/
struct struct_union_s {
  char*         name;                /*!< Name of this struct or union */
  int           type;                /*!< Specifies whether this is a struct, union or tagged union */
  bool          packed;              /*!< Specifies if the data in this struct/union should be handled in a packed or unpacked manner */
  bool          is_signed;           /*!< Specifies if the data in the struct/union should be handled as a signed value or not */
  bool          owns_data;           /*!< Specifies if this struct/union owns its vector data */
  int           tag_pos;             /*!< Specifies the current tag position */
  vector*       data;                /*!< Pointer to all data needed for this structure */
  su_member*    mem_head;            /*!< Pointer to head of struct/union member list */
  su_member*    mem_tail;            /*!< Pointer to tail of struct/union member list */
  struct_union* next;                /*!< Pointer to next struct/union in list */
};

/*!
 Represents a single structure/union member variable.
*/
struct su_member_s {
  int             type;              /*!< Type of struct/union member */
  int             pos;               /*!< Position of member in the list */
  union {
    vsignal*      sig;               /*!< Points to a signal */
    struct_union* su;                /*!< Points to a struct/union */
    enum_item*    ei;                /*!< Points to an enumerated item */
    typedef_item* tdi;               /*!< Points to a typedef'ed item */
  } elem;                            /*!< Member element pointer */
  su_member*      parent;            /*!< Pointer to parent struct/union member */
  su_member*      next;              /*!< Pointer to next struct/union member */
};

/*!
 Represents a profiling entry for a given function.
*/
struct profiler_s {
  char*  func_name;                  /*!< Name of function that this profiler is associated with */
#ifdef HAVE_SYS_TIME_H
  /*@null@*/ timer* time_in;         /*!< Time spent running this function */
#endif
  int    calls;                      /*!< Number of times this function has been called */
  int    mallocs;                    /*!< Number of malloc calls made in this function */
  int    frees;                      /*!< Number of free calls made in this function */
  bool   timed;                      /*!< Specifies if the function should be timed or not */
};

/*!
 Represents a single design database.
*/
struct db_s {
  char*       top_module;            /*!< Name of top-most module in the design */
  inst_link*  inst_head;             /*!< Pointer to head of instance tree list */
  inst_link*  inst_tail;             /*!< Pointer to tail of instance tree list */
  funit_link* funit_head;            /*!< Pointer to head of functional unit list */
  funit_link* funit_tail;            /*!< Pointer to tail of functional unit list */
};

/*!
 This will define the exception type that gets thrown (Covered does not care about this value)
*/
define_exception_type(int);

extern struct exception_context the_exception_context[1];

/*
 $Log$
 Revision 1.294.2.18  2008/05/28 22:12:31  phase1geo
 Adding further support for 32-/64-bit support.  Checkpointing.

 Revision 1.294.2.17  2008/05/28 05:57:10  phase1geo
 Updating code to use unsigned long instead of uint32.  Checkpointing.

 Revision 1.294.2.16  2008/05/23 14:50:21  phase1geo
 Optimizing vector_op_add and vector_op_subtract algorithms.  Also fixing issue with
 vector set bit.  Updating regressions per this change.

 Revision 1.294.2.15  2008/05/15 21:58:11  phase1geo
 Updating regression files per changes for increment and decrement operators.
 Checkpointing.

 Revision 1.294.2.14  2008/05/15 07:02:04  phase1geo
 Another attempt to fix static_afunc1 diagnostic failure.  Checkpointing.

 Revision 1.294.2.13  2008/05/13 07:23:26  phase1geo
 Fixing problem with parameterized multi-bit positive/negative select types.
 Full regression passes for IV and Cver.  Checkpointing.

 Revision 1.294.2.12  2008/05/13 06:42:23  phase1geo
 Finishing up initial pass of part-select code modifications.  Still getting an
 error in regression.  Checkpointing.

 Revision 1.294.2.11  2008/05/12 23:12:03  phase1geo
 Ripping apart part selection code and reworking it.  Things compile but are
 functionally quite broken at this point.  Checkpointing.

 Revision 1.294.2.10  2008/05/08 23:12:41  phase1geo
 Fixing several bugs and reworking code in arc to get FSM diagnostics
 to pass.  Checkpointing.

 Revision 1.294.2.9  2008/05/03 20:10:37  phase1geo
 Fixing some bugs, completing initial pass of vector_op_multiply and updating
 regression files accordingly.  Checkpointing.

 Revision 1.294.2.8  2008/05/02 22:06:10  phase1geo
 Updating arc code for new data structure.  This code is completely untested
 but does compile and has been completely rewritten.  Checkpointing.

 Revision 1.294.2.7  2008/04/30 23:12:31  phase1geo
 Fixing simulation issues.

 Revision 1.294.2.6  2008/04/23 23:06:03  phase1geo
 More bug fixes to vector functionality.  Bitwise operators appear to be
 working correctly when 2-state values are used.  Checkpointing.

 Revision 1.294.2.5  2008/04/20 05:43:45  phase1geo
 More work on the vector file.  Completed initial pass of conversion operations,
 bitwise operations and comparison operations.

 Revision 1.294.2.4  2008/04/18 22:04:15  phase1geo
 More work on vector functions for new data structure implementation.  Worked
 on vector_set_value, bit_fill and some checking functions.  Checkpointing.

 Revision 1.294.2.3  2008/04/17 23:16:08  phase1geo
 More work on vector.c.  Completed initial pass of vector_db_write/read and
 vector_copy/clone functionality.  Checkpointing.

 Revision 1.294.2.2  2008/04/16 22:29:58  phase1geo
 Finished format for new vector value format.  Completed work on vector_init_uint32
 and vector_create.

 Revision 1.294.2.1  2008/04/16 04:05:26  phase1geo
 Starting to modify defines.h for new vector data storage scheme.  Just changed
 some structures and added some newly needed structures at this point.

 Revision 1.294  2008/04/15 13:59:13  phase1geo
 Starting to add support for multiple databases.  Things compile but are
 quite broken at the moment.  Checkpointing.

 Revision 1.293  2008/04/08 22:45:10  phase1geo
 Optimizations for op-and-assign expressions.  This is an untested checkin
 at this point but it does compile cleanly.  Checkpointing.

 Revision 1.292  2008/04/08 05:26:33  phase1geo
 Second checkin of performance optimizations (regressions do not pass at this
 point).

 Revision 1.291  2008/04/07 23:14:00  phase1geo
 Beginnings of adding support for temporary vector storage for expressions.  Still
 work to go here before it is working.  Checkpointing.

 Revision 1.290  2008/04/07 19:35:41  phase1geo
 Incremented CDD version and updated regression files.  Also fixed issue
 with expression_dealloc function for FUNC_CALL operations.  Full regression
 passes.

 Revision 1.289  2008/03/31 21:40:23  phase1geo
 Fixing several more memory issues and optimizing a bit of code per regression
 failures.  Full regression still does not pass but does complete (yeah!)
 Checkpointing.

 Revision 1.288  2008/03/26 21:29:31  phase1geo
 Initial checkin of new optimizations for unknown and not_zero values in vectors.
 This attempts to speed up expression operations across the board.  Working on
 debugging regressions.  Checkpointing.

 Revision 1.287  2008/03/17 22:02:30  phase1geo
 Adding new check_mem script and adding output to perform memory checking during
 regression runs.  Completed work on free_safe and added realloc_safe function
 calls.  Regressions are pretty broke at the moment.  Checkpointing.

 Revision 1.286  2008/02/28 07:54:09  phase1geo
 Starting to add functionality for simulation optimization in the sim_expr_changed
 function (feature request 1897410).

 Revision 1.285  2008/02/28 03:53:17  phase1geo
 Code addition to support feature request 1902840.  Added race6 diagnostic and updated
 race5 diagnostics per this change.  For loop control assignments are now no longer
 considered when performing race condition checking.

 Revision 1.284  2008/02/27 05:26:51  phase1geo
 Adding support for $finish and $stop.

 Revision 1.283  2008/02/25 20:43:49  phase1geo
 Checking in code to allow the use of racecheck pragmas.  Added new tests to
 regression suite to verify this functionality.  Still need to document in
 User's Guide and manpage.

 Revision 1.282  2008/02/25 18:22:16  phase1geo
 Moved statement supplemental bits from root expression to statement and starting
 to add support for race condition checking pragmas (still some work left to do
 on this item).  Updated IV and Cver regressions per these changes.

 Revision 1.281  2008/02/08 23:58:06  phase1geo
 Starting to work on exception handling.  Much work to do here (things don't
 compile at the moment).

 Revision 1.280  2008/02/01 06:37:07  phase1geo
 Fixing bug in genprof.pl.  Added initial code for excluding final blocks and
 using pragma excludes (this code is not fully working yet).  More to be done.

 Revision 1.279  2008/01/18 05:03:14  phase1geo
 Fixing bug in CLI that didn't stop the CLI prompt at the right location.
 Added "goto" command to allow us to simply simulate to a specific timestep.
 Added status bar to indicate to the user how much we have gotten to our
 simulation goal for the CLI.

 Revision 1.278  2008/01/16 06:40:33  phase1geo
 More splint updates.

 Revision 1.277  2008/01/16 05:01:22  phase1geo
 Switched totals over from float types to int types for splint purposes.

 Revision 1.276  2008/01/07 05:01:57  phase1geo
 Cleaning up more splint errors.

 Revision 1.275  2007/12/31 23:43:36  phase1geo
 Fixing bug 1858408.  Also fixing issues with vector_op_add functionality.

 Revision 1.274  2007/12/20 04:47:50  phase1geo
 Fixing the last of the regression failures from previous changes.  Removing unnecessary
 output used for debugging.

 Revision 1.273  2007/12/19 22:54:35  phase1geo
 More compiler fixes (almost there now).  Checkpointing.

 Revision 1.272  2007/12/19 04:27:52  phase1geo
 More fixes for compiler errors (still more to go).  Checkpointing.

 Revision 1.271  2007/12/18 23:55:21  phase1geo
 Starting to remove 64-bit time and replacing it with a sim_time structure
 for performance enhancement purposes.  Also removing global variables for time-related
 information and passing this information around by reference for performance
 enhancement purposes.

 Revision 1.270  2007/12/12 23:36:57  phase1geo
 Optimized vector_op_add function significantly.  Other improvements made to
 profiler output.  Attempted to optimize the sim_simulation function although
 it hasn't had the intended effect and delay1.3 is currently failing.  Checkpointing
 for now.

 Revision 1.269  2007/12/12 07:23:19  phase1geo
 More work on profiling.  I have now included the ability to get function runtimes.
 Still more work to do but everything is currently working at the moment.

 Revision 1.268  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.267  2007/12/10 23:16:21  phase1geo
 Working on adding profiler for use in finding performance issues.  Things don't compile
 at the moment.

 Revision 1.266  2007/11/20 05:28:58  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.265  2007/09/18 21:41:54  phase1geo
 Removing inport indicator bit in vector and replacing with owns_data bit
 indicator.  Full regression passes.

 Revision 1.264  2007/09/17 13:32:58  phase1geo
 Adding some new structure members to support struct/union.

 Revision 1.263  2007/09/14 06:22:12  phase1geo
 Filling in existing functions in struct_union.  Completed parser code for handling
 struct/union declarations.  Code compiles thus far.

 Revision 1.262  2007/09/13 22:50:46  phase1geo
 Initial creation of struct_union files.  Added initial parsing ability for
 structs and unions (though I don't believe this is complete at this time).

 Revision 1.261  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.260  2007/07/29 03:32:06  phase1geo
 First attempt to make FUNC_CALL expressions copy the functional return value
 to the expression vector.  Not quite working yet -- checkpointing.

 Revision 1.259  2007/07/27 22:43:50  phase1geo
 Starting to add support for saving expression information for re-entrant
 tasks/functions.  Still more work to go.

 Revision 1.258  2007/07/26 22:23:00  phase1geo
 Starting to work on the functionality for automatic tasks/functions.  Just
 checkpointing some work.

 Revision 1.257  2007/07/26 20:12:45  phase1geo
 Fixing bug related to failure of hier1.1 diagnostic.  Placing functional unit
 scope in quotes for cases where backslashes are used in the scope names (requiring
 spaces in the names to escape the backslash).  Incrementing CDD version and
 regenerated all regression files.  Only atask1 is currently failing in regressions
 now.

 Revision 1.256  2007/07/16 18:39:59  phase1geo
 Finishing adding accumulated coverage output to report files.  Also fixed
 compiler warnings with static values in C code that are inputs to 64-bit
 variables.  Full regression was not run with these changes due to pre-existing
 simulator problems in core code.

 Revision 1.255  2007/04/18 22:34:58  phase1geo
 Revamping simulator core again.  Checkpointing.

 Revision 1.254  2007/04/13 21:47:12  phase1geo
 More simulation debugging.  Added 'display all_list' command to CLI to output
 the list of all threads.  Updated regressions though we are not fully passing
 at the moment.  Checkpointing.

 Revision 1.253  2007/04/09 22:47:52  phase1geo
 Starting to modify the simulation engine for performance purposes.  Code is
 not complete and is untested at this point.

 Revision 1.252  2007/03/30 22:43:13  phase1geo
 Regression fixes.  Still have a ways to go but we are getting close.

 Revision 1.251  2007/03/16 21:41:08  phase1geo
 Checkpointing some work in fixing regressions for unnamed scope additions.
 Getting closer but still need to properly handle the removal of functional units.

 Revision 1.250  2006/12/18 23:58:34  phase1geo
 Fixes for automatic tasks.  Added atask1 diagnostic to regression suite to verify.
 Other fixes to parser for blocks.  We need to add code to properly handle unnamed
 scopes now before regressions will get to a passing state.  Checkpointing.

 Revision 1.249  2006/12/15 17:33:45  phase1geo
 Updating TODO list.  Fixing more problems associated with handling re-entrant
 tasks/functions.  Still not quite there yet for simulation, but we are getting
 quite close now.  Checkpointing.

 Revision 1.248  2006/12/14 23:46:57  phase1geo
 Fixing remaining compile issues with support for functional unit pointers in
 expressions and unnamed scope handling.  Starting to debug run-time issues now.
 Added atask1 diagnostic to begin this verification process.  Checkpointing.

 Revision 1.247  2006/12/14 04:19:35  phase1geo
 More updates to parser and associated code to handle unnamed scopes and
 fixing more code to use functional unit pointers in expressions instead of
 statement pointers.  Still not fully compiling at this point.  Checkpointing.

 Revision 1.246  2006/12/12 06:20:22  phase1geo
 More updates to support re-entrant tasks/functions.  Still working through
 compiler errors.  Checkpointing.

 Revision 1.245  2006/12/11 23:29:16  phase1geo
 Starting to add support for re-entrant tasks and functions.  Currently, compiling
 fails.  Checkpointing.

 Revision 1.244  2006/11/29 23:15:46  phase1geo
 Major overhaul to simulation engine by including an appropriate delay queue
 mechanism to handle simulation timing for delay operations.  Regression not
 fully passing at this moment but enough is working to checkpoint this work.

 Revision 1.243  2006/11/25 04:24:39  phase1geo
 Adding initial code to fully support the timescale directive and its usage.
 Added -vpi_ts score option to allow the user to specify a top-level timescale
 value for the generated VPI file (this code has not been tested at this point,
 however).  Also updated diagnostic Makefile to get the VPI shared object files
 from the current lib directory (instead of the checked in one).

 Revision 1.242  2006/11/24 05:30:15  phase1geo
 Checking in fix for proper handling of delays.  This does not include the use
 of timescales (which will be fixed next).  Full IV regression now passes.

 Revision 1.241  2006/11/22 20:20:01  phase1geo
 Updates to properly support 64-bit time.  Also starting to make changes to
 simulator to support "back simulation" for when the current simulation time
 has advanced out quite a bit of time and the simulator needs to catch up.
 This last feature is not quite working at the moment and regressions are
 currently broken.  Checkpointing.

 Revision 1.240  2006/11/21 19:54:13  phase1geo
 Making modifications to defines.h to help in creating appropriately sized types.
 Other changes to VPI code (but this is still broken at the moment).  Checkpointing.

 Revision 1.239  2006/10/16 21:34:46  phase1geo
 Increased max bit width from 1024 to 65536 to allow for more room for memories.
 Fixed issue with enumerated values being explicitly assigned unknown values and
 created error output message when an implicitly assigned enum followed an explicitly
 assign unknown enum value.  Fixed problem with generate blocks in different
 instantiations of the same module.  Fixed bug in parser related to setting the
 curr_packed global variable correctly.  Added enum2 and enum2.1 diagnostics to test
 suite to verify correct enumerate behavior for the changes made in this checkin.
 Full regression now passes.

 Revision 1.238  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.237  2006/10/06 17:18:13  phase1geo
 Adding support for the final block type.  Added final1 diagnostic to regression
 suite.  Full regression passes.

 Revision 1.236  2006/10/05 21:43:17  phase1geo
 Added support for increment and decrement operators in expressions.  Also added
 proper parsing and handling support for immediate and postponed increment/decrement.
 Added inc3, inc3.1, dec3 and dec3.1 diagnostics to verify this new functionality.
 Still need to run regressions.

 Revision 1.235  2006/09/25 22:22:28  phase1geo
 Adding more support for memory reporting to both score and report commands.
 We are getting closer; however, regressions are currently broken.  Checkpointing.

 Revision 1.234  2006/09/25 04:15:03  phase1geo
 Starting to add support for new memory coverage metric.  This includes changes
 for the report command only at this point.

 Revision 1.233  2006/09/22 19:56:45  phase1geo
 Final set of fixes and regression updates per recent changes.  Full regression
 now passes.

 Revision 1.232  2006/09/20 22:38:09  phase1geo
 Lots of changes to support memories and multi-dimensional arrays.  We still have
 issues with endianness and VCS regressions have not been run, but this is a significant
 amount of work that needs to be checkpointed.

 Revision 1.231  2006/09/15 22:14:54  phase1geo
 Working on adding arrayed signals.  This is currently in progress and doesn't
 even compile at this point, much less work.  Checkpointing work.

 Revision 1.230  2006/09/12 22:24:42  phase1geo
 Added first attempt at collecting bitwise coverage information during simulation.
 Updated regressions for this change; however, we currently do not report on this
 information currently.  Also added missing keywords to GUI Verilog highlighter.
 Checkpointing.

 Revision 1.229  2006/09/11 22:27:55  phase1geo
 Starting to work on supporting bitwise coverage.  Moving bits around in supplemental
 fields to allow this to work.  Full regression has been updated for the current changes
 though this feature is still not fully implemented at this time.  Also added parsing support
 for SystemVerilog program blocks (ignored) and final blocks (not handled properly at this
 time).  Also added lexer support for the return, void, continue, break, final, program and
 endprogram SystemVerilog keywords.  Checkpointing work.

 Revision 1.228  2006/09/05 21:00:44  phase1geo
 Fixing bug in removing statements that are generate items.  Also added parsing
 support for multi-dimensional array accessing (no functionality here to support
 these, however).  Fixing bug in race condition checker for generated items.
 Currently hitting into problem with genvars used in SBIT_SEL or MBIT_SEL type
 expressions -- we are hitting into an assertion error in expression_operate_recursively.

 Revision 1.227  2006/09/01 23:06:02  phase1geo
 Fixing regressions per latest round of changes.  Full regression now passes.

 Revision 1.226  2006/09/01 04:06:36  phase1geo
 Added code to support more than one instance tree.  Currently, I am seeing
 quite a few memory errors that are causing some major problems at the moment.
 Checkpointing.

 Revision 1.225  2006/08/29 22:49:31  phase1geo
 Added enumeration support and partial support for typedefs.  Added enum1
 diagnostic to verify initial enumeration support.  Full regression has not
 been run at this point -- checkpointing.

 Revision 1.224  2006/08/28 22:28:28  phase1geo
 Fixing bug 1546059 to match stable branch.  Adding support for repeated delay
 expressions (i.e., a = repeat(2) @(b) c).  Fixing support for event delayed
 assignments (i.e., a = @(b) c).  Adding several new diagnostics to verify this
 new level of support and updating regressions for these changes.  Also added
 parser support for logic port types.

 Revision 1.223  2006/08/25 18:25:24  phase1geo
 Modified gen39 and gen40 to not use the Verilog-2001 port syntax.  Fixed problem
 with detecting implicit .name and .* syntax.  Fixed op-and-assign report output.
 Added support for 'typedef', 'struct', 'union' and 'enum' syntax for SystemVerilog.
 Updated user documentation.  Full regression completely passes now.

 Revision 1.222  2006/08/21 22:50:00  phase1geo
 Adding more support for delayed assignments.  Added dly_assign1 to testsuite
 to verify the #... type of delayed assignment.  This seems to be working for
 this case but has a potential issue in the report generation.  Checkpointing
 work.

 Revision 1.221  2006/08/20 03:21:00  phase1geo
 Adding support for +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, <<<=, >>>=, ++
 and -- operators.  The op-and-assign operators are currently good for
 simulation and code generation purposes but still need work in the comb.c
 file for proper combinational logic underline and reporting support.  The
 increment and decrement operations should be fully supported with the exception
 of their use in FOR loops (I'm not sure if this is supported by SystemVerilog
 or not yet).  Also started adding support for delayed assignments; however, I
 need to rework this completely as it currently causes segfaults.  Added lots of
 new diagnostics to verify this new functionality and updated regression for
 these changes.  Full IV regression now passes.

 Revision 1.220  2006/08/18 22:41:22  phase1geo
 Adding enumerated values for the operate-and-assign SystemVerilog operations.

 Revision 1.219  2006/08/16 15:32:14  phase1geo
 Fixing issues with do..while loop handling.  Full regression now passes.

 Revision 1.218  2006/08/11 18:57:03  phase1geo
 Adding support for always_comb, always_latch and always_ff statement block
 types.  Added several diagnostics to regression suite to verify this new
 behavior.

 Revision 1.217  2006/08/11 15:16:49  phase1geo
 Joining slist3.3 diagnostic to latest development branch.  Adding changes to
 fix memory issues from bug 1535412.

 Revision 1.216  2006/08/10 22:35:14  phase1geo
 Updating with fixes for upcoming 0.4.7 stable release.  Updated regressions
 for this change.  Full regression still fails due to an unrelated issue.

 Revision 1.215  2006/08/02 22:28:31  phase1geo
 Attempting to fix the bug pulled out by generate11.v.  We are just having an issue
 with setting the assigned bit in a signal expression that contains a hierarchical reference
 using a genvar reference.  Adding generate11.1 diagnostic to verify a slightly different
 syntax style for the same code.  Note sure how badly I broke regression at this point.

 Revision 1.214  2006/08/01 04:38:20  phase1geo
 Fixing issues with binding to non-module scope and not binding references
 that reference a "no score" module.  Full regression passes.

 Revision 1.213  2006/07/29 20:53:43  phase1geo
 Fixing some code related to generate statements; however, generate8.1 is still
 not completely working at this point.  Full regression passes for IV.

 Revision 1.212  2006/07/27 18:02:22  phase1geo
 More diagnostic additions and upgraded the generate item display functionality
 for better debugging using this feature.  We are about to forge into some new
 territory with regard to generate blocks, so we will tag at this point.

 Revision 1.211  2006/07/27 16:08:46  phase1geo
 Fixing several memory leak bugs, cleaning up output and fixing regression
 bugs.  Full regression now passes (including all current generate diagnostics).

 Revision 1.210  2006/07/27 04:34:39  phase1geo
 Adding initial support for generate case statements.  This has only been
 verified to compile at this point.

 Revision 1.209  2006/07/24 22:20:23  phase1geo
 Things are quite hosed at the moment -- trying to come up with a scheme to
 handle embedded hierarchy in generate blocks.  Chances are that a lot of
 things are currently broken at the moment.

 Revision 1.208  2006/07/21 05:47:42  phase1geo
 More code additions for generate functionality.  At this point, we seem to
 be creating proper generate item blocks and are creating the generate loop
 namespace appropriately.  However, the binder is still unable to find a signal
 created by a generate block.

 Revision 1.207  2006/07/20 20:11:09  phase1geo
 More work on generate statements.  Trying to figure out a methodology for
 handling namespaces.  Still a lot of work to go...

 Revision 1.206  2006/07/20 04:55:18  phase1geo
 More updates to support generate blocks.  We seem to be passing the parser
 stage now.  Getting segfaults in the generate_resolve code, presently.

 Revision 1.205  2006/07/17 22:12:42  phase1geo
 Adding more code for generate block support.  Still just adding code at this
 point -- hopefully I haven't broke anything that doesn't use generate blocks.

 Revision 1.204  2006/07/13 05:31:52  phase1geo
 Adding -g option to score command parser/usage information.  Still a lot of
 work to go before this feature is complete.

 Revision 1.203  2006/07/10 03:05:04  phase1geo
 Contains bug fixes for memory leaks and segmentation faults.  Also contains
 some starting code to support generate blocks.  There is absolutely no
 functionality here, however.

 Revision 1.202  2006/07/09 01:40:39  phase1geo
 Removing the vpi directory (again).  Also fixing a bug in Covered's expression
 deallocator where a case statement contains an unbindable signal.  Previously
 the case test expression was being deallocated twice.  This submission fixes
 this bug (bug was also fixed in the 0.4.5 stable release).  Added new tests
 to verify fix.  Full regression passes.

 Revision 1.201  2006/07/08 02:06:53  phase1geo
 Updating build scripts for next development release and fixing a bug in
 the score command that caused segfaults for signals that used the same
 parameters in their range declaration.  Added diagnostic to regression
 to test this fix.  Full regression passes.

 Revision 1.200  2006/06/29 20:57:24  phase1geo
 Added stmt_excluded bit to expression to allow us to individually control line
 and combinational logic exclusion.  This also allows us to exclude combinational
 logic within excluded lines.  Also fixing problem with highlighting the listbox
 (due to recent changes).

 Revision 1.199  2006/06/29 20:06:33  phase1geo
 Adding assertion exclusion code.  Things seem to be working properly with this
 now.  This concludes the initial version of code exclusion.  There are some
 things to clean up (and maybe make better looking).

 Revision 1.198  2006/06/29 04:26:02  phase1geo
 More updates for FSM coverage.  We are getting close but are just not to fully
 working order yet.

 Revision 1.197  2006/06/23 19:45:26  phase1geo
 Adding full C support for excluding/including coverage points.  Fixed regression
 suite failures -- full regression now passes.  We just need to start adding support
 to the Tcl/Tk files for full user-specified exclusion support.

 Revision 1.196  2006/06/22 21:56:21  phase1geo
 Adding excluded bits to signal and arc structures and changed statistic gathering
 functions to not gather coverage for excluded structures.  Started to work on
 exclude.c file which will quickly adjust coverage information from GUI modifications.
 Regression has been updated for this change and it fully passes.

 Revision 1.195  2006/06/21 19:44:45  phase1geo
 Adding exclusion bit to expression supplemental field and incrementing
 CDD version number as a result.  No functionality to support excluding
 has been added at this point.  Full regression now passes.

 Revision 1.194  2006/05/28 02:43:49  phase1geo
 Integrating stable release 0.4.4 changes into main branch.  Updated regressions
 appropriately.

 Revision 1.193  2006/05/25 12:11:00  phase1geo
 Including bug fix from 0.4.4 stable release and updating regressions.

 Revision 1.192  2006/05/02 21:49:41  phase1geo
 Updating regression files -- all but three diagnostics pass (due to known problems).
 Added SCORE_ARGS line type to CDD format which stores the directory that the score
 command was executed from as well as the command-line arguments to the score
 command.

 Revision 1.191  2006/04/21 06:14:45  phase1geo
 Merged in changes from 0.4.3 stable release.  Updated all regression files
 for inclusion of OVL library.  More documentation updates for next development
 release (but there is more to go here).

 Revision 1.190  2006/04/18 21:59:54  phase1geo
 Adding support for environment variable substitution in configuration files passed
 to the score command.  Adding ovl.c/ovl.h files.  Working on support for assertion
 coverage in report command.  Still have a bit to go here yet.

 Revision 1.189  2006/04/14 17:05:13  phase1geo
 Reorganizing info line to make it more succinct and easier for future needs.
 Fixed problems with VPI library with recent merge changes.  Regression has
 been completely updated for these changes.

 Revision 1.188  2006/04/13 21:04:24  phase1geo
 Adding NOOP expression and allowing $display system calls to not cause its
 statement block to be excluded from coverage.  Updating regressions which fully
 pass.

 Revision 1.187  2006/04/13 14:59:23  phase1geo
 Updating CDD version from 6 to 7 due to changes in the merge facility.  Full
 regression now passes.

 Revision 1.186  2006/04/11 22:42:16  phase1geo
 First pass at adding multi-file merging.  Still need quite a bit of work here yet.

 Revision 1.185.4.1.4.1  2006/05/25 10:59:35  phase1geo
 Adding bug fix for hierarchically referencing parameters.  Added param13 and
 param13.1 diagnostics to verify this functionality.  Updated regressions.

 Revision 1.185.4.1  2006/04/20 21:55:16  phase1geo
 Adding support for big endian signals.  Added new endian1 diagnostic to regression
 suite to verify this new functionality.  Full regression passes.  We may want to do
 some more testing on variants of this before calling it ready for stable release 0.4.3.

 Revision 1.185  2006/03/28 22:28:27  phase1geo
 Updates to user guide and added copyright information to each source file in the
 src directory.  Added test directory in user documentation directory containing the
 example used in line, toggle, combinational logic and FSM descriptions.

 Revision 1.184  2006/03/27 23:25:30  phase1geo
 Updating development documentation for 0.4 stable release.

 Revision 1.183  2006/03/24 19:01:44  phase1geo
 Changed two variable report output to be as concise as possible.  Updating
 regressions per these changes.

 Revision 1.182  2006/03/23 22:42:54  phase1geo
 Changed two variable combinational expressions that have a constant value in either the
 left or right expression tree to unary expressions.  Removed expressions containing only
 static values from coverage totals.  Fixed bug in stmt_iter_get_next_in_order for looping
 cases (the verbose output was not being emitted for these cases).  Updated regressions for
 these changes -- full regression passes.

 Revision 1.181  2006/03/22 22:00:43  phase1geo
 Fixing bug in missed combinational logic determination where a static expression
 on the left/right of a combination expression should cause the entire expression to
 be considered fully covered.  Regressions have not been run which may contain some
 miscompares due to this change.

 Revision 1.180  2006/02/16 21:19:26  phase1geo
 Adding support for arrays of instances.  Also fixing some memory problems for
 constant functions and fixed binding problems when hierarchical references are
 made to merged modules.  Full regression now passes.

 Revision 1.179  2006/02/10 16:44:28  phase1geo
 Adding support for register assignment.  Added diagnostic to regression suite
 to verify its implementation.  Updated TODO.  Full regression passes at this
 point.

 Revision 1.178  2006/02/03 23:49:38  phase1geo
 More fixes to support signed comparison and propagation.  Still more testing
 to do here before I call it good.  Regression may fail at this point.

 Revision 1.177  2006/02/02 22:37:40  phase1geo
 Starting to put in support for signed values and inline register initialization.
 Also added support for more attribute locations in code.  Regression updated for
 these changes.  Interestingly, with the changes that were made to the parser,
 signals are output to reports in order (before they were completely reversed).
 This is a nice surprise...  Full regression passes.

 Revision 1.176  2006/02/01 19:58:28  phase1geo
 More updates to allow parsing of various parameter formats.  At this point
 I believe full parameter support is functional.  Regression has been updated
 which now completely passes.  A few new diagnostics have been added to the
 testsuite to verify additional functionality that is supported.

 Revision 1.175  2006/02/01 15:13:11  phase1geo
 Added support for handling bit selections in RHS parameter calculations.  New
 mbit_sel5.4 diagnostic added to verify this change.  Added the start of a
 regression utility that will eventually replace the old Makefile system.

 Revision 1.174  2006/01/31 16:41:00  phase1geo
 Adding initial support and diagnostics for the variable multi-bit select
 operators +: and -:.  More to come but full regression passes.

 Revision 1.173  2006/01/25 22:13:46  phase1geo
 Adding LXT-style dumpfile parsing support.  Everything is wired in but I still
 need to look at a problem at the end of the dumpfile -- I'm getting coredumps
 when using the new -lxt option.  I also need to disable LXT code if the z
 library is missing along with documenting the new feature in the user's guide
 and man page.

 Revision 1.172  2006/01/25 16:51:27  phase1geo
 Fixing performance/output issue with hierarchical references.  Added support
 for hierarchical references to parser.  Full regression passes.

 Revision 1.171  2006/01/24 23:24:37  phase1geo
 More updates to handle static functions properly.  I have redone quite a bit
 of code here which has regressions pretty broke at the moment.  More work
 to do but I'm checkpointing.

 Revision 1.170  2006/01/23 03:53:29  phase1geo
 Adding support for input/output ports of tasks/functions.  Regressions are not
 running cleanly at this point so there is still some work to do here.  Checkpointing.

 Revision 1.169  2006/01/19 23:10:38  phase1geo
 Adding line and starting column information to vsignal structure (and associated CDD
 files).  Regression has been fully updated for this change which now fully passes.  Final
 changes to summary GUI.  Fixed signal underlining for toggle coverage to work for both
 explicit and implicit signals.  Getting things ready for a preferences window.

 Revision 1.168  2006/01/16 17:27:41  phase1geo
 Fixing binding issues when designs have modules/tasks/functions that are either used
 more than once in a design or have the same name.  Full regression now passes.

 Revision 1.167  2006/01/13 04:01:04  phase1geo
 Adding support for exponential operation.  Added exponent1 diagnostic to verify
 but Icarus does not support this currently.

 Revision 1.166  2006/01/12 22:53:01  phase1geo
 Adding support for localparam construct.  Added tests to regression suite to
 verify correct functionality.  Full regression passes.

 Revision 1.165  2006/01/12 22:14:45  phase1geo
 Completed code for handling parameter value pass by name Verilog-2001 syntax.
 Added diagnostics to regression suite and updated regression files for this
 change.  Full regression now passes.

 Revision 1.164  2006/01/11 19:57:10  phase1geo
 Added diagnostics to verify proper handling of inline parameters, inline port
 declarations and added is_signed bit to vector supplemental field (though we still
 do not handle signed values appropriately).

 Revision 1.163  2006/01/10 23:13:50  phase1geo
 Completed support for implicit event sensitivity list.  Added diagnostics to verify
 this new capability.  Also started support for parsing inline parameters and port
 declarations (though this is probably not complete and not passing at this point).
 Checkpointing.

 Revision 1.162  2006/01/10 05:56:36  phase1geo
 In the middle of adding support for event sensitivity lists to score command.
 Regressions should pass but this code is not complete at this time.

 Revision 1.161  2006/01/10 05:12:48  phase1geo
 Added arithmetic left and right shift operators.  Added ashift1 diagnostic
 to verify their correct operation.

 Revision 1.160  2006/01/09 04:15:25  phase1geo
 Attempting to fix one last problem with latest changes.  Regression runs are
 currently running.  Checkpointing.

 Revision 1.159  2006/01/06 23:39:10  phase1geo
 Started working on removing the need to simulate more than is necessary.  Things
 are pretty broken at this point, but all of the code should be in -- debugging.

 Revision 1.158  2006/01/06 18:54:03  phase1geo
 Breaking up expression_operate function into individual functions for each
 expression operation.  Also storing additional information in a globally accessible,
 constant structure array to increase performance.  Updating full regression for these
 changes.  Full regression passes.

 Revision 1.157  2006/01/05 05:52:06  phase1geo
 Removing wait bit in vector supplemental field and modifying algorithm to only
 assign in the post-sim location (pre-sim now is gone).  This fixes some issues
 with simulation results and increases performance a bit.  Updated regressions
 for these changes.  Full regression passes.

 Revision 1.156  2006/01/02 21:35:36  phase1geo
 Added simulation performance statistical information to end of score command
 when we are in debug mode.

 Revision 1.155  2005/12/31 05:00:57  phase1geo
 Updating regression due to recent changes in adding exec_num field in expression
 and removing the executed bit in the expression supplemental field.  This will eventually
 allow us to get information on where the simulator is spending the most time.

 Revision 1.154  2005/12/16 23:09:15  phase1geo
 More updates to remove memory leaks.  Full regression passes.

 Revision 1.153  2005/12/16 06:25:15  phase1geo
 Fixing lshift/rshift operations to avoid reading unallocated memory.  Updated
 assign1.cdd file.

 Revision 1.152  2005/12/12 23:25:37  phase1geo
 Fixing memory faults.  This is a work in progress.

 Revision 1.151  2005/12/10 06:41:18  phase1geo
 Added support for FOR loops and added diagnostics to regression suite to verify
 functionality.  Fixed statement deallocation function (removed a bunch of code
 there now that statement stopping is working as intended).  Full regression passes.

 Revision 1.150  2005/12/08 22:50:59  phase1geo
 Adding support for while loops.  Added while1 and while1.1 to regression suite.
 Ran VCS on regression suite and updated.  Full regression passes.

 Revision 1.149  2005/12/08 19:47:00  phase1geo
 Fixed repeat2 simulation issues.  Fixed statement_connect algorithm, removed the
 need for a separate set_stop function and reshuffled the positions of esuppl bits.
 Full regression passes.

 Revision 1.148  2005/12/07 21:50:50  phase1geo
 Added support for repeat blocks.  Added repeat1 to regression and fixed errors.
 Full regression passes.

 Revision 1.147  2005/12/05 22:45:38  phase1geo
 Bug fixes to disable code when disabling ourselves -- we move thread killing
 to be done by the sim_thread routine only.

 Revision 1.146  2005/12/05 22:02:24  phase1geo
 Added initial support for disable expression.  Added test to verify functionality.
 Full regression passes.

 Revision 1.145  2005/12/05 20:26:55  phase1geo
 Fixing bugs in code to remove statement blocks that are pointed to by expressions
 in NB_CALL and FORK cases.  Fixed bugs in fork code -- this is now working at the
 moment.  Updated regressions which now fully pass.

 Revision 1.144  2005/12/02 19:58:36  phase1geo
 Added initial support for FORK/JOIN expressions.  Code is not working correctly
 yet as we need to determine if a statement should be done in parallel or not.

 Revision 1.143  2005/12/01 20:49:02  phase1geo
 Adding nested_block3 to verify nested named blocks in tasks.  Fixed named block
 usage to be FUNC_CALL or TASK_CALL -like based on its placement.

 Revision 1.142  2005/12/01 18:35:17  phase1geo
 Fixing bug where functions in continuous assignments could cause the
 assignment to constantly be reevaluated (infinite looping).  Added new nested_block2
 diagnostic to verify nested named blocks in functions.  Also verifies that nested
 named blocks can call functions in the same module.  Also modified NB_CALL expressions
 to act like functions (no context switching involved).  Full regression passes.

 Revision 1.141  2005/12/01 16:08:19  phase1geo
 Allowing nested functional units within a module to get parsed and handled correctly.
 Added new nested_block1 diagnostic to test nested named blocks -- will add more tests
 later for different combinations.  Updated regression suite which now passes.

 Revision 1.140  2005/11/30 18:25:56  phase1geo
 Fixing named block code.  Full regression now passes.  Still more work to do on
 named blocks, however.

 Revision 1.139  2005/11/29 23:14:37  phase1geo
 Adding support for named blocks.  Still not working at this point but checkpointing
 anyways.  Added new task3.1 diagnostic to verify task removal when a task is calling
 another task.

 Revision 1.138  2005/11/29 19:04:47  phase1geo
 Adding tests to verify task functionality.  Updating failing tests and fixed
 bugs for context switch expressions at the end of a statement block, statement
 block removal for missing function/tasks and thread killing.

 Revision 1.137  2005/11/28 23:28:47  phase1geo
 Checkpointing with additions for threads.

 Revision 1.136  2005/11/25 16:48:48  phase1geo
 Fixing bugs in binding algorithm.  Full regression now passes.

 Revision 1.135  2005/11/23 23:05:24  phase1geo
 Updating regression files.  Full regression now passes.

 Revision 1.134  2005/11/22 23:03:48  phase1geo
 Adding support for event trigger mechanism.  Regression is currently broke
 due to these changes -- we need to remove statement blocks that contain
 triggers that are not simulated.

 Revision 1.133  2005/11/22 16:46:27  phase1geo
 Fixed bug with clearing the assigned bit in the binding phase.  Full regression
 now runs cleanly.

 Revision 1.132  2005/11/22 05:30:33  phase1geo
 Updates to regression suite for clearing the assigned bit when a statement
 block is removed from coverage consideration and it is assigning that signal.
 This is not fully working at this point.

 Revision 1.131  2005/11/21 04:17:43  phase1geo
 More updates to regression suite -- includes several bug fixes.  Also added --enable-debug
 facility to configuration file which will include or exclude debugging output from being
 generated.

 Revision 1.130  2005/11/18 23:52:55  phase1geo
 More regression cleanup -- still quite a few errors to handle here.

 Revision 1.129  2005/11/17 23:35:16  phase1geo
 Blocking assignment is now working properly along with support for event expressions
 (currently only the original PEDGE, NEDGE, AEDGE and DELAY are supported but more
 can now follow).  Added new race4 diagnostic to verify that a register cannot be
 assigned from more than one location -- this works.  Regression fails at this point.

 Revision 1.128  2005/11/16 22:01:51  phase1geo
 Fixing more problems related to simulation of function/task calls.  Regression
 runs are now running without errors.

 Revision 1.127  2005/11/15 23:08:02  phase1geo
 Updates for new binding scheme.  Binding occurs for all expressions, signals,
 FSMs, and functional units after parsing has completed or after database reading
 has been completed.  This should allow for any hierarchical reference or scope
 issues to be handled correctly.  Regression mostly passes but there are still
 a few failures at this point.  Checkpointing.

 Revision 1.126  2005/11/11 23:29:12  phase1geo
 Checkpointing some work in progress.  This will cause compile errors.  In
 the process of moving db read expression signal binding from vsignal output to
 expression output so that we can just call the binder in the expression_db_read
 function.

 Revision 1.125  2005/11/10 23:27:37  phase1geo
 Adding scope files to handle scope searching.  The functions are complete (not
 debugged) but are not as of yet used anywhere in the code.  Added new func2 diagnostic
 which brings out scoping issues for functions.

 Revision 1.124  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.123  2005/05/09 03:08:34  phase1geo
 Intermediate checkin for VPI changes.  Also contains parser fix which should
 be branch applied to the latest stable and development versions.

 Revision 1.122  2005/02/08 23:18:23  phase1geo
 Starting to add code to handle expression assignment for blocking assignments.
 At this point, regressions will probably still pass but new code isn't doing exactly
 what I want.

 Revision 1.121  2005/02/05 04:13:29  phase1geo
 Started to add reporting capabilities for race condition information.  Modified
 race condition reason calculation and handling.  Ran -Wall on all code and cleaned
 things up.  Cleaned up regression as a result of these changes.  Full regression
 now passes.

 Revision 1.120  2005/02/04 23:55:48  phase1geo
 Adding code to support race condition information in CDD files.  All code is
 now in place for writing/reading this data to/from the CDD file (although
 nothing is currently done with it and it is currently untested).

 Revision 1.119  2005/02/01 05:11:18  phase1geo
 Updates to race condition checker to find blocking/non-blocking assignments in
 statement block.  Regression still runs clean.

 Revision 1.118  2005/01/27 13:33:49  phase1geo
 Added code to calculate if statement block is sequential, combinational, both
 or none.

 Revision 1.117  2005/01/10 23:03:39  phase1geo
 Added code to properly report race conditions.  Added code to remove statement blocks
 from module when race conditions are found.

 Revision 1.116  2005/01/10 02:59:29  phase1geo
 Code added for race condition checking that checks for signals being assigned
 in multiple statements.  Working on handling bit selects -- this is in progress.

 Revision 1.115  2005/01/07 23:00:09  phase1geo
 Regression now passes for previous changes.  Also added ability to properly
 convert quoted strings to vectors and vectors to quoted strings.  This will
 allow us to support strings in expressions.  This is a known good.

 Revision 1.114  2005/01/07 17:59:51  phase1geo
 Finalized updates for supplemental field changes.  Everything compiles and links
 correctly at this time; however, a regression run has not confirmed the changes.

 Revision 1.113  2005/01/06 23:51:16  phase1geo
 Intermediate checkin.  Files don't fully compile yet.

 Revision 1.112  2005/01/06 14:32:24  phase1geo
 Starting to make updates to supplemental fields.  Work is in progress.

 Revision 1.111  2005/01/04 14:37:00  phase1geo
 New changes for race condition checking.  Things are uncompilable at this
 point.

 Revision 1.110  2004/12/17 14:27:46  phase1geo
 More code added to race condition checker.  This is in an unusable state at
 this time.

 Revision 1.109  2004/12/16 13:52:58  phase1geo
 Starting to add support for race-condition detection and handling.

 Revision 1.108  2004/11/06 13:22:48  phase1geo
 Updating CDD files for change where EVAL_T and EVAL_F bits are now being masked out
 of the CDD files.

 Revision 1.107  2004/04/19 04:54:55  phase1geo
 Adding first and last column information to expression and related code.  This is
 not working correctly yet.

 Revision 1.106  2004/04/17 14:07:55  phase1geo
 Adding replace and merge options to file menu.

 Revision 1.105  2004/03/30 15:42:14  phase1geo
 Renaming signal type to vsignal type to eliminate compilation problems on systems
 that contain a signal type in the OS.

 Revision 1.104  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.103  2004/03/15 21:38:17  phase1geo
 Updated source files after running lint on these files.  Full regression
 still passes at this point.

 Revision 1.102  2004/01/30 06:04:43  phase1geo
 More report output format tweaks.  Adjusted lines and spaces to make things
 look more organized.  Still some more to go.  Regression will fail at this
 point.

 Revision 1.101  2004/01/25 03:41:48  phase1geo
 Fixes bugs in summary information not matching verbose information.  Also fixes
 bugs where instances were output when no logic was missing, where instance
 children were missing but not output.  Changed code to output summary
 information on a per instance basis (where children instances are not merged
 into parent instance summary information).  Updated regressions as a result.
 Updates to user documentation (though this is not complete at this time).

 Revision 1.100  2004/01/21 22:26:56  phase1geo
 Changed default CDD file name from "cov.db" to "cov.cdd".  Changed instance
 statistic gathering from a child merging algorithm to just calculating
 instance coverage for the individual instances.  Updated full regression for
 this change and updated VCS regression for all past changes of this release.

 Revision 1.99  2004/01/16 23:05:00  phase1geo
 Removing SET bit from being written to CDD files.  This value is meaningless
 after scoring has completed and sometimes causes miscompares when simulators
 change.  Updated regression for this change.  This change should also be made
 to stable release.

 Revision 1.98  2004/01/09 21:57:55  phase1geo
 Fixing bug in combinational logic report generator where partially covered
 expressions were being logged in summary report but not displayed when verbose
 output was needed.  Updated regression for this change.  Also added new multi_exp
 diagnostics to verify multiple expression combination logic expressions in report
 output.  Full regression passes at this point.

 Revision 1.97  2004/01/04 04:52:03  phase1geo
 Updating ChangeLog and TODO files.  Adding merge information to INFO line
 of CDD files and outputting this information to the merged reports.  Adding
 starting and ending line information to modules and added function for GUI
 to retrieve this information.  Updating full regression.

 Revision 1.96  2004/01/02 22:11:03  phase1geo
 Updating regression for latest batch of changes.  Full regression now passes.
 Fixed bug with event or operation in report command.

 Revision 1.95  2003/12/30 23:02:28  phase1geo
 Contains rest of fixes for multi-expression combinational logic report output.
 Full regression fails currently.

 Revision 1.94  2003/12/18 18:40:23  phase1geo
 Increasing detailed depth from 1 to 2 and making detail depth somewhat
 programmable.

 Revision 1.93  2003/12/16 23:22:07  phase1geo
 Adding initial code for line width specification for report output.

 Revision 1.92  2003/12/01 23:27:15  phase1geo
 Adding code for retrieving line summary module coverage information for
 GUI.

 Revision 1.91  2003/11/30 05:46:45  phase1geo
 Adding IF report outputting capability.  Updated always9 diagnostic for these
 changes and updated rest of regression CDD files accordingly.

 Revision 1.90  2003/11/29 06:55:48  phase1geo
 Fixing leftover bugs in better report output changes.  Fixed bug in param.c
 where parameters found in RHS expressions that were part of statements that
 were being removed were not being properly removed.  Fixed bug in sim.c where
 expressions in tree above conditional operator were not being evaluated if
 conditional expression was not at the top of tree.

 Revision 1.89  2003/11/15 04:21:57  phase1geo
 Fixing syntax errors found in Doxygen and GCC compiler.

 Revision 1.88  2003/11/05 05:22:56  phase1geo
 Final fix for bug 835366.  Full regression passes once again.

 Revision 1.87  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

 Revision 1.86  2003/10/17 12:55:36  phase1geo
 Intermediate checkin for LSB fixes.

 Revision 1.85  2003/10/17 02:12:38  phase1geo
 Adding CDD version information to info line of CDD file.  Updating regression
 for this change.

 Revision 1.84  2003/10/13 03:56:29  phase1geo
 Fixing some problems with new FSM code.  Not quite there yet.

 Revision 1.83  2003/10/11 05:15:07  phase1geo
 Updates for code optimizations for vector value data type (integers to chars).
 Updated regression for changes.

 Revision 1.82  2003/10/10 20:52:07  phase1geo
 Initial submission of FSM expression allowance code.  We are still not quite
 there yet, but we are getting close.

 Revision 1.81  2003/10/03 21:28:43  phase1geo
 Restructuring FSM handling to be better suited to handle new FSM input/output
 state variable allowances.  Regression should still pass but new FSM support
 is not supported.

 Revision 1.80  2003/09/22 19:42:31  phase1geo
 Adding print_output WARNING_WRAP and FATAL_WRAP lines to allow multi-line
 error output to be properly formatted to the output stream.

 Revision 1.79  2003/09/13 19:53:59  phase1geo
 Adding correct way of calculating state and state transition totals.  Modifying
 FSM summary reporting to reflect these changes.  Also added function documentation
 that was missing from last submission.

 Revision 1.78  2003/09/13 02:59:34  phase1geo
 Fixing bugs in arc.c created by extending entry supplemental field to 5 bits
 from 3 bits.  Additional two bits added for calculating unique states.

 Revision 1.77  2003/09/12 04:47:00  phase1geo
 More fixes for new FSM arc transition protocol.  Everything seems to work now
 except that state hits are not being counted correctly.

 Revision 1.76  2003/08/26 21:53:23  phase1geo
 Added database read/write functions and fixed problems with other arc functions.

 Revision 1.75  2003/08/25 13:02:03  phase1geo
 Initial stab at adding FSM support.  Contains summary reporting capability
 at this point and roughly works.  Updated regress suite as a result of these
 changes.

 Revision 1.74  2003/08/22 00:23:59  phase1geo
 Adding development documentation comment to defines.h for new sym_sig structure.

 Revision 1.73  2003/08/21 21:57:30  phase1geo
 Fixing bug with certain flavors of VCD files that alias signals that have differing
 MSBs and LSBs.  This takes care of the rest of the bugs for the 0.2 stable release.

 Revision 1.72  2003/08/15 20:02:08  phase1geo
 Added check for sys/times.h file for new code additions.

 Revision 1.71  2003/08/15 03:52:22  phase1geo
 More checkins of last checkin and adding some missing files.

 Revision 1.70  2003/08/10 03:50:10  phase1geo
 More development documentation updates.  All global variables are now
 documented correctly.  Also fixed some generated documentation warnings.
 Removed some unnecessary global variables.

 Revision 1.69  2003/08/05 20:25:05  phase1geo
 Fixing non-blocking bug and updating regression files according to the fix.
 Also added function vector_is_unknown() which can be called before making
 a call to vector_to_int() which will eleviate any X/Z-values causing problems
 with this conversion.  Additionally, the real1.1 regression report files were
 updated.

 Revision 1.68  2003/02/27 03:41:56  phase1geo
 More development documentation updates.

 Revision 1.67  2003/02/13 23:44:08  phase1geo
 Tentative fix for VCD file reading.  Not sure if it works correctly when
 original signal LSB is != 0.  Icarus Verilog testsuite passes.

 Revision 1.66  2003/02/12 14:56:22  phase1geo
 Adding info.c and info.h files to handle new general information line in
 CDD file.  Support for this new feature is not complete at this time.

 Revision 1.65  2003/02/07 23:12:30  phase1geo
 Optimizing db_add_statement function to avoid memory errors.  Adding check
 for -i option to avoid user error.

 Revision 1.64  2003/01/04 09:25:15  phase1geo
 Fixing file search algorithm to fix bug where unexpected module that was
 ignored cannot be found.  Added instance7.v diagnostic to verify appropriate
 handling of this problem.  Added tree.c and tree.h and removed define_t
 structure in lexer.

 Revision 1.63  2003/01/03 05:52:01  phase1geo
 Adding code to help safeguard from segmentation faults due to array overflow
 in VCD parser and symtable.  Reorganized code for symtable symbol lookup and
 value assignment.

 Revision 1.62  2002/12/07 17:46:52  phase1geo
 Fixing bug with handling memory declarations.  Added diagnostic to verify
 that memory declarations are handled properly.  Fixed bug with infinite
 looping in statement_connect function and optimized this part of the score
 command.  Added diagnostic to verify this fix (always9.v).  Fixed bug in
 report command with ordering of lines and combinational logic verbose output.
 This is now fixed correctly.

 Revision 1.61  2002/12/06 02:18:59  phase1geo
 Fixing bug with calculating list and concatenation lengths when MBIT_SEL
 expressions were included.  Also modified file parsing algorithm to be
 smarter when searching files for modules.  This change makes the parsing
 algorithm much more optimized and fixes the bug specified in our bug list.
 Added diagnostic to verify fix for first bug.

 Revision 1.60  2002/11/27 03:49:20  phase1geo
 Fixing bugs in score and report commands for regression.  Finally fixed
 static expression calculation to yield proper coverage results for constant
 expressions.  Updated regression suite and development documentation for
 changes.

 Revision 1.59  2002/11/24 14:38:12  phase1geo
 Updating more regression CDD files for bug fixes.  Fixing bugs where combinational
 expressions were counted more than once.  Adding new diagnostics to regression
 suite.

 Revision 1.58  2002/11/23 21:27:25  phase1geo
 Fixing bug with combinational logic being output when unmeasurable.

 Revision 1.57  2002/11/23 16:10:46  phase1geo
 Updating changelog and development documentation to include FSM description
 (this is a brainstorm on how to handle FSMs when we get to this point).  Fixed
 bug with code underlining function in handling parameter in reports.  Fixing bugs
 with MBIT/SBIT handling (this is not verified to be completely correct yet).

 Revision 1.56  2002/11/05 00:20:06  phase1geo
 Adding development documentation.  Fixing problem with combinational logic
 output in report command and updating full regression.

 Revision 1.55  2002/10/31 23:13:36  phase1geo
 Fixing C compatibility problems with cc and gcc.  Found a few possible problems
 with 64-bit vs. 32-bit compilation of the tool.  Fixed bug in parser that
 lead to bus errors.  Ran full regression in 64-bit mode without error.

 Revision 1.54  2002/10/29 19:57:50  phase1geo
 Fixing problems with beginning block comments within comments which are
 produced automatically by CVS.  Should fix warning messages from compiler.

 Revision 1.53  2002/10/25 13:43:49  phase1geo
 Adding statement iterators for moving in both directions in a list with a single
 pointer (two-way).  This allows us to reverse statement lists without additional
 memory and time (very efficient).  Full regression passes and TODO list items
 2 and 3 are completed.

 Revision 1.52  2002/10/24 05:48:58  phase1geo
 Additional fixes for MBIT_SEL.  Changed some philosophical stuff around for
 cleaner code and for correctness.  Added some development documentation for
 expressions and vectors.  At this point, there is a bug in the way that
 parameters are handled as far as scoring purposes are concerned but we no
 longer segfault.

 Revision 1.51  2002/10/23 03:39:06  phase1geo
 Fixing bug in MBIT_SEL expressions to calculate the expression widths
 correctly.  Updated diagnostic testsuite and added diagnostic that
 found the original bug.  A few documentation updates.

 Revision 1.50  2002/10/13 19:20:42  phase1geo
 Added -T option to score command for properly handling min:typ:max delay expressions.
 Updated documentation for -i and -T options to score command and added additional
 diagnostic to test instance handling.

 Revision 1.49  2002/10/11 05:23:21  phase1geo
 Removing local user message allocation and replacing with global to help
 with memory efficiency.

 Revision 1.48  2002/10/11 04:24:01  phase1geo
 This checkin represents some major code renovation in the score command to
 fully accommodate parameter support.  All parameter support is in at this
 point and the most commonly used parameter usages have been verified.  Some
 bugs were fixed in handling default values of constants and expression tree
 resizing has been optimized to its fullest.  Full regression has been
 updated and passes.  Adding new diagnostics to test suite.  Fixed a few
 problems in report outputting.

 Revision 1.47  2002/09/26 22:58:46  phase1geo
 Fixing syntax error.

 Revision 1.46  2002/09/25 02:51:44  phase1geo
 Removing need of vector nibble array allocation and deallocation during
 expression resizing for efficiency and bug reduction.  Other enhancements
 for parameter support.  Parameter stuff still not quite complete.

 Revision 1.45  2002/09/21 07:03:28  phase1geo
 Attached all parameter functions into db.c.  Just need to finish getting
 parser to correctly add override parameters.  Once this is complete, phase 3
 can start and will include regenerating expressions and signals before
 getting output to CDD file.

 Revision 1.44  2002/09/21 04:11:32  phase1geo
 Completed phase 1 for adding in parameter support.  Main code is written
 that will create an instance parameter from a given module parameter in
 its entirety.  The next step will be to complete the module parameter
 creation code all the way to the parser.  Regression still passes and
 everything compiles at this point.

 Revision 1.43  2002/09/19 05:25:19  phase1geo
 Fixing incorrect simulation of static values and fixing reports generated
 from these static expressions.  Also includes some modifications for parameters
 though these changes are not useful at this point.

 Revision 1.42  2002/09/13 05:12:25  phase1geo
 Adding final touches to -d option to report.  Adding documentation and
 updating development documentation to stay in sync.

 Revision 1.41  2002/09/06 03:05:28  phase1geo
 Some ideas about handling parameters have been added to these files.  Added
 "Special Thanks" section in User's Guide for acknowledgements to people
 helping in project.

 Revision 1.40  2002/08/27 11:53:16  phase1geo
 Adding more code for parameter support.  Moving parameters from being a
 part of modules to being a part of instances and calling the expression
 operation function in the parameter add functions.

 Revision 1.39  2002/08/26 12:57:03  phase1geo
 In the middle of adding parameter support.  Intermediate checkin but does
 not break regressions at this point.

 Revision 1.38  2002/08/23 12:55:33  phase1geo
 Starting to make modifications for parameter support.  Added parameter source
 and header files, changed vector_from_string function to be more verbose
 and updated Makefiles for new param.h/.c files.

 Revision 1.37  2002/07/23 12:56:22  phase1geo
 Fixing some memory overflow issues.  Still getting core dumps in some areas.

 Revision 1.36  2002/07/22 05:24:46  phase1geo
 Creating new VCD parser.  This should have performance benefits as well as
 have the ability to handle any problems that come up in parsing.

 Revision 1.35  2002/07/20 18:46:38  phase1geo
 Causing fully covered modules to not be output in reports.  Adding
 instance3.v diagnostic to verify this works correctly.

 Revision 1.34  2002/07/18 02:33:23  phase1geo
 Fixed instantiation addition.  Multiple hierarchy instantiation trees should
 now work.

 Revision 1.33  2002/07/14 05:10:42  phase1geo
 Added support for signal concatenation in score and report commands.  Fixed
 bugs in this code (and multiplication).

 Revision 1.32  2002/07/11 16:59:10  phase1geo
 Added release information to NEWS for upcoming release.

 Revision 1.31  2002/07/10 13:15:57  phase1geo
 Adding case1.1.v Verilog diagnostic to check default case statement.  There
 were reporting problems related to this.  Report problems have been fixed and
 full regression passes.

 Revision 1.30  2002/07/10 04:57:07  phase1geo
 Adding bits to vector nibble to allow us to specify what type of input
 static value was read in so that the output value may be displayed in
 the same format (DECIMAL, BINARY, OCTAL, HEXIDECIMAL).  Full regression
 passes.

 Revision 1.29  2002/07/09 17:27:25  phase1geo
 Fixing default case item handling and in the middle of making fixes for
 report outputting.

 Revision 1.28  2002/07/09 04:46:26  phase1geo
 Adding -D and -Q options to covered for outputting debug information or
 suppressing normal output entirely.  Updated generated documentation and
 modified Verilog diagnostic Makefile to use these new options.

 Revision 1.27  2002/07/05 16:49:47  phase1geo
 Modified a lot of code this go around.  Fixed VCD reader to handle changes in
 the reverse order (last changes are stored instead of first for timestamp).
 Fixed problem with AEDGE operator to handle vector value changes correctly.
 Added casez2.v diagnostic to verify proper handling of casez with '?' characters.
 Full regression passes; however, the recent changes seem to have impacted
 performance -- need to look into this.

 Revision 1.26  2002/07/05 05:00:13  phase1geo
 Removing CASE, CASEX, and CASEZ from line and combinational logic results.

 Revision 1.25  2002/07/05 04:12:46  phase1geo
 Correcting case, casex and casez equality calculation to conform to correct
 equality check for each case type.  Verified that case statements work correctly
 at this point.  Added diagnostics to verify case statements.

 Revision 1.24  2002/07/04 23:10:12  phase1geo
 Added proper support for case, casex, and casez statements in score command.
 Report command still incorrect for these statement types.

 Revision 1.23  2002/07/03 21:30:52  phase1geo
 Fixed remaining issues with always statements.  Full regression is running
 error free at this point.  Regenerated documentation.  Added EOR expression
 operation to handle the or expression in event lists.

 Revision 1.22  2002/07/03 19:54:36  phase1geo
 Adding/fixing code to properly handle always blocks with the event control
 structures attached.  Added several new diagnostics to test this ability.
 always1.v is still failing but the rest are passing.

 Revision 1.21  2002/07/02 19:52:50  phase1geo
 Removing unecessary diagnostics.  Cleaning up extraneous output and
 generating new documentation from source.  Regression passes at the
 current time.

 Revision 1.20  2002/07/02 18:42:18  phase1geo
 Various bug fixes.  Added support for multiple signals sharing the same VCD
 symbol.  Changed conditional support to allow proper simulation results.
 Updated VCD parser to allow for symbols containing only alphanumeric characters.

 Revision 1.19  2002/06/30 22:23:20  phase1geo
 Working on fixing looping in parser.  Statement connector needs to be revamped.

 Revision 1.18  2002/06/28 03:04:59  phase1geo
 Fixing more errors found by diagnostics.  Things are running pretty well at
 this point with current diagnostics.  Still some report output problems.

 Revision 1.17  2002/06/27 21:18:48  phase1geo
 Fixing report Verilog output.  simple.v verilog diagnostic now passes.

 Revision 1.16  2002/06/27 20:39:43  phase1geo
 Fixing scoring bugs as well as report bugs.  Things are starting to work
 fairly well now.  Added rest of support for delays.

 Revision 1.15  2002/06/26 04:59:50  phase1geo
 Adding initial support for delays.  Support is not yet complete and is
 currently untested.

 Revision 1.14  2002/06/23 21:18:21  phase1geo
 Added appropriate statement support in parser.  All parts should be in place
 and ready to start testing.

 Revision 1.13  2002/06/22 05:27:30  phase1geo
 Additional supporting code for simulation engine and statement support in
 parser.

 Revision 1.11  2002/05/13 03:02:58  phase1geo
 Adding lines back to expressions and removing them from statements (since the line
 number range of an expression can be calculated by looking at the expression line
 numbers).

 Revision 1.10  2002/05/03 03:39:36  phase1geo
 Removing all syntax errors due to addition of statements.  Added more statement
 support code.  Still have a ways to go before we can try anything.  Removed lines
 from expressions though we may want to consider putting these back for reporting
 purposes.

 Revision 1.9  2002/05/02 03:27:42  phase1geo
 Initial creation of statement structure and manipulation files.  Internals are
 still in a chaotic state.

 Revision 1.8  2002/04/30 05:04:25  phase1geo
 Added initial go-round of adding statement handling to parser.  Added simple
 Verilog test to check correct statement handling.  At this point there is a
 bug in the expression write function (we need to display statement trees in
 the proper order since they are unlike normal expression trees.)
*/

#endif

