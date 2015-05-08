"""Compiler for SnakeBed.

Please keep in sync with unmarshal code.

"""

import sys
import os
import struct
import argparse
import __future__

COMPILER_VERSION = 0x0101

_strtab = []
_count_ints = 0
_count_small_ints = 0
_count_proper_ints = 0
_count_strrefs = 0
_count_tuples = 0
_count_lists = 0
_count_dicts = 0
_count_codes = 0

def write_raw_byte(output, o):
    global _count_ints, _count_small_ints
    output.write(struct.pack('<B', o & 0xFF))
    _count_ints += 1
    _count_small_ints += 1

def write_raw_half(output, o):
    global _count_ints
    output.write(struct.pack('<H', o & 0xFFFF))
    _count_ints += 1

def write_raw_int(output, o):
    global _count_ints, _count_small_ints
    output.write(struct.pack('<i', o))
    _count_ints += 1

def write_int(output, o):
    global _count_proper_ints
    output.write('i')
    write_raw_int(output, o)
    _count_proper_ints += 1

def write_str(output, o):
    global _count_strrefs
    try:
        strtab_index = _strtab.index(o)
    except ValueError:
        _strtab.append(o)
        length = len(o)
        if length < 0x100:
            output.write('s')
            write_raw_byte(output, length)
        else:
            output.write('S')
            write_raw_int(output, length)
        output.write(o)
    else:
        if strtab_index < 0x100:
            output.write('r')
            write_raw_byte(output, strtab_index)
        elif strtab_index < 0x10000:
            output.write('R')
            write_raw_half(output, strtab_index)
        else:
            # Highly unlikely.
            raise ValueError, "string table index overflow"
        _count_strrefs += 1

def write_obj(output, o):
    global _count_tuples, _count_lists, _count_dicts, _count_codes
    otype = type(o)
    if o is None:
        output.write('N')
    elif o is False:
        output.write('F')
    elif o is True:
        output.write('T')
    elif otype is int:
        write_int(output, o)
    elif otype is str:
        write_str(output, o)
    elif otype is tuple:
        output.write('(')
        write_raw_int(output, len(o))
        for x in o:
            write_obj(output, x)
        _count_tuples += 1
    elif otype is list:
        output.write('[')
        write_raw_int(output, len(o))
        for x in o:
            write_obj(output, x)
        _count_lists += 1
    elif otype is dict:
        output.write('{')
        for k, v in o.iteritems():
            # Drop docstrings
            #if k == '__doc__':
            #    continue
            write_obj(output, k)
            write_obj(output, v)
        output.write('0')
        _count_dicts += 1
    elif str(otype) == "<type 'code'>":
        output.write('c')
        write_obj(output, o.co_name)
        write_raw_int(output, o.co_flags)
        write_raw_int(output, o.co_stacksize)
        write_raw_int(output, o.co_argcount)
        write_obj(output, o.co_code)
        write_obj(output, o.co_consts)
        # These are used with {Load|Store|Delete}{Global|Name}
        write_obj(output, o.co_names)
        # These are used with {Load|Store|Delete}Fast
        write_obj(output, o.co_varnames)
        # free/cellvars
        _count_codes += 1
    else:
        # Currently not handled: long, unicode
        raise TypeError, "unknown type passed: %s" % str(otype)

def do_compile(module_name, input, output):
    flags = __future__.print_function.compiler_flag
    module = compile(input.read(), '<source>', 'exec', flags, 1)
    
    output.write(struct.pack('<14sH', 'MyLittlePython', COMPILER_VERSION))
    write_obj(output, module)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='sbcompile.py: bytecode compiler for SnakeBed.')
    parser.add_argument('input',
        help='the input Python file to be compiled')
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()

    input_path = os.path.abspath(args.input)
    if args.verbose:
        print 'Input file: %s' % input_path
    input = open(input_path, 'r')
    head = os.path.splitext(input_path)[0]
    output_path = head + '.sb'
    if args.verbose:
        print 'Output file: %s' % output_path
    output = open(output_path, 'wb')
    
    module_name = os.path.basename(head)
    if args.verbose:
        print 'Module name: %s' % module_name
        print
    
    do_compile(module_name, input, output)
    
    if args.verbose:
        print 'Done.'
        print 'Compiled file size: %d bytes' % output.tell()
        print 'Objects in file by type:'
        print '  int:    %d, of those:' % _count_ints
        print '    byte-sized: %d' % _count_small_ints
        print '    proper:     %d' % _count_proper_ints
        print '  str:    %d' % len(_strtab)
        print '  strref: %d' % _count_strrefs
        print '  tuple:  %d' % _count_tuples
        print '  list:   %d' % _count_lists
        print '  dict:   %d' % _count_dicts
        print '  code:   %d' % _count_codes

    input.close()
    output.close()
