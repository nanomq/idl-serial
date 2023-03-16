#include "idl_serial.h"
#include "string.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        char *c_file;
        if (argc >= 2)
        {
            c_file = argv[2];
        }
        else
        {
            c_file = "idl_convert.c";
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
