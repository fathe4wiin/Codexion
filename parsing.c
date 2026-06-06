#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "codexion.h"

static int parse_positive_int(const char *s, int *out)
{
    long long value;
    const char *ptr;
    int digit;

    if (!s || !*s)
        return 0;
    value = 0;
    ptr = s;
    while (*ptr)
    {
        if (*ptr < '0' || *ptr > '9')
            return 0;
        digit = *ptr - '0';
        if (value > (INT_MAX - digit) / 10)
            return 0;
        value = value * 10 + digit;
        ptr++;
    }
    if (value <= 0)
        return 0;
    *out = (int)value;
    return 1;
}

static int parse_non_negative_int(const char *s, int *out)
{
    long long value;
    const char *ptr;
    int digit;

    if (!s || !*s)
        return 0;
    value = 0;
    ptr = s;
    while (*ptr)
    {
        if (*ptr < '0' || *ptr > '9')
            return 0;
        digit = *ptr - '0';
        if (value > (INT_MAX - digit) / 10)
            return 0;
        value = value * 10 + digit;
        ptr++;
    }
    if (value < 0 || value > INT_MAX)
        return 0;
    *out = (int)value;
    return 1;
}

static int parse_non_negative_ll(const char *s, long long *out)
{
    unsigned long long value;
    const char *ptr;
    int digit;

    if (!s || !*s)
        return 0;
    value = 0;
    ptr = s;
    while (*ptr)
    {
        if (*ptr < '0' || *ptr > '9')
            return 0;
        digit = *ptr - '0';
        if (value > (unsigned long long)(LLONG_MAX - digit) / 10ULL)
            return 0;
        value = value * 10ULL + (unsigned long long)digit;
        ptr++;
    }
    *out = (long long)value;
    return 1;
}

t_args *parse_args(char *av[])
{
    t_args *args;

    if (!av || !av[1] || !av[2] || !av[3] || !av[4] || !av[5] || !av[6]
        || !av[7] || !av[8] || av[9])
        return NULL;
    if (!parse_positive_int(av[1], &((int){0})))
        return NULL;
    if (!parse_non_negative_ll(av[2], &((long long){0})))
        return NULL;
    if (!parse_non_negative_int(av[3], &((int){0})))
        return NULL;
    if (!parse_non_negative_int(av[4], &((int){0})))
        return NULL;
    if (!parse_non_negative_int(av[5], &((int){0})))
        return NULL;
    if (!parse_positive_int(av[6], &((int){0})))
        return NULL;
    if (!parse_non_negative_ll(av[7], &((long long){0})))
        return NULL;
    if (strcmp(av[8], "fifo") != 0 && strcmp(av[8], "edf") != 0)
        return NULL;

    args = malloc(sizeof(t_args));
    if (!args)
        return NULL;

    parse_positive_int(av[1], &args->num_coders);
    parse_non_negative_ll(av[2], &args->time_to_burnout);
    parse_non_negative_int(av[3], &args->time_to_compile);
    parse_non_negative_int(av[4], &args->time_to_debug);
    parse_non_negative_int(av[5], &args->time_to_refactor);
    parse_positive_int(av[6], &args->compiles_required);
    parse_non_negative_ll(av[7], &args->dongle_cooldown);
    args->scheduler_policy = av[8];

    return args;
}
