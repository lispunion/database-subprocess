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

#include <sqlite3.h>

static sqlite3 *database;
static sqlite3_stmt *stmt;
static int should_quit;

static void warn(const char *msg) { fprintf(stderr, "%s\n", msg); }

static void die(const char *msg)
{
    if (msg) {
        warn(msg);
    }
    exit(2);
}

static void diemem(void) { die("out of memory"); }

static struct sexp *new_ok(void)
{
    return sexp_new_pair(sexp_new_symbol("ok"), sexp_new_null());
}

static struct sexp *new_error(const char *code, const char *message)
{
    return sexp_new_pair(
    sexp_new_symbol("error"),
    sexp_new_pair(sexp_new_symbol(code),
                  sexp_new_pair(sexp_new_string(message), sexp_new_null())));
}

static struct sexp *cmd_connect(struct sexp *args)
{
    const char *dbname;
    size_t len;
    int error;

    dbname = 0;  // same as ":memory:"

    len = sexp_list_len(args);
    if (len % 2 != 0) {
        return new_error("args", "odd number of args");
    }
    for (; !sexp_is_null(args); args = sexp_list_tail(args, 2)) {
        struct sexp *name = sexp_list_ref(args, 0);
        struct sexp *value = sexp_list_ref(args, 1);

        if (!sexp_is_symbol(name)) {
            return new_error("args", "option name is not a symbol");
        }
        if (!sexp_is_string(value)) {
            return new_error("args", "option value is not a string");
        }
        if (sexp_is_symbol_name(name, "dbname")) {
            if (!(dbname = sexp_strdup(value))) {
                diemem();
            }
        } else {
            return new_error("args", "no such database option for sqlite");
        }
    }
    if ((error = sqlite3_open(dbname, &database))) {
        if (!database) {
            die(sqlite3_errstr(error));
        }
        warn(sqlite3_errmsg(database));
        if ((error = sqlite3_close(database))) {
            die(sqlite3_errmsg(database));
        }
        die(0);
    }
    return new_ok();
}

static struct sexp *cmd_disconnect(struct sexp *args)
{
    if (sexp_list_len(args)) {
        return new_error("args", "wrong number of args");
    }
    should_quit = 1;
    return new_ok();
}

// sqlite3_prepare()
// sqlite3_step()
// sqlite3_column()
// sqlite3_finalize()

static struct sexp *cmd_execute(struct sexp *args)
{
    struct sexp *sql_sexp;
    static char *sql;
    const char *tail;
    int error;

    free(sql);
    if (sexp_list_len(args) != 1) {
        return new_error("args", "wrong number of args");
    }
    if (!database) {
        return new_error("not-connected", "not connected to database");
    }
    sql_sexp = sexp_list_ref(args, 0);
    if (!sexp_is_string(sql_sexp)) {
        return new_error("args", "SQL query is not a string");
    }
    if (!(sql = sexp_strdup(sql_sexp))) {
        return new_error("args", "cannot turn SQL query into C string");
    }
    if ((error = sqlite3_prepare_v2(database, sql, -1, &stmt, &tail))) {
        return new_error("args", sqlite3_errmsg(database));
    }
    if (*tail) {
        return new_error("args",
                         "execute: got more than one SQL statement at once");
    }
    error = sqlite3_step(stmt);
    if (error == SQLITE_ROW) {
        return sexp_new_int64(1);
    } else if (error != SQLITE_DONE) {
        return new_error("args", sqlite3_errmsg(database));
    }
    if ((error = sqlite3_finalize(stmt))) {
        return new_error("args", sqlite3_errmsg(database));
    }
    return new_ok();
}

static struct sexp *cmd_read_result_rows(struct sexp *args)
{
    if (sexp_list_len(args)) {
        return new_error("args", "wrong number of args");
    }
    return new_ok();
}

typedef struct sexp *(*cmd_func_t)(struct sexp *args);

struct cmd {
    const char *name;
    cmd_func_t func;
};

static const struct cmd cmds[] = {
    { "connect", cmd_connect },
    { "disconnect", cmd_disconnect },
    { "execute", cmd_execute },
    { "read-result-rows", cmd_read_result_rows },
    { 0 },
};

static const struct cmd *cmd_by_symbol(struct sexp *symbol)
{
    const struct cmd *cmd;

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
    const struct cmd *cmd;
    const char *errstr;
    int error;

    if ((errstr = sexp_binary_pipe(&rd, &wr))) {
        die(errstr);
    }
    while (!should_quit) {
        if (!sexp_binary_read(rd, &command)) {
            die(sexp_binary_read_error(rd));
        }
        if (!sexp_is_list(command)) {
            die("command is not a list");
        }
        if (!(cmd = cmd_by_symbol(sexp_head(command)))) {
            die("no such command");
        }
        response = cmd->func(sexp_tail(command));
        if (!sexp_binary_write(wr, response)) {
            die(sexp_binary_write_error(wr));
        }
        sexp_free(response);
        sexp_free(command);
    }
    if (database) {
        if ((error = sqlite3_close(database))) {
            die(sqlite3_errmsg(database));
        }
    }
    return 0;
}
