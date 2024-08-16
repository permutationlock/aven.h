#ifndef AVEN_ARG_H
#define AVEN_ARG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aven.h"

typedef enum {
    AVEN_ARG_TYPE_BOOL = 0,
    AVEN_ARG_TYPE_INT,
    AVEN_ARG_TYPE_STRING,
} AvenArgType;

typedef struct {
    AvenArgType type;
    union {
        bool arg_bool;
        int arg_int;
        char *arg_string;
    } data;
} AvenArgValue;

typedef struct {
    char *name;
    char *description;
    bool optional;
    AvenArgType type;
    AvenArgValue value;
} AvenArg;

static void aven_arg_print_type(AvenArgType arg_type) {
    switch (arg_type) {
        case AVEN_ARG_TYPE_BOOL:
            break;
        case AVEN_ARG_TYPE_INT:
            printf(" n");
            break;
        case AVEN_ARG_TYPE_STRING:
            printf(" \"string\"");
            break;
        default:
            break;
    }
}

static void aven_arg_print_value(AvenArgValue value) {
    switch (value.type) {
        case AVEN_ARG_TYPE_BOOL:
            break;
        case AVEN_ARG_TYPE_INT:
            printf("%d", value.data.arg_int);
            break;
        case AVEN_ARG_TYPE_STRING:
            printf("\"%s\"", value.data.arg_string);
            break;
        default:
            break;
    }
}

static void aven_arg_print(AvenArg arg) {
    assert(arg.name != NULL);
    printf("\t%s", arg.name);

    aven_arg_print_type(arg.type);
 
    if (arg.description != NULL) {
        printf("  --  %s", arg.description);
    }

    if (arg.type != AVEN_ARG_TYPE_BOOL and arg.type == arg.value.type) {
        printf(" (default=");
        aven_arg_print_value(arg.value);
        printf(")");
    } else if (arg.optional) {
        printf(" (optional)");
    }

    printf("\n");
}

static void aven_arg_help(AvenArg *args, int args_len) {
    printf("arguments:\n");
    for (int i = 0; i < args_len; i += 1) {
        aven_arg_print(args[i]);
    }
    printf("to show this message use:\n\t-h, -help, --help\n");
}

static inline int aven_arg_parse(
    AvenArg *args,
    int args_len,
    char **argv,
    int argc
) {
    for (int i = 1; i < argc; i += 1) {
        char *arg_str = argv[i];
        if (
            strcmp(arg_str, "-h") == 0 or
            strcmp(arg_str, "-help") == 0 or
            strcmp(arg_str, "--help") == 0
        ) {
            aven_arg_help(args, args_len);
            return -1;
        }

        bool found = false;
        for (int j = 0; j < args_len; j += 1) {
            AvenArg *arg = &args[j];
            if (strcmp(arg_str, arg->name) != 0) {
                continue;
            }

            found = true;
            switch (arg->type) {
                case AVEN_ARG_TYPE_BOOL:
                    arg->value.data.arg_bool = true;
                    break;
                case AVEN_ARG_TYPE_INT:
                    if (i + 1 >= argc) {
                        printf("missing expected argument value:\n");
                        aven_arg_print(*arg);
                        return -1;
                    }
                    arg->value.data.arg_int = atoi(argv[i + 1]);
                    i += 1;
                    break;
                case AVEN_ARG_TYPE_STRING:
                    if (i + 1 >= argc) {
                        printf("missing expected argument value:\n");
                        aven_arg_print(*arg);
                        return -1;
                    }
                    arg->value.data.arg_string = argv[i + 1];
                    i += 1;
                    break;
                default:
                    break;
            }
        }

        if (!found) {
            printf("unknown flag: %s\n", arg_str);
            aven_arg_help(args, args_len);
            return -1;
        }
    }

    int error = 0;
    for (int j = 0; j < args_len; j += 1) {
        AvenArg arg = args[j];
        if (!arg.optional and arg.value.type != arg.type) {
            printf("missing required argument:\n");
            aven_arg_print(arg);
            error = -1;
        }
    }

    return error;
}

typedef Optional(AvenArg) AvenArgOptional;
typedef Slice(AvenArg) AvenArgSlice;

static AvenArgOptional aven_arg_get(
    AvenArgSlice arg_slice,
    char *argname
) {
    for (size_t i = 0; i < arg_slice.len; i += 1) {
        if (strcmp(argname, slice_get(arg_slice, i).name) == 0) {
            return (AvenArgOptional){
                .valid = true,
                .value = slice_get(arg_slice, i),
            };
        }
    }
    
    return (AvenArgOptional){ .valid = false };
}

static inline bool aven_arg_has_arg(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    return opt_arg.valid and (opt_arg.value.type == opt_arg.value.value.type);
}

static inline bool aven_arg_get_bool(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    assert(opt_arg.valid);
    assert(opt_arg.value.type == opt_arg.value.value.type);
    assert(opt_arg.value.type == AVEN_ARG_TYPE_BOOL);
    return opt_arg.value.value.data.arg_bool;
}

static inline int aven_arg_get_int(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    assert(opt_arg.valid);
    assert(opt_arg.value.type == opt_arg.value.value.type);
    assert(opt_arg.value.type == AVEN_ARG_TYPE_INT);
    return opt_arg.value.value.data.arg_int;
}

static inline char *aven_arg_get_string(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    assert(opt_arg.valid);
    assert(opt_arg.value.type == opt_arg.value.value.type);
    assert(opt_arg.value.type == AVEN_ARG_TYPE_STRING);
    return opt_arg.value.value.data.arg_string;
}


#endif // AVEN_ARG_H
