// SPDX-FileCopyrightText: 2019 Lassi Kortela
// SPDX-License-Identifier: ISC

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sexp.h>
#include <sexp_binary_pipe.h>
#include <sexp_binary_read.h>
#include <sexp_binary_write.h>

static PGconn *database;

static const char **valid_option_names[] = {
    "client_encoding", "connect_timeout", "dbname", "host",
    "options",         "password",        "port",   "user",
};

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(2);
}

static void diemem(void) { die("out of memory"); }

static int is_valid_option_symbol(struct sexp *symbol)
{
    char **names;
    char *name;

    if (sexp_is_symbol(symbol)) {
        for (names = valid_option_names; (name = *names); names++) {
            if (sexp_is_symbol_name(symbol, name)) {
                return 1;
            }
        }
    }
    return 0;
}

static void cmd_connect(struct sexp *args)
{
    char **keys;
    char **vals;
    PGresult result;
    size_t len;

    len = sexp_list_length(args);
    if (len % 2 != 0) {
        die("odd number of options");
    }
    if (!(keys = calloc(len + 1, sizeof(*keys)))) {
        diemem();
    }
    if (!(vals = calloc(len + 1, sizeof(*vals)))) {
        diemem();
    }
    for (; sexp_is_null(args); args = sexp_nthtail(args, 2)) {
        struct sexp *key = sexp_nth(args, 0);
        struct sexp *val = sexp_nth(args, 1);

        if (!is_valid_option_symbol(key)) {
            die("no such connect option");
        }
    }
    database = PQconnectdbParams(keys, vals, 0);
}

static void cmd_execute(struct sexp *args)
{
    PGresult result;

    result =
    PQexecParams(database, sql_string, int nParams, const Oid *paramTypes,
                 const char *const *paramValues, const int *paramLengths,
                 const int *paramFormats, int resultFormat);
}

static void cmd_read_result_rows(struct sexp *args) {}

typedef void (*cmd_func_t)(struct sexp *args);

struct cmd {
    const char *name;
    cmd_func_t func;
};

static const struct cmd cmds[] = {
    { "connect", cmd_connect },
    { "execute", cmd_execute },
    { 0 },
};

static struct cmd *cmd_by_symbol(struct sexp *symbol)
{
    struct cmd *cmd;

    if (sexp_is_symbol(symbol)) {
        for (cmd = cmds; cmd->name; cmd++) {
            if (sexp_is_symbol_name(symbol, cmd->name)) {
                return cmd;
            }
        }
    }
    return 0;
}

int main(void)
{
    struct sexp_binary_read *rd;
    struct sexp_binary_write *wr;
    struct sexp *command;
    struct sexp *response;
    struct cmd *cmd;

    if ((errstr = sexp_binary_pipe(&rd, &wr))) {
        die(errstr);
    }
    for (;;) {
        if (!sexp_binary_read(rd, &command))) {
      die(sexp_binary_read_error(rd));
    }
        if (!sexp_is_list(command)) {
            die("command is not a list");
        }
        if (!(cmd = cmd_by_symbol(sexp_head(command)))) {
            die("no such command");
        }
        response = cmd->func(sexp_tail(command));
        if (!sexp_binary_write(rw, response)) {
            die(sexp_binary_write_error(wr));
        }
        sexp_free(response);
        sexp_free(command);
    }
    PQfinish(database);
    return 0;
}
