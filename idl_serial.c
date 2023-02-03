#include "cvector.h"
#include "idl_serial.h"
#include "parser.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern FILE *yyin;

static char func_head[] =
	"\ncJSON *dds_to_mqtt_%s_convert(%s *st)"
	"\n{\n"
	"\n 	cJSON *obj     = NULL;"
	"\n 	/* Assemble cJSON* obj with *st. */"
	"\n 	obj = cJSON_CreateObject();\n";

static char func_tail[] =
	"\n\treturn obj;"
	"\n}\n\n";

const char *cJSON_Add(char *key)
{
	if (0 == strcmp(key, "BOOLEAN"))
	{
		return "cJSON_AddBoolToObject";
	}
	else if (0 == strcmp(key, "NUMBER"))
	{
		return "cJSON_AddNumberToObject";
	}
	else
	{
		return "Unsupport now";
	}
}

int idl_serial_generator(const char *file)
{
	int rv = 0;
	if (!(yyin = fopen(file, "r")))
	{
		perror((file));
		return -1;
	}

	cJSON *jso;
	rv = yyparse(&jso);
	if (0 != rv)
	{
		fprintf(stderr, "invalid data to parse!\n");
		return -1;
	}
	puts(cJSON_PrintUnformatted(jso));

	cJSON *arrs = jso;
	cJSON *arr = NULL;
	cJSON *eles = NULL;
	cJSON *ele = NULL;
	cJSON *e = NULL;

	cJSON_ArrayForEach(arr, arrs)
	{
		cJSON_ArrayForEach(eles, arr)
		{

			printf(func_head, eles->string, eles->string);

			cJSON_ArrayForEach(ele, eles)
			{
				cJSON_ArrayForEach(e, ele)
				{
					printf("\t%s(obj, \"%s\", st->%s);\n", cJSON_Add(e->string), e->string, e->valuestring);
				}
			}

			puts(func_tail);
		}
	}

	return 0;
}
