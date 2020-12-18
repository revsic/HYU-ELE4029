/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored.
 * Return 1 for success, 0 for scope not found.
 */
int st_insert( char * scope, char * name, ExpType type, int lineno, int loc );

/* Function st_lookup returns the bucket pointer
 * of a variable or NULL if not found.
 */
BucketList st_lookup ( char * scope, char * name );

/* Function st_lookup_excluding_parent returns the bucket pointer
 * of variable or NULL if not found,
 * only in the specified scope (excluding parent).
 */
BucketList st_lookup_excluding_parent ( char * scope, char * name );

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

#endif
