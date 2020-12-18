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

/* HASHSIZE is the size of the hash table */
#define HASHSIZE 211

/* Maximum number of the parameters */
#define MAXPARAM 10

/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;

typedef struct
  { ExpType retn;
    int numparam;
    struct {
      ExpType type;
      char * name;
    } params[MAXPARAM];
  } FunctionInfo;

/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
   { char * name;
     ExpType type;
     LineList lines;
     int memloc ; /* memory location for variable */
     FunctionInfo * fninfo;
     struct BucketListRec * next;
   } * BucketList;

/* The list of ID scopes.
 */
typedef struct ScopeListRec
   { char * name;
     BucketList bucket[HASHSIZE];
     struct ScopeListRec * parent;  // parent node
     struct ScopeListRec * child;   // first child
     struct ScopeListRec * next;    // linked list, siblings.
   } * ScopeList;

/* Address to symbol table.
 */
typedef struct
   { ScopeList scope;
     BucketList bucket;
   } SymAddr;

/* Procedure for initializing global scope.
 */
int global_init ( void );

/* Get global scope.
 */
ScopeList global_scope( void );

/* Find scope. */
ScopeList scope_find ( char * scope );

/* Search identity from given record. */
BucketList scope_search ( ScopeList record, char * name );

/* Insert new scope to specified parent. */
int scope_insert ( char * parent, char * name );

/* Function st_lookup returns the bucket pointer
 * of a variable or 0 if not found.
 */
SymAddr st_lookup ( char * scope, char * name );

/* Function st_lookup_excluding_parent returns the bucket pointer
 * of variable or 0 if not found,
 * only in the specified scope (excluding parent).
 */
SymAddr st_lookup_excluding_parent ( char * scope, char * name );

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored.
 * Return 1 for success, 0 for scope not found.
 */
BucketList st_insert( ScopeList scope, char * name, ExpType type, int lineno, int loc );

/* Append new line list to the given bucket. */
void st_appendline ( BucketList bucket, int lineno );

/* Add function infos to bucket */
void st_appendfn ( BucketList bucket, TreeNode * node );

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

/* Print function table. */
void printFnTab(FILE * listing);

#endif
