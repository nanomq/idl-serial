%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dbg.h"
#define YYDEBUG 1

extern FILE *yyin;
extern int yylex();
extern void yyerror(const char*);

%}


%union {
    int intval;
    char *strval;
}


%token LCURLY RCURLY LBRAC RBRAC COMMA SEMIC
%token NUMBER STRING BOOLEAN ENUM STRUCT
%token <strval> VARIABLE;
%token <intval> INTEGER;


%type  value
%type  values
%type  member
%type  members
%%

start:  values
        | error
        ;

values: value                   { log_info("Value terminate");}
        | values value          { log_info("values SEMIC value"); }
        ;

value:  STRUCT VARIABLE LCURLY members RCURLY SEMIC { log_info("STRUCT VARIABLE LCURLY members RCURLY"); }
        | NUMBER VARIABLE SEMIC                     { log_info("INTEGER VARIABLE"); }
        | ENUM VARIABLE LCURLY members RCURLY SEMIC { log_info("ENUM VARIABLE LCURLY members RCURLY"); }
        ;

members: member                 { log_info("member"); }
        | members member        { log_info("members member"); }
        ;

member:  NUMBER VARIABLE SEMIC          { log_info("INTEGER VARIABLE SEMIC"); }
         | BOOLEAN VARIABLE SEMIC       { log_info("BOOLEAN VARIABLE SEMIC"); }
         | VARIABLE VARIABLE SEMIC      { log_info("VARIABLE VARIABLE SEMIC");}
         | VARIABLE COMMA               { log_info("VARIABLE SEMIC"); }
         | VARIABLE                     { log_info("VARIABLE"); }
         ;


%%

void yyerror(const char *s)
{
        fprintf(stderr, "error: %s\n", s);
}

int main(int argc, char **argv)
{

  if (!(yyin = fopen(argv[1], "r")))
	{
		perror((argv[1]));
		return 0;
	}
  yyparse();
}