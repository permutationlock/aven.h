#ifndef AVEN_ARG_H
#define AVEN_ARG_H

#include "../aven.h"

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
        char *arg_str;
    } data;
} AvenArgValue;

typedef struct {
    char *name;
    char *description;
    bool optional;
    AvenArgType type;
    AvenArgValue value;
} AvenArg;

typedef Optional(AvenArg) AvenArgOptional;
typedef Slice(AvenArg) AvenArgSlice;

typedef enum {
    AVEN_ARG_ERROR_NONE = 0,
    AVEN_ARG_ERROR_HELP,
    AVEN_ARG_ERROR_VALUE,
    AVEN_ARG_ERROR_MISSING,
    AVEN_ARG_ERROR_UNKNOWN,
} AvenArgError;

AVEN_FN int aven_arg_parse(
    AvenArgSlice args,
    char **argv,
    int argc,
    char *overview,
    char *usage
);

AVEN_FN AvenArgOptional aven_arg_get(AvenArgSlice arg_slice, char *argname);
AVEN_FN bool aven_arg_has_arg(AvenArgSlice arg_slice, char *argname);
AVEN_FN bool aven_arg_get_bool(AvenArgSlice arg_slice, char *argname);
AVEN_FN int aven_arg_get_int(AvenArgSlice arg_slice, char *argname);
AVEN_FN char *aven_arg_get_str(AvenArgSlice arg_slice, char *argname);

#ifdef AVEN_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void aven_arg_print_type(AvenArgType arg_type) {
    switch (arg_type) {
        case AVEN_ARG_TYPE_BOOL:
            break;
        case AVEN_ARG_TYPE_INT:
            printf(" n");
            break;
        case AVEN_ARG_TYPE_STRING:
            printf(" \"str\"");
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
            printf("\"%s\"", value.data.arg_str);
            break;
        default:
            break;
    }
}

static void aven_arg_print(AvenArg arg) {
    assert(arg.name != NULL);
    printf("    %s", arg.name);

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

static void aven_arg_help(AvenArgSlice args, char *overview, char *usage) {
    if (overview != NULL) {
        printf("OVERVIEW: %s\n\n", overview);
    }
    if (usage != NULL) {
        printf("USAGE: %s\n\n", usage);
    }
    printf("OPTIONS:\n");
    printf("    help, -h, -help, --help -- Show this message\n");
    for (size_t i = 0; i < args.len; i += 1) {
        aven_arg_print(slice_get(args, i));
    }
}

AVEN_FN int aven_arg_parse(
    AvenArgSlice args,
    char **argv,
    int argc,
    char *overview,
    char *usage
) {
    for (int i = 1; i < argc; i += 1) {
        char *arg_str = argv[i];
        if (
            strcmp(arg_str, "help") == 0 or
            strcmp(arg_str, "-h") == 0 or
            strcmp(arg_str, "-help") == 0 or
            strcmp(arg_str, "--help") == 0
        ) {
            aven_arg_help(args, overview, usage);
            return AVEN_ARG_ERROR_HELP;
        }

        bool found = false;
        for (size_t j = 0; j < args.len; j += 1) {
            AvenArg *arg = &slice_get(args, j);
            if (strcmp(arg_str, arg->name) != 0) {
                continue;
            }

            switch (arg->type) {
                case AVEN_ARG_TYPE_BOOL:
                    arg->value.data.arg_bool = true;
                    break;
                case AVEN_ARG_TYPE_INT:
                    if (i + 1 >= argc) {
                        printf("missing expected argument value:\n");
                        aven_arg_print(*arg);
                        return AVEN_ARG_ERROR_VALUE;
                    }
                    arg->value.data.arg_int = atoi(argv[i + 1]);
                    arg->value.type = AVEN_ARG_TYPE_INT;
                    i += 1;
                    break;
                case AVEN_ARG_TYPE_STRING:
                    if (i + 1 >= argc) {
                        printf("missing expected argument value:\n");
                        aven_arg_print(*arg);
                        return AVEN_ARG_ERROR_VALUE;
                    }
                    arg->value.data.arg_str = argv[i + 1];
                    arg->value.type = AVEN_ARG_TYPE_STRING;
                    i += 1;
                    break;
                default:
                    break;
            }

            found = true;
            break;
        }

        if (!found) {
            printf("unknown option: %s\n", arg_str);
            aven_arg_help(args, overview, usage);
            return AVEN_ARG_ERROR_UNKNOWN;
        }
    }

    int error = 0;
    for (size_t j = 0; j < args.len; j += 1) {
        AvenArg arg = slice_get(args, j);
        if (!arg.optional and arg.value.type != arg.type) {
            printf("missing required argument:\n");
            aven_arg_print(arg);
            error = AVEN_ARG_ERROR_MISSING;
        }
    }

    return error;
}

AVEN_FN AvenArgOptional aven_arg_get(
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

AVEN_FN bool aven_arg_has_arg(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    return opt_arg.valid and (opt_arg.value.type == opt_arg.value.value.type);
}

AVEN_FN bool aven_arg_get_bool(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    assert(opt_arg.valid);
    assert(opt_arg.value.type == opt_arg.value.value.type);
    assert(opt_arg.value.type == AVEN_ARG_TYPE_BOOL);
    return opt_arg.value.value.data.arg_bool;
}

AVEN_FN int aven_arg_get_int(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    assert(opt_arg.valid);
    assert(opt_arg.value.type == opt_arg.value.value.type);
    assert(opt_arg.value.type == AVEN_ARG_TYPE_INT);
    return opt_arg.value.value.data.arg_int;
}

AVEN_FN char *aven_arg_get_str(AvenArgSlice arg_slice, char *argname) {
    AvenArgOptional opt_arg = aven_arg_get(arg_slice, argname);
    assert(opt_arg.valid);
    assert(opt_arg.value.type == opt_arg.value.value.type);
    assert(opt_arg.value.type == AVEN_ARG_TYPE_STRING);
    return opt_arg.value.value.data.arg_str;
}

#endif // AVEN_IMPLEMENTATION

#endif // AVEN_ARG_H
