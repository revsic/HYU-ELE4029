/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedNum;     /* for use in array assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */

static int yylex(void);

%}

%token IF ELSE WHILE RETURN INT VOID
%token ID NUM 
%token ASSIGN EQ NE LT LE GT GE PLUS MINUS TIMES OVER LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token ERROR 

%% /* Grammar for TINY */

program     : decl_list
                { savedTree = $1; } 
            ;
decl_list   : decl_list decl
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                      t = t->sibling;
                    /* $2 null check is unnecessary since decl is not nullable. */
                    t->sibling = $2;
                    $$ = $1;
                  }
                  else $$ = $2;
                }
            | decl { $$ = $1; }
            ;
decl        : var_decl { $$ = $1; }
            | fn_decl  { $$ = $1; }
            ;
var_decl    : type_spec
              ID { savedName = copyString(tokenString);
                   savedLineNo = lineno; }
              SEMI
                { $$ = newDeclNode(VarK);
                  $$->attr.name = savedName;
                  $$->lineno = savedLineNo;
                  $$->type = $1->type;
                  free($1);
                }
            | type_spec
              ID { savedName = copyString(tokenString);
                   savedLineNo = lineno; }
              LBRACE
              NUM { savedNum = atoi(tokenString); }
              RBRACE SEMI
                { $$ = newDeclNode(VarK);
                  $$->attr.array = arrayAttr(savedName, savedNum);
                  $$->lineno = savedLineNo;
                  $$->type = $1->type;
                  free($1);
                }
            ;
type_spec   : INT { $$ = newExpNode(IdK);
                    $$->type = Integer; }
            | VOID { $$ = newExpNode(IdK);
                     $$->type = Void; }
            ;
fn_decl     : type_spec ID { savedName = copyString(tokenString);
                             savedLineNo = lineno; }
              LPAREN params RPAREN comp_stmt
                { $$ = newDeclNode(FnK);
                  $$->child[0] = $4;
                  $$->child[1] = $6;
                  $$->attr.name = savedName;
                  $$->lineno = savedLineNo;
                  $$->type = $1->type;
                  free($1);
                }
            ;
params      : param_list { $$ = $1; }
            | VOID
                { $$ = newDeclNode(VarK);
                  $$->attr.name = "void";
                  $$->lineno = lineno;
                  $$->type = Boolean;
                }
            ;
param_list  : param_list COMMA param
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                      t = t->sibling;
                    /* $3 null check is unnecessary since param is not nullable. */
                    t->sibling = $3;
                    $$ = $1;
                  }
                  else $$ = $3;
                }
            | param { $$ = $1; }
            ;
param       : type_spec ID
                { $$ = newDeclNode(VarK);
                  $$->attr.name = copyString(tokenString);
                  $$->lineno = lineno;
                  $$->type = $1->type;
                  free($1);
                }
            | type_spec ID { savedName = copyString(tokenString);
                             savedLineNo = lineno; }
              LBRACE RBRACE
                { $$ = newDeclNode(VarK);
                  $$->attr.array = arrayAttr(savedName, -1);
                  $$->lineno = savedLineNo;
                  $$->type = $1->type;
                  free($1);
                }
            ;
comp_stmt   : LCURLY local_decl stmt_list RCURLY
                { if ($2 == NULL)
                    $$ = $3;
                  else if ($3 == NULL)
                    $$ = $2;
                  else {
                    YYSTYPE t = $1;
                    while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $2;
                    $$ = $1;
                  }
                }
            ;
local_decl  : local_decl var_decl
                { if ($1 == NULL)
                    $$ = $2;
                  else if ($2 == NULL)
                    $$ = $1;
                  else {
                    YYSTYPE t = $1;
                    while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $2;
                    $$ = $1;
                  }
                }
            | /* empty */ { $$ = NULL; }
            ;
stmt_list   : stmt_list stmt
                { if ($1 == NULL)
                    $$ = $2;
                  else if ($2 == NULL)
                    $$ = $1;
                  else {
                    YYSTYPE t = $1;
                    while (t->sibling != NULL)
                      t = t->sibling;
                    t->sibling = $2;
                    $$ = $1;
                  }
                }
            | /* empty */ { $$ = NULL; }
            ;
stmt        : expr_stmt { $$ = $1; }
            | comp_stmt { $$ = $1; }
            | if_stmt { $$ = $1; }
            | while_stmt { $$ = $1; }
            | return_stmt { $$ = $1; }
            ;
expr_stmt   : expr SEMI { $$ = $1; }
            | SEMI { $$ = NULL; }
            ;
if_stmt     : IF LPAREN expr RPAREN stmt
                { $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                }
            | IF LPAREN expr RPAREN stmt ELSE stmt
                { $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                  $$->child[2] = $7;
                }
            ;
while_stmt  : WHILE LPAREN expr RPAREN stmt
                { $$ = newStmtNode(WhileK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                }
            ;
return_stmt : RETURN SEMI { $$ = newStmtNode(ReturnK); }
            | RETURN expr SEMI
                { $$ = newStmtNode(ReturnK);
                  $$->child[0] = $2;
                }
            ;
expr        : var ASSIGN expr
                { $$ = newExpNode(AssignK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                }
            | simple_expr { $$ = $1; }
            ;
var         : ID 
                { $$ = newExpNode(IdK);
                  $$->attr.name = copyString(tokenString);
                }
            | ID { savedName = copyString(tokenString); } 
              LBRACE expr RBRACE
                { $$ = newExpNode(IdK);
                  $$->attr.name = copyString(tokenString);
                  $$->child[0] = $4;
                }
            ;
simple_expr : add_expr relop add_expr
                { $$ = newExpNode(OpK);
                  $$->attr.op = $2->attr.op;
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  free($2);
                }
            | add_expr { $$ = $1; }
            ;
relop       : LE { $$ = newOpNode(LE); }
            | LT { $$ = newOpNode(LT); }
            | GT { $$ = newOpNode(GT); }
            | GE { $$ = newOpNode(GE); }
            | EQ { $$ = newOpNode(EQ); }
            | NE { $$ = newOpNode(NE); }
            ;
add_expr    : add_expr addop term
                { $$ = newExpNode(OpK);
                  $$->attr.op = $2->attr.op;
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  free($2);
                }
            | term { $$ = $1; }
            ;
addop       : PLUS { $$ = newOpNode(PLUS); }
            | MINUS { $$ = newOpNode(MINUS); }
            ;
term        : term mulop factor
                { $$ = newExpNode(OpK);
                  $$->attr.op = $2->attr.op;
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  free($2);
                }
            | factor { $$ = $1; }
            ;
mulop       : TIMES { $$ = newOpNode(TIMES); }
            | OVER { $$ = newOpNode(OVER); }
            ;
factor      : LPAREN expr RPAREN { $$ = $2; }
            | var { $$ = $1; }
            | call { $$ = $1; }
            | NUM
                { $$ = newExpNode(ConstK);
                  $$->attr.val = atoi(tokenString);
                }
            ;
call        : ID { savedName = copyString(tokenString); }
              LPAREN args RPAREN
                { $$ = newExpNode(CallK);
                  $$->attr.name = savedName;
                  $$->child[0] = $4;
                }
            ;
args        : arg_list { $$ = $1; }
            | /* empty */ { $$ = NULL; }
            ;
arg_list    : arg_list COMMA expr
                { YYSTYPE t = $1;
                  while (t->sibling != NULL)
                    t = t->sibling;
                  t->sibling = $3;
                  $$ = $1;
                }
            | expr { $$ = $1; }
            ;
%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

