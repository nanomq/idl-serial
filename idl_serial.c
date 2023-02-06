#include "cvector.h"
#include "idl_serial.h"
#include "parser.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern FILE *yyin;

static char ser_num_func[] =
	"\ncJSON *dds_to_mqtt_%s_convert(%s num)"
	"\n{\n"
	"\n\treturn cJSON_CreateNumber(num);\n"
	"\n}\n";

static char deser_num_func[] =
	"\n%s mqtt_to_dds_%s_convert(cJSON *jso)"
	"\n{\n"
	"\n\treturn cJSON_GetNumberValue(jso);\n"
	"\n}\n\n";

static char ser_func_head[] =
	"\ncJSON *dds_to_mqtt_%s_convert(%s *st)"
	"\n{\n"
	"\n 	cJSON *obj     = NULL;"
	"\n 	/* Assemble cJSON* obj with *st. */"
	"\n 	obj = cJSON_CreateObject();\n";

static char ser_func_tail[] =
	"\n\treturn obj;"
	"\n}\n\n";

static char deser_func_head[] =
	"\n%s *mqtt_to_dds_%s_convert(cJSON *jso)"
	"\n{"
	"\n\t%s *st = (%s) malloc(sizeof(*st));"
	"\n\tcJSON *item = NULL;\n";

static char deser_func_tail[] =
	"\n\treturn st;"
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

const char *cJSON_Get(char *key)
{
	if (0 == strcmp(key, "BOOLEAN"))
	{
		return "";
	}
	else if (0 == strcmp(key, "NUMBER"))
	{
		return "";
	}
	else
	{
		return "Unsupport now";
	}
}

int idl_serial_generator_to_json(cJSON *jso)
{
	cJSON *arrs = jso;
	cJSON *arr = NULL;
	cJSON *eles = NULL;
	cJSON *ele = NULL;
	cJSON *e = NULL;

	cJSON_ArrayForEach(arr, arrs)
	{
		cJSON_ArrayForEach(eles, arr)
		{

			char num[] = "NUMBER";
			char num_len = strlen(num);
			if (0 == strncmp(eles->string, num, num_len))
			{
				char *num_type = eles->string + num_len;
				printf(ser_num_func, eles->valuestring, num_type);
			}
			else
			{
				printf(ser_func_head, eles->string, eles->string);

				cJSON_ArrayForEach(ele, eles)
				{
					cJSON_ArrayForEach(e, ele)
					{
						printf("\t%s(obj, \"%s\", st->%s);\n", cJSON_Add(e->string), e->valuestring, e->valuestring);
					}
				}

				puts(ser_func_tail);
			}
		}
	}

	return 0;
}

int idl_serial_generator_to_struct(cJSON *jso)
{
	cJSON *arrs = jso;
	cJSON *arr = NULL;
	cJSON *eles = NULL;
	cJSON *ele = NULL;
	cJSON *e = NULL;

	cJSON_ArrayForEach(arr, arrs)
	{
		cJSON_ArrayForEach(eles, arr)
		{

			char num[] = "NUMBER";
			char num_len = strlen(num);
			if (0 == strncmp(eles->string, num, num_len))
			{
				char *num_type = eles->string + num_len;
				printf(deser_num_func, num_type, eles->valuestring);
			}
			else
			{
				printf(deser_func_head, eles->string, eles->string, eles->string, eles->string);

				cJSON_ArrayForEach(ele, eles)
				{
					cJSON_ArrayForEach(e, ele)
					{
						printf("\titem = cJSON_GetObjectItem(obj, \"%s\");\n", e->valuestring);
						printf("\tst->%s = item->valuedouble;\n", e->valuestring);
					}
				}

				puts(deser_func_tail);
			}
		}
	}

	return 0;
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

	idl_serial_generator_to_json(jso);
	idl_serial_generator_to_struct(jso);

	return 0;
}
