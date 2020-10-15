#ifndef __CBOR_DEFINE_H__
#define __CBOR_DEFINE_H__

#include "list.h"

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
    struct _cbor_value *parent;
};

struct _cbor_value *cbor_create(cbor_type type);
#endif  /* !__CBOR_DEFINE_H__ */
