#include "cvector.h"
#include "idl_serial.h"
#include "parser.h"
#include "cJSON.h"
#include "dbg.h"
#include "cvector.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

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

void cJSON_AddArrayCommon(char *p, char *val, char *type)
{
	char *p_b = p;
	int times = 0;
	char tab[32] = {0};
	tab[times] = '\t';

	if (strchr(p, '_'))
	{
		printf("%scJSON *%s%d = cJSON_CreateArray();\n", tab, val, times);
		printf("%scJSON_AddItemToObject(obj, \"%s\", %s%d)\n", tab, val, val, times);
	}
	else
	{
		int num = atoi(p);
		char tmp[64];
		size_t size = snprintf(tmp, 64, "st->%s", val);
		printf("%scJSON *%s = cJSON_Create%sArray(%s, %d);\n", tab, val, type, tmp, num);
		printf("%scJSON_AddItemToObject(obj, \"%s\", %s);\n", tab, val, val);
	}

	while (p = strchr(p, '_'))
	{

		*p = '\0';
		int num = atoi(p_b);
		printf("%sfor (int i%d = 0; i%d < %d; i%d++) {\n", tab, times, times, num, times);

		*p++ = '_';
		p_b = p;

		if (strchr(p, '_'))
		{
			printf("%s\tcJSON *%s%d = cJSON_CreateArray();\n", tab, val, times + 1);
			printf("%s\tcJSON_AddItemToArray(%s%d, %s%d);\n", tab, val, times, val, times + 1);
		}
		else
		{
			num = atoi(p);
			char tmp[64];
			size_t size = snprintf(tmp, 64, "st->%s", val);
			char *where = tmp + size;
			for (int i = 0; i <= times; i++)
			{
				size = snprintf(where, 64, "[i%d]", i);
				where = where + size;
			}
			printf("%s\tcJSON *%s%d = cJSON_Create%sArray(%s, %d);\n", tab, val, times + 1, type, tmp, num);
			printf("%s\tcJSON_AddItemToArray(%s%d, %s%d);\n", tab, val, times, val, times + 1);
		}

		tab[++times] = '\t';
	}

	while (times)
	{
		tab[times--] = '\0';
		printf("%s}\n", tab);
	}
	printf("\n");
}

void cJSON_AddArray(char *key, char *val)
{

	char *p = key + strlen("ARRAY_");
	char *ret = NULL;

	printf("\n\t// %s\n", key);
	if (0 == strncmp(p, "BOOLEAN", strlen("BOOLEAN")))
	{

		p += strlen("BOOLEAN_bool_");
		cJSON_AddArrayCommon(p, val, "Number");
	}
	else if (0 == strncmp(p, "NUMBER", strlen("NUMBER")))
	{
		// NUMBER_uint16_100_200
		p += strlen("NUMBER_");
		p = strchr(p, '_') + 1;

		cJSON_AddArrayCommon(p, val, "Double");
	}
	else if (0 == strncmp(p, "STRING_T_", strlen("STRING_T_")))
	{
		// STRING_T_string_100_3222_2222
		p += strlen("STRING_T_string_");
		cJSON_AddArrayCommon(p, val, "String");
	}
	else if (0 == strncmp(p, "STRING", strlen("STRING")))
	{
		p += strlen("STRING_string_");
		cJSON_AddArrayCommon(p, val, "String");
	}
}

void cJSON_Add(cJSON *jso)
{
	char *key = jso->string;
	char *val = jso->valuestring;

	if (0 == strcmp(key, "BOOLEAN"))
	{
		printf("\tcJSON_AddBoolToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strcmp(key, "NUMBER"))
	{
		printf("\tcJSON_AddNumberToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strcmp(key, "STRING"))
	{
		printf("\tcJSON_AddStringToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strncmp(key, "STRING_", strlen("STRING_")))
	{
		printf("\tcJSON_AddStringToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strncmp(key, "ARRAY", strlen("ARRAY")))
	{
		cJSON_AddArray(key, val);
	}
	else
	{
		log_err("Unsupport now");
	}
	return;
}

void cJSON_GetArrayCommon(char *p, char *val, char *type)
{
	char *p_b = p;
	int times = 0;
	char tab[16] = {0};
	tab[times] = '\t';
	static bool first_time = true;

	first_time ? printf("%sint i%d = 0;\n", tab, 0) : printf("%si%d = 0;\n", tab, 0);

	first_time = false;
	printf("%scJSON *%s%d = NULL;\n", tab, val, times);
	printf("%scJSON_ArrayForEach(item, %s%d) {\n", tab, val, times);
	tab[++times] = '\t';

	if (strchr(p, '_'))
	{
		p = strchr(p, '_') + 1;
	}
	else
	{

		char tmp[64];
		size_t size = snprintf(tmp, 64, "st->%s", val);
		char *where = tmp + size;
		for (int i = 0; i < times; i++)
		{
			size = snprintf(where, 64, "[i%d]", i);
			where = where + size;
		}
		if (0 == strncmp(type, "string", strlen("string")))
		{
			printf("%s%s = strdup(%s%d->value%s);\n", tab, tmp, val, times, type);
		}
		else if (0 == strncmp(type, "string_", strlen("string_")))
		{
			printf("%s%s = strcpy(%s%d->value%s);\n", tab, tmp, val, times, type);
		}
		else
		{
			printf("%s%s = %s%d->value%s;\n", tab, tmp, val, times, type);
		}
		tab[times--] = '\0';
	}

	while (p = strchr(p, '_'))
	{

		*p = '\0';
		int num = atoi(p_b);

		printf("%sint i%d = 0;\n", tab, times);
		printf("%scJSON *%s%d = NULL;\n", tab, val, times);
		printf("%scJSON_ArrayForEach(%s%d, %s%d) {\n", tab, val, times - 1, val, times);
		*p++ = '_';
		p_b = p;

		if (NULL == strchr(p, '_'))
		{

			if (0 == strncmp(type, "string_", strlen("string_")))
			{
				char tmp[64];
				size_t size = snprintf(tmp, 64, "st->%s", val);
				char *where = tmp + size;
				for (int i = 0; i <= times; i++)
				{
					size = snprintf(where, 64, "[i%d]", i);
					where = where + size;
				}

				printf("%s\tstrcpy(%s, %s%d->valuestring);\n", tab, tmp, val, times);
			}
			else
			{

				tab[++times] = '\t';
				printf("%sint i%d = 0;\n", tab, times);
				printf("%scJSON *%s%d = NULL;\n", tab, val, times);
				printf("%scJSON_ArrayForEach(%s%d, %s%d) {\n", tab, val, times - 1, val, times);
				*p++ = '_';
				p_b = p;

				char tmp[64];
				size_t size = snprintf(tmp, 64, "st->%s", val);
				char *where = tmp + size;
				for (int i = 0; i <= times; i++)
				{
					size = snprintf(where, 64, "[i%d]", i);
					where = where + size;
				}
				if (0 == strncmp(type, "string", strlen("string")))
				{
					printf("%s\t%s = strdup(%s%d->value%s);\n", tab, tmp, val, times, type);
				}
				else
				{
					printf("%s\t%s = %s%d->value%s;\n", tab, tmp, val, times, type);
				}
			}
			break;
		}

		tab[++times] = '\t';
	}

	int i = 0;
	while (times >= 0)
	{
		printf("%s\ti%d++;\n", tab, times);
		printf("%s}\n", tab);
		tab[times--] = '\0';
	}
	printf("\n");
}

void cJSON_GetArray(char *key, char *val)
{

	char *p = key + strlen("ARRAY_");
	char *ret = NULL;

	printf("\n\t// %s\n", key);
	if (0 == strncmp(p, "BOOLEAN", strlen("BOOLEAN")))
	{

		p += strlen("BOOLEAN_bool_");
		cJSON_GetArrayCommon(p, val, "int");
	}
	else if (0 == strncmp(p, "NUMBER", strlen("NUMBER")))
	{
		p += strlen("NUMBER_");
		p = strchr(p, '_') + 1;

		cJSON_GetArrayCommon(p, val, "double");
	}
	else if (0 == strncmp(p, "STRING_T_", strlen("STRING_T_")))
	{
		// STRING_T_string_100_3222_2222
		p += strlen("STRING_T_string_");
		cJSON_GetArrayCommon(p, val, "string_");
	}
	else if (0 == strncmp(p, "STRING", strlen("STRING")))
	{
		p += strlen("STRING_string_");
		cJSON_GetArrayCommon(p, val, "string");
	}
}

void cJSON_Get(cJSON *item)
{
	char fmt_non_cp[] = "\tst->%s = item->value%s;\n";
	char fmt_cp[] = "\tstrncpy(st->%s, item->value%s, %d);\n";
	char fmt_dup[] = "\tstrdup(st->%s, item->value%s);\n";
	char *type = item->string;
	char *name = item->valuestring;

	char *ret = (char *)malloc(sizeof(char) * 64);
	if (NULL == ret)
	{
		log_err("malloc error");
	}

	if (0 == strcmp(type, "BOOLEAN"))
	{
		snprintf(ret, 64, fmt_non_cp, "bool", name);
	}
	else if (0 == strcmp(type, "NUMBER"))
	{
		snprintf(ret, 64, fmt_non_cp, "double", name);
	}
	else if (0 == strncmp(type, "STRING_", strlen("STRING_")))
	{
		int len = 0;
		sscanf(type, "STRING_T_string_%d", &len);
		snprintf(ret, 64, fmt_cp, "string", name, len);
	}
	else if (0 == strcmp(type, "STRING"))
	{
		snprintf(ret, 64, fmt_dup, "string", name);
	}
	else if (0 == strncmp(type, "SEQUENCE_", strlen("SEQUENCE_")))
	{
		int len = 0;
		sscanf(type, "SEQUENCE_%d", &len);
		snprintf(ret, 64, fmt_cp, "string", name, len);
	}
	else if (0 == strcmp(type, "STRING"))
	{
		snprintf(ret, 64, fmt_dup, "string", name);
	}
	else if (0 == strncmp(type, "ARRAY_", strlen("ARRAY_")))
	{
		cJSON_GetArray(type, name);
	}
	else
	{
		log_err("Nonsupport now : %s", type);
		return;
	}

	puts(ret);
	return;
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

				if (NULL != strstr(num_type, "64") || NULL != strstr(num_type, "long") || NULL != strstr(num_type, u32) || NULL != strstr(num_type, dou) || NULL != strstr(num_type, flo))
				{
					if (0 == strcmp("long", num_type))
					{
						printf(ser_arr_func, val_name, num_type, "Int", eles->valueint);
					}
					else
					{
						printf(ser_arr_func, val_name, num_type, "Double", eles->valueint);
					}
				}
				else
				{
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
						cJSON_Add(e);
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
				char *val_name = strlen(num_type) + num_type + 1;
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
						cJSON_Get(e);
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
