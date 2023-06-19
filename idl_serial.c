#include "cvector.h"
#include "idl_serial.h"
#include "parser.h"
#include "cJSON.h"
#include "dbg.h"
#include "idl_fmt.h"
#include "utils.h"
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

static FILE *g_fp = NULL; // Source file pointer
static FILE *g_hfp = NULL; // Header file pointer

static char g_map[4096] = { 0 };
static int g_map_cursor = 0;
static int g_map_num = 0;

void generate_struct_arr_get(char *dest, size_t len, int times, const char *val)
{
	size_t size = snprintf(dest, len, "st->%s", val);
	len -= size;
	char *where = dest + size;
	for (int i = 0; i <= times; i++)
	{
		size = snprintf(where, len, "[i%d]", i);
		where += size;
		len -= size;
	}
}


static void AddArrayCommonHelper(char *tab, char *val, char *name, int *times, int num, type t)
{

	fprintf(g_fp, "%scJSON *%s = cJSON_CreateArray();\n", tab, val);
	switch (t)
	{
	case obj_t:
		fprintf(g_fp, "%scJSON_AddItemToObject(obj, \"%s\", %s);\n", tab, val, val);
		break;
	case arr_t:
		fprintf(g_fp, "%scJSON_AddItemToArray(%s%d, %s);\n", tab, val, *times - 1, val);
		break;
	default:
		break;
	}
	fprintf(g_fp, "%sfor (int i = 0; (i < (size_t) %d); i++) {\n", tab, num);
	tab[++(*times)] = '\t';


	char tmp[64];
	generate_struct_arr_get(tmp, 64, *times-2, val);

	if (name) {
		fprintf(g_fp, "%scJSON *n = dds_to_mqtt_%s_convert(&%s[i]);\n", tab, name, tmp);
	} else {
		fprintf(g_fp, "%scJSON *n = cJSON_CreateNumber(%s[i]);\n", tab, tmp);
	}
	fprintf(g_fp, "%scJSON_AddItemToArray(%s, n);\n", tab, val);
}



void cJSON_AddArrayCommon(char *p, char *val, char *type, int ot)
{
	char *p_b = p;
	int times = 0;
	char tab[32] = {0};
	tab[times] = '\t';

	if (strchr(p, '_'))
	{
		fprintf(g_fp, "%scJSON *%s%d = cJSON_CreateArray();\n", tab, val, times);
		fprintf(g_fp, "%scJSON_AddItemToObject(obj, \"%s\", %s%d);\n", tab, val, val, times);
	}
	else
	{
		int num = atoi(p);
		switch (ot)
		{
		case OBJECT_TYPE_STRUCT:
			AddArrayCommonHelper(tab, val, type, &times, num, obj_t);
			break;
		default:
			AddArrayCommonHelper(tab, val, NULL, &times, num, obj_t);
			break;
		}
	}

	while (NULL != (p = strchr(p, '_')))
	{

		*p = '\0';
		int num = atoi(p_b);
		fprintf(g_fp, "%sfor (int i%d = 0; i%d < %d; i%d++) {\n", tab, times, times, num, times);

		*p++ = '_';
		p_b = p;

		tab[++times] = '\t';

		if (strchr(p, '_'))
		{
			fprintf(g_fp, "%scJSON *%s%d = cJSON_CreateArray();\n", tab, val, times);
			fprintf(g_fp, "%scJSON_AddItemToArray(%s%d, %s%d);\n", tab, val, times-1, val, times);
		}
		else
		{
			num = atoi(p);
			switch (ot)
			{
			case OBJECT_TYPE_STRUCT:
				AddArrayCommonHelper(tab, val, type, &times, num, arr_t);
				break;
			default:
				AddArrayCommonHelper(tab, val, NULL, &times, num, arr_t);
				break;
			}

		}

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
		cJSON_AddArrayCommon(p, val, "Number", OBJECT_TYPE_NUMBER);
	}
	else if (0 == strncmp(p, "NUMBER", strlen("NUMBER")))
	{
		// NUMBER_uint16_100_200
		p += strlen("NUMBER_");
		p = strchr(p, '_') + 1;

		cJSON_AddArrayCommon(p, val, "Double", OBJECT_TYPE_NUMBER);
	}
	else if (0 == strncmp(p, "STRING_T_", strlen("STRING_T_")))
	{
		// STRING_T_string_100_3222_2222
		p += strlen("STRING_T_string_");
		cJSON_AddArrayCommon(p, val, "String", OBJECT_TYPE_STRING_T);
	}
	else if (0 == strncmp(p, "STRING", strlen("STRING")))
	{
		p += strlen("STRING_string_");
		cJSON_AddArrayCommon(p, val, "String", OBJECT_TYPE_STRING);
	}
	else if (0 == strncmp(p, "STRUCT", strlen("STRUCT")))
	{
		char *p_b =p + strlen("STRUCT_");
		p = strchr(p_b, '@');
		*p = '\0';
		p+= 2;
		cJSON_AddArrayCommon(p, val, p_b, OBJECT_TYPE_STRUCT);
		// recover for next time use
		*(p-2) = '@';
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

void cJSON_GetArrayCommon(char *p, char *val, char *type, int ot)
{
	char *p_b = p;
	int times = 0;
	char tab[16] = {0};
	tab[times] = '\t';

	fprintf(g_fp, "\t%sint i0 = 0;\n", tab);
	fprintf(g_fp, "\t%scJSON *%s%d = NULL;\n", tab, val, times);
	fprintf(g_fp, "\t%scJSON_ArrayForEach(%s%d, item) {\n", tab, val, times);
	tab[++times] = '\t';

	if (strchr(p, '_'))
	{
		while (NULL != (p = strchr(p, '_'))) {

			*p = '\0';
			int num = atoi(p_b);

			fprintf(g_fp, "\t%sint i%d = 0;\n", tab, times);
			fprintf(g_fp, "\t%scJSON *%s%d = NULL;\n", tab, val, times);
			fprintf(g_fp, "\t%scJSON_ArrayForEach(%s%d, %s%d) {\n", tab, val, times - 1, val, times);
			*p++ = '_';
			p_b = p;

			if (NULL == strchr(p, '_'))
			{
					switch (ot)
					{
					case OBJECT_TYPE_STRING_T:;
						char tmp[64] = { 0 };
						generate_struct_arr_get(tmp, 64, times, val);
						fprintf(g_fp, "%s\t\tstrcpy(%s, %s%d->valuestring);\n", tab, tmp, val, times);
						break;

					case OBJECT_TYPE_STRING:
					case OBJECT_TYPE_NUMBER:
					case OBJECT_TYPE_STRUCT:

						tab[++times] = '\t';
						p++;
						p_b = p;

						generate_struct_arr_get(tmp, 64, times-1, val);

						switch (ot)
						{
						case OBJECT_TYPE_STRING:
							fprintf(g_fp, "\t%s%s = strdup(%s%d->value%s);\n", tab, tmp, val, times, type);
							break;
						case OBJECT_TYPE_NUMBER:
							char t[16] = {0};
							if (is_convert(type))
							{
								sprintf(t, "%s_t", type);
							}
							fprintf(g_fp, "\t%s%s = (%s) %s%d->valuedouble;\n", tab, tmp, type, val, times);
							break;
						case OBJECT_TYPE_STRUCT:
							fprintf(g_fp, "\t%smqtt_to_dds_%s_convert(%s0, &%s);\n", tab, type, val, tmp);
							break;

						default:
							break;
						}
					default:
						break;
					}

			} else {
				tab[++times] = '\t';
			}
		}
		// p = strchr(p, '_') + 1;
	}
	else
	{
		char tmp[64];
		generate_struct_arr_get(tmp, 64, times-1, val);

		switch (ot)
		{
		case OBJECT_TYPE_STRING:
			fprintf(g_fp, "\t%s%s = strdup(%s%d->value%s);\n", tab, tmp, val, times - 1, type);
			break;
		case OBJECT_TYPE_STRING_T:
			fprintf(g_fp, "\tstrcpy(%s%s, %s%d->value%s);\n", tab, tmp, val, times - 1, type);
			break;
		case OBJECT_TYPE_NUMBER:
			char t[16] = {0};
			if (is_convert(type))
			{
				fprintf(g_fp, "\t%s%s = (%s_t) %s%d->valuedouble;\n", tab, tmp, type, val, times - 1);
			} else {
				fprintf(g_fp, "\t%s%s = (%s) %s%d->valuedouble;\n", tab, tmp, type, val, times - 1);
			}
			break;
		case OBJECT_TYPE_STRUCT:
			fprintf(g_fp, "\t%smqtt_to_dds_%s_convert(%s0, &%s);\n", tab, type, val, tmp);
			break;
		default:
			break;
		}


	}


	tab[times--] = '\0';
	int i = 0;
	while (times >= 0)
	{
		fprintf(g_fp, "\t\t%si%d++;\n", tab, times);
		fprintf(g_fp, "\t%s}\n", tab);
		tab[times--] = '\0';
	}
}

void cJSON_GetArray(char *key, char *val)
{

	char *p = key + strlen("ARRAY_");
	char *ret = NULL;

	fprintf(g_fp, "\n\t\t// %s\n", key);
	if (0 == strncmp(p, "BOOLEAN", strlen("BOOLEAN")))
	{
		p += strlen("BOOLEAN_bool_");
		cJSON_GetArrayCommon(p, val, "bool", OBJECT_TYPE_NUMBER);
	}
	else if (0 == strncmp(p, "NUMBER", strlen("NUMBER")))
	{
		char *p_b = p + strlen("NUMBER_");
		p = strchr(p_b, '_');
		*p = '\0';
		p++;

		cJSON_GetArrayCommon(p, val, p_b, OBJECT_TYPE_NUMBER);
	}
	else if (0 == strncmp(p, "STRING_T_", strlen("STRING_T_")))
	{
		// STRING_T_string_100_3222_2222
		p += strlen("STRING_T_string_");
		cJSON_GetArrayCommon(p, val, "string_", OBJECT_TYPE_STRING_T);
	}
	else if (0 == strncmp(p, "STRING", strlen("STRING")))
	{
		p += strlen("STRING_string_");
		cJSON_GetArrayCommon(p, val, "string", OBJECT_TYPE_STRING);
	}
	else if (0 == strncmp(p, "STRUCT", strlen("STRUCT")))
	{
		char *p_b =p + strlen("STRUCT_");
		p = strchr(p_b, '@');
		*p = '\0';
		p += 2;
		cJSON_GetArrayCommon(p, val, p_b, OBJECT_TYPE_STRUCT);
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

	static char tab[32] = {0};
	tab[*times] = '\t';

	char *p = key;
	char *ret = NULL;

	fprintf(g_fp, "\n\t%s// %s", tab, key);

	int i = 0;
	char tmp[128] = {0};
	size_t size = snprintf(tmp, 128, "\n\t%scJSON *%s_array0", tab, val);
	char *where = tmp + size;
	while ((p = strstr(p, "sequence")))
	{
		size = snprintf(where, 128, ", *%s_array%d", val, i + 1);
		where = where + size;
		p++;
		i++;
	}

	fprintf(g_fp, "%s;", tmp);
	fprintf(g_fp, "\n\t%s%s_array0 = item;\n", tab, val);

	i = 0;
	p = key;
	memset(tmp, 0, 128);
	size = snprintf(tmp, 128, "%s", val);
	where = tmp + size;
	while ((p = strstr(p, "sequence")))
	{
		clean_data(p);
		fprintf(g_fp, "\n\t%ssize_t %s_sz%d = cJSON_GetArraySize(%s_array%d);", tab, val, i, val, i);
		fprintf(g_fp, "\n\t%sst->%s = dds_%s__alloc();", tab, tmp, p);
		fprintf(g_fp, "\n\t%sst->%s->_buffer = dds_%s_allocbuf(%s_sz%d);", tab, tmp, p, val, i);
		fprintf(g_fp, "\n\t%sint %s_i%d = 0;", tab, val, i);
		fprintf(g_fp, "\n\t%scJSON_ArrayForEach(%s_array%d, %s_array%d) {", tab, val, i + 1, val, i);

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
	size = snprintf(tmp, 128, "\n\t%sst->%s", tab, val);
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
		fprintf(g_fp, "\tstrcpy(%s, cJSON_GetStringValue(%s_array%d));\n", tmp, val, i);
	}
	else if (0 == strncmp(p_b, "string", strlen("string")))
	{
		fprintf(g_fp, "\t%s = strdup(cJSON_GetStringValue(%s_array%d));\n", tmp, val, i);
	}
	else
	{
		fprintf(g_fp, "\t%s = cJSON_GetNumberValue(%s_array%d);\n", tmp, val, i);
	}

	while (*times > 0)
	{
		fprintf(g_fp, "\t%s%s_i%d++;\n", tab, val, (*times) - 1);
		tab[(*times)--] = '\0';
		fprintf(g_fp, "\t%s}\n", tab);
	}
}

void cJSON_Get(cJSON *item)
{
	char fmt_non_cp[] = "\t\tst->%s = item->value%s;\n";
	char fmt_cp[] = "\t\ttstrncpy(st->%s, item->value%s, %d);\n";
	char fmt_dup[] = "\t\tst->%s = strdup(item->value%s);\n";
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

	fprintf(g_fp, "%s", ret);
	return;
}

void generate_init_fini_func(char *str)
{
	int size = sprintf(g_map + g_map_cursor, init_format, str, str);
	g_map_cursor += size;
	size = sprintf(g_map + g_map_cursor, fini_format, str, str);
	g_map_cursor += size;
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

				if (keylist_vec == NULL) {
					generate_init_fini_func(str);
				} else {
					for (int i = 0; i < cvector_size(keylist_vec); i++) {
						if (0 == strcmp(str, keylist_vec[i])) {
							generate_init_fini_func(str);
						}
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

void generate_func_map(char *str)
{
	int size = sprintf(g_map + g_map_cursor, map_format, str, str, str, str, str, str);
	g_map_cursor += size;
	g_map_num++;
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
				fprintf(g_fp, deser_func_head, str, str);

				if (keylist_vec == NULL) {
					generate_func_map(str);
				} else {
					for (int i = 0; i < cvector_size(keylist_vec); i++) {
						if (0 == strcmp(str, keylist_vec[i])) {
							generate_func_map(str);
							break;
						}
					}
				}

				cJSON_ArrayForEach(ele, eles)
				{
					cJSON_ArrayForEach(e, ele)
					{
						fprintf(g_fp, "\titem = cJSON_GetObjectItem(obj, \"%s\");\n", e->valuestring);
						fprintf(g_fp, "\tif (item) {\n");
						cJSON_Get(e);
						fprintf(g_fp, "\t}\n");
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

char *get_idl_file_name(char *file)
{
	char *p = strrchr(file, '.');
	char *p_b = strrchr(file, '/');
	*p = '\0';
	return p_b ? p_b + 1 : file;
}

int idl_append_header_inc(char *file)
{
	char *name = get_idl_file_name(file);
	fprintf(g_hfp, "#ifndef __IDL_CONVERT_H__\n");
	fprintf(g_hfp, "#define __IDL_CONVERT_H__\n\n");
	fprintf(g_hfp, "#include \"nng/supplemental/nanolib/cJSON.h\"\n");
	fprintf(g_hfp, "#include \"%s.h\"\n\n", name);
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

static void move_file_to_dir(const char *src, const char *out_dir)
{
	if (out_dir != NULL)
	{
		size_t dir_len = strlen(out_dir);
		if (dir_len > 0)
		{
			char new_path[512] = {0};
			if (out_dir[dir_len - 1] == '/')
			{
				snprintf(new_path, 512, "%s%s", out_dir, src);
			}
			else
			{
				snprintf(new_path, 512, "%s/%s", out_dir, src);
			}
			rename(src, new_path);
		}
	}
}

int idl_serial_generator(char *file, const char *out, const char *out_dir)
{

	// Open src/header file
	char src[255] = { 0 };
	char header[255] = { 0 };

	snprintf(src, 255, "%s.c", out);
	snprintf(header, 255, "%s.h", out);
	g_fp = fopen(src, "w");
	g_hfp = fopen(header, "w");

	// Parser
	cJSON *jso = idl_parser(file);

	// Append include
	idl_append_src_inc(out, header);
	idl_append_header_inc(file);


	// Generate code
	idl_struct_to_json(jso);

	char map[] = "\ndds_handler_map dds_struct_handler_map";
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

	if (out_dir != NULL)
	{
		move_file_to_dir(src, out_dir);
		move_file_to_dir(header, out_dir);
	}

	return 0;
}
