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
Lbrac                   "<"
Rbrac                   ">"
LMbrac                  "["
RMbrac                  "]"
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
String                  string

/* Sequence type */
Sequence                sequence

/* Enum && struct type */
Enum                    enum
Struct                  struct

/* Variable name */
Letter                  [A-Za-z]
Variable                {Letter}[A-Za-z0-9_]*

/* Annotations */
Bool_type               true|TRUE|FALSE|false
Representation_type     {WhiteSpace}?((XCDR[12])|XML){WhiteSpace}?
Keylist                 {WhiteSpace}?keylist{WhiteSpace}?

Key                     @key
Bit_bound               @bit_bound\({Integer}\)
Final                   @final
External                @external
Optional                @optional(\({Bool_type}\))?
Appendable              @appendable
Mutable                 @mutable
Nested                  @nested
Autoid                  @autoid\((SEQUENTIAL|HASH)\)
Hashid                  @hashid(\([A-Za-z0-9]*\))?
Id                      @id\({Integer}\)
Value                   @value\({Integer}\)
Position                @position\({Integer}\)
Must_understand         @must_understand(\({Bool_type}\))?
Data_representation     @data_representation\((({Representation_type}\|)?)*{Representation_type}\)
Topic                   @topic
Default_nested          @default_nested(\({Bool_type}\))?
Pragma_keylist          #pragma{Keylist}
Macro                   #define{WhiteSpace}([A-Za-z_][A-Za-z0-9_]*){WhiteSpace}([^\n]*)


%%
{Ignored}               { /*debug("Ignored: %s\n", yytext);*/ }
{String}                { debug("String type: %s", yytext); return STRING; }
{Sequence}              { debug("Sequence type: %s", yytext); return SEQUENCE; }
{Boolean}               { debug("Boolean type: %s", yytext); return BOOLEAN; }
{Lcurly}                { debug("Lcurly type: %s", yytext); return LCURLY; }
{Rcurly}                { debug("Rcurly type: %s", yytext); return RCURLY; }
{Lbrac}                 { debug("Lbrac type: %s", yytext); return LBRAC; }
{Rbrac}                 { debug("Rbrac type: %s", yytext); return RBRAC; }
{LMbrac}                { debug("LMbrac type: %s", yytext); return LMBRAC; }
{RMbrac}                { debug("RMbrac type: %s", yytext); return RMBRAC; }
{Comma}                 { debug("Comma type: %s", yytext); return COMMA; }
{Semic}                 { debug("Semic type: %s", yytext); return SEMIC; }
{Enum}                  { debug("Enum type: %s", yytext); return ENUM; }
{Struct}                { debug("Struct type: %s", yytext); return STRUCT; }
{Integer}               {
                            debug("Integer value: %s", yytext);
                            yylval.intval = atoi(yytext);
                            return INTEGER;
                        }
{Number}                {
                            debug("Number type: %s", yytext);
                            yylval.strval = strdup(yytext);
                            return NUMBER;
                        }
{Variable}              { 
                            debug("Variable name: %s", yytext); 
                            yylval.strval = strdup(yytext); 
                            return VARIABLE; 
                        }
{Key}                   { debug("Key type: %s", yytext); }
{Bit_bound}             { debug("Bit bound type: %s", yytext); }
{Final}                 { debug("Final type: %s", yytext); }
{Appendable}            { debug("Appendable type: %s", yytext); }
{Mutable}               { debug("Mutable type: %s", yytext); }
{Nested}                { debug("Nested type: %s", yytext); }
{Autoid}                { debug("Autoid type: %s", yytext); }
{Hashid}                { debug("Hashid type: %s", yytext); }
{Id}                    { debug("Id type: %s", yytext); }
{Value}                 { debug("Value type: %s", yytext); }
{Position}              { debug("Position type: %s", yytext); }
{Must_understand}       { debug("Must_understand type: %s", yytext); }
{Data_representation}   { debug("Data_representation type: %s", yytext); }
{Topic}                 { debug("Topic type: %s", yytext); }
{Default_nested}        { debug("Default_nested type: %s", yytext); }
{Optional}              { debug("Optional type"); }
{External}              { debug("External type"); }
{Pragma_keylist}        {
                            debug("Pragma keylist type: %s", yytext);
                            return PRAGMA_KEYLIST;
                        }
{Macro}                 {
                            debug("Macro type: %s", yytext);
                            char *val = strstr(yytext, "#define") + strlen("#define");
                            while (*val == ' ') val++;
                            yylval.strval = strdup(val);
                            return MACRO;
                        }
.                       { debug("Unrecognized %c", yytext[0]); }

%%
