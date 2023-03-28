#include "idl_serial.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        char *name = NULL;
        char *dir = NULL;
        if (argc > 2)
        {
            name = argv[2];
            if (argc > 3)
            {
                dir = argv[3];
            }
        }
        else
        {
            name = "idl_convert";
        }
        int ret = idl_serial_generator(argv[1], name, dir);
        if (0 != ret)
        {
            fprintf(stderr, "Parse idl failed.\n");
        }
    }
    else
    {
        fprintf(stderr, "Usage: idl-serial <your idl file> [output name] [output directory]\n");
    }

    return 0;
}
