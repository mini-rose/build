/* Unit tests
   Copyright (C) 2022 bellrise */

#include "../build.h"

const char *test_name;

#define TEST(NAME) do {                                                 \
    test_name = NAME;                                                   \
    printf("Testing %s\n", NAME);                                       \
} while (0);
#define ASSERT(V1, V2)  do {                                            \
    printf("[%d] %s == %s\n", __COUNTER__ + 1, #V1, #V2);               \
    if ((V1) != (V2)) {                                                 \
        printf("\033[91m%s failed on line %d\033[0m\n", test_name, __LINE__);\
        exit(1);                                                        \
    }                                                                   \
} while (0);


int main()
{
    TEST("wordlen") {
        ASSERT(wordlen("asd"), 3);
        ASSERT(wordlen(""), 0);
        ASSERT(wordlen("string with spaces"), 6);
        ASSERT(wordlen("   !!!"), 0);
    }

    TEST("linelen") {
        ASSERT(linelen("th\nis is a line"), 2);
        ASSERT(linelen("this is a line"), 14);
        ASSERT(linelen(""), 0);
        ASSERT(linelen("   this is a line\\   \n"), 21);
    }

    TEST("strlstrip") {
        char *a, *b, *c;

        a = strlstrip("");
        b = strlstrip("  anything else");
        c = strlstrip("no spaces ");

        ASSERT(strcmp(a, ""), 0);
        ASSERT(strcmp(b, "anything else"), 0);
        ASSERT(strcmp(c, "no spaces "), 0);

        free(a);
        free(b);
        free(c);
    }

    TEST("strsplit") {
        struct strlist a = {0}, b = {0}, c = {0}, d = {0};

        strsplit(&a, "one two three");
        strsplit(&b, "");
        strsplit(&c, "  one");
        strsplit(&d, "  one-another    else ");

        ASSERT(a.size, 3);
        ASSERT(b.size, 0);
        ASSERT(c.size, 1);
        ASSERT(d.size, 2);

        ASSERT(strcmp(a.strs[0], "one"), 0);
        ASSERT(strcmp(a.strs[1], "two"), 0);
        ASSERT(strcmp(a.strs[2], "three"), 0);

        ASSERT(strcmp(c.strs[0], "one"), 0);

        ASSERT(strcmp(d.strs[0], "one-another"), 0);
        ASSERT(strcmp(d.strs[1], "else"), 0);

        strlist_free(&a);
        strlist_free(&b);
        strlist_free(&c);
        strlist_free(&d);
    }

    TEST("strreplace") {
        char *a, *b, *c;
        int al, bl, cl;

        a = strdup("11111");
        b = strdup("something_else");
        c = strdup("");

        al = strreplace(a, '1', '0');
        bl = strreplace(b, '_', '-');
        cl = strreplace(c, 'a', 'b');

        ASSERT(al, 5);
        ASSERT(bl, 1);
        ASSERT(cl, 0);

        ASSERT(strcmp(a, "00000"), 0);
        ASSERT(strcmp(b, "something-else"), 0);
        ASSERT(strcmp(c, ""), 0);

        free(a);
        free(b);
        free(c);
    }

    printf("\033[92mPassed %d tests\033[0m\n", __COUNTER__);
}
