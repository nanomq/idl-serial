%{
#include <string.h>
#include "parser.h"
#include "dbg.h"

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
{Ignored}               { /*debug("Ignored: %s\n", yytext);*/ }
{Number}                { debug("Number type: %s", yytext); return NUMBER; }
{Integer}               { debug("Integer value: %s", yytext); return INTEGER; }
{String}                { debug("String type: %s", yytext); return STRING; }
{Boolean}               { debug("Boolean type: %s", yytext); return BOOLEAN; }
{Lcurly}                { debug("Lcurly type: %s", yytext); return LCURLY; }
{Rcurly}                { debug("Rcurly type: %s", yytext); return RCURLY; }
{Lbrac}                 { debug("Lbrac type: %s", yytext); return LBRAC; }
{Rbrac}                 { debug("Rbrac type: %s", yytext); return RBRAC; }
{Comma}                 { debug("Comma type: %s", yytext); return COMMA; }
{Semic}                 { debug("Semic type: %s", yytext); return SEMIC; }
{Enum}                  { debug("Enum type: %s", yytext); return ENUM; }
{Struct}                { debug("Struct type: %s", yytext); return STRUCT; }
{Variable}              { 
                            debug("Variable name: %s", yytext); 
                            yylval.strval = strdup(yytext); 
                            return VARIABLE; 
                        }
.                       { debug("Unrecognized %c", yytext[0]); }

%%
