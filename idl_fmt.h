#ifndef IDL_FMT_H
#define IDL_FMT_H

const char ser_num_func[] =
    "\nstatic cJSON *"
    "\ndds_to_mqtt_%s_convert(%s *enu)"
    "\n{\n"
    "\n\tif (enu == NULL) {"
    "\n\t\tDDS_FATAL(\"enu is NULL\\n\");"
    "\n\t\treturn NULL;"
    "\n\t}\n"
    "\n\treturn cJSON_CreateNumber(*enu);\n"
    "\n}\n";

const char deser_num_func[] =
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

const char ser_func_head[] =
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

const char ser_func_tail[] =
    "\n\treturn obj;"
    "\n}\n\n";

const char deser_func_head[] =
    "\nstatic int"
    "\nmqtt_to_dds_%s_convert(cJSON *obj, %s *st)"
    "\n{"
    "\n\tif (obj == NULL || st == NULL) {"
    "\n\t\tDDS_FATAL(\"obj or st is NULL\\n\");"
    "\n\t\treturn -1;"
    "\n\t}\n"
    "\n\tint rc = 0;"
    "\n\tcJSON *item = NULL;\n";

const char deser_func_tail[] =
    "\n\treturn 0;"
    "\n}\n\n";

const char ser_func_call[] =
    "\n\tcJSON_AddItemToObject(obj, \"%s\", dds_to_mqtt_%s_convert(&st->%s));\n";

const char deser_func_call[] =
    "\n\t\trc = mqtt_to_dds_%s_convert(item, &st->%s);"
    "\n\t\tif (rc != 0) {"
    "\n\t\t\treturn rc;"
    "\n\t\t}\n";

const char init_format[] =
    "\nstatic void *"
    "\n%s_init(void)"
    "\n{"
    "\n	return %s__alloc();"
    "\n}\n";

const char fini_format[] =
    "\nstatic void"
    "\n%s_fini(void *sample, dds_free_op_t op)"
    "\n{"
    "\n	return %s_free(sample, op);"
    "\n}\n";

const char map_format[] =
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

#endif