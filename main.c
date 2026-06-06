#include <stdlib.h>
#include "codexion.h"


int main(int ac, char *av[])
{
    t_args *args;

    if (ac != 9)
        return 1;

    args = parse_args(av);
    if (!args)
        return 1;

    free(args);
    return 0;
}