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

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % HASHSIZE;
    ++i;
  }
  return temp;
}

static FunctionInfo * fninfo_init ()
{ int i;
  FunctionInfo * fninfo = (FunctionInfo * )malloc(sizeof(FunctionInfo));
  fninfo->retn = Void;
  fninfo->numparam = 0;
  for (i = 0; i < MAXPARAM; ++i)
  { fninfo->params[i].type = Void;
    fninfo->params[i].name = NULL;
  }
  return fninfo;
}

/* global scope */
static ScopeList globalScope = NULL;

ScopeList scope_init ( char * name )
{ int i;
  ScopeList scope = (ScopeList)malloc(sizeof(struct ScopeListRec));
  scope->name = copyString(name);
  for (i = 0; i < HASHSIZE; ++i)
    scope->bucket[i] = NULL;
  scope->parent = NULL;
  scope->child = NULL;
  scope->next = NULL;
  return scope;
}

/* Initialize global scope. */
int global_init ( void )
{ BucketList input, output;
  globalScope = scope_init("global");
  // predefined, method input
  input = st_insert(global_scope(), "input", Function, 0, 0);
  input->fninfo = fninfo_init();
  input->fninfo->retn = Integer;
  // predefined, method output
  output = st_insert(global_scope(), "output", Function, 0, 1);
  output->fninfo = fninfo_init();
  output->fninfo->retn = Void;
  output->fninfo->numparam = 1;
  output->fninfo->params[0].type = Integer;
  output->fninfo->params[0].name = "";
  return 2;
}

/* Get global scope. */
ScopeList global_scope( void )
{ return globalScope;
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
{ return scope_find_recur(globalScope, scope);
}

/* Find variable bucket from specified scope. */
BucketList scope_search ( ScopeList record, char * name )
{ int h = hash(name);
  BucketList l =  record->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  return l;
}

/* Insert new scope to specified parent. */
int scope_insert ( char * parent, char * name )
{ ScopeList newscope, siblings;
  // find scope
  ScopeList parentscope = scope_find(parent);
  if (parentscope == NULL)
    return 0;

  newscope = scope_init(name);
  newscope->parent = parentscope;
  // check child is null
  if (parentscope->child == NULL)
  { parentscope->child = newscope;
    return 1;
  }
  // insert to last siblings of child
  siblings = parentscope->child;
  while (siblings->next != NULL)
    siblings = siblings->next;
  siblings->next = newscope;
  return 1;
}

static SymAddr symaddr(ScopeList scope, BucketList bucket)
{ SymAddr addr;
  addr.scope = scope;
  addr.bucket = bucket;
  return addr;
}

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
SymAddr st_lookup ( char * scope, char * name )
{ // find scope
  ScopeList scopeRec = scope_find(scope);
  if (scopeRec == NULL)
    return symaddr(scopeRec, NULL);
  // find variable
  BucketList l;
  while ((l = scope_search(scopeRec, name)) == NULL && scopeRec->parent != NULL)
    scopeRec = scopeRec->parent;
  return symaddr(scopeRec, l);
}

/* Find ID bucket from specified scope, excluding its parent. */
SymAddr st_lookup_excluding_parent ( char * scope, char * name )
{ // find scope
  ScopeList scopeRec = scope_find(scope);
  if (scopeRec == NULL)
    return symaddr(NULL, NULL);
  // find variable
  return symaddr(scopeRec, scope_search(scopeRec, name));
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table.
 */
BucketList st_insert ( ScopeList scope, char * name, ExpType type, int lineno, int loc )
{ // find hashtable bucket
  int h = hash(name);
  BucketList l = scope->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l != NULL)
    return NULL;
  l = (BucketList) malloc(sizeof(struct BucketListRec));
  l->name = copyString(name);
  l->type = type;
  l->lines = (LineList) malloc(sizeof(struct LineListRec));
  l->lines->lineno = lineno;
  l->memloc = loc;
  l->fninfo = NULL;
  l->lines->next = NULL;
  l->next = scope->bucket[h];
  scope->bucket[h] = l;
  return l;
} /* st_insert */

/* Append lineno to bucket. */
void st_appendline ( BucketList bucket, int lineno )
{ LineList t = bucket->lines;
  while (t->next != NULL) t = t->next;
  t->next = (LineList) malloc(sizeof(struct LineListRec));
  t->next->lineno = lineno;
  t->next->next = NULL;
}

void st_appendfn ( BucketList bucket, TreeNode * node )
{ int i;
  FunctionInfo * fninfo = fninfo_init();
  bucket->fninfo = fninfo;
  fninfo->retn = node->type;
  // read parameters;
  node = node->child[0];
  if (node->type == Void)
    return;
  for (i = 0; i < MAXPARAM; ++i)
  { if (node == NULL)
      break;
    fninfo->numparam++;
    fninfo->params[i].type = node->type;
    fninfo->params[i].name = copyString(node->attr.name);
    node = node->sibling;
  }
}

/* Traverse scope with given callback. */ 
static void scope_traverse ( ScopeList scope, void (* callback) (ScopeList) )
{ callback(scope);
  // if in siblings, breadth-first
  if (scope->next != NULL)
    scope_traverse(scope->next, callback);
  // if in children
  if (scope->child != NULL)
    scope_traverse(scope->child, callback);
}

/* Print symbol table of given scope. */
static void scope_print ( ScopeList list, FILE * listing )
{ int i;
  if (list == NULL)
    return;
  for (i = 0; i < HASHSIZE; ++i)
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
static void scope_print_stream(ScopeList list)
{ scope_print(list, stream);
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab ( FILE * listing )
{ fprintf(listing,"Variable Name Variable Type  Scope Name  Location  Line Numbers\n");
  fprintf(listing,"------------- -------------  ----------  --------  ------------\n");
  // implicit parameter setting
  stream = listing;
  scope_traverse(globalScope, scope_print_stream);
} /* printSymTab */

/* Print function table of given scope. */
static void fn_print ( ScopeList list, FILE * listing )
{ int i, j;
  if (list == NULL)
    return;
  for (i = 0; i < HASHSIZE; ++i)
  { if (list->bucket[i] != NULL)
    { BucketList l = list->bucket[i];
      while (l != NULL)
      { if (l->type != Function)
        { l = l->next;
          continue;
        }
        LineList t = l->lines;
        fprintf(listing, "%-13s  ",l->name);
        fprintf(listing, "%-10s  ", list->name);
        fprintf(listing, "%-11s  ", dbgExpType(l->fninfo->retn));
        if (l->fninfo->numparam == 0)
          fprintf(listing, "                %-14s", dbgExpType(Void));
        else
        { for (j = 0; j < l->fninfo->numparam; ++j)
          { fprintf(listing, "\n%-40s%-14s  ",
              " ", l->fninfo->params[j].name);
            fprintf(listing, "%-14s", dbgExpType(l->fninfo->params[j].type));
          }
        }
        fprintf(listing,"\n");
        l = l->next;
      }
    }
  }
}

/* Print function table of given scope,
 * assume `stream` as parameter implicitly.
 */
static void fn_print_stream(ScopeList list)
{ fn_print(list, stream);
}

/* print function table */
void printFnTab ( FILE * listing )
{ fprintf(listing,"Function Name  Scope Name  Return Type  Parameter Name  Parameter Type\n");
  fprintf(listing,"-------------  ----------  -----------  --------------  --------------\n");
  // implicit parameter setting
  stream = listing;
  scope_traverse(globalScope, fn_print_stream);
}

/* Print function table of given scope. */
static void fn_and_global_print ( ScopeList list, FILE * listing )
{ int i, j;
  if (list == NULL)
    return;
  for (i = 0; i < HASHSIZE; ++i)
  { if (list->bucket[i] != NULL)
    { BucketList l = list->bucket[i];
      while (l != NULL)
      { if (l->type != Function)
        { l = l->next;
          continue;
        }
        LineList t = l->lines;
        fprintf(listing, "%-11s  ",l->name);
        fprintf(listing, "%-9s  ", dbgExpType(l->type));
        if (l->type == Function)
          fprintf(listing, "%-11s", dbgExpType(l->fninfo->retn));
        else
          fprintf(listing, "%-11s", dbgExpType(l->type));
        fprintf(listing,"\n");
        l = l->next;
      }
    }
  }
}

/* Print function table of given scope,
 * assume `stream` as parameter implicitly.
 */
static void fn_and_global_print_stream(ScopeList list)
{ fn_and_global_print(list, stream);
}

/* print function table */
void printFnAndGlobalTab ( FILE * listing )
{ fprintf(listing,"  ID Name     ID Type    Data Type \n");
  fprintf(listing,"-----------  ---------  -----------\n");
  // implicit parameter setting
  stream = listing;
  scope_traverse(globalScope, fn_and_global_print_stream);
}
