#ifndef IDL_SERIAL_H
#define IDL_SERIAL_H
#include "cJSON.h"

typedef enum
{
	arr_t,
	obj_t
} type;

int idl_serial_generator(const char *file, const char *out);

#endif