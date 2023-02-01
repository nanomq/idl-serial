%{
#include <string.h>
%}

/* %option debug */
%option noyywrap



/* Ignored */
Comment                 (("//")[^\n]*)|("/*".*"*/")
WhiteSpace              ([ \t\r\n]+)
Ignored                 {WhiteSpace}|{Comment}

/* Number */
Char                    char
Octet                   octet
Short                   ((unsigned[ ]+)?(short))
Long                    ((unsigned[ ]+)?(long|(long[ ]+long)))
Integer                 (u?int(8|16|32|64))
Float                   (float|double)
Number                  {Char}|{Octet}|{Short}|{Long}|{Integer}|{Float}

/* Boolean */
Boolean                 boolean

/* Variable */
Letter                  [A-Za-z]
Variable                {Letter}[A-Za-z0-9_]*

%%
{Ignored}               { printf("Ignored: %s\n", yytext); }
{Number}                { printf("Number type: %s\n", yytext); }
{Boolean}               { printf("Boolean type: %s\n", yytext); }
{Variable}              { printf("Variable name: %s\n", yytext); } 
.                       { printf("Unrecognized %c\n", yytext[0]); }

%%


int main(int argc, char **argv)
{
  yylex();
}
