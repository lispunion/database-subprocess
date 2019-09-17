struct sexp_binary_read;
struct sexp_binary_write;

void *sexp_binary_pipe(struct sexp_binary_read **out_rd,
                       struct sexp_binary_write **out_wr);
