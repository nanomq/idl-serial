#include "idl_serial.h"
#include "string.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        int ret = idl_serial_generator(argv[1], "idl_convert.c");
        if (0 != ret)
        {
            fprintf(stderr, "Parse idl failed.\n");
        }
    }
    else
    {
        fprintf(stderr, "Usage: idl-serial <your idl file>\n");
    }

    return 0;
}
