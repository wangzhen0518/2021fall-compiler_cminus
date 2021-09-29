/* calc.l */
%option noyywrap

%{
/* 引入 calc.y 定义的 token */
#include "calc.tab.h"
%}

%%

\( { return LPAREN; }
\) { return RPAREN; }
"+"|"-" { yylval.op = yytext[0]; return ADDOP; }
"*" { yylval.op = yytext[0]; return MUL; }
"/" { yylval.op = yytext[0]; return DIV; }
[1-9][0-9]*|[1-9][0-9]*\.[0-9]*|[0]*\.[0]*[1-9][0-9]* { yylval.num = atof(yytext); return NUMBER_N0; }
0+|0*\.0+|0+\.0* { yylval.num = atof(yytext); return NUMBER_0; }
" "|\t {  }
\r\n|\n|\r { return RET; }

%%