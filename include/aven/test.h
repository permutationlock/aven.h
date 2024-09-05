#ifndef AVEN_TEST_H
#define AVEN_TEST_H

#include "../aven.h"
#include "arena.h"

typedef struct {
    int error;
    const char *message;
} AvenTestResult;

typedef AvenTestResult (*AvenTestFn)(AvenArena arena, void *args);

typedef struct {
    const char *desc;
    void *args;
    AvenTestFn fn;
} AvenTestCase;

typedef Slice(AvenTestCase) AvenTestCaseSlice;

AVEN_FN void aven_test(
    AvenTestCaseSlice tcases,
    const char *fname,
    AvenArena arena
);

#ifdef AVEN_IMPLEMENTATION

#include <stdio.h>

AVEN_FN void aven_test(
    AvenTestCaseSlice tcases,
    const char *fname,
    AvenArena arena
) {
    printf("running %lu test(s) for %s:", (unsigned long)tcases.len, fname);
    size_t passed = 0;
    for (size_t i = 0; i < tcases.len; i += 1) {
        AvenTestCase *tcase = &slice_get(tcases, i);
        AvenTestResult result = tcase->fn(arena, tcase->args);
        if (result.error != 0) {
            printf(
                "\n\ttest \"%s\" failed:\n\t\t\"%s\"\n\t\tcode: %d",
                tcase->desc,
                result.message,
                result.error
            );
        } else {
            passed += 1;
        }
    }

    if (passed == tcases.len) {
        printf(" all tests passed\n");
    } else {
        printf(
            "\ncompleted tests for %s: %lu passed, %lu failed\n",
            fname,
            (unsigned long)passed,
            (unsigned long)(tcases.len - passed)
        );
    }
}

#endif // AVEN_IMPLEMENTATION

#endif // TEST_H
