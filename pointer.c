#include "cbor.h"
#include "define.h"
#include <assert.h>
#include <string.h>
#include <stdarg.h>

cbor_value_t *cbor_map_find(const cbor_value_t *container, const char *key) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    cbor_iter_init(&iter, container, CBOR_ITER_AFTER);
    assert(cbor_is_map(container));
    while ((ele = cbor_iter_next(&iter)) != NULL) {
        assert(cbor_is_string(ele->pair.key));
        if (!strcmp(cbor_string(ele->pair.key), key)) {
            return ele;
        }
    }
    return NULL;
}

/* return: destination value */
cbor_value_t *cbor_pointer_get(const cbor_value_t *container, const char *path) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    const cbor_value_t *current = NULL;
    cbor_value_t *next = NULL;
    cbor_value_t *split = cbor_string_split(path, "/");

    if (path[0] == 0) {
        cbor_destroy(split);
        return (cbor_value_t *)container;
    }

    cbor_iter_init(&iter, split, CBOR_ITER_AFTER);
    while ((ele = cbor_iter_next(&iter)) != NULL) {
        cbor_string_replace(ele, "~1", "/");
        cbor_string_replace(ele, "~0", "~");

        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele));
                if (find) {
                    next = cbor_pair_value(find);
                    current = next;
                    continue;
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    next = cbor_container_last(current);
                    current = next;
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0) {
                        cbor_value_t *val = cbor_container_first(current);
                        for ( ; val != NULL && idx > 0; idx--) {
                            val = cbor_container_next(current, val);
                        }
                        if (val) {
                            next = val;
                            current = next;
                            continue;
                        }
                    }
                }
            }
            next = NULL;
            break;
        }
    }
    cbor_destroy(split);
    return next;
}

/*
 * |-     function      -|- Overwrite if already exists? -|- Create if does not exists? -|- Return -|
 * | cbor_pointer_insert |           No                   |      Yes                     |  parent  |
 * | cbor_pointer_replace|           Yes                  |      No                      |  parent  |
 * | cbor_pointer_set    |           Yes                  |      Yes                     |  parent  |
 */
cbor_value_t *cbor_pointer_insert(cbor_value_t *container, const char *path, cbor_value_t *value) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;

    if (value == NULL) {
        return NULL;
    }
    assert(value->parent == NULL);

    cbor_value_t *split = cbor_string_split(path, "/");

    cbor_iter_init(&iter, split, CBOR_ITER_AFTER);
    while ((ele = cbor_iter_next(&iter)) != NULL) {
        cbor_string_replace(ele, "~1", "/");
        cbor_string_replace(ele, "~0", "~");

        if (cbor_container_next(split, ele) == NULL) {
            last = true;
        }
        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele));
                if (find) {
                    if (last) {
                        current = NULL;
                        break;
                    } else {
                        current = cbor_pair_value(find);
                    }
                } else if (last) {
                    cbor_value_t *val = cbor_init_pair(cbor_duplicate(ele), value);
                    cbor_container_insert_tail(current, val);
                    continue;
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    if (last) {
                        cbor_container_insert_tail(current, value);
                    } else {
                        current = cbor_container_last(current);
                    }
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0) {
                        if (cbor_container_empty(current) && idx == 0 && last) {
                            cbor_container_insert_head(current, value);
                            continue;
                        }
                        cbor_value_t *val = cbor_container_first(current);
                        cbor_value_t *prev = NULL;
                        for ( ; idx > 0; idx--) {
                            prev = val;
                            val = cbor_container_next(current, val);
                        }
                        if (val) {
                            if (!last) {
                                current = val;
                                continue;
                            }
                        } else {
                            if (last && prev != NULL) {
                                cbor_container_insert_after(current, prev, value);
                                continue;
                            }
                        }
                    }
                }
                current = NULL;
                break;
            }
        }
    }
    cbor_destroy(split);
    return current;
}

/* return: replaced value's parent container */
cbor_value_t *cbor_pointer_replace(cbor_value_t *container, const char *path, cbor_value_t *value) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;

    if (value == NULL) {
        return NULL;
    }

    assert(value->parent == NULL);

    cbor_value_t *split = cbor_string_split(path, "/");

    cbor_iter_init(&iter, split, CBOR_ITER_AFTER);
    while ((ele = cbor_iter_next(&iter)) != NULL) {
        cbor_string_replace(ele, "~1", "/");
        cbor_string_replace(ele, "~0", "~");

        if (cbor_container_next(split, ele) == NULL) {
            last = true;
        }

        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele));
                if (find) {
                    if (last) {
                        cbor_value_t *tmp = cbor_pair_set_value(find, value);
                        cbor_destroy(tmp);
                    } else {
                        current = cbor_pair_value(find);
                    }
                    continue;
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    cbor_value_t *val = cbor_container_last(current);
                    if (val) {
                        if (last) {
                            cbor_container_insert_before(current, val, value);
                            cbor_container_remove(current, val);
                            cbor_destroy(val);
                        } else {
                            current = val;
                        }
                        continue;
                    }
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0) {
                        cbor_value_t *val = cbor_container_first(current);
                        for ( ; idx > 0; idx--) {
                            val = cbor_container_next(current, val);
                        }
                        if (val) {
                            if (last) {
                                cbor_container_insert_before(current, val, value);
                                cbor_container_remove(current, val);
                                cbor_destroy(val);
                            } else {
                                current = val;
                            }
                            continue;
                        }
                    }
                }
            }
            current = NULL;
            break;
        }
    }
    cbor_destroy(split);
    return current;
}

cbor_value_t *cbor_pointer_set(cbor_value_t *container, const char *path, cbor_value_t *value) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;

    if (value == NULL) {
        return NULL;
    }
    assert(value->parent == NULL);

    cbor_value_t *split = cbor_string_split(path, "/");

    cbor_iter_init(&iter, split, CBOR_ITER_AFTER);
    while ((ele = cbor_iter_next(&iter)) != NULL) {
        cbor_string_replace(ele, "~1", "/");
        cbor_string_replace(ele, "~0", "~");

        if (cbor_container_next(split, ele) == NULL) {
            last = true;
        }

        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele));
                if (find) {
                    if (last) {
                        cbor_value_t *tmp = cbor_pair_set_value(find, value);
                        cbor_destroy(tmp);
                    } else {
                        current = cbor_pair_value(find);
                    }
                    continue;
                } else if (last) {
                    cbor_value_t *val = cbor_init_pair(cbor_duplicate(ele), value);
                    cbor_container_insert_tail(current, val);
                    continue;
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    if (last) {
                        cbor_container_insert_tail(current, value);
                    } else {
                        current = cbor_container_last(current);
                    }
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0) {
                        if (cbor_container_empty(current) && idx == 0 && last) {
                            cbor_container_insert_head(current, value);
                            continue;
                        }
                        cbor_value_t *prev = NULL;
                        cbor_value_t *val = cbor_container_first(current);
                        for ( ; idx > 0; idx--) {
                            prev = val;
                            val = cbor_container_next(current, val);
                        }
                        if (val) {
                            if (last) {
                                cbor_container_insert_before(current, val, value);
                            } else {
                                current = val;
                            }
                            continue;
                        } else {
                            if (last && prev != NULL) {
                                cbor_container_insert_after(current, prev, value);
                                continue;
                            }
                        }
                    }
                }
                current = NULL;
                break;
            }
        }
    }
    cbor_destroy(split);
    return current;
}

/*
 * define MergePatch(Target, Patch):
 *    if Patch is an Object:
 *       if Target is not an Object:
 *          Target = {} # Ignore the contents and set it to an empty Object
 *       for each Name/Value pair in Patch:
 *          if Value is null:
 *             if Name exists in Target:
 *                remove the Name/Value pair from Target
 *          else:
 *             Target[Name] = MergePatch(Target[Name], Value)
 *       return Target
 *    else:
 *       return Patch
 */
cbor_value_t *cbor_merge_patch(cbor_value_t *target, const cbor_value_t *patch) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    if (cbor_is_map(patch)) {
        if (!cbor_is_map(target)) {
            cbor_value_t *tmp = cbor_init_map();
            cbor_copy(target, tmp);
            cbor_destroy(tmp);
        }
        cbor_iter_init(&iter, patch, CBOR_ITER_AFTER);
        while ((ele = cbor_iter_next(&iter)) != NULL) {
            cbor_value_t *key = cbor_pair_key(ele);
            cbor_value_t *value = cbor_pair_value(ele);
            cbor_value_t *find = cbor_map_find(target, cbor_string(key));
            if (cbor_is_null(value)) {
                if (find != NULL) {
                    cbor_container_remove(target, find);
                    cbor_destroy(find);
                }
            } else {
                if (find != NULL) {
                    cbor_merge_patch(cbor_pair_value(find), value);
                } else {
                    cbor_value_t *var = cbor_merge_patch(cbor_init_null(), value);
                    cbor_value_t *pair = cbor_init_pair(cbor_duplicate(key), var);
                    cbor_container_insert_tail(target, pair);
                }
            }
        }
    } else {
        cbor_copy(target, patch);
    }
    return target;
}

int cbor_patch(cbor_value_t *dst, const cbor_value_t *src) {
    cbor_merge_patch(dst, src);
    return 0;
}

/* return: removed value */
cbor_value_t *cbor_pointer_remove(cbor_value_t *container, const char *path) {
    cbor_value_t *remval = cbor_pointer_get(container, path);
    if (remval && remval->parent) {
        if (remval->parent->type == CBOR_TYPE_ARRAY) {
            cbor_container_remove(remval->parent, remval);
            return remval;
        }
        if (remval->parent->type == CBOR__TYPE_PAIR) {
            cbor_value_t *pair = remval->parent;
            if (pair->parent && pair->parent->type == CBOR_TYPE_MAP) {
                cbor_container_remove(pair->parent, pair);
                cbor_value_t *val = cbor_pair_set_value(pair, cbor_init_null());
                cbor_destroy(pair);
                assert(val == remval);
                return remval;
            }
        }
    }
    return NULL;
}

int cbor_pointer_seti(cbor_value_t *container, const char *path, long long integer) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_integer(integer);
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setb(cbor_value_t *container, const char *path, bool boolean) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_boolean(boolean);
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_sets(cbor_value_t *container, const char *path, const char *str) {
    assert(path != NULL && path[0] == '/');
    if (str == NULL) {
        str = "";
    }
    cbor_value_t *val = cbor_init_string(str, -1);
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setf(cbor_value_t *container, const char *path, double dbl) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_double(dbl);
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setn(cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_null();
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_seto(cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_map();
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_seta(cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_array();
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setv(cbor_value_t *container, const char *path, cbor_value_t *val) {
    if (!container || !path || !val) {
        return -1;
    }
    assert(container->type == CBOR_TYPE_MAP || container->type == CBOR_TYPE_ARRAY);
    assert(path[0] == '/');
    assert(val->parent == NULL);
    if (cbor_pointer_set(container, path, val)) {
        return 0;
    }
    return -1;
}

long long cbor_pointer_geti(const cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_pointer_get(container, path);
    return cbor_integer(val);
}

const char *cbor_pointer_gets(const cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_pointer_get(container, path);
    return cbor_string(val);
}

bool cbor_pointer_getb(const cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_pointer_get(container, path);
    return cbor_boolean(val);
}

double cbor_pointer_getf(const cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_pointer_get(container, path);
    return cbor_real(val);
}

int cbor_pointer_join(char *buf, size_t size, ...) {
    va_list ap;
    size_t len = 0;
    va_start(ap, size);
    const char *arg = va_arg(ap, const char *);
    while (arg != NULL && len + 1 < size) {
        buf[len++] = '/';
        while (*arg && len + 2 < size) {
            if (*arg == '~') {
                buf[len++] = '~';
                buf[len++] = '0';
            } else if (*arg == '/') {
                buf[len++] = '~';
                buf[len++] = '1';
            } else {
                buf[len++] = *arg;
            }
            arg++;
        }
        arg = va_arg(ap, const char *);
    }
    va_end(ap);
    if (arg == NULL && len + 1 < size) {
        buf[len] = 0;
        return len;
    }
    return 0;
}
