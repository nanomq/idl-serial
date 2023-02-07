%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "dbg.h"
#include "cJSON.h"
#define YYDEBUG 1

extern FILE *yyin;
extern int yylex();
extern void yyerror(struct cJSON** jso, const char*);

%}

%parse-param {struct cJSON **jso}

%union {
    int intval;
    char *strval;
    struct cJSON *jsonval;
}


%token LCURLY RCURLY LBRAC RBRAC COMMA SEMIC
%token STRING BOOLEAN ENUM STRUCT
%token <strval> NUMBER;
%token <strval> VARIABLE;
%token <intval> INTEGER;



%type <jsonval> value
%type <jsonval> values
%type <jsonval> member
%type <jsonval> members
%%

start:  values
        {
                *jso = $1;
        }
        | error
        ;

values: value
        {
                log_info("Value terminate");
                $$ = cJSON_CreateArray();
                cJSON_AddItemToArray($$, $1);
        }
        | values value
        {
                log_info("values SEMIC value");
                cJSON_AddItemToArray($$, $2);
        }
        ;

value:  STRUCT VARIABLE LCURLY members RCURLY SEMIC
        {
                log_info("STRUCT VARIABLE LCURLY members RCURLY");
                $$ = cJSON_CreateObject();
                cJSON_AddItemToObject($$, $2, $4);
        }
        | NUMBER VARIABLE SEMIC
        {
                log_info("INTEGER VARIABLE");
                $$ = cJSON_CreateObject();
                char tmp[32] = {0};
                snprintf(tmp, 32, "NUMBER%s", $1);
                cJSON_AddStringToObject($$, tmp, $2);
        }
        | ENUM VARIABLE LCURLY members RCURLY SEMIC
        {
                log_info("ENUM VARIABLE LCURLY members RCURLY");
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, "ENUM", $2);
        }
        | NUMBER VARIABLE LBRAC INTEGER RBRAC SEMIC
        {
                log_info("NUMBER VARIABLE LBRAC INTEGER RBRAC");
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "ARRAY%s_%s", $1, $2);
                cJSON_AddNumberToObject($$, tmp, $4);
        }
        | BOOLEAN VARIABLE LBRAC INTEGER RBRAC SEMIC
        {
                log_info("BOOLEAN VARIABLE LBRAC INTEGER RBRAC");
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "ARRAY%s_%s", "bool", $2);
                cJSON_AddNumberToObject($$, tmp, $4);
        }
        | VARIABLE VARIABLE LBRAC INTEGER RBRAC SEMIC
        {
                log_info("VARIABLE VARIABLE LBRAC INTEGER RBRAC");
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "ARRAY%s%s", $1, $2);
                cJSON_AddNumberToObject($$, tmp, $4);

        }

        ;

members: member
        {
                log_info("member");
                $$ = cJSON_CreateArray();
                cJSON_AddItemToArray($$, $1);
        }
        | members member
        {
                log_info("members member");
                cJSON_AddItemToArray($$, $2);

        }
        ;

member: NUMBER VARIABLE SEMIC
        {
                log_info("NUMBER VARIABLE SEMIC");
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, "NUMBER", $2);
        }
        | BOOLEAN VARIABLE SEMIC
        {
                log_info("BOOLEAN VARIABLE SEMIC");
                cJSON_AddStringToObject($$, "BOOLEAN", $2);
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, "BOOLEAN", $2);

        }
        | VARIABLE VARIABLE SEMIC 
        {
                log_info("VARIABLE VARIABLE SEMIC");
                cJSON_AddStringToObject($$, $1, $2);
        }
        | VARIABLE COMMA
        {
                log_info("VARIABLE SEMIC");
        }
        | VARIABLE                     
        { 
                log_info("VARIABLE"); 
        }
        ;


%%

void yyerror(struct cJSON **jso, const char *s)
{
        fprintf(stderr, "error: %s\n", s);
}
