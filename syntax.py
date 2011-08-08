def block_str(stmts):
    return '\n'.join('    %s;' % s for s in stmts)

class Node:
    def is_atom(self):
        return False

class IntConst(Node):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return '(new int_const(%s))' % self.value

class StringConst(Node):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return '(new string_const("%s"))' % repr(self.value)[1:-1]

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
        return '(new %s(%s))' % (self.ref_type, ', '.join(str(a) for a in self.args))

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
    def __init__(self, name):
        self.name = name

    def __str__(self):
        return 'ctx->load("%s")' % self.name

class Store(Node):
    def __init__(self, name, expr):
        self.name = name
        self.expr = expr

    def __str__(self):
        return 'ctx->store("%s", %s)' % (self.name, self.expr)

class StoreAttr(Node):
    def __init__(self, name, attr, expr):
        self.name = name
        self.attr = attr
        self.expr = expr

    def __str__(self):
        return '%s->__setattr__(%s, %s)' % (self.name, self.attr, self.expr)

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
    def __init__(self, func, args):
        self.func = func
        self.args = args

    def __str__(self):
        return '%s->__call__(ctx, %s)' % (self.func, self.args)

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
        body =  """if (test_truth({expr})) {{
    {true_stmts}
    {temp} = {true_expr};
}} else {{
    {false_stmts}
    {temp} = {false_expr};
}}
""".format(expr=self.expr, temp=self.temp.name, true_stmts=true_stmts,
        true_expr=self.true_expr, false_stmts=false_stmts, false_expr=self.false_expr)
        return body


class Assign(Node):
    def __init__(self, target, expr, target_type='node'):
        self.target = target
        self.expr = expr
        self.target_type = target_type

    def __str__(self):
        return '%s *%s = %s' % (self.target_type, self.target, self.expr)

class If(Node):
    def __init__(self, expr, stmts, else_block):
        self.expr = expr
        self.stmts = stmts
        self.else_block = else_block

    def __str__(self):
        stmts = block_str(self.stmts)
        body =  """if (test_truth({expr})) {{
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

class For(Node):
    def __init__(self, expr, iter, stmts):
        self.expr = expr
        self.iter = iter
        self.stmts = stmts

    def __str__(self):
        stmts = block_str(self.stmts)
        arg_unpacking = [Store(self.expr, '*__iter')]
        arg_unpacking = block_str(arg_unpacking)
        # XXX sorta weird?
        body = """
if (!{iter}->is_list())
    error("cannot iterate over non-list");
for (node_list::iterator __iter = {iter}->begin(); __iter != {iter}->end(); __iter++) {{
{arg_unpacking}
{stmts}
}}
""".format(expr=self.expr, iter=self.iter, arg_unpacking=arg_unpacking, stmts=stmts)
        return body

class Return(Node):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        if self.value is not None:
            return 'return %s' % self.value
        else:
            return 'return'

class Assert(Node):
    def __init__(self, expr, lineno):
        self.expr = expr
        self.lineno = lineno

    def __str__(self):
        body =  """if (!test_truth({expr})) {{
    error("assert failed at line {lineno}");
}}
""".format(expr=self.expr, lineno=self.lineno)
        return body

class FunctionDef(Node):
    def __init__(self, name, args, stmts):
        self.name = name
        self.args = args
        self.stmts = stmts

    def flatten(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref('function_def', Identifier(self.name)))]

    def __str__(self):
        stmts = block_str(self.stmts)
        arg_unpacking = []
        for i, arg in enumerate(self.args.args):
            arg_unpacking += [Store(arg.arg, 'args->__getitem__(%s)' % IntConst(i))]
        arg_unpacking = block_str(arg_unpacking)
        body = """
node *{name}(context *parent_ctx, node *args) {{
    context *ctx = new context(parent_ctx);
{arg_unpacking}
{stmts}
    return NULL;
}}""".format(name=self.name, arg_unpacking=arg_unpacking, stmts=stmts)
        return body

class ClassDef(Node):
    def __init__(self, name, stmts):
        self.name = name
        self.stmts = stmts

    def flatten(self, ctx):
        ctx.functions += [self]
        return [Store(self.name, Ref('class_def', '"%s"' % self.name, Identifier('_%s__create__' % self.name)))]

    def __str__(self):
        stmts = block_str(self.stmts)
        body = """
void _{name}__create__(class_def *ctx) {{
{stmts}
}}""".format(name=self.name, stmts=stmts)
        return body
