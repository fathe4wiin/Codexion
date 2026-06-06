#ifndef CODEXION_H
#define CODEXION_H

typedef struct s_args {
    int         num_coders;          // number_of_coders: > 0 [cite: 124, 125, 147]
    long long   time_to_burnout;     // time_to_burnout: in milliseconds [cite: 124, 126]
    int         time_to_compile;     // time_to_compile: in milliseconds [cite: 124, 127]
    int         time_to_debug;       // time_to_debug: in milliseconds [cite: 124, 128]
    int         time_to_refactor;    // time_to_refactor: in milliseconds [cite: 124, 128]
    int         compiles_required;   // number_of_compiles_required: > 0 [cite: 124, 130, 147]
    long long   dongle_cooldown;     // dongle_cooldown: in milliseconds [cite: 124, 132]
    char        *scheduler_policy;   // scheduler: exactly "fifo" or "edf" [cite: 124, 133, 134, 147]
} t_args;

t_args  *parse_args(char *av[]);

#endif