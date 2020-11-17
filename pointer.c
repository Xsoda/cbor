#include "cbor.h"
#include "define.h"
#include <assert.h>
#include <string.h>
#include <stdarg.h>

bool cbor_value_test(const cbor_value_t *va, const cbor_value_t *vb) {
    if (va == NULL || vb == NULL) {
        return false;
    }
    if (va->type != vb->type) {
        return false;
    }
    if (va->type == CBOR_TYPE_UINT || va->type == CBOR_TYPE_NEGINT) {
        return cbor_integer(va) == cbor_integer(vb);
    }
    if (va->type == CBOR_TYPE_STRING || vb->type == CBOR_TYPE_BYTESTRING) {
        if (cbor_string_size(va) == cbor_string_size(vb)) {
            if (!memcmp(cbor_string(va), cbor_string(vb), cbor_string_size(va))) {
                return true;
            }
        }
        return false;
    }
    if (va->type == CBOR_TYPE_ARRAY) {
        cbor_value_t *ea, *eb;
        for (ea = cbor_container_first(va), eb = cbor_container_first(vb);
             ea != NULL && eb != NULL;
             ea = cbor_container_next(va, ea), eb = cbor_container_next(vb, eb)) {
            if (!cbor_value_test(ea, eb)) {
                return false;
            }
        }
        return true;
    }
    if (va->type == CBOR_TYPE_MAP) {
        cbor_value_t *ea, *eb;
        if (cbor_container_size(va) != cbor_container_size(vb)) {
            return false;
        }
        for (ea = cbor_container_first(va); ea != NULL; ea = cbor_container_next(va, ea)) {
            for (eb = cbor_container_first(vb); eb != NULL; eb = cbor_container_next(vb, eb)) {
                if (cbor_value_test(ea->pair.key, eb->pair.key)) {
                    if (!cbor_value_test(ea->pair.value, eb->pair.value)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    if (va->type == CBOR_TYPE_SIMPLE) {
        if (va->simple.ctrl == vb->simple.ctrl && va->simple.real == vb->simple.real) {
            return true;
        }
    }
    if (va->type == CBOR_TYPE_TAG) {
        return va->tag.item == vb->tag.item && cbor_value_test(va->tag.content, vb->tag.content);
    }
    return false;
}

/* return: destination value */
cbor_value_t *cbor_pointer_get(const cbor_value_t *container, const char *path) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    const cbor_value_t *current = NULL;
    cbor_value_t *next = NULL;
    cbor_value_t *split = cbor_string_split(path, "/");

    cbor_iter_init(&iter, split, CBOR_ITER_AFTER);
    while ((ele = cbor_iter_next(&iter)) != NULL) {
        cbor_string_replace(ele, "~1", "/");
        cbor_string_replace(ele, "~0", "~");

        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find;
                for (find = cbor_container_first(current);
                     find != NULL;
                     find = cbor_container_next(current, find)) {
                    if (cbor_value_test(ele, find->pair.key)) {
                        break;
                    }
                }
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
                pair->pair.value = NULL;
                cbor_destroy(pair);
                return remval;
            }
        }
    }
    return NULL;
}

/* return: added value's parent container */
cbor_value_t *cbor_pointer_add(cbor_value_t *container, const char *path, cbor_value_t *value) {
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
                cbor_value_t *find;
                for (find = cbor_container_first(current);
                     find != NULL;
                     find = cbor_container_next(current, find)) {
                    if (cbor_value_test(ele, find->pair.key)) {
                        break;
                    }
                }
                if (find) {
                    if (last) {
                        cbor_destroy(find->pair.value);
                        find->pair.value = value;
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
                        cbor_value_t *val = cbor_container_first(current);
                        for ( ; idx > 0; idx--) {
                            val = cbor_container_next(current, val);
                        }
                        if (val) {
                            if (last) {
                                cbor_container_insert_before(current, val, value);
                            } else {
                                current = val;
                            }
                            continue;
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
                cbor_value_t *find;
                for (find = cbor_container_first(current);
                     find != NULL;
                     find = cbor_container_next(current, find)) {
                    if (cbor_value_test(ele, find->pair.key)) {
                        break;
                    }
                }
                if (find) {
                    if (last) {
                        cbor_destroy(find->pair.value);
                        find->pair.value = value;
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

/* return: moved value */
cbor_value_t *cbor_pointer_move(cbor_value_t *container, const char *from, const char *path) {
    cbor_value_t *ele;
    cbor_iter_t iter;
    bool last = false;
    cbor_value_t *current = NULL;
    cbor_value_t *split = cbor_string_split(from, "/");

    cbor_value_t *root = NULL, *value = NULL;

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
                cbor_value_t *find;
                for (find = cbor_container_first(current);
                     find != NULL;
                     find = cbor_container_next(current, find)) {
                    if (cbor_value_test(ele, find->pair.key)) {
                        break;
                    }
                }
                if (find) {
                    if (last) {
                        root = current;
                        value = find;
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
                            value = val;
                            root = current;
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
                                value = val;
                                root = current;
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

    if (current == NULL || root == NULL || value == NULL) {
        return NULL;
    }

    last = false;
    current = NULL;
    cbor_destroy(split);

    split = cbor_string_split(path, "/");
    cbor_iter_init(&iter, split, CBOR_ITER_AFTER);
    while ((ele = cbor_iter_next(&iter))) {
        cbor_string_replace(ele, "~1", "/");
        cbor_string_replace(ele, "~0", "~");

        if (cbor_container_next(split, ele) == NULL) {
            last = true;
        }

        if (current == value) {
            current = NULL;
            break;
        }

        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find;
                for (find = cbor_container_first(current);
                     find != NULL;
                     find = cbor_container_next(current, find)) {
                    if (cbor_value_test(ele, find->pair.key)) {
                        break;
                    }
                }
                if (find) {
                    if (last) {
                        cbor_container_remove(root, value);
                        if (root->type == CBOR_TYPE_MAP) {
                            cbor_value_t *tmp = value->pair.value;
                            value->pair.value = NULL;
                            cbor_destroy(value);
                            value = tmp;
                        }
                        cbor_destroy(find->pair.value);
                        find->pair.value = value;
                    } else {
                        current = cbor_pair_value(find);
                    }
                    continue;
                } else {
                    if (last) {
                        cbor_container_remove(root, value);
                        if (root->type == CBOR_TYPE_MAP) {
                            cbor_destroy(value->pair.key);
                            value->pair.key = cbor_duplicate(ele);
                        } else {
                            cbor_value_t *tmp = cbor_init_pair(cbor_duplicate(ele), value);
                            value = tmp;
                        }
                        cbor_container_insert_tail(current, value);
                        continue;
                    }
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    if (last) {
                        cbor_container_remove(root, value);
                        if (cbor_is_map(root)) {
                            cbor_value_t *tmp = value->pair.value;
                            value->pair.value = NULL;
                            cbor_destroy(value);
                            value = tmp;
                        }
                        cbor_container_insert_tail(current, value);
                    } else {
                        current = cbor_container_last(current);
                    }
                    continue;
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
                                cbor_container_remove(root, value);
                                if (root->type == CBOR_TYPE_MAP) {
                                    cbor_value_t *tmp = value->pair.value;
                                    value->pair.value = NULL;
                                    cbor_destroy(value);
                                    value = tmp;
                                }
                                cbor_container_insert_before(current, val, value);
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

/* return: copyed value */
cbor_value_t *cbor_pointer_copy(cbor_value_t *container, const char *from, const char *path) {
    cbor_value_t *value = cbor_pointer_get(container, from);
    if (value) {
        cbor_value_t *copy = cbor_duplicate(value);
        if (cbor_pointer_add(container, path, copy)) {
            return copy;
        }
        cbor_destroy(copy);
    }
    return NULL;
}

bool cbor_pointer_test(cbor_value_t *container, const char *path, const cbor_value_t *value) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *var = cbor_pointer_get(container, path);
    return cbor_value_test(var, value);
}

int cbor_pointer_seti(cbor_value_t *container, const char *path, long long integer) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_integer(integer);
    if (cbor_pointer_add(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setb(cbor_value_t *container, const char *path, bool boolean) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_boolean(boolean);
    if (cbor_pointer_add(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_sets(cbor_value_t *container, const char *path, const char *str) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_string(str, -1);
    if (cbor_pointer_add(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setf(cbor_value_t *container, const char *path, double dbl) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_double(dbl);
    if (cbor_pointer_add(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setn(cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_null();
    if (cbor_pointer_add(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_seto(cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_map();
    if (cbor_pointer_add(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_seta(cbor_value_t *container, const char *path) {
    assert(path != NULL && path[0] == '/');
    cbor_value_t *val = cbor_init_array();
    if (cbor_pointer_add(container, path, val)) {
        return 0;
    }
    cbor_destroy(val);
    return -1;
}

int cbor_pointer_setv(cbor_value_t *container, const char *path, cbor_value_t *val) {
    assert(container->type == CBOR_TYPE_MAP || container->type == CBOR_TYPE_ARRAY);
    assert(val->parent == NULL);
    assert(path != NULL && path[0] == '/');
    if (cbor_pointer_add(container, path, val)) {
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
    return cbor_string(val);
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
