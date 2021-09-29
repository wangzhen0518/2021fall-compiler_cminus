%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart();
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
syntax_tree_node *node(const char *node_name, int children_num, ...);
%}

/* TODO: Complete this definition.
   Hint: See pass_node(), node(), and syntax_tree.h.
         Use forward declaring. */
%code requires { #include "syntax_tree.h" }
%union {
    syntax_tree_node *node;
}

/* TODO: Your tokens here. */
%token <node> ERROR
%token <node> IF ELSE WHILE RETURN VOID INT FLOAT
%token <node> ID INTEGER FLOATPOINT COMMENT
 // %token <node> ADD MINUS MUL DIV
%token <node> OP_EQ OP_LE OP_LT OP_GE OP_GT OP_NEQ
%type <node> program
%type <node> type-specifier relop addop mulop
%type <node> declaration-list declaration var-declaration fun-declaration local-declarations
%type <node> compound-stmt statement-list statement expression-stmt iteration-stmt selection-stmt return-stmt
%type <node> expression simple-expression var additive-expression term factor integer float call
%type <node> params param-list param args arg-list

%start program

%%
/* TODO: Your rules here. */

program
: declaration-list { $$ = node( "program", 1, $1); gt->root = $$; }

declaration-list
: declaration-list declaration { $$ = node( "declaration-list", 2, $1, $2); }
| declaration { $$ = node( "declaration-list", 1, $1); }

declaration
: var-declaration { $$ = node( "declaration", 1, $1); }
| fun-declaration { $$ = node( "declaration", 1, $1); }

var-declaration
: type-specifier ID ';' { $$ = node( "var-declaration", 3, $1, $2, new_syntax_tree_node(";")); } //TODO:
| type-specifier ID '[' INTEGER ']' ';' { $$ = node( "var-declaration", 6, $1, $2, new_syntax_tree_node("["), $4, new_syntax_tree_node("]"), new_syntax_tree_node(";")); }

type-specifier //TODO:
: INT { $$ = node( "type-specifier", 1, $1); }
| FLOAT { $$ = node( "type-specifier", 1, $1); }
| VOID { $$ = node( "type-specifier", 1, $1); }

fun-declaration
: type-specifier ID '(' params ')' compound-stmt { $$ = node( "fun-declaration", 6, $1, $2, new_syntax_tree_node("("), $4, new_syntax_tree_node(")"), $6); } //TODO:

params
: param-list { $$ = node( "params", 1, $1); }
| VOID { $$ = node( "params", 1, $1); }

param-list
: param-list ',' param { $$ = node( "param-list", 3, $1, new_syntax_tree_node(","), $3); } //TODO:
| param { $$ = node( "param-list", 1, $1); }

param
: type-specifier ID { $$ = node( "param", 2, $1, $2); }
| type-specifier ID '[' ']' { $$ = node( "param", 4, $1, $2, new_syntax_tree_node("["), new_syntax_tree_node("]")); } //TODO:

compound-stmt
: '{' local-declarations statement-list '}' { $$ = node( "compound-stmt", 4, new_syntax_tree_node("{"), $2, $3, new_syntax_tree_node("}")); } //TODO:

local-declarations
: local-declarations var-declaration { $$ = node( "local-declarations", 2, $1, $2); }
| empty { $$ = node( "local-declarations", 0); }

statement-list
: statement-list statement { $$ = node( "statement-list", 2, $1, $2); }
| empty { $$ = node( "statement-list", 0); }

statement
: expression-stmt { $$ = node( "statement", 1, $1); }
| compound-stmt { $$ = node( "statement", 1, $1); }
| selection-stmt { $$ = node( "statement", 1, $1); }
| iteration-stmt { $$ = node( "statement", 1, $1); }
| return-stmt { $$ = node( "statement", 1, $1); }

expression-stmt
: expression ';' { $$ = node( "expression-stmt", 2, $1, new_syntax_tree_node(";")); } //TODO:
| ';' { $$ = node( "expression-stmt", 1, new_syntax_tree_node(";")); } //TODO:

selection-stmt
: IF '(' expression ')' statement { $$ = node( "selection-stmt", 5, $1, new_syntax_tree_node("("), $3, new_syntax_tree_node(")"), $5); } //TODO:
| IF '(' expression ')' statement ELSE statement { $$ = node( "selection-stmt", 7, $1, new_syntax_tree_node("("), $3, new_syntax_tree_node(")"), $5, $6, $7); } //TODO:

iteration-stmt
: WHILE '(' expression ')' statement { $$ = node( "iteration-stmt", 5, $1, new_syntax_tree_node("("), $3, new_syntax_tree_node(")"), $5); } //TODO:

return-stmt
: RETURN ';' { $$ = node( "return-stmt", 2, $1, new_syntax_tree_node(";")); } //TODO:
| RETURN expression ';' { $$ = node( "return-stmt", 3, $1, $2, new_syntax_tree_node(";")); } //TODO:

expression
: var '=' expression { $$ = node( "expression", 3, $1, new_syntax_tree_node("="), $3); } //TODO:
| simple-expression { $$ = node( "expression", 1, $1); }

var
: ID { $$ = node( "var", 1, $1); }
| ID '[' expression ']' { $$ = node( "var", 4, $1, new_syntax_tree_node("["), $3, new_syntax_tree_node("]")); } //TODO:

simple-expression
: additive-expression relop additive-expression { $$ = node( "simple-expression", 3, $1, $2, $3); }
| additive-expression { $$ = node( "simple-expression", 1, $1); }

relop
: OP_LE  { $$ = node( "relop", 1, $1); }
| OP_LT  { $$ = node( "relop", 1, $1); }
| OP_GE  { $$ = node( "relop", 1, $1); }
| OP_GT  { $$ = node( "relop", 1, $1); }
| OP_EQ  { $$ = node( "relop", 1, $1); }
| OP_NEQ { $$ = node( "relop", 1, $1); }

additive-expression
: additive-expression addop term { $$ = node( "additive-expression", 3, $1, $2, $3); }
| term { $$ = node( "additive-expression", 1, $1); }

addop
: '+' { $$ = node( "addop", 1, new_syntax_tree_node("+")); } //TODO:
| '-' { $$ = node( "addop", 1, new_syntax_tree_node("-")); } //TODO:

term
: term mulop factor { $$ = node( "term", 3, $1, $2, $3); }
| factor { $$ = node( "term", 1, $1); }

mulop
: '*' { $$ = node( "mulop", 1, new_syntax_tree_node("*")); } //TODO:
| '/' { $$ = node( "mulop", 1, new_syntax_tree_node("/")); } //TODO:

factor
: '(' expression ')' { $$ = node( "factor", 3, new_syntax_tree_node("("), $2, new_syntax_tree_node(")")); } //TODO:
| var { $$ = node( "factor", 1, $1); }
| call { $$ = node( "factor", 1, $1); }
| integer { $$ = node( "factor", 1, $1); }
| float { $$ = node( "factor", 1, $1); }

integer : INTEGER { $$ = node( "integer", 1, $1); }
float : FLOATPOINT { $$ = node( "float", 1, $1); }

call
: ID '(' args ')' { $$ = node( "call", 4, $1, new_syntax_tree_node("("), $3, new_syntax_tree_node(")")); } //TODO:

args
: arg-list { $$ = node( "args", 1, $1); }
| empty { $$ = node("args", 0); }

arg-list
: arg-list ',' expression { $$ = node( "arg-list", 3, $1, new_syntax_tree_node(","), $3); } //TODO:
| expression { $$ = node( "arg-list", 1, $1); }

empty: {}

%%

/// The error reporting function.
void yyerror(const char * s)
{
    // TO STUDENTS: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

/// A helper function to quickly construct a tree node.
///
/// e.g. $$ = node("program", 1, $1);
syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
