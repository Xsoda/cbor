#include "cbor.h"
#include "list.h"
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>

/* Major Types */
typedef enum {
    CBOR_TYPE_UINT = 0,
    CBOR_TYPE_NEGINT,
    CBOR_TYPE_BYTESTRING,
    CBOR_TYPE_STRING,
    CBOR_TYPE_ARRAY,
    CBOR_TYPE_MAP,
    CBOR_TYPE_TAG,
    CBOR_TYPE_SIMPLE,

    CBOR__TYPE_PAIR,
} cbor_type;

typedef enum {
    CBOR_SIMPLE_NONE = 0,
    CBOR_SIMPLE_FALSE = 20,
    CBOR_SIMPLE_TRUE = 21,
    CBOR_SIMPLE_NULL = 22,
    CBOR_SIMPLE_UNDEF = 23,
    CBOR_SIMPLE_EXTENSION = 24,
    CBOR_SIMPLE_REAL = 25,
} cbor_simple;

struct _cbor_value {
    cbor_type type;
    union {
        struct {
            size_t allocated;
            size_t length;
            char *ptr;
        } blob;
        struct {
            struct _cbor_value *key;
            struct _cbor_value *val;
        } pair;
        struct {
            unsigned long long item;
            struct _cbor_value *content;
        } tag;
        struct {
            double real;
            cbor_simple ctrl;
        } simple;
        unsigned long long uint;
        list_head(_cbor_cname, _cbor_value) container;
    };
    list_entry(_cbor_value) entry;
};

cbor_value_t *cbor_create(cbor_type type) {
    cbor_value_t *val = (cbor_value_t *)malloc(sizeof(cbor_value_t));
    memset(val, 0, sizeof(cbor_value_t));
    val->type = type;
    if (val->type == CBOR_TYPE_ARRAY || val->type == CBOR_TYPE_MAP) {
        list_init(&val->container);
    } else if (val->type == CBOR_TYPE_STRING || val->type == CBOR_TYPE_BYTESTRING) {
        val->blob.ptr = (char *)malloc(32);
        val->blob.allocated = 32;
    }
    return val;
}

int cbor_destroy(cbor_value_t *val) {
    if (val == NULL) {
        return -1;
    }

    if (val->type == CBOR_TYPE_ARRAY || val->type == CBOR_TYPE_MAP) {
        cbor_value_t *var, *tvar;
        list_foreach_safe(var, &val->container, entry, tvar) {
            list_remove(&val->container, var, entry);
            cbor_destroy(var);
        }
    } else if (val->type == CBOR_TYPE_BYTESTRING || val->type == CBOR_TYPE_STRING) {
        free(val->blob.ptr);
        val->blob.ptr = NULL;
    } else if (val->type == CBOR__TYPE_PAIR) {
        cbor_destroy(val->pair.key);
        val->pair.key = NULL;
        cbor_destroy(val->pair.val);
        val->pair.val = NULL;
    } else if (val->type == CBOR_TYPE_TAG) {
        cbor_destroy(val->tag.content);
        val->tag.content = NULL;
    }
    free(val);
    return 0;
}

static size_t cbor_blob_avalible(cbor_value_t *val, size_t size) {
    if (val && (val->type == CBOR_TYPE_BYTESTRING || val->type == CBOR_TYPE_STRING)) {
        if (val->blob.allocated - val->blob.length < size + 1) {
            char *tmp = (char *)realloc(val->blob.ptr, val->blob.allocated + size + 1);
            if (tmp) {
                val->blob.ptr = tmp;
                val->blob.allocated += size + 1;
            }
        }
    }
    return val->blob.allocated - val->blob.length;
}

int cbor_blob_append(cbor_value_t *val, const char *src, size_t length) {
    if (cbor_blob_avalible(val, length) > length) {
        memcpy(val->blob.ptr + val->blob.length, src, length);
        val->blob.length += length;
        val->blob.ptr[val->blob.length] = 0;
        return 0;
    }
    return -1;
}

int cbor_blob_append_v(cbor_value_t *val, const char *fmt, ...) {
    va_list ap, cpy;
    int length;
    char *str;
    va_start(ap, fmt);
    va_copy(cpy, ap);

    length = vsnprintf(NULL, 0, fmt, cpy);
    str = (char *)malloc(length + 1);

    if (str) {
        length = vsnprintf(str, length + 1, fmt, ap);
        length = cbor_blob_append(val, str, length);
        free(str);
    } else {
        length = -1;
    }

    va_end(ap);
    return length;
}

int cbor_blob_append_byte(cbor_value_t *val, uint8_t byte) {
    if (cbor_blob_avalible(val, 1) > 1) {
        int length = val->blob.length;
        val->blob.ptr[length] = byte;
        val->blob.length += 1;
        return 0;
    }
    return -1;
}

int cbor_blob_append_word(cbor_value_t *val, uint16_t word) {
    if (cbor_blob_avalible(val, 2) > 2) {
        *(uint16_t *)&val->blob.ptr[val->blob.length] = htobe16(word);
        val->blob.length += 2;
        return 0;
    }
    return -1;
}

int cbor_blob_append_dword(cbor_value_t *val, uint32_t dword) {
    if (cbor_blob_avalible(val, 4) > 4) {
        *(uint32_t *)&val->blob.ptr[val->blob.length] = htobe32(dword);
        val->blob.length += 4;
        return 0;
    }
    return -1;
}

int cbor_blob_append_qword(cbor_value_t *val, uint64_t qword) {
    if (cbor_blob_avalible(val, 8) > 8) {
        *(uint64_t *)&val->blob.ptr[val->blob.length] = htobe64(qword);
        val->blob.length += 8;
        return 0;
    }
    return -1;
}

int cbor_container_empty(const cbor_value_t *container) {
    if (container && (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP)) {
        return list_empty(&container->container);
    }
    return 0;
}

int cbor_container_size(const cbor_value_t *container) {
    cbor_value_t *var;
    int size = 0;
    if (container && (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP)) {
        list_foreach(var, &container->container, entry) {
            size++;
        }
    }
    return size;
}

int cbor_container_clear(cbor_value_t *container) {
    cbor_value_t *var, *tvar;
    if (!container) {
        return -1;
    }

    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        list_foreach_safe(var, &container->container, entry, tvar) {
            list_remove(&container->container, var, entry);
            cbor_destroy(var);
        }
    }
    return 0;
}

/* container: [A, B, C, D, E, F, G]
 * sub: []
 * split E
 *
 * ==>
 *
 * container: [F, G]
 * sub: [A, B, C, D, E]
 */
int cbor_container_split(cbor_value_t *container, cbor_value_t *val, cbor_value_t *sub) {
    bool found = false;
    cbor_value_t *var, *tvar;
    if (!container || !sub) {
        return -1;
    }

    if ((container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP)
        && container->type == sub->type) {
        list_foreach_safe(var, &container->container, entry, tvar) {
            if (var == val) {
                assert(found == false);
                found = true;
                continue;
            } else if (found) {
                list_remove(&container->container, var, entry);
                list_insert_tail(&sub->container, var, entry);
            }
        }
    }
    if (found) {
        cbor_container_swap(container, sub);
    }
    return 0;
}

int cbor_container_swap(cbor_value_t *ca, cbor_value_t *cb) {
    if (ca
        && cb
        && ca->type == cb->type
        && (ca->type == CBOR_TYPE_ARRAY || ca->type == CBOR_TYPE_MAP)) {
        list_swap(&ca->container, &cb->container, _cbor_value, entry);
    }
    return -1;
}

int cbor_container_insert_tail(cbor_value_t *container, cbor_value_t *val) {
    if (!container || !val) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);
        assert((container->type == CBOR_TYPE_MAP && val->type == CBOR__TYPE_PAIR) || container->type == CBOR_TYPE_ARRAY);
        list_insert_tail(&container->container, val, entry);
        return 0;
    }
    return -1;
}

int cbor_container_insert_head(cbor_value_t *container, cbor_value_t *val) {
    if (!container || !val) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);

        list_insert_head(&container->container, val, entry);
        return 0;
    }
    return -1;
}

int cbor_container_insert_after(cbor_value_t *container, cbor_value_t *elm, cbor_value_t *val) {
    if (!container || !val || !elm) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(elm->entry.le_next != NULL || elm->entry.le_prev != NULL);
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);

        list_insert_after(&container->container, elm, val, entry);
        return 0;
    }
    return -1;
}

int cbor_container_insert_before(cbor_value_t *container, cbor_value_t *elm, cbor_value_t *val) {
    if (!container || !val || !elm) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(elm->entry.le_next != NULL || elm->entry.le_prev != NULL);
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);

        list_insert_before(&container->container, elm, val, entry);
        return 0;
    }
    return -1;
}

cbor_value_t *cbor_container_first(const cbor_value_t *container) {
    if (container && (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP)) {
        return list_first(&container->container);
    }
    return NULL;
}

cbor_value_t *cbor_container_last(const cbor_value_t *container) {
    if (container && (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP)) {
        return list_last(&container->container, _cbor_cname);
    }
    return NULL;
}

cbor_value_t *cbor_container_next(const cbor_value_t *container, cbor_value_t *elm) {
    if (!container || !elm) {
        return NULL;
    }
    return list_next(elm, entry);
}

cbor_value_t *cbor_container_prev(const cbor_value_t *container, cbor_value_t *elm) {
    if (!container || !elm) {
        return NULL;
    }
    return list_prev(elm, _cbor_cname, entry);
}

cbor_value_t *cbor_container_remove(cbor_value_t *container, cbor_value_t *elm) {
    if (!container || !elm) {
        return NULL;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(elm->entry.le_next != NULL && elm->entry.le_prev != NULL);
        list_remove(&container->container, elm, entry);
        elm->entry.le_next = NULL;
        elm->entry.le_prev = NULL;
        return elm;
    }
    return NULL;
}

int cbor_container_concat(cbor_value_t *dst, cbor_value_t *src) {
    if (dst
        && src
        && dst->type == src->type
        && (dst->type == CBOR_TYPE_ARRAY
            || dst->type == CBOR_TYPE_MAP)) {
        list_concat(&dst->container, &src->container, entry);
        return 0;
    }
    return -1;
}

int cbor_container_slice(cbor_value_t *dst, cbor_value_t *src, cbor_value_t *elm) {
    if (dst
        && src
        && elm
        && dst->type == src->type
        && (dst->type == CBOR_TYPE_ARRAY
            || dst->type == CBOR_TYPE_MAP)) {
        assert(cbor_container_empty(dst));
        cbor_value_t *prev = list_prev(elm, _cbor_cname, entry);
        if (prev == NULL) {
            return cbor_container_swap(dst, src);
        } else {
            dst->container.lh_last = src->container.lh_last;
            dst->container.lh_first = elm;

            prev->entry.le_next = NULL;
            src->container.lh_last = &list_next(prev, entry);
            elm->entry.le_prev = &list_first(&dst->container);
            return 0;
        }
    }
    return -1;
}

int cbor_map_insert(cbor_value_t *map, cbor_value_t *key, cbor_value_t *val) {
    if (!map || !key || !val) {
        return -1;
    }
    if (map->type == CBOR_TYPE_MAP) {
        cbor_value_t *pair = cbor_create(CBOR__TYPE_PAIR);
        pair->pair.key = key;
        pair->pair.val = val;
        cbor_container_insert_tail(map, pair);
        return 0;
    }
    return -1;
}

int cbor_map_destroy(cbor_value_t *map, const char *key) {
    if (!map || !key) {
        return -1;
    }
    if (map->type == CBOR_TYPE_MAP) {
        cbor_value_t *var, *tvar;
        list_foreach_safe(var, &map->container, entry, tvar) {
            if (cbor_is_string(var->pair.key) && !strcmp(key, var->pair.key->blob.ptr)) {
                cbor_container_remove(map, var);
                cbor_destroy(var);
                return 0;
            }
        }
        return -1;
    }
    return -1;
}

cbor_value_t *cbor_loads(const char *src, size_t *length) {
    cbor_type type;
    char addition;
    cbor_value_t *val = NULL;
    size_t offset = 0;

    if (src == NULL || length == NULL || *length == 0) {
        return NULL;
    }

    type = (unsigned char)src[offset] >> 5;
    addition = (unsigned char)src[offset] & 0x1F;

    switch (type) {
    case CBOR_TYPE_UINT: {
        val = cbor_create(CBOR_TYPE_UINT);
        offset++;
        if (addition < 24) {
            val->uint = addition;
        } else if (addition == 24) { /* uint8_t */
            val->uint = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25) { /* uint16_t */
            val->uint = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26) { /* uint32_t */
            val->uint = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27) { /* uint64_t */
            val->uint = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        }
        break;
    }
    case CBOR_TYPE_NEGINT: {    /* NOTE: negint = -1 - uint */
        val = cbor_create(CBOR_TYPE_NEGINT);
        offset++;
        if (addition < 24) {
            val->uint = addition;
        } else if (addition == 24) { /* uint8_t */
            val->uint = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25) { /* uint16_t */
            val->uint = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26) { /* uint32_t */
            val->uint = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27) { /* uint64_t */
            val->uint = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        }
        break;
    }
    case CBOR_TYPE_BYTESTRING: {
        val = cbor_create(CBOR_TYPE_BYTESTRING);
        offset++;
        int len = 0;
        if (addition < 24) {
            len = addition;
        } else if (addition == 24) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27) { /* uint64_t */
            len = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        } else if (addition == 31) { /* indefinite */
            while (offset < *length) {
                if (src[offset] == -1) {
                    offset++;
                    break;
                }
                size_t remain = *length - offset;
                cbor_value_t *sub = cbor_loads(src + offset, &remain);
                if (sub && sub->type == CBOR_TYPE_BYTESTRING) {
                    cbor_blob_append(val, sub->blob.ptr, sub->blob.length);
                    val->blob.ptr[val->blob.length] = 0;
                    offset += remain;
                } else {
                    /* error */
                    cbor_destroy(val);
                    val = NULL;
                    offset = 0;
                    cbor_destroy(sub);
                    break;
                }
                if (sub) {
                    cbor_destroy(sub);
                }
            }
        }
        if (addition != 31 && offset + len <= *length) {
            cbor_blob_append(val, &src[offset], len);
            val->blob.ptr[val->blob.length] = 0;
            offset += len;
        }
        break;
    }
    case CBOR_TYPE_STRING: {
        val = cbor_create(CBOR_TYPE_STRING);
        offset++;
        int len = 0;
        if (addition < 24) {
            len = addition;
        } else if (addition == 24) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27) { /* uint64_t */
            len = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        } else if (addition == 31) { /* indefinite */
            while (offset < *length) {
                if (src[offset] == -1) {
                    offset++;
                    break;
                }
                size_t remain = *length - offset;
                cbor_value_t *sub = cbor_loads(src + offset, &remain);
                if (sub && sub->type == CBOR_TYPE_STRING) {
                    cbor_blob_append(val, sub->blob.ptr, sub->blob.length);
                    val->blob.ptr[val->blob.length] = 0;
                    offset += remain;
                } else {
                    /* error */
                    cbor_destroy(val);
                    val = NULL;
                    offset = 0;
                    cbor_destroy(sub);
                    break;
                }
                if (sub) {
                    cbor_destroy(sub);
                }
            }
        }
        if (addition != 31 && offset + len <= *length) {
            cbor_blob_append(val, &src[offset], len);
            val->blob.ptr[val->blob.length] = 0;
            offset += len;
        } else {
            cbor_destroy(val);
            val = NULL;
            offset = 0;
        }
        break;
    }
    case CBOR_TYPE_ARRAY: {
        val = cbor_create(CBOR_TYPE_ARRAY);
        offset++;
        int len = 0;
        if (addition < 24) {
            len = addition;
        } else if (addition == 24) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27) { /* uint64_t */
            len = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        } else if (addition == 31) { /* indefinite */
            while (offset < *length) {
                if (src[offset] == -1) {
                    offset++;
                    break;
                }
                size_t remain = *length - offset;
                cbor_value_t *elm = cbor_loads(src + offset, &remain);
                if (elm) {
                    offset += remain;
                    cbor_container_insert_tail(val, elm);
                } else {
                    /* error */
                    cbor_destroy(val);
                    val = NULL;
                    offset = 0;
                    break;
                }
            }
        }
        if (addition != 31) {
            while (len > 0 && offset < *length) {
                size_t remain = *length - offset;
                cbor_value_t *elm = cbor_loads(src + offset, &remain);
                if (elm) {
                    offset += remain;
                    cbor_container_insert_tail(val, elm);
                    len--;
                } else {
                    /* error */
                    cbor_destroy(val);
                    val = NULL;
                    offset = 0;
                    break;
                }
            }
            if (val && len > 0) {
                cbor_destroy(val);
                val = NULL;
                offset = 0;
            }
        }
        break;
    }
    case CBOR_TYPE_MAP: {
        val = cbor_create(CBOR_TYPE_MAP);
        offset++;
        int len = 0;
        if (addition < 24) {
            len = addition;
        } else if (addition == 24) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27) { /* uint64_t */
            len = be64toh(*(uint32_t *)&src[offset]);
        } else if (addition == 31) { /* indefinite */
            while (offset < *length) {
                if (src[offset] == -1) {
                    offset++;
                    break;
                }
                size_t remain = *length - offset;
                cbor_value_t *k = cbor_loads(src + offset, &remain);
                if (k) {
                    offset += remain;
                    remain = *length - offset;
                    cbor_value_t *v = cbor_loads(src + offset, &remain);
                    if (v && offset < *length) {
                        offset += remain;
                        cbor_value_t *pair = cbor_create(CBOR__TYPE_PAIR);
                        pair->pair.key = k;
                        pair->pair.val = v;
                        cbor_container_insert_tail(val, pair);
                    } else {
                        /* error */
                        cbor_destroy(k);
                        cbor_destroy(val);
                        val = NULL;
                        offset = 0;
                        break;
                    }
                } else {
                    /* error */
                    cbor_destroy(val);
                    val = NULL;
                    offset = 0;
                    break;
                }
            }
        }
        if (addition != 31) {
            while (len > 0 && offset < *length) {
                size_t remain = *length - offset;
                cbor_value_t *k = cbor_loads(src + offset, &remain);
                if (k) {
                    offset += remain;
                    remain = *length - offset;
                    cbor_value_t *v = cbor_loads(src + offset, &remain);
                    if (v && remain < *length) {
                        offset += remain;
                        cbor_value_t *pair = cbor_create(CBOR__TYPE_PAIR);
                        pair->pair.key = k;
                        pair->pair.val = v;
                        cbor_container_insert_tail(val, pair);
                        len--;
                    } else {
                        /* error */
                        cbor_destroy(val);
                        cbor_destroy(k);
                        offset = 0;
                        val = NULL;
                        break;
                    }
                } else {
                    /* error */
                    cbor_destroy(val);
                    val = NULL;
                    offset = 0;
                    break;
                }
            }
            if (val && len > 0) {
                cbor_destroy(val);
                val = NULL;
                offset = 0;
            }
        }
        break;
    }
    case CBOR_TYPE_TAG: {
        val = cbor_create(CBOR_TYPE_TAG);
        offset++;
        if (addition < 24) {
            val->tag.item = addition;
        } else if (addition == 24) { /* uint8_t */
            val->tag.item =(unsigned char)src[offset];
            offset++;
        } else if (addition == 25) { /* uint16_t */
            val->tag.item = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26) {
            val->tag.item = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27) {
            val->tag.item = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        }
        size_t remain = *length - offset;
        val->tag.content = cbor_loads(src + offset, &remain);
        if (val->tag.content) {
            offset += remain;
        } else {
            /* error */
            cbor_destroy(val);
            val = NULL;
            offset = 0;
        }
        break;
    }
    case CBOR_TYPE_SIMPLE: {
        val = cbor_create(CBOR_TYPE_SIMPLE);
        offset++;
        if (addition < 20) {
            val->simple.ctrl = addition;
            /* assert(("simple value is unassigned", 0)); */
        } else if (addition == 20) {   /* False */
            val->simple.ctrl = CBOR_SIMPLE_FALSE;
        } else if (addition == 21) { /* True */
            val->simple.ctrl = CBOR_SIMPLE_TRUE;
        } else if (addition == 22) { /* Null */
            val->simple.ctrl = CBOR_SIMPLE_NONE;
        } else if (addition == 23) { /* Undefined value */
            val->simple.ctrl = CBOR_SIMPLE_UNDEF;
        } else if (addition == 24) { /* Simple value: extension */
            /* Reserved */
            val->simple.ctrl = src[offset];
            offset++;
        } else if (addition == 25) { /* half float */
            union {
                uint64_t u64;
                double dbl;
            } f64_val;
            val->simple.ctrl = CBOR_SIMPLE_REAL;
            uint16_t u16 = be16toh(*(uint16_t *)&src[offset]);
            int sign = (u16 & 0x8000) >> 15;
            int exp = (u16 >> 10) & 0x1F;
            uint64_t frac = u16 & 0x3FF;

            f64_val.u64 = frac << (52 - 10);
            if (sign) {
                f64_val.u64 |= (uint64_t)1 << 63;
            }

            if (exp == 0) {
                /* do nothing */
            } else if (exp == 31) {
                f64_val.u64 |= (uint64_t)0x7FF << 52;
            } else {
                f64_val.u64 |= (uint64_t)(exp - 15 + 1023) << 52;
            }
            val->simple.real = f64_val.dbl;
            offset += 2;
        } else if (addition == 26) { /* float */
            union {
                uint64_t u64;
                double dbl;
            } f64_val;
            val->simple.ctrl = CBOR_SIMPLE_REAL;
            uint32_t u32 = be32toh(*(uint32_t *)&src[offset]);
            int sign = u32 >> 31;
            int exp = (u32 >> 23) & 0xFF;
            uint64_t frac = u32 & 0x7FFFFF;

            f64_val.u64 = frac << (52 - 23);
            if (sign) {
                f64_val.u64 |= (uint64_t)1 << 63;
            }
            if (exp == 0) {
                /* do nothing */
            } else if (exp == 255) {
                f64_val.u64 |= (uint64_t)0x7FF << 52;
            } else {
                f64_val.u64 |= (uint64_t)(exp - 127 + 1023) << 52;
            }
            val->simple.real = f64_val.dbl;
            offset += 4;
        } else if (addition == 27) { /* double */
            union {
                uint64_t u64;
                double dbl;
            } var;
            val->simple.ctrl = CBOR_SIMPLE_REAL;
            var.u64 = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
            val->simple.real = var.dbl;
        } else {
            assert(("simple value is unassigned", 0));
        }
        break;
    }
    default:
        assert(0);
    }
    *length = offset;
    return val;
}

/* IEEE 754
 *
 *             sign | exponent | fraction
 * -----------+-----+----------+---------
 * half-float |  1  |   5      |   10
 * -----------+-----+----------+---------
 *      float |  1  |   8      |   23
 * -----------+-----+----------+---------
 *     double |  1  |   11     |   52
 *
 */
int cbor__dumps(const cbor_value_t *src, cbor_value_t *dst) {
    uint8_t type = src->type;
    type <<= 5;
    switch (src->type) {
    case CBOR_TYPE_UINT:
    case CBOR_TYPE_NEGINT:  {
        if (src->uint < 24) {
            type |= (uint8_t)src->uint;
            cbor_blob_append_byte(dst, type);
        } else if (src->uint <= UINT8_MAX) {
            type |= 24;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_byte(dst, src->uint);
        } else if (src->uint <= UINT16_MAX) {
            type |= 25;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_word(dst, src->uint);
        } else if (src->uint <= UINT32_MAX) {
            type |= 26;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_dword(dst, src->uint);
        } else if (src->uint <= UINT64_MAX) {
            type |= 27;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_qword(dst, src->uint);
        }
        break;
    }
    case CBOR_TYPE_BYTESTRING:
    case CBOR_TYPE_STRING: {
        if (src->blob.length < 24) {
            type |= (uint8_t)src->blob.length;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append(dst, src->blob.ptr, src->blob.length);
        } else if (src->blob.length <= UINT8_MAX) {
            type |= 24;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_byte(dst, src->blob.length);
            cbor_blob_append(dst, src->blob.ptr, src->blob.length);
        } else if (src->blob.length <= UINT16_MAX) {
            type |= 25;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_word(dst, src->blob.length);
            cbor_blob_append(dst, src->blob.ptr, src->blob.length);
        } else if (src->blob.length <= UINT32_MAX) {
            type |= 26;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_dword(dst, src->blob.length);
            cbor_blob_append(dst, src->blob.ptr, src->blob.length);
        } else if (src->blob.length <= UINT64_MAX) {
            type |= 27;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_qword(dst, src->blob.length);
            cbor_blob_append(dst, src->blob.ptr, src->blob.length);
        }
        break;
    }
    case CBOR_TYPE_ARRAY: {
        cbor_value_t *var;
        unsigned long long size = cbor_container_size(src);
        if (size < 24) {
            type |= (uint8_t)size;
            cbor_blob_append_byte(dst, type);
        } else if (size <= UINT8_MAX) {
            type |= 24;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_byte(dst, size);
        } else if (size <= UINT16_MAX) {
            type |= 25;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_word(dst, size);
        } else if (size <= UINT32_MAX) {
            type |= 26;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_dword(dst, size);
        } else if (size <= UINT64_MAX) {
            type |= 27;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_qword(dst, size);
        }
        for (var = cbor_container_first(src);
             var != NULL;
             var = cbor_container_next(src, var)) {
            cbor__dumps(var, dst);
        }
        break;
    }
    case CBOR_TYPE_MAP: {
        cbor_value_t *var;
        unsigned long long size = cbor_container_size(src);
        if (size < 24) {
            type |= (uint8_t)size;
            cbor_blob_append_byte(dst, type);
        } else if (size <= UINT8_MAX) {
            type |= 24;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_byte(dst, size);
        } else if (size <= UINT16_MAX) {
            type |= 25;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_word(dst, size);
        } else if (size <= UINT32_MAX) {
            type |= 26;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_dword(dst, size);
        } else if (size <= UINT64_MAX) {
            type |= 27;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_qword(dst, size);
        }
        for (var = cbor_container_first(src);
             var != NULL;
             var = cbor_container_next(src, var)) {
            cbor__dumps(var->pair.key, dst);
            cbor__dumps(var->pair.val, dst);
        }
        break;
    }
    case CBOR_TYPE_TAG: {
        if (src->tag.item < 24) {
            type |= (uint8_t)src->tag.item;
            cbor_blob_append_byte(dst, type);
        } else if (src->tag.item <= UINT8_MAX) {
            type |= 24;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_byte(dst, src->tag.item);
        } else if (src->tag.item <= UINT16_MAX) {
            type |= 25;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_word(dst, src->tag.item);
        } else if (src->tag.item <= UINT32_MAX) {
            type |= 26;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_dword(dst, src->tag.item);
        } else if (src->tag.item <= UINT64_MAX) {
            type |= 27;
            cbor_blob_append_byte(dst, type);
            cbor_blob_append_qword(dst, src->tag.item);
        }
        cbor__dumps(src->tag.content, dst);
        break;
    }
    case CBOR_TYPE_SIMPLE: {
        uint8_t type = src->type;
        type <<= 5;
        if (src->simple.ctrl == CBOR_SIMPLE_FALSE) {
            type |= 20;
            cbor_blob_append_byte(dst, type);
        } else if (src->simple.ctrl == CBOR_SIMPLE_TRUE) {
            type |= 21;
            cbor_blob_append_byte(dst, type);
        } else if (src->simple.ctrl == CBOR_SIMPLE_NONE) {
            type |= 22;
            cbor_blob_append_byte(dst, type);
        } else if (src->simple.ctrl == CBOR_SIMPLE_UNDEF) {
            type |= 23;
            cbor_blob_append_byte(dst, type);
        } else if (src->simple.ctrl == CBOR_SIMPLE_REAL) {
            union {
                uint64_t u64;
                double dbl;
            } f64_val;
            f64_val.dbl = src->simple.real;
            int exponent = (f64_val.u64 >> 52) & 0x7FF;
            int sign = f64_val.u64 >> 63;
            uint64_t frac = f64_val.u64 & 0xFFFFFFFFFFFFF;
            int frac_bitcnt;
            if (frac) {
                for (frac_bitcnt = 0; frac_bitcnt < 52; frac_bitcnt++) {
                    if (frac & 1) {
                        break;
                    }
                    frac >>= 1;
                }
            } else {
                frac_bitcnt = 52;
            }

            frac_bitcnt = 52 - frac_bitcnt;
            frac = f64_val.u64 & 0xFFFFFFFFFFFFF;

            if (exponent == 0 || exponent == 0x7FF) { /* subnormal double, infinity, nan */
                if (frac_bitcnt <= 10) {
                    type |= 25;
                    cbor_blob_append_byte(dst, type);

                    uint16_t u16 = frac >> (52 - 10);
                    if (sign) {
                        u16 |= (1 << 15);
                    }
                    if (exponent) {
                        u16 |= 0x1F << 10;
                    }
                    cbor_blob_append_word(dst, u16);
                } else if (frac_bitcnt <= 23) {
                    type |= 26;
                    cbor_blob_append_byte(dst, type);

                    uint32_t u32 = frac >> (52 - 23);
                    if (sign) {
                        u32 |= (1 << 31);
                    }
                    if (exponent) {
                        u32 |= 0x3F << 23;
                    }
                    cbor_blob_append_dword(dst, u32);
                } else {
                    type |= 27;
                    cbor_blob_append_byte(dst, type);

                    uint64_t u64 = frac;
                    if (sign) {
                        u64 |= (uint64_t)1 << 63;
                    }
                    if (exponent) {
                        u64 |= (uint64_t)0x7FF << 52;
                    }
                    cbor_blob_append_qword(dst, u64);
                }
            } else {
                exponent -= 1023;
                if (exponent >= -14
                    && exponent <= 15
                    && frac_bitcnt <= 10) {
                    type |= 25;
                    cbor_blob_append_byte(dst, type);

                    uint16_t u16 = frac >> (52 - 10);
                    if (sign) {
                        u16 |= (1 << 15);
                    }
                    u16 |= (exponent + 15) << 10;
                    cbor_blob_append_word(dst, u16);
                } else if (exponent >= -126
                           && exponent <= 127
                           && frac_bitcnt <= 23) {
                    type |= 26;
                    cbor_blob_append_byte(dst, type);

                    uint32_t u32 = frac >> (52 - 23);
                    if (sign) {
                        u32 |= (1 << 31);
                    }
                    u32 |= (exponent + 127) << 23;
                    cbor_blob_append_dword(dst, u32);
                } else {
                    type |= 27;
                    cbor_blob_append_byte(dst, type);

                    uint64_t u64 = frac;
                    if (sign) {
                        u64 |= (uint64_t)1 << 63;
                    }
                    u64 |= (uint64_t)(exponent + 1023) << 52;
                    cbor_blob_append_qword(dst, u64);
                }
            }
        } else {
            if (src->simple.ctrl < 20) {
                type |= (unsigned char)src->simple.ctrl;
                cbor_blob_append_byte(dst, type);
            } else {
                type |= 24;
                cbor_blob_append_byte(dst, type);
                cbor_blob_append_byte(dst, (unsigned char)src->simple.ctrl);
            }
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

char *cbor_dumps(const cbor_value_t *src, size_t *length) {
    cbor_value_t *dst;
    char *ptr;

    if (!src || !length) {
        return NULL;
    }
    dst = cbor_create(CBOR_TYPE_BYTESTRING);
    cbor__dumps(src, dst);
    *length = dst->blob.length;
    ptr = dst->blob.ptr;
    free(dst);
    return ptr;
}

long long cbor_integer(const cbor_value_t *val) {
    if (val == NULL) {
        return 0;
    }
    if (val->type == CBOR_TYPE_UINT) {
        return val->uint;
    }
    if (val->type == CBOR_TYPE_NEGINT) {
        return -1 - val->uint;
    }
    if (val->type == CBOR_TYPE_SIMPLE && val->simple.ctrl == CBOR_SIMPLE_REAL) {
        return val->simple.real;
    }
    return 0;
}

double cbor_real(const cbor_value_t *val) {
    if (val == NULL) {
        return .0f;
    }
    if (val->type == CBOR_TYPE_UINT) {
        return val->uint;
    }
    if (val->type == CBOR_TYPE_NEGINT) {
        return -1 - val->uint;
    }
    if (val->type == CBOR_TYPE_SIMPLE && val->simple.ctrl == CBOR_SIMPLE_REAL) {
        return val->simple.real;
    }
    return .0f;
}

unsigned long long cbor_raw_uint(const cbor_value_t *val) {
    if (val && (val->type == CBOR_TYPE_UINT || val->type == CBOR_TYPE_NEGINT)) {
        return val->uint;
    }
    return 0;
}

int cbor_string_size(const cbor_value_t *val) {
    if (cbor_is_string(val) || cbor_is_bytestring(val)) {
        return val->blob.length;
    }
    return 0;
}

const char *cbor_string(const cbor_value_t *val) {
    if (cbor_is_string(val) || cbor_is_bytestring(val)) {
        return val->blob.ptr;
    }
    return 0;
}

bool cbor_boolean(const cbor_value_t *val) {
    if (val == NULL) {
        return false;
    }
    if (val->type == CBOR_TYPE_SIMPLE) {
        if (val->simple.ctrl == CBOR_SIMPLE_FALSE) {
            return false;
        } else if (val->simple.ctrl == CBOR_SIMPLE_TRUE) {
            return true;
        }
    }
    return false;
}

long cbor_tag_get_item(cbor_value_t *val) {
    if (val && val->type == CBOR_TYPE_TAG) {
        return val->tag.item;
    }
    return 0;
}

cbor_value_t *cbor_tag_get_content(cbor_value_t *val) {
    if (val && val->type == CBOR_TYPE_TAG) {
        return val->tag.content;
    }
    return NULL;
}

int cbor_tag_set(cbor_value_t *val, long item, cbor_value_t *content) {
    if (val && val->type == CBOR_TYPE_TAG) {
        if (val->tag.content) {
            cbor_destroy(val->tag.content);
        }
        val->tag.item = item;
        val->tag.content = content;
    }
    return 0;
}

bool cbor_is_boolean(const cbor_value_t *val) {
    if (val && val->type == CBOR_TYPE_SIMPLE) {
        return val->simple.ctrl == CBOR_SIMPLE_TRUE || val->simple.ctrl == CBOR_SIMPLE_FALSE;
    }
    return false;
}

bool cbor_is_integer(const cbor_value_t *val) {
    return val && (val->type == CBOR_TYPE_UINT || val->type == CBOR_TYPE_NEGINT);
}

bool cbor_is_double(const cbor_value_t *val) {
    if (val && val->type == CBOR_TYPE_SIMPLE) {
        return val->simple.ctrl == CBOR_SIMPLE_REAL;
    }
    return false;
}

bool cbor_is_null(const cbor_value_t *val) {
    if (val && val->type == CBOR_TYPE_SIMPLE) {
        return val->simple.ctrl == CBOR_SIMPLE_NULL;
    }
    return false;
}

bool cbor_is_bytestring(const cbor_value_t *val) {
    return val && val->type == CBOR_TYPE_BYTESTRING;
}

bool cbor_is_string(const cbor_value_t *val) {
    return val && val->type == CBOR_TYPE_STRING;
}

bool cbor_is_map(const cbor_value_t *val) {
    return val && val->type == CBOR_TYPE_MAP;
}

bool cbor_is_array(const cbor_value_t *val) {
    return val && val->type == CBOR_TYPE_ARRAY;
}

bool cbor_is_tag(const cbor_value_t *val) {
    return val && val->type == CBOR_TYPE_TAG;
}

bool cbor_is_number(const cbor_value_t *val) {
    return cbor_is_integer(val) || cbor_is_double(val);
}

cbor_type cbor_get_type(cbor_value_t *val) {
    return val->type;
}

cbor_value_t *cbor_init_boolean(bool b) {
    cbor_value_t *val = cbor_create(CBOR_TYPE_SIMPLE);
    if (b) {
        val->simple.ctrl = CBOR_SIMPLE_TRUE;
    } else {
        val->simple.ctrl = CBOR_SIMPLE_FALSE;
    }
    return val;
}

cbor_value_t *cbor_init_null() {
    cbor_value_t *val = cbor_create(CBOR_TYPE_SIMPLE);
    val->simple.ctrl = CBOR_SIMPLE_NULL;
    return val;
}

cbor_value_t *cbor_init_map() {
    return cbor_create(CBOR_TYPE_MAP);
}

cbor_value_t *cbor_init_array() {
    return cbor_create(CBOR_TYPE_ARRAY);
}

cbor_value_t *cbor_init_integer(long long l) {
    cbor_value_t *val = cbor_create(CBOR_TYPE_UINT);
    if (l < 0) {
        val->type = CBOR_TYPE_NEGINT;
        val->uint = -l -1;
    } else {
        val->uint = l;
    }
    return val;
}

cbor_value_t *cbor_init_string(const char *str, int len) {
    cbor_value_t *val = cbor_create(CBOR_TYPE_STRING);
    if (len < 0) {
        len = strlen(str);
    }
    cbor_blob_append(val, str, len);
    return val;
}

cbor_value_t *cbor_init_bytestring(const char *str, int len) {
    cbor_value_t *val = cbor_create(CBOR_TYPE_BYTESTRING);
    cbor_blob_append(val, str, len);
    return val;
}

cbor_value_t *cbor_init_double(double d) {
    cbor_value_t *val = cbor_create(CBOR_TYPE_SIMPLE);
    val->simple.ctrl = CBOR_SIMPLE_REAL;
    val->simple.real = d;
    return val;
}

void cbor_iter_init(cbor_iter_t *iter, const cbor_value_t *container, cbor_iter_dir dir) {
    iter->container = container;
    iter->dir = dir;
    if (dir == CBOR_ITER_AFTER) {
        iter->next = cbor_container_first(container);
    } else {
        iter->next = cbor_container_last(container);
    }
}

cbor_value_t *cbor_iter_next(cbor_iter_t *iter) {
    cbor_value_t *current = iter->next;
    if (iter->dir == CBOR_ITER_AFTER) {
        iter->next = cbor_container_next(iter->container, current);
    } else {
        iter->next = cbor_container_prev(iter->container, current);
    }
    return current;
}

cbor_value_t *cbor_map_find(const cbor_value_t *map, const char *key, size_t len) {
    cbor_value_t *var;
    if (map == NULL || map->type != CBOR_TYPE_MAP || key == NULL) {
        return NULL;
    }
    list_foreach(var, &map->container, entry) {
        if (var->pair.key->type == CBOR_TYPE_STRING) {
            if (var->pair.key->blob.length == len
                && !strncmp(var->pair.key->blob.ptr, key, len)) {
                break;
            }
        }
    }
    return var;
}

int cbor_map_set(cbor_value_t *map, const char *key, cbor_value_t *val) {
    size_t len;
    cbor_value_t *find;
    if (map == NULL || map->type != CBOR_TYPE_MAP || val == NULL || key == NULL) {
        return -1;
    }
    len = strlen(key);
    find = cbor_map_find(map, key, len);
    if (find == NULL) {
        cbor_value_t *pair = cbor_create(CBOR__TYPE_PAIR);
        pair->pair.key = cbor_create(CBOR_TYPE_STRING);
        cbor_blob_append(pair->pair.key, key, strlen(key));
        pair->pair.val = val;
        cbor_container_insert_tail(map, pair);
    } else {
        cbor_destroy(find->pair.val);
        find->pair.val = val;
    }
    return 0;
}

int cbor_map_set_integer(cbor_value_t *map, const char *key, long long integer) {
    size_t len;
    cbor_value_t *find;
    if (map == NULL || map->type != CBOR_TYPE_MAP || key == NULL) {
        return -1;
    }
    len = strlen(key);
    find = cbor_map_find(map, key, len);
    if (find == NULL) {
        find = cbor_create(CBOR__TYPE_PAIR);
        find->pair.key = cbor_init_string(key, len);
        find->pair.val = cbor_init_integer(integer);
        cbor_container_insert_tail(map, find);
    } else {
        cbor_destroy(find->pair.val);
        find->pair.val = cbor_init_integer(integer);
    }
    return 0;
}

int cbor_map_set_double(cbor_value_t *map, const char *key, double dbl) {
    size_t len;
    cbor_value_t *find;
    if (map == NULL || map->type != CBOR_TYPE_MAP || key == NULL) {
        return -1;
    }
    len = strlen(key);
    find = cbor_map_find(map, key, len);
    if (find == NULL) {
        find = cbor_create(CBOR__TYPE_PAIR);
        find->pair.key = cbor_init_string(key, len);
        find->pair.val = cbor_init_double(dbl);
        cbor_container_insert_tail(map, find);
    } else {
        cbor_destroy(find->pair.val);
        find->pair.val = cbor_init_double(dbl);
    }
    return 0;
}

int cbor_map_set_boolean(cbor_value_t *map, const char *key, bool boolean) {
    size_t len;
    cbor_value_t *find;
    if (map == NULL || map->type != CBOR_TYPE_MAP || key == NULL) {
        return -1;
    }
    len = strlen(key);
    find = cbor_map_find(map, key, len);
    if (find == NULL) {
        find = cbor_create(CBOR__TYPE_PAIR);
        find->pair.key = cbor_init_string(key, len);
        find->pair.val = cbor_init_boolean(boolean);
        cbor_container_insert_tail(map, find);
    } else {
        cbor_destroy(find->pair.val);
        find->pair.val = cbor_init_boolean(boolean);
    }
    return 0;
}
int cbor_map_set_string(cbor_value_t *map, const char *key, const char *str) {
    size_t len;
    cbor_value_t *find;
    if (map == NULL || map->type != CBOR_TYPE_MAP || key == NULL) {
        return -1;
    }
    if (str == NULL) {
        str = "";
    }

    len = strlen(key);
    find = cbor_map_find(map, key, len);
    if (find == NULL) {
        find = cbor_create(CBOR__TYPE_PAIR);
        find->pair.key = cbor_init_string(key, len);
        find->pair.val = cbor_init_string(str, strlen(str));
        cbor_container_insert_tail(map, find);
    } else {
        cbor_destroy(find->pair.val);
        find->pair.val = cbor_init_string(str, strlen(str));
    }
    return 0;
}

int cbor_map_set_null(cbor_value_t *map, const char *key) {
    size_t len;
    cbor_value_t *find;
    if (map == NULL || map->type != CBOR_TYPE_MAP || key == NULL) {
        return -1;
    }
    len = strlen(key);
    find = cbor_map_find(map, key, len);
    if (find == NULL) {
        find = cbor_create(CBOR__TYPE_PAIR);
        find->pair.key = cbor_init_string(key, len);
        find->pair.val = cbor_init_null();
        cbor_container_insert_tail(map, find);
    } else {
        cbor_destroy(find->pair.val);
        find->pair.val = cbor_init_null();
    }
    return 0;
}

int cbor_map_set_value(cbor_value_t *map, const char *key, cbor_value_t *value) {
    size_t len;
    cbor_value_t *find;
    if (map == NULL || map->type != CBOR_TYPE_MAP || value == NULL) {
        return -1;
    }

    if (value->entry.le_prev || value->entry.le_next) {
        return -1;
    }

    len = strlen(key);
    find = cbor_map_find(map, key, len);
    if (find == NULL) {
        find = cbor_create(CBOR__TYPE_PAIR);
        find->pair.key = cbor_init_string(key, len);
        find->pair.val = value;
        cbor_container_insert_tail(map, find);
    } else {
        cbor_destroy(find->pair.val);
        find->pair.val = value;
    }
    return 0;
}

cbor_value_t *cbor_map_dotget(const cbor_value_t *map, const char *key) {
    size_t len;
    const char *ch;
    cbor_value_t *val = NULL;

    while (map != NULL && map->type == CBOR_TYPE_MAP && key != NULL) {
        ch = strchr(key, '.');
        if (ch) {
            len = ch - key;
            ch += 1;
        } else {
            len = strlen(key);
        }
        map = cbor_map_find(map, key, len);
        key = ch;
        val = map ? map->pair.val : NULL;
        map = val;
    }
    return val;
}

cbor_value_t *cbor_map_remove(cbor_value_t *map, const char *key) {
    size_t len;
    const char *ch;
    cbor_value_t *val = map;

    while (map != NULL && map->type == CBOR_TYPE_MAP && key != NULL) {
        ch = strchr(key, '.');
        if (ch) {
            len = ch - key;
            ch += 1;
        } else {
            len = strlen(key);
        }
        map = cbor_map_find(map, key, len);
        key = ch;
        map = key ? map->pair.val : map;
    }

    if (map && map->type == CBOR__TYPE_PAIR) {
        cbor_container_remove(val, map);
        val = map->pair.val;
        map->pair.val = NULL;
        cbor_destroy(map);
    } else {
        val = NULL;
    }
    return val;
}

cbor_value_t *cbor_map_unlink(cbor_value_t *map, const char *key) {
    size_t len;
    const char *ch;
    cbor_value_t *val = map;

    while (map != NULL && map->type == CBOR_TYPE_MAP && key != NULL) {
        ch = strchr(key, '.');
        if (ch) {
            len = ch - key;
            ch += 1;
        } else {
            len = strlen(key);
        }
        map = cbor_map_find(map, key, len);
        key = ch;
        map = key ? map->pair.val : map;
    }

    if (map && map->type == CBOR__TYPE_PAIR) {
        cbor_container_remove(val, map);
        val = map;
    } else {
        val = NULL;
    }
    return val;
}

const char *cbor_map_dotget_string(const cbor_value_t *map, const char *key) {
    cbor_value_t *val = cbor_map_dotget(map, key);
    return cbor_string(val);
}

long long cbor_map_dotget_integer(const cbor_value_t *map, const char *key) {
    cbor_value_t *val = cbor_map_dotget(map, key);
    return cbor_integer(val);
}

bool cbor_map_dotget_boolean(const cbor_value_t *map, const char *key) {
    cbor_value_t *val = cbor_map_dotget(map, key);
    return cbor_boolean(val);
}

double cbor_map_dotget_double(const cbor_value_t *map, const char *key) {
    cbor_value_t *val = cbor_map_dotget(map, key);
    return cbor_real(val);
}

cbor_value_t *cbor_array_get(const cbor_value_t *array, int idx) {
    cbor_value_t *var = NULL;
    if (array == NULL || array->type != CBOR_TYPE_ARRAY) {
        return NULL;
    }

    if (idx >= 0) {
        for (var = cbor_container_first(array); idx > 0 && var != NULL; idx--)
            var = cbor_container_next(array, var);
    } else {
        for (var = cbor_container_last(array); idx < -1 && var != NULL; idx++)
            var = cbor_container_prev(array, var);
    }
    return var;
}

const char *cbor_array_get_string(const cbor_value_t *array, int idx) {
    cbor_value_t *val = cbor_array_get(array, idx);
    return cbor_string(val);
}

long long cbor_array_get_integer(const cbor_value_t *array, int idx) {
    cbor_value_t *val = cbor_array_get(array, idx);
    return cbor_integer(val);
}
double cbor_array_get_double(const cbor_value_t *array, int idx) {
    cbor_value_t *val = cbor_array_get(array, idx);
    return cbor_real(val);
}

bool cbor_array_get_boolean(const cbor_value_t *array, int idx) {
    cbor_value_t *val = cbor_array_get(array, idx);
    return cbor_boolean(val);
}

cbor_value_t *cbor_duplicate(cbor_value_t *val) {
    cbor_value_t *dup;
    if (val == NULL) {
        return NULL;
    }
    switch (val->type) {
    case CBOR_TYPE_UINT:
    case CBOR_TYPE_NEGINT: {
        dup = cbor_create(val->type);
        dup->uint = val->uint;
        break;
    }
    case CBOR_TYPE_BYTESTRING:
    case CBOR_TYPE_STRING: {
        dup = cbor_create(val->type);
        cbor_blob_append(dup, val->blob.ptr, val->blob.length);
        break;
    }
    case CBOR_TYPE_ARRAY:
    case CBOR_TYPE_MAP: {
        cbor_value_t *elm;
        dup = cbor_create(val->type);
        for (elm = cbor_container_first(val);
             elm != NULL;
             elm = cbor_container_next(val, elm)) {
            cbor_container_insert_tail(dup, cbor_duplicate(elm));
        }
        break;
    }
    case CBOR_TYPE_TAG: {
        dup = cbor_create(val->type);
        dup->tag.item = val->tag.item;
        dup->tag.content = cbor_duplicate(val->tag.content);
        break;
    }
    case CBOR_TYPE_SIMPLE: {
        dup = cbor_create(val->type);
        dup->simple.ctrl = val->simple.ctrl;
        dup->simple.real = val->simple.real;
        break;
    }
    case CBOR__TYPE_PAIR: {
        dup = cbor_create(val->type);
        dup->pair.key = cbor_duplicate(val->pair.key);
        dup->pair.val = cbor_duplicate(val->pair.val);
        break;
    }
    }
    return dup;
}

cbor_value_t *cbor_pair_key(const cbor_value_t *val) {
    if (val && val->type == CBOR__TYPE_PAIR) {
        return val->pair.key;
    }
    return NULL;
}

cbor_value_t *cbor_pair_value(const cbor_value_t *val) {
    if (val && val->type == CBOR__TYPE_PAIR) {
        return val->pair.val;
    }
    return NULL;
}

int cbor_blob_replace(cbor_value_t *val, char **str, size_t *length) {
    if (val && (val->type == CBOR_TYPE_STRING || val->type == CBOR_TYPE_BYTESTRING)) {
        char *ptmp = val->blob.ptr;
        size_t stmp = val->blob.length;

        val->blob.ptr = *str;
        val->blob.length = *length;
        val->blob.allocated = *length;

        *str = ptmp;
        *length = stmp;
        return 0;
    }
    return -1;
}

const char *cbor_type_str(const cbor_value_t *val) {
    if (val == NULL) {
        return "";
    }
    switch (val->type) {
    case CBOR_TYPE_UINT:
        return "integer(unsigned)";
    case CBOR_TYPE_NEGINT:
        return "integer(signed)";
    case CBOR_TYPE_BYTESTRING:
        return "byte-string";
    case CBOR_TYPE_STRING:
        return "string";
    case CBOR_TYPE_ARRAY:
        return "array";
    case CBOR_TYPE_MAP:
        return "map";
    case CBOR_TYPE_TAG:
        return "tag";
    case CBOR_TYPE_SIMPLE:
        return "simple";
    case CBOR__TYPE_PAIR:
        return ":pair:";
    }
    return "";
}
