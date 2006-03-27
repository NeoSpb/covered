/*!
 \file     attr.c
 \author   Trevor Williams  (trevorw@charter.net)
 \date     10/25/2003

 \par What is an attribute?
 An attribute is a Verilog-2001 syntatical feature that allows meta-comment-like information to
 be stored in a Verilog file which can be used by any tool that is capable of implementing its functionality.
 It uses the syntax (* ... *) which may be placed before just about any Verilog construct within a design.
 Covered is able to parse attributes, implementing its functionality for all Covered-defined attribute commands.
 Currently covered can parse the following attribute commands:

 \par
 -# \b covered_fsm - Allows FSM-specific coverage information to be embedded within a design file
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "defines.h"
#include "attr.h"
#include "expr.h"
#include "func_unit.h"
#include "util.h"
#include "fsm_arg.h"


/*!
 \param name  String identifier of this attribute parameter.
 \param expr  Pointer to the expression assigned to the parameter identifier.

 \return Returns a pointer to the newly allocated/initialized attribute parameter.

 Allocates and initializes an attribute parameter entry.
*/
attr_param* attribute_create( const char* name, expression* expr ) {

  attr_param* ap;  /* Pointer to newly allocated attribute parameter */

  ap        = (attr_param*)malloc_safe( sizeof( attr_param ), __FILE__, __LINE__ );
  ap->name  = strdup_safe( name, __FILE__, __LINE__ );
  ap->expr  = expr;
  ap->index = 0;
  ap->next  = NULL;
  ap->prev  = NULL;

  return( ap );

}

/*!
 \param ap     Pointer to current element of attribute parameter list to parse.
 \param funit  Pointer to current functional unit containing this attribute.

 Parses the attribute parameter list in a recursive fashion.  First,
 we go for the last entry and see if it refers to an attribute that covered
 should parse.  If this attribute is identified by Covered as one of its own, it
 calls the appropriate function to handle the entire attribute parameter list.
*/
void attribute_parse( attr_param* ap, func_unit* funit ) {

  if( ap != NULL ) {

    if( ap->next != NULL ) {
      attribute_parse( ap->next, funit );
    } else {
      if( strcmp( ap->name, "covered_fsm" ) == 0 ) {
        fsm_arg_parse_attr( ap->prev, funit );
      }
    }

  }

}

/*!
 \param ap  Pointer to the attribute parameter list to remove.

 Deallocates all memory for the entire attribute parameter list.
*/
void attribute_dealloc( attr_param* ap ) {

  if( ap != NULL ) {

    /* Deallocate the next attribute */
    attribute_dealloc( ap->next );

    /* Deallocate the name string */
    free_safe( ap->name );

    /* Deallocate the expression tree */
    expression_dealloc( ap->expr, FALSE );

    /* Finally, deallocate myself */
    free_safe( ap );

  }

}

/*
 $Log$
 Revision 1.3  2005/11/08 23:12:09  phase1geo
 Fixes for function/task additions.  Still a lot of testing on these structures;
 however, regressions now pass again so we are checkpointing here.

 Revision 1.2  2004/03/16 05:45:43  phase1geo
 Checkin contains a plethora of changes, bug fixes, enhancements...
 Some of which include:  new diagnostics to verify bug fixes found in field,
 test generator script for creating new diagnostics, enhancing error reporting
 output to include filename and line number of failing code (useful for error
 regression testing), support for error regression testing, bug fixes for
 segmentation fault errors found in field, additional data integrity features,
 and code support for GUI tool (this submission does not include TCL files).

 Revision 1.1  2003/10/28 00:18:05  phase1geo
 Adding initial support for inline attributes to specify FSMs.  Still more
 work to go but full regression still passes at this point.

*/

