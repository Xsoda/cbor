/*
   +------------------------------+------------------------------------+
   | Diagnostic                   | Encoded                            |
   +------------------------------+------------------------------------+
   | 0                            | 0x00                               |
   |                              |                                    |
   | 1                            | 0x01                               |
   |                              |                                    |
   | 10                           | 0x0a                               |
   |                              |                                    |
   | 23                           | 0x17                               |
   |                              |                                    |
   | 24                           | 0x1818                             |
   |                              |                                    |
   | 25                           | 0x1819                             |
   |                              |                                    |
   | 100                          | 0x1864                             |
   |                              |                                    |
   | 1000                         | 0x1903e8                           |
   |                              |                                    |
   | 1000000                      | 0x1a000f4240                       |
   |                              |                                    |
   | 1000000000000                | 0x1b000000e8d4a51000               |
   |                              |                                    |
   | 18446744073709551615         | 0x1bffffffffffffffff               |
   |                              |                                    |
   | 18446744073709551616         | 0xc249010000000000000000           |
   |                              |                                    |
   | -18446744073709551616        | 0x3bffffffffffffffff               |
   |                              |                                    |
   | -18446744073709551617        | 0xc349010000000000000000           |
   |                              |                                    |
   | -1                           | 0x20                               |
   |                              |                                    |
   | -10                          | 0x29                               |
   |                              |                                    |
   | -100                         | 0x3863                             |
   |                              |                                    |
   | -1000                        | 0x3903e7                           |
   |                              |                                    |
   | 0.0                          | 0xf90000                           |
   |                              |                                    |
   | -0.0                         | 0xf98000                           |
   |                              |                                    |
   | 1.0                          | 0xf93c00                           |
   |                              |                                    |
   | 1.1                          | 0xfb3ff199999999999a               |
   |                              |                                    |
   | 1.5                          | 0xf93e00                           |
   |                              |                                    |
   | 65504.0                      | 0xf97bff                           |
   |                              |                                    |
   | 100000.0                     | 0xfa47c35000                       |
   |                              |                                    |
   | 3.4028234663852886e+38       | 0xfa7f7fffff                       |
   |                              |                                    |
   | 1.0e+300                     | 0xfb7e37e43c8800759c               |
   |                              |                                    |
   | 5.960464477539063e-8         | 0xf90001                           |
   |                              |                                    |
   | 0.00006103515625             | 0xf90400                           |
   |                              |                                    |
   | -4.0                         | 0xf9c400                           |
   |                              |                                    |
   | -4.1                         | 0xfbc010666666666666               |
   |                              |                                    |
   | Infinity                     | 0xf97c00                           |
   |                              |                                    |
   | NaN                          | 0xf97e00                           |
   |                              |                                    |
   | -Infinity                    | 0xf9fc00                           |
   |                              |                                    |
   | Infinity                     | 0xfa7f800000                       |
   |                              |                                    |
   | NaN                          | 0xfa7fc00000                       |
   |                              |                                    |
   | -Infinity                    | 0xfaff800000                       |
   |                              |                                    |
   | Infinity                     | 0xfb7ff0000000000000               |
   |                              |                                    |
   | NaN                          | 0xfb7ff8000000000000               |
   |                              |                                    |
   | -Infinity                    | 0xfbfff0000000000000               |
   |                              |                                    |
   | false                        | 0xf4                               |
   |                              |                                    |
   | true                         | 0xf5                               |
   |                              |                                    |
   | null                         | 0xf6                               |
   |                              |                                    |
   | undefined                    | 0xf7                               |
   |                              |                                    |
   | simple(16)                   | 0xf0                               |
   |                              |                                    |
   | simple(24)                   | 0xf818                             |
   |                              |                                    |
   | simple(255)                  | 0xf8ff                             |
   |                              |                                    |
   | 0("2013-03-21T20:04:00Z")    | 0xc074323031332d30332d32315432303a |
   |                              | 30343a30305a                       |
   |                              |                                    |
   | 1(1363896240)                | 0xc11a514b67b0                     |
   |                              |                                    |
   | 1(1363896240.5)              | 0xc1fb41d452d9ec200000             |
   |                              |                                    |
   | 23(h'01020304')              | 0xd74401020304                     |
   |                              |                                    |
   | 24(h'6449455446')            | 0xd818456449455446                 |
   |                              |                                    |
   | 32("http://www.example.com") | 0xd82076687474703a2f2f7777772e6578 |
   |                              | 616d706c652e636f6d                 |
   |                              |                                    |
   | h''                          | 0x40                               |
   |                              |                                    |
   | h'01020304'                  | 0x4401020304                       |
   |                              |                                    |
   | ""                           | 0x60                               |
   |                              |                                    |
   | "a"                          | 0x6161                             |
   |                              |                                    |
   | "IETF"                       | 0x6449455446                       |
   |                              |                                    |
   | "\"\\"                       | 0x62225c                           |
   |                              |                                    |
   | "\u00fc"                     | 0x62c3bc                           |
   |                              |                                    |
   | "\u6c34"                     | 0x63e6b0b4                         |
   |                              |                                    |
   | "\ud800\udd51"               | 0x64f0908591                       |
   |                              |                                    |
   | []                           | 0x80                               |
   |                              |                                    |
   | [1, 2, 3]                    | 0x83010203                         |
   |                              |                                    |
   | [1, [2, 3], [4, 5]]          | 0x8301820203820405                 |
   |                              |                                    |
   | [1, 2, 3, 4, 5, 6, 7, 8, 9,  | 0x98190102030405060708090a0b0c0d0e |
   | 10, 11, 12, 13, 14, 15, 16,  | 0f101112131415161718181819         |
   | 17, 18, 19, 20, 21, 22, 23,  |                                    |
   | 24, 25]                      |                                    |
   |                              |                                    |
   | {}                           | 0xa0                               |
   |                              |                                    |
   | {1: 2, 3: 4}                 | 0xa201020304                       |
   |                              |                                    |
   | {"a": 1, "b": [2, 3]}        | 0xa26161016162820203               |
   |                              |                                    |
   | ["a", {"b": "c"}]            | 0x826161a161626163                 |
   |                              |                                    |
   | {"a": "A", "b": "B", "c":    | 0xa5616161416162614261636143616461 |
   | "C", "d": "D", "e": "E"}     | 4461656145                         |
   |                              |                                    |
   | (_ h'0102', h'030405')       | 0x5f42010243030405ff               |
   |                              |                                    |
   | (_ "strea", "ming")          | 0x7f657374726561646d696e67ff       |
   |                              |                                    |
   | [_ ]                         | 0x9fff                             |
   |                              |                                    |
   | [_ 1, [2, 3], [_ 4, 5]]      | 0x9f018202039f0405ffff             |
   |                              |                                    |
   | [_ 1, [2, 3], [4, 5]]        | 0x9f01820203820405ff               |
   |                              |                                    |
   | [1, [2, 3], [_ 4, 5]]        | 0x83018202039f0405ff               |
   |                              |                                    |
   | [1, [_ 2, 3], [4, 5]]        | 0x83019f0203ff820405               |
   |                              |                                    |
   | [_ 1, 2, 3, 4, 5, 6, 7, 8,   | 0x9f0102030405060708090a0b0c0d0e0f |
   | 9, 10, 11, 12, 13, 14, 15,   | 101112131415161718181819ff         |
   | 16, 17, 18, 19, 20, 21, 22,  |                                    |
   | 23, 24, 25]                  |                                    |
   |                              |                                    |
   | {_ "a": 1, "b": [_ 2, 3]}    | 0xbf61610161629f0203ffff           |
   |                              |                                    |
   | ["a", {_ "b": "c"}]          | 0x826161bf61626163ff               |
   |                              |                                    |
   | {_ "Fun": true, "Amt": -2}   | 0xbf6346756ef563416d7421ff         |
   +------------------------------+------------------------------------+

               Table 4: Examples of Encoded CBOR Data Items

*/
#define _BSD_SOURCE
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cbor.c"
#include "json.c"

const char *content[] = {
    "\x00",
    "\x01",
    "\x0a",
    "\x17",
    "\x18\x18",
    "\x18\x19",
    "\x18\x64",
    "\x19\x03\xe8",
    "\x1a\x00\x0f\x42\x40",
    "\x1b\x00\x00\x00\xe8\xd4\xa5\x10\x00",
    "\x1b\xff\xff\xff\xff\xff\xff\xff\xff",
    "\xc2\x49\x01\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x3b\xff\xff\xff\xff\xff\xff\xff\xff",
    "\xc3\x49\x01\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x20",
    "\x29",
    "\x38\x63",
    "\x39\x03\xe7",
    "\xf9\x00\x00",
    "\xf9\x80\x00",
    "\xf9\x3c\x00",
    "\xfb\x3f\xf1\x99\x99\x99\x99\x99\x9a",
    "\xf9\x3e\x00",
    "\xf9\x7b\xff",
    "\xfa\x47\xc3\x50\x00",
    "\xfa\x7f\x7f\xff\xff",
    "\xfb\x7e\x37\xe4\x3c\x88\x00\x75\x9c",
    "\xf9\x00\x01",
    "\xf9\x04\x00",
    "\xf9\xc4\x00",
    "\xfb\xc0\x10\x66\x66\x66\x66\x66\x66",
    "\xf9\x7c\x00",
    "\xf9\x7e\x00",
    "\xf9\xfc\x00",
    "\xfa\x7f\x80\x00\x00",
    "\xfa\x7f\xc0\x00\x00",
    "\xfa\xff\x80\x00\x00",
    "\xfb\x7f\xf0\x00\x00\x00\x00\x00\x00",
    "\xfb\x7f\xf8\x00\x00\x00\x00\x00\x00",
    "\xfb\xff\xf0\x00\x00\x00\x00\x00\x00",
    "\xf4",
    "\xf5",
    "\xf6",
    "\xf7",
    "\xf0",
    "\xf8\x18",
    "\xf8\xff",
    "\xc0\x74\x32\x30\x31\x33\x2d\x30\x33\x2d\x32\x31\x54\x32\x30\x3a\x30\x34\x3a\x30\x30\x5a",
    "\xc1\x1a\x51\x4b\x67\xb0",
    "\xc1\xfb\x41\xd4\x52\xd9\xec\x20\x00\x00",
    "\xd7\x44\x01\x02\x03\x04",
    "\xd8\x18\x45\x64\x49\x45\x54\x46",
    "\xd8\x20\x76\x68\x74\x74\x70\x3a\x2f\x2f\x77\x77\x77\x2e\x65\x78\x61\x6d\x70\x6c\x65\x2e\x63\x6f\x6d",
    "\x40",
    "\x44\x01\x02\x03\x04",
    "\x60",
    "\x61\x61",
    "\x64\x49\x45\x54\x46",
    "\x62\x22\x5c",
    "\x62\xc3\xbc",
    "\x63\xe6\xb0\xb4",
    "\x64\xf0\x90\x85\x91",
    "\x80",
    "\x83\x01\x02\x03",
    "\x83\x01\x82\x02\x03\x82\x04\x05",
    "\x98\x19\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x18\x18\x19",
    "\xa0",
    "\xa2\x01\x02\x03\x04",
    "\xa2\x61\x61\x01\x61\x62\x82\x02\x03",
    "\x82\x61\x61\xa1\x61\x62\x61\x63",
    "\xa5\x61\x61\x61\x41\x61\x62\x61\x42\x61\x63\x61\x43\x61\x64\x61\x44\x61\x65\x61\x45",
    "\x5f\x42\x01\x02\x43\x03\x04\x05\xff",
    "\x7f\x65\x73\x74\x72\x65\x61\x64\x6d\x69\x6e\x67\xff",
    "\x9f\xff",
    "\x9f\x01\x82\x02\x03\x9f\x04\x05\xff\xff",
    "\x9f\x01\x82\x02\x03\x82\x04\x05\xff",
    "\x83\x01\x82\x02\x03\x9f\x04\x05\xff",
    "\x83\x01\x9f\x02\x03\xff\x82\x04\x05",
    "\x9f\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x18\x18\x19\xff",
    "\xbf\x61\x61\x01\x61\x62\x9f\x02\x03\xff\xff",
    "\x82\x61\x61\xbf\x61\x62\x61\x63\xff",
    "\xbf\x63\x46\x75\x6e\xf5\x63\x41\x6d\x74\x21\xff",
    NULL
};

const char *describe[] = {
    "0",
    "1",
    "10",
    "23",
    "24",
    "25",
    "100",
    "1000",
    "1000000",
    "1000000000000",
    "18446744073709551615",
    "18446744073709551616",
    "-18446744073709551616",
    "-18446744073709551617",
    "-1",
    "-10",
    "-100",
    "-1000",
    "0.0",
    "-0.0",
    "1.0",
    "1.1",
    "1.5",
    "65504.0",
    "100000.0",
    "3.4028234663852886e+38",
    "1.0e+300",
    "5.960464477539063e-8",
    "0.00006103515625",
    "-4.0",
    "-4.1",
    "Infinity",
    "NaN",
    "-Infinity",
    "Infinity",
    "NaN",
    "-Infinity",
    "Infinity",
    "NaN",
    "-Infinity",
    "false",
    "true",
    "null",
    "undefined",
    "simple(16)",
    "simple(24)",
    "simple(255)",
    "0(\"2013-03-21T20:04:00Z\")",
    "1(1363896240)",
    "1(1363896240.5)",
    "23(h\"01020304\")",
    "24(h\"6449455446\")",
    "32(\"http://www.example.com\")",
    "h\"\"",
    "h\"01020304\"",
    "\"\"",
    "a",
    "IETF",
    "\\\"\\\\",
    "\\u00fc",
    "\\u6c34",
    "\\ud800\\udd51",
    "[]",
    "[1, 2, 3]",
    "[1, [2, 3], [4, 5]]",
    "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]",
    "{}",
    "{1: 2, 3: 4}",
    "{\"a\": 1, \"b\": [2, 3]}",
    "[\"a\", {\"b\": \"c\"}]",
    "{\"a\": \"A\", \"b\": \"B\", \"c\": \"C\", \"d\": \"D\", \"e\": \"E\"}",
    "(_ h\"0102\", h\"030405\")",
    "(_ \"strea\", \"ming\")",
    "[_ ]",
    "[_ 1, [2, 3], [_ 4, 5]]",
    "[_ 1, [2, 3], [4, 5]]",
    "[1, [2, 3], [_ 4, 5]]",
    "[1, [_ 2, 3], [4, 5]]",
    "[_ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]",
    "{_ \"a\": 1, \"b\": [_ 2, 3]}",
    "[\"a\", {_ \"b\": \"c\"}]",
    "{_ \"Fun\": true, \"Amt\": -2}",
    NULL
};

static int len[] = {
    1,
    1,
    1,
    1,
    2,
    2,
    2,
    3,
    5,
    9,
    9,
    11,
    9,
    11,
    1,
    1,
    2,
    3,
    3,
    3,
    3,
    9,
    3,
    3,
    5,
    5,
    9,
    3,
    3,
    3,
    9,
    3,
    3,
    3,
    5,
    5,
    5,
    9,
    9,
    9,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    22,
    6,
    10,
    6,
    8,
    25,
    1,
    5,
    1,
    2,
    5,
    3,
    3,
    4,
    5,
    1,
    4,
    8,
    29,
    1,
    5,
    9,
    8,
    21,
    9,
    13,
    2,
    10,
    9,
    9,
    9,
    29,
    11,
    9,
    12
};

char *hexprint(const char *ptr, size_t len) {
    int l = 0;
    char *buf = (char *)malloc(len * 4 + 1);
    for (int i = 0; i < len; i++) {
        l += sprintf(buf + l, "\\x%02x", (unsigned char)ptr[i]);
    }
    return buf;
}

int test_f64() {
    union {
        uint64_t u64;
        double dbl;
    } f64_val;
    {
        f64_val.dbl = INFINITY;
        int exponent = (f64_val.u64 >> 52) & 0x7FF;
        int sign = f64_val.u64 >> 63;
        uint64_t frac = f64_val.u64 & 0xFFFFFFFFFFFFF;
        printf("number: %lf, exponent: %d, frac: %lld\n", f64_val.dbl, exponent, frac);
    }
    {
        f64_val.dbl = NAN;
        int exponent = (f64_val.u64 >> 52) & 0x7FF;
        int sign = f64_val.u64 >> 63;
        uint64_t frac = f64_val.u64 & 0xFFFFFFFFFFFFF;
        printf("number: %lf, exponent: %d, frac: %lld\n", f64_val.dbl, exponent, frac);
    }
}

int test_json() {
    size_t length;
    double dbl = 1.1e200;
    printf("start load\n");
    cbor_value_t *val = cbor_json_loadf("97838.json");
    printf("end load\n");
    cbor_value_t *dup = cbor_duplicate(val);
    printf("end duplicate\n");
    char *content = cbor_dumps(val, &length);
    printf("end dump\n");
    free(content);
    content = cbor_json_dumps(val, &length, true);
    printf("end dumpj: %d\n", length);
    free(content);
    cbor_destroy(val);
    cbor_destroy(dup);
    return 0;
}

int assert_a_next_b(cbor_value_t *a, cbor_value_t *b) {
    assert(a->entry.le_next == b);
    assert(b->entry.le_prev == &a->entry.le_next);
    return 0;
}

int assert_container_start_a(cbor_value_t *con, cbor_value_t *a) {
    assert(a->entry.le_prev == &con->container.lh_first);
    assert(con->container.lh_first == a);
    return 0;
}

int assert_container_end_a(cbor_value_t *con, cbor_value_t *a) {
    assert(con->container.lh_last == &a->entry.le_next);
    assert(a->entry.le_next == NULL);
    return 0;
}

int test_slice() {
    cbor_value_t *src, *dst, *start, *stop, *first, *last, *prev, *next;
    printf("test slice ..............\n");
    src = cbor_init_array();
    dst = cbor_init_array();
    for (int i = 0; i < 100; i++) {
        cbor_container_insert_tail(src, cbor_init_integer(i));
    }
    first = cbor_container_first(src);
    last = cbor_container_last(src);
    start = cbor_array_get(src, 50);
    stop = cbor_container_prev(src, start);
    cbor_container_slice_before(dst, src, start);
    assert_container_start_a(dst, first);
    assert_container_end_a(dst, stop);
    assert_container_start_a(src, start);
    assert_container_end_a(src, last);
    printf("slice before done\n");

    cbor_container_concat(dst, src);
    assert_container_start_a(dst, first);
    assert_container_end_a(dst, last);
    printf("concat done\n");

    stop = cbor_container_next(dst, start);
    cbor_container_slice_after(src, dst, start);
    assert_container_start_a(dst, first);
    assert_container_end_a(dst, start);
    assert_container_start_a(src, stop);
    assert_container_end_a(src, last);
    printf("slice after done\n");

    cbor_container_concat(dst, src);
    assert_container_start_a(dst, first);
    assert_container_end_a(dst, last);
    printf("concat done\n");

    stop = cbor_array_get(dst, 60);
    prev = cbor_container_prev(dst, start);
    next = cbor_container_next(dst, stop);
    cbor_container_slice(src, dst, start, stop);
    assert_container_start_a(src, start);
    assert_container_end_a(src, stop);
    assert_container_start_a(dst, first);
    assert_container_end_a(dst, last);
    assert_a_next_b(prev, next);
    printf("1. slice done\n");

    cbor_container_clear(src);
    start = cbor_container_first(dst);
    stop = cbor_array_get(dst, 10);
    first = cbor_container_next(dst, stop);
    last = cbor_container_last(dst);
    cbor_container_slice(src, dst, start, stop);
    assert_container_start_a(src, start);
    assert_container_end_a(src, stop);
    assert_container_start_a(dst, first);
    assert_container_end_a(dst, last);
    printf("2. slice done\n");

    cbor_container_clear(src);
    start = cbor_array_get(dst, 20);
    last = cbor_container_prev(dst, start);
    stop = cbor_container_last(dst);
    first = cbor_container_first(dst);
    cbor_container_slice(src, dst, start, stop);
    assert_container_start_a(src, start);
    assert_container_end_a(src, stop);
    assert_container_start_a(dst, first);
    assert_container_end_a(dst, last);
    printf("3. slice done\n");
}

int test_json_pointer() {
    const char *content =
        "{\
      \"foo\": [\"bar\", \"baz\"],\
      \"\": 0,\
      \"a/b\": 1,\
      \"c%d\": 2,\
      \"e^f\": 3,\
      \"g|h\": 4,\
      \"i\\\\j\": 5,\
      \"k\\\"l\": 6,\
      \" \": 7,\
      \"m~n\": 8\
}";
    cbor_value_t *val = cbor_json_loads(content, -1);
    cbor_value_t *e = cbor_pointer_eval(val, "");
    assert(val == e);
    e = cbor_pointer_eval(val, "/foo/0");
    assert(strcmp(cbor_string(e), "bar") == 0);
    e = cbor_pointer_eval(val, "/");
    assert(cbor_integer(e) == 0);
    e = cbor_pointer_eval(val, "/a~1b");
    assert(cbor_integer(e) == 1);
    e = cbor_pointer_eval(val, "/c%d");
    assert(cbor_integer(e) == 2);
    e = cbor_pointer_eval(val, "/e^f");
    assert(cbor_integer(e) == 3);
    e = cbor_pointer_eval(val, "/g|h");
    assert(cbor_integer(e) == 4);
    e = cbor_pointer_eval(val, "/i\\j");
    assert(cbor_integer(e) == 5);
    e = cbor_pointer_eval(val, "/k\"l");
    assert(cbor_integer(e) == 6);
    e = cbor_pointer_eval(val, "/ ");
    assert(cbor_integer(e) == 7);
    e = cbor_pointer_eval(val, "/m~0n");
    assert(cbor_integer(e) == 8);
}

int main(int argc, char **argv) {
    size_t length;
    for (int i = 0; content[i] != NULL; i++) {
        printf("start load %d %s\n", i, describe[i]);
        length = len[i];
        cbor_value_t *val = cbor_loads(content[i], &length);
        if (val == NULL) {
            printf("load fail: %s\n", describe[i]);
        }
        if (val && length != len[i]) {
            printf("load length error: %s (%d) - %d\n", describe[i], len[i], length);
        }
        if (cbor_is_double(val)) {
            printf("value is %lf\n", val->simple.real);
        }
        if (val->type == CBOR_TYPE_UINT) {
            printf("value is %llu\n", val->uint);
        }
        if (val->type == CBOR_TYPE_NEGINT) {
            printf("value is %lld\n", -val->uint - 1);
        }
        char *buf = hexprint(content[i], len[i]);
        printf("origin: %s\n", buf);
        free(buf);
        char *ser = cbor_dumps(val, &length);
        if (ser)
            buf = hexprint(ser, length);
        else
            buf = NULL;
        printf("serial: %s\n", buf);
        free(buf);
        cbor_destroy(val);
        printf("-----------------------\n");
    }
    test_f64();
    test_json();
    test_slice();
    test_json_pointer();
    return 0;
}
