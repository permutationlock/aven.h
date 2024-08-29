#include <aven.h>
#include <aven/path.h>
#include <aven/str.h>
#include <aven/test.h>

#include <stdlib.h>

AvenArena test_arena;

typedef struct {
    char *expected;
    char *parts[8];
    size_t nparts;
} TestAvenPathArgs;


AvenTestResult test_aven_path(void *args) {
    TestAvenPathArgs *pargs = args;
    AvenArena arena = test_arena;

    AvenStr path = { 0 };
    switch (pargs->nparts) {
        case 0:
            break;
        case 1:
            path = aven_path(&arena, pargs->parts[0], NULL);
            break;
        case 2:
            path = aven_path(
                &arena,
                pargs->parts[0],
                pargs->parts[1],
                NULL
            );
            break;
        case 3:
            path = aven_path(
                &arena,
                pargs->parts[0],
                pargs->parts[1],
                pargs->parts[2],
                NULL
            );
            break;
        default:
            path = aven_path(
                &arena,
                pargs->parts[0],
                pargs->parts[1],
                pargs->parts[2],
                pargs->parts[3],
                NULL
            );
            break;
    }

    AvenStr expected_path = aven_str_cstr(pargs->expected);
    bool match = aven_str_compare(path, expected_path);

    if (!match) {
        char fmt[] = "expected \"%s\", found \"%s\"";
       
        char *buffer = malloc(
            sizeof(fmt) +
            path.len +
            expected_path.len
        );

        int len = sprintf(buffer, fmt, expected_path.ptr, path.ptr);
        assert(len > 0);

        return (AvenTestResult){
            .error = 2,
            .message = buffer,
        };
    }

    return (AvenTestResult){ 0 };
}

typedef struct {
    char *expected;
    char *path;
} TestAvenPathDirArgs;

AvenTestResult test_aven_path_rel_dir(void *args) {
    TestAvenPathDirArgs *pargs = args;
    AvenArena arena = test_arena;

    AvenStr path = aven_path_rel_dir(aven_str_cstr(pargs->path), &arena);
    AvenStr expected_path = aven_str_cstr(pargs->expected);
    bool match = aven_str_compare(path, expected_path);

    if (!match) {
        char fmt[] = "expected \"%s\", found \"%s\"";
       
        char *buffer = malloc(
            sizeof(fmt) +
            path.len +
            expected_path.len
        );

        int len = sprintf(buffer, fmt, expected_path.ptr, path.ptr);
        assert(len > 0);

        return (AvenTestResult){
            .error = 2,
            .message = buffer,
        };
    }

    return (AvenTestResult){ 0 };
}

int test_path(void) {
    AvenTestCase tcase_data[] = {
        {
            .desc = "aven_path empty path",
            .fn = test_aven_path,
            .args = &(TestAvenPathArgs){
                .expected = "",
                .nparts = 1,
                .parts = { "" },
            },
        },
        {
            .desc = "aven_path 1 level path",
            .fn = test_aven_path,
            .args = &(TestAvenPathArgs){
                .expected = "dir",
                .nparts = 1,
                .parts = { "dir" },
            },
        },
        {
            .desc = "aven_path 2 level path",
            .fn = test_aven_path,
            .args = &(TestAvenPathArgs){
#ifdef _WIN32
                .expected = "dir\\a",
#else
                .expected = "dir/a",
#endif
                .nparts = 2,
                .parts = { "dir", "a" },
            },
        },
        {
            .desc = "aven_path 3 level path",
            .fn = test_aven_path,
            .args = &(TestAvenPathArgs){
#ifdef _WIN32
                .expected = "learn\\you\\for",
#else
                .expected = "learn/you/for",
#endif
                .nparts = 3,
                .parts = { "learn", "you", "for" },
            },
        },
        {
            .desc = "aven_path 4 level path",
            .fn = test_aven_path,
            .args = &(TestAvenPathArgs){
#ifdef _WIN32
                .expected = "purely\\functional\\data\\structures",
#else
                .expected = "purely/functional/data/structures",
#endif
                .nparts = 4,
                .parts = { "purely", "functional", "data", "structures" },
            },
        },
        {
            .desc = "aven_path_dir 1 level relative path",
            .fn = test_aven_path_rel_dir,
            .args = &(TestAvenPathDirArgs){
                .expected = ".",
                .path = "dir",
            },
        },
        {
            .desc = "aven_path_dir 2 level relative path",
            .fn = test_aven_path_rel_dir,
            .args = &(TestAvenPathDirArgs){
#ifdef _WIN32
                .expected = "a",
                .path = "a\\b",
#else
                .expected = "a",
                .path = "a/b",
#endif
            },
        },
    };
    AvenTestCaseSlice tcases = {
        .ptr = tcase_data,
        .len = countof(tcase_data),
    };

    aven_test(tcases, __FILE__);

    return 0;
}
