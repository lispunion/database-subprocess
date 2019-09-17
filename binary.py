#! /usr/bin/env python3

import sys
from collections import namedtuple
from io import BytesIO


Eof = namedtuple("Eof", "")


class Sym:
    def __init__(self, name):
        self.name = name

    def __repr__(self):
        return self.name


def _byte(b):
    return bytes((b,))


def read_varint_or_none(inp):
    value = None
    shift = 0
    while True:
        c = inp.read(1)
        if c == b"":
            raise EOFError("Unexpected EOF while reading bytes")
        i = ord(c)
        value = value or 0
        value |= (i & 0x7f) << shift
        shift += 7
        if not (i & 0x80):
            break
    return value


def read_varint(inp):
    value = read_varint_or_none(inp)
    if value is None:
        raise IOError("Expected varint but got eof")
    return value


def write_varint(out, value):
    buf = b""
    while value > 0x7f:
        buf += _byte(0x80 | (value & 0x7f))
        value >>= 7
    buf += _byte(value)
    out.write(buf)


def read_varbytes(inp):
    n = read_varint(inp)
    buf = inp.read(n)
    if len(buf) != n:
        raise IOError("Not enough varbytes")
    return buf


def write_varbytes(out, buf):
    write_varint(out, len(buf))
    out.write(buf)


def read_binary_sexp_tagged(inp, tag):
    if tag == 0:
        return None
    if tag == 1:
        return False
    if tag == 2:
        return True
    if tag == 3:
        return read_varbytes(inp)
    if tag == 4:
        return read_varint(inp)
    if tag == 5:
        return -read_varint(inp)
    if tag == 0xc:
        elts = []
        while True:
            elts.append(read_binary_sexp_nested(inp))
            tail_tag = read_varint(inp)
            if tail_tag == 0:
                break
            if tail_tag != 0xc:
                raise Error("Improper list")
        return elts
    if tag == 0xd:
        raise Error("Vector not implemented")
    if tag == 0xe:
        return read_varbytes(inp).decode("utf8")
    if tag == 0xf:
        return Sym(read_varbytes(inp).decode("utf8"))


def read_binary_sexp_nested(inp):
    return read_binary_sexp_tagged(inp, read_varint(inp))


def read_binary_sexp(inp):
    tag = read_varint_or_none(inp)
    if tag is None:
        return Eof()
    return read_binary_sexp_tagged(inp, tag)


def write_nested_binary_sexp(out, obj):
    if obj is None:
        write_varint(out, 0)
    elif obj is False:
        write_varint(out, 1)
    elif obj is True:
        write_varint(out, 2)
    elif isinstance(obj, bytes):
        write_varint(out, 3)
        write_varbytes(out, obj)
    elif isinstance(obj, int) and obj >= 0:
        write_varint(out, 4)
        write_varint(out, obj)
    elif isinstance(obj, int):
        write_varint(out, 5)
        write_varint(out, abs(obj))
    elif isinstance(obj, list):
        for elt in obj:
            write_varint(out, 0xc)  # pair
            write_nested_binary_sexp(out, elt)
        write_varint(out, 0)  # null
    elif isinstance(obj, str):
        write_varint(out, 0xe)
        write_varbytes(out, obj.encode("utf8"))
    elif isinstance(obj, Sym):
        write_varint(out, 0xf)
        write_varbytes(out, obj.name.encode("utf8"))
    else:
        raise TypeError("Don't know how to write that kind of object")


def write_binary_sexp(out, obj):
    write_nested_binary_sexp(out, obj)
    out.flush()


if __name__ == "__main__":
    if sys.argv[1] == "r":
        # print(hex(read_varint(sys.stdin.buffer)))
        # print(read_varbytes(sys.stdin.buffer))
        print(read_binary_sexp(sys.stdin.buffer))
    elif sys.argv[1] == "w":
        # write_varint(sys.stdout.buffer, 0x12_34_56_78)
        # write_varbytes(sys.stdout.buffer, b"Hello world")
        print(write_binary_sexp(sys.stdout.buffer, "Hello world"))
