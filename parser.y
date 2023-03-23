%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
#include "dbg.h"
#include "cJSON.h"
#include "cvector.h"
#define YYDEBUG 1

extern FILE *yyin;
extern int yylex();
extern void yyerror(struct cJSON** jso, const char*);
static cJSON *map = NULL;

%}

%parse-param {struct cJSON **jso}

%union {
    int intval;
    int *intsval;
    char *strval;
    struct cJSON *jsonval;
    struct object *objval;
}


%token LCURLY RCURLY LBRAC RBRAC LMBRAC RMBRAC COMMA SEMIC
%token STRING SEQUENCE BOOLEAN ENUM STRUCT
%token <strval> NUMBER;
%token <strval> VARIABLE;
%token <strval> MACRO;
%token <intval> INTEGER;



%type <jsonval> value
%type <jsonval> values
%type <jsonval> member
%type <jsonval> members
%type <objval> data_type
%type <intval> data_dim
%type <intsval> data_dims
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
                log_info("values value");
                if ($2) cJSON_AddItemToArray($$, $2);
        }
        ;

value:  STRUCT VARIABLE LCURLY members RCURLY SEMIC
        {
                log_info("STRUCT VARIABLE LCURLY members RCURLY");
                $$ = cJSON_CreateObject();
                cJSON_AddItemToObject($$, $2, $4);
        }
        | ENUM VARIABLE LCURLY members RCURLY SEMIC
        {
                log_info("ENUM VARIABLE LCURLY members RCURLY");
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, "ENUM", $2);
        }
        | VARIABLE VARIABLE LMBRAC INTEGER RMBRAC SEMIC
        {
                log_info("VARIABLE VARIABLE LMBRAC INTEGER RMBRAC");
                $$ = cJSON_CreateObject();
                char tmp[64] = {0};
                snprintf(tmp, 64, "ARRAY%s%s", $1, $2);
                cJSON_AddNumberToObject($$, tmp, $4);
        }
        | MACRO
        {
                log_info("MACRO");
                char *val = $1;
                while (*val != ' ') val++;
                *val++ = '\0';
                while (*val == ' ') val++;
                if (map == NULL) {
                        map = cJSON_CreateObject();
                }
                cJSON_AddStringToObject(map, $1, val);
                $$ = NULL;
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
                        char tmp[64] = { 0 }; 
                        snprintf(tmp, 64, "STRING_T_%s", $1->data);
                        $$ = cJSON_CreateObject();
                        cJSON_AddStringToObject($$, tmp, $2);
                        break;
                case OBJECT_TYPE_ARRAY:
                        log_info("ARRAY VARIABLE SEMIC");
                        $$ = cJSON_CreateObject();
                        cJSON_AddStringToObject($$, $1->data, $2);
                        break;
                case OBJECT_TYPE_SEQUENCE:
                        log_info("SEQUENCE VARIABLE SEMIC");
                        snprintf(tmp, 64, "sequence_%s", $1->data);
                        $$ = cJSON_CreateObject();
                        cJSON_AddStringToObject($$, tmp, $2);
                        break;
                default:
                        log_err("Unsupport type: %d", $1->type);
                        break;
                }

        }
        | data_type VARIABLE data_dims SEMIC
        {
                log_info("LBRAC data_type RBRAC");
                char tmp[64];
                size_t size = 0;

                switch ($1->type)
                {
                case OBJECT_TYPE_NUMBER:
                        size = snprintf(tmp, 64, "ARRAY_NUMBER_%s", $1->data);
                        break;
                case OBJECT_TYPE_BOOLEAN:
                        size = snprintf(tmp, 64, "ARRAY_BOOLEAN_%s", $1->data);
                        break;
                case OBJECT_TYPE_STRING:
                        size = snprintf(tmp, 64, "ARRAY_STRING_%s", $1->data);
                        break;
                case OBJECT_TYPE_STRING_T:
                        size = snprintf(tmp, 64, "ARRAY_STRING_T_%s", $1->data);
                        break;
                default:
                        log_err("Unsupport type: %d", $1->type);
                        break;
                }

                char *where = tmp + size;
                for (int i = 0; i < cvector_size($3); i++) {
                        size = snprintf(where, 64, "_%d", $3[i]);
                        where = where + size;
                }
                $$ = cJSON_CreateObject();
                cJSON_AddStringToObject($$, tmp, $2);
        }
        | VARIABLE VARIABLE SEMIC 
        {
                log_info("VARIABLE VARIABLE SEMIC");
                $$ = cJSON_CreateObject();
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
                $$ = object_alloc(OBJECT_TYPE_STRING, "string");
        }
        | STRING LBRAC INTEGER RBRAC
        {
                log_info("STRING LBRAC INTEGER RBRAC");
                char tmp[32] = { 0 };
                snprintf(tmp, 32, "string_%d", $3);
                $$ = object_alloc(OBJECT_TYPE_STRING_T, tmp);
        }
        | SEQUENCE LBRAC data_type RBRAC
        {

                log_info("SEQUENCE LBRAC data_type RBRAC");
                char tmp[64] = { 0 };

                 switch ($3->type)
                 {
                 case OBJECT_TYPE_NUMBER:
                 case OBJECT_TYPE_BOOLEAN:
                 case OBJECT_TYPE_STRING:
                         log_info("NUMBER/BOOLEAN/STRING VARIABLE SEMIC");
                         snprintf(tmp, 64, "%s", $3->data);
                         break;
                 case OBJECT_TYPE_STRING_T:
                         log_info("STRING TEMPLATE VARIABLE SEMIC");
                         snprintf(tmp, 64, "%s", $3->data);
                         break;
                 case OBJECT_TYPE_ARRAY:
                         log_info("ARRAY VARIABLE SEMIC");
                         snprintf(tmp, 64, "%s", $3->data);
                         break;
                 case OBJECT_TYPE_SEQUENCE:
                         snprintf(tmp, 64, "sequence_%s", $3->data);
                         log_info("SEQUENCE VARIABLE SEMIC");
                         break;
                 default:
                         log_err("Unsupport type: %d", $3->type);
                         break;
                 }
 

                $$ = object_alloc(OBJECT_TYPE_SEQUENCE, tmp);
                log_info("%s", tmp);

        }
        | SEQUENCE LBRAC data_type COMMA INTEGER RBRAC
        {
                log_info("SEQUENCE LBRAC data_type COMMA INTEGER RBRAC");
                $$ = object_alloc(OBJECT_TYPE_SEQUENCE, $3->data);
                log_info("%s", $3->data);
        }

data_dims: data_dim
        {
                log_info("data_dim");
                $$ = NULL;
                cvector_push_back($$, $1);
        }
        | data_dims data_dim
        {
                log_info("data_dims data_dim");
                cvector_push_back($$, $2);
        }

data_dim: LMBRAC INTEGER RMBRAC
        {
                log_info("LMBRAC INTEGER RMBRAC");
                $$ = $2;
        }
        | LMBRAC VARIABLE RMBRAC
        {
                log_info("LMBRAC VARIABLE RMBRAC");
                cJSON *ele = NULL;
                char *val = NULL;
                cJSON_ArrayForEach(ele, map) {
                        if (ele && strcmp(ele->string, $2) == 0) {
                                val = ele->valuestring;
                                break;
                        }
                }

                long rc = atol(val);
                if (rc == 0) {
                        sscanf(val, "(%ld)", &rc);
                }
                $$ = rc;
        }



%%

void yyerror(struct cJSON **jso, const char *s)
{
        fprintf(stderr, "error: %s\n", s);
}
