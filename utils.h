#ifndef __utils_h__
#define __utils_h__

typedef enum
{
    OBJECT_TYPE_NUMBER,
    OBJECT_TYPE_BOOLEAN,
    OBJECT_TYPE_STRING,
    OBJECT_TYPE_STRING_T,
    OBJECT_TYPE_ARRAY,
    OBJECT_TYPE_SEQUENCE
} object_type;

typedef struct object object;
struct object
{
    object_type type;
    char *data;
} ;

object *object_alloc(object_type t, char *data);
void object_free(object *o);


#endif