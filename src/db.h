#ifndef __DB_H__
#define __DB_H__

/*!
 \file     db.h
 \author   Trevor Williams  (trevorw@charter.net)
 \date     12/7/2001
 \brief    Contains functions for writing and reading contents of
           covered database file.
*/


//! Writes contents of expressions, modules and signals to database file.
bool db_write( char* file );

//! Reads contents of database file and stores into internal lists.
bool db_read( char* file, int read_mode );

//! Adds specified module node to module tree.  Called by parser.
void db_add_instance( char* scope, char* modname );

//! Adds specified module to module list.  Called by parser.
void db_add_module( char* name, char* file );

//! Adds specified signal to signal list.  Called by parser.
void db_add_signal( char* name, int width, int lsb, int is_static );

//! Finds specified signal in module and returns pointer to the signal structure.  Called by parser.
signal* db_find_signal( char* name );

//! Creates new expression from specified information.  Called by parser and db_add_expression.
expression* db_create_expression( expression* right, expression* left, int op, int line, char* sig_name );

//! Adds specified expression to expression list.  Called by parser.
void db_add_expression( expression* root );

//! Creates new statement expression from specified information.  Called by parser.
statement* db_create_statement( expression* exp );

//! Adds specified statement to current module's statement list.  Called by parser.
void db_add_statement( statement* stmt );

//! Connects one statement block to another.
void db_statement_connect( statement* curr_stmt, statement* next_stmt );

//! Connects true statement to specified statement.
void db_connect_statement_true( statement* stmt, statement* exp_true );

//! Connects false statement to specified statement.
void db_connect_statement_false( statement* stmt, statement* exp_false );

//! Sets current VCD scope to specified scope.
void db_set_vcd_scope( char* scope );

//! Adds symbol to signal specified by name.
void db_assign_symbol( char* name, char* symbol );

//! Finds specified signal and returns TRUE if found.
bool db_symbol_found( char* symbol );

//! Finds specified signal, sets the value and adds its expression to expression queue. 
void db_find_set_add_signal( char* symbol, vector* vec );

//! Returns width of signal for specified symbol.
int db_get_signal_size( char* symbol );

//! Performs a timestep for all signal changes during this timestep.
void db_do_timestep( int time ); 

/* $Log$
/* Revision 1.6  2002/06/24 12:34:56  phase1geo
/* Fixing the set of the STMT_HEAD and STMT_STOP bits.  We are getting close.
/*
/* Revision 1.5  2002/06/24 04:54:48  phase1geo
/* More fixes and code additions to make statements work properly.  Still not
/* there at this point.
/*
/* Revision 1.4  2002/05/13 03:02:58  phase1geo
/* Adding lines back to expressions and removing them from statements (since the line
/* number range of an expression can be calculated by looking at the expression line
/* numbers).
/*/* Revision 1.3  2002/05/03 03:39:36  phase1geo/* Removing all syntax errors due to addition of statements.  Added more statement/* support code.  Still have a ways to go before we can try anything.  Removed lines/* from expressions though we may want to consider putting these back for reporting/* purposes./*/* Revision 1.2  2002/04/30 05:04:25  phase1geo/* Added initial go-round of adding statement handling to parser.  Added simple/* Verilog test to check correct statement handling.  At this point there is a/* bug in the expression write function (we need to display statement trees in/* the proper order since they are unlike normal expression trees.)/* */

#endif

