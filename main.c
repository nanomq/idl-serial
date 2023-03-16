#include "idl_serial.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        char *c_file = NULL;
        if (argc > 2)
        {
            c_file = argv[2];
        }
        else
        {
            c_file = "idl_convert.c";
        }
        if (c_file[strlen(c_file) - 1] != 'c')
        {
            fprintf(stderr, "Require C file, file '%s' is invalid\n", c_file);
            exit(1);
        }
        int ret = idl_serial_generator(argv[1], c_file);
        if (0 != ret)
        {
            fprintf(stderr, "Parse idl failed.\n");
        }
    }
    else
    {
        fprintf(stderr, "Usage: idl-serial <your idl file> [output c file]\n");
    }

    return 0;
}
