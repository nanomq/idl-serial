%{
#include <string.h>
%}

%option debug
%option noyywrap



Comment                 (("//")[^\n]*)|("/*".*"*/")
WhiteSpace              ([ \t\r\n]+)
Ignored                 {WhiteSpace}|{Comment}




%%
{Ignored}               { printf("Ignored %s\n", yytext); }
.                       { printf("Unrecognized %c\n", yytext[0]); }

%%


int main(int argc, char **argv)
{
  yylex();
}
