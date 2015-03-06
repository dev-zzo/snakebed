#!/usr/bin/env /usr/bin/python
"""
Dumps all the code within a compiled .py file.

"""

import sys
import dis
import __future__

_flags_decode = (
#   ( 'CO_OPTIMIZED', 0x0001),
    ( 'CO_NEWLOCALS', 0x0002),
    ( 'CO_VARARGS', 0x0004),
    ( 'CO_VARKEYWORDS', 0x0008),
    ( 'CO_NESTED', 0x0010),
    ( 'CO_GENERATOR', 0x0020),
    ( 'CO_NOFREE', 0x0040),
    ( 'CO_FUTURE_DIVISION', 0x2000),
    ( 'CO_FUTURE_ABSOLUTE_IMPORT', 0x4000),
    ( 'CO_FUTURE_WITH_STATEMENT', 0x8000),
#   ( 'CO_FUTURE_PRINT_FUNCTION', 0x10000),
    ( 'CO_FUTURE_UNICODE_LITERALS', 0x20000),
    )

 
def output(text):
    print text

def output_table(tab):
    i = 0
    for item in tab:
        output('%5d: %s' % (i, item))
        i += 1

def flags_text(f):
    global _flags_decode
    items = []
    for e in _flags_decode:
        if (e[1] & f) != 0:
            items.append(e[0])
    return ', '.join(items)

def dumpy_co(co, parent=None):
    output('')
    name = (parent + '.' + co.co_name) if parent is not None else co.co_name
    output('Code object: %s' % name)
    
    # https://docs.python.org/2/library/inspect.html
    output('  co_name:        %s' % co.co_name)
    
    # name of file in which this code object was created
    output('  co_filename:    %s' % co.co_filename)
    # number of first line in Python source code
    output('  co_firstlineno: %s' % co.co_firstlineno)
    # encoded mapping of line numbers to bytecode indices
    output('  co_lnotab:      %s' % repr(co.co_lnotab))
    
    # bitmap: 1=optimized | 2=newlocals | 4=*arg | 8=**arg
    output('  co_flags:       %04X (%s)' % (co.co_flags, flags_text(co.co_flags)))
    # virtual machine stack space required
    output('  co_stacksize:   %d' % co.co_stacksize)
    # number of arguments (not including * or ** args)
    output('  co_argcount:    %d' % co.co_argcount)
    # number of local variables
    output('  co_nlocals:     %d' % co.co_nlocals)
    if co.co_nlocals != len(co.co_varnames):
        print 'WARNING!! co_nlocals and co_varnames differ!'

    output('  co_varnames:    %d' % len(co.co_varnames))
    output_table(co.co_varnames)

    output('  co_names:       %d' % len(co.co_names))
    output_table(co.co_names)

    output('  co_freevars:    %d' % len(co.co_freevars))
    output_table(co.co_freevars)

    output('  co_cellvars:    %d' % len(co.co_cellvars))
    output_table(co.co_cellvars)

    output('  co_consts:      %d' % len(co.co_consts))
    i = 0
    for item in co.co_consts:
        output('%5d: %s' % (i, repr(item)))
        i += 1

    output('  co_code:        %d' % len(co.co_code))
    dis.dis(co)
    
    for item in co.co_consts:
        if repr(item)[:5] == '<code':
            dumpy_co(item, name)
    output('')

def dumpy_text(text, path):
    output("* * * DUMP START\n")
    
    output('Source path:   %s' % path)
    output('Source length: %d' % len(text))
    output('')
    
    flags = __future__.print_function.compiler_flag
    co = compile(text, path, 'exec', flags, 1)
    dumpy_co(co)
    
    output('')
    output("* * * DUMP END\n")

def dumpy_file(path):
    with open(path) as fp:
        text = fp.read()
        dumpy_text(text, path)


if __name__ == '__main__':
    dumpy_file(sys.argv[1])
