#include <stdio.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifdef __BORLANDC__
#define _setmode setmode
#endif

#include <sexp.h>
#include <sexp_binary_read.h>
#include <sexp_binary_write.h>

#ifndef _WIN32
static void *binary_stdio(void)
{
    if (isatty(fileno(stdin))) {
        return "standard input is a terminal";
    }
    if (isatty(fileno(stdout))) {
        return "standard output is a terminal";
    }
    return 0;
}
#endif

#ifdef _WIN32
static void *binary_stdio(void)
{
    if (_isatty(_fileno(stdin))) {
        return "standard input is a terminal";
    }
    if (_isatty(_fileno(stdout))) {
        return "standard output is a terminal";
    }
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    return 0;
}
#endif

static void *read_from_file(void *file_void, void *bytes, size_t nbyte)
{
    FILE *file = file_void;

    if (fread(bytes, 1, nbyte, file) == nbyte) {
        return 0;
    }
    return ferror(file) ? "read error" : "did not read enough data";
}

static void *write_to_file(void *file_void, void *bytes, size_t nbyte)
{
    FILE *file = file_void;

    if (fwrite(bytes, 1, nbyte, file) == nbyte) {
        return 0;
    }
    return ferror(file) ? "write error" : "did not write enough data";
}

static void *flush_file(void *file)
{
    fflush((FILE *)file);
    return ferror(file) ? "write error" : 0;
}

void *sexp_binary_pipe(struct sexp_binary_read **out_rd,
                       struct sexp_binary_write **out_wr)
{
    void *error;

    if ((error = binary_stdio())) {
        return error;
    }
    if (!(*out_rd = sexp_binary_read_new(read_from_file, stdin))) {
        return "out of memory";
    }
    if (!(*out_wr =
          sexp_binary_write_new(write_to_file, flush_file, stdout))) {
        sexp_binary_read_free(*out_rd);
        return "out of memory";
    }
    return 0;
}
