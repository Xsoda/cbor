#ifndef __CBOR_INTERNAL_H__
#define __CBOR_INTERNAL_H__

#include "list.h"

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

#endif  /* !__CBOR_INTERNAL_H__ */
