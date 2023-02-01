%{
#include <string.h>
extern FILE *yyin;
%}

/* %option debug */
%option noyywrap

/* Ignored */
Comment                 (("//")[^\n]*)|("/*".*"*/")
WhiteSpace              ([ \t\r\n]+)
Ignored                 {WhiteSpace}|{Comment}

/* Symbol */
Lcurly                  "{"
Rcurly                  "}"
Lbrac                   "["
Rbrac                   "]"
Comma                   ","
Semic                   ";"

/* Number type */
Char                    char
Octet                   octet
Short                   ((unsigned[ ]+)?(short))
Long                    ((unsigned[ ]+)?(long|(long[ ]+long)))
Int                     (u?int(8|16|32|64))
Float                   (float|double)
Number                  {Char}|{Octet}|{Short}|{Long}|{Int}|{Float}
Hex                     (0x[0-9A-Fa-f]*)
Integer                 ([0-9]*)

/* Boolean type */
Boolean                 boolean

/* String type */
StrT                    (<({Hex}|{Integer})>)
String                  string{StrT}?

/* Enum && struct type */
Enum                    enum
Struct                  struct

/* Variable name */
Letter                  [A-Za-z]
Variable                {Letter}[A-Za-z0-9_]*

%%
{Ignored}               { /*printf("Ignored: %s\n", yytext);*/ }
{Number}                { printf("Number type: %s\n", yytext); }
{Integer}               { printf("Integer value: %s\n", yytext); }
{String}                { printf("String type: %s\n", yytext); }
{Boolean}               { printf("Boolean type: %s\n", yytext); }
{Lcurly}                { printf("Lcurly type: %s\n", yytext); }
{Rcurly}                { printf("Rcurly type: %s\n", yytext); }
{Lbrac}                 { printf("Lbrac type: %s\n", yytext); }
{Rbrac}                 { printf("Rbrac type: %s\n", yytext); }
{Comma}                 { printf("Comma type: %s\n", yytext); }
{Semic}                 { printf("Semic type: %s\n", yytext); }
{Enum}                  { printf("Enum type: %s\n", yytext); }
{Struct}                { printf("Struct type: %s\n", yytext); }
{Variable}              { printf("Variable name: %s\n", yytext); } 
.                       { printf("Unrecognized %c\n", yytext[0]); }

%%

int main(int argc, char **argv)
{
  
  if (!(yyin = fopen(argv[1], "r")))
	{
		perror((argv[1]));
		return 0;
	}
  yylex();
}
