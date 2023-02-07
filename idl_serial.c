#include "cvector.h"
#include "idl_serial.h"
#include "parser.h"
#include "cJSON.h"
#include "dbg.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern FILE *yyin;

static char g_num[] = "NUMBER";
static char g_arr[] = "ARRAY";
static char g_enu[] = "ENUM";
static char g_str[] = "STRING";

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

static char ser_str_func[] =
	"\ncJSON *dds_to_mqtt_%s_convert(char *str)"
	"\n{\n"
	"\n\tcJSON *obj = cJSON_CreateObject();"
	"\n\tcJSON_AddStringToObject(obj, \"%s\", str);"
	"\n\treturn obj;\n"
	"\n}\n";

static char deser_str_func[] =
	"\nchar *mqtt_to_dds_%s_convert(cJSON *jso)"
	"\n{\n"
	"\n\tcJSON *item = cJSON_GetObjectItem(jso, \"%s\");"
	"\n\treturn item->valuestring;"
	"\n}\n\n";

static char ser_arr_func[] =
	"\ncJSON *dds_to_mqtt_%s_convert(%s *arr)"
	"\n{\n"
	"\n\treturn cJSON_Create%sArray((const double *)arr, %d);\n"
	"\n}\n";

static char deser_arr_func[] =
	"\n%s *mqtt_to_dds_%s_convert(cJSON *jso)"
	"\n{"
	"\n\t%s *arr = (%s*) malloc(%d*sizeof(%s));"
	"\n\tcJSON *item = NULL;"
	"\n\tint i = 0;"
	"\n\tcJSON_ArrayForEach(item, jso) {"
	"\n\t\tarr[i++] = item->valuedouble;"
	"\n\t}"
	"\n\treturn arr;\n"
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

			if (0 == strncmp(eles->string, g_num, strlen(g_num)))
			{
				char *num_type = eles->string + strlen(g_num);
				printf(ser_num_func, eles->valuestring, num_type);
			}
			else if (0 == strncmp(eles->string, g_enu, strlen(g_enu)))
			{
				printf(ser_num_func, eles->valuestring, eles->valuestring);
			}
			else if (0 == strncmp(eles->string, g_str, strlen(g_str)))
			{
				printf(ser_str_func, eles->valuestring, eles->valuestring);
			}
			else if (0 == strncmp(eles->string, g_arr, strlen(g_arr)))
			{
				char *num_type = eles->string + strlen(g_arr);
				char *val_name = strchr(num_type, '_');
				*val_name++ = '\0';

				char u32[] = "uint32";
				char dou[] = "double";
				char flo[] = "float";
				char i64[] = "int64";


				if (NULL != strstr(num_type, "64") || NULL != strstr(num_type, "long") || NULL != strstr(num_type, u32) || NULL != strstr(num_type, dou) || NULL != strstr(num_type, flo)) {
					if (0 == strcmp("long", num_type)) {
						printf(ser_arr_func, val_name, num_type, "Int", eles->valueint);
					} else {
						printf(ser_arr_func, val_name, num_type, "Double", eles->valueint);
					}
				} else {
					printf(ser_arr_func, val_name, num_type, "Int", eles->valueint);
				}
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

			if (0 == strncmp(eles->string, g_num, strlen(g_num)))
			{
				char *num_type = eles->string + strlen(g_num);
				printf(deser_num_func, num_type, eles->valuestring);
			}
			else if (0 == strncmp(eles->string, g_enu, strlen(g_enu)))
			{
				printf(deser_num_func, eles->valuestring, eles->valuestring);
			}
			else if (0 == strncmp(eles->string, g_str, strlen(g_str)))
			{
				printf(deser_str_func, "string", eles->valuestring);
			}
			else if (0 == strncmp(eles->string, g_arr, strlen(g_arr)))
			{
				char *num_type = eles->string + strlen(g_arr);
				char *val_name = strlen(num_type) + num_type +1;
				printf(deser_arr_func, num_type, val_name, num_type, num_type, eles->valueint, num_type);
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
