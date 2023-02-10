%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
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
    struct object *objval;
}


%token LCURLY RCURLY LBRAC RBRAC LMBRAC RMBRAC COMMA SEMIC
%token STRING SEQUENCE BOOLEAN ENUM STRUCT
%token <strval> NUMBER;
%token <strval> VARIABLE;
%token <intval> INTEGER;



%type <jsonval> value
%type <jsonval> values
%type <jsonval> member
%type <jsonval> members
%type <objval> data_type
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
        | STRING VARIABLE SEMIC
        {
                log_info("STRING VARIABLE");
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, "STRING", $2);
        }
        | STRING LBRAC INTEGER RBRAC VARIABLE SEMIC
        {
                log_info("STRING LBRAC INTEGER RBRAC VARIABLE SEMIC");
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, "STRING", $5);
        }
        | SEQUENCE LBRAC data_type RBRAC VARIABLE SEMIC
        {
                log_info("SEQUENCE LBRAC %s RBRAC VARIABLE SEMIC", $3->data);
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "SEQUENCE%s", $3->data);
                cJSON_AddStringToObject($$, tmp, $5);
        }
        | SEQUENCE LBRAC data_type COMMA INTEGER RBRAC VARIABLE SEMIC
        {
                log_info("SEQUENCE LBRAC %s COMMA INTEGER RBRAC VARIABLE SEMIC", $3->data);
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "SEQUENCE%s_%s", $3->data, $7);
                cJSON_AddNumberToObject($$, tmp, $5);
        }

        | ENUM VARIABLE LCURLY members RCURLY SEMIC
        {
                log_info("ENUM VARIABLE LCURLY members RCURLY");
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, "ENUM", $2);
        }
        | NUMBER VARIABLE LMBRAC INTEGER RMBRAC SEMIC
        {
                log_info("NUMBER VARIABLE LMBRAC INTEGER RMBRAC");
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "ARRAY%s_%s", $1, $2);
                cJSON_AddNumberToObject($$, tmp, $4);
        }
        | BOOLEAN VARIABLE LMBRAC INTEGER RMBRAC SEMIC
        {
                log_info("BOOLEAN VARIABLE LMBRAC INTEGER RMBRAC");
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "ARRAY%s_%s", "bool", $2);
                cJSON_AddNumberToObject($$, tmp, $4);
        }
        | VARIABLE VARIABLE LMBRAC INTEGER RMBRAC SEMIC
        {
                log_info("VARIABLE VARIABLE LMBRAC INTEGER RMBRAC");
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

member: data_type VARIABLE SEMIC
        {
                switch ($1->type)
                {
                case OBJECT_TYPE_NUMBER:
                        log_info("NUMBER VARIABLE SEMIC");
                        $$ = cJSON_CreateObject();
                        cJSON_AddStringToObject($$, "NUMBER", $2);
                        break;
                case OBJECT_TYPE_BOOLEAN:
                        log_info("BOOLEAN VARIABLE SEMIC");
                        $$ = cJSON_CreateObject();
                        cJSON_AddStringToObject($$, "BOOLEAN", $2);
                        break;
                case OBJECT_TYPE_STRING:
                        log_info("STRING VARIABLE SEMIC");
                        $$ = cJSON_CreateObject();
                        cJSON_AddStringToObject($$, "STRING", $2);
                        break;
                case OBJECT_TYPE_STRING_T:
                        log_info("STRING TEMPLATE VARIABLE SEMIC");
                        $$ = cJSON_CreateObject();
                        cJSON_AddStringToObject($$, $1->data, $2);
                        break;
                case OBJECT_TYPE_SEQUENCE:
                        log_info("SEQUENCE VARIABLE SEMIC");
                        break;
                default:
                        log_err("Unsupport type: %d", $1->type);
                        break;
                }

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


data_type: NUMBER
        {
                log_info("NUMBER");
                $$ = object_alloc(OBJECT_TYPE_NUMBER, $1);
        }
        | BOOLEAN
        {
                log_info("BOOLEAN");
                $$ = object_alloc(OBJECT_TYPE_BOOLEAN, "bool");
        }
        | STRING
        {
                log_info("STRING");
                $$ = object_alloc(OBJECT_TYPE_STRING, NULL);
        }
        | STRING LBRAC INTEGER RBRAC
        {
                log_info("STRING LBRAC INTEGER RBRAC");
                char tmp[32] = { 0 };
                snprintf(tmp, 32, "STRING_%d", $3);
                $$ = object_alloc(OBJECT_TYPE_STRING_T, tmp);
        }
        | SEQUENCE
        {
                log_info("SEQUENCE");
                $$ = object_alloc(OBJECT_TYPE_SEQUENCE, NULL);
        }

%%

void yyerror(struct cJSON **jso, const char *s)
{
        fprintf(stderr, "error: %s\n", s);
}
