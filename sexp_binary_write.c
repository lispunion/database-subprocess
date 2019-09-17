// SPDX-FileCopyrightText: 2019 Lassi Kortela
// SPDX-License-Identifier: ISC

#include <stdlib.h>
#include <string.h>

#include "sexp.h"
#include "sexp_binary_write.h"

struct sexp_binary_write {
    void *(*write)(void *port, void *bytes, size_t nbyte);
    void *(*flush)(void *port);
    void *port;
    void *error;
};

int write_nested(struct sexp_binary_write *wr, struct sexp *sexp);

struct sexp_binary_write *
sexp_binary_write_new(void *(*write)(void *, void *, size_t),
                      void *(*flush)(void *), void *port)
{
    struct sexp_binary_write *wr;

    if (!(wr = calloc(1, sizeof(*wr)))) {
        return 0;
    }
    wr->write = write;
    wr->flush = flush;
    wr->port = port;
    return wr;
}

void *sexp_binary_write_error(struct sexp_binary_write *wr)
{
    return wr->error;
}

void sexp_binary_write_free(struct sexp_binary_write *wr) { free(wr); }

static int write_rawuint64(struct sexp_binary_write *wr, uint64_t value)
{
    unsigned char buffer[10];
    unsigned char *const limit = buffer + sizeof(buffer);
    unsigned char *bytes = limit;

    while (value > 0x7f) {
        *--bytes = 0x80 | (value & 0x7f);
        value >>= 7;
    }
    *--bytes = value;
    wr->error = wr->write(wr->port, bytes, limit - bytes);
    return !wr->error;
}

static int write_rawsize(struct sexp_binary_write *wr, size_t value)
{
    return write_rawuint64(wr, (uint64_t)value);
}

static int write_tagged_bytes(struct sexp_binary_write *wr, size_t tag,
                              struct sexp *sb)
{
    size_t n;

    if (!write_rawsize(wr, tag)) {
        return 0;
    }
    n = sexp_nbyte(sb);
    if (!write_rawsize(wr, n)) {
        return 0;
    }
    wr->error = wr->write(wr->port, sexp_bytes(sb), n);
    return !wr->error;
}

static int write_tagged_vector(struct sexp_binary_write *wr, size_t tag,
                               struct sexp *vec)
{
    size_t n, i;

    if (!write_rawsize(wr, tag)) {
        return 0;
    }
    n = sexp_vector_len(vec);
    if (!write_rawsize(wr, n)) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (!write_nested(wr, sexp_vector_ref(vec, i))) {
            return 0;
        }
    }
    return 1;
}

static int write_tagged_uint64(struct sexp_binary_write *wr, size_t tag,
                               uint64_t value)
{
    if (!write_rawsize(wr, tag)) {
        return 0;
    }
    return write_rawuint64(wr, value);
}

int write_nested(struct sexp_binary_write *wr, struct sexp *sexp)
{
    if (sexp_is_null(sexp)) {
        return write_rawsize(wr, 0);
    }
    if (sexp_is_false(sexp)) {
        return write_rawsize(wr, 1);
    }
    if (sexp_is_true(sexp)) {
        return write_rawsize(wr, 2);
    }
    if (sexp_is_bytevector(sexp)) {
        return write_tagged_bytes(wr, 3, sexp);
    }
    if (sexp_is_int64(sexp)) {
        int64_t value = sexp_int64_value(sexp);
        if (value >= 0) {
            return write_tagged_uint64(wr, 4, value);
        } else {
            return write_tagged_uint64(wr, 5, -value);
        }
    }
    if (sexp_is_pair(sexp)) {
        if (!write_rawsize(wr, 0xc))
            return 0;
        if (!write_nested(wr, sexp_head(sexp)))
            return 0;
        if (!write_nested(wr, sexp_tail(sexp)))
            return 0;
        return 1;
    }
    if (sexp_is_vector(sexp)) {
        return write_tagged_vector(wr, 0xd, sexp);
    }
    if (sexp_is_string(sexp)) {
        return write_tagged_bytes(wr, 0xe, sexp);
    }
    if (sexp_is_symbol(sexp)) {
        return write_tagged_bytes(wr, 0xf, sexp);
    }
    wr->error = "do not know how to write that kind of object";
    return 0;
}

int sexp_binary_write(struct sexp_binary_write *wr, struct sexp *sexp)
{
    if (!write_nested(wr, sexp)) {
        return 0;
    }
    wr->error = wr->flush(wr->port);
    return !wr->error;
}
