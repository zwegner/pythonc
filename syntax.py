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

def export_consts(f):
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

class Flattener:
    def __init__(self):
        self.statements = []

    def get_temp_name(self):
        self.temp_id += 1
        return 'temp_%02i' % self.temp_id

    def get_temp(self):
        self.temp_id += 1
        return syntax.Identifier('temp_%02i' % self.temp_id)

    def flatten_edge(self, edge):
        node = edge()
        node.flatten()
        if node.is_atom():
            r = node
        else:
            temp = self.get_temp()
            self.statements.append(syntax.Assign(temp, node))
            r = temp
        node.forward(r)

    def flatten_list(self, node_list):
        old_stmts = self.statements
        self.statements = []
        for edge in node_list:
            edge().flatten(self)
        [statements, self.statements] = self.statements, old_stmts
        return statements

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
        assert False

# Edge class represents the use of a Node by another. This allows us to use
# value forwarding and such. It is used like so:
#
# self.expr = Edge(self, expr) # done implicitly by each class' constructor
# value = self.expr()
# self.expr.set(new_value)
class Edge:
    def __init__(self, node, value):
        self.node = node
        self.value = value
        value.add_use(self)

    def __call__(self):
        return self.value

    def set(self, value):
        self.value.remove_use(self)
        self.value = value
        value.add_use(self)

    def __str__(self):
        assert False

# Weird decorator: a given arg string represents a standard form for arguments
# to Node subclasses. We use these notations:
# op, &expr, &*stmts, *args
# op -> normal attribute
# &expr -> edge attribute, will create an Edge object (used for linking to other Nodes)
# &*stmts -> python list of edges
# *args -> varargs, additional args are assigned to this attribute
def node(argstr=''):
    args = argstr.split(',')
    new_args = []
    varargs = None
    for i, arg_name in enumerate(args):
        arg_name = arg_name.strip()
        if not arg_name:
            continue
        if arg_name[0] == '*':
            assert i == len(args) - 1
            varargs = arg_name[1:]
        else:
            new_args.append(arg_name)
    args = new_args

    atom = not any(a[0] == '&' for a in args)

    # Decorators must return a function. This adds __init__ and is_atom methods
    # to a Node subclass
    def attach(node):
        cmp = int.__ge__ if varargs else int.__eq__
        def __init__(self, *iargs):
            assert cmp(len(iargs), len(args)), 'bad args, expected %s(%s)' % (node.__name__, argstr)

            if varargs:
                setattr(self, varargs, iargs[len(args):])

            for k, v in zip(args, iargs):
                if k[0] == '&':
                    k = k[1:]
                    if k[0] == '*':
                        k = k[1:]
                        setattr(self, k, [Edge(self, item) for item in v])
                    else:
                        setattr(self, k, Edge(self, v) if v is not None else None)
                else:
                    setattr(self, k, v)
            self.uses = []
            if hasattr(self, 'setup'):
                self.setup()

        def flatten(self, ctx):
            for k in args:
                if k[0] == '&':
                    k = k[1:]
                    if k[0] == '*':
                        k = k[1:]
                        items = getattr(self, k)
                        for edge in items:
                            ctx.flatten_edge(edge)
                    else:
                        ctx.flatten_edge(getattr(self, k))

        def is_atom(self):
            return atom

        node.__init__ = __init__
        node.flatten = flatten
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

@node('ref_type, *args')
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
    def __str__(self):
        if self.scope == 'global':
            return 'globals->load(%s)' % self.idx
        elif self.scope == 'class':
            return 'this->load("%s")' % self.name
        return 'ctx.load(%s)' % self.idx

@node('name, &expr')
class Store(Node):
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

@node('&obj, method_name, &*args')
class MethodCall(Node):
    def __str__(self):
        args = ['%s' % a() for a in self.args]
        return '%s->%s(%s)' % (self.obj(), self.method_name, ', '.join(args))

@node('&*items')
class List(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('list', len(self.items)), 'list')]
        ctx.statements += [StoreSubscriptDirect(name, i, item()) for i, item in enumerate(self.items)]
        return name

@node('&*items')
class Tuple(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('tuple', len(self.items)), 'tuple')]
        ctx.statements += [StoreSubscriptDirect(name, i, item()) for i, item in enumerate(self.items)]
        return name

@node('&items')
class TupleFromIter(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
        iter_name = ctx.get_temp()
        ctx.statements += [
            Assign(name, Ref('tuple'), 'tuple'),
            Assign(iter_name, UnaryOp('__iter__', self.items()), 'node'),
            'while (node *item = %s->next()) %s->items.push_back(item)' % (iter_name, name),
        ]
        return name

@node('&*keys, &*values')
class Dict(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('dict'), 'dict')]
        ctx.statements += [StoreSubscript(name, k(), v()) for k, v in zip(self.keys, self.values)]
        return name

@node('&*items')
class Set(Node):
    def reduce(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('set'), 'set')]
        ctx.statements += [MethodCall(name, 'add', [i()]) for i in self.items]
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
        ctx.statements += [Assign(temp, NullConst(), 'node'), self]
        ctx.statements += [If(self.expr(), [Assign(temp, self.true_expr(), 'node')],
            [Assign(temp, self.false_expr(), 'node')])]
        return temp

@node('op, &lhs_expr, &rhs_expr')
class BoolOp(Node):
    def reduce(self, ctx, statements):
        temp = ctx.get_temp()
        statements += [Assign(temp, self.lhs_expr, 'node'), self]
        expr = self.expr()
        if self.op == 'or':
            expr = UnaryOp('__not__', expr)
        ctx.statements += [If(expr, [Assign(temp, self.true_expr(), 'node')], [])]
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

@node('&expr, &*stmts, &*else_block')
class If(Node):
    def __str__(self):
        stmts = block_str(self.stmts)
        body =  """if ({expr}->bool_value()) {{
{stmts}
}}""".format(expr=self.expr(), stmts=stmts)
        if self.else_block:
            stmts = block_str(self.else_block)
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
        ctx.statements += [Store(iter_name, UnaryOp('__iter__', self.iter()))]

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

        w = While(BoolConst(True), stmts)

        ctx.statements += [l]

        return self.temp

@node()
class Break(Node):
    def __str__(self):
        return 'break'

@node()
class Continue(Node):
    def __str__(self):
        return 'continue'

@node('target, &iter, &*stmts')
class For(Node):
    def reduce(self, ctx):
        self.iter_name = Edge(self, ctx.get_temp())
        ctx.statements += [Assign(self.iter_name(), UnaryOp('__iter__', self.iter()), 'node')]
        arg_unpacking = []
        if isinstance(self.target, list):
            for i, arg in enumerate(self.target):
                arg_unpacking += [Store(arg, 'item->__getitem__(%s)' % i)]
        else:
            arg_unpacking = [Store(self.target, 'item')]
        self.stmts = ctx.flatten_list(arg_unpacking + self.stmts)
        # HACK: prevent iterator from being garbage collected
        self.iter_store = Edge(self, Store(self.iter_name(), self.iter_name()))
        return self

    def __str__(self):
        stmts = block_str(self.stmts)
        body = """\
while (node *item = {iter}->next()) {{
{stmts}
}}""".format(iter=self.iter_name(), stmts=stmts)
        return body

@node('&test, &*stmts')
class While(Node):
    def reduce(self, ctx):
        self.stmts = ctx.flatten_list([If(UnaryOp('__not__', self.test), [Break()], [])] + self.stmts)

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
            self.value = Edge(self, NoneConst())

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

@node('args, &*defaults')
class Arguments(Node):
    def reduce(self, ctx):
        new_def = [None] * (len(self.args) - len(self.defaults))
        defaults = new_def + self.defaults
        name_strings = [Edge(self, StringConst(a)) for a in self.args]

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
            arg_unpacking += [Edge(self, Store(arg, arg_value))]
        if self.no_kwargs:
            arg_unpacking = '    if (kwargs && kwargs->len()) error("function does not take keyword arguments");\n' + arg_unpacking

        return self

    def __str__(self):
        arg_unpacking = block_str(arg_unpacking)
        return arg_unpacking

@node('name, &args, &*stmts, exp_name, local_count')
class FunctionDef(Node):
    def setup(self):
        self.exp_name = self.exp_name if self.exp_name else self.name
        self.exp_name = 'fn_%s' % self.exp_name # make sure no name collisions

    def reduce(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref('function_def', Identifier(self.exp_name)))]

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

@node('name, &*stmts')
class ClassDef(Node):
    def setup(self):
        self.class_name = 'class_%s' % self.name

    def reduce(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref(self.class_name))]

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
