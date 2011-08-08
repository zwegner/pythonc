import ast
import syntax

all_nodes = set()
class Transformer(ast.NodeTransformer):
    def __init__(self):
        self.temp_id = 0
        self.statements = []
        self.functions = []

    def get_temp(self):
        self.temp_id += 1
        return syntax.Identifier('temp_%02i' % self.temp_id)

    def flatten_ref(self, node):
        node = self.visit(node)
        if node.is_atom():
            return node
        else:
            temp = self.get_temp()
            self.statements.append(syntax.Assign(temp, node))
            return temp

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

    def visit(self, node):
        global all_nodes
        all_nodes |= {type(node)}
        return super().visit(node)

#    def generic_visit(self, node):
#        raise RuntimeError('can\'t translate %s' % node)

    def visit_children(self, node):
        return [self.visit(i) for i in ast.iter_child_nodes(node)]

    def visit_Name(self, node):
        assert isinstance(node.ctx, ast.Load)
        return syntax.Load(node.id)

    def visit_Num(self, node):
        assert isinstance(node.n, int)
        return syntax.IntConst(node.n)

    # Binary Ops
    def visit_Add(self, node): return '__add__'
    def visit_BinOp(self, node):
        op = self.visit(node.op)
        lhs = self.flatten_ref(node.left)
        rhs = self.flatten_ref(node.right)
        return syntax.BinaryOp(op, lhs, rhs)

    def visit_List(self, node):
        items = [self.flatten_ref(i) for i in node.elts]
        l = syntax.List(items)
        return l.flatten(self)

    def visit_Subscript(self, node):
        assert isinstance(node.slice, ast.Index)
        l = self.flatten_ref(node.value)
        index = self.flatten_ref(node.slice.value)
        sub = syntax.Subscript(l, index)
        return sub

    def visit_Assign(self, node):
        assert len(node.targets) == 1
        assert isinstance(node.targets[0], ast.Name)
        value = self.flatten_ref(node.value)
        return [syntax.Store(node.targets[0].id, value)]

    def visit_Return(self, node):
        if node.value is not None:
            expr = self.flatten_ref(node.value)
            return syntax.Return(expr)
        else:
            return syntax.Return(None)

    def visit_FunctionDef(self, node):
        body = self.flatten_list(node.body)
        fn = syntax.FunctionDef(node.name, node.args, body)
        return fn.flatten(self)

    def visit_Call(self, node):
        fn = self.flatten_ref(node.func)
        args = syntax.List([self.flatten_ref(a) for a in node.args])
        args = args.flatten(self)
        return syntax.Call(fn, args)

    def visit_Module(self, node):
        return self.flatten_list(node.body)

    def visit_Load(self, node): pass
    def visit_Store(self, node): pass

with open('test.py') as f:
    node = ast.parse(f.read())
    x = Transformer()
    node = x.visit(node)
    with open('out.cpp', 'w') as f:
        f.write('#include "backend.cpp"\n')
        for func in x.functions:
            f.write('%s\n' % func)

        f.write('int main() {\n')
        f.write('    context *ctx = new context();\n')
        for stmt in node:
            f.write('    %s;\n' % stmt)
        f.write('    ctx->dump();\n')
        f.write('}\n')
