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

    virtual node *__add__(node *rhs) { error("add unimplemented"); return NULL; }

    virtual node *__subscript__(node *rhs) { error("subscript unimplemented"); return NULL; }
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

    virtual node *__add__(node *rhs)
    {
        if (rhs->is_int_const())
            return new int_const(this->int_value() + rhs->int_value());
        error("add unimplemented");
        return NULL;
    }
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
    virtual node *__subscript__(node *rhs)
    {
        if (rhs->is_int_const())
            return items[rhs->int_value()];
        error("subscript unimplemented");
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
