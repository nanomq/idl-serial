#include "utils.h"
#include "dbg.h"
#include <stdlib.h>
#include <string.h>

object *object_alloc(object_type t, char *data)
{
    object *o = (object*) malloc(sizeof(object));
    if (NULL == o) {
        log_err("malloc failed!");
    }
    if (data) {
        o->data = strdup(data);
    }
    o->type = t;
    return o;

}

void object_free(object *o)
{
    if (NULL != o) {
        if (NULL != o->data) {
            free(o->data);
        }
        free(o);
    }
}