// SPDX-FileCopyrightText: 2019 Lassi Kortela
// SPDX-License-Identifier: ISC

#include <stdlib.h>
#include <string.h>

#define SEXP_NULL 0
#define SEXP_FALSE 1
#define SEXP_TRUE 2

#define SEXP_PAIR 3
#define SEXP_VECTOR 4

#define SEXP_INT64 5

#define SEXP_SYMBOL 6
#define SEXP_STRING 7
#define SEXP_BYTEVECTOR 8

#define SEXP_TYPE_BITS 4
#define SEXP_TYPE_MASK 15
#define SEXP_TYPE_BYTES_START SEXP_SYMBOL

struct sexp {
    uintptr_t bits;
};

struct sexp_pair {
    uintptr_t bits;
    struct sexp *head;
    struct sexp *tail;
};

struct sexp_vector {
    uintptr_t bits;
    struct sexp *elts[];
};

struct sexp_bytes {
    uintptr_t bits;
    char bytes[];
};

struct sexp_int64 {
    uintptr_t bits;
    int64_t value;
};

static uintptr_t sexp_type(struct sexp *sexp)
{
    return sexp ? (sexp->bits & SEXP_TYPE_MASK) : SEXP_NULL;
}

void *sexp_bytes(struct sexp *sexp)
{
    if (sexp_type(sexp) < SEXP_TYPE_BYTES_START) {
        return 0;
    }
    return ((struct sexp_bytes *)sexp)->bytes;
}

size_t sexp_nbyte(struct sexp *sexp)
{
    if (sexp_type(sexp) < SEXP_TYPE_BYTES_START) {
        return 0;
    }
    return sexp->bits >> SEXP_TYPE_BITS;
}

char *sexp_strdup(struct sexp *sexp)
{
    char *bytes;
    char *str;
    size_t len;

    if (!(bytes = sexp_bytes(sexp))) {
        return 0;
    }
    len = sexp_nbyte(sexp);

    if (memchr(bytes, 0, len)) {
        return 0;  // null bytes cannot appear in a null-terminated string
    }
    if (!(str = calloc(len + 1, 1))) {
        return 0;
    }
    memcpy(str, bytes, len);
    return str;
}

int sexp_is_null(struct sexp *sexp) { return !sexp; }

int sexp_is_false(struct sexp *sexp) { return sexp_type(sexp) == SEXP_FALSE; }

int sexp_is_true(struct sexp *sexp) { return sexp_type(sexp) == SEXP_TRUE; }

struct sexp *sexp_new_null(void) { return 0; }

struct sexp *sexp_new_bool(int value)
{
    struct sexp *sexp;

    if (!(sexp = calloc(1, sizeof(*sexp)))) {
        return 0;
    }
    sexp->bits = value ? SEXP_TRUE : SEXP_FALSE;
    return sexp;
}

int sexp_is_pair(struct sexp *sexp) { return sexp_type(sexp) == SEXP_PAIR; }

struct sexp *sexp_new_pair(struct sexp *head, struct sexp *tail)
{
    struct sexp_pair *pair;

    if (!(pair = calloc(1, sizeof(*pair)))) {
        return 0;
    }
    pair->bits = SEXP_PAIR;
    pair->head = head;
    pair->tail = tail;
    return (struct sexp *)pair;
}

struct sexp *sexp_head(struct sexp *sexp)
{
    return sexp_is_pair(sexp) ? ((struct sexp_pair *)sexp)->head : 0;
}

struct sexp *sexp_tail(struct sexp *sexp)
{
    return sexp_is_pair(sexp) ? ((struct sexp_pair *)sexp)->tail : 0;
}

struct sexp *sexp_set_tail(struct sexp *sexp, struct sexp *tail)
{
    if (!sexp_is_pair(sexp)) {
        return 0;
    }
    ((struct sexp_pair *)sexp)->tail = tail;
    return tail;
}

int sexp_is_list(struct sexp *sexp)
{
    for (; !sexp_is_null(sexp); sexp = sexp_tail(sexp)) {
        if (!sexp_is_pair(sexp))
            return 0;
    }
    return 1;
}

struct sexp *sexp_list_tail(struct sexp *sexp, size_t n)
{
    size_t i;

    i = 0;
    for (; !sexp_is_null(sexp); sexp = sexp_tail(sexp)) {
        if (i == n)
            break;
        if (!sexp_is_pair(sexp))
            return 0;
        i++;
    }
    return sexp;
}

struct sexp *sexp_list_ref(struct sexp *sexp, size_t n)
{
    return sexp_head(sexp_list_tail(sexp, n));
}

size_t sexp_list_len_bounded(struct sexp *sexp, size_t maxlen)
{
    size_t n;

    n = 0;
    for (; !sexp_is_null(sexp); sexp = sexp_tail(sexp)) {
        if (!sexp_is_pair(sexp))
            return 0;
        if (n == maxlen)
            break;
        n++;
    }
    return n;
}

size_t sexp_list_len(struct sexp *sexp)
{
    return sexp_list_len_bounded(sexp, SIZE_MAX);
}

static void *sexp_new_bytes_zeros(size_t bits, size_t nbyte)
{
    struct sexp_bytes *sb;

    if (!(sb = calloc(1, nbyte + sizeof(*sb)))) {
        return 0;
    }
    sb->bits = bits | (nbyte << SEXP_TYPE_BITS);
    return sb;
}

static void *sexp_new_bytes(size_t bits, const void *bytes, size_t nbyte)
{
    struct sexp_bytes *sb;

    if (!(sb = sexp_new_bytes_zeros(bits, nbyte))) {
        return 0;
    }
    memcpy(sb->bytes, bytes, nbyte);
    return sb;
}

int sexp_is_symbol(struct sexp *sexp)
{
    return sexp_type(sexp) == SEXP_SYMBOL;
}

int sexp_is_symbol_name(struct sexp *sexp, const char *name)
{
    size_t len;

    if (!sexp_is_symbol(sexp))
        return 0;
    len = strlen(name);
    if (len != sexp_nbyte(sexp))
        return 0;
    return !memcmp(name, sexp_bytes(sexp), len);
}

struct sexp *sexp_new_symbol(const char *str)
{
    return sexp_new_bytes(SEXP_SYMBOL, str, strlen(str));
}

struct sexp *sexp_new_symbol_zeros(size_t nbyte)
{
    return sexp_new_bytes_zeros(SEXP_SYMBOL, nbyte);
}

struct sexp *sexp_new_symbol_bytes(const void *bytes, size_t nbyte)
{
    return sexp_new_bytes(SEXP_SYMBOL, bytes, nbyte);
}

int sexp_is_string(struct sexp *sexp)
{
    return sexp_type(sexp) == SEXP_STRING;
}

struct sexp *sexp_new_string(const char *str)
{
    return sexp_new_bytes(SEXP_STRING, str, strlen(str));
}

struct sexp *sexp_new_string_zeros(size_t nbyte)
{
    return sexp_new_bytes_zeros(SEXP_STRING, nbyte);
}

struct sexp *sexp_new_string_bytes(const void *bytes, size_t nbyte)
{
    return sexp_new_bytes(SEXP_STRING, bytes, nbyte);
}

int sexp_is_bytevector(struct sexp *sexp)
{
    return sexp_type(sexp) == SEXP_BYTEVECTOR;
}

struct sexp *sexp_new_bytevector_zeros(size_t nbyte)
{
    return sexp_new_bytes_zeros(SEXP_BYTEVECTOR, nbyte);
}

struct sexp *sexp_new_bytevector_bytes(const void *bytes, size_t nbyte)
{
    return sexp_new_bytes(SEXP_BYTEVECTOR, bytes, nbyte);
}

int sexp_is_vector(struct sexp *sexp)
{
    return sexp_type(sexp) == SEXP_VECTOR;
}

struct sexp *sexp_new_vector(size_t len)
{
    struct sexp *sexp;

    if (len == SIZE_MAX) {
        return 0;
    }
    if (!(sexp = calloc(1 + len, sizeof(struct sexp *)))) {
        return 0;
    }
    sexp->bits = SEXP_VECTOR | (len << SEXP_TYPE_BITS);
    return sexp;
}

size_t sexp_vector_len(struct sexp *sexp)
{
    return sexp_is_vector(sexp) ? sexp->bits >> SEXP_TYPE_BITS : 0;
}

struct sexp *sexp_vector_ref(struct sexp *sexp, size_t idx)
{
    if (idx < sexp_vector_len(sexp)) {
        return ((struct sexp_vector *)sexp)->elts[idx];
    }
    return 0;
}

int sexp_vector_set(struct sexp *sexp, size_t idx, struct sexp *elt)
{
    if (idx < sexp_vector_len(sexp)) {
        ((struct sexp_vector *)sexp)->elts[idx] = elt;
        return 1;
    }
    return 0;
}

int sexp_is_int64(struct sexp *sexp) { return sexp_type(sexp) == SEXP_INT64; }

struct sexp *sexp_new_int64(int64_t value)
{
    struct sexp_int64 *int64;

    if (!(int64 = calloc(1, sizeof(*int64)))) {
        return 0;
    }
    int64->bits = SEXP_INT64;
    int64->value = value;
    return (struct sexp *)int64;
}

int sexp_int64_value(struct sexp *sexp)
{
    return sexp_is_int64(sexp) ? ((struct sexp_int64 *)sexp)->value : 0;
}

void sexp_free_only(struct sexp *sexp) { free(sexp); }

// Does not detect shared structure!
void sexp_free(struct sexp *sexp)
{
    size_t n;

    switch (sexp_type(sexp)) {
    case SEXP_PAIR:
        sexp_free(((struct sexp_pair *)sexp)->head);
        sexp_free(((struct sexp_pair *)sexp)->tail);
        break;
    case SEXP_VECTOR:
        for (n = sexp_vector_len(sexp); n;) {
            sexp_free(((struct sexp_vector *)sexp)->elts[--n]);
        }
        break;
    }
}
