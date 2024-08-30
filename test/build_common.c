#include <aven.h>
#include <aven/build/common.h>
#include <aven/str.h>
#include <aven/test.h>

#include <stdlib.h>

AvenArena test_arena;

typedef struct {
    char *expected;
    char *name;
    char *value;
} TestAvenBuildCommonCMacro;

AvenTestResult test_aven_build_common_cmacro(void *args) {
    TestAvenBuildCommonCMacro *pargs = args;
    AvenArena arena = test_arena;

    AvenStr macro = aven_build_common_cmacro(
        aven_str_cstr(pargs->name),
        aven_str_cstr(pargs->value),
        &arena
    );
    AvenStr expected_macro = aven_str_cstr(pargs->expected);
    bool match = aven_str_compare(macro, expected_macro);

    if (!match) {
        char fmt[] = "expected \"%s\", found \"%s\"";
       
        char *buffer = malloc(
            sizeof(fmt) +
            macro.len +
            expected_macro.len
        );

        int len = sprintf(buffer, fmt, expected_macro.ptr, macro.ptr);
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    return (AvenTestResult){ 0 };
}

int test_build_common(void) {
    AvenTestCase tcase_data[] = {
        {
            .desc = "aven_build_common_cmacro",
            .fn = test_aven_build_common_cmacro,
            .args = &(TestAvenBuildCommonCMacro){
                .expected = "MY_MACRO=\"a\"",
                .name = "MY_MACRO",
                .value = "a",
            },
        },
        {
            .desc = "aven_build_common_cmacro",
            .fn = test_aven_build_common_cmacro,
            .args = &(TestAvenBuildCommonCMacro){
                .expected = "MY_PATH=\"a\\\\b\"",
                .name = "MY_PATH",
                .value = "a\\b",
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
