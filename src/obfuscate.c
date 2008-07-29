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
 \file     obfuscate.c
 \author   Trevor Williams  (phase1geo@gmail.com)
 \date     9/16/2006
*/

#include <string.h>
#include <stdio.h>

#include "defines.h"
#include "obfuscate.h"
#include "util.h"
#include "tree.h"


/*!
 Pointer to the root of the obfuscation tree containing real to obfuscated name pairings.
*/
static tnode* obf_tree = NULL;

/*!
 Current obfuscation identifier.  Incremented by one each time that a new obfuscation occurs.
*/
static int obf_curr_id = 1000;

/*!
 Specifies obfuscation mode.
*/
bool obf_mode;


/*!
 Sets the global 'obf_mode' variable to the specified value.
*/
void obfuscate_set_mode(
  bool value  /*!< Boolean value to set obfuscation mode to */
) { PROFILE(OBFUSCATE_SET_MODE);

  obf_mode = value;

}

/*!
 \return Returns the obfuscated name for this object.

 Looks up the given real name in the obfuscation tree.  If it exists, simply
 return the given name; otherwise, create a new element in the tree to represent
 this new name.
*/
char* obfuscate_name(
  const char* real_name,  /*!< Name of actual object in design */
  char        prefix      /*!< Character representing the prefix of the obfuscated name */
) { PROFILE(OBFUSCATE_NAME);

  tnode*       obfnode;    /* Pointer to obfuscated tree node */
  char*        obfname;    /* Obfuscated name */
  char*        key;        /* Temporary name used for searching */
  char         tname[30];  /* Temporary name used for sizing obfuscation ID */
  unsigned int slen;       /* Length of string to allocate and populate */
  unsigned int rv;         /* Return value from snprintf calls */

  /* Create temporary name */
  slen = strlen( real_name ) + 3;
  key = (char*)malloc_safe( slen );
  rv = snprintf( key, slen, "%s-%c", real_name, prefix );
  assert( rv < slen );

  /* If the name was previously obfuscated, return that name */
  if( (obfnode = tree_find( key, obf_tree )) != NULL ) {

    obfname = obfnode->value;

  /* Otherwise, create a new obfuscated entry in the tree and return the new name */
  } else {

    /* Calculate the size needed for storing the obfuscated name */
    rv = snprintf( tname, 30, "%04d", obf_curr_id );
    assert( rv < 30 );

    /* Create obfuscated name */
    slen    = strlen( tname ) + 2;
    obfname = (char*)malloc_safe( slen );
    rv = snprintf( obfname, slen, "%c%04d", prefix, obf_curr_id );
    assert( rv < slen );
    obf_curr_id++;

    /* Add the obfuscated name to the tree */
    (void)tree_add( key, obfname, FALSE, &obf_tree );

  }

  /* Deallocate key string */
  free_safe( key, (strlen( key ) + 1) );

  PROFILE_END;

  return( obfname );

}

/*!
 Deallocates all memory associated with obfuscation.
*/
void obfuscate_dealloc() { PROFILE(OBFUSCATE_DEALLOC);

  tree_dealloc( obf_tree );

  PROFILE_END;

}


/*
 $Log$
 Revision 1.12.4.1  2008/07/10 22:43:52  phase1geo
 Merging in rank-devel-branch into this branch.  Added -f options for all commands
 to allow files containing command-line arguments to be added.  A few error diagnostics
 are currently failing due to changes in the rank branch that never got fixed in that
 branch.  Checkpointing.

 Revision 1.13  2008/06/27 14:02:03  phase1geo
 Fixing splint and -Wextra warnings.  Also fixing comment formatting.

 Revision 1.12  2008/03/17 05:26:16  phase1geo
 Checkpointing.  Things don't compile at the moment.

 Revision 1.11  2008/01/16 23:10:31  phase1geo
 More splint updates.  Code is now warning/error free with current version
 of run_splint.  Still have regression issues to debug.

 Revision 1.10  2008/01/09 23:54:15  phase1geo
 More splint updates.

 Revision 1.9  2008/01/08 21:13:08  phase1geo
 Completed -weak splint run.  Full regressions pass.

 Revision 1.8  2007/12/11 05:48:25  phase1geo
 Fixing more compile errors with new code changes and adding more profiling.
 Still have a ways to go before we can compile cleanly again (next submission
 should do it).

 Revision 1.7  2007/11/20 05:28:59  phase1geo
 Updating e-mail address from trevorw@charter.net to phase1geo@gmail.com.

 Revision 1.6  2007/09/13 17:03:30  phase1geo
 Cleaning up some const-ness corrections -- still more to go but it's a good
 start.

 Revision 1.5  2006/10/06 22:45:57  phase1geo
 Added support for the wait() statement.  Added wait1 diagnostic to regression
 suite to verify its behavior.  Also added missing GPL license note at the top
 of several *.h and *.c files that are somewhat new.

 Revision 1.4  2006/08/18 22:19:54  phase1geo
 Fully integrated obfuscation into the development release.

 Revision 1.3  2006/08/18 22:07:45  phase1geo
 Integrating obfuscation into all user-viewable output.  Verified that these
 changes have not made an impact on regressions.  Also improved performance
 impact of not obfuscating output.

 Revision 1.1.2.2  2006/08/18 04:50:51  phase1geo
 First swag at integrating name obfuscation for all output (with the exception
 of CDD output).

 Revision 1.1.2.1  2006/08/17 04:17:37  phase1geo
 Adding files to obfuscate actual names when outputting any user-visible
 information.
*/

