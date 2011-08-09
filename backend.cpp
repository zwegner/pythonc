////////////////////////////////////////////////////////////////////////////////
// Pythonc backend
//
// Copyright 2011 Zach Wegner
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <string>
#include <vector>

#define error(...) do { printf(__VA_ARGS__); puts(""); exit(1); } while(0)

class node;
class context;
class string_const;

typedef std::vector<node *> node_list;
typedef std::map<const char *, node *> symbol_table;
typedef std::map<int64_t, node *> node_dict;
typedef std::set<std::string> globals_set;

class node
{
public:
    virtual bool is_bool() { return false; }
    virtual bool is_dict() { return false; }
    virtual bool is_file() { return false; }
    virtual bool is_int_const() { return false; }
    virtual bool is_list() { return false; }
    virtual bool is_none() { return false; }
    virtual bool is_set() { return false; }
    virtual bool is_string() { return false; }
    virtual bool bool_value() { error("bool_value unimplemented"); return false; }
    virtual int64_t int_value() { error("int_value unimplemented"); return 0; }
    virtual std::string string_value() { error("string_value unimplemented"); return NULL; }
    virtual node_list *list_value() { error("string_value unimplemented"); return NULL; }

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

#define UNIMP_UNOP(NAME) \
    virtual node *__##NAME##__() { error(#NAME " unimplemented"); return NULL; }

    UNIMP_UNOP(invert)
    UNIMP_UNOP(pos)
    UNIMP_UNOP(neg)

    virtual node *__not__();
    virtual node *__is__(node *rhs);
    virtual node *__isnot__(node *rhs);

    virtual node *__call__(context *ctx, node *args) { error("call unimplemented"); return NULL; }
    virtual void __delitem__(node *rhs) { error("delitem unimplemented"); }
    virtual node *__getitem__(node *rhs) { error("getitem unimplemented"); return NULL; }
    virtual node *__getattr__(node *rhs) { error("getattr unimplemented"); return NULL; }
    virtual node *__hash__() { error("hash unimplemented"); return NULL; }
    virtual node *__len__() { error("len unimplemented"); return NULL; }
    virtual void __setattr__(node *rhs, node *key) { error("setattr unimplemented"); }
    virtual void __setitem__(node *key, node *value) { error("setitem unimplemented"); }
    virtual node *__slice__(node *start, node *end, node *step) { error("slice unimplemented"); return NULL; }
    virtual node *__str__() { error("str unimplemented"); return NULL; }
};

class context
{
private:
    symbol_table symbols;
    context *parent_ctx;
    context *globals_ctx;
    globals_set globals;

public:
    context()
    {
        this->parent_ctx = NULL;
        this->globals_ctx = NULL;
    }

    context(context *parent_ctx)
    {
        this->parent_ctx = parent_ctx;
        this->globals_ctx = parent_ctx;
        while (this->globals_ctx->parent_ctx)
            this->globals_ctx = this->globals_ctx->parent_ctx;
    }

    void store(const char *name, node *obj)
    {
        if (this->globals.find(std::string(name)) != this->globals.end())
            this->globals_ctx->store(name, obj);
        else
            this->symbols[name] = obj;
    }
    node *load(const char *name)
    {
        if (this->globals.find(std::string(name)) != this->globals.end())
            return this->globals_ctx->load(name);
        symbol_table::const_iterator v = this->symbols.find(name);
        if (v == this->symbols.end())
        {
            if (this->parent_ctx)
                return this->parent_ctx->load(name);
            else
                error("cannot find '%s' in symbol table", name);
        }
        return v->second;
    }
    void set_global(const char *name)
    {
        this->globals.insert(name);
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

class none_const : public node
{
public:
    // For some reason this causes errors without an argument to the constructor...
    none_const(int value)
    {
    }

    virtual bool is_none() { return true; }
};

class bool_const : public node
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

#define INT_UNOP(NAME, OP) \
    virtual node *__##NAME##__() \
    { \
        return new int_const(OP this->int_value()); \
    }
    INT_UNOP(invert, ~)
    INT_UNOP(not, !)
    INT_UNOP(pos, +)
    INT_UNOP(neg, -)

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

#define STRING_OP(NAME, OP) \
    virtual node *__##NAME##__(node *rhs) \
    { \
        if (rhs->is_string()) \
            return new bool_const(this->string_value() OP rhs->string_value()); \
        error(#NAME " unimplemented"); \
        return NULL; \
    }

    STRING_OP(eq, ==)
    STRING_OP(ne, !=)
    STRING_OP(lt, <)
    STRING_OP(le, <=)
    STRING_OP(gt, >)
    STRING_OP(ge, >=)

    // FNV-1a algorithm
    virtual node *__hash__()
    {
        int64_t hash = 14695981039346656037ull;
        for (const char *c = this->value.c_str(); *c; c++)
        {
            hash ^= *c;
            hash *= 1099511628211ll;
        }
        return new int_const(hash);
    }
    virtual node *__str__() { return this; }
    virtual node *__getitem__(node *rhs)
    {
        if (!rhs->is_int_const())
        {
            error("getitem unimplemented");
            return NULL;
        }
        return new string_const(value.substr(rhs->int_value(), 1));
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
    node_list::iterator begin() { return items.begin(); }
    node_list::iterator end() { return items.end(); }
    int64_t index(int64_t base)
    {
        if (base < 0)
            base = items.size() + base;
        return base;
    }

    virtual bool is_list() { return true; }
    virtual node_list *list_value() { return &items; }

    virtual void __delitem__(node *rhs)
    {
        if (!rhs->is_int_const())
        {
            error("delitem unimplemented");
            return;
        }
        node_list::iterator f = items.begin() + this->index(rhs->int_value());
        items.erase(f);
    }
    virtual node *__getitem__(node *rhs)
    {
        if (!rhs->is_int_const())
        {
            error("getitem unimplemented");
            return NULL;
        }
        return items[this->index(rhs->int_value())];
    }
    virtual node *__len__()
    {
        return new int_const(this->items.size());
    }
    virtual node *__slice__(node *start, node *end, node *step)
    {
        if ((!start->is_none() && !start->is_int_const()) ||
            (!end->is_none() && !end->is_int_const()) ||
            (!step->is_none() && !step->is_int_const()))
            error("slice error");
        int64_t lo = start->is_none() ? 0 : start->int_value();
        int64_t hi = end->is_none() ? items.size() : end->int_value();
        int64_t st = step->is_none() ? 1 : step->int_value();
        list *new_list = new list();
        for (; st > 0 ? (lo < hi) : (lo > hi); lo += st)
            new_list->append(items[lo]);
        return new_list;
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

    node *lookup(node *key)
    {
        if (!key->is_int_const())
            key = key->__hash__();
        node_dict::const_iterator v = this->items.find(key->int_value());
        if (v == this->items.end())
            return NULL;
        // XXX: check equality of keys
        return v->second;
    }
    virtual bool is_dict() { return true; }
    virtual node *__getitem__(node *key)
    {
        node *old_key = key;
        node *value = this->lookup(key);
        if (value == NULL)
            error("cannot find '%s' in dict", old_key->__str__()->string_value().c_str());
        return value;
    }
    virtual node *__len__()
    {
        return new int_const(this->items.size());
    }
    virtual void __setitem__(node *key, node *value)
    {
        if (!key->is_int_const())
            key = key->__hash__();
        items[key->int_value()] = value;
    }
};

class set : public node
{
private:
    dict items;

public:
    set()
    {
    }

    void add(node *key)
    {
        items.__setitem__(key, (node *)1); // XXX HACK: just something non-NULL
    }

    virtual bool is_set() { return true; }
    virtual node *__contains__(node *key)
    {
        return new bool_const(items.lookup(key) != NULL);
    }
    virtual node *__len__()
    {
        return this->items.__len__();
    }
    virtual node *__str__()
    {
        std::string r = "{}";
        return new string_const(r);
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

class file : public node
{
private:
    FILE *f;

public:
    file(const char *path, const char *mode)
    {
        f = fopen(path, mode);
        if (!f)
            error("%s: file not found", path);
    }

    node *read(int len)
    {
        static char buf[64*1024];
        fread(buf, len, 1, this->f);
        std::string s(buf, len);
        return new string_const(s);
    }

    virtual bool is_file() { return true; }
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
        return items.__getitem__(new string_const(name));
    }
    void store(const char *name, node *value)
    {
        items.__setitem__(new string_const(name), value);
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

node *node::__not__()
{
    return new bool_const(!test_truth(this));
}

node *node::__is__(node *rhs)
{
    return new bool_const(this == rhs);
}

node *node::__isnot__(node *rhs)
{
    return new bool_const(this != rhs);
}

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

node *builtin_fread(context *ctx, node *args)
{
    node *f = args->__getitem__(new int_const(0));
    node *len = args->__getitem__(new int_const(1));
    if (!f->is_file() || !len->is_int_const())
        error("bad arguments to fread()");
    return ((file *)f)->read(len->int_value());
}

node *builtin_len(context *ctx, node *args)
{
    return args->__getitem__(new int_const(0))->__len__();
}

node *builtin_open(context *ctx, node *args)
{
    node *path = args->__getitem__(new int_const(0));
    node *mode = args->__getitem__(new int_const(1));
    if (!path->is_string() || !mode->is_string())
        error("bad arguments to open()");
    file *f = new file(path->string_value().c_str(), mode->string_value().c_str());
    return f;
}

node *builtin_ord(context *ctx, node *args)
{
    node *arg = args->__getitem__(new int_const(0));
    if (!arg->is_string() || arg->__len__()->int_value() != 1)
        error("bad arguments to ord()");
    return new int_const((int)arg->string_value().c_str()[0]);
}

node *builtin_print(context *ctx, node *args)
{
    node *s = args->__getitem__(new int_const(0));
    if (!s->is_string())
        s = s->__str__();
    printf("%s\n", s->string_value().c_str());
}

node *builtin_range(context *ctx, node *args)
{
    list *new_list = new list();
    int64_t st;
    int64_t end = args->__getitem__(new int_const(0))->int_value();

    for (st = 0; st < end; st++)
        new_list->append(new int_const(st));
    return new_list;
}

node *builtin_set(context *ctx, node *args)
{
    return new set();
}

void init_context(context *ctx, int argc, char **argv)
{
    ctx->store("fread", new function_def(builtin_fread));
    ctx->store("len", new function_def(builtin_len));
    ctx->store("open", new function_def(builtin_open));
    ctx->store("ord", new function_def(builtin_ord));
    ctx->store("print", new function_def(builtin_print));
    ctx->store("range", new function_def(builtin_range));
    ctx->store("set", new function_def(builtin_set));

    ctx->store("__name__", new string_const("__main__"));
    list *plist = new list();
    for (int a = 0; a < argc; a++)
        plist->append(new string_const(argv[a]));
    ctx->store("__args__", plist);
}
