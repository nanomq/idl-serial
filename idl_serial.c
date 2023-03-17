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
extern int yyparse (struct cJSON **jso);

static char g_num[] = "NUMBER";
static char g_arr[] = "ARRAY";
static char g_enu[] = "ENUM";
static char g_str[] = "STRING";
static FILE *g_fp = NULL;
static FILE *g_hfp = NULL;

static char ser_num_func[] =
	"\ncJSON *dds_to_mqtt_%s_convert(%s *num)"
	"\n{\n"
	"\n\treturn cJSON_CreateNumber(*num);\n"
	"\n}\n";

static char deser_num_func[] =
	"\n%s mqtt_to_dds_%s_convert(cJSON *obj)"
	"\n{\n"
	"\n\treturn cJSON_GetNumberValue(obj);\n"
	"\n}\n\n";

static char ser_func_head[] =
	"\ncJSON *dds_to_mqtt_%s_convert(%s *st)"
	"\n{\n"
	"\n 	cJSON *obj = NULL;"
	"\n 	/* Assemble cJSON* obj with *st. */"
	"\n 	obj = cJSON_CreateObject();\n";

static char ser_func_tail[] =
	"\n\treturn obj;"
	"\n}\n\n";

static char deser_func_head[] =
	"\n%s *mqtt_to_dds_%s_convert(cJSON *obj)"
	"\n{"
	"\n\t%s *st = (%s*) malloc(sizeof(*st));"
	"\n\tcJSON *item = NULL;\n";

static char deser_func_tail[] =
	"\n\treturn st;"
	"\n}\n\n";

static char ser_func_call[] =
	"\n\tcJSON_AddItemToObject(obj, \"%s\", dds_to_mqtt_%s_convert(st->%s));\n";

static char deser_func_call[] =
	"\n\tst->%s = mqtt_to_dds_%s_convert(item);\n";

void cJSON_AddArrayCommon(char *p, char *val, char *type)
{
	char *p_b = p;
	int times = 0;
	char tab[32] = {0};
	tab[times] = '\t';

	if (strchr(p, '_'))
	{
		fprintf(g_fp, "%scJSON *%s%d = cJSON_CreateArray();\n", tab, val, times);
		fprintf(g_fp, "%scJSON_AddItemToObject(obj, \"%s\", %s%d)\n", tab, val, val, times);
	}
	else
	{
		int num = atoi(p);
		char tmp[64];
		size_t size = snprintf(tmp, 64, "st->%s", val);
		fprintf(g_fp, "%scJSON *%s = cJSON_Create%sArray(%s, %d);\n", tab, val, type, tmp, num);
		fprintf(g_fp, "%scJSON_AddItemToObject(obj, \"%s\", %s);\n", tab, val, val);
	}

	while (NULL != (p = strchr(p, '_')))
	{

		*p = '\0';
		int num = atoi(p_b);
		fprintf(g_fp, "%sfor (int i%d = 0; i%d < %d; i%d++) {\n", tab, times, times, num, times);

		*p++ = '_';
		p_b = p;

		if (strchr(p, '_'))
		{
			fprintf(g_fp, "%s\tcJSON *%s%d = cJSON_CreateArray();\n", tab, val, times + 1);
			fprintf(g_fp, "%s\tcJSON_AddItemToArray(%s%d, %s%d);\n", tab, val, times, val, times + 1);
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
			fprintf(g_fp, "%s\tcJSON *%s%d = cJSON_Create%sArray(%s, %d);\n", tab, val, times + 1, type, tmp, num);
			fprintf(g_fp, "%s\tcJSON_AddItemToArray(%s%d, %s%d);\n", tab, val, times, val, times + 1);
		}

		tab[++times] = '\t';
	}

	while (times)
	{
		tab[times--] = '\0';
		fprintf(g_fp, "%s}\n", tab);
	}
	fprintf(g_fp, "\n");
}

void cJSON_AddArray(char *key, char *val)
{

	char *p = key + strlen("ARRAY_");
	char *ret = NULL;

	fprintf(g_fp, "\n\t// %s\n", key);
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

void cJSON_AddSequence(char *key, char *val, int *times)
{

	// static int times = 0;
	static char tab[32] = {0};
	tab[*times] = '\t';

	fprintf(g_fp, "\n\n%s// %s", tab, key);
	char *p = key;
	char *p_b = p;
	char *ret = NULL;
	int i = 0;
	while ((p = strstr(p, "sequence")))
	{
		p_b = p;
		i++;
		p++;
	}
	if (1 == i)
	{
		p_b += strlen("sequence") + 1;
		if (0 == strncmp(p_b, "string", strlen("string")))
		{
			fprintf(g_fp, "\n%scJSON * %s_arr0 = cJSON_CreateStringArray(st->%s->_buffer, st->%s->_length);", tab, val, val, val);
		}
		else
		{
			fprintf(g_fp, "\n%scJSON * %s_arr0 = cJSON_CreateDoubleArray(st->%s->_buffer, st->%s->_length);", tab, val, val, val);
		}

		fprintf(g_fp, "\n%scJSON_AddItemToObject(obj, \"%s\", %s_arr0);", tab, val, val);
		return;
	}

	p = key;
	i = 0;
	char tmp[128] = {0};
	memset(tmp, 0, 128);
	size_t size = snprintf(tmp, 128, "%s", val);
	char *where = tmp + size;
	while ((p = strstr(p, "sequence")))
	{

		if (NULL == strstr(++p, "sequence"))
		{

			p += strlen("sequence");
			if (0 == strncmp(p, "string", strlen("string")))
			{
				fprintf(g_fp, "\n%scJSON * %s_arr%d = cJSON_CreateStringArray(st->%s->_buffer, st->%s->_length);", tab, val, i, tmp, tmp);
			}
			else
			{
				fprintf(g_fp, "\n%scJSON * %s_arr%d = cJSON_CreateDoubleArray(st->%s->_buffer, st->%s->_length);", tab, val, i, tmp, tmp);
			}
		}
		else
		{

			fprintf(g_fp, "\n%scJSON *%s_arr%d = cJSON_CreateArray();\n", tab, val, i);
			fprintf(g_fp, "\n%sfor (int i%d = 0; i%d < st->%s->_length; i%d++) {", tab, i, i, tmp, i);
			size = snprintf(where, 128, "->_buffer[i%d]", i);
			where = where + size;

			i++;
			(*times)++;
			tab[*times] = '\t';
		}
	}

	while (*times > 0)
	{
		fprintf(g_fp, "\n%scJSON_AddItemToArray(%s_arr%d, %s_arr%d);", tab, val, (*times) - 1, val, (*times));
		tab[(*times)--] = '\0';
		fprintf(g_fp, "\n%s}", tab);
	}

	fprintf(g_fp, "\n%scJSON_AddItemToObject(obj, \"%s\", %s_arr0);\n", tab, val, val);
}

void cJSON_Add(cJSON *jso)
{
	char *key = jso->string;
	char *val = jso->valuestring;

	if (0 == strcmp(key, "BOOLEAN"))
	{
		fprintf(g_fp, "\tcJSON_AddBoolToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strcmp(key, "NUMBER"))
	{
		fprintf(g_fp, "\tcJSON_AddNumberToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strcmp(key, "STRING"))
	{
		fprintf(g_fp, "\tcJSON_AddStringToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strncmp(key, "STRING_", strlen("STRING_")))
	{
		fprintf(g_fp, "\tcJSON_AddStringToObject(obj, \"%s\", st->%s);\n", val, val);
	}
	else if (0 == strncmp(key, "ARRAY", strlen("ARRAY")))
	{
		cJSON_AddArray(key, val);
	}
	else if (0 == strncmp(key, "sequence", strlen("sequence")))
	{
		int times = 0;
		cJSON_AddSequence(key, val, &times);
	}
	else
	{
		fprintf(g_fp, ser_func_call, val, key, val);
		// log_err("Unsupport now: %s", key);
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

	first_time ? fprintf(g_fp, "%sint i%d = 0;\n", tab, 0) : fprintf(g_fp, "%si%d = 0;\n", tab, 0);

	first_time = false;
	fprintf(g_fp, "%scJSON *%s%d = NULL;\n", tab, val, times);
	fprintf(g_fp, "%scJSON_ArrayForEach(item, %s%d) {\n", tab, val, times);
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
			fprintf(g_fp, "%s%s = strdup(%s%d->value%s);\n", tab, tmp, val, times - 1, type);
		}
		else if (0 == strncmp(type, "string_", strlen("string_")))
		{
			fprintf(g_fp, "%s%s = strcpy(%s%d->value%s);\n", tab, tmp, val, times - 1, type);
		}
		else
		{
			fprintf(g_fp, "%s%s = %s%d->value%s;\n", tab, tmp, val, times - 1, type);
		}
		tab[times--] = '\0';
	}

	while (NULL != (p = strchr(p, '_')))
	{

		*p = '\0';
		int num = atoi(p_b);

		fprintf(g_fp, "%sint i%d = 0;\n", tab, times);
		fprintf(g_fp, "%scJSON *%s%d = NULL;\n", tab, val, times);
		fprintf(g_fp, "%scJSON_ArrayForEach(%s%d, %s%d) {\n", tab, val, times - 1, val, times);
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

				fprintf(g_fp, "%s\tstrcpy(%s, %s%d->valuestring);\n", tab, tmp, val, times);
			}
			else
			{

				tab[++times] = '\t';
				fprintf(g_fp, "%sint i%d = 0;\n", tab, times);
				fprintf(g_fp, "%scJSON *%s%d = NULL;\n", tab, val, times);
				fprintf(g_fp, "%scJSON_ArrayForEach(%s%d, %s%d) {\n", tab, val, times - 1, val, times);
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
					fprintf(g_fp, "%s\t%s = strdup(%s%d->value%s);\n", tab, tmp, val, times, type);
				}
				else
				{
					fprintf(g_fp, "%s\t%s = %s%d->value%s;\n", tab, tmp, val, times, type);
				}
			}
			break;
		}

		tab[++times] = '\t';
	}

	int i = 0;
	while (times >= 0)
	{
		fprintf(g_fp, "%s\ti%d++;\n", tab, times);
		fprintf(g_fp, "%s}\n", tab);
		tab[times--] = '\0';
	}
	fprintf(g_fp, "\n");
}

void cJSON_GetArray(char *key, char *val)
{

	char *p = key + strlen("ARRAY_");
	char *ret = NULL;

	fprintf(g_fp, "\n\t// %s\n", key);
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

void clean_data(char *p)
{
	char *p_b = strrchr(p, '_');
	if (atoi(p_b+1)) {
		*p_b = '\0';
	}
	p_b = p;

	while ((p_b = strchr(p_b, ' '))) {
		*p_b = '_';
	} 

}


void cJSON_GetSequence(char *key, char *val, int *times)
{

	// static int times = 0;
	static char tab[32] = {0};
	tab[*times] = '\t';

	char *p = key;
	char *ret = NULL;

	fprintf(g_fp, "\n%s// %s", tab, key);

	// sequence<double> sl;
	int i = 0;
	char tmp[128] = {0};
	size_t size = snprintf(tmp, 128, "\n%scJSON *%s_array0", tab, val);
	char *where = tmp + size;
	while ((p = strstr(p, "sequence")))
	{
		size = snprintf(where, 128, ", *%s_array%d", val, i + 1);
		where = where + size;
		p++;
		i++;
	}

	fprintf(g_fp, "%s;", tmp);
	fprintf(g_fp, "\n%s%s_array0 = item;\n", tab, val);

	i = 0;
	p = key;
	memset(tmp, 0, 128);
	size = snprintf(tmp, 128, "%s", val);
	where = tmp + size;
	while ((p = strstr(p, "sequence")))
	{
		clean_data(p);
		fprintf(g_fp, "\n%ssize_t %s_sz%d = cJSON_GetArraySize(%s_array%d);", tab, val, i, val, i);
		fprintf(g_fp, "\n%sst->%s = dds_%s__alloc();", tab, tmp, p);
		fprintf(g_fp, "\n%sst->%s->_buffer = dds_%s_allocbuf(%s_sz%d);", tab, tmp, p, val, i);
		fprintf(g_fp, "\n%sint %s_i%d = 0;", tab, val, i);
		fprintf(g_fp, "\n%scJSON_ArrayForEach(%s_array%d, %s_array%d) {", tab, val, i + 1, val, i);

		size = snprintf(where, 128, "->_buffer[%s_i%d]", val, i);
		where = where + size;

		p++;
		i++;
		(*times)++;
		tab[*times] = '\t';
	}

	i = 0;
	p = key;
	memset(tmp, 0, 128);
	size = snprintf(tmp, 128, "\n%sst->%s", tab, val);
	where = tmp + size;
	char *p_b = p;
	while ((p = strstr(p, "sequence")))
	{
		size = snprintf(where, 128, "->_buffer[%s_i%d]", val, i);
		where = where + size;
		p += strlen("sequence_");
		i++;
		p_b = p;
	}

	if (0 == strncmp(p_b, "string_", strlen("string_")))
	{
		fprintf(g_fp, "strcpy(%s, cJSON_GetStringValue(%s_array%d));\n", tmp, val, i);
	}
	else if (0 == strncmp(p_b, "string", strlen("string")))
	{
		fprintf(g_fp, "%s = strdup(cJSON_GetStringValue(%s_array%d));\n", tmp, val, i);
	}
	else
	{
		fprintf(g_fp, "%s = cJSON_GetNumberValue(%s_array%d);\n", tmp, val, i);
	}

	while (*times > 0)
	{
		fprintf(g_fp, "%s%s_i%d++;\n", tab, val, (*times) - 1);
		tab[(*times)--] = '\0';
		fprintf(g_fp, "%s}\n", tab);
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
		snprintf(ret, 64, fmt_non_cp, name, "bool");
	}
	else if (0 == strcmp(type, "NUMBER"))
	{
		snprintf(ret, 64, fmt_non_cp, name, "double");
	}
	else if (0 == strncmp(type, "STRING_", strlen("STRING_")))
	{
		int len = 0;
		sscanf(type, "STRING_T_string_%d", &len);
		snprintf(ret, 64, fmt_cp, name, "string", len);
	}
	else if (0 == strcmp(type, "STRING"))
	{
		snprintf(ret, 64, fmt_dup, name, "string");
	}
	else if (0 == strncmp(type, "sequence_", strlen("sequence_")))
	{
		int times = 0;
		cJSON_GetSequence(type, name, &times);

		fprintf(g_fp, "\n");
		return;
	}
	else if (0 == strcmp(type, "STRING"))
	{
		snprintf(ret, 64, fmt_dup, name, "string");
	}
	else if (0 == strncmp(type, "ARRAY_", strlen("ARRAY_")))
	{
		cJSON_GetArray(type, name);
	}
	else
	{
		snprintf(ret, 64, deser_func_call, name, type);
		// log_err("Nonsupport now : %s", type);
		// return;
	}

	fprintf(g_fp, "%s\n", ret);
	return;
}

int idl_struct_to_json(cJSON *jso)
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
			if (0 == strncmp(eles->string, g_enu, strlen(g_enu)))
			{
				fprintf(g_fp, ser_num_func, eles->valuestring, eles->valuestring);
			}
			else
			{
				fprintf(g_fp, ser_func_head, eles->string, eles->string);

				cJSON_ArrayForEach(ele, eles)
				{
					cJSON_ArrayForEach(e, ele)
					{
						cJSON_Add(e);
					}
				}

				fprintf(g_fp, "%s\n", ser_func_tail);
			}
		}
	}

	return 0;
}

int idl_json_to_struct(cJSON *jso)
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

			if (0 == strncmp(eles->string, g_enu, strlen(g_enu)))
			{
				fprintf(g_fp, deser_num_func, eles->valuestring, eles->valuestring);
			}
			else
			{
				fprintf(g_fp, deser_func_head, eles->string, eles->string, eles->string, eles->string);

				cJSON_ArrayForEach(ele, eles)
				{
					cJSON_ArrayForEach(e, ele)
					{

						fprintf(g_fp, "\titem = cJSON_GetObjectItem(obj, \"%s\");\n", e->valuestring);
						cJSON_Get(e);
					}
				}

				fprintf(g_fp, "%s\n", deser_func_tail);
			}
		}
	}

	return 0;
}


int idl_append_header(const char *out, char *header)
{
	char name[32] = { 0 };
	sscanf(out, "%[^.]", name);
	sprintf(header, "#include \"%s.h\"", name);
	fprintf(g_fp, "%s\n", header);
	fprintf(g_fp, "#include <stdio.h>\n");
	fprintf(g_fp, "#include <string.h>\n");
	fprintf(g_fp, "#include <stdlib.h>\n");
	return 0;
}

int idl_serial_generator(const char *file, const char *out)
{
	int rv = 0;
	if (!(yyin = fopen(file, "r")))
	{
		fprintf(stderr, "Failed to open %s: %s\n", file, strerror(errno));
		return -1;
	}

	cJSON *jso;
	rv = yyparse(&jso);
	if (0 != rv)
	{
		fprintf(stderr, "invalid data to parse!\n");
		return -1;
	}

	char *str = cJSON_PrintUnformatted(jso);
	log_info("%s", str);


	g_fp = fopen(out, "w");

	char header[32] = { 0 };
	idl_append_header(out, header);
	g_hfp = fopen(header, "w");

	idl_struct_to_json(jso);
	idl_json_to_struct(jso);

	cJSON_free(str);
	cJSON_Delete(jso);
	fclose(g_fp);
	fclose(g_hfp);

	return 0;
}
