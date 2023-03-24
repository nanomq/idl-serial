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
extern int yyparse(struct cJSON **jso);
extern char **keylist_vec;

static char g_num[] = "NUMBER";
static char g_arr[] = "ARRAY";
static char g_enu[] = "ENUM";
static char g_str[] = "STRING";
static FILE *g_fp = NULL;
static FILE *g_hfp = NULL;
static bool first_time = true;
static char g_map[4096] = {0};
static int g_map_cursor = 0;
static int g_map_num = 0;

static char ser_num_func[] =
	"\nstatic cJSON *"
	"\ndds_to_mqtt_%s_convert(%s *enu)"
	"\n{\n"
	"\n\tif (enu == NULL) {"
	"\n\t\tDDS_FATAL(\"enu is NULL\\n\");"
	"\n\t\treturn NULL;"
	"\n\t}\n"
	"\n\treturn cJSON_CreateNumber(*enu);\n"
	"\n}\n";

static char deser_num_func[] =
	"\nstatic int"
	"\nmqtt_to_dds_%s_convert(cJSON *obj, %s *enu)"
	"\n{\n"
	"\n\tif (obj == NULL || enu == NULL) {"
	"\n\t\tDDS_FATAL(\"obj or enu is NULL\\n\");"
	"\n\t\treturn -1;"
	"\n\t}\n"
	"\n\t*enu = cJSON_GetNumberValue(obj);\n"
	"\n\treturn 0;\n"
	"\n}\n\n";

static char ser_func_head[] =
	"\nstatic cJSON *"
	"\ndds_to_mqtt_%s_convert(%s *st)"
	"\n{\n"
	"\n\tif (st == NULL) {"
	"\n\t\tDDS_FATAL(\"st is NULL\\n\");"
	"\n\t\treturn NULL;"
	"\n\t}\n"
	"\n\tcJSON *obj = NULL;"
	"\n\t/* Assemble cJSON* obj with *st. */"
	"\n\tobj = cJSON_CreateObject();\n";

static char ser_func_tail[] =
	"\n\treturn obj;"
	"\n}\n\n";

static char deser_func_head[] =
	"\nstatic int"
	"\nmqtt_to_dds_%s_convert(cJSON *obj, %s *st)"
	"\n{"
	"\n\tif (obj == NULL || st == NULL) {"
	"\n\t\tDDS_FATAL(\"obj or st is NULL\\n\");"
	"\n\t\treturn -1;"
	"\n\t}\n"
	"\n\tint rc = 0;"
	"\n\tcJSON *item = NULL;\n";

static char deser_func_tail[] =
	"\n\treturn 0;"
	"\n}\n\n";

static char ser_func_call[] =
	"\n\tcJSON_AddItemToObject(obj, \"%s\", dds_to_mqtt_%s_convert(&st->%s));\n";

static char deser_func_call[] =
	"\n\trc = mqtt_to_dds_%s_convert(item, &st->%s);"
	"\n\tif (rc != 0) {"
	"\n\t\treturn rc;"
	"\n\t}\n";


static char init_format[] = 
	"\nstatic void *"
	"\n%s_init(void)"
	"\n{"
	"\n	return %s__alloc();"
	"\n}\n";

static char fini_format[] =
	"\nstatic void"
	"\n%s_fini(void *sample, dds_free_op_t op)"
	"\n{"
	"\n	return %s_free(sample, op);"
	"\n}\n";


static char map_format[] =
	"\t{\n"
	"\t\t\"%s\",\n"
	"\t\t{\n"
	"\t\t\t&%s_desc,\n"
	"\t\t\t%s_init,\n"
	"\t\t\t%s_fini,\n"
	"\t\t\t(mqtt_to_dds_fn_t) mqtt_to_dds_%s_convert,\n"
	"\t\t\t(dds_to_mqtt_fn_t) dds_to_mqtt_%s_convert\n"
	"\t\t}\n"
	"\t},\n";

typedef enum
{
	arr_t,
	obj_t
} type;

static void AddArrayCommonHelper(char *tab, char *val, int *times, int num, type t)
{

	fprintf(g_fp, "%scJSON *%s = cJSON_CreateArray();\n", tab, val);
	switch (t)
	{
	case obj_t:
		fprintf(g_fp, "%scJSON_AddItemToObject(obj, \"%s\", %s);\n", tab, val, val);
		break;
	case arr_t:
		fprintf(g_fp, "%scJSON_AddItemToArray(%s%d, %s%d);\n", tab, val, *times, val, *times + 1);
		break;
	default:
		break;
	}
	fprintf(g_fp, "%sfor (int i = 0; (i < (size_t) %d); i++) {\n", tab, num);
	tab[++(*times)] = '\t';
	fprintf(g_fp, "%scJSON *n = cJSON_CreateNumber(st->%s[i]);\n", tab, val);
	fprintf(g_fp, "%scJSON_AddItemToArray(%s, n);\n", tab, val);
}

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
		AddArrayCommonHelper(tab, val, &times, num, obj_t);
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

			AddArrayCommonHelper(tab, val, &times, num, arr_t);
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
	}
	return;
}

bool is_convert(char *type)
{
	if (strstr(type, "int"))
	{
		return true;
	}

	return false;
}

void cJSON_GetArrayCommon(char *p, char *val, char *type)
{
	char *p_b = p;
	int times = 0;
	char tab[16] = {0};
	tab[times] = '\t';

	first_time ? fprintf(g_fp, "%sint i%d = 0;\n", tab, 0) : fprintf(g_fp, "%si%d = 0;\n", tab, 0);

	first_time = false;
	fprintf(g_fp, "%scJSON *%s%d = NULL;\n", tab, val, times);
	fprintf(g_fp, "%scJSON_ArrayForEach(%s%d, item) {\n", tab, val, times);
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
			fprintf(g_fp, "strcpy(%s%s, %s%d->value%s);\n", tab, tmp, val, times - 1, type);
		}
		else
		{
			char t[16] = {0};
			if (is_convert(type))
			{
				fprintf(g_fp, "%s%s = (%s_t) %s%d->valuedouble;\n", tab, tmp, type, val, times - 1);
			} else {
				fprintf(g_fp, "%s%s = (%s) %s%d->valuedouble;\n", tab, tmp, type, val, times - 1);
			}

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
					char t[16] = {0};
					if (is_convert(type))
					{
						sprintf(t, "%s_t", type);
					}
					fprintf(g_fp, "%s\t%s = (%s) %s%d->valuedouble;\n", tab, tmp, t, val, times);
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
}

void cJSON_GetArray(char *key, char *val)
{

	char *p = key + strlen("ARRAY_");
	char *ret = NULL;

	fprintf(g_fp, "\n\t// %s\n", key);
	if (0 == strncmp(p, "BOOLEAN", strlen("BOOLEAN")))
	{
		p += strlen("BOOLEAN_bool_");
		cJSON_GetArrayCommon(p, val, "bool");
	}
	else if (0 == strncmp(p, "NUMBER", strlen("NUMBER")))
	{
		char *p_b = p + strlen("NUMBER_");
		p = strchr(p_b, '_');
		*p = '\0';
		p++;

		cJSON_GetArrayCommon(p, val, p_b);
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
	if (atoi(p_b + 1))
	{
		*p_b = '\0';
	}
	p_b = p;

	while ((p_b = strchr(p_b, ' ')))
	{
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
	char fmt_dup[] = "\tst->%s = strdup(item->value%s);\n";
	char *type = item->string;
	char *name = item->valuestring;

	char *ret = (char *)calloc(1, sizeof(char) * 128);
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
		snprintf(ret, 128, deser_func_call, type, name);
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
			char *str = eles->string;
			char *vstr = eles->valuestring;

			if (0 == strncmp(str, g_enu, strlen(g_enu)))
			{
				fprintf(g_fp, ser_num_func, vstr, vstr);
			}
			else
			{
				fprintf(g_fp, ser_func_head, str, str);
				
				for (int i = 0; i < cvector_size(keylist_vec); i++) {
					if (0 == strcmp(str, keylist_vec[i])) {
						int size = sprintf(g_map + g_map_cursor, init_format, str, str);
						g_map_cursor += size;
						size = sprintf(g_map + g_map_cursor, fini_format, str, str);
						g_map_cursor += size;
					}

				}


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
			char *str = eles->string;
			char *vstr = eles->valuestring;

			if (0 == strncmp(str, g_enu, strlen(g_enu)))
			{
				fprintf(g_fp, deser_num_func, vstr, vstr);
			}
			else
			{
				first_time = true;
				fprintf(g_fp, deser_func_head, str, str);

				for (int i = 0; i < cvector_size(keylist_vec); i++) {
					if (0 == strcmp(str, keylist_vec[i])) {
						int size = sprintf(g_map + g_map_cursor, map_format, str, str, str, str, str, str);
						g_map_cursor += size;
						g_map_num++;
					}
				}

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

int idl_append_src_inc(const char *src, char *header)
{
	fprintf(g_fp, "#include \"%s\"\n", header);
	fprintf(g_fp, "#include \"dds/ddsrt/log.h\"\n");
	fprintf(g_fp, "#include <stdio.h>\n");
	fprintf(g_fp, "#include <string.h>\n");
	fprintf(g_fp, "#include <stdlib.h>\n");
	return 0;
}

int idl_append_header_inc()
{
	fprintf(g_hfp, "#ifndef __IDL_CONVERT_H__\n");
	fprintf(g_hfp, "#define __IDL_CONVERT_H__\n\n");
	fprintf(g_hfp, "#include \"nng/supplemental/nanolib/cJSON.h\"\n");
	fprintf(g_hfp, "#include \"dds_type.h\"\n\n");

	fprintf(g_hfp, "typedef int (*mqtt_to_dds_fn_t)(cJSON *, void *);\n");
	fprintf(g_hfp, "typedef cJSON *(*dds_to_mqtt_fn_t)(void *);\n");
	fprintf(g_hfp, "typedef void *(*dalloc_fn_t)();\n");
	fprintf(g_hfp, "typedef void (*dfree_fn_t)(\n");
	fprintf(g_hfp, "    void *sample, dds_free_op_t op);\n");
	fprintf(g_hfp, "\n");
	fprintf(g_hfp, "typedef struct\n");
	fprintf(g_hfp, "{\n");
	fprintf(g_hfp, "	const dds_topic_descriptor_t *desc;\n");
	fprintf(g_hfp, "	dalloc_fn_t      alloc;\n");
	fprintf(g_hfp, "	dfree_fn_t       free;\n");
	fprintf(g_hfp, "	mqtt_to_dds_fn_t mqtt2dds;\n");
	fprintf(g_hfp, "	dds_to_mqtt_fn_t dds2mqtt;\n");
	fprintf(g_hfp, "} dds_handler_set;\n");
	fprintf(g_hfp, "\n");
	fprintf(g_hfp, "typedef struct\n");
	fprintf(g_hfp, "{\n");
	fprintf(g_hfp, "	char         *struct_name;\n");
	fprintf(g_hfp, "	dds_handler_set op_set;\n");
	fprintf(g_hfp, "} dds_handler_map;\n\n");

	return 0;
}

static cJSON *idl_parser(const char *file)
{
	int rv = 0;
	if (!(yyin = fopen(file, "r")))
	{
		fprintf(stderr, "Failed to open %s: %s\n", file, strerror(errno));
		return NULL;
	}

	cJSON *jso;
	rv = yyparse(&jso);
	if (0 != rv)
	{
		fprintf(stderr, "invalid data to parse!\n");
		return NULL;
	}

	char *str = cJSON_PrintUnformatted(jso);
	log_info("%s", str);
	cJSON_free(str);

	return jso;
}

int idl_serial_generator(const char *file, const char *out)
{

	// Open src/header file
	char src[64] = { 0 };
	char header[64] = { 0 };
	snprintf(src, 64, "%s.c", out);
	snprintf(header, 64, "%s.h", out);
	g_fp = fopen(src, "w");
	g_hfp = fopen(header, "w");

	// Append include
	idl_append_src_inc(out, header);
	idl_append_header_inc();

	// Parser
	cJSON *jso = idl_parser(file);

	// Generate code
	idl_struct_to_json(jso);

	char map[] = "dds_handler_map dds_struct_handler_map";
	int size = sprintf(g_map+g_map_cursor, "%s[] = {\n", map);
	g_map_cursor += size;

	idl_json_to_struct(jso);

	fprintf(g_hfp, "\nextern %s[%d];\n", map, g_map_num);
	fprintf(g_hfp, "\n#endif\n");
	sprintf(g_map + g_map_cursor, "};\n");
	fprintf(g_fp, "%s", g_map);

	cJSON_Delete(jso);
	fclose(g_fp);
	fclose(g_hfp);

	return 0;
}
