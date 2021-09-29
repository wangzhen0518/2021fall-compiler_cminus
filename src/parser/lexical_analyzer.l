%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

// #define DEBUG
#ifdef DEBUG
#define DEBUG_INFO(T) printf(T)
#else
#define DEBUG_INFO(T)
#endif

int lines;
int pos_start;
int pos_end;

void pass_node(const char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}

INTEGER [0-9]+
FLOATPOINT [0-9]+\.|[0-9]*\.[0-9]+
ID [a-zA-Z]+
noMul [^\*]
noMulLine [^\*\/]
COMMENT "/*"{noMul}*(\*+{noMulLine}{noMul}*)*\**"*/"
%%
 /* to do for students */
 /* two cases for you, pass_node will send flex"s token to bison */
"," { pos_start = pos_end; pos_end += 1; pass_node(yytext); return COMMA; }
";" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return SEMICOLON; }
"(" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return LPAREN; }
")" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return RPAREN; }
"[" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACKET; }
"]" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACKET; }
"{" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return LBRACE; }
"}" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return RBRACE; }
"=" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return EQ; }
"<"  { pos_start = pos_end; pos_end += 1; pass_node(yytext); return OP_LT; }
">"  { pos_start = pos_end; pos_end += 1; pass_node(yytext); return OP_GT; }
"<=" { pos_start = pos_end; pos_end += 2; pass_node(yytext); return OP_LE; }
">=" { pos_start = pos_end; pos_end += 2; pass_node(yytext); return OP_GE; }
"==" { pos_start = pos_end; pos_end += 2; pass_node(yytext); return OP_EQ; }
"!=" { pos_start = pos_end; pos_end += 2; pass_node(yytext); return OP_NEQ; }
"+" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD; }
"-" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return MINUS; }
"*" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return MUL; }
"/" { pos_start = pos_end; pos_end += 1; pass_node(yytext); return DIV; }
"if"      { pos_start = pos_end; pos_end += 2; pass_node(yytext); return IF;  }
"else"    { pos_start = pos_end; pos_end += 4; pass_node(yytext); return ELSE;  }
"while"   { pos_start = pos_end; pos_end += 5; pass_node(yytext); return WHILE; }
"return"  { pos_start = pos_end; pos_end += 6; pass_node(yytext); return RETURN; }
"void"    { pos_start = pos_end; pos_end += 4; pass_node(yytext); return VOID; }
"int"     { pos_start = pos_end; pos_end += 3; pass_node(yytext); return INT; }
"float"   { pos_start = pos_end; pos_end += 5; pass_node(yytext); return FLOAT; }
{INTEGER} { DEBUG_INFO("INTEGER: "); pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTEGER; }
{FLOATPOINT} { pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATPOINT; }
{ID} { DEBUG_INFO("ID: "); pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ID; }
{COMMENT} {
               char *t = yytext;
               int n = 0;
               while(*t!='\0')
               {     
                    if(*t=='\n')
                         n++;
                    t++;
               }
               lines += n;
               pos_start = pos_end;
               t = strrchr(yytext, '\n');
               if(t!=NULL)
                    pos_end = strlen(t) - 1; //pos_end以0为开始
               else
                    pos_end += strlen(yytext);
          }
" "|\t { pos_start = pos_end; pos_end++; }
\r\n|\n|\r { lines++; pos_start = pos_end = 0;}
. { DEBUG_INFO("error: "); pos_start = pos_end; pos_end++; return ERROR; }

 /****请在此补全所有flex的模式与动作  end******/
%%
