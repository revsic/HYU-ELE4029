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
/* For annonymous scope */
static int annon_lineno;
static int annon_num;

static void init_scope_info(int startloc)
{ scopeidx = 0;
  fnscope = 0;
  annon_lineno = 0;
  annon_num = 0;
  scope[0].name = "global";
  scope[0].location = startloc;
}

static char * annon_scope_name(int lineno)
{
#define ANNON_PREFIX "annon_"
#define ANNON_PREFIX_SIZE 6
// maximum number of the digits.
#define ANNON_POSTFIX 10
  char * name = (char *)malloc(
    sizeof(char) * (ANNON_PREFIX_SIZE + ANNON_POSTFIX + 1));
  strcpy(name, ANNON_PREFIX);
  if (annon_lineno != lineno)
  { annon_lineno = lineno;
    annon_num = 0;
  }
  snprintf(name + ANNON_PREFIX_SIZE, ANNON_POSTFIX,
    "%d_%d", annon_lineno, annon_num++);
  return name;
}

/* Post process for insert node. */
static void postInsert( TreeNode * t )
{ switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      { case CompK:
          free(scope[scopeidx].name);
          scopeidx -= 1;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Error: %s at line %d (name : %s)\n",
    message, t->lineno, t->attr.name);
  Error = TRUE;
}

static void simpleError(TreeNode * t, char * message)
{ fprintf(listing, "Error: %s at line %d\n", message, t->lineno);
  Error = TRUE;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t)
{ int size;
  char * name;
  SymAddr addr;
  switch (t->nodekind)
  { case DeclK:
      addr = st_lookup(scope[scopeidx].name, t->attr.name);
      switch (t->kind.decl)
      { case ParamK:
          if (t->type == Void)
            break;
          // fall through
        case VarK:
          if (addr.bucket != 0)
          { typeError(t, "redeclared variable");
            break;
          }
          size = -1;
          if (t->child[0] != NULL)
          { size = t->child[0]->attr.val;
            if (t->kind.decl == VarK && size <= 0)
            { typeError(t, "array size cannot be non-positive");
              break;
            }
          }
          st_insert(
            scope_find(scope[scopeidx].name),
            t->attr.name, t->type, size,
            t->lineno, scope[scopeidx].location++);
          break;
        case FnK:
          if (addr.bucket != 0)
          { typeError(t, "redeclared function");
            break;
          }
          // insert function
          addr.bucket = st_insert(
            scope_find(scope[scopeidx].name),
            t->attr.name, Function, -1,
            t->lineno, scope[scopeidx].location++);
          st_appendfn(addr.bucket, t);
          scope_insert(scope[scopeidx].name, t->attr.name);
          // update current scope info
          scopeidx += 1;
          scope[scopeidx].name = copyString(t->attr.name);
          scope[scopeidx].location = 0;
          fnscope = 1;
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
          { // generate annonymous scope with normalized postfix
            name = annon_scope_name(t->lineno);
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
          addr = st_lookup(scope[scopeidx].name, t->attr.name);
          if (addr.bucket == 0)
            typeError(t, "undeclared id");
          else
          /* already in table, so ignore location, 
             add line number of use only */ 
            st_appendline(addr.bucket, t->lineno);
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

#define INIT_LOC 2  // assume INI_LOC = nextloc
/* Initialize states in global scope. */
static void init_state()
{ // initialize scope
  int nextloc = global_init();
  // initialize scope block
  init_scope_info(nextloc);
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ init_state();
  traverse(syntaxTree,insertNode,postInsert);
  if (Error == 0 && TraceAnalyze)
  { fprintf(listing,"\n< Symbol table >\n");
    printSymTab(listing);
    fprintf(listing, "\n< Function Table >\n");
    printFnTab(listing);
    fprintf(listing, "\n< Function and Global Variables >\n");
    printFnAndGlobalTab(listing);
    fprintf(listing, "\n< Function Parameters and Local Variables >\n");
    printFnParamAndLocals(listing);
  }
}

static void scopeSetting(TreeNode * t)
{ char * name;
  switch (t->nodekind)
  { case DeclK:
      switch (t->kind.decl)
      { case FnK:
          // update current scope info
          scopeidx += 1;
          scope[scopeidx].name = copyString(t->attr.name);
          scope[scopeidx].location = 0;
          fnscope = 1;
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
          { // update current scope
            scopeidx += 1;
            scope[scopeidx].name = annon_scope_name(t->lineno);;
            scope[scopeidx].location = 0;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ int i;
  TreeNode * node;
  SymAddr addr;
  switch (t->nodekind)
  { case DeclK:
      switch (t->kind.decl)
      { case ParamK:
          // only single unnamed void parameter is available
          if (t->type == Void && (
            // (null) is predefined empty void parameter name
              strcmp(t->attr.name, "(null)") || t->sibling != NULL))
            typeError(t, "Variable Type cannot be Void");
          break;
        case VarK:
          // void cannot be variable type
          if (t->type == Void)
            typeError(t, "Variable Type cannot be Void");
          break;
        case FnK:
          // Do Nothing
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      {
       case AssignK:
          // only expression to variable is assignable
          if (t->child[0]->nodekind != ExpK || t->child[0]->kind.exp != IdK
              || t->child[1]->nodekind != ExpK)
          { simpleError(t, "invalid expression");
            break;
          }
          // cannot assign to function
          if (t->child[0]->type == Function)
          { simpleError(t, "cannot assign to function");
            break;
          }
          if (t->child[0]->type == Array)
          { simpleError(t, "cannot assign to array variable");
            break;
          }
          // only expression of same type is assignable
          if (t->child[0]->type != t->child[1]->type)
          { simpleError(t, "type miss match");
            break;
          }
          // assign type
          t->type = t->child[0]->type;
          break;
        case OpK:
          // currently only integer operation is available
          if ((t->child[0]->type != Integer) ||
              (t->child[1]->type != Integer))
          { simpleError(t,"operation applied to non-integer");
            break;
          }
          // assume all operation results are integer, no boolean
          t->type = Integer;
          break;
        case ConstK:
          // assume that all constant is integer
          t->type = Integer;
          break;
        case IdK:
          addr = st_lookup(scope[scopeidx].name, t->attr.name);
          if (addr.bucket->size > 0 && t->child[0] == NULL)
            // array declared but do not have indexing child
            t->type = Array;
          else
            t->type = addr.bucket->type;
          break;
        case CallK:
          addr = st_lookup(scope[scopeidx].name, t->attr.name);
          // counting number of the arguments
          i = 0;
          node = t->child[0];
          while (node != NULL)
          { i += 1;
            node = node->sibling;
          }
          // check parameter numbers
          if (i != addr.bucket->fninfo->numparam)
          { simpleError(t, "the numbers of the parameters are different");
            break;
          }
          // check param type
          node = t->child[0];
          for (i = 0; i < addr.bucket->fninfo->numparam; ++i)
          { if (addr.bucket->fninfo->params[i].type != node->type)
            { simpleError(t, "parameter type mismatch");
              break;
            }
            node = node->sibling;
          }
          // assign type
          t->type = addr.bucket->fninfo->retn;
          break;
        case IdxK:
          if (t->child[0]->type != Integer)
            simpleError(t, "index should be integer");
            break;
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case CompK:
          // for scope post processing
          free(scope[scopeidx].name);
          scopeidx -= 1;
          break;
        case IfK:
          // only integer condition is available
          // on parsing level, empty condition is filtered
          if (t->child[0]->type != Integer)
            simpleError(t->child[0],"if test is not integer");
          break;
        case WhileK:
          // only integer condition is available
          // on parsing level, empty condition is filtered
          if (t->child[0]->type != Integer)
            simpleError(t->child[0],"while test is not integer");
          break;
        case ReturnK:
          // then scopeidx >= 1 and scope[1] is method scope,
          // since function can be only declared on global scope by parser.
          addr = st_lookup("global", scope[1].name);
          if (t->child[0] == NULL)
          { if (addr.bucket->fninfo->retn != Void)
              simpleError(t, "return nothing on non-void function");
          }
          // child[0] is not null
          else if (t->child[0]->type != addr.bucket->fninfo->retn)
            simpleError(t, "return type mismatch");
          break;
        default:
          break;
      }
      break;
    default:
      break;

  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ init_scope_info(INIT_LOC);
  traverse(syntaxTree,scopeSetting,checkNode);
}
