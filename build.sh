#!/bin/sh
set -eu
cd "$(dirname "$0")"

CC=${CC:-clang}
CFLAGS=${CFLAGS:--Wall -Wextra -Werror -std=gnu99}
LFLAGS=${LFLAGS:-}

CFLAGS_SQLITE3=${CFLAGS_SQLITE3:-$(pkg-config --cflags sqlite3)}
LFLAGS_SQLITE3=${LFLAGS_SQLITE3:-$(pkg-config --libs sqlite3)}

CFLAGS_PQ=${CFLAGS_PQ:-$(pkg-config --cflags libpq)}
LFLAGS_PQ=${LFLAGS_PQ:-$(pkg-config --libs libpq)}

set -x

$CC $CFLAGS -I . -c sexp.c
$CC $CFLAGS -I . -c sexp_binary_read.c
$CC $CFLAGS -I . -c sexp_binary_write.c
$CC $CFLAGS -I . -c sexp_binary_pipe.c

$CC $CFLAGS $CFLAGS_SQLITE3 -I . -c driver-sqlite.c
$CC $LFLAGS $LFLAGS_SQLITE3 -o driver-sqlite \
    sexp.o \
    sexp_binary_read.o \
    sexp_binary_write.o \
    sexp_binary_pipe.o \
    driver-sqlite.o
