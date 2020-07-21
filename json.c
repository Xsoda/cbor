#include "cbor.h"
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    JSON_ERR_NONE = 0,
    JSON_ERR_UNEXPECTED_CHARACTER,
    JSON_ERR_CONVERT_NUMBER,
    JSON_ERR_CHARACTER_SEQUENCE,
    JSON_ERR_HEX_VALUE,
    JSON_ERR_UTF16,
    JSON_ERR_STRING_BREAKLINE,
    JSON_ERR_STRING_INFINITY,
    JSON_ERR_STRING_CODEPOINT,
    JSON_ERR_UNSUPPORTED_TYPE,
} lexer_error;

const char *json_err_str[] = {
    "success",
    "unexpected character",
    "convert number fail",
    "expected character sequence",
    "4-byte hex digit error",
    "utf-16 surrogate error",
    "unexpected break line in string",
    "infinity string value",
    "unicode point error",
    "unsupported cbor type",
    NULL,
};

typedef struct lexer_s {
    const char *source;
    const char *eof;
    const char *cursor;
    const char *linst;
    lexer_error last_error;
    int linum;
    int linoff;
} lexer_t;

extern int cbor_blob_replace(cbor_value_t *val, char **str, size_t *length);
cbor_value_t *json_parse_string(lexer_t *lexer);
cbor_value_t *json_parse_value(lexer_t *lexer);

static void lexer_skip_block_comment(lexer_t *lexer) {
    while (lexer->cursor < lexer->eof) {
        int ch = (unsigned char)*lexer->cursor;
        if (ch == '*' && lexer->cursor + 2 < lexer->eof) {
            int next = lexer->cursor[1];
            if (next == '/') {
                lexer->linoff += 2;
                lexer->cursor += 2;
                break;
            } else {
                lexer->linoff++;
                lexer->cursor++;
            }
        } else if (ch == '/' && lexer->cursor + 2 < lexer->eof) {
            int next = (unsigned char)lexer->cursor[1];
            if (next == '*') {
                lexer->linoff += 2;
                lexer->cursor += 2;
                lexer_skip_block_comment(lexer);
            } else {
                lexer->cursor++;
                lexer->linoff++;
            }
        } else if (ch == '\r') {
            lexer->linum++;
            lexer->cursor++;
            lexer->linoff = 0;
            lexer->linst = lexer->cursor;
        } else if (ch == '\n') {
            int prev = (unsigned char)lexer->cursor[-1];
            if (prev != '\r' && lexer->cursor > lexer->source) {
                lexer->linum++;
                lexer->cursor++;
                lexer->linoff = 0;
                lexer->linst = lexer->cursor;
            } else {
                lexer->cursor++;
            }
        } else {
            lexer->cursor++;
            lexer->linoff++;
        }
    }
}

static void lexer_skip_whitespace(lexer_t *lexer) {
    while (lexer->cursor < lexer->eof) {
        int ch = (unsigned char)*lexer->cursor;
        if (ch == '\r') {
            lexer->linum++;
            lexer->cursor++;
            lexer->linoff = 0;
            lexer->linst = lexer->cursor;
        } else if (ch == '\n') {
            int prev = lexer->cursor[-1];
            if (prev != '\r' && lexer->cursor > lexer->source) {
                lexer->linum++;
                lexer->cursor++;
                lexer->linoff = 0;
                lexer->linst = lexer->cursor;
            } else {
                lexer->cursor++;
            }
        } else if (isspace(ch)) {
            lexer->cursor++;
            lexer->linoff++;
        } else if (ch == '#') {
            while (lexer->cursor < lexer->eof
                   && *lexer->cursor != '\n'
                   && *lexer->cursor != '\r') {
                lexer->cursor++;
                lexer->linoff++;
            }
        } else if (ch == '/' && lexer->cursor + 2 < lexer->eof) {
            int next = lexer->cursor[1];
            if (next == '/') {
                lexer->cursor += 2;
                lexer->linoff += 2;
                while (lexer->cursor < lexer->eof
                       && *lexer->cursor != '\n'
                       && *lexer->cursor != '\r') {
                    lexer->cursor++;
                    lexer->linoff++;
                }
            } else if (next == '*') {
                lexer->cursor += 2;
                lexer->linoff += 2;
                lexer_skip_block_comment(lexer);
            }
        } else {
            break;
        }
    }
}

void json_lexer_error(lexer_t *lexer) {
    char buffer[4096];
    size_t length = 0;
    int offset = 0;
    length = snprintf(buffer, sizeof(buffer), "json lexer error at line %d, offset %d\n", lexer->linum, lexer->linoff);
    offset = snprintf(buffer + length, sizeof(buffer) - length, "%04d | ", lexer->linum);
    length += offset;
    if (lexer->linoff > 50) {
        int i;
        length += snprintf(buffer + length, sizeof(buffer) - length, " ... ");
        offset += 5;
        for (i = lexer->linoff - 20;
             lexer->linst[i] != '\r' && lexer->linst[i] != '\n' && i < lexer->linoff + 20;
             i++) {
            length += snprintf(buffer + length, sizeof(buffer) - length, "%c", (unsigned char)lexer->linst[i]);
        }
        if (lexer->linst[i] != '\r' && lexer->linst[i] != '\n') {
            length += snprintf(buffer + length, sizeof(buffer) - length, " ...\n");
        } else {
            length += snprintf(buffer + length, sizeof(buffer) - length, "\n");
        }
        offset += 20;
    } else {
        int i;
        offset += lexer->linoff;
        for (i = 0;
             lexer->linst[i] != '\r' && lexer->linst[i] != '\n' && i < 70;
             i++) {
            length += snprintf(buffer + length, sizeof(buffer) - length, "%c", (unsigned char)lexer->linst[i]);
        }
        if (lexer->linst[i] != '\r' && lexer->linst[i] != '\n') {
            length += snprintf(buffer + length, sizeof(buffer) - length, " ...\n");
        } else {
            length += snprintf(buffer + length, sizeof(buffer) - length, "\n");
        }
    }
    offset++;
    length += snprintf(buffer + length, sizeof(buffer) - length, "%*s\n", offset, "^");
    length += snprintf(buffer + length, sizeof(buffer) - length, "%*s\n", offset, "|");
    length += snprintf(buffer + length, sizeof(buffer) - length, "%*s\n", offset, json_err_str[lexer->last_error]);
    fprintf(stdout, "%s", buffer);
}

cbor_value_t *json_parse_object(lexer_t *lexer) {
    cbor_value_t *object;

    object = cbor_init_map();

    lexer_skip_whitespace(lexer);

    int leader = (unsigned char)*lexer->cursor;
    if (leader == '}') {
        lexer->cursor++;
        lexer->linoff++;
        return object;
    }

    while (lexer->cursor < lexer->eof) {
        cbor_value_t *key = NULL;
        cbor_value_t *val = NULL;

        lexer_skip_whitespace(lexer);

        int leader = (unsigned char)*lexer->cursor;

        if (leader == '"') {
            key = json_parse_string(lexer);
            if (key == NULL) {
                cbor_destroy(object);
                object = NULL;
                break;
            }

            lexer_skip_whitespace(lexer);

            leader = (unsigned char)*lexer->cursor;

            if (leader == ':') {
                lexer->cursor++;
                lexer->linoff++;

                val = json_parse_value(lexer);

                if (val == NULL) {
                    cbor_destroy(key);
                    cbor_destroy(object);
                    object = NULL;
                    break;
                }

                cbor_map_insert(object, key, val);

                lexer_skip_whitespace(lexer);

                leader = *lexer->cursor;

                if (leader == ',') {
                    lexer->cursor++;
                    lexer->linoff++;
                    continue;
                } else if (leader == '}') {
                    lexer->cursor++;
                    lexer->linoff++;
                    break;
                } else {
                    /* unexpected */
                    lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                    cbor_destroy(object);
                    object = NULL;
                    break;
                }
            } else {
                /* unexpected */
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                cbor_destroy(key);
                cbor_destroy(object);
                object = NULL;
                break;
            }
        } else {
            /* unexpected */
            lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
            cbor_destroy(object);
            object = NULL;
            break;
        }
    }
    return object;
}

cbor_value_t *json_parse_array(lexer_t *lexer) {
    cbor_value_t *array;

    array = cbor_init_array();

    lexer_skip_whitespace(lexer);

    int leader = (unsigned char)*lexer->cursor;
    if (leader == ']') {
        lexer->cursor++;
        lexer->linoff++;
        return array;
    }

    while (lexer->cursor < lexer->eof) {
        cbor_value_t *elm = NULL;

        elm = json_parse_value(lexer);
        if (elm) {
            cbor_container_insert_tail(array, elm);
            elm = NULL;
        } else {
            cbor_destroy(array);
            array = NULL;
            break;
        }

        lexer_skip_whitespace(lexer);

        int leader = (unsigned char)*lexer->cursor;

        if (leader == ',') {
            lexer->cursor++;
            lexer->linoff++;
            continue;
        } else if (leader == ']') {
            lexer->cursor++;
            lexer->linoff++;
            break;
        } else {
            /* unexpected */
            lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
            cbor_destroy(array);
            array = NULL;
            break;
        }
    }
    return array;
}

int json_read_utf16(lexer_t *lexer) {
    int codepoint = 0;
    for (int i = 0; i < 4; i++) {
        codepoint <<= 4;
        if ((unsigned char)lexer->cursor[i] >= '0' && (unsigned char)lexer->cursor[i] <= '9') {
            codepoint |= (unsigned char)lexer->cursor[i] & 0x0F;
        } else {
            codepoint |= ((unsigned char)lexer->cursor[i] | 0x20) - 'a' + 0x0A;
        }
    }
    lexer->cursor += 4;
    lexer->linoff += 4;
    return codepoint;
}

void json_string_write_codepoint(cbor_value_t *str, int codepoint) {
    if (codepoint <= 0x7F) {
        cbor_blob_append_byte(str, codepoint);
    } else if (codepoint <= 0x7FF) {
        uint8_t byte = (codepoint >> 6) | 0xC0;
        cbor_blob_append_byte(str, byte);
        byte = (codepoint & 0x3F) | 0x80;
        cbor_blob_append_byte(str, byte);
    } else if (codepoint <= 0xFFFF) {
        uint8_t byte = (codepoint >> 12) | 0xE0;
        cbor_blob_append_byte(str, byte);
        byte = ((codepoint >> 6) & 0x3F) | 0x80;
        cbor_blob_append_byte(str, byte);
        byte = (codepoint & 0x3F) | 0x80;
        cbor_blob_append_byte(str, byte);
    } else if (codepoint <= 0x10FFFF) {
        uint8_t byte = (codepoint >> 18) | 0xF0;
        cbor_blob_append_byte(str, byte);
        byte = ((codepoint >> 12) & 0x3F) | 0x80;
        cbor_blob_append_byte(str, byte);
        byte = ((codepoint >> 6) & 0x3F) | 0x80;
        cbor_blob_append_byte(str, byte);
        byte = (codepoint & 0x3F) | 0x80;
        cbor_blob_append_byte(str, byte);
    }
}

cbor_value_t *json_parse_string(lexer_t *lexer) {
    cbor_value_t *str = cbor_init_string("", 0);

    lexer->cursor++;
    lexer->linoff++;

    while (*lexer->cursor != '"') {
        if (*lexer->cursor == '\\') {
            if (lexer->cursor[1] == 'r') {
                cbor_blob_append_byte(str, '\r');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == 'n') {
                cbor_blob_append_byte(str, '\n');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == 't') {
                cbor_blob_append_byte(str, '\t');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == 'f') {
                cbor_blob_append_byte(str, '\f');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == '"') {
                cbor_blob_append_byte(str, '"');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == '\\') {
                cbor_blob_append_byte(str, '\\');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == '/') {
                cbor_blob_append_byte(str, '/');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == 'b') {
                cbor_blob_append_byte(str, '\b');
                lexer->cursor += 2;
                lexer->linoff += 2;
            } else if (lexer->cursor[1] == 'u') {
                int high_surrogate = 0;
                int low_surrogate = 0;
                if (isxdigit((unsigned char)lexer->cursor[2])
                    && isxdigit((unsigned char)lexer->cursor[3])
                    && isxdigit((unsigned char)lexer->cursor[4])
                    && isxdigit((unsigned char)lexer->cursor[5])) {
                    lexer->cursor += 2;
                    lexer->linoff += 2;
                    high_surrogate = json_read_utf16(lexer);
                } else {
                    lexer->last_error = JSON_ERR_HEX_VALUE;
                    lexer->linoff += 2;
                    cbor_destroy(str);
                    str = NULL;
                    break;
                }
                if (high_surrogate >= 0xD800 && high_surrogate <= 0xDBFF) {
                    if (isxdigit((unsigned char)lexer->cursor[2])
                        && isxdigit((unsigned char)lexer->cursor[3])
                        && isxdigit((unsigned char)lexer->cursor[4])
                        && isxdigit((unsigned char)lexer->cursor[5])) {
                        lexer->cursor += 2;
                        lexer->linoff += 2;

                        low_surrogate = json_read_utf16(lexer);
                        if (low_surrogate >= 0xDC00 && low_surrogate <= 0xDFFF) {
                            high_surrogate &= 0x3FF;
                            high_surrogate <<= 10;
                            low_surrogate &= 0x3FF;
                            low_surrogate |= high_surrogate;
                            low_surrogate += 0x10000;
                            json_string_write_codepoint(str, low_surrogate);
                        } else {
                            lexer->last_error = JSON_ERR_UTF16;
                            lexer->linoff -= 4;
                            cbor_destroy(str);
                            str = NULL;
                            break;
                        }
                    } else {
                        lexer->last_error = JSON_ERR_HEX_VALUE;
                        lexer->linoff += 2;
                        cbor_destroy(str);
                        str = NULL;
                        break;
                    }
                } else {
                    json_string_write_codepoint(str, high_surrogate);
                }
            } else {
                cbor_blob_append_byte(str, '\\');
                lexer->cursor++;
                lexer->linoff++;
            }
        } else if (*lexer->cursor == '\n' || *lexer->cursor == '\r') {
            lexer->last_error = JSON_ERR_STRING_BREAKLINE;
            cbor_destroy(str);
            str = NULL;
            break;
        } else {
            cbor_blob_append_byte(str, *lexer->cursor);
            lexer->cursor++;
            lexer->linoff++;
        }
    }

    if (lexer->cursor < lexer->eof && *lexer->cursor == '"') {
        lexer->cursor++;
        lexer->linoff++;
    } else {
        if (lexer->last_error == JSON_ERR_NONE) {
            lexer->last_error = JSON_ERR_STRING_INFINITY;
            cbor_destroy(str);
            str = NULL;
        }
    }

    if (str) {
        cbor_blob_append(str, "", 0);
    }

    return str;
}

cbor_value_t *json_parse_number(lexer_t *lexer) {
    char *end;
    int sign = 0;
    int frac = 0;
    int exponent = 0;

    const char *ptr = lexer->cursor;
    cbor_value_t *num = NULL;

    if (*lexer->cursor == '-') {
        sign = 1;
        lexer->cursor++;
        lexer->linoff++;
    } else {
        sign = 0;
    }

    if (*lexer->cursor == '0') {
        lexer->cursor++;
        lexer->linoff++;
    } else {
        while (lexer->cursor < lexer->eof && isdigit(*lexer->cursor)) {
            lexer->cursor++;
            lexer->linoff++;
        }
    }

    if (*lexer->cursor == '.') {
        lexer->cursor++;
        lexer->linoff++;
        frac = 1;
        while (lexer->cursor < lexer->eof && isdigit(*lexer->cursor)) {
            lexer->cursor++;
            lexer->linoff++;
        }
    }

    if (*lexer->cursor == 'e' || *lexer->cursor == 'E') {
        exponent = 1;
        lexer->cursor++;
        lexer->linoff++;
        if (*lexer->cursor == '-') {
            lexer->cursor++;
            lexer->linoff++;
        } else if (*lexer->cursor == '+') {
            lexer->cursor++;
            lexer->linoff++;
        }

        while (lexer->cursor < lexer->eof && isdigit(*lexer->cursor)) {
            lexer->cursor++;
            lexer->linoff++;
        }
    }
    int tail = (unsigned char)*lexer->cursor;

    if (isprint(tail)
        && (isalpha(tail) || isdigit(tail))) {
        /* handle +inf, -inf */
        double dbl = strtod(ptr, &end);
        if (isinf(dbl)) {
            lexer->linoff += (end - ptr);
            lexer->cursor = end;
            num = cbor_init_double(dbl);
        } else {
            lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
        }
    } else {
        if (frac || exponent) {
            num = cbor_init_double(strtod(ptr, &end));
        } else {
            num = cbor_init_integer(strtoll(ptr, &end, 10));
        }
        if (end != lexer->cursor) {
            lexer->last_error = JSON_ERR_CONVERT_NUMBER;
        }
    }

    return num;
}

cbor_value_t *json_parse_value(lexer_t *lexer) {
    lexer_skip_whitespace(lexer);
    int leader = (unsigned char)*lexer->cursor;
    if (leader == '{') {
        lexer->cursor += 1;
        lexer->linoff += 1;
        return json_parse_object(lexer);
    } else if (leader == '[') {
        lexer->cursor += 1;
        lexer->linoff += 1;
        return json_parse_array(lexer);
    } else if (leader == '\"') {
        return json_parse_string(lexer);
    } else if (leader == 't') {
        if (!strncmp(lexer->cursor, "true", 4)) {
            lexer->cursor += 4;
            lexer->linoff += 4;
            int tail = (unsigned char)*lexer->cursor;
            if (isprint(tail)
                && (isalpha(tail)
                    || isdigit(tail))) {
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                return NULL;
            } else {
                return cbor_init_boolean(true);
            }
        } else {
            lexer->last_error = JSON_ERR_CHARACTER_SEQUENCE;
            return NULL;
        }

    } else if (leader == 'f') {
        if (!strncmp(lexer->cursor, "false", 5)) {
            lexer->cursor += 5;
            lexer->linoff += 5;
            int tail = (unsigned char)*lexer->cursor;
            if (isprint(tail)
                && (isalpha(tail)
                    || isdigit(tail))) {
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                return NULL;
            } else {
                return cbor_init_boolean(false);
            }
        } else {
            lexer->last_error = JSON_ERR_CHARACTER_SEQUENCE;
            return NULL;
        }
    } else if (leader == 'n') {
        if (!strncmp(lexer->cursor, "null", 4)) {
            lexer->cursor += 4;
            lexer->linoff += 4;
            int tail = (unsigned char)*lexer->cursor;
            if (isprint(tail)
                && (isalpha(tail)
                    || isdigit(tail))) {
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                return NULL;
            } else {
                return cbor_init_null();
            }
        } else if (!strncasecmp(lexer->cursor, "nan", 3)) {
            double dbl = strtod(lexer->cursor, NULL);
            lexer->cursor += 3;
            lexer->linoff += 3;
            int tail = (unsigned char)*lexer->cursor;
            if (isprint(tail)
                && (isalpha(tail)
                    || isdigit(tail))) {
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                return NULL;
            } else {
                return cbor_init_double(dbl);
            }
        } else {
            lexer->last_error = JSON_ERR_CHARACTER_SEQUENCE;
            return NULL;
        }
    } else if (leader == '-' || isdigit(leader)) {
        return json_parse_number(lexer);
    } else if (leader == 'i' || leader == 'I') {
        char *end;
        if (!strncasecmp(lexer->cursor, "infinity", 8)) {
            double dbl = strtod(lexer->cursor, &end);
            lexer->cursor += 8;
            lexer->linoff += 8;
            int tail = (unsigned char)*lexer->cursor;
            if (isprint(tail)
                && (isalpha(tail)
                    || isdigit(tail))) {
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                return NULL;
            } else {
                return cbor_init_double(dbl);
            }
        } else if (!strncasecmp(lexer->cursor, "inf", 3)) {
            double dbl = strtod(lexer->cursor, &end);
            lexer->cursor += 3;
            lexer->linoff += 3;
            int tail = (unsigned char)*lexer->cursor;
            if (isprint(tail)
                && (isalpha(tail)
                    || isdigit(tail))) {
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                return NULL;
            } else {
                return cbor_init_double(dbl);
            }
        } else {
            lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
            return NULL;
        }
    } else if (leader == 'N') {
        if (!strncasecmp(lexer->cursor, "nan", 3)) {
            double dbl = strtod(lexer->cursor, NULL);
            lexer->cursor += 3;
            lexer->linoff += 3;
            int tail = (unsigned char)*lexer->cursor;
            if (isprint(tail)
                && (isalpha(tail)
                    || isdigit(tail))) {
                lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
                return NULL;
            } else {
                return cbor_init_double(dbl);
            }
        } else {
            lexer->last_error = JSON_ERR_CHARACTER_SEQUENCE;
            return NULL;
        }
    } else {
        lexer->last_error = JSON_ERR_UNEXPECTED_CHARACTER;
        return NULL;
    }
}

cbor_value_t *cbor_json_loads(const void *src, int size) {
    lexer_t lexer;
    cbor_value_t *json;
    memset(&lexer, 0, sizeof(lexer));
    lexer.source = src;
    if (!src) {
        return NULL;
    }
    if (size < 0) {
        size = strlen(src);
    }
    lexer.eof = src + size;
    lexer.cursor = lexer.source;
    lexer.linst = lexer.source;
    lexer.linum = 1;
    lexer.linoff = 0;
    json = json_parse_value(&lexer);
    if (lexer.last_error != JSON_ERR_NONE) {
        json_lexer_error(&lexer);
    }
    return json;
}

cbor_value_t *cbor_json_loadss(const void *src, size_t *size) {
    lexer_t lexer;
    cbor_value_t *json;
    memset(&lexer, 0, sizeof(lexer));
    lexer.source = src;
    if (!src) {
        return NULL;
    }
    lexer.eof = src + *size;
    lexer.cursor = lexer.source;
    lexer.linst = lexer.source;
    lexer.linum = 1;
    lexer.linoff = 0;
    json = json_parse_value(&lexer);
    if (lexer.last_error != JSON_ERR_NONE) {
        json_lexer_error(&lexer);
    }
    if (json == NULL) {
        *size = 0;
    } else {
        *size = lexer.cursor - lexer.source;
    }
    return json;
}

void json__dumps(const cbor_value_t *src, int indent, const char *space, int length, cbor_value_t *dst) {
    int i;
    char buffer[1024];
    cbor_value_t *val;
    if (cbor_is_integer(src)) {
        int len = snprintf(buffer, sizeof(buffer), "%lld", cbor_integer(src));
        cbor_blob_append(dst, buffer, len);
    } else if (cbor_is_string(src)) {
        cbor_blob_append_byte(dst, '"');
        const char *ptr = cbor_string(src);
        int length = cbor_string_size(src);

        for (i = 0; i < length; i++) {
            switch ((unsigned char)ptr[i]) {
            case '\n':
                cbor_blob_append_byte(dst, '\\');
                cbor_blob_append_byte(dst, 'n');
                break;
            case '\t':
                cbor_blob_append_byte(dst, '\\');
                cbor_blob_append_byte(dst, 't');
                break;
            case '\\':
                cbor_blob_append_byte(dst, '\\');
                cbor_blob_append_byte(dst, '\\');
                break;
            /* case '/': */
            /*     cbor_blob_append_byte(dst, '\\'); */
            /*     cbor_blob_append_byte(dst, '/'); */
            /*     break; */
            case '"':
                cbor_blob_append_byte(dst, '\\');
                cbor_blob_append_byte(dst, '"');
                break;
            case '\r':
                cbor_blob_append_byte(dst, '\\');
                cbor_blob_append_byte(dst, 'r');
                break;
            case '\f':
                cbor_blob_append_byte(dst, '\\');
                cbor_blob_append_byte(dst, 'f');
                break;
            default: {
                int codepoint = -1;
                if ((unsigned char)ptr[i] <= 0x7F) {
                    /* 0xxxxxxx */
                    codepoint = (unsigned char)ptr[i];
                } else if ((unsigned char)ptr[i] >= 0xC0
                           && (unsigned char)ptr[i] <= 0xDF) {
                    /* 110xxxxx 10xxxxxx */
                    codepoint = (unsigned char)ptr[i] & 0x1F;
                    codepoint <<= 6;
                    if ((unsigned char)ptr[i + 1] >= 0x80
                        && (unsigned char)ptr[i + 1] <= 0xBF) {
                        codepoint |= (unsigned char)ptr[i + 1] & 0x3F;
                    } else {
                        codepoint = -1;
                        fprintf(stderr, "dump utf-8 code point fail 0x%02x, offset %d", ptr[i + 1], i + 1);
                    }
                    i += 1;
                } else if ((unsigned char)ptr[i] >= 0xE0
                           && (unsigned char)ptr[i] <= 0xEF) {
                    /* 1110xxxx 10xxxxxx 10xxxxxx */
                    codepoint = (unsigned char)ptr[i] & 0xF;
                    codepoint <<= 12;
                    if ((unsigned char)ptr[i + 1] >= 0x80
                        && (unsigned char)ptr[i + 1] <= 0xBF) {
                        codepoint |= ((unsigned char)ptr[i + 1] & 0x3F) << 6;
                        if ((unsigned char)ptr[i + 2] >= 0x80
                            && (unsigned char)ptr[i + 2] <= 0xBF) {
                            codepoint |= (unsigned char)ptr[i + 2] & 0x3F;
                        } else {
                            codepoint = -1;
                            fprintf(stderr, "dump utf-8 code point fail 0x%02x, offset %d", ptr[i + 2], i + 2);
                        }
                    } else {
                        codepoint = -1;
                        fprintf(stderr, "dump utf-8 code point fail 0x%02x, offset %d", ptr[i + 1], i + 1);
                    }
                    i += 2;
                } else if ((unsigned char)ptr[i] >= 0xF0
                           && (unsigned char)ptr[i] <= 0xF7) {
                    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
                    codepoint = (unsigned char)ptr[i] & 0x7;
                    codepoint <<= 18;
                    if ((unsigned char)ptr[i + 1] >= 0x80
                        && (unsigned char)ptr[i + 1] <= 0xBF) {
                        codepoint |= ((unsigned char)ptr[i + 1] & 0x3F) << 12;
                        if ((unsigned char)ptr[i + 2] >= 0x80
                            && (unsigned char)ptr[i + 2] <= 0xBF) {
                            codepoint |= ((unsigned char)ptr[i + 2] & 0x3F) << 6;
                            if ((unsigned char)ptr[i + 3] >= 0x80
                                && (unsigned char)ptr[i + 3] <= 0xBF) {
                                codepoint |= (unsigned char)ptr[i + 3] & 0x3F;
                            } else {
                                codepoint = -1;
                                fprintf(stderr, "dump utf-8 code point fail 0x%02x, offset %d", ptr[i + 3], i + 3);
                            }
                        } else {
                            codepoint = -1;
                            fprintf(stderr, "dump utf-8 code point fail 0x%02x, offset %d", ptr[i + 2], i + 2);
                        }
                    } else {
                        codepoint = -1;
                        fprintf(stderr, "dump utf-8 code point fail 0x%02x, offset %d", ptr[i + 1], i + 1);
                    }
                    i += 3;
                }
                if (codepoint != -1) {
                    if (codepoint <= 0x7F) {
                        if (isprint(codepoint)) {
                            cbor_blob_append_byte(dst, codepoint);
                        } else {
                            int len = snprintf(buffer, sizeof(buffer), "\\u%04x", codepoint);
                            cbor_blob_append(dst, buffer, len);
                        }
                    } else if (codepoint <= 0xD7FF || (codepoint >= 0xE000 && codepoint <= 0xFFFF)) {
                        int len = snprintf(buffer, sizeof(buffer), "\\u%04x", codepoint);
                        cbor_blob_append(dst, buffer, len);
                    } else if (codepoint <= 0x10FFFF) {
                        codepoint -= 0x10000;
                        int len = snprintf(buffer, sizeof(buffer), "\\u%04x", ((codepoint >> 10) & 0x3FF) | 0xD800);
                        cbor_blob_append(dst, buffer, len);
                        len = snprintf(buffer, sizeof(buffer), "\\u%04x", (codepoint & 0x3FF) | 0xDC00);
                        cbor_blob_append(dst, buffer, len);
                    } else {
                        fprintf(stderr, "invalid codepoint: 0x%08x, offset %d", codepoint, i);
                    }
                } else {
                    fprintf(stderr, "dump string character fail, offset %d", i);
                }
            }
            }
        }
        cbor_blob_append_byte(dst, '"');
    } else if (cbor_is_array(src)) {
        cbor_blob_append_byte(dst, '[');
        if (space) {
            cbor_blob_append_byte(dst, '\n');
        }
        indent++;
        for (val = cbor_container_first(src);
             val != NULL;
             val = cbor_container_next(src, val)) {
            if (space) {
                for (i = 0; i < indent; i++) {
                    cbor_blob_append(dst, space, length);
                }
            }
            json__dumps(val, indent, space, length, dst);
            if (cbor_container_next(src, val)) {
                cbor_blob_append_byte(dst, ',');
                if (!space) {
                    cbor_blob_append_byte(dst, ' ');
                }
            }
            if (space) {
                cbor_blob_append_byte(dst, '\n');
            }
        }
        indent--;
        if (space) {
            for (i = 0; i < indent; i++) {
                cbor_blob_append(dst, space, length);
            }
        }
        cbor_blob_append_byte(dst, ']');
    } else if (cbor_is_map(src)) {
        cbor_blob_append_byte(dst, '{');
        if (space) {
            cbor_blob_append_byte(dst, '\n');
        }
        indent++;
        for (val = cbor_container_first(src);
             val != NULL;
             val = cbor_container_next(src, val)) {
            if (space) {
                for (i = 0; i < indent; i++) {
                    cbor_blob_append(dst, space, length);
                }
            }
            json__dumps(cbor_pair_key(val), indent, space, length, dst);
            cbor_blob_append_byte(dst, ':');
            cbor_blob_append_byte(dst, ' ');
            json__dumps(cbor_pair_value(val), indent, space, length, dst);
            if (cbor_container_next(src, val)) {
                cbor_blob_append_byte(dst, ',');
                if (!space) {
                    cbor_blob_append_byte(dst, ' ');
                }
            }
            if (space) {
                cbor_blob_append_byte(dst, '\n');
            }
        }
        indent--;
        if (space) {
            for (i = 0; i < indent; i++) {
                cbor_blob_append(dst, space, length);
            }
        }
        cbor_blob_append_byte(dst, '}');
    } else if (cbor_is_double(src)) {
        int len;
        if (isinf(cbor_real(src)) || isnan(cbor_real(src))) {
            len = snprintf(buffer, sizeof(buffer), "%lf", .0);
        } else {
            len = snprintf(buffer, sizeof(buffer), "%lf", cbor_real(src));
        }
        cbor_blob_append(dst, buffer, len);
    } else if (cbor_is_null(src)) {
        cbor_blob_append(dst, "null", 4);
    } else if (cbor_is_boolean(src)) {
        if (cbor_boolean(src)) {
            cbor_blob_append(dst, "true", 4);
        } else {
            cbor_blob_append(dst, "false", 5);
        }
    } else {
        fprintf(stderr, "unsupported cbor type (%s) dump to json\n", cbor_type_str(src));
    }
}

char *cbor_json_dumps(const cbor_value_t *src, size_t *length, bool pretty) {
    char *ptr = NULL;
    *length = 0;
    if (src) {
        cbor_value_t *dst = cbor_init_string("", 0);
        if (pretty) {
            json__dumps(src, 0, "    ", 4, dst);
        } else {
            json__dumps(src, 0, NULL, 0, dst);
        }
        cbor_blob_replace(dst, &ptr, length);
        ptr[*length] = 0;
        cbor_destroy(dst);
    }
    return ptr;
}

cbor_value_t *cbor_json_loadf(const char *path) {
    char *content;
    size_t length;
    cbor_value_t *val = NULL;
    FILE *fp = fopen(path, "r");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        content = (char *)malloc(length + 1);
        if (length == fread(content, sizeof(char), length, fp)) {
            content[length] = 0;
            val = cbor_json_loads(content, length);
        }
        free(content);
        fclose(fp);
    }
    return val;
}

int cbor_json_dumpf(cbor_value_t *val, const char *path, bool pretty) {
    FILE *fp;
    size_t length;
    int r = -1;
    char *content = cbor_json_dumps(val, &length, pretty);
    if (content && length) {
        fp = fopen(path, "w");
        if (length != fwrite(content, sizeof(char), length, fp)) {
            r = -1;
        } else {
            r = 0;
        }
        fclose(fp);
        free(content);
    }
    return r;
}
