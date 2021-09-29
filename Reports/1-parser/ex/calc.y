/* calc.y */
%{
#include <stdio.h>
    int yylex(void);
    void yyerror(const char *s);
%}

%token RET
%token <num> NUMBER_0 NUMBER_N0
%token <op> ADDOP MUL DIV LPAREN RPAREN
%type <num> top line expr term factor factor_n0

%start top

%union {
    char   op;
    double num;
}

%%

top
: top line {}
| {}

line
: expr RET { printf(" = %f\n", $1); }

expr 
: term { $$ = $1; }
| expr ADDOP term
{
    switch ($2) {
    case '+': $$ = $1 + $3; break;
    case '-': $$ = $1 - $3; break;
    }
}

term
: factor { $$ = $1; }
| term MUL factor { $$ = $1 * $3; }
| term DIV factor_n0 { $$ = $1 / $3; }

factor
: LPAREN expr RPAREN { $$ = $2; }
| NUMBER_0 { $$ = $1; }
| NUMBER_N0 { $$ = $1; }

factor_n0
: LPAREN expr RPAREN { $$ = $2; }
| NUMBER_N0 { $$ = $1; }
| NUMBER_0 { printf("cannot divide 0\n"); }

%%

void yyerror(const char *s)
{
    fprintf(stderr, "%s\n", s);
}