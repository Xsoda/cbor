#include "cbor.h"
#include <stdio.h>
#include <string.h>

/***
 * json_pointer_insert('[1,2,3,4]','/-',99) → '[1,2,3,4,99]'
 * json_pointer_insert('[1,[2,3],4]','/1/-',99) → '[1,[2,3,99],4]'
 * json_pointer_insert('{"a":2,"c":4}', '/a', 99) → '{"a":2,"c":4}'
 * json_pointer_insert('{"a":2,"c":4}', '/e', 99) → '{"a":2,"c":4,"e":99}'
 * json_pointer_replace('{"a":2,"c":4}', '/a', 99) → '{"a":99,"c":4}'
 * json_pointer_replace('{"a":2,"c":4}', '/e', 99) → '{"a":2,"c":4}'
 * json_pointer_set('{"a":2,"c":4}', '/a', 99) → '{"a":99,"c":4}'
 * json_pointer_set('{"a":2,"c":4}', '/e', 99) → '{"a":2,"c":4,"e":99}'
 * json_pointer_set('{"a":2,"c":4}', '/c', '[97,96]') → '{"a":2,"c":"[97,96]"}'
 * json_pointer_set('{"a":2,"c":4}', '/c', json('[97,96]')) → '{"a":2,"c":[97,96]}'
 * json_pointer_set('{"a":2,"c":4}', '/c', json_array(97,96)) → '{"a":2,"c":[97,96]}'
 */


#define JSON(...) #__VA_ARGS__

void insert_test(const char *input, const char *path, const char *value, const char *output) {
    size_t length;
    cbor_value_t *src = cbor_json_loads(input, -1);
    cbor_value_t *val = cbor_json_loads(value, -1);
    cbor_value_t *result = cbor_pointer_insert(src, path, val);
    char *content = cbor_json_dumps(src, &length, false);
    if (strcmp(content, output)) {
        fprintf(stderr, "FAIL: cbor_pointer_insert('%s', '%s', '%s') -> '%s'\n", input, path, value, content);
    } else {
        fprintf(stdout, "PASS: cbor_pointer_insert('%s', '%s', '%s') -> '%s'\n", input, path, value, content);
    }
    cbor_destroy(src);
    if (result == NULL) {
        cbor_destroy(val);
    }
    free(content);
}

void replace_test(const char *input, const char *path, const char *value, const char *output) {
    size_t length;
    cbor_value_t *src = cbor_json_loads(input, -1);
    cbor_value_t *val = cbor_json_loads(value, -1);
    cbor_value_t *result = cbor_pointer_replace(src, path, val);
    char *content = cbor_json_dumps(src, &length, false);
    if (strcmp(content, output)) {
        fprintf(stderr, "FAIL: cbor_pointer_replace('%s', '%s', '%s') -> '%s'\n", input, path, value, content);
    } else {
        fprintf(stdout, "PASS: cbor_pointer_replace('%s', '%s', '%s') -> '%s'\n", input, path, value, content);
    }
    cbor_destroy(src);
    if (result == NULL) {
        cbor_destroy(val);
    }
    free(content);
}

void set_test(const char *input, const char *path, const char *value, const char *output) {
    size_t length;
    cbor_value_t *src = cbor_json_loads(input, -1);
    cbor_value_t *val = cbor_json_loads(value, -1);
    cbor_value_t *result = cbor_pointer_set(src, path, val);
    char *content = cbor_json_dumps(src, &length, false);
    if (strcmp(content, output)) {
        fprintf(stderr, "FAIL: cbor_pointer_set('%s', '%s', '%s') -> '%s'\n", input, path, value, content);
    } else {
        fprintf(stdout, "PASS: cbor_pointer_set('%s', '%s', '%s') -> '%s'\n", input, path, value, content);
    }
    cbor_destroy(src);
    if (result == NULL) {
        cbor_destroy(val);
    }
    free(content);
}

void patch_test(const char *input, const char *value, const char *output) {
    size_t length;
    cbor_value_t *src = cbor_json_loads(input, -1);
    cbor_value_t *val = cbor_json_loads(value, -1);
    cbor_patch(src, val);
    char *content = cbor_json_dumps(src, &length, false);
    if (strcmp(content, output)) {
        fprintf(stderr, "FAIL: cbor_patch('%s', '%s') -> '%s'\n", input, value, content);
    } else {
        fprintf(stdout, "PASS: cbor_patch('%s', '%s') -> '%s'\n", input, value, content);
    }
    cbor_destroy(src);
    cbor_destroy(val);
    free(content);
}


int main(int argc, char **argv) {
    insert_test(JSON([1,2,3,4]), "/-", JSON(99), JSON([1, 2, 3, 4, 99]));
    insert_test(JSON([1,[2,3],4]), "/1/-", JSON(99), JSON([1, [2, 3, 99], 4]));
    insert_test(JSON({"a":2,"c":4}), "/a", JSON(99), JSON({"a": 2, "c": 4}));
    insert_test(JSON({"a":2,"c":4}), "/e", JSON(99), JSON({"a": 2, "c": 4, "e": 99}));
    replace_test(JSON({"a":2,"c":4}), "/a", JSON(99), JSON({"a": 99, "c": 4}));
    replace_test(JSON({"a":2,"c":4}), "/e", JSON(99), JSON({"a": 2, "c": 4}));
    set_test(JSON({"a":2,"c":4}), "/a", JSON(99), JSON({"a": 99, "c": 4}));
    set_test(JSON({"a":2,"c":4}), "/e", JSON(99), JSON({"a": 2, "c": 4, "e": 99}));
    set_test(JSON({"a":2,"c":4}), "/c", JSON("[97, 96]"), JSON({"a": 2, "c": "[97, 96]"}));
    set_test(JSON({"a":2,"c":4}), "/c", JSON([97, 96]), JSON({"a": 2, "c": [97, 96]}));
    patch_test(JSON({"a":1,"b":2}), JSON({"c":3,"d":4}), JSON({"a": 1, "b": 2, "c": 3, "d": 4}));
    patch_test(JSON({"a":[1,2],"b":2}), JSON({"a":9}), JSON({"a": 9, "b": 2}));
    patch_test(JSON({"a":[1,2],"b":2}), JSON({"a":null}), JSON({"b": 2}));
    patch_test(JSON({"a":1,"b":2}), JSON({"a":9,"b":null,"c":8}), JSON({"a": 9, "c": 8}));
    patch_test(JSON({"a":{"x":1,"y":2},"b":3}), JSON({"a":{"y":9},"c":8}), JSON({"a": {"x": 1, "y": 9}, "b": 3, "c": 8}));
    patch_test(JSON({}), JSON({"a":{"bb":{"ccc":null}}}), JSON({"a": {"bb": {}}}));
    patch_test(JSON([1,2]), JSON({"a":"b","c":null}), JSON({"a": "b"}));
    patch_test(JSON(["a", "b"]), JSON(["c", "d"]), JSON(["c", "d"]));
    patch_test(JSON({"a": "b"}), JSON(["c"]), JSON(["c"]));
    return 0;
}
