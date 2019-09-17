// SPDX-FileCopyrightText: 2019 Lassi Kortela
// SPDX-License-Identifier: ISC

struct sexp_binary_read;

struct sexp_binary_read *
sexp_binary_read_new(void *(*read)(void *, void *, size_t), void *port);
void *sexp_binary_read_error(struct sexp_binary_read *rd);
void sexp_binary_read_free(struct sexp_binary_read *rd);
int sexp_binary_read(struct sexp_binary_read *rd, struct sexp **out);
