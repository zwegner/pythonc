#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <vector>

#define error(...) do { printf(__VA_ARGS__); puts(""); exit(1); } while(0)

class node;
class context;
class string_const;

typedef std::vector<node *> node_list;
typedef std::map<const char *, node *> symbol_table;
typedef std::map<int64_t, node *> node_dict;

class node
{
public:
    virtual bool is_bool() { return false; }
    virtual bool is_dict() { return false; }
    virtual bool is_int_const() { return false; }
    virtual bool is_list() { return false; }
    virtual bool is_string() { return false; }
    virtual bool bool_value() { error("bool_value unimplemented"); return false; }
    virtual int64_t int_value() { error("int_value unimplemented"); return 0; }
    virtual std::string string_value() { error("string_value unimplemented"); return NULL; }

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

    UNIMP_OP(eq)
    UNIMP_OP(ne)
    UNIMP_OP(lt)
    UNIMP_OP(le)
    UNIMP_OP(gt)
    UNIMP_OP(ge)

    UNIMP_OP(contains)
    UNIMP_OP(ncontains)

    virtual node *__call__(context *ctx, node *args) { error("call unimplemented"); return NULL; }
    virtual node *__getitem__(node *rhs) { error("getitem unimplemented"); return NULL; }
    virtual node *__getattr__(node *rhs) { error("getattr unimplemented"); return NULL; }
    virtual node *__hash__() { error("hash unimplemented"); return NULL; }
    virtual void __setattr__(node *rhs, node *key) { error("setattr unimplemented"); }
    virtual node *__setitem__(node *rhs) { error("setitem unimplemented"); return NULL; }
    virtual node *__str__() { error("str unimplemented"); return NULL; }
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

class bool_const : public node // lame name
{
private:
    bool value;

public:
    bool_const(bool value)
    {
        this->value = value;
    }

    virtual bool is_bool() { return true; }
    virtual bool bool_value() { return this->value; }

    virtual node *__str__();
};

class int_const : public node
{
private:
    int64_t value;

public:
    int_const(int64_t value)
    {
        this->value = value;
    }

    virtual bool is_int_const() { return true; }
    virtual int64_t int_value() { return this->value; }

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

    INT_OP(eq, ==)
    INT_OP(ne, !=)
    INT_OP(lt, <)
    INT_OP(le, <=)
    INT_OP(gt, >)
    INT_OP(ge, >=)

    virtual node *__str__();
};

class string_const : public node
{
private:
    std::string value;

public:
    string_const(std::string value)
    {
        this->value = value;
    }

    virtual bool is_string() { return true; }
    virtual std::string string_value() { return this->value; }

    // FNV-1a algorithm
    virtual node *__hash__()
    {
        int64_t hash = 14695981039346656037ull;
        for (const char *c = this->value.c_str(); *c; c++)
        {
            hash ^= *c;
            hash *= 1099511628211;
        }
        return new int_const(hash);
    }
    virtual node *__str__() { return this; }
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
    node_list::iterator begin() { return items.begin(); }
    node_list::iterator end() { return items.end(); }

    virtual bool is_list() { return true; }
    virtual node *__getitem__(node *rhs)
    {
        if (rhs->is_int_const())
            return items[rhs->int_value()];
        error("getitem unimplemented");
        return NULL;
    }
};

class dict : public node
{
private:
    node_dict items;

public:
    dict()
    {
    }

    virtual bool is_dict() { return true; }
    virtual node *__getitem__(node *key)
    {
        node *old_key = key;
        if (!key->is_int_const())
            key = key->__hash__();
        node_dict::const_iterator v = this->items.find(key->int_value());
        if (v == this->items.end())
            error("cannot find '%s' in dict", old_key->__str__()->string_value().c_str());
        // XXX: check equality of keys
        return v->second;
    }
    virtual void __setitem__(node *key, node *value)
    {
        if (!key->is_int_const())
            key = key->__hash__();
        items[key->int_value()] = value;
    }
};

class object : public node
{
private:
    dict items;

public:
    object() {}

    virtual node *__getattr__(node *key)
    {
        return items.__getitem__(key);
    }
    virtual void __setattr__(node *key, node *value)
    {
        items.__setitem__(key, value);
    }
};

typedef node *(*fptr)(context *parent_ctx, node *args);

class function_def : public node
{
private:
    fptr base_function;

public:
    function_def(fptr base_function)
    {
        this->base_function = base_function;
    }

    virtual bool is_function() { return true; }

    virtual node *__call__(context *ctx, node *args)
    {
        return this->base_function(ctx, args);
    }
};

class class_def : public node
{
private:
    std::string name;
    dict items;

public:
    class_def(std::string name, void (*creator)(class_def *))
    {
        this->name = name;
        creator(this);
    }
    node *load(const char *name)
    {
        return items.__getitem__(new string_const("_"+this->name+"_"+name));
    }
    void store(const char *name, node *value)
    {
        items.__setitem__(new string_const("_"+this->name+"_"+name), value);
    }

    virtual bool is_function() { return true; }

    virtual node *__call__(context *ctx, node *args)
    {
        node *init = this->load("__init__");
        node *obj = new object();
        list *call_args = new list();
        call_args->append(obj);
        context *call_ctx = new context(ctx);
        init->__call__(call_ctx, call_args);
        return obj;
    }
    virtual node *__getattr__(node *attr)
    {
        return this->load(attr->string_value().c_str());
    }
};

node *int_const::__str__()
{
    char buf[32];
    sprintf(buf, "%lld", this->value);
    return new string_const(std::string(buf));
}

node *bool_const::__str__()
{
    return new string_const(std::string(this->value ? "True" : "False"));
}

bool test_truth(node *expr)
{
    if (expr->is_bool())
        return expr->bool_value();
    if (expr->is_int_const())
        return expr->int_value() != 0;
    if (expr->is_string())
        return expr->string_value().length() != 0;
    error("cannot determine truth value of expr");
    return false;
}

#include "builtins.cpp"

void set_builtins(context *ctx)
{
    ctx->store("print", new function_def(builtin_print));
}
