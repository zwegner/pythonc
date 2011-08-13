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
import sys

import syntax

class Transformer(ast.NodeTransformer):
    def __init__(self):
        self.temp_id = 0
        self.statements = []
        self.functions = []
        self.in_class = False

    def get_temp(self):
        self.temp_id += 1
        return syntax.Identifier('temp_%02i' % self.temp_id)

    def flatten_node(self, node, statements=None):
        old_stmts = self.statements
        if statements is not None:
            self.statements = statements
        node = self.visit(node)
        if node.is_atom():
            r = node
        else:
            temp = self.get_temp()
            self.statements.append(syntax.Assign(temp, node))
            r = temp
        self.statements = old_stmts
        return r

    def flatten_list(self, node_list):
        old_stmts = self.statements
        statements = []
        for stmt in node_list:
            self.statements = []
            stmts = self.visit(stmt)
            if stmts:
                if isinstance(stmts, list):
                    statements += self.statements + stmts
                else:
                    statements += self.statements + [stmts]
        self.statements = old_stmts
        return statements

    def generic_visit(self, node):
        print(node.lineno)
        raise RuntimeError('can\'t translate %s' % node)

    def visit_children(self, node):
        return [self.visit(i) for i in ast.iter_child_nodes(node)]

    def visit_Name(self, node):
        assert isinstance(node.ctx, ast.Load)
        if node.id in ['True', 'False']:
            return syntax.BoolConst(node.id == 'True')
        elif node.id == 'None':
            return syntax.NoneConst()
        return syntax.Load(node.id)

    def visit_Num(self, node):
        assert isinstance(node.n, int)
        return syntax.IntConst(node.n)

    def visit_Str(self, node):
        assert isinstance(node.s, str)
        return syntax.StringConst(node.s)

    # Unary Ops
    def visit_Invert(self, node): return '__invert__'
    def visit_Not(self, node): return '__not__'
    def visit_UAdd(self, node): return '__pos__'
    def visit_USub(self, node): return '__neg__'
    def visit_UnaryOp(self, node):
        op = self.visit(node.op)
        rhs = self.flatten_node(node.operand)
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
        lhs = self.flatten_node(node.left)
        rhs = self.flatten_node(node.right)
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
        lhs = self.flatten_node(node.left)
        rhs = self.flatten_node(node.comparators[0])
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
        rhs_stmts = []
        rhs_expr = self.flatten_node(node.values[-1], statements=rhs_stmts)
        for v in reversed(node.values[:-1]):
            lhs_stmts = []
            lhs = self.flatten_node(v, statements=lhs_stmts)
            bool_op = syntax.BoolOp(op, lhs, rhs_stmts, rhs_expr)
            rhs_expr = bool_op.flatten(self, lhs_stmts)
            rhs_stmts = lhs_stmts
        self.statements += rhs_stmts
        return rhs_expr

    def visit_IfExp(self, node):
        expr = self.flatten_node(node.test)
        true_stmts = []
        true_expr = self.flatten_node(node.body, statements=true_stmts)
        false_stmts = []
        false_expr = self.flatten_node(node.orelse, statements=false_stmts)
        if_exp = syntax.IfExp(expr, true_stmts, true_expr, false_stmts, false_expr)
        return if_exp.flatten(self)

    def visit_List(self, node):
        items = [self.flatten_node(i) for i in node.elts]
        l = syntax.List(items)
        return l.flatten(self)

    def visit_Tuple(self, node):
        items = [self.flatten_node(i) for i in node.elts]
        l = syntax.List(items)
        return l.flatten(self)

    def visit_Dict(self, node):
        keys = [self.flatten_node(i) for i in node.keys]
        values = [self.flatten_node(i) for i in node.values]
        d = syntax.Dict(keys, values)
        return d.flatten(self)

    def visit_Set(self, node):
        items = [self.flatten_node(i) for i in node.elts]
        d = syntax.Set(items)
        return d.flatten(self)

    def visit_Subscript(self, node):
        l = self.flatten_node(node.value)
        if isinstance(node.slice, ast.Index):
            index = self.flatten_node(node.slice.value)
            return syntax.Subscript(l, index)
        elif isinstance(node.slice, ast.Slice):
            [start, end, step] = [self.flatten_node(a) if a else syntax.NoneConst() for a in
                    [node.slice.lower, node.slice.upper, node.slice.step]]
            return syntax.Slice(l, start, end, step)

    def visit_Attribute(self, node):
        assert isinstance(node.ctx, ast.Load)
        l = self.flatten_node(node.value)
        attr = syntax.Attribute(l, syntax.StringConst(node.attr))
        return attr

    def visit_Call(self, node):
        fn = self.flatten_node(node.func)
        args = syntax.List([self.flatten_node(a) for a in node.args])
        args = args.flatten(self)
        keys = [syntax.StringConst(i.arg) for i in node.keywords]
        values = [self.flatten_node(i.value) for i in node.keywords]
        kwargs = syntax.Dict(keys, values)
        kwargs = kwargs.flatten(self)
        return syntax.Call(fn, args, kwargs)

    def visit_Assign(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        value = self.flatten_node(node.value)
        if isinstance(target, ast.Name):
            return [syntax.Store(target.id, value)]
        elif isinstance(target, ast.Tuple):
            assert all(isinstance(t, ast.Name) for t in target.elts)
            stmts = []
            for i, t in enumerate(target.elts):
                stmts += [syntax.Store(t.id, syntax.Subscript(value, syntax.IntConst(i)))]
            return stmts
        elif isinstance(target, ast.Attribute):
            base = self.flatten_node(target.value)
            return [syntax.StoreAttr(base, syntax.StringConst(target.attr), value)]
        elif isinstance(target, ast.Subscript):
            assert isinstance(target.slice, ast.Index)
            base = self.flatten_node(target.value)
            index = self.flatten_node(target.slice.value)
            return [syntax.StoreSubscript(base, index, value)]
        else:
            assert False

    def visit_AugAssign(self, node):
        op = self.visit(node.op)
        value = self.flatten_node(node.value)
        if isinstance(node.target, ast.Name):
            target = node.target.id
            # XXX HACK: doesn't modify in place
            binop = syntax.BinaryOp(op, syntax.Load(target), value)
            return [syntax.Store(target, binop)]
        else:
            return None

    def visit_Delete(self, node):
        assert len(node.targets) == 1
        target = node.targets[0]
        assert isinstance(target, ast.Subscript)
        assert isinstance(target.slice, ast.Index)

        name = self.flatten_node(target.value)
        value = self.flatten_node(target.slice.value)
        return [syntax.DeleteSubscript(name, value)]

    def visit_Global(self, node):
        return [syntax.Global(name) for name in node.names]

    def visit_If(self, node):
        expr = self.flatten_node(node.test)
        stmts = self.flatten_list(node.body)
        if node.orelse:
            else_block = self.flatten_list(node.orelse)
        else:
            else_block = None
        return syntax.If(expr, stmts, else_block)

    def visit_Break(self, node):
        return syntax.Break()

    def visit_Continue(self, node):
        return syntax.Continue()

    def visit_For(self, node):
        assert not node.orelse
        iter = self.flatten_node(node.iter)
        stmts = self.flatten_list(node.body)

        if isinstance(node.target, ast.Name):
            target = node.target.id
        elif isinstance(node.target, ast.Tuple):
            target = node.target.elts
        else:
            assert False
        return syntax.For(target, iter, stmts)

    def visit_While(self, node):
        assert not node.orelse
        test_stmts = []
        test = self.flatten_node(node.test, statements=test_stmts)
        stmts = self.flatten_list(node.body)
        return syntax.While(test_stmts, test, stmts)

    def visit_ListComp(self, node):
        assert len(node.generators) == 1
        gen = node.generators[0]
        assert not gen.ifs

        if isinstance(gen.target, ast.Name):
            target = gen.target.id
        elif isinstance(gen.target, ast.Tuple):
            target = gen.target.elts
        else:
            assert False

        iter = self.flatten_node(gen.iter)
        stmts = []
        expr = self.flatten_node(node.elt, statements=stmts)
        comp = syntax.ListComp(target, iter, stmts, expr)
        return comp.flatten(self)

    # XXX HACK if we ever want these to differ...
    def visit_GeneratorExp(self, node):
        return self.visit_ListComp(node)

    def visit_Return(self, node):
        if node.value is not None:
            expr = self.flatten_node(node.value)
            return syntax.Return(expr)
        else:
            return syntax.Return(None)

    def visit_Assert(self, node):
        expr = self.flatten_node(node.test)
        return syntax.Assert(expr, node.lineno)

    def visit_arguments(self, node):
        assert not node.vararg
        assert not node.kwarg

        args = [a.arg for a in node.args]
        defaults = self.flatten_list(node.defaults)
        args = syntax.Arguments(args, defaults)
        return args.flatten(self)

    def visit_FunctionDef(self, node):
        args = self.visit(node.args)
        body = self.flatten_list(node.body)
        exp_name = node.exp_name if 'exp_name' in dir(node) else None
        fn = syntax.FunctionDef(node.name, args, body, exp_name).flatten(self)
        return fn

    def visit_ClassDef(self, node):
        assert not node.bases
        assert not node.keywords
        assert not node.starargs
        assert not node.kwargs
        assert not node.decorator_list

        for fn in node.body:
            if isinstance(fn, ast.FunctionDef):
                fn.exp_name = '_%s_%s' % (node.name, fn.name)

        body = self.flatten_list(node.body)

        c = syntax.ClassDef(node.name, body)
        return c.flatten(self)

    def visit_Expr(self, node):
        return self.visit(node.value)

    def visit_Module(self, node):
        return self.flatten_list(node.body)

    def visit_Pass(self, node): pass
    def visit_Load(self, node): pass
    def visit_Store(self, node): pass

with open(sys.argv[1]) as f:
    node = ast.parse(f.read())

x = Transformer()
node = x.visit(node)

with open(sys.argv[2], 'w') as f:
    f.write('#include "backend.cpp"\n')
    syntax.export_consts(f)

    for func in x.functions:
        f.write('%s\n' % func)

    f.write('int main(int argc, char **argv) {\n')
    f.write('    context *ctx = new context();\n')
    f.write('    init_context(ctx, argc, argv);\n')

    for stmt in node:
        f.write('    %s;\n' % stmt)

    f.write('}\n')
