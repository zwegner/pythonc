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

import ast

import syntax

inplace_op_table = {
    '__%s__' % x: '__i%s__' % x
    for x in ['add', 'and', 'floordiv', 'lshift', 'mod', 'mul', 'or', 'pow', 'rshift', 'sub', 'truediv', 'xor']
}

class TranslateError(Exception):
    def __init__(self, node, msg):
        super().__init__('error at line %s: %s' % (node.lineno, msg))

class Transformer(ast.NodeTransformer):
    def __init__(self):
        self.temp_id = 0
        self.statements = []
        self.in_class = False
        self.in_function = False
        self.globals_set = None

    def generic_visit(self, node):
        raise TranslateError(node, 'can\'t translate %s' % node)

    def visit_child_list(self, node):
        r = []
        for i in node:
            c = self.visit(i) 
            if isinstance(c, list):
                r.extend(c)
            else:
                r.append(c)
        return r

    def visit_Name(self, node):
        #assert isinstance(node.ctx, ast.Load)
        if node.id in ['True', 'False']:
            return syntax.BoolConst(node.id == 'True')
        elif node.id == 'None':
            return syntax.NoneConst()
        return syntax.Load(node.id)

    def visit_Num(self, node):
        if isinstance(node.n, float):
            raise TranslateError(node, 'Pythonc currently does not support float literals')
        assert isinstance(node.n, int)
        return syntax.IntConst(node.n)

    def visit_Str(self, node):
        assert isinstance(node.s, str)
        return syntax.StringConst(node.s)

    def visit_Bytes(self, node):
        assert isinstance(node.s, bytes)
        return syntax.BytesConst(node.s)

    # Unary Ops
    def visit_Invert(self, node): return '__invert__'
    def visit_Not(self, node): return '__not__'
    def visit_UAdd(self, node): return '__pos__'
    def visit_USub(self, node): return '__neg__'
    def visit_UnaryOp(self, node):
        op = self.visit(node.op)
        rhs = self.visit(node.operand)
        return syntax.UnaryOp(op, rhs)

    # Binary Ops
    def visit_Add(self, node): return '__add__'
    def visit_BitAnd(self, node): return '__and__'
    def visit_BitOr(self, node): return '__or__'
    def visit_BitXor(self, node): return '__xor__'
    def visit_Div(self, node): return '__truediv__'
    def visit_FloorDiv(self, node): return '__floordiv__'
    def visit_LShift(self, node): return '__lshift__'
    def visit_Mod(self, node): return '__mod__'
    def visit_Mult(self, node): return '__mul__'
    def visit_Pow(self, node): return '__pow__'
    def visit_RShift(self, node): return '__rshift__'
    def visit_Sub(self, node): return '__sub__'

    def visit_BinOp(self, node):
        op = self.visit(node.op)
        lhs = self.visit(node.left)
        rhs = self.visit(node.right)
        return syntax.BinaryOp(op, lhs, rhs)

    # Comparisons
    def visit_Eq(self, node): return '__eq__'
    def visit_NotEq(self, node): return '__ne__'
    def visit_Lt(self, node): return '__lt__'
    def visit_LtE(self, node): return '__le__'
    def visit_Gt(self, node): return '__gt__'
    def visit_GtE(self, node): return '__ge__'
    def visit_In(self, node): return '__contains__'
    def visit_NotIn(self, node): return '__ncontains__'
    def visit_Is(self, node): return '__is__'
    def visit_IsNot(self, node): return '__isnot__'

    def visit_Compare(self, node):
        assert len(node.ops) == 1
        assert len(node.comparators) == 1
        op = self.visit(node.ops[0])
        lhs = self.visit(node.left)
        rhs = self.visit(node.comparators[0])
        # Sigh--Python has these ordered weirdly
        if op in ['__contains__', '__ncontains__']:
            lhs, rhs = rhs, lhs
        return syntax.BinaryOp(op, lhs, rhs)

    # Bool ops
    def visit_And(self, node): return 'and'
    def visit_Or(self, node): return 'or'

    def visit_BoolOp(self, node):
        assert len(node.values) >= 2
        op = self.visit(node.op)
        rhs = self.visit(node.values[-1])
        for v in reversed(node.values[:-1]):
            lhs = self.visit(v)
            rhs = syntax.BoolOp(op, lhs, rhs)
        return rhs_expr

    def visit_IfExp(self, node):
        expr = self.visit(node.test)
        true_expr = self.visit(node.body)
        false_expr = self.visit(node.orelse)
        return syntax.IfExp(expr, true_stmts, true_expr, false_stmts, false_expr)

    def visit_List(self, node):
        items = [self.visit(i) for i in node.elts]
        return syntax.List(items)

    def visit_Tuple(self, node):
        items = [self.visit(i) for i in node.elts]
        return syntax.Tuple(items)

    def visit_Dict(self, node):
        keys = [self.visit(i) for i in node.keys]
        values = [self.visit(i) for i in node.values]
        return syntax.Dict(keys, values)

    def visit_Set(self, node):
        items = [self.visit(i) for i in node.elts]
        return syntax.Set(items)

    def visit_Subscript(self, node):
        l = self.visit(node.value)
        if isinstance(node.slice, ast.Index):
            index = self.visit(node.slice.value)
            return syntax.Subscript(l, index)
        elif isinstance(node.slice, ast.Slice):
            [start, end, step] = [self.visit(a) if a else syntax.NoneConst() for a in
                    [node.slice.lower, node.slice.upper, node.slice.step]]
            return syntax.Slice(l, start, end, step)

    def visit_Attribute(self, node):
        assert isinstance(node.ctx, ast.Load)
        l = self.visit(node.value)
        attr = syntax.Attribute(l, syntax.StringConst(node.attr))
        return attr

    def visit_Call(self, node):
        fn = self.visit(node.func)

        if node.starargs:
            assert not node.args
            args = syntax.TupleFromIter(self.visit(node.starargs))
        else:
            args = syntax.Tuple([self.visit(a) for a in node.args])

        assert not node.kwargs
        if node.keywords:
            keys = [syntax.StringConst(i.arg) for i in node.keywords]
            values = [self.visit(i.value) for i in node.keywords]
            kwargs = syntax.Dict(keys, values)
        else:
            kwargs = syntax.NullConst()

        return syntax.Call(fn, args, kwargs)

    def visit_Assign(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        value = self.visit(node.value)
        if isinstance(target, ast.Name):
            return [syntax.Store(target.id, value)]
        elif isinstance(target, ast.Tuple):
            if not all(isinstance(t, ast.Name) for t in target.elts):
                raise TranslateError(target, 'Pythonc does not yet support nested tuple assignment')
            stmts = []
            for i, t in enumerate(target.elts):
                stmts += [syntax.Store(t.id, syntax.Subscript(value, syntax.IntConst(i)))]
            return stmts
        elif isinstance(target, ast.Attribute):
            base = self.visit(target.value)
            return [syntax.StoreAttr(base, syntax.StringConst(target.attr), value)]
        elif isinstance(target, ast.Subscript):
            assert isinstance(target.slice, ast.Index)
            base = self.visit(target.value)
            index = self.visit(target.slice.value)
            return [syntax.StoreSubscript(base, index, value)]
        else:
            assert False

    def visit_AugAssign(self, node):
        op = self.visit(node.op)
        value = self.visit(node.value)
        op = inplace_op_table[op]
        if isinstance(node.target, ast.Name):
            target = node.target.id
            # XXX HACK: doesn't modify in place
            binop = syntax.BinaryOp(op, syntax.Load(target), value)
            return [syntax.Store(target, binop)]
        elif isinstance(node.target, ast.Attribute):
            l = self.visit(node.target.value)
            attr_name = syntax.StringConst(node.target.attr)
            attr = syntax.Attribute(l, attr_name)
            binop = syntax.BinaryOp(op, attr, value)
            return [syntax.StoreAttr(l, attr_name, binop)]
        elif isinstance(node.target, ast.Subscript):
            assert isinstance(node.target.slice, ast.Index)
            base = self.visit(node.target.value)
            index = self.visit(node.target.slice.value)
            old = syntax.Subscript(base, index)
            binop = syntax.BinaryOp(op, old, value)
            return [syntax.StoreSubscript(base, index, binop)]
        else:
            assert False

    def visit_Delete(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        assert isinstance(target, ast.Subscript)
        assert isinstance(target.slice, ast.Index)

        name = self.visit(target.value)
        value = self.visit(target.slice.value)
        return [syntax.DeleteSubscript(name, value)]

    def visit_If(self, node):
        expr = self.visit(node.test)
        stmts = self.visit_child_list(node.body)
        if node.orelse:
            else_block = self.visit_child_list(node.orelse)
        else:
            else_block = []
        return syntax.If(expr, stmts, else_block)

    def visit_Break(self, node):
        return syntax.Break()

    def visit_Continue(self, node):
        return syntax.Continue()

    def visit_For(self, node):
        assert not node.orelse
        iter = self.visit(node.iter)
        stmts = self.visit_child_list(node.body)
        stmts.append(syntax.CollectGarbage(None))

        if isinstance(node.target, ast.Name):
            target = node.target.id
        elif isinstance(node.target, ast.Tuple):
            target = [t.id for t in node.target.elts]
        else:
            assert False
        # HACK: self.iter_temp gets set when enumerating symbols
        return syntax.For(target, iter, stmts, node.iter_temp)

    def visit_While(self, node):
        assert not node.orelse
        test = self.visit(node.test)
        stmts = self.visit_child_list(node.body)
        stmts.append(syntax.CollectGarbage(None))
        return syntax.While(test, stmts)

    # XXX We are just flattening "with x as y:" into "y = x" (this works in some simple cases with open()).
    def visit_With(self, node):
        assert node.optional_vars
        expr = self.visit(node.context_expr)
        stmts = [syntax.Store(node.optional_vars.id, expr)]
        stmts += self.visit_child_list(node.body)
        return stmts

    def visit_Comprehension(self, node, comp_type):
        assert len(node.generators) == 1
        gen = node.generators[0]
        assert len(gen.ifs) <= 1

        if isinstance(gen.target, ast.Name):
            target = (gen.target.id)
        elif isinstance(gen.target, ast.Tuple):
            target = [(t.id) for t in gen.target.elts]
        else:
            assert False

        iter = self.visit(gen.iter)
        cond = None
        if gen.ifs:
            cond = self.visit(gen.ifs[0])
        if comp_type == 'dict':
            expr = self.visit(node.key)
            expr2 = self.visit(node.value)
        else:
            expr = self.visit(node.elt)
            expr2 = None
        return syntax.Comprehension(comp_type, target, iter, node.iter_temp,
                cond, expr, expr2)

    def visit_ListComp(self, node):
        return self.visit_Comprehension(node, 'list')

    def visit_SetComp(self, node):
        return self.visit_Comprehension(node, 'set')

    def visit_DictComp(self, node):
        return self.visit_Comprehension(node, 'dict')

    def visit_GeneratorExp(self, node):
        return self.visit_Comprehension(node, 'generator')

    def visit_Return(self, node):
        if node.value is not None:
            expr = self.visit(node.value)
            self.statements.append(syntax.CollectGarbage(expr))
            return syntax.Return(expr)
        else:
            return syntax.Return(None)

    def visit_Assert(self, node):
        expr = self.visit(node.test)
        return syntax.Assert(expr, node.lineno)

    def visit_Raise(self, node):
        assert not node.cause
        expr = self.visit(node.exc)
        return syntax.Raise(expr, node.lineno)

    def visit_arguments(self, node):
        assert not node.vararg
        assert not node.kwarg

        args = [a.arg for a in node.args]
        defaults = self.visit_child_list(node.defaults)
        return syntax.Arguments(args, defaults)

    def visit_FunctionDef(self, node):
        assert not self.in_function

        decorators = set()
        for decorator in node.decorator_list:
            assert isinstance(decorator, ast.Name)
            decorators.add(decorator.id)

        # Get bindings of all variables. Globals are the variables that have "global x"
        # somewhere in the function, or are never written in the function.
        globals_set = set()
        locals_set = set()
        all_vars_set = set()
        self.get_globals(node, globals_set, locals_set, all_vars_set)
        globals_set |= (all_vars_set - locals_set)

        self.symbol_idx['local'] = {symbol: idx for idx, symbol in enumerate(sorted(locals_set))}

        # Set some state and recursively visit child nodes, then restore state
        self.globals_set = globals_set
        self.in_function = True
        args = self.visit(node.args)
        body = self.visit_child_list(node.body)
        if not body or not isinstance(body[-1], syntax.Return):
            body.append(syntax.Return(None))
        self.globals_set = None
        self.in_function = False

        args.no_kwargs = 'no_kwargs' in decorators

        exp_name = node.exp_name if 'exp_name' in dir(node) else None
        return syntax.FunctionDef(node.name, args, body, exp_name, len(locals_set))

    def visit_ClassDef(self, node):
        assert not node.bases
        assert not node.keywords
        assert not node.starargs
        assert not node.kwargs
        assert not node.decorator_list
        assert not self.in_class
        assert not self.in_function

        for fn in node.body:
            if isinstance(fn, ast.FunctionDef):
                fn.exp_name = '_%s_%s' % (node.name, fn.name)

        self.in_class = True
        body = self.visit_child_list(node.body)
        self.in_class = False

        return syntax.ClassDef(node.name, body)

    # XXX This just turns "import x" into "x = 0".  It's certainly not what we really want...
    def visit_Import(self, node):
        statements = []
        for name in node.names:
            assert not name.asname
            assert name.name
            statements.append(syntax.Store(name.name, syntax.IntConst(0)))
        return statements

    def visit_Expr(self, node):
        return self.visit(node.value)

    def visit_Module(self, node):
        return self.visit_child_list(node.body)

    def visit_Pass(self, node): pass
    def visit_Load(self, node): pass
    def visit_Store(self, node): pass
    def visit_Global(self, node): pass

def transform(path):
    with open(path) as f:
        node = ast.parse(f.read())

    return Transformer().visit(node)
