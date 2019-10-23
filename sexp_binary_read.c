// SPDX-FileCopyrightText: 2019 Lassi Kortela
// SPDX-License-Identifier: ISC

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>  // TODO: only for debug prints

#include "sexp.h"
#include "sexp_binary_read.h"

struct sexp_binary_read {
    void *(*read)(void *port, void *bytes, size_t nbyte);
    void *port;
    void *error;
};

struct sexp_binary_read *
sexp_binary_read_new(void *(*read)(void *, void *, size_t), void *port)
{
    struct sexp_binary_read *rd;

    if (!(rd = calloc(1, sizeof(*rd)))) {
        return 0;
    }
    rd->read = read;
    rd->port = port;
    return rd;
}

void *sexp_binary_read_error(struct sexp_binary_read *rd)
{
    return rd->error;
}

void sexp_binary_read_free(struct sexp_binary_read *rd) { free(rd); }

static int read_rawsize(struct sexp_binary_read *rd, size_t *out,
                        size_t minval, size_t maxval)
{
    size_t value, shift;
    unsigned char byte;

    *out = value = shift = 0;
    for (;;) {
        if ((rd->error = rd->read(rd->port, &byte, 1))) {
            return 0;
        }
        value |= (byte & 0x7f) << shift;
        if (!(byte & 0x80)) {
            break;
        }
        shift += 7;
        if (shift >= sizeof(size_t) * CHAR_BIT - 7) {
            rd->error = "number too big to represent";
            return 0;
        }
    }
    if (value < minval) {
        rd->error = "number smaller than expected";
        return 0;
    }
    if (value > maxval) {
        rd->error = "number bigger than expected";
        return 0;
    }
    *out = value;
    return 1;
}

static struct sexp *read_pair(struct sexp_binary_read *rd)
{
    struct sexp *head;
    struct sexp *tail;

    if (!sexp_binary_read(rd, &head))
        return 0;
    if (!sexp_binary_read(rd, &tail)) {
        sexp_free(head);
        return 0;
    }
    return sexp_new_pair(head, tail);
}

static struct sexp *read_vector(struct sexp_binary_read *rd)
{
    struct sexp *vec;
    struct sexp *elt;
    size_t i, n;

    if (!read_rawsize(rd, &n, 0, SIZE_MAX)) {
        return 0;
    }
    vec = sexp_new_vector(n);
    for (i = 0; i < n; i++) {
        if (!sexp_binary_read(rd, &elt)) {
            sexp_free(vec);
            return 0;
        }
        sexp_vector_set(vec, i, elt);
    }
    return vec;
}

static struct sexp *read_bytevector(struct sexp_binary_read *rd)
{
    struct sexp *bv;
    size_t n;

    if (!read_rawsize(rd, &n, 0, SIZE_MAX)) {
        return 0;
    }
    bv = sexp_new_bytevector_zeros(n);
    if ((rd->error = rd->read(rd->port, sexp_bytes(bv), n))) {
        return 0;
    }
    return bv;
}

static struct sexp *read_string(struct sexp_binary_read *rd)
{
    struct sexp *bv;
    size_t n;

    if (!read_rawsize(rd, &n, 0, SIZE_MAX)) {
        return 0;
    }
    bv = sexp_new_string_zeros(n);
    if ((rd->error = rd->read(rd->port, sexp_bytes(bv), n))) {
        return 0;
    }
    return bv;
}

static struct sexp *read_symbol(struct sexp_binary_read *rd)
{
    struct sexp *bv;
    size_t n;

    if (!read_rawsize(rd, &n, 0, SIZE_MAX)) {
        return 0;
    }
    bv = sexp_new_symbol_zeros(n);
    if ((rd->error = rd->read(rd->port, sexp_bytes(bv), n))) {
        return 0;
    }
    return bv;
}

int sexp_binary_read(struct sexp_binary_read *rd, struct sexp **out)
{
    size_t tag;

    if (!read_rawsize(rd, &tag, 0, SIZE_MAX)) {
        return 0;
    }
    switch (tag) {
    case 0x0:
        *out = sexp_new_null();
        return 1;
    case 0x1:
        *out = sexp_new_bool(0);
        break;
    case 0x2:
        *out = sexp_new_bool(1);
        break;
    case 0x3:
        *out = read_bytevector(rd);
        break;
    case 0x4: {
        size_t val;

        if (!read_rawsize(rd, &val, 0, SIZE_MAX)) {
            return 0;
        }
        *out = sexp_new_int64((int64_t)val);
        break;
    }
    case 0x5: {
        size_t val;

        if (!read_rawsize(rd, &val, 0, SIZE_MAX)) {
            return 0;
        }
        *out = sexp_new_int64(-(int64_t)val);
        break;
    }
    case 0xc:
        *out = read_pair(rd);
        break;
    case 0xd:
        *out = read_vector(rd);
        break;
    case 0xe:
        *out = read_string(rd);
        break;
    case 0xf:
        *out = read_symbol(rd);
        break;
    default:
        rd->error = "unknown type tag";
        fprintf(stderr, "unknown type tag #x%02zx\n", tag);
        return 0;  // TODO
    }
    return !!*out;
}
