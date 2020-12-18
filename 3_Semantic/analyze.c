/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "util.h"

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

/* Scope block. */
typedef struct {
  int location;
  char * name;
} ScopeBlock;

/* Scope stack. */
#define SCOPE_MAX 100
static ScopeBlock scope[SCOPE_MAX];
static int scopeidx;
/* For empty name compound statement of new scoping */
static int fnscope;

/* Post process for insert node. */
static void postInsert( TreeNode * t )
{ switch (t->nodekind)
  { case DeclK:
      switch (t->kind.decl)
      { case ParamK:
        case VarK:
        case FnK:
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case CompK:
          free(scope[scopeidx].name);
          scopeidx -= 1;
          break;
        case IfK:
        case WhileK:
        case ReturnK:
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case AssignK:
        case OpK:
        case ConstK:
        case IdK:
        case CallK:
        case IdxK:
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t)
{ char * name;
  switch (t->nodekind)
  { case DeclK:
      switch (t->kind.decl)
      { case ParamK:
          if (t->type == Void)
            break;
          // fall through
        case VarK:
          if (st_lookup(scope[scopeidx].name, t->attr.name) != 0)
          { Error = TRUE;
            fprintf(listing, "Error: Redeclared Variable \"%s\" at line %d\n",
              t->attr.name, t->lineno);
          }
          else
            st_insert(scope[scopeidx].name, t->attr.name, t->type,
              t->lineno, scope[scopeidx].location++);
          break;
        case FnK:
          if (st_lookup(scope[scopeidx].name, t->attr.name) != 0)
          { Error = TRUE;
            fprintf(listing, "Error: Redeclared function \"%s\" at line %d\n",
              t->attr.name, t->lineno);
          }
          else
          { // insert function
            st_insert(scope[scopeidx].name, t->attr.name, Function,
              t->lineno, scope[scopeidx].location++);
            scope_insert(scope[scopeidx].name, t->attr.name);
            // update current scope info
            scopeidx += 1;
            scope[scopeidx].name = copyString(t->attr.name);
            scope[scopeidx].location = 0;
            fnscope = 1;
          }
          break;
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case CompK:
          if (fnscope)
            fnscope = 0;
          else
          { // generate annonymous scope with random postfix
#define ANNON_PREFIX "annon_"
#define ANNON_PREFIX_SIZE 6
#define ANNON_POSTFIX 5
            name = (char *)malloc(
              sizeof(char) * (ANNON_PREFIX_SIZE + ANNON_POSTFIX + 1));
            strcpy(name, ANNON_PREFIX);
            randomFill(name + ANNON_PREFIX_SIZE, ANNON_POSTFIX);
            name[ANNON_PREFIX_SIZE + ANNON_POSTFIX] = '\0';
            // generate new scope
            scope_insert(scope[scopeidx].name, name);
            // update current scope
            scopeidx += 1;
            scope[scopeidx].name = name;
            scope[scopeidx].location = 0;
          }
          break;
        case IfK:
        case WhileK:
        case ReturnK:
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case AssignK:
        case OpK:
        case ConstK:
          break;
        case IdK:
          // fall through
        case CallK:
          if (st_lookup(scope[scopeidx].name, t->attr.name) == 0)
          { Error = TRUE;
            fprintf(listing, "Error: Undeclared ID \"%s\" at line %d\n",
              t->attr.name, t->lineno);
          }
          else
          /* already in table, so ignore location, 
             add line number of use only */ 
            st_insert(scope[scopeidx].name, t->attr.name, t->type,
              t->lineno, -1);
          break;
        case IdxK:
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Initialize states in global scope. */
static void init_state()
{ // initialize scope
  global_init();
  // predefined
  st_insert("global", "input", Function, 0, 0);
  st_insert("global", "output", Function, 0, 1);
  // initialize scope block
  scope[0].name = copyString("global");
  scope[0].location = 2;
  scopeidx = 0;
  fnscope = 0;
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ init_state();
  traverse(syntaxTree,insertNode,postInsert);
  if (Error == 0 && TraceAnalyze)
  { fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ 
  // switch (t->nodekind)
  // { case ExpK:
  //     switch (t->kind.exp)
  //     { case OpK:
  //         if ((t->child[0]->type != Integer) ||
  //             (t->child[1]->type != Integer))
  //           typeError(t,"Op applied to non-integer");
  //         if ((t->attr.op == EQ) || (t->attr.op == LT))
  //           t->type = Boolean;
  //         else
  //           t->type = Integer;
  //         break;
  //       case ConstK:
  //       case IdK:
  //         t->type = Integer;
  //         break;
  //       default:
  //         break;
  //     }
  //     break;
  //   case StmtK:
  //     switch (t->kind.stmt)
  //     { case IfK:
  //         if (t->child[0]->type == Integer)
  //           typeError(t->child[0],"if test is not Boolean");
  //         break;
  //       case AssignK:
  //         if (t->child[0]->type != Integer)
  //           typeError(t->child[0],"assignment of non-integer value");
  //         break;
  //       case WriteK:
  //         if (t->child[0]->type != Integer)
  //           typeError(t->child[0],"write of non-integer value");
  //         break;
  //       case RepeatK:
  //         if (t->child[1]->type == Integer)
  //           typeError(t->child[1],"repeat test is not Boolean");
  //         break;
  //       default:
  //         break;
  //     }
  //     break;
  //   default:
  //     break;

  // }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}
