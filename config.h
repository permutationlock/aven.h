// config.h defines custom defaultss for common flags

#ifndef _WIN32
    #define AVEN_BUILD_COMMON_DEFAULT_CCFLAGS \
        "-pedantic -fstrict-aliasing -O1 -g3 -Werror -Wall -Wextra " \
        "-Wshadow -Wconversion -Wdouble-promotion -Winit-self " \
        "-Wcast-align -Wstrict-prototypes -Wold-style-definition " \
        "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined"
#endif

