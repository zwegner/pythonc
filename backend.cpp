#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <vector>

#define error(...) do { printf(__VA_ARGS__); puts(""); exit(1); } while(0)

class node;
class context;

typedef std::vector<node *> node_list;
typedef std::map<const char *, node *> symbol_table;

class node
{
public:
    virtual bool is_int_const() { return false; }
    virtual uint64_t int_value() { error("int_value unimplemented"); return 0; }

#define UNIMP_OP(NAME) \
    virtual node *__##NAME##__(node *rhs) { error(#NAME " unimplemented"); return NULL; }
    UNIMP_OP(add)
    UNIMP_OP(and)
    UNIMP_OP(divmod)
    UNIMP_OP(floordiv)
    UNIMP_OP(lshift)
    UNIMP_OP(mod)
    UNIMP_OP(mul)
    UNIMP_OP(or)
    UNIMP_OP(pow)
    UNIMP_OP(rshift)
    UNIMP_OP(sub)
    UNIMP_OP(truediv)
    UNIMP_OP(xor)

    virtual node *__getitem__(node *rhs) { error("getitem unimplemented"); return NULL; }
    virtual node *__call__(context *ctx, node *args) { error("call unimplemented"); return NULL; }
};

class int_const : public node
{
private:
    uint64_t value;

public:
    int_const(uint64_t value)
    {
        this->value = value;
    }

    virtual bool is_int_const() { return true; }
    virtual uint64_t int_value() { return this->value; }

#define INT_OP(NAME, OP) \
    virtual node *__##NAME##__(node *rhs) \
    { \
        if (rhs->is_int_const()) \
            return new int_const(this->int_value() OP rhs->int_value()); \
        error(#NAME " unimplemented"); \
        return NULL; \
    }
    INT_OP(add, +)
    INT_OP(and, &)
    INT_OP(floordiv, /)
    INT_OP(lshift, <<)
    INT_OP(mod, %)
    INT_OP(mul, *)
    INT_OP(or, |)
    INT_OP(rshift, >>)
    INT_OP(sub, -)
    INT_OP(xor, ^)
};

class list : public node
{
private:
    node_list items;

public:
    list()
    {
    }
    void append(node *obj)
    {
        items.push_back(obj);
    }
    virtual node *__getitem__(node *rhs)
    {
        if (rhs->is_int_const())
            return items[rhs->int_value()];
        error("getitem unimplemented");
        return NULL;
    }
};

typedef node *(*fptr)(context *parent_ctx, node *args);

class function : public node
{
private:
    fptr base_function;

public:
    function(fptr base_function)
    {
        this->base_function = base_function;
    }

    virtual bool is_function() { return true; }

    virtual node *__call__(context *ctx, node *args)
    {
        return this->base_function(ctx, args);
    }
};

class context
{
private:
    symbol_table symbols;
    context *parent_ctx;

public:
    context()
    {
    }

    context(context *parent_ctx)
    {
        this->parent_ctx = parent_ctx;
    }

    void store(const char *name, node *obj)
    {
        this->symbols[name] = obj;
    }
    node *load(const char *name)
    {
        symbol_table::const_iterator v = this->symbols.find(name);
        if (v == this->symbols.end())
            error("cannot find '%s' in symbol table", name);
        return v->second;
    }
    void dump()
    {
        for (symbol_table::const_iterator i = this->symbols.begin(); i != this->symbols.end(); i++)
            if (i->second->is_int_const())
                printf("symbol['%s'] = int(%i);\n", i->first, i->second->int_value());
            else
                printf("symbol['%s'] = %p;\n", i->first, i->second);
    }
};

bool test_truth(node *expr)
{
    if (expr->is_int_const())
        return expr->int_value() != 0;
    error("cannot determine truth value of expr");
    return false;
}
