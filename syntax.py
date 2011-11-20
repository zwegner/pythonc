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

def indent(stmts):
    stmts = [str(s) for s in stmts]
    stmts = '\n'.join('%s%s' % (s, ';' if s and not s.endswith('}') else '') for s in stmts).splitlines()
    return '\n'.join('    %s' % s for s in stmts)

def block_str(stmts):
    return indent(s() for s in stmts)

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

class Node:
    def is_atom(self):
        return False

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
        # XXX value.add_use(self)

    def __call__(self):
        return self.value

    def set(self, value):
        # XXX self.value.remove_use(self)
        self.value = value
        # XXX value.add_use(self)

# Weird decorator: a given arg string represents a standard form for arguments
# to Node subclasses. We use these notations:
# op, &expr, &*stmts, *args
# op -> normal attribute
# &expr -> edge attribute, will create an Edge object (used for linking to other Nodes)
# &*stmts -> python list of edges
# *args -> varargs, additional args are assigned to this attribute
def node(argstr='', atom=False):
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
            if hasattr(self, 'setup'):
                self.setup()

        def is_atom(self):
            return atom

        node.__init__ = __init__
        node.is_atom = is_atom

        return node

    return attach

@node(atom=True)
class NullConst(Node):
    def __str__(self):
        return 'NULL'

@node(atom=True)
class NoneConst(Node):
    def __str__(self):
        return '(&none_singleton)'

@node('value', atom=True)
class BoolConst(Node):
    def __str__(self):
        return '(&bool_singleton_%s)' % self.value

@node('value', atom=True)
class IntConst(Node):
    def setup(self):
        register_int(self.value)

    def __str__(self):
        return '(&%s)' % int_name(self.value)

@node('value', atom=True)
class StringConst(Node):
    def setup(self):
        self.id = register_string(self.value)

    def __str__(self):
        return '(&string_singleton_%s)' % self.id

@node('value', atom=True)
class BytesConst(Node):
    def setup(self):
        self.id = register_bytes(self.value)

    def __str__(self):
        return '(&bytes_singleton_%s)' % self.id

@node('name', atom=True)
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

@node('name, binding')
class Load(Node):
    def setup(self):
        self.scope, self.idx = self.binding

    def __str__(self):
        if self.scope == 'global':
            return 'globals->load(%s)' % self.idx
        elif self.scope == 'class':
            return 'class_ctx->load("%s")' % self.name
        return 'ctx.load(%s)' % self.idx

@node('name, &expr, binding')
class Store(Node):
    def setup(self):
        self.scope, self.idx = self.binding

    def __str__(self):
        if self.scope == 'global':
            return 'globals->store(%s, %s)' % (self.idx, self.expr())
        elif self.scope == 'class':
            return 'class_ctx->store("%s", %s)' % (self.name, self.expr())
        return 'ctx.store(%s, %s)' % (self.idx, self.expr())

@node('name, &expr, binding')
class StoreAttr(Node):
    def __init__(self, name, attr, expr):
        self.name = name
        self.attr = attr
        self.expr = expr

    def __str__(self):
        return '%s->__setattr__(%s, %s)' % (self.name, self.attr, self.expr)

@node('&expr, &index, &value')
class StoreSubscript(Node):
    def __str__(self):
        return '%s->__setitem__(%s, %s)' % (self.expr(), self.index(), self.value())

@node('&expr, &index')
class DeleteSubscript(Node):
    def __str__(self):
        return '%s->__delitem__(%s)' % (self.expr(), self.index())

@node('&*items')
class List(Node):
    def flatten(self, ctx):
        list_name = ctx.get_temp()
        name = ctx.get_temp()
        ctx.statements += ['node *%s[%d]' % (list_name, len(self.items))]
        for i, item in enumerate(self.items):
            ctx.statements += ['%s[%d] = %s' % (list_name, i, item())]
        ctx.statements += [Assign(name, Ref('list', len(self.items), list_name), 'list')]
        return name

    def __str__(self):
        return ''

@node('items')
class Tuple(Node):
    # XXX clean up
    def setup(self):
        if isinstance(self.items, list):
            self.items = [Edge(self, item) for item in self.items]
        else:
            self.items = Edge(self, self.items)

    def flatten(self, ctx):
        list_name = ctx.get_temp()
        name = ctx.get_temp()
        if isinstance(self.items, list):
            ctx.statements += ['node *%s[%d]' % (list_name, len(self.items))]
            for i, item in enumerate(self.items):
                ctx.statements += ['%s[%d] = %s' % (list_name, i, item())]
            ctx.statements += [Assign(name, Ref('tuple', len(self.items), list_name), 'tuple')]
        else:
            iter_name = ctx.get_temp()
            ctx.statements += [
                'node_list %s' % list_name,
                'node *%s = %s->__iter__()' % (iter_name, self.items()),
                'while (node *item = %s->next()) %s.push_back(item)' % (iter_name, list_name),
                Assign(name, Ref('tuple', '%s.size()' % list_name, '&%s[0]' % list_name), 'tuple')
            ]
        return name

    def __str__(self):
        return ''

@node('&*keys, &*values')
class Dict(Node):
    def flatten(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('dict'), 'dict')]
        for k, v in zip(self.keys, self.values):
            ctx.statements += ['%s->__setitem__(%s, %s)' % (name, k(), v())]
        return name

    def __str__(self):
        return ''

@node('&*items')
class Set(Node):
    def flatten(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('set'), 'set')]
        for i in self.items:
            ctx.statements += ['%s->add(%s)' % (name, i())]
        return name

    def __str__(self):
        return ''

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

@node('&expr, &*true_stmts, &true_expr, &*false_stmts, &false_expr')
class IfExp(Node):
    def flatten(self, ctx):
        self.temp = ctx.get_temp()
        ctx.statements += [Assign(self.temp, NullConst(), 'node'), self]
        return self.temp

    def __str__(self):
        true_stmts = block_str(self.true_stmts)
        false_stmts = block_str(self.false_stmts)
        body =  """if ({expr}->bool_value()) {{
{true_stmts}
    {temp} = {true_expr};
}} else {{
{false_stmts}
    {temp} = {false_expr};
}}""".format(expr=self.expr(), temp=self.temp.name, true_stmts=true_stmts,
        true_expr=self.true_expr(), false_stmts=false_stmts, false_expr=self.false_expr())
        return body

@node('op, &lhs_expr, &*rhs_stmts, &rhs_expr')
class BoolOp(Node):
    def flatten(self, ctx, statements):
        self.temp = ctx.get_temp()
        statements += [Assign(self.temp, self.lhs_expr, 'node'), self]
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

@node('comp_type, target, &iter, iter_name, iter_binding, &*cond_stmts, &cond, &*expr_stmts, &expr, &expr2')
class Comprehension(Node):
    def flatten(self, ctx):
        if self.comp_type == 'set':
            l = Set([])
        elif self.comp_type == 'dict':
            l = Dict([], [])
        else:
            l = List([])
        self.temp = l.flatten(ctx)
        ctx.statements += [Assign(self.iter_name, '%s->__iter__()' % self.iter(), 'node')]
        ctx.statements += [self]
        # HACK: prevent iterator from being garbage collected
        self.iter_store = Store(self.iter_name, self.iter_name, self.iter_binding)
        return self.temp

    def __str__(self):
        cond_stmts = block_str(self.cond_stmts)
        expr_stmts = block_str(self.expr_stmts)
        arg_unpacking = []
        # XXX HACK
        if isinstance(self.target[0], tuple):
            for i, (target, binding) in enumerate(self.target):
                arg_unpacking += [Edge(self, Store(target, 'item->__getitem__(%s)' % i, binding))]
        else:
            target, binding = self.target
            arg_unpacking = [Edge(self, Store(target, 'item', binding))]
        arg_unpacking = block_str(arg_unpacking)
        if self.cond:
            cond = 'if (!(%s)->bool_value()) continue;' % self.cond()
        else:
            cond = ''
        if self.comp_type == 'set':
            adder = '%s->add(%s);' % (self.temp, self.expr())
        elif self.comp_type == 'dict':
            adder = '%s->__setitem__(%s, %s);' % (self.temp, self.expr(), self.expr2())
        else:
            adder = '%s->append(%s);' % (self.temp, self.expr())
        body = """
{iter_store};
while (node *item = {iter}->next()) {{
{arg_unpacking}
{cond_stmts}
{cond}
{expr_stmts}
    {adder}
}}""".format(iter=self.iter_name, iter_store=self.iter_store, arg_unpacking=arg_unpacking,
        cond_stmts=cond_stmts, cond=cond, expr_stmts=expr_stmts, adder=adder)
        return body

@node(atom=True)
class Break(Node):
    def __str__(self):
        return 'break'

@node(atom=True)
class Continue(Node):
    def __str__(self):
        return 'continue'

@node('target, &iter, &*stmts, iter_name, iter_binding')
class For(Node):
    # XXX clean up
    def setup(self):
        self.iter_name = Edge(self, Identifier(self.iter_name))

    def flatten(self, ctx):
        ctx.statements += [Assign(self.iter_name(), '%s->__iter__()' % self.iter(), 'node')]
        # HACK: prevent iterator from being garbage collected
        self.iter_store = Edge(self, Store(self.iter_name(), self.iter_name(), self.iter_binding))
        return self

    def __str__(self):
        stmts = block_str(self.stmts)
        arg_unpacking = []
        if isinstance(self.target, list):
            for i, (arg, binding) in enumerate(self.target):
                arg_unpacking += [Edge(self, Store(arg, 'item->__getitem__(%s)' % i, binding))]
        else:
            arg, binding = self.target
            arg_unpacking = [Edge(self, Store(arg, 'item', binding))]
        arg_unpacking = block_str(arg_unpacking)
        # XXX sorta weird?
        body = """
{iter_store};
while (node *item = {iter}->next()) {{
{arg_unpacking}
{stmts}
}}""".format(iter=self.iter_name(), iter_store=self.iter_store(), arg_unpacking=arg_unpacking,
        stmts=stmts)
        return body

@node('&*test_stmts, &test, &*stmts')
class While(Node):
    def __str__(self):
        # XXX Super hack: too lazy to do this properly now
        dup_test_stmts = copy.deepcopy(self.test_stmts)
        assign = dup_test_stmts[-1]
        assert isinstance(assign, Edge) and isinstance(assign(), Assign)
        assign().target_type = None

        test_stmts = block_str(self.test_stmts)
        dup_test_stmts = block_str(dup_test_stmts)
        stmts = block_str(self.stmts)
        body = """
{test_stmts}
while ({test}->bool_value())
{{
{stmts}
{dup_test_stmts}
}}""".format(test_stmts=test_stmts, dup_test_stmts=dup_test_stmts, test=self.test(), stmts=stmts)
        return body

@node('&value')
class Return(Node):
    def setup(self):
        if self.value is None:
            self.value = Edge(self, NoneConst())

    def __str__(self):
        body = """
collect_garbage(&ctx, %s);
return %s""" % (self.value(), self.value())
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

@node('args, binding, &*defaults')
class Arguments(Node):
    def flatten(self, ctx):
        new_def = [None] * (len(self.args) - len(self.defaults))
        self.defaults = new_def + self.defaults
        self.name_strings = [Edge(self, StringConst(a)) for a in self.args]
        return self

    def __str__(self):
        arg_unpacking = []
        for i, (arg, binding, default, name) in enumerate(zip(self.args, self.binding,
            self.defaults, self.name_strings)):
            if default:
                arg_value = '(args->len() > %d) ? args->items[%d] : %s' % (i, i, default())
            else:
                arg_value = 'args->__getitem__(%d)' % i
            if not self.no_kwargs:
                arg_value = '(kwargs && kwargs->lookup(%s)) ? kwargs->lookup(%s) : %s' % \
                    (name(), name(), arg_value)
            arg_unpacking += [Edge(self, Store(arg, arg_value, binding))]
        arg_unpacking = block_str(arg_unpacking)
        if self.no_kwargs:
            arg_unpacking = '    if (kwargs && kwargs->len()) error("function does not take keyword arguments");\n' + arg_unpacking
        return arg_unpacking

@node('name, &args, &*stmts, exp_name, binding, local_count')
class FunctionDef(Node):
    def setup(self):
        self.exp_name = self.exp_name if self.exp_name else self.name
        self.exp_name = 'fn_%s' % self.exp_name # make sure no name collisions

    def flatten(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref('function_def', Identifier(self.exp_name)), self.binding)]

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

@node('name, binding, &*stmts')
class ClassDef(Node):
    def flatten(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref('class_def', '"%s"' % self.name, Identifier('_%s__create__' % self.name)), self.binding)]

    def __str__(self):
        stmts = block_str(self.stmts)
        body = """
void _{name}__create__(class_def *class_ctx) {{
{stmts}
}}""".format(name=self.name, stmts=stmts)
        return body
