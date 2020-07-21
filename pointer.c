#include "cbor.h"
#include <string.h>

cbor_value_t *cbor_pointer_get(cbor_value_t *container, const char *str) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    cbor_value_t *current = NULL;
    cbor_value_t *split = cbor_string_split(str, "/");

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
                } else {
                    current = NULL;
                    break;
                }
            } else if (cbor_is_array(current)) {
                char *end;
                int idx = strtol(cbor_string(ele), &end, 10);
                if (*end == '\0') {
                    current = cbor_array_get(current, idx);
                } else {
                    current = NULL;
                    break;
                }
            } else {
                current = NULL;
                break;
            }
        }
    }
    cbor_destroy(split);
    return current;
}

cbor_value_t *cbor_pointer_remove(cbor_value_t *container, const char *str) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;
    cbor_value_t *split = cbor_string_split(str, "/");

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
                } else {
                    current = NULL;
                    break;
                }
            } else if (cbor_is_array(current)) {
                char *end;
                int idx = strtol(cbor_string(ele), &end, 10);
                if (*end == '\0') {
                    if (last) {
                        cbor_value_t *tmp = current;
                        current = cbor_array_get(current, idx);
                        cbor_container_remove(tmp, current);
                    } else {
                        current = cbor_array_get(current, idx);
                    }
                } else {
                    current = NULL;
                    break;
                }
            } else {
                current = NULL;
                break;
            }
        }
    }
    cbor_destroy(split);
    return current;
}

cbor_value_t *cbor_pointer_add(cbor_value_t *container, const char *str, cbor_value_t *value) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;
    cbor_value_t *split = cbor_string_split(str, "/");

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
                } else {
                    if (last) {
                        cbor_map_set_value(current, cbor_string(ele), value);
                    } else {
                        current = NULL;
                        break;
                    }
                }
            } else if (cbor_is_array(current)) {
                if (last && !strcmp(cbor_string(ele), "-")) {
                    cbor_container_insert_tail(current, value);
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0') {
                        if (last) {
                            if (idx > cbor_container_size(current)) {
                                current = NULL;
                                break;
                            }
                            cbor_array_set_value(current, idx, value);
                        } else {
                            current = cbor_array_get(current, idx);
                        }
                    }
                }
            } else {
                current = NULL;
                break;
            }
        }
    }
    cbor_destroy(split);
    return current;
}

cbor_value_t *cbor_pointer_replace(cbor_value_t *container, const char *str, cbor_value_t *value) {
    cbor_iter_t iter;
    cbor_value_t *ele;
    bool last = false;
    cbor_value_t *current = NULL;
    cbor_value_t *split = cbor_string_split(str, "/");

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
                    if (last) {
                        cbor_pair_set_value(find, value);
                    } else {
                        current = cbor_pair_value(find);
                    }
                } else {
                    if (last) {
                        cbor_map_set_value(current, cbor_string(ele), value);
                    } else {
                        current = NULL;
                    }
                }
            } else if (cbor_is_array(current)) {
                char *end;
                int idx = strtol(cbor_string(ele), &end, 10);
                if (last) {
                    cbor_value_t *replace = cbor_array_get(current, idx);
                    cbor_container_remove(current, replace);
                    cbor_array_set_value(current, idx, value);
                    cbor_destroy(replace);
                } else {
                    current = cbor_array_get(current, idx);
                }
            } else {
                current = NULL;
                break;
            }
        }
    }
    cbor_destroy(split);
    return NULL;
}

cbor_value_t *cbor_pointer_move(cbor_value_t *container, const char *dst, const char *src) {
    cbor_value_t *ele;
    cbor_iter_t iter;
    bool last = false;
    cbor_value_t *current = NULL;
    cbor_value_t *split = cbor_string_split(src, "/");

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
                } else {
                    current = NULL;
                    break;
                }
            } else if (cbor_is_array(current)) {
                char *end;
                int idx = strtol(cbor_string(ele), &end, 10);
                if (*end == '\0') {
                    if (last) {
                        value = cbor_array_get(current, idx);
                        root = current;
                    } else {
                        current = cbor_array_get(current, idx);
                    }
                } else {
                    current = NULL;
                    break;
                }
            } else {
                current = NULL;
                break;
            }
        }
    }

    if (current == NULL || root == NULL || value == NULL) {
        return NULL;
    }

    last = false;
    current = NULL;
    cbor_destroy(split);

    split = cbor_string_split(dst, "/");
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
                        if (root == current) {
                            current = NULL;
                            break;
                        }

                        cbor_container_remove(root, value);
                        if (cbor_is_map(root)) {
                            cbor_value_t *tmp = cbor_pair_remove_value(value);
                            cbor_destroy(value);
                            value = tmp;
                        }
                        cbor_pair_set_value(find, value);
                    } else {
                        current = cbor_pair_value(find);
                    }
                } else {
                    if (last) {
                        cbor_container_remove(root, value);
                        if (cbor_is_map(root)) {
                            cbor_value_t *tmp = cbor_pair_remove_value(value);
                            cbor_destroy(value);
                            value = tmp;
                        }
                        cbor_map_set_value(current, cbor_string(ele), value);
                    } else {
                        current = NULL;
                        break;
                    }
                }
            } else if (cbor_is_array(current)) {
                if (last && !strcmp(cbor_string(ele), "-")) {
                    if (root == current) {
                        current = NULL;
                        break;
                    }

                    cbor_container_remove(root, value);
                    if (cbor_is_map(root)) {
                        cbor_value_t *tmp = cbor_pair_remove_value(value);
                        cbor_destroy(value);
                        value = tmp;
                    }
                    cbor_container_insert_tail(current, value);
                } else {
                    char *end;
                    int idx = strtol(cbor_string(ele), &end, 10);
                    if (*end == '\0') {
                        if (last) {
                            if (idx > cbor_container_size(current) || idx < 0) {
                                current = NULL;
                                break;
                            }
                            if (current == root) {
                                current = NULL;
                                break;
                            }

                            cbor_container_remove(root, value);
                            if (cbor_is_map(root)) {
                                cbor_value_t *tmp = cbor_pair_remove_value(value);
                                cbor_destroy(value);
                                value = tmp;
                            }
                            cbor_array_set_value(current, idx, value);
                        } else {
                            current = cbor_array_get(current, idx);
                        }
                    } else {
                        current = NULL;
                        break;
                    }
                }
            } else {
                current = NULL;
                break;
            }
        }
    }
    cbor_destroy(split);
    return current;
}
