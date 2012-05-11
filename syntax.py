################################################################################
##
## Pythonc--Python to C++ translator
##
## Copyright 2011 Zach Wegner
##
## This file is part of Pythonc.
##
## Pythonc is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Pythonc is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Pythonc.  If not, see <http://www.gnu.org/licenses/>.
##
################################################################################

import copy

import alloc

builtin_functions = {
    'abs': 1,
    'all': 1,
    'any': 1,
    'isinstance': 2,
    'iter': 1,
    'len': 1,
    'max': 1,
    'min': 1,
    'open': 2,
    'ord': 1,
    'print': -1,
    'print_nonl': 1,
    'repr': 1,
    'sorted': 1,
}
builtin_methods = {
    'dict': {
        'clear': 1,
        'copy': 1,
        'get': 3,
        'keys': 1,
        'items': 1,
        'values': 1,
    },
    'file': {
        'read': 2,
        'write': 2,
    },
    'list': {
        'append': 2,
        'count': 2,
        'extend': 2,
        'index': 2,
        'insert': 3,
        'pop': (1, 2),
        'remove': 2,
        'reverse': 1,
        'sort': 1,
    },
    'set': {
        'add': 2,
        'clear': 1,
        'copy': 1,
        'difference_update': -1,
        'discard': 2,
        'remove': 2,
        'update': -1,
    },
    'str': {
        'join': 2,
        'split': (1, 2),
        'startswith': 2,
        'upper': 1,
    },
    'tuple': {
        'count': 2,
        'index': 2,
    },
}
builtin_classes = {
    'bool': (0, 1),
    'bytes': (0, 1),
    'dict': (0, 1),
    'enumerate': 1,
    'int': (0, 2),
    'list': (0, 1),
    'range': (1, 3),
    'reversed': 1,
    'set': (0, 1),
    'str': (0, 1),
    'tuple': (0, 1),
    'type': 1,
    'zip': 2,
}
builtin_hidden_classes = {
    'NoneType',
    'bound_method',
    'builtin_function_or_method',
    'bytes_iterator',
    'dict_itemiterator',
    'dict_items',
    'dict_keyiterator',
    'dict_keys',
    'dict_valueiterator',
    'dict_values',
    'file',
    'function',
    'list_iterator',
    'method_descriptor',
    'range_iterator',
    'set_iterator',
    'str_iterator',
    'tuple_iterator',
}

def write_backend_setup(f):
    f.write('#define __STDC_FORMAT_MACROS\n')
    f.write('#include <assert.h>\n')
    f.write('#include <inttypes.h>\n')
    f.write('#include <stdarg.h>\n')
    f.write('#include <stddef.h>\n')
    f.write('#include <stdint.h>\n')
    f.write('#include <stdio.h>\n')
    f.write('#include <stdlib.h>\n')
    f.write('#include <string.h>\n')
    f.write('#include <algorithm>\n')
    f.write('#include <map>\n')
    f.write('#include <set>\n')
    f.write('#include <sstream>\n')
    f.write('#include <string>\n')
    f.write('#include <vector>\n')

    alloc.write_allocator(f)

    f.write('class node;\n')
    f.write('class tuple;\n')
    f.write('class dict;\n')
    f.write('class context;\n')

    f.write('#define LIST_BUILTIN_FUNCTIONS(x) %s\n' %
        ' '.join('x(%s)' % name for name in sorted(builtin_functions)))
    f.write('#define LIST_BUILTIN_CLASSES(x) %s\n' %
        ' '.join('x(%s)' % name for name in sorted(builtin_classes)))
    f.write('#define LIST_BUILTIN_HIDDEN_CLASSES(x) %s\n' %
        ' '.join('x(%s)' % name for name in sorted(builtin_hidden_classes)))

    for class_name in sorted(builtin_classes) + ['file']:
        methods = builtin_methods.get(class_name, [])
        f.write('#define LIST_%s_CLASS_METHODS(x) %s\n' % (class_name,
            ' '.join('x(%s, %s)' % (class_name, name) for name in sorted(methods))))

    f.write('#define LIST_BUILTIN_CLASS_METHODS(x) %s\n' %
        ' '.join('LIST_%s_CLASS_METHODS(x)' % name for name in sorted(builtin_methods)))

    for name in sorted(builtin_functions):
        f.write('node *wrapped_builtin_%s(context *ctx, tuple *args, dict *kwargs);\n' % name)
    for class_name in sorted(builtin_methods):
        methods = builtin_methods[class_name]
        for name in sorted(methods):
            f.write('node *wrapped_builtin_%s_%s(context *ctx, tuple *args, dict *kwargs);\n' % (class_name, name))

def print_arg_logic(name, f, n_args, self_class=None, method_name=None):
    f.write('    if (kwargs && kwargs->items.size())\n')
    f.write('        error("%s() does not take keyword arguments");\n' % name)

    if isinstance(n_args, tuple):
        (min_args, max_args) = n_args
        assert max_args > min_args
        f.write('    size_t args_len = args->items.size();\n')
        f.write('    if (args_len < %d)\n' % min_args)
        f.write('        error("too few arguments to %s()");\n' % name)
        f.write('    if (args_len > %d)\n' % max_args)
        f.write('        error("too many arguments to %s()");\n' % name)
        for i in range(min_args):
            f.write('    node *arg%d = args->items[%d];\n' % (i, i))
        for i in range(min_args, max_args):
            f.write('    node *arg%d = (args_len > %d) ? args->items[%d] : NULL;\n' % (i, i, i))
        return ', '.join('arg%d' % i for i in range(max_args))
    elif n_args < 0:
        return 'args'
    else:
        f.write('    if (args->items.size() != %d)\n' % n_args)
        f.write('        error("wrong number of arguments to %s()");\n' % name)
        for i in range(n_args):
            f.write('    node *arg%d = args->items[%d];\n' % (i, i))
        if self_class:
            f.write('    if (!arg0->is_%s())\n' % self_class)
            f.write('        error("bad argument to %s.%s()");\n' % (self_class, method_name))
            class_name = {'str': 'string_const', 'int': 'int_const'}.get(self_class, self_class)
            f.write('    %s *self = (%s *)arg0;\n' % (class_name, class_name))
            return ', '.join(['self'] + ['arg%d' % i for i in range(1, n_args)])
        else:
            return ', '.join('arg%d' % i for i in range(n_args))

def write_backend_post_setup(f):
    for name in sorted(builtin_functions):
        n_args = builtin_functions[name]
        f.write('node *wrapped_builtin_%s(context *ctx, tuple *args, dict *kwargs) {\n' % name)
        args = print_arg_logic(name, f, n_args)
        f.write('    return builtin_%s(%s);\n' % (name, args))
        f.write('}\n')

    for class_name in sorted(builtin_methods):
        methods = builtin_methods[class_name]
        for name in sorted(methods):
            n_args = methods[name]
            f.write('node *wrapped_builtin_%s_%s(context *ctx, tuple *args, dict *kwargs) {\n' % (class_name, name))
            args = print_arg_logic(name, f, n_args, self_class=class_name, method_name=name)
            f.write('    return builtin_%s_%s(%s);\n' % (class_name, name, args))
            f.write('}\n')

    for name in sorted(builtin_classes):
        n_args = builtin_classes[name]
        f.write('node *%s_class::__call__(context *ctx, tuple *args, dict *kwargs) {\n' % name)
        args = print_arg_logic(name, f, n_args)
        f.write('    return %s_init(%s);\n' % (name, args))
        f.write('}\n')

    for i in sorted(all_ints):
        f.write('int_const_singleton %s(%sll);\n' % (int_name(i), i))

    char_escape = {
        '"': r'\"',
        '\\': r'\\',
        '\n': r'\n',
        '\r': r'\r',
        '\t': r'\t',
    }
    for k, (v, hashkey) in all_strings.items():
        c_str = ''.join(char_escape.get(c, c) for c in k)
        f.write('string_const_singleton string_singleton_%s("%s", %sull);\n' % (v, c_str, hashkey))

    for k, v in all_bytes.items():
        f.write('const uint8_t bytes_singleton_%d_data[] = {%s};\n' % (v, ', '.join(str(x) for x in k)))
        f.write('bytes_singleton bytes_singleton_%d(sizeof(bytes_singleton_%d_data), bytes_singleton_%d_data);\n' % (v, v, v))

def globals_init(ctx):
    stmts = [Store('__name__', StringConst(ctx.module))]
    for t, l in [['function', builtin_functions], ['class', builtin_classes]]:
        for name in l:
            stmts.append(Store(name, SingletonRef('builtin_%s_%s' % (t, name))))

    return stmts

def write_output(stmts, path):
    ctx = Context('__main__')
    stmts = ctx.translate(globals_init(ctx) + stmts)

    with open(path, 'w') as f:
        write_backend_setup(f)

        f.write('#include "backend.cpp"\n')

        write_backend_post_setup(f)

        ctx.write_mod_init(f)

        f.write('int main(int argc, char **argv) {\n')
        f.write('    context *ctx = &ctx___main__, *globals = ctx;\n')

        f.write('    module_sys_singleton.init(argc, argv);\n')

        f.write(indent(stmts))

        f.write('\n}\n')

def indent(stmts, spaces=4):
    stmts = [str(s) for s in stmts]
    stmts = '\n'.join('%s%s' % (s, ';' if s and not s.endswith('}') else '') for s in stmts).splitlines()
    return '\n'.join('%s%s' % (' ' * spaces, s) for s in stmts)

def block_str(stmts, spaces=4):
    return indent((s() for s in stmts), spaces=spaces)

all_ints = set()
def register_int(value):
    global all_ints
    all_ints |= {value}

all_strings = {}
def register_string(value):
    global all_strings
    if value in all_strings:
        return all_strings[value][0]
    # Compute hash via FNV-1a algorithm. Python makes signed 64-bit arithmetic hard.
    hashkey = 14695981039346656037
    for c in value:
        hashkey ^= ord(c)
        hashkey *= 1099511628211
        hashkey &= (1 << 64) - 1
    all_strings[value] = (len(all_strings), hashkey)
    return all_strings[value][0]

all_bytes = {}
def register_bytes(value):
    global all_bytes
    if value in all_bytes:
        return all_bytes[value]
    all_bytes[value] = len(all_bytes)
    return all_bytes[value]

def int_name(i):
    return 'int_singleton_neg%d' % -i if i < 0 else 'int_singleton_%d' % i

class Context:
    def __init__(self, module):
        self.statements = []
        self.functions = []
        self.classes = []
        self.modules = []
        self.module = module
        self.temp_id = 0

    def get_temp(self):
        self.temp_id += 1
        return 'temp_%02i' % self.temp_id

    def get_temp_id(self):
        return Identifier(self.get_temp())

    def flatten_edge(self, edge, force_atom):
        node = edge()
        node = node.reduce_internal(self)
        edge.set(node)
        if not force_atom and not node.is_atom():
            temp = self.get_temp()
            edge.set(Load(temp))
            self.statements.append(Edge(Store(temp, node)))

    def flatten_list(self, node_list):
        old_stmts = self.statements
        self.statements = []
        for edge in node_list:
            node = edge().reduce_internal(self)
            if node:
                edge.set(node)
                self.statements.append(edge)
        [statements, self.statements] = self.statements, old_stmts
        return statements

    def add_module(self, m):
        self.modules.append(m)

    def add_class(self, c):
        c.flatten(self)
        self.classes.append(c)

    def add_function(self, fn):
        fn.flatten(self)
        self.functions.append(fn)

    def add_statement(self, stmt):
        stmt = stmt.reduce_internal(self)
        if stmt:
            self.statements.append(Edge(stmt))

    def translate(self, stmts):
        # Add all the statements. This reduces/flattens as well.
        for i in stmts:
            self.add_statement(i)
        stmts = [s.value for s in self.statements]

        all_globals = set()

        # Get bindings for all classes/functions
        for node in self.classes + self.functions:
            assert isinstance(node, (ClassDef, FunctionDef))
            all_globals |= node.set_binding(self)

        for node in stmts:
            if isinstance(node, Global):
                for name in node.names:
                    all_globals.add(name)
            else:
                assert not isinstance(node, (ClassDef, FunctionDef))
                for i in node.iterate_subtree():
                    if isinstance(i, (Load, Store)):
                        all_globals.add(i.name)

        # Enumerate globals. Index 0 is saved for undefined symbols.
        self.global_idx = {symbol: idx+1 for idx, symbol in enumerate(sorted(all_globals))}

        for s in self.classes + self.functions + stmts:
            for node in s.iterate_subtree():
                if isinstance(node, (Load, Store)):
                    if node.scope == 'global':
                        node.set_binding('global', self.global_idx.get(node.name, 0))

        self.global_sym_count = len(self.global_idx) + 1

        return stmts

    def write_mod_init(self, f):
        for mod in self.modules:
            mod.module_ctx.write_mod_init(f)

        f.write('node *mod_syms_%s[%s] = {};\n' % (self.module,
            self.global_sym_count))
        f.write('context ctx_%s(%s, mod_syms_%s);\n' % (self.module,
            self.global_sym_count, self.module))
        for func in self.modules + self.functions + self.classes:
            f.write('%s\n' % func)

class Node:
    def add_use(self, edge):
        assert edge not in self.uses
        self.uses.append(edge)

    def remove_use(self, edge):
        self.uses.remove(edge)

    def forward(self, new_value):
        for edge in self.uses:
            edge.value = new_value
            new_value.add_use(edge)

    def __str__(self):
        print(type(self))
        assert False

# Edge class represents the use of a Node by another. This allows us to use
# value forwarding and such. It is used like so:
#
# self.expr = Edge(expr) # done implicitly by each class' constructor
# value = self.expr()
# self.expr.set(new_value)
class Edge:
    def __init__(self, value):
        self.value = value
        value.add_use(self)

    def __call__(self):
        return self.value

    def set(self, value):
        self.value.remove_use(self)
        self.value = value
        value.add_use(self)

    def __str__(self):
        print(self.value)
        assert False

ARG_REG, ARG_EDGE, ARG_EDGE_LIST, ARG_BLOCK = list(range(4))
arg_map = {'&': ARG_EDGE, '*': ARG_EDGE_LIST, '$': ARG_BLOCK}

# Weird decorator: a given arg string represents a standard form for arguments
# to Node subclasses. We use these notations:
# op, &expr, *explist, $block
# op -> normal attribute
# &expr -> edge attribute, will create an Edge object (used for linking to other Nodes)
# *explist -> python list of edges
# $stmts -> also python list of edges, but representing an enclosed block
def node(argstr='', no_flatten=[]):
    args = [a.strip() for a in argstr.split(',') if a.strip()]
    new_args = []
    for a in args:
        if a[0] in arg_map:
            new_args.append((arg_map[a[0]], a[1:]))
        else:
            new_args.append((ARG_REG, a))

    args = new_args

    atom = not any(a[0] != ARG_REG for a in args)

    # Decorators must return a function. This adds __init__ and is_atom methods
    # to a Node subclass
    def attach(node):
        def __init__(self, *iargs):
            assert len(iargs) == len(args), 'bad args, expected %s(%s)' % (node.__name__, argstr)

            for (arg_type, arg_name), v in zip(args, iargs):
                if arg_type == ARG_EDGE:
                    setattr(self, arg_name, Edge(v) if v is not None else None)
                elif arg_type in {ARG_EDGE_LIST, ARG_BLOCK}:
                    setattr(self, arg_name, [Edge(item) for item in v])
                else:
                    setattr(self, arg_name, v)
            self.uses = []
            if hasattr(self, 'setup'):
                self.setup()

        def iterate_subtree(self):
            yield self
            for (arg_type, arg_name) in args:
                if arg_type == ARG_EDGE:
                    edge = getattr(self, arg_name)
                    if edge:
                        for i in edge().iterate_subtree():
                            yield i
                elif arg_type in {ARG_EDGE_LIST, ARG_BLOCK}:
                    for edge in getattr(self, arg_name):
                        for i in edge().iterate_subtree():
                            yield i

        def reduce_internal(self, ctx):
            if hasattr(self, 'reduce'):
                new_self = self.reduce(ctx)
                if new_self != self:
                    self = new_self.reduce(ctx) if hasattr(new_self, 'reduce') else new_self

            if self:
                self.flatten(ctx)

            return self

        def flatten(self, ctx):
            for (arg_type, arg_name) in args:
                if arg_type == ARG_EDGE:
                    edge = getattr(self, arg_name)
                    if edge:
                        ctx.flatten_edge(edge, arg_name in no_flatten)
                elif arg_type == ARG_EDGE_LIST:
                    for edge in getattr(self, arg_name):
                        ctx.flatten_edge(edge, arg_name in no_flatten)
                elif arg_type == ARG_BLOCK:
                    setattr(self, arg_name, ctx.flatten_list(getattr(self, arg_name)))

        def print_tree(self, indent=0):
            if len(args) == 0:
                print('%s' % type(self), sep='')
            else:
                print('%s%s(' % (' ' * indent, type(self)), sep='')
                for (arg_type, arg_name) in args:
                    if arg_type == ARG_EDGE:
                        print('  %s%s=' % (' ' * indent, arg_name))
                        edge = getattr(self, arg_name)
                        if edge:
                            edge().print_tree(indent=indent+4)
                    elif arg_type in {ARG_EDGE_LIST, ARG_BLOCK}:
                        print('  %s%s=[' % (' ' * indent, arg_name))
                        for item in getattr(self, arg_name):
                            item().print_tree(indent=indent+4)
                        print('  %s],' % (' ' * indent))
                    else:
                        print('  %s%s=%s,' % (' ' * indent, arg_name,
                            getattr(self, arg_name)))
                print('%s)' % (' ' * indent))
                
        def is_atom(self):
            return atom

        node.__init__ = __init__
        node.iterate_subtree = iterate_subtree
        node.reduce_internal = reduce_internal
        node.flatten = flatten
        node.print_tree = print_tree
        node.is_atom = is_atom

        return node

    return attach

@node()
class NullConst(Node):
    def __str__(self):
        return 'NULL'

@node()
class NoneConst(Node):
    def __str__(self):
        return '(&none_singleton)'

@node('value')
class BoolConst(Node):
    def __str__(self):
        return '(&bool_singleton_%s)' % self.value

@node('value')
class IntConst(Node):
    def setup(self):
        register_int(self.value)

    def __str__(self):
        return '(&%s)' % int_name(self.value)

@node('value')
class StringConst(Node):
    def setup(self):
        self.id = register_string(self.value)

    def __str__(self):
        return '(&string_singleton_%s)' % self.id

@node('value')
class BytesConst(Node):
    def setup(self):
        self.id = register_bytes(self.value)

    def __str__(self):
        return '(&bytes_singleton_%s)' % self.id

@node('name')
class Identifier(Node):
    def __str__(self):
        return self.name

@node('names')
class Global(Node):
    def __str__(self):
        return ''

@node('name')
class SingletonRef(Node):
    def __str__(self):
        return '&%s' % self.name

@node('ref_type, args')
class Ref(Node):
    def __str__(self):
        return '(pc_new(%s)(%s))' % (self.ref_type, ', '.join(str(a) for a in self.args))

@node('op, &rhs')
class UnaryOp(Node):
    def __str__(self):
        return '%s->%s()' % (self.rhs(), self.op)

@node('op, &lhs, &rhs')
class BinaryOp(Node):
    def __str__(self):
        return '%s->%s(%s)' % (self.lhs(), self.op, self.rhs())

@node('name')
class Load(Node):
    def setup(self):
        self.scope = 'global'

    def set_binding(self, scope, idx):
        self.scope = scope
        self.idx = idx

    def __str__(self):
        if self.scope == 'global':
            return 'globals->load(%s)' % self.idx
        elif self.scope == 'class':
            return 'this->getattr("%s")' % self.name
        elif self.scope == 'module':
            return 'globals->load(%s)' % self.idx
        return 'ctx->load(%s)' % self.idx

@node('name, &expr', no_flatten=['expr'])
class Store(Node):
    def setup(self):
        self.scope = 'global'

    def set_binding(self, scope, idx):
        self.scope = scope
        self.idx = idx

    def __str__(self):
        if self.scope == 'global':
            return 'globals->store(%s, %s)' % (self.idx, self.expr())
        elif self.scope == 'class':
            return 'this->setattr("%s", %s)' % (self.name, self.expr())
        elif self.scope == 'module':
            return 'globals->store(%s, %s)' % (self.idx, self.expr())
        return 'ctx->store(%s, %s)' % (self.idx, self.expr())

@node('&name, attr, &expr', no_flatten=['expr'])
class StoreAttr(Node):
    def __str__(self):
        return '%s->__setattr__(%s, %s)' % (self.name(), self.attr, self.expr())

@node('&name, &index, &expr', no_flatten=['expr'])
class StoreSubscript(Node):
    def __str__(self):
        return '%s->__setitem__(%s, %s)' % (self.name(), self.index(), self.expr())

@node('&expr, &index')
class DeleteSubscript(Node):
    def __str__(self):
        return '%s->__delitem__(%s)' % (self.expr(), self.index())

@node('&expr, index, &value')
class StoreSubscriptDirect(Node):
    def __str__(self):
        return '%s->items[%d] = %s' % (self.expr(), self.index, self.value())

@node('&obj, method_name, *args')
class MethodCall(Node):
    def __str__(self):
        args = ['%s' % a() for a in self.args]
        return '%s->%s(%s)' % (self.obj(), self.method_name, ', '.join(args))

@node('*items')
class List(Node):
    def reduce(self, ctx):
        name = ctx.get_temp_id()
        ctx.add_statement(Assign(name, Ref('list', [len(self.items)]), 'list'))
        for i, item in enumerate(self.items):
            ctx.add_statement(StoreSubscriptDirect(name, i, item()))
        return name

@node('*items')
class Tuple(Node):
    def reduce(self, ctx):
        name = ctx.get_temp_id()
        ctx.add_statement(Assign(name, Ref('tuple', [len(self.items)]), 'tuple'))
        for i, item in enumerate(self.items):
            ctx.add_statement(StoreSubscriptDirect(name, i, item()))
        return name

@node('&items')
class TupleFromIter(Node):
    def reduce(self, ctx):
        name = ctx.get_temp_id()
        ctx.add_statement(Assign(name, Ref('tuple', []), 'tuple'))
        iter_temp = ctx.get_temp()
        stmts = [MethodCall(name, 'append', [Load(iter_temp)])]
        ctx.add_statement(For(iter_temp, self.items(), stmts))
        return name

@node('*keys, *values')
class Dict(Node):
    def reduce(self, ctx):
        name = ctx.get_temp_id()
        ctx.add_statement(Assign(name, Ref('dict', []), 'dict'))
        for k, v in zip(self.keys, self.values):
            ctx.add_statement(StoreSubscript(name, k(), v()))
        return name

@node('*items')
class Set(Node):
    def reduce(self, ctx):
        name = ctx.get_temp_id()
        ctx.add_statement(Assign(name, Ref('set', []), 'set'))
        for i in self.items:
            ctx.add_statement(MethodCall(name, 'add', [i()]))
        return name

@node('&expr, &start, &end, &step')
class Slice(Node):
    def __str__(self):
        return '%s->__slice__(%s, %s, %s)' % (self.expr(), self.start(), self.end(), self.step())

@node('&expr, &index')
class Subscript(Node):
    def __str__(self):
        return '%s->__getitem__(%s)' % (self.expr(), self.index())

@node('&expr, &attr')
class Attribute(Node):
    def __str__(self):
        return '%s->__getattr__(%s)' % (self.expr(), self.attr())

@node('&func, &args, &kwargs')
class Call(Node):
    def __str__(self):
        return '%s->__call__(ctx, %s, %s)' % (self.func(), self.args(), self.kwargs())

@node('&expr, &true_expr, &false_expr')
class IfExp(Node):
    def reduce(self, ctx):
        temp = ctx.get_temp_id()
        ctx.add_statement(Assign(temp, NullConst(), 'node'))
        ctx.add_statement(If(self.expr(), [Assign(temp, self.true_expr(), None)],
            [Assign(temp, self.false_expr(), None)]))
        return temp

@node('op, &lhs_expr, &rhs_expr')
class BoolOp(Node):
    def reduce(self, ctx):
        temp = ctx.get_temp_id()
        ctx.add_statement(Assign(temp, self.lhs_expr(), 'node'))
        expr = temp
        true, false = [Assign(temp, self.rhs_expr(), None)], []
        if self.op == 'or':
            true, false, = false, true
        ctx.add_statement(If(Test(expr), true, false))
        return temp

@node('&target, &expr, target_type', no_flatten=['expr'])
class Assign(Node):
    def __str__(self):
        target_type = ('%s *' % self.target_type) if self.target_type else ''
        return '%s%s = %s' % (target_type, self.target(), self.expr())

@node('&expr')
class Test(Node):
    def __str__(self):
        return '%s->bool_value()' % self.expr()

@node('&expr, $true_stmts, $false_stmts', no_flatten=['expr'])
class If(Node):
    def __str__(self):
        # Invert expression if nothing in the if-block
        if not self.true_stmts and self.false_stmts:
            true_stmts = block_str(self.false_stmts)
            body =  """if (!{expr}) {{
{stmts}
}}""".format(expr=self.expr(), stmts=true_stmts)
        else:
            true_stmts = block_str(self.true_stmts)
            body =  """if ({expr}) {{
{stmts}
}}""".format(expr=self.expr(), stmts=true_stmts)
            if self.false_stmts:
                false_stmts = block_str(self.false_stmts)
                body +=  """ else {{
{stmts}
}}""".format(stmts=false_stmts)
        return body

@node('comp_type, target, &iter, &cond, &expr, &expr2')
class Comprehension(Node):
    def reduce(self, ctx):
        if self.comp_type == 'set':
            l = Set([])
        elif self.comp_type == 'dict':
            l = Dict([], [])
        else:
            l = List([])
        temp = ctx.get_temp_id()
        ctx.add_statement(Assign(temp, l, self.comp_type))
        iter_name = ctx.get_temp()
        ctx.add_statement(Store(iter_name, UnaryOp('__iter__', self.iter())))

        # Construct body of while loop that implements comprehension
        # Get next item of iterable
        iter_next = MethodCall(Load(iter_name), 'next', [])
        item = ctx.get_temp_id()
        stmts = [Assign(item, iter_next, 'node')]
        stmts += [If(item, [], [Break()])]

        # Unpack arguments
        if isinstance(self.target, tuple):
            for i, target in enumerate(self.target):
                stmts += [Store(target, '%s->__getitem__(%s)' % (item, i))]
        else:
            stmts += [Store(self.target, item)]

        # Condtional
        if self.cond:
            stmts += [If(Test(self.cond()), [], [Continue()])]

        if self.comp_type == 'set':
            stmts += [MethodCall(temp, 'add', [self.expr()])]
        elif self.comp_type == 'dict':
            stmts += [MethodCall(temp, '__setitem__', [self.expr(), self.expr2()])]
        else:
            stmts += [MethodCall(temp, 'append', [self.expr()])]

        w = While(stmts)

        ctx.add_statement(w)

        return temp

@node()
class Break(Node):
    def __str__(self):
        return 'break'

@node()
class Continue(Node):
    def __str__(self):
        return 'continue'

@node('target, &iter, $stmts')
class For(Node):
    def reduce(self, ctx):
        iter_name = ctx.get_temp()
        ctx.add_statement(Store(iter_name, UnaryOp('__iter__', self.iter())))

        # Get next item of iterable
        iter_next = MethodCall(Load(iter_name), 'next', [])
        item = ctx.get_temp_id()
        stmts = [Assign(item, iter_next, 'node')]
        stmts += [If(item, [], [Break()])]

        # Unpack arguments
        if isinstance(self.target, list):
            for i, target in enumerate(self.target):
                stmts += [Store(target, MethodCall(item, '__getitem__', [IntConst(i)]))]
        else:
            stmts += [Store(self.target, item)]

        stmts += [s() for s in self.stmts]

        w = While(stmts)

        return w

@node('$stmts')
class While(Node):
    def __str__(self):
        stmts = block_str(self.stmts)
        body = """\
while (1)
{{
{stmts}
}}""".format(stmts=stmts)
        return body

@node('&value')
class Return(Node):
    def setup(self):
        if self.value is None:
            self.value = Edge(NoneConst())

    def __str__(self):
        body = 'return %s' % self.value()
        return body

@node('&expr, lineno')
class Assert(Node):
    def __str__(self):
        body = """if (!{expr}->bool_value()) {{
    error("assert failed at line {lineno}");
}}""".format(expr=self.expr(), lineno=self.lineno)
        return body

@node('&expr, lineno')
class Raise(Node):
    def __str__(self):
        return 'error("exception %%p raised at line %%d", %s, %d)' % (self.expr(), self.lineno)

@node('&expr')
class CollectGarbage(Node):
    def __str__(self):
        return 'collect_garbage(ctx, %s)' % (self.expr() if self.expr else 'NULL')

@node('args, *defaults')
class Arguments(Node):
    def reduce(self, ctx):
        new_def = [None] * (len(self.args) - len(self.defaults))
        defaults = new_def + self.defaults
        name_strings = [Edge(StringConst(a)) for a in self.args]

        args = Identifier('args')
        kwargs = Identifier('kwargs')
        for i, (arg, default, name) in enumerate(zip(self.args, defaults, name_strings)):
            arg_value = Subscript(args, IntConst(i))
            if default:
                comp = BinaryOp('__gt__', MethodCall(args, 'len', []), IntConst(i))
                arg_value = IfExp(comp, arg_value, default())

#            temp = ctx.get_temp_id()
#            ctx.add_statement(Assign(temp, MethodCall(kwargs, 'lookup', [name()])))
#            comp = BinaryOp('__gt__', MethodCall(args, 'len', []), IntConst(i))
#            arg_value = IfExp(comp, arg_value, default())
#            arg_value = '(kwargs && kwargs->lookup(%s)) ? kwargs->lookup(%s) : %s' % \
#                (name(), name(), arg_value)

            ctx.add_statement(Store(arg, arg_value))

        return self

    def __str__(self):
        return ''

def get_globals_locals(node):
    # Get bindings of all variables. Globals are the variables that have "global x"
    # somewhere in the function, or are never written in the function.
    all_globals = set()
    all_loads = set()
    all_stores = set()

    for child in node.iterate_subtree():
        if isinstance(child, Global):
            for name in child.names:
                all_globals.add(name)
        elif isinstance(child, Load):
            all_loads.add(child.name)
        elif isinstance(child, Store):
            all_stores.add(child.name)

    all_globals |= (all_loads - all_stores)
    all_locals = (all_loads | all_stores) - all_globals

    return all_globals, all_locals

@node('name, $stmts, exp_name')
class FunctionDef(Node):
    def setup(self):
        self.exp_name = self.exp_name if self.exp_name else self.name
        self.exp_name = 'fn_%s' % self.exp_name # make sure no name collisions

    def reduce(self, ctx):
        self.module = ctx.module
        ctx.add_function(self)
        return Store(self.name, Ref('function_def', [Identifier(self.exp_name)]))

    def set_binding(self, ctx):
        all_globals, all_locals = get_globals_locals(self)

        self.local_count = len(all_locals)

        local_idx = {symbol: idx for idx, symbol in enumerate(sorted(all_locals))}

        for node in self.iterate_subtree():
            if isinstance(node, (Load, Store)):
                if node.name not in all_globals:
                    node.set_binding('local', local_idx[node.name])

        return all_globals

    def __str__(self):
        stmts = block_str(self.stmts)
        body = """
node *{name}(context *parent_ctx, tuple *args, dict *kwargs) {{
    node *local_syms[{local_count}] = {{}};
    context ctx[1] = {{context(parent_ctx, {local_count}, local_syms)}};
    context *globals = &ctx_{module};
{stmts}
}}""".format(name=self.exp_name, module=self.module,
        local_count=self.local_count, stmts=stmts)
        return body

@node('name, $stmts')
class ClassDef(Node):
    def setup(self):
        self.class_name = 'class_%s' % self.name
        self.class_inst = '%s_singleton' % self.class_name
        self.obj_name = '%s_obj' % self.class_name

    def reduce(self, ctx):
        self.module = ctx.module
        ctx.add_class(self)
        return Store(self.name, SingletonRef(self.class_inst))

    def set_binding(self, ctx):
        all_globals, _ = get_globals_locals(self)

        for node in self.iterate_subtree():
            if isinstance(node, (Load, Store)):
                if node.name not in all_globals:
                    node.set_binding('class', 0)

        return all_globals

    def __str__(self):
        stmts = block_str(self.stmts, spaces=8)
        body = """
class {cname}: public class_def {{
public:
    {cname}() {{
    context *globals = &ctx_{module};
{stmts}
    }}
    virtual std::string repr() {{
        return std::string("<class '{name}'>");
    }}
    virtual node *__call__(context *ctx, tuple *args, dict *kwargs);
    virtual const char *type_name() {{ return "{cname}"; }}
}} {cinst};
class {oname}: public object {{
public:
    virtual node *getattr(const char *key) {{
        auto attr = this->lookup(key);
        if (attr)
            return attr;
        if (!strcmp(key, "__class__"))
            return &{cinst};
        return pc_new(bound_method)(this, {cinst}.getattr(key));
    }}
    virtual node *type() {{ return &{cinst}; }}
}};
node *{cname}::__call__(context *ctx, tuple *args, dict *kwargs) {{
    node *init = this->getattr("__init__");
    object *obj = pc_new({oname})();
    if (!init)
        return obj;
    int_t len = args->items.size();
    tuple *new_args = pc_new(tuple)(len + 1);
    new_args->items[0] = obj;
    for (int_t i = 0; i < len; i++)
        new_args->items[i+1] = args->items[i];
    init->__call__(ctx, new_args, kwargs);
    return obj;
}}
""".format(name=self.name, cname=self.class_name, cinst=self.class_inst,
        module=self.module, oname=self.obj_name, stmts=stmts)
        return body

@node('name, path, *stmts')
class ModuleDef(Node):
    def setup(self):
        self.module_name = 'module_%s' % self.name
        self.module_inst = '%s_singleton' % self.module_name

    def reduce(self, ctx):
        self.module_ctx = Context(self.name)
        stmts = globals_init(self.module_ctx) + [s() for s in self.stmts]
        self.stmts = [Edge(s) for s in self.module_ctx.translate(stmts)]

        ctx.add_module(self)

        return Store(self.name, SingletonRef(self.module_inst))

    def __str__(self):
        stmts = block_str(self.stmts, spaces=8)

        getattrs = []
        for key, idx in self.module_ctx.global_idx.items():
            getattrs += ['else if (!strcmp(attr, "%s")) return ctx_%s.load(%s);' % (key, self.name, idx)]
        getattrs = '\n'.join(getattrs)

        body = """
class {mname}: public module_def {{
public:
    {mname}() {{
        context *ctx = &ctx_{name}, *globals = ctx;
{stmts}
    }}
    virtual node *getattr(const char *attr) {{
        if (0) ;
{getattrs}
        error("not found");
    }}
    virtual std::string repr() {{
        return std::string("<module '{name}' from '{path}'>");
    }}
    virtual const char *type_name() {{ return "{mname}"; }}
}} {minst};
""".format(name=self.name, mname=self.module_name, stmts=stmts,
        getattrs=getattrs, minst=self.module_inst, path=self.path)
        return body
