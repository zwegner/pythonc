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
builtin_symbols = sorted(builtin_functions) + sorted(builtin_classes) + [
    '__name__',
    '__args__',
]

def write_backend_setup(f):
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

#    for x in builtin_symbols:
#        f.write('#define sym_id_%s %s\n' % (x, transformer.symbol_idx['global'][x]))

    for name in sorted(builtin_functions):
        f.write('node *wrapped_builtin_%s(context *globals, context *ctx, tuple *args, dict *kwargs);\n' % name)
    for class_name in sorted(builtin_methods):
        methods = builtin_methods[class_name]
        for name in sorted(methods):
            f.write('node *wrapped_builtin_%s_%s(context *globals, context *ctx, tuple *args, dict *kwargs);\n' % (class_name, name))

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
        f.write('node *wrapped_builtin_%s(context *globals, context *ctx, tuple *args, dict *kwargs) {\n' % name)
        args = print_arg_logic(name, f, n_args)
        f.write('    return builtin_%s(%s);\n' % (name, args))
        f.write('}\n')

    for class_name in sorted(builtin_methods):
        methods = builtin_methods[class_name]
        for name in sorted(methods):
            n_args = methods[name]
            f.write('node *wrapped_builtin_%s_%s(context *globals, context *ctx, tuple *args, dict *kwargs) {\n' % (class_name, name))
            args = print_arg_logic(name, f, n_args, self_class=class_name, method_name=name)
            f.write('    return builtin_%s_%s(%s);\n' % (class_name, name, args))
            f.write('}\n')

    for name in sorted(builtin_classes):
        n_args = builtin_classes[name]
        f.write('node *%s_class::__call__(context *globals, context *ctx, tuple *args, dict *kwargs) {\n' % name)
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

def write_output(node, path):
    stmts, functions = flatten_node_list(node)

    with open(path, 'w') as f:
        write_backend_setup(f)

        f.write('#include "backend.cpp"\n')

        write_backend_post_setup(f)

        for func in functions:
            f.write('%s\n' % func)

        global_sym_count = 100
        f.write('int main(int argc, char **argv) {\n')
        f.write('    node *global_syms[%s] = {0};\n' % (global_sym_count))
        f.write('    context ctx(%s, global_syms), *globals = &ctx;\n' % (global_sym_count))
        f.write('    init_context(&ctx, argc, argv);\n')

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

def flatten_node_list(stmts):
    ctx = Flattener()
    for i in stmts:
        i = i.reduce_internal(ctx)
        ctx.statements.append(Edge(i))
    stmts = [v.value for v in ctx.statements]

    return stmts, ctx.functions

class Flattener:
    def __init__(self):
        self.statements = []
        self.functions = []
        self.temp_id = 0

    def get_temp_name(self):
        self.temp_id += 1
        return 'temp_%02i' % self.temp_id

    def get_temp(self):
        self.temp_id += 1
        return Identifier('temp_%02i' % self.temp_id)

    def flatten_edge(self, edge):
        node = edge()
        node = node.reduce_internal(self)
        edge.set(node)
        if not node.is_atom():
            temp = self.get_temp()
            edge.set(temp)
            self.statements.append(Edge(Assign(temp, node, 'node')))

    def flatten_list(self, node_list):
        old_stmts = self.statements
        self.statements = []
        for edge in node_list:
            edge.set(edge().reduce_internal(self))
            self.statements.append(edge)
        [statements, self.statements] = self.statements, old_stmts
        return statements

    def add_statement(self, stmt):
        stmt = stmt.reduce_internal(self)
        self.statements.append(Edge(stmt))

    def get_sym_id(self, scope, name):
        if name in self.symbol_idx[scope]:
            return self.symbol_idx[scope][name]
        return self.symbol_idx[scope]['$undefined']

    def get_binding(self, name):
        if self.in_function:
            if name in self.globals_set:
                scope = 'global'
            else:
                scope = 'local'
        elif self.in_class:
            scope = 'class'
        else:
            scope = 'global'
        return scope

    def index_global_class_symbols(self, node, globals_set, class_set):
        if isinstance(node, ast.Global):
            for name in node.names:
                globals_set.add(name)
        # XXX make this check scope
        elif isinstance(node, ast.Name) and isinstance(node.ctx,
                (ast.Store, ast.AugStore)):
            globals_set.add(node.id)
            class_set.add(node.id)
        elif isinstance(node, (ast.FunctionDef, ast.ClassDef)):
            globals_set.add(node.name)
            class_set.add(node.name)
        elif isinstance(node, ast.Import):
            for name in node.names:
                globals_set.add(name.name)
        elif isinstance(node, (ast.For, ast.ListComp, ast.DictComp, ast.SetComp,
            ast.GeneratorExp)):
            # HACK: set self.iter_temp for the space in the symbol table
            node.iter_temp = self.get_temp_name()
            globals_set.add(node.iter_temp)
        for i in ast.iter_child_nodes(node):
            self.index_global_class_symbols(i, globals_set, class_set)

    def get_globals(self, node, globals_set, locals_set, all_vars_set):
        if isinstance(node, ast.Global):
            for name in node.names:
                globals_set.add(name)
        elif isinstance(node, ast.Name):
            all_vars_set.add(node.id)
            if isinstance(node.ctx, (ast.Store, ast.AugStore)):
                locals_set.add(node.id)
        elif isinstance(node, ast.arg):
            all_vars_set.add(node.arg)
            locals_set.add(node.arg)
        elif isinstance(node, (ast.For, ast.ListComp, ast.DictComp, ast.SetComp,
            ast.GeneratorExp)):
            locals_set.add(node.iter_temp)
        for i in ast.iter_child_nodes(node):
            self.get_globals(i, globals_set, locals_set, all_vars_set)

    def analyze_scoping(self):
        # Set up an index of all possible global/class symbols
        all_global_syms = set()
        all_class_syms = set()
        self.index_global_class_symbols(node, all_global_syms, all_class_syms)

        all_global_syms.add('$undefined')
        all_global_syms |= set(builtin_symbols)

        self.symbol_idx = {
            scope: {symbol: idx for idx, symbol in enumerate(sorted(symbols))}
            for scope, symbols in [['class', all_class_syms], ['global', all_global_syms]]
        }
        self.global_sym_count = len(all_global_syms)
        self.class_sym_count = len(all_class_syms)

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
# self.expr = Edge(self, expr) # done implicitly by each class' constructor
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
def node(argstr=''):
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

        def reduce_internal(self, ctx):
            if hasattr(self, 'reduce'):
                self = self.reduce(ctx)

            self.flatten(ctx)

            return self

        def flatten(self, ctx):
            for (arg_type, arg_name) in args:
                if arg_type == ARG_EDGE:
                    edge = getattr(self, arg_name)
                    if edge:
                        ctx.flatten_edge(edge)
                elif arg_type == ARG_EDGE_LIST:
                    for edge in getattr(self, arg_name):
                        ctx.flatten_edge(edge)
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

@node('ref_type, args')
class Ref(Node):
    def __str__(self):
        return '(new(allocator) %s(%s))' % (self.ref_type, ', '.join(str(a) for a in self.args))

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
    # XXX temporary hack
    def setup(self):
        self.scope = 'local'
        self.idx = 0

    def __str__(self):
        if self.scope == 'global':
            return 'globals->load(%s)' % self.idx
        elif self.scope == 'class':
            return 'this->load("%s")' % self.name
        return 'ctx.load(%s)' % self.idx

@node('name, &expr')
class Store(Node):
    # XXX temporary hack
    def setup(self):
        self.scope = 'local'
        self.idx = 0

    def __str__(self):
        if self.scope == 'global':
            return 'globals->store(%s, %s)' % (self.idx, self.expr())
        elif self.scope == 'class':
            return 'this->store("%s", %s)' % (self.name, self.expr())
        return 'ctx.store(%s, %s)' % (self.idx, self.expr())

@node('name, attr, &expr')
class StoreAttr(Node):
    def __str__(self):
        return '%s->__setattr__(%s, %s)' % (self.name, self.attr, self.expr())

@node('&expr, &index, &value')
class StoreSubscript(Node):
    def __str__(self):
        return '%s->__setitem__(%s, %s)' % (self.expr(), self.index(), self.value())

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
        name = ctx.get_temp()
        ctx.add_statement(Assign(name, Ref('list', [len(self.items)]), 'list'))
        for i, item in enumerate(self.items):
            ctx.add_statement(StoreSubscriptDirect(name, i, item()))
        return name

@node('*items')
class Tuple(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
        ctx.add_statement(Assign(name, Ref('tuple', [len(self.items)]), 'tuple'))
        for i, item in enumerate(self.items):
            ctx.add_statement(StoreSubscriptDirect(name, i, item()))
        return name

@node('*keys, *values')
class Dict(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
        ctx.add_statement(Assign(name, Ref('dict', []), 'dict'))
        for k, v in zip(self.keys, self.values):
            ctx.add_statement(StoreSubscript(name, k(), v()))
        return name

@node('*items')
class Set(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
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
        return '%s->__call__(globals, &ctx, %s, %s)' % (self.func(), self.args(), self.kwargs())

@node('&expr, &true_expr, &false_expr')
class IfExp(Node):
    def reduce(self, ctx):
        temp = ctx.get_temp()
        ctx.add_statement(Assign(temp, NullConst(), 'node'))
        ctx.add_statement(If(self.expr(), [Assign(temp, self.true_expr(), 'node')],
            [Assign(temp, self.false_expr(), 'node')]))
        return temp

@node('op, &lhs_expr, &rhs_expr')
class BoolOp(Node):
    def reduce(self, ctx):
        temp = ctx.get_temp()
        ctx.add_statement(Assign(temp, self.lhs_expr(), 'node'))
        expr = self.expr()
        if self.op == 'or':
            expr = UnaryOp('__not__', expr)
        ctx.add_statement(If(expr, [Assign(temp, self.rhs_expr(), 'node')], []))
        return self.temp

    def __str__(self):
        rhs_stmts = block_str(self.rhs_stmts)
        body =  """if ({op}{lhs_expr}->bool_value()) {{
{rhs_stmts}
    {temp} = {rhs_expr};
}}""".format(op='!' if self.op == 'or' else '', lhs_expr=self.lhs_expr(),
        temp=self.temp.name, rhs_stmts=rhs_stmts, rhs_expr=self.rhs_expr())
        return body

@node('&target, &expr, target_type')
class Assign(Node):
    def __str__(self):
        target_type = ('%s *' % self.target_type) if self.target_type else ''
        return '%s%s = %s' % (target_type, self.target(), self.expr())

@node('&expr, $true_stmts, $false_stmts')
class If(Node):
    def __str__(self):
        stmts = block_str(self.true_stmts)
        body =  """if ({expr}->bool_value()) {{
{stmts}
}}""".format(expr=self.expr(), stmts=stmts)
        if self.false_stmts:
            stmts = block_str(self.false_stmts)
            body +=  """ else {{
{stmts}
}}""".format(stmts=stmts)
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
        temp = l.reduce(ctx)
        iter_name = ctx.get_temp()
        ctx.add_statement(Store(iter_name, UnaryOp('__iter__', self.iter())))

        # Construct body of while loop that implements comprehension
        # Get next item of iterable
        item = MethodCall(Load(iter_name), 'next', [])
        stmts = [If(UnaryOp('__not__', item), [Break()], [])]

        # Unpack arguments
        if isinstance(self.target, tuple):
            for i, target in enumerate(self.target):
                stmts += [Store(target, '%s->__getitem__(%s)' % (item, i))]
        else:
            stmts += [Store(self.target, item)]

        # Condtional
        if self.cond:
            stmts += [If(UnaryOp('__not__', self.cond()), [Continue()], [])]

        if self.comp_type == 'set':
            stmts += [MethodCall(temp, 'add', [self.expr()])]
        elif self.comp_type == 'dict':
            stmts += [MethodCall(temp, '__setitem__', [self.expr(), self.expr2()])]
        else:
            stmts += [MethodCall(temp, 'append', [self.expr()])]

        w = While(None, stmts)

        ctx.add_statement(l)

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
        item = MethodCall(Load(iter_name), 'next', [])
        stmts = [If(UnaryOp('__not__', item), [Break()], [])]

        # Unpack arguments
        if isinstance(self.target, tuple):
            for i, target in enumerate(self.target):
                stmts += [Store(target, '%s->__getitem__(%s)' % (item, i))]
        else:
            stmts += [Store(self.target, item)]

        w = While(None, stmts)

        return w

@node('&test, $stmts')
class While(Node):
    def reduce(self, ctx):
        test = If(UnaryOp('__not__', self.test()), [Break()], [])
        self.stmts = [Edge(test)] + self.stmts
        self.test = None
        return self

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
        return 'collect_garbage(&ctx, %s)' % (self.expr() if self.expr else 'NULL')

@node('args, *defaults')
class Arguments(Node):
    def reduce(self, ctx):
        new_def = [None] * (len(self.args) - len(self.defaults))
        defaults = new_def + self.defaults
        name_strings = [Edge(StringConst(a)) for a in self.args]

        arg_unpacking = []
        for i, (arg, default, name) in enumerate(zip(self.args, defaults, name_strings)):
            arg_unpacking += [IfExp()]
            if default:
                arg_value = '(args->len() > %d) ? args->items[%d] : %s' % (i, i, default())
            else:
                arg_value = 'args->__getitem__(%d)' % i
            if not self.no_kwargs:
                arg_value = '(kwargs && kwargs->lookup(%s)) ? kwargs->lookup(%s) : %s' % \
                    (name(), name(), arg_value)
            arg_unpacking += [Edge(Store(arg, arg_value))]
        if self.no_kwargs:
            arg_unpacking = '    if (kwargs && kwargs->len()) error("function does not take keyword arguments");\n' + arg_unpacking

        return self

    def __str__(self):
        arg_unpacking = block_str(arg_unpacking)
        return arg_unpacking

@node('name, &args, $stmts, exp_name, local_count')
class FunctionDef(Node):
    def setup(self):
        self.exp_name = self.exp_name if self.exp_name else self.name
        self.exp_name = 'fn_%s' % self.exp_name # make sure no name collisions

    def reduce(self, ctx):
        ctx.functions += [self]
        return Store(self.name, Ref('function_def', [Identifier(self.exp_name)]))

    def __str__(self):
        stmts = block_str(self.stmts)
        arg_unpacking = str(self.args())
        body = """
node *{name}(context *globals, context *parent_ctx, tuple *args, dict *kwargs) {{
    node *local_syms[{local_count}] = {{0}};
    context ctx(parent_ctx, {local_count}, local_syms);
{arg_unpacking}
{stmts}
}}""".format(name=self.exp_name, local_count=self.local_count,
        arg_unpacking=arg_unpacking, stmts=stmts)
        return body

@node('name, $stmts')
class ClassDef(Node):
    def setup(self):
        self.class_name = 'class_%s' % self.name

    def reduce(self, ctx):
        ctx.functions += [self]
        return Store(self.name, Ref(self.class_name, []))

    def __str__(self):
        stmts = block_str(self.stmts, spaces=8)
        body = """
class {cname} : public class_def {{
public:
    {cname}() {{
{stmts}
    }}
    virtual std::string repr() {{
        return std::string("<class '{name}'>");
    }}
}};
""".format(name=self.name, cname=self.class_name, stmts=stmts)
        return body
