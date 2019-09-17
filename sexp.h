// SPDX-FileCopyrightText: 2019 Lassi Kortela
// SPDX-License-Identifier: ISC

struct sexp;

size_t sexp_nbyte(struct sexp *sexp);
void *sexp_bytes(struct sexp *sexp);
char *sexp_strdup(struct sexp *sexp);

int sexp_is_null(struct sexp *sexp);
int sexp_is_false(struct sexp *sexp);
int sexp_is_true(struct sexp *sexp);
struct sexp *sexp_new_null(void);
struct sexp *sexp_new_bool(int value);

int sexp_is_pair(struct sexp *sexp);
struct sexp *sexp_new_pair(struct sexp *head, struct sexp *tail);
struct sexp *sexp_head(struct sexp *sexp);
struct sexp *sexp_tail(struct sexp *sexp);
struct sexp *sexp_set_tail(struct sexp *sexp, struct sexp *tail);

int sexp_is_list(struct sexp *sexp);
struct sexp *sexp_list_tail(struct sexp *sexp, size_t n);
struct sexp *sexp_list_ref(struct sexp *sexp, size_t n);
size_t sexp_list_len_bounded(struct sexp *sexp, size_t maxlen);
size_t sexp_list_len(struct sexp *sexp);

int sexp_is_symbol(struct sexp *sexp);
int sexp_is_symbol_name(struct sexp *sexp, const char *str);
struct sexp *sexp_new_symbol(const char *str);
struct sexp *sexp_new_symbol_zeros(size_t nbyte);
struct sexp *sexp_new_symbol_bytes(void *bytes, size_t nbyte);

int sexp_is_string(struct sexp *sexp);
struct sexp *sexp_new_string(const char *str);
struct sexp *sexp_new_string_zeros(size_t nbyte);
struct sexp *sexp_new_string_bytes(void *bytes, size_t nbyte);

int sexp_is_bytevector(struct sexp *sexp);
struct sexp *sexp_new_bytevector_zeros(size_t nbyte);
struct sexp *sexp_new_bytevector_bytes(void *bytes, size_t nbyte);

int sexp_is_vector(struct sexp *sexp);
struct sexp *sexp_new_vector(size_t len);
size_t sexp_vector_len(struct sexp *sexp);
struct sexp *sexp_vector_ref(struct sexp *sexp, size_t idx);
int sexp_vector_set(struct sexp *sexp, size_t idx, struct sexp *elt);

int sexp_is_int64(struct sexp *sexp);
struct sexp *sexp_new_int64(int64_t value);
int sexp_int64_value(struct sexp *sexp);

void sexp_free_only(struct sexp *sexp);
void sexp_free(struct sexp *sexp);
