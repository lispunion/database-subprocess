// SPDX-FileCopyrightText: 2019 Lassi Kortela
// SPDX-License-Identifier: ISC

struct sexp_binary_write;

struct sexp_binary_write *
sexp_binary_write_new(void *(*write)(void *, void *, size_t),
                      void *(*flush)(void *), void *port);
void *sexp_binary_write_error(struct sexp_binary_write *wr);
void sexp_binary_write_free(struct sexp_binary_write *wr);
int sexp_binary_write(struct sexp_binary_write *wr, struct sexp *sexp);
