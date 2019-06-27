#ifndef __CBOR_H__
#define __CBOR_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CBOR_ITER_AFTER,
    CBOR_ITER_BEFORE,
} cbor_iter_dir;

typedef struct _cbor_value cbor_value_t;

typedef struct _cbor_iter {
    const cbor_value_t *container;
    cbor_value_t *next;
    cbor_iter_dir dir;
} cbor_iter_t;

const char *cbor_type_str(const cbor_value_t *val);
int cbor_destroy(cbor_value_t *val);

int cbor_blob_append(cbor_value_t *val, const char *src, size_t length);
int cbor_blob_append_v(cbor_value_t *val, const char *fmt, ...);
int cbor_blob_append_byte(cbor_value_t *val, uint8_t byte);
int cbor_blob_append_word(cbor_value_t *val, uint16_t word);
int cbor_blob_append_dword(cbor_value_t *val, uint32_t dword);
int cbor_blob_append_qword(cbor_value_t *val, uint64_t qword);
int cbor_blob_replace(cbor_value_t *val, char **str, size_t *length);

int cbor_container_empty(const cbor_value_t *container);
int cbor_container_size(const cbor_value_t *container);
int cbor_container_swap(cbor_value_t *ca, cbor_value_t *cb);
int cbor_container_clear(cbor_value_t *container);
int cbor_container_split(cbor_value_t *container, cbor_value_t *val, cbor_value_t *sub);
int cbor_container_insert_tail(cbor_value_t *container, cbor_value_t *val);
int cbor_container_insert_head(cbor_value_t *container, cbor_value_t *val);
int cbor_container_insert_after(cbor_value_t *container, cbor_value_t *elm, cbor_value_t *val);
int cbor_container_insert_before(cbor_value_t *container, cbor_value_t *elm, cbor_value_t *val);

int cbor_map_insert(cbor_value_t *map, cbor_value_t *key, cbor_value_t *val);
int cbor_map_destroy(cbor_value_t *map, const char *key);
int cbor_map_set_integer(cbor_value_t *map, const char *key, long long integer);
int cbor_map_set_double(cbor_value_t *map, const char *key, double dbl);
int cbor_map_set_boolean(cbor_value_t *map, const char *key, bool boolean);
int cbor_map_set_string(cbor_value_t *map, const char *key, const char *str);
int cbor_map_set_null(cbor_value_t *map, const char *key);
int cbor_map_set_value(cbor_value_t *map, const char *key, cbor_value_t *value);

cbor_value_t *cbor_map_unlink(cbor_value_t *map, const char *key);
cbor_value_t *cbor_map_remove(cbor_value_t *map, const char *key);
cbor_value_t *cbor_map_dotget(const cbor_value_t *map, const char *key);
const char *cbor_map_dotget_string(const cbor_value_t *map, const char *key);
long long cbor_map_dotget_integer(const cbor_value_t *map, const char *key);
bool cbor_map_dotget_boolean(const cbor_value_t *map, const char *key);
double cbor_map_dotget_double(const cbor_value_t *map, const char *key);

cbor_value_t *cbor_array_get(const cbor_value_t *array, int idx);
const char *cbor_array_get_string(const cbor_value_t *array, int idx);
long long cbor_array_get_integer(const cbor_value_t *array, int idx);
double cbor_array_get_double(const cbor_value_t *array, int idx);
bool cbor_array_get_boolean(const cbor_value_t *array, int idx);

long long cbor_integer(const cbor_value_t *val);
double cbor_real(const cbor_value_t *val);
int cbor_string_size(const cbor_value_t *val);
const char *cbor_string(const cbor_value_t *val);
bool cbor_boolean(const cbor_value_t *val);

bool cbor_is_boolean(const cbor_value_t *val);
bool cbor_is_integer(const cbor_value_t *val);
bool cbor_is_double(const cbor_value_t *val);
bool cbor_is_bytestring(const cbor_value_t *val);
bool cbor_is_string(const cbor_value_t *val);
bool cbor_is_map(const cbor_value_t *val);
bool cbor_is_array(const cbor_value_t *val);
bool cbor_is_tag(const cbor_value_t *val);
bool cbor_is_null(const cbor_value_t *val);
bool cbor_is_number(const cbor_value_t *val);

cbor_value_t *cbor_pair_key(const cbor_value_t *val);
cbor_value_t *cbor_pair_value(const cbor_value_t *val);

cbor_value_t *cbor_init_boolean(bool b);
cbor_value_t *cbor_init_null();
cbor_value_t *cbor_init_map();
cbor_value_t *cbor_init_array();
cbor_value_t *cbor_init_integer(long long l);
cbor_value_t *cbor_init_string(const char *str, int len);
cbor_value_t *cbor_init_double(double d);
cbor_value_t *cbor_init_bytestring(const char *str, int len);

cbor_value_t *cbor_container_first(const cbor_value_t *container);
cbor_value_t *cbor_container_last(const cbor_value_t *container);
cbor_value_t *cbor_container_next(const cbor_value_t *container, cbor_value_t *elm);
cbor_value_t *cbor_container_prev(const cbor_value_t *container, cbor_value_t *elm);
cbor_value_t *cbor_container_remove(cbor_value_t *container, cbor_value_t *elm);
int cbor_container_concat(cbor_value_t *dst, cbor_value_t *src);

cbor_value_t *cbor_duplicate(cbor_value_t *val);

void cbor_iter_init(cbor_iter_t *iter, const cbor_value_t *container, cbor_iter_dir dir);
cbor_value_t *cbor_iter_next(cbor_iter_t *iter);

long cbor_tag_get_item(cbor_value_t *val);
cbor_value_t *cbor_tag_get_content(cbor_value_t *val);
int cbor_tag_set(cbor_value_t *tag, long item, cbor_value_t *content);

cbor_value_t *cbor_loads(const char *src, size_t *length);
char *cbor_dumps(const cbor_value_t *src, size_t *length);

cbor_value_t *cbor_json_loads(const void *src, int size);
char *cbor_json_dumps(const cbor_value_t *src, size_t *length, bool pretty);
cbor_value_t *cbor_json_loadss(const void *src, size_t *size);


cbor_value_t *cbor_json_loadf(const char *path);
int cbor_json_dumpf(cbor_value_t *val, const char *path, bool pretty);

#ifdef __cplusplus
}
#endif

#endif  /* !__CBOR_H__ */
