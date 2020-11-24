#include "cbor.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdarg.h>
#include "define.h"
#include "fastsearch.h"
#ifdef __FreeBSD__
#include <sys/endian.h>
#endif

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

    assert(val->parent == NULL);

    if (val->type == CBOR_TYPE_ARRAY || val->type == CBOR_TYPE_MAP) {
        cbor_value_t *var, *tvar;
        list_foreach_safe(var, &val->container, entry, tvar) {
            cbor_container_remove(val, var);
            cbor_destroy(var);
        }
    } else if (val->type == CBOR_TYPE_BYTESTRING || val->type == CBOR_TYPE_STRING) {
        free(val->blob.ptr);
        val->blob.ptr = NULL;
    } else if (val->type == CBOR__TYPE_PAIR) {
        cbor_value_t *var;
        var = cbor_pair_unset_key(val);
        cbor_destroy(var);
        var = cbor_pair_unset_value(val);
        cbor_destroy(var);
    } else if (val->type == CBOR_TYPE_TAG) {
        cbor_value_t *var = cbor_tag_unset_content(val);
        cbor_destroy(var);
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
        return val->blob.allocated - val->blob.length;
    }
    return 0;
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
        val->blob.ptr[val->blob.length] = 0;
        return 0;
    }
    return -1;
}

int cbor_blob_append_word(cbor_value_t *val, uint16_t word) {
    if (cbor_blob_avalible(val, 2) > 2) {
        *(uint16_t *)&val->blob.ptr[val->blob.length] = htobe16(word);
        val->blob.length += 2;
        val->blob.ptr[val->blob.length] = 0;
        return 0;
    }
    return -1;
}

int cbor_blob_append_dword(cbor_value_t *val, uint32_t dword) {
    if (cbor_blob_avalible(val, 4) > 4) {
        *(uint32_t *)&val->blob.ptr[val->blob.length] = htobe32(dword);
        val->blob.length += 4;
        val->blob.ptr[val->blob.length] = 0;
        return 0;
    }
    return -1;
}

int cbor_blob_append_qword(cbor_value_t *val, uint64_t qword) {
    if (cbor_blob_avalible(val, 8) > 8) {
        *(uint64_t *)&val->blob.ptr[val->blob.length] = htobe64(qword);
        val->blob.length += 8;
        val->blob.ptr[val->blob.length] = 0;
        return 0;
    }
    return -1;
}

void cbor_blob_trim(cbor_value_t *val) {
    size_t t;
    if (val == NULL || val->type != CBOR_TYPE_STRING) {
        return;
    }
    for (t = 0; t < val->blob.length; t++) {
        if (!isspace((unsigned char)val->blob.ptr[t])) {
            break;
        }
    }
    if (t > 0) {
        memmove(val->blob.ptr, val->blob.ptr + t, val->blob.length - t);
        val->blob.length -= t;
    }
    while (val->blob.length > 0 && isspace((unsigned char)val->blob.ptr[val->blob.length - 1]))
        val->blob.length -= 1;
    val->blob.ptr[val->blob.length] = 0;
}

int cbor_container_empty(const cbor_value_t *container) {
    if (container && (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP)) {
        return list_empty(&container->container);
    }
    return 0;
}

int cbor_container_size(const cbor_value_t *container) {
    int size = 0;
    if (container && (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP)) {
        cbor_value_t *var;
        list_foreach(var, &container->container, entry) {
            size++;
        }
    }
    return size;
}

int cbor_container_clear(cbor_value_t *container) {
    if (!container) {
        return -1;
    }

    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        cbor_value_t *var, *tvar;
        list_foreach_safe(var, &container->container, entry, tvar) {
            list_remove(&container->container, var, entry);
            cbor_destroy(var);
        }
    }
    return 0;
}

int cbor_container_swap(cbor_value_t *ca, cbor_value_t *cb) {
    if (ca
        && cb
        && ca->type == cb->type
        && (ca->type == CBOR_TYPE_ARRAY || ca->type == CBOR_TYPE_MAP)) {
        cbor_value_t *var;
        list_swap(&ca->container, &cb->container, _cbor_value, entry);
        list_foreach(var, &ca->container, entry) {
            var->parent = ca;
        }
        list_foreach(var, &cb->container, entry) {
            var->parent = cb;
        }
    }
    return -1;
}

int cbor_container_insert_tail(cbor_value_t *container, cbor_value_t *val) {
    if (!container || !val) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(val->parent == NULL);
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);
        assert((container->type == CBOR_TYPE_MAP && val->type == CBOR__TYPE_PAIR) || container->type == CBOR_TYPE_ARRAY);
        list_insert_tail(&container->container, val, entry);
        val->parent = container;
        return 0;
    }
    return -1;
}

int cbor_container_insert_head(cbor_value_t *container, cbor_value_t *val) {
    if (!container || !val) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(val->parent == NULL);
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);
        list_insert_head(&container->container, val, entry);
        val->parent = container;
        return 0;
    }
    return -1;
}

int cbor_container_insert_after(cbor_value_t *container, cbor_value_t *elm, cbor_value_t *val) {
    if (!container || !val || !elm) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(elm->parent == container && val->parent == NULL);
        assert(elm->entry.le_next != NULL || elm->entry.le_prev != NULL);
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);

        list_insert_after(&container->container, elm, val, entry);
        val->parent = container;
        return 0;
    }
    return -1;
}

int cbor_container_insert_before(cbor_value_t *container, cbor_value_t *elm, cbor_value_t *val) {
    if (!container || !val || !elm) {
        return -1;
    }
    if (container->type == CBOR_TYPE_ARRAY || container->type == CBOR_TYPE_MAP) {
        assert(elm->parent == container && val->parent == NULL);
        assert(elm->entry.le_next != NULL || elm->entry.le_prev != NULL);
        assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);

        list_insert_before(&container->container, elm, val, entry);
        val->parent = container;
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
        assert(elm->parent == container);
        assert(elm->entry.le_next != NULL || elm->entry.le_prev != NULL);
        list_remove(&container->container, elm, entry);
        elm->entry.le_next = NULL;
        elm->entry.le_prev = NULL;
        elm->parent = NULL;
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
        cbor_value_t *first = cbor_container_first(src);
        list_concat(&dst->container, &src->container, entry);
        list_foreach_from(first, &dst->container, entry) {
            first->parent = dst;
        }
        return 0;
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
        } else if (addition == 24 && offset + 1 <= *length) { /* uint8_t */
            val->uint = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* uint16_t */
            val->uint = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26 && offset + 4 <= *length) { /* uint32_t */
            val->uint = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27 && offset + 8 <= *length) { /* uint64_t */
            val->uint = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        } else {
            cbor_destroy(val);
            offset = 0;
            val = NULL;
        }
        break;
    }
    case CBOR_TYPE_NEGINT: {    /* NOTE: negint = -1 - uint */
        val = cbor_create(CBOR_TYPE_NEGINT);
        offset++;
        if (addition < 24) {
            val->uint = addition;
        } else if (addition == 24 && offset + 1 <= *length) { /* uint8_t */
            val->uint = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* uint16_t */
            val->uint = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26 && offset + 4 <= *length) { /* uint32_t */
            val->uint = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27 && offset + 8 <= *length) { /* uint64_t */
            val->uint = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        } else {
            cbor_destroy(val);
            offset = 0;
            val = NULL;
        }
        break;
    }
    case CBOR_TYPE_BYTESTRING: {
        val = cbor_create(CBOR_TYPE_BYTESTRING);
        offset++;
        int len = 0;
        if (addition < 24) {
            len = addition;
        } else if (addition == 24 && offset + 1 <= *length) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26 && offset + 4 <= *length) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27 && offset + 8 <= *length) { /* uint64_t */
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
                    sub = NULL;
                    break;
                }
                if (sub) {
                    cbor_destroy(sub);
                }
            }
        }
        if (addition != 31) {
            if (offset + len <= *length) {
                cbor_blob_append(val, &src[offset], len);
                val->blob.ptr[val->blob.length] = 0;
                offset += len;
            } else {
                cbor_destroy(val);
                val = NULL;
                offset = 0;
            }
        }
        break;
    }
    case CBOR_TYPE_STRING: {
        val = cbor_create(CBOR_TYPE_STRING);
        offset++;
        int len = 0;
        if (addition < 24) {
            len = addition;
        } else if (addition == 24 && offset + 1 <= *length) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26 && offset + 4 <= *length) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27 && offset + 8 <= *length) { /* uint64_t */
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
                    sub = NULL;
                    break;
                }
                if (sub) {
                    cbor_destroy(sub);
                }
            }
        }
        if (addition != 31) {
            if (offset + len <= *length) {
                cbor_blob_append(val, &src[offset], len);
                val->blob.ptr[val->blob.length] = 0;
                offset += len;
            } else {
                cbor_destroy(val);
                val = NULL;
                offset = 0;
            }
        }
        break;
    }
    case CBOR_TYPE_ARRAY: {
        val = cbor_create(CBOR_TYPE_ARRAY);
        offset++;               /* array count */
        int len = 0;
        if (addition < 24) {
            len = addition;
        } else if (addition == 24 && offset + 1 <= *length) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26 && offset + 4 <= *length) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27 && offset + 8 <= *length) { /* uint64_t */
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
                if (elm && offset + remain <= *length) {
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
                if (elm && offset + remain <= *length) {
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
        } else if (addition == 24 && offset + 1 <= *length) { /* uint8_t */
            len = (unsigned char)src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* uint16_t */
            len = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26 && offset + 4 <= *length) { /* uint32_t */
            len = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27 && offset + 8 <= *length) { /* uint64_t */
            len = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        } else if (addition == 31) { /* indefinite */
            while (offset < *length) {
                if (src[offset] == -1) {
                    offset++;
                    break;
                }
                size_t remain = *length - offset;
                cbor_value_t *k = cbor_loads(src + offset, &remain);
                if (k && offset + remain <= *length) {
                    offset += remain;
                    remain = *length - offset;
                    cbor_value_t *v = cbor_loads(src + offset, &remain);
                    if (v && offset + remain <= *length) {
                        offset += remain;
                        cbor_value_t *pair = cbor_create(CBOR__TYPE_PAIR);
                        pair->pair.key = k;
                        pair->pair.value = v;
                        cbor_container_insert_tail(val, pair);
                    } else {
                        /* error */
                        cbor_destroy(v);
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
                if (k && offset + remain <= *length) {
                    offset += remain;
                    remain = *length - offset;
                    cbor_value_t *v = cbor_loads(src + offset, &remain);
                    if (v && offset + remain <= *length) {
                        offset += remain;
                        cbor_value_t *pair = cbor_create(CBOR__TYPE_PAIR);
                        pair->pair.key = k;
                        pair->pair.value = v;
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
        } else if (addition == 24 && offset + 1 <= *length) { /* uint8_t */
            val->tag.item =(unsigned char)src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* uint16_t */
            val->tag.item = be16toh(*(uint16_t *)&src[offset]);
            offset += 2;
        } else if (addition == 26 && offset + 4 <= *length) { /* uint32_t */
            val->tag.item = be32toh(*(uint32_t *)&src[offset]);
            offset += 4;
        } else if (addition == 27 && offset + 8 <= *length) { /* uint64_t */
            val->tag.item = be64toh(*(uint64_t *)&src[offset]);
            offset += 8;
        }
        size_t remain = *length - offset;
        val->tag.content = cbor_loads(src + offset, &remain);
        if (val->tag.content && offset + remain <= *length) {
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
        } else if (addition == 24 && offset + 1 <= *length) { /* Simple value: extension */
            /* Reserved */
            val->simple.ctrl = src[offset];
            offset++;
        } else if (addition == 25 && offset + 2 <= *length) { /* half float */
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
        } else if (addition == 26 && offset + 4 <= *length) { /* float */
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
        } else if (addition == 27 && offset + 8 <= *length) { /* double */
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
            cbor__dumps(var->pair.value, dst);
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

cbor_value_t *cbor_tag_unset_content(cbor_value_t *val) {
    assert(val != NULL && val->type == CBOR_TYPE_TAG);
    cbor_value_t *tmp = val->tag.content;
    if (tmp) {
        tmp->parent = NULL;
    }
    val->tag.content = NULL;
    return tmp;
}

void cbor_tag_reset_content(cbor_value_t *val, cbor_value_t *content) {
    assert(content == NULL || (content != NULL && val->parent == NULL));
    cbor_value_t *tmp = cbor_tag_unset_content(val);
    val->tag.content = content;
    if (content) {
        content->parent = val;
    }
    cbor_destroy(tmp);
}

void cbor_tag_reset_item(cbor_value_t *val, long item) {
    assert(val != NULL && val->type == CBOR_TYPE_TAG);
    val->tag.item = item;
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

cbor_value_t *cbor_duplicate(const cbor_value_t *val) {
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
        dup = cbor_init_pair(cbor_duplicate(val->pair.key), cbor_duplicate(val->pair.value));
        break;
    }
    }
    return dup;
}

cbor_value_t *cbor_init_pair(cbor_value_t *key, cbor_value_t *val) {
    assert(key->parent == NULL);
    assert(val->parent == NULL);
    assert(key->entry.le_next == NULL && key->entry.le_prev == NULL);
    assert(val->entry.le_next == NULL && val->entry.le_prev == NULL);
    cbor_value_t *p = cbor_create(CBOR__TYPE_PAIR);
    p->pair.key = key;
    p->pair.value = val;
    key->parent = p;
    val->parent = p;
    return p;
}

cbor_value_t *cbor_pair_key(const cbor_value_t *val) {
    if (val && val->type == CBOR__TYPE_PAIR) {
        return val->pair.key;
    }
    return NULL;
}

cbor_value_t *cbor_pair_value(const cbor_value_t *val) {
    if (val && val->type == CBOR__TYPE_PAIR) {
        return val->pair.value;
    }
    return NULL;
}

cbor_value_t *cbor_pair_unset_key(cbor_value_t *val) {
    assert(val != NULL && val->type == CBOR__TYPE_PAIR);
    cbor_value_t *tmp = val->pair.key;
    if (tmp) {
        tmp->parent = NULL;
    }
    val->pair.key = NULL;
    return tmp;
}

cbor_value_t *cbor_pair_unset_value(cbor_value_t *val) {
    assert(val != NULL && val->type == CBOR__TYPE_PAIR);
    cbor_value_t *tmp = val->pair.value;
    if (tmp) {
        tmp->parent = NULL;
    }
    val->pair.value = NULL;
    return tmp;
}

void cbor_pair_reset_key(cbor_value_t *val, cbor_value_t *key) {
    assert(key == NULL || (key != NULL && key->parent == NULL));
    cbor_value_t *tmp = cbor_pair_unset_key(val);
    val->pair.key = key;
    if (key) {
        key->parent = val;
    }
    cbor_destroy(tmp);
}

void cbor_pair_reset_value(cbor_value_t *val, cbor_value_t *value) {
    assert(value == NULL || (value != NULL && value->parent == NULL));
    cbor_value_t *tmp = cbor_pair_unset_value(val);
    val->pair.value = value;
    if (value) {
        value->parent = val;
    }
    cbor_destroy(tmp);
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

cbor_value_t *cbor_string_split(const char *str, const char *f) {
    if (!str || !f) {
        return NULL;
    }

    int l = strlen(str);
    int m = strlen(f);

    int size;
    int offset = 0;
    cbor_value_t *result = cbor_init_array();
    do {
        size = FASTSEARCH(str + offset, l - offset, f, m, -1, FAST_SEARCH);
        if (size >= 0) {
            cbor_value_t *s = cbor_init_string(str + offset, size);
            offset += size;
            offset += m;
            cbor_container_insert_tail(result, s);
        }
    } while (size >= 0);

    if (offset <= l) {
        cbor_value_t *s = cbor_init_string(str + offset, l - offset);
        cbor_container_insert_tail(result, s);
    }

    return result;
}

cbor_value_t *cbor_string_join(cbor_value_t *array, const char *str) {
    if (cbor_is_array(array) && str) {
        cbor_iter_t iter;
        cbor_value_t *ele;
        int len = strlen(str);
        cbor_value_t *result = cbor_init_string("", 0);
        cbor_iter_init(&iter, array, CBOR_ITER_AFTER);
        while ((ele = cbor_iter_next(&iter)) != NULL) {
            if (cbor_is_string(ele)) {
                cbor_blob_append(result, cbor_string(ele), cbor_string_size(ele));
            }
            cbor_blob_append(result, str, len);
        }
        return result;
    }
    return NULL;
}

cbor_value_t *cbor_string_split_whitespace(const char *str) {
    return cbor_string_split_character(str, -1, "\t\n\v\f\r ", 6);
}

cbor_value_t *cbor_string_split_linebreak(const char *str) {
    const char *origin = str;
    cbor_value_t *result = cbor_init_array();
    while (*str) {
        if (str[0] == '\r' && str[1] == '\n') {
            cbor_value_t *ele = cbor_init_string(origin, str - origin);
            cbor_container_insert_tail(result, ele);
            str += 2;
            origin = str;
        } else if (str[0] == '\r' || str[0] == '\n') {
            cbor_value_t *ele = cbor_init_string(origin, str - origin);
            cbor_container_insert_tail(result, ele);
            str += 1;
            origin = str;
        } else {
            str += 1;
        }
    }
    cbor_value_t *ele = cbor_init_string(origin, str - origin);
    cbor_container_insert_tail(result, ele);
    return result;
}

cbor_value_t *cbor_string_split_character(const char *str, int length, const char *characters, int size) {
    int from, to;
    cbor_value_t *result;
    if (!str || !characters)
        return NULL;
    if (length < 0) {
        length = strlen(str);
    }
    if (size < 0) {
        size = strlen(characters);
    }
    from = 0;
    to = 0;
    result = cbor_init_array();
    while (to < length) {
        from = to;
        while (from < length) {
            if (memchr(characters, str[from], size)) {
                from += 1;
            } else {
                break;
            }
        }
        to = from;
        while (to < length) {
            if (!memchr(characters, str[to], size)) {
                to += 1;
            } else {
                break;
            }
        }
        if (to > from) {
            cbor_value_t *s = cbor_init_string(str + from, to - from);
            cbor_container_insert_tail(result, s);
        }
    }
    return result;
}

int cbor_string_replace(cbor_value_t *str, const char *find, const char *repl) {
    int l, m, off, size;
    if (!cbor_is_string(str) || !find || !repl) {
        return -1;
    }

    l = strlen(find);
    m = strlen(repl);

    off = 0;
    do {
        size = FASTSEARCH(str->blob.ptr + off, str->blob.length - off, find, l, -1, FAST_SEARCH);
        if (size >= 0) {
            if (cbor_blob_avalible(str, m)) {
                memmove(str->blob.ptr + off + size + m,
                        str->blob.ptr + off + size + l,
                        str->blob.length - off - size - l);
                memcpy(str->blob.ptr + off + size, repl, m);
                str->blob.length += m;
                str->blob.length -= l;
                str->blob.ptr[str->blob.length] = 0;
                off += size;
                off += m;
            }
        }
    } while (size >= 0);
    return cbor_string_size(str);
}

bool cbor_string_startswith(cbor_value_t *str, const char *first) {
    if (first == NULL || !cbor_is_string(str))
        return false;
    int len = strlen(first);
    if (len > cbor_string_size(str)) {
        return false;
    }
    return memcmp(cbor_string(str), first, len) == 0;
}

bool cbor_string_endswith(cbor_value_t *str, const char *last) {
    if (last == NULL || !cbor_is_string(str))
        return false;
    int len = strlen(last);
    if (len > cbor_string_size(str))
        return false;
    return memcmp(cbor_string(str) + cbor_string_size(str) - len, last, len) == 0;
}

int cbor_string_strip(cbor_value_t *str) {
    int loff, roff, i, size;
    if (!cbor_is_string(str)) {
        return 0;
    }

    loff = 0;
    size = cbor_string_size(str);
    for (i = 0; i < size; i++) {
        if (isspace((unsigned char)str->blob.ptr[i])) {
            loff += 1;
        } else {
            break;
        }
    }

    roff = 0;
    for (i = size - 1; i >= 0; i--) {
        if (isspace((unsigned char)str->blob.ptr[i])) {
            roff += 1;
        } else {
            break;
        }
    }

    size -= loff;
    size -= roff;

    if (size <= 0) {
        str->blob.ptr[0] = 0;
        str->blob.length = 0;
    } else {
        memmove(str->blob.ptr, str->blob.ptr + loff, size);
        str->blob.ptr[size] = 0;
        str->blob.length = size;
    }
    return cbor_string_size(str);
}

int cbor_string_lstrip(cbor_value_t *str) {
    int loff, i, size;
    if (!cbor_is_string(str)) {
        return 0;
    }

    loff = 0;
    size = cbor_string_size(str);
    for (i = 0; i < size; i++) {
        if (isspace((unsigned char)str->blob.ptr[i])) {
            loff += 1;
        } else {
            break;
        }
    }

    size -= loff;
    if (loff > 0) {
        if (size > 0) {
            memmove(str->blob.ptr, str->blob.ptr + loff, size);
            str->blob.ptr[size] = 0;
            str->blob.length = size;
        } else {
            str->blob.ptr[0] = 0;
            str->blob.length = 0;
        }
    }
    return cbor_string_size(str);
}

int cbor_string_rstrip(cbor_value_t *str) {
    int roff, i, size;
    if (!cbor_is_string(str)) {
        return 0;
    }

    roff = 0;
    size = cbor_string_size(str);
    for (i = size - 1; i >= 0; i--) {
        if (isspace((unsigned char)str->blob.ptr[i])) {
            roff += 1;
        } else {
            break;
        }
    }

    size -= roff;
    if (roff > 0) {
        if (size > 0) {
            str->blob.ptr[size] = 0;
            str->blob.length = size;
        } else {
            str->blob.ptr[0] = 0;
            str->blob.length = 0;
        }
    }
    return cbor_string_size(str);
}

cbor_value_t *cbor_get_parent(cbor_value_t *val) {
    if (val) {
        return val->parent;
    }
    return NULL;
}
