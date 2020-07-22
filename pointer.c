#include "cbor.h"
#include <string.h>

/* return: destination value */
cbor_value_t *cbor_pointer_get(cbor_value_t *container, const char *path) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    cbor_value_t *current = NULL;
    cbor_value_t *split = cbor_string_split(path, "/");

    cbor_iter_init(&iter, split, CBOR_ITER_AFTER);
    while ((ele = cbor_iter_next(&iter)) != NULL) {
        cbor_string_replace(ele, "~1", "/");
        cbor_string_replace(ele, "~0", "~");

        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele), cbor_string_size(ele));
                if (find) {
                    current = cbor_pair_value(find);
                    continue;
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    current = cbor_array_get(current, -1);
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0 && idx < cbor_container_size(current)) {
                        current = cbor_array_get(current, idx);
                        continue;
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

/* return: removed value */
cbor_value_t *cbor_pointer_remove(cbor_value_t *container, const char *path) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;
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
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele), cbor_string_size(ele));
                if (find) {
                    if (last) {
                        cbor_container_remove(current, find);
                        current = cbor_pair_remove_value(find);
                        cbor_destroy(find);
                    } else {
                        current = cbor_pair_value(find);
                    }
                    continue;
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    cbor_value_t *tmp = cbor_array_get(current, -1);
                    if (last) {
                        cbor_container_remove(current, tmp);
                    }
                    current = tmp;
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0 && idx < cbor_container_size(current)) {
                        if (last) {
                            cbor_value_t *tmp = current;
                            current = cbor_array_get(current, idx);
                            cbor_container_remove(tmp, current);
                        } else {
                            current = cbor_array_get(current, idx);
                        }
                        continue;
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

/* return: added value's parent container */
cbor_value_t *cbor_pointer_add(cbor_value_t *container, const char *path, cbor_value_t *value) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;
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
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele), cbor_string_size(ele));
                if (find) {
                    if (last) {
                        cbor_pair_set_value(find, value);
                    } else {
                        current = cbor_pair_value(find);
                    }
                    continue;
                } else if (last) {
                    cbor_map_set_value(current, cbor_string(ele), value);
                    continue;
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    if (last) {
                        cbor_container_insert_tail(current, value);
                    } else {
                        current = cbor_array_get(current, -1);
                    }
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0 && idx <= cbor_container_size(current)) {
                        if (last) {
                            cbor_array_set_value(current, idx, value);
                        } else {
                            current = cbor_array_get(current, idx);
                        }
                        continue;
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
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele), cbor_string_size(ele));
                if (find) {
                    if (last) {
                        cbor_pair_set_value(find, value);
                    } else {
                        current = cbor_pair_value(find);
                    }
                    continue;
                } else {
                    if (last) {
                        cbor_map_set_value(current, cbor_string(ele), value);
                        continue;
                    }
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    if (last) {
                        cbor_value_t *replace = cbor_array_get(current, -1);
                        cbor_container_remove(current, replace);
                        cbor_array_set_value(current, -1, value);
                        cbor_destroy(replace);
                    } else {
                        current = cbor_array_get(current, -1);
                    }
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (idx >= 0 && idx < cbor_container_size(current)) {
                        if (last) {
                            cbor_value_t *replace = cbor_array_get(current, idx);
                            cbor_container_remove(current, replace);
                            cbor_array_set_value(current, idx, value);
                            cbor_destroy(replace);
                        } else {
                            current = cbor_array_get(current, idx);
                        }
                        continue;
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
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele), cbor_string_size(ele));
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
                    if (last) {
                        value = cbor_array_get(current, -1);
                        root = current;
                    } else {
                        current = cbor_array_get(current, -1);
                    }
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0') {
                        if (last) {
                            value = cbor_array_get(current, idx);
                            root = current;
                        } else {
                            current = cbor_array_get(current, idx);
                        }
                        continue;
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
        if (cbor_string_size(ele) == 0 && cbor_container_prev(split, ele) == NULL) {
            current = container;
        } else {
            if (cbor_is_map(current)) {
                cbor_value_t *find = cbor_map_find(current, cbor_string(ele), cbor_string_size(ele));
                if (find) {
                    if (last) {
                        cbor_container_remove(root, value);
                        if (cbor_is_map(root)) {
                            cbor_value_t *tmp = cbor_pair_remove_value(value);
                            cbor_destroy(value);
                            value = tmp;
                        }
                        cbor_pair_set_value(find, value);
                    } else {
                        current = cbor_pair_value(find);

                        if (current == value) {
                            current = NULL;
                            break;
                        }
                    }
                    continue;
                } else {
                    if (last) {
                        cbor_container_remove(root, value);
                        if (cbor_is_map(root)) {
                            cbor_value_t *tmp = cbor_pair_remove_value(value);
                            cbor_destroy(value);
                            value = tmp;
                        }
                        cbor_map_set_value(current, cbor_string(ele), value);
                        continue;
                    }
                }
            } else if (cbor_is_array(current)) {
                if (!strcmp(cbor_string(ele), "-")) {
                    if (last) {
                        cbor_container_remove(root, value);
                        if (cbor_is_map(root)) {
                            cbor_value_t *tmp = cbor_pair_remove_value(value);
                            cbor_destroy(value);
                            value = tmp;
                        }
                        cbor_container_insert_tail(current, value);
                    } else {
                        current = cbor_array_get(current, -1);
                        if (current == value) {
                            current = NULL;
                            break;
                        }
                    }
                    continue;
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0' && idx >= 0 && idx <= cbor_container_size(current)) {
                        if (last) {
                            cbor_container_remove(root, value);
                            if (cbor_is_map(root)) {
                                cbor_value_t *tmp = cbor_pair_remove_value(value);
                                cbor_destroy(value);
                                value = tmp;
                            }
                            cbor_array_set_value(current, idx, value);
                        } else {
                            current = cbor_array_get(current, idx);
                            if (current == value) {
                                current = NULL;
                                break;
                            }
                        }
                        continue;
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

bool cbor_value_equal(const cbor_value_t *a, const cbor_value_t *b) {
    if (a == NULL || b == NULL) {
        return false;
    }

    if (cbor_is_string(a) && cbor_is_string(b)) {
        if (cbor_string_size(a) == cbor_string_size(b)) {
            return memcmp(cbor_string(a), cbor_string(b), cbor_string_size(a)) == 0;
        }
    } else if (cbor_is_number(a) && cbor_is_number(b)) {
        return cbor_real(a) == cbor_real(b);
    } else if (cbor_is_boolean(a) && cbor_is_boolean(b)) {
        return cbor_boolean(a) == cbor_boolean(b);
    } else if (cbor_is_null(a) && cbor_is_null(b)) {
        return true;
    } else if (cbor_is_array(a) && cbor_is_array(b)) {
        cbor_iter_t itera, iterb;
        cbor_value_t *elea, *eleb;

        cbor_iter_init(&itera, a, CBOR_ITER_AFTER);
        cbor_iter_init(&iterb, b, CBOR_ITER_AFTER);
        while ((elea = cbor_iter_next(&itera)) != NULL && ((eleb = cbor_iter_next(&iterb))) != NULL) {
            if (!cbor_value_equal(elea, eleb)) {
                return false;
            }
        }
        if (elea || eleb)
            return false;
        return true;
    } else if (cbor_is_map(a) && cbor_is_map(b)) {
        cbor_iter_t iter;
            cbor_value_t *ele;
        if (cbor_container_size(a) != cbor_container_size(b))
            return false;
        cbor_iter_init(&iter, a, CBOR_ITER_AFTER);
        while ((ele = cbor_iter_next(&iter)) != NULL) {
            cbor_value_t *key = cbor_pair_key(ele);
            cbor_value_t *pair = cbor_map_find(b, cbor_string(key), cbor_string_size(key));
            cbor_value_t *val = cbor_pair_value(pair);
            if (!cbor_value_equal(val, cbor_pair_value(ele))) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool cbor_pointer_test(cbor_value_t *container, const char *path, const cbor_value_t *value) {
    cbor_value_t *var = cbor_pointer_get(container, path);
    return cbor_value_equal(var, value);
}
