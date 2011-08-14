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

def block_str(stmts):
    return '\n'.join('    %s;' % s for s in stmts)

all_ints = set()
def register_int(value):
    global all_ints
    all_ints |= {value}

all_strings = {}
def register_string(value):
    global all_strings
    if value in all_strings:
        return all_strings[value]
    all_strings[value] = len(all_strings)
    return all_strings[value]

def int_name(i):
    return 'int_singleton_neg%d' % -i if i < 0 else 'int_singleton_%d' % i

def export_consts(f):
    global all_ints, all_strings

    for i in all_ints:
        f.write('int_const_singleton %s(%sll);\n' % (int_name(i), i))

    for k, v in all_strings.items():
        f.write('string_const_singleton string_singleton_%s("%s");\n' % (v, repr(k)[1:-1]))

class Node:
    def is_atom(self):
        return False

class NoneConst(Node):
    def __init__(self):
        pass

    def __str__(self):
        return '&none_singleton'

class BoolConst(Node):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return '&bool_singleton_%s' % self.value

class IntConst(Node):
    def __init__(self, value):
        self.value = value
        register_int(value)

    def __str__(self):
        return '&%s' % int_name(self.value)

class StringConst(Node):
    def __init__(self, value):
        self.value = value
        self.id = register_string(value)

    def __str__(self):
        return '&string_singleton_%s' % self.id

class Identifier(Node):
    def __init__(self, name):
        self.name = name

    def is_atom(self):
        return True

    def __str__(self):
        return self.name

class Ref(Node):
    def __init__(self, ref_type, *args):
        self.ref_type = ref_type
        self.args = args

    def __str__(self):
        return '(new(allocator) %s(%s))' % (self.ref_type, ', '.join(str(a) for a in self.args))

class UnaryOp(Node):
    def __init__(self, op, rhs):
        self.op = op
        self.rhs = rhs

    def __str__(self):
        return '%s->%s()' % (self.rhs, self.op)

class BinaryOp(Node):
    def __init__(self, op, lhs, rhs):
        self.op = op
        self.lhs = lhs
        self.rhs = rhs

    def __str__(self):
        return '%s->%s(%s)' % (self.lhs, self.op, self.rhs)

class Load(Node):
    def __init__(self, name, is_global):
        self.name = name
        self.is_global = is_global

    def __str__(self):
        if self.is_global:
            return 'globals->load("%s")' % self.name
        else:
            return 'ctx->load("%s")' % self.name

class Store(Node):
    def __init__(self, name, expr, is_global):
        self.name = name
        self.expr = expr
        self.is_global = is_global

    def __str__(self):
        return '%s->store("%s", %s)' % ('globals' if self.is_global else 'ctx',
                self.name, self.expr)

class StoreAttr(Node):
    def __init__(self, name, attr, expr):
        self.name = name
        self.attr = attr
        self.expr = expr

    def __str__(self):
        return '%s->__setattr__(%s, %s)' % (self.name, self.attr, self.expr)

class StoreSubscript(Node):
    def __init__(self, expr, index, value):
        self.expr = expr
        self.index = index
        self.value = value

    def __str__(self):
        return '%s->__setitem__(%s, %s)' % (self.expr, self.index, self.value)

class DeleteSubscript(Node):
    def __init__(self, expr, index):
        self.expr = expr
        self.index = index

    def __str__(self):
        return '%s->__delitem__(%s)' % (self.expr, self.index)

class List(Node):
    def __init__(self, items):
        self.items = items

    def flatten(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('list'), target_type='list')]
        for i in self.items:
            # XXX HACK: add just some C++ text instead of syntax nodes...
            ctx.statements += ['%s->append(%s)' % (name, i)]
        return name

    def __str__(self):
        return ''

class Dict(Node):
    def __init__(self, keys, values):
        self.keys = keys
        self.values = values

    def flatten(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('dict'), target_type='dict')]
        for k, v in zip(self.keys, self.values):
            # XXX HACK: add just some C++ text instead of syntax nodes...
            ctx.statements += ['%s->__setitem__(%s, %s)' % (name, k, v)]
        return name

    def __str__(self):
        return ''

class Set(Node):
    def __init__(self, items):
        self.items = items

    def flatten(self, ctx):
        name = ctx.get_temp()
        ctx.statements += [Assign(name, Ref('set'), target_type='set')]
        for i in self.items:
            # XXX HACK: add just some C++ text instead of syntax nodes...
            ctx.statements += ['%s->add(%s)' % (name, i)]
        return name

    def __str__(self):
        return ''

class Slice(Node):
    def __init__(self, expr, start, end, step):
        self.expr = expr
        self.start = start
        self.end = end
        self.step = step

    def __str__(self):
        return '%s->__slice__(%s, %s, %s)' % (self.expr, self.start, self.end, self.step)

class Subscript(Node):
    def __init__(self, expr, index):
        self.expr = expr
        self.index = index

    def __str__(self):
        return '%s->__getitem__(%s)' % (self.expr, self.index)

class Attribute(Node):
    def __init__(self, expr, attr):
        self.expr = expr
        self.attr = attr

    def __str__(self):
        return '%s->__getattr__(%s)' % (self.expr, self.attr)

class Call(Node):
    def __init__(self, func, args, kwargs):
        self.func = func
        self.args = args
        self.kwargs = kwargs

    def __str__(self):
        return '%s->__call__(globals, ctx, %s, %s)' % (self.func, self.args, self.kwargs)

class IfExp(Node):
    def __init__(self, expr, true_stmts, true_expr, false_stmts, false_expr):
        self.expr = expr
        self.true_stmts = true_stmts
        self.true_expr = true_expr
        self.false_stmts = false_stmts
        self.false_expr = false_expr

    def flatten(self, ctx):
        self.temp = ctx.get_temp()
        ctx.statements += [Assign(self.temp, 'NULL'), self]
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
}}
""".format(expr=self.expr, temp=self.temp.name, true_stmts=true_stmts,
        true_expr=self.true_expr, false_stmts=false_stmts, false_expr=self.false_expr)
        return body

class BoolOp(Node):
    def __init__(self, op, lhs_expr, rhs_stmts, rhs_expr):
        self.op = op
        self.lhs_expr = lhs_expr
        self.rhs_stmts = rhs_stmts
        self.rhs_expr = rhs_expr

    # XXX hack
    def flatten(self, ctx, statements):
        self.temp = ctx.get_temp()
        statements += [Assign(self.temp, self.lhs_expr), self]
        return self.temp

    def __str__(self):
        rhs_stmts = block_str(self.rhs_stmts)
        body =  """if ({op}{lhs_expr}->bool_value()) {{
    {rhs_stmts}
    {temp} = {rhs_expr};
}}
""".format(op='!' if self.op == 'or' else '', lhs_expr=self.lhs_expr,
        temp=self.temp.name, rhs_stmts=rhs_stmts, rhs_expr=self.rhs_expr)
        return body

class Assign(Node):
    def __init__(self, target, expr, target_type='node'):
        self.target = target
        self.expr = expr
        self.target_type = target_type

    def __str__(self):
        if self.target_type is None:
            return '%s = %s' % (self.target, self.expr)
        else:
            return '%s *%s = %s' % (self.target_type, self.target, self.expr)

class If(Node):
    def __init__(self, expr, stmts, else_block):
        self.expr = expr
        self.stmts = stmts
        self.else_block = else_block

    def __str__(self):
        stmts = block_str(self.stmts)
        body =  """if ({expr}->bool_value()) {{
{stmts}
}}
""".format(expr=self.expr, stmts=stmts)
        if self.else_block:
            stmts = block_str(self.else_block)
            body +=  """else {{
    {stmts}
    }}
    """.format(expr=self.expr, stmts=stmts)
        return body

class ListComp(Node):
    def __init__(self, target, iter, stmts, expr):
        self.target = target
        self.iter = iter
        self.stmts = stmts
        self.expr = expr

    def flatten(self, ctx):
        l = List([])
        self.temp = l.flatten(ctx)
        ctx.statements += [self]
        return self.temp

    def __str__(self):
        stmts = block_str(self.stmts)
        arg_unpacking = []
        if isinstance(self.target, list):
            for i, arg in enumerate(self.target):
                arg_unpacking += [Store(arg.id, '(*__iter)->__getitem__(%s)' % i, False)]
        else:
            arg_unpacking = [Store(self.target, '*__iter', False)]
        arg_unpacking = block_str(arg_unpacking)
        body = """
for (node_list::iterator __iter = {iter}->list_value()->begin(); __iter != {iter}->list_value()->end(); __iter++) {{
{arg_unpacking}
{stmts}
    {temp}->append({expr});
}}
""".format(iter=self.iter, arg_unpacking=arg_unpacking, stmts=stmts, temp=self.temp, expr=self.expr)
        return body

class Break(Node):
    def __init__(self):
        pass

    def is_atom(self):
        return True

    def __str__(self):
        return 'break'

class Continue(Node):
    def __init__(self):
        pass

    def is_atom(self):
        return True

    def __str__(self):
        return 'continue'

class For(Node):
    def __init__(self, target, iter, stmts):
        self.target = target
        self.iter = iter
        self.stmts = stmts

    def __str__(self):
        stmts = block_str(self.stmts)
        arg_unpacking = []
        if isinstance(self.target, list):
            for i, (arg, is_global) in enumerate(self.target):
                arg_unpacking += [Store(arg, '(*__iter)->__getitem__(%s)' % i, is_global)]
        else:
            arg_unpacking = [Store(self.target[0], '*__iter', self.target[1])]
        arg_unpacking = block_str(arg_unpacking)
        # XXX sorta weird?
        body = """
for (node_list::iterator __iter = {iter}->list_value()->begin(); __iter != {iter}->list_value()->end(); __iter++) {{
{arg_unpacking}
{stmts}
    collect_garbage(ctx, false);
}}
""".format(iter=self.iter, arg_unpacking=arg_unpacking, stmts=stmts)
        return body

class While(Node):
    def __init__(self, test_stmts, test, stmts):
        self.test_stmts = test_stmts
        self.test = test
        self.stmts = stmts

    def __str__(self):
        # XXX Super hack: too lazy to do this properly now
        dup_test_stmts = copy.deepcopy(self.test_stmts)
        assert isinstance(dup_test_stmts[-1], Assign)
        dup_test_stmts[-1].target_type = None

        test_stmts = block_str(self.test_stmts)
        dup_test_stmts = block_str(dup_test_stmts)
        stmts = block_str(self.stmts)
        body = """
{test_stmts}
while ({test}->bool_value())
{{
{stmts}
    collect_garbage(ctx, false);
{dup_test_stmts}
}}
""".format(test_stmts=test_stmts, dup_test_stmts=dup_test_stmts, test=self.test, stmts=stmts)
        return body

class Return(Node):
    def __init__(self, value):
        self.value = value
        if self.value is None:
            self.value = NoneConst()

    def __str__(self):
        body = """
        collect_garbage(ctx, true);
        return %s;
""" % self.value
        return body

class Assert(Node):
    def __init__(self, expr, lineno):
        self.expr = expr
        self.lineno = lineno

    def __str__(self):
        body = """if (!{expr}->bool_value()) {{
    error("assert failed at line {lineno}");
}}
""".format(expr=self.expr, lineno=self.lineno)
        return body

class Arguments(Node):
    def __init__(self, args, defaults):
        self.args = args
        self.defaults = defaults

    def flatten(self, ctx):
        new_def = [None] * (len(self.args) - len(self.defaults))
        self.defaults = new_def + self.defaults
        self.name_strings = [StringConst(a) for a in self.args]
        return self

    def __str__(self):
        arg_unpacking = []
        for i, (arg, default, name) in enumerate(zip(self.args, self.defaults, self.name_strings)):
            if default:
                arg_unpacking += [Store(arg, 'kwargs->lookup(%s) ? kwargs->lookup(%s) '
                    ': (args->len() > %s ? args->__getitem__(%s) : %s)' %
                    (name, name, i, i, default), False)]
            else:
                arg_unpacking += [Store(arg, 'args->__getitem__(%s)' % i, False)]
        return block_str(arg_unpacking)

class FunctionDef(Node):
    def __init__(self, name, args, stmts, exp_name=None):
        self.name = name
        self.exp_name = exp_name if exp_name else name
        self.exp_name = 'fn_%s' % self.exp_name # make sure no name collisions
        self.args = args
        self.stmts = stmts

    def flatten(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref('function_def', Identifier(self.exp_name)), False)]

    def __str__(self):
        stmts = block_str(self.stmts)
        arg_unpacking = str(self.args)
        body = """
node *{name}(context *globals, context *parent_ctx, list *args, dict *kwargs) {{
    context *ctx = new(allocator) context(parent_ctx);
{arg_unpacking}
{stmts}
    return &none_singleton;
}}""".format(name=self.exp_name, arg_unpacking=arg_unpacking, stmts=stmts)
        return body

class ClassDef(Node):
    def __init__(self, name, stmts):
        self.name = name
        self.stmts = stmts

    def flatten(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref('class_def', '"%s"' % self.name, Identifier('_%s__create__' % self.name)), False)]

    def __str__(self):
        stmts = block_str(self.stmts)
        body = """
void _{name}__create__(class_def *ctx) {{
{stmts}
}}""".format(name=self.name, stmts=stmts)
        return body
