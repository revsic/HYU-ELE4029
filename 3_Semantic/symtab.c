/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "util.h"

/* SIZE is the size of the hash table */
#define SIZE 211

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;

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
     struct BucketListRec * next;
   } * BucketList;

/* The list of ID scopes.
 */
typedef struct ScopeListRec
   { char * name;
     BucketList bucket[SIZE];
     struct ScopeListRec * parent;  // parent node
     struct ScopeListRec * child;   // first child
     struct ScopeListRec * next;    // linked list, siblings.
   } * ScopeList;

/* global scope */
static ScopeList globalScope = NULL;

ScopeList scope_init ( const char * name )
{ int i;
  ScopeList scope = malloc(sizeof(struct ScopeListRec));
  scope->name = malloc(strlen(name) + 1);
  strcpy(scope->name, name);
  for (i = 0; i < SIZE; ++i)
    scope->bucket[i] = NULL;
  scope->parent = NULL;
  scope->child = NULL;
  scope->next = NULL;
  return scope;
}

/* Initialize global scope. */
void global_init ( void )
{ globalScope = scope_init("global");
}

/* Traverse scope to find the given scope name, BFS. */
ScopeList scope_find_recur ( ScopeList scope, char * name )
{ if (!strcmp(scope->name, name))
    return scope;
  ScopeList retn;
  // if in siblings, breadth-first
  if (scope->next != NULL &&
      (retn = scope_find_recur(scope->next, name)) != NULL)
    return retn;
  // if in children
  if (scope->child != NULL &&
      (retn = scope_find_recur(scope->child, name)) != NULL)
    return retn;
  // if not found
  return NULL;
} 

/* Find scope, return proper pointer if found or NULL. */
ScopeList scope_find ( char * scope )
{ if (globalScope == NULL)
    global_init();
  return scope_find_recur(globalScope, scope);
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
int st_insert ( char * scope, char * name, ExpType type, int lineno, int loc )
{ // find scope
  ScopeList scopeRec = scope_find(scope);
  if (scopeRec == NULL)
    return 0;
  // find hashtable bucket
  int h = hash(name);
  BucketList l =  scopeRec->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) /* variable not yet in table */
  { l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->type = type;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->lines->next = NULL;
    l->next = scopeRec->bucket[h];
    scopeRec->bucket[h] = l; }
  else /* found in table, so just add line number */
  { LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
  return 1;
} /* st_insert */

/* Find variable bucket from specified scope. */
BucketList scope_search ( ScopeList record, char * name )
{ int h = hash(name);
  BucketList l =  scopeRec->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  return l;
}

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
Bucketlist st_lookup ( char * scope, char * name )
{ // find scope
  ScopeList scopeRec = scope_find(scope);
  if (scopeRec == NULL)
    return NULL;
  // find variable
  BucketList l;
  while ((l = scope_search(scopeRec, name)) == NULL && scopeRec->parent != NULL)
    scopeRec = scopeRec->parent;
  return l;
}

/* Find ID bucket from specified scope, excluding its parent. */
BucketList st_lookup_excluding_parent ( char * scope, char * name )
{ // find scope
  ScopeList scopeRec = scope_find(scope);
  if (scopeRec == NULL)
    return NULL;
  // find variable
  return scope_search(scopeRec, name);
}

/* Traverse scope with given callback. */ 
void scope_traverse ( ScopeList scope, void (* callback) (ScopeList) )
{ callback(scope);
  // if in siblings, breadth-first
  if (scope->next != NULL)
    scope_traverse(scope->next, callback);
  // if in children
  if (scope->child != NULL)
    scope_traverse(scope->child, callback);
}

/* Print symbol table of given scope. */
void scope_print ( ScopeList list, FILE * listing )
{ int i;
  if (list == NULL)
    return;
  for (i = 0; i < SIZE; ++i)
  { if (list->bucket[i] != NULL)
    { BucketList l = list->bucket[i];
      while (l != NULL)
      { LineList t = l->lines;
        fprintf(listing, "%-14s ",l->name);
        fprintf(listing, "%-13s  ", dbgExpType(l->type));
        fprintf(listing, "%-10s  ", list->name);
        fprintf(listing, "%-8d  ",l->memloc);
        while (t != NULL)
        { fprintf(listing,"%4d ",t->lineno);
          t = t->next;
        }
        fprintf(listing,"\n");
        l = l->next;
      }
    }
  }
}

/* File stream for writing symbol table */
static FILE * stream;
/* Print symbol table of given scope,
 * assume `stream` as parameter implicitly.
 */
void scope_print_stream(ScopeList list)
{ scope_print(list, stream)
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab ( FILE * listing )
{ fprintf(listing,"Variable Name Variable Type  Scope Name  Location  Line Numbers\n");
  fprintf(listing,"------------- -------------  ----------  --------  ------------\n");
  // init global
  if (globalScope == NULL)
    global_init();
  // implicit parameter setting
  stream = listing;
  scope_traverse(globalScope, scope_print_stream);
} /* printSymTab */
