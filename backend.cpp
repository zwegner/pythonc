////////////////////////////////////////////////////////////////////////////////
//
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

#define __STDC_FORMAT_MACROS
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "alloc.h"

void error(const char *msg, ...) {
    va_list va;
    va_start(va, msg);
    vprintf(msg, va);
    va_end(va);
    puts("");
    fflush(stdout);
    exit(1);
}

class node;
class dict;
class list;
class context;
class string_const;

typedef int64_t int_t;

typedef std::pair<node *, node *> node_pair;
typedef std::map<const char *, node *, std::less<const char *>, alloc<std::pair<const char *, node *> > > symbol_table;
typedef std::map<int_t, node_pair, std::less<int_t>, alloc<std::pair<int_t, node *> > > node_dict;
typedef std::map<int_t, node *, std::less<int_t>, alloc<std::pair<int_t, node *> > > node_set;
typedef std::set<std::string, std::less<std::string>, alloc<std::string> > globals_set;
typedef std::vector<node *, alloc<node *> > node_list;

node *builtin_dict_get(context *ctx, list *args, dict *kwargs);
node *builtin_dict_keys(context *ctx, list *args, dict *kwargs);
node *builtin_fread(context *ctx, list *args, dict *kwargs);
node *builtin_isinstance(context *ctx, list *args, dict *kwargs);
node *builtin_len(context *ctx, list *args, dict *kwargs);
node *builtin_list_append(context *ctx, list *args, dict *kwargs);
node *builtin_list_index(context *ctx, list *args, dict *kwargs);
node *builtin_list_pop(context *ctx, list *args, dict *kwargs);
node *builtin_open(context *ctx, list *args, dict *kwargs);
node *builtin_ord(context *ctx, list *args, dict *kwargs);
node *builtin_print(context *ctx, list *args, dict *kwargs);
node *builtin_print_nonl(context *ctx, list *args, dict *kwargs);
node *builtin_range(context *ctx, list *args, dict *kwargs);
node *builtin_reversed(context *ctx, list *args, dict *kwargs);
node *builtin_set(context *ctx, list *args, dict *kwargs);
node *builtin_set_add(context *ctx, list *args, dict *kwargs);
node *builtin_sorted(context *ctx, list *args, dict *kwargs);
node *builtin_str_join(context *ctx, list *args, dict *kwargs);
node *builtin_str_upper(context *ctx, list *args, dict *kwargs);
node *builtin_str_startswith(context *ctx, list *args, dict *kwargs);
node *builtin_zip(context *ctx, list *args, dict *kwargs);

inline node *create_bool_const(bool b);

class node {
private:
    const char *node_type;

public:
    node(const char *type) {
        this->node_type = type;
    }
    virtual bool is_bool() { return false; }
    virtual bool is_dict() { return false; }
    virtual bool is_file() { return false; }
    virtual bool is_function() { return false; }
    virtual bool is_int_const() { return false; }
    virtual bool is_list() { return false; }
    virtual bool is_none() { return false; }
    virtual bool is_set() { return false; }
    virtual bool is_string() { return false; }
    virtual bool bool_value() { error("bool_value unimplemented for %s", node_type); return false; }
    virtual int_t int_value() { error("int_value unimplemented for %s", node_type); return 0; }
    virtual std::string string_value() { error("string_value unimplemented for %s", node_type); return NULL; }
    virtual node_list *list_value() { error("list_value unimplemented for %s", node_type); return NULL; }

#define UNIMP_OP(NAME) \
    virtual node *__##NAME##__(node *rhs) { error(#NAME " unimplemented for %s", node_type); return NULL; }

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

#define UNIMP_CMP_OP(NAME) \
    virtual bool _##NAME(node *rhs) { error(#NAME " unimplemented for %s", node_type); return false; } \
    virtual node *__##NAME##__(node *rhs) { error(#NAME " unimplemented for %s", node_type); return NULL; }

    UNIMP_CMP_OP(eq)
    UNIMP_CMP_OP(ne)
    UNIMP_CMP_OP(lt)
    UNIMP_CMP_OP(le)
    UNIMP_CMP_OP(gt)
    UNIMP_CMP_OP(ge)

    UNIMP_OP(contains)

#define UNIMP_UNOP(NAME) \
    virtual node *__##NAME##__() { error(#NAME " unimplemented for %s", node_type); return NULL; }

    UNIMP_UNOP(invert)
    UNIMP_UNOP(pos)
    UNIMP_UNOP(neg)

    node *__len__();
    node *__hash__();
    node *__getattr__(node *rhs);
    node *__not__();
    node *__is__(node *rhs);
    node *__isnot__(node *rhs);
    node *__str__();

    virtual node *__ncontains__(node *rhs) { return this->__contains__(rhs)->__not__(); }

    virtual node *__call__(context *ctx, list *args, dict *kwargs) { error("call unimplemented for %s", node_type); return NULL; }
    virtual void __delitem__(node *rhs) { error("delitem unimplemented for %s", node_type); }
    virtual node *__getitem__(node *rhs) { error("getitem unimplemented for %s", node_type); return NULL; }
    virtual node *__getitem__(int index) { error("getitem unimplemented for %s", node_type); return NULL; }
    virtual void __setattr__(node *rhs, node *key) { error("setattr unimplemented for %s", node_type); }
    virtual void __setitem__(node *key, node *value) { error("setitem unimplemented for %s", node_type); }
    virtual node *__slice__(node *start, node *end, node *step) { error("slice unimplemented for %s", node_type); return NULL; }

    // unwrapped versions
    virtual int_t len() { error("len unimplemented for %s", node_type); return 0; }
    virtual node *getattr(const char *rhs) { error("getattr unimplemented (%s) for %s", rhs, node_type); return NULL; }
    virtual int_t hash() { error("hash unimplemented for %s", node_type); return 0; }
    virtual std::string str() { error("str unimplemented for %s", node_type); return NULL; }
};

class context {
private:
    symbol_table symbols;
    context *parent_ctx;
    context *globals_ctx;
    globals_set globals;

public:
    context() {
        this->parent_ctx = NULL;
        this->globals_ctx = NULL;
    }

    context(context *parent_ctx) {
        this->parent_ctx = parent_ctx;
        this->globals_ctx = parent_ctx;
        while (this->globals_ctx->parent_ctx)
            this->globals_ctx = this->globals_ctx->parent_ctx;
    }

    void store(const char *name, node *obj) {
        if (this->globals.find(std::string(name)) != this->globals.end())
            this->globals_ctx->store(name, obj);
        else
            this->symbols[name] = obj;
    }
    node *load(const char *name) {
        if (this->globals.find(std::string(name)) != this->globals.end())
            return this->globals_ctx->load(name);
        symbol_table::const_iterator v = this->symbols.find(name);
        if (v == this->symbols.end()) {
            if (this->parent_ctx)
                return this->parent_ctx->load(name);
            else
                error("cannot find '%s' in symbol table", name);
        }
        return v->second;
    }
    void set_global(const char *name) {
        this->globals.insert(name);
    }
    void dump() {
        for (symbol_table::const_iterator i = this->symbols.begin(); i != this->symbols.end(); i++)
            if (i->second->is_int_const())
                printf("symbol['%s'] = int(%" PRId64 ");\n", i->first, i->second->int_value());
            else
                printf("symbol['%s'] = %p;\n", i->first, i->second);
    }
};

class none_const : public node {
public:
    // For some reason this causes errors without an argument to the constructor...
    none_const(int_t value) : node("none") {
    }

    virtual bool is_none() { return true; }

    virtual node *__eq__(node *rhs);
    virtual int_t hash() { return 0; }
    virtual std::string str() { return std::string("None"); }
};

class int_const : public node {
private:
    int_t value;

public:
    int_const(int_t value) : node("int") {
        this->value = value;
    }

    virtual bool is_int_const() { return true; }
    virtual int_t int_value() { return this->value; }
    virtual bool bool_value() { return this->value != 0; }

#define INT_OP(NAME, OP) \
    virtual int_t _##NAME(node *rhs) { \
        if (rhs->is_int_const() || rhs->is_bool()) \
            return this->int_value() OP rhs->int_value(); \
        error(#NAME " error in int"); \
        return 0; \
    } \
    virtual node *__##NAME##__(node *rhs) { \
        return new(allocator) int_const(this->_##NAME(rhs)); \
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

#define CMP_OP(NAME, OP) \
    virtual bool _##NAME(node *rhs) { \
        if (rhs->is_int_const() || rhs->is_bool()) \
            return this->int_value() OP rhs->int_value(); \
        error(#NAME " error in int"); \
        return 0; \
    } \
    virtual node *__##NAME##__(node *rhs) { \
        return create_bool_const(this->_##NAME(rhs)); \
    }

    CMP_OP(eq, ==)
    CMP_OP(ne, !=)
    CMP_OP(lt, <)
    CMP_OP(le, <=)
    CMP_OP(gt, >)
    CMP_OP(ge, >=)

#define INT_UNOP(NAME, OP) \
    virtual node *__##NAME##__() { \
        return new(allocator) int_const(OP this->int_value()); \
    }
    INT_UNOP(invert, ~)
    INT_UNOP(not, !)
    INT_UNOP(pos, +)
    INT_UNOP(neg, -)

    virtual int_t hash() { return this->value; }
    virtual node *getattr(const char *key);
    virtual std::string str();
};

class bool_const : public node {
private:
    bool value;

public:
    bool_const(bool value) : node("bool") {
        this->value = value;
    }

    virtual bool is_bool() { return true; }
    virtual bool bool_value() { return this->value; }
    virtual int_t int_value() { return (int_t)this->value; }

#define BOOL_AS_INT_OP(NAME, OP) \
    virtual node *__##NAME##__(node *rhs) { \
        if (rhs->is_int_const() || rhs->is_bool()) \
            return new(allocator) int_const(this->int_value() OP rhs->int_value()); \
        error(#NAME " error in bool"); \
        return NULL; \
    }
    BOOL_AS_INT_OP(add, +)
    BOOL_AS_INT_OP(floordiv, /)
    BOOL_AS_INT_OP(lshift, <<)
    BOOL_AS_INT_OP(mod, %)
    BOOL_AS_INT_OP(mul, *)
    BOOL_AS_INT_OP(rshift, >>)
    BOOL_AS_INT_OP(sub, -)

    BOOL_AS_INT_OP(and, &)
    BOOL_AS_INT_OP(or, |)
    BOOL_AS_INT_OP(xor, ^)

#define BOOL_OP(NAME, OP) \
    virtual node *__##NAME##__(node *rhs) { \
        if (rhs->is_int_const() || rhs->is_bool()) \
            return create_bool_const(this->int_value() OP rhs->int_value()); \
        error(#NAME " error in bool"); \
        return NULL; \
    }
    BOOL_OP(eq, ==)
    BOOL_OP(ne, !=)
    BOOL_OP(lt, <)
    BOOL_OP(le, <=)
    BOOL_OP(gt, >)
    BOOL_OP(ge, >=)

    virtual int_t hash() { return (int_t)this->value; }
    virtual std::string str();
};

class string_const : public node {
private:
    std::string value;

public:
    string_const(std::string value) : node("str") {
        this->value = value;
    }

    virtual bool is_string() { return true; }
    virtual std::string string_value() { return this->value; }

    std::string::iterator begin() { return value.begin(); }
    std::string::iterator end() { return value.end(); }

#define STRING_OP(NAME, OP) \
    virtual bool _##NAME(node *rhs) { \
        if (rhs->is_string()) \
            return this->string_value() OP rhs->string_value(); \
        error(#NAME " unimplemented"); \
        return false; \
    } \
    virtual node *__##NAME##__(node *rhs) { \
        return create_bool_const(this->_##NAME(rhs)); \
    }

    STRING_OP(eq, ==)
    STRING_OP(ne, !=)
    STRING_OP(lt, <)
    STRING_OP(le, <=)
    STRING_OP(gt, >)
    STRING_OP(ge, >=)

    virtual node *__mod__(node *rhs);
    virtual node *__add__(node *rhs);
    virtual node *__mul__(node *rhs);

    virtual node *getattr(const char *key);

    virtual node *__getitem__(node *rhs) {
        if (!rhs->is_int_const()) {
            error("getitem unimplemented");
            return NULL;
        }
        return new(allocator) string_const(value.substr(rhs->int_value(), 1));
    }
    // FNV-1a algorithm
    virtual int_t hash() {
        int_t hashkey = 14695981039346656037ull;
        for (std::string::iterator c = this->begin(); c != this->end(); c++) {
            hashkey ^= *c;
            hashkey *= 1099511628211ll;
        }
        return hashkey;
    }
    virtual int_t len() {
        return value.length();
    }
    virtual node *__slice__(node *start, node *end, node *step) {
        if ((!start->is_none() && !start->is_int_const()) ||
            (!end->is_none() && !end->is_int_const()) ||
            (!step->is_none() && !step->is_int_const()))
            error("slice error");
        int_t lo = start->is_none() ? 0 : start->int_value();
        int_t hi = end->is_none() ? value.length() : end->int_value();
        int_t st = step->is_none() ? 1 : step->int_value();
        if (st != 1)
            error("slice step != 1 not supported for string");
        return new(allocator) string_const(this->value.substr(lo, hi - lo + 1));
    }
    virtual std::string str() { return this->value; }
};

class list : public node {
private:
    node_list items;

public:
    list() : node("list") {
    }
    list(node_list &l) : items(l), node("list") {
    }
    void append(node *obj) {
        items.push_back(obj);
    }
    void prepend(node *obj) {
        items.insert(items.begin(), obj);
    }
    node *pop() {
        // would be nice if STL wasn't stupid, and this was one line...
        node *popped = items.back();
        items.pop_back();
        return popped;
    }
    node_list::iterator begin() { return items.begin(); }
    node_list::iterator end() { return items.end(); }
    int_t index(int_t base) {
        if (base < 0)
            base = items.size() + base;
        return base;
    }

    virtual bool is_list() { return true; }
    virtual node_list *list_value() { return &items; }

    virtual node *__add__(node *rhs);
    virtual node *__mul__(node *rhs);

    virtual node *__contains__(node *key) {
        bool found = false;
        for (int_t i = 0; i < this->items.size(); i++)
            if (this->items[i]->__eq__(key)->bool_value()) {
                found = true;
                break;
            }
        return create_bool_const(found);
    }
    virtual void __delitem__(node *rhs) {
        if (!rhs->is_int_const()) {
            error("delitem unimplemented");
            return;
        }
        node_list::iterator f = items.begin() + this->index(rhs->int_value());
        items.erase(f);
    }
    virtual node *__getitem__(int idx) {
        return this->items[this->index(idx)];
    }
    virtual node *__getitem__(node *rhs) {
        if (!rhs->is_int_const()) {
            error("getitem unimplemented");
            return NULL;
        }
        return this->__getitem__(rhs->int_value());
    }
    virtual int_t len() {
        return this->items.size();
    }
    virtual void __setitem__(node *key, node *value) {
        if (!key->is_int_const())
            error("error in list.setitem");
        int_t idx = key->int_value();
        items[this->index(idx)] = value;
    }
    virtual node *__slice__(node *start, node *end, node *step) {
        if ((!start->is_none() && !start->is_int_const()) ||
            (!end->is_none() && !end->is_int_const()) ||
            (!step->is_none() && !step->is_int_const()))
            error("slice error");
        int_t lo = start->is_none() ? 0 : start->int_value();
        int_t hi = end->is_none() ? items.size() : end->int_value();
        int_t st = step->is_none() ? 1 : step->int_value();
        list *new_list = new(allocator) list();
        for (; st > 0 ? (lo < hi) : (lo > hi); lo += st)
            new_list->append(items[lo]);
        return new_list;
    }
    virtual node *getattr(const char *key);
};

class dict : public node {
private:
    node_dict items;

public:
    dict() : node("dict") {
    }
    node *lookup(node *key) {
        int_t hashkey;
        if (key->is_int_const())
            hashkey = key->int_value();
        else
            hashkey = key->hash();
        node_dict::const_iterator v = this->items.find(hashkey);
        if (v == this->items.end() || !v->second.first->__eq__(key)->bool_value())
            return NULL;
        return v->second.second;
    }
    node_dict::iterator begin() { return items.begin(); }
    node_dict::iterator end() { return items.end(); }

    virtual bool is_dict() { return true; }
    virtual node *__contains__(node *key) {
        return create_bool_const(this->lookup(key) != NULL);
    }
    virtual node *__getitem__(node *key) {
        node *old_key = key;
        node *value = this->lookup(key);
        if (value == NULL)
            error("cannot find '%s' in dict", old_key->str().c_str());
        return value;
    }
    virtual int_t len() {
        return this->items.size();
    }
    virtual void __setitem__(node *key, node *value) {
        int_t hashkey;
        if (key->is_int_const())
            hashkey = key->int_value();
        else
            hashkey = key->hash();
        items[hashkey] = node_pair(key, value);
    }
    virtual std::string str() {
        std::string new_string = "{";
        bool first = true;
        for (node_dict::iterator i = this->items.begin(); i != this->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += i->second.first->str() + ": " + i->second.second->str();
        }
        new_string += "}";
        return new_string;
    }
    virtual node *getattr(const char *key);
};

class set : public node {
private:
    node_set items;

public:
    set() : node("set") {
    }

    node *lookup(node *key) {
        int_t hashkey;
        if (key->is_int_const())
            hashkey = key->int_value();
        else
            hashkey = key->hash();
        node_set::const_iterator v = this->items.find(hashkey);
        if (v == this->items.end() || !v->second->__eq__(key)->bool_value())
            return NULL;
        return v->second;
    }
    void add(node *key) {
        int_t hashkey;
        if (key->is_int_const())
            hashkey = key->int_value();
        else
            hashkey = key->hash();
        items[hashkey] = key;
    }

    virtual bool is_set() { return true; }
    virtual node *__contains__(node *key) {
        return create_bool_const(this->lookup(key) != NULL);
    }
    virtual int_t len() {
        return this->items.size();
    }
    virtual std::string str() {
        std::string new_string = "{";
        bool first = true;
        for (node_set::iterator i = this->items.begin(); i != this->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += i->second->str();
        }
        new_string += "}";
        return new_string;
    }
    virtual node *getattr(const char *key);
};

class object : public node {
private:
    dict items;

public:
    object() : node("object")
    {}

    virtual node *getattr(const char *key) {
        return items.__getitem__(new(allocator) string_const(key));
    }
    virtual void __setattr__(node *key, node *value) {
        items.__setitem__(key, value);
    }
    virtual node *__eq__(node *rhs) {
        return create_bool_const(this == rhs);
    }
};

class file : public node {
private:
    FILE *f;

public:
    file(const char *path, const char *mode) : node("file") {
        f = fopen(path, mode);
        if (!f)
            error("%s: file not found", path);
    }

    node *read(int_t len) {
        static char buf[64*1024];
        fread(buf, len, 1, this->f);
        std::string s(buf, len);
        return new(allocator) string_const(s);
    }

    virtual bool is_file() { return true; }
};

typedef node *(*fptr)(context *parent_ctx, list *args, dict *kwargs);

class bound_method : public node {
private:
    node *self;
    node *function;

public:
    bound_method(node *self, node *function) : node("bound_method") {
        this->self = self;
        this->function = function;
    }

    virtual bool is_function() { return true; } // XXX is it?

    virtual node *__call__(context *ctx, list *args, dict *kwargs) {
        if (!args->is_list())
            error("call with non-list args?");
        ((list *)args)->prepend(this->self);
        return this->function->__call__(ctx, args, kwargs);
    }
};

class function_def : public node {
private:
    fptr base_function;

public:
    function_def(fptr base_function) : node("function") {
        this->base_function = base_function;
    }

    virtual bool is_function() { return true; }

    virtual node *__call__(context *ctx, list *args, dict *kwargs) {
        return this->base_function(ctx, args, kwargs);
    }
};

class class_def : public node {
private:
    std::string name;
    dict items;

public:
    class_def(std::string name, void (*creator)(class_def *)) : node("class") {
        this->name = name;
        creator(this);
    }
    node *load(const char *name) {
        return items.__getitem__(new(allocator) string_const(name));
    }
    void store(const char *name, node *value) {
        items.__setitem__(new(allocator) string_const(name), value);
    }

    virtual node *__call__(context *ctx, list *args, dict *kwargs) {
        node *init = this->load("__init__");
        node *obj = new(allocator) object();

        obj->__setattr__(new(allocator) string_const("__class__"), this);

        // Create bound methods
        for (node_dict::iterator i = items.begin(); i != items.end(); i++)
            if (i->second.second->is_function())
                obj->__setattr__(i->second.first, new(allocator) bound_method(obj, i->second.second));

        ((list *)args)->prepend(obj);
        context *call_ctx = new(allocator) context(ctx);
        init->__call__(call_ctx, args, kwargs);
        return obj;
    }
    virtual node *getattr(const char *attr) {
        return this->load(attr);
    }
};

bool_const bool_singleton_True(true);
bool_const bool_singleton_False(false);
none_const none_singleton(0);

inline node *create_bool_const(bool b) {
    return b ? &bool_singleton_True : &bool_singleton_False;
}

// Silly builtin classes
void _int__create_(class_def *ctx) {
}

void _str__create_(class_def *ctx) {
}

class_def builtin_class_int("int", _int__create_);
class_def builtin_class_str("str", _str__create_);

bool test_truth(node *expr) {
    if (expr->is_none())
        return false;
    else if (expr->is_bool())
        return expr->bool_value();
    else if (expr->is_int_const())
        return expr->int_value() != 0;
    else if (expr->is_string() || expr->is_list() || expr->is_dict() || expr->is_set())
        return expr->len() != 0;
    return true;
}

node *node::__getattr__(node *key) {
    if (!key->is_string())
        error("getattr with non-string");
    return this->getattr(key->string_value().c_str());
}

node *node::__hash__() {
    return new(allocator) int_const(this->hash());
}

node *node::__len__() {
    return new(allocator) int_const(this->len());
}

node *node::__not__() {
    return create_bool_const(!test_truth(this));
}

node *node::__is__(node *rhs) {
    return create_bool_const(this == rhs);
}

node *node::__isnot__(node *rhs) {
    return create_bool_const(this != rhs);
}

node *node::__str__() {
    return new(allocator) string_const(this->str());
}

node *none_const::__eq__(node *rhs) {
    return create_bool_const(this == rhs);
}

node *int_const::getattr(const char *key) {
    if (!strcmp(key, "__class__"))
        return &builtin_class_int;
    error("int has no attribute %s", key);
}

std::string int_const::str() {
    char buf[32];
    sprintf(buf, "%" PRId64, this->value);
    return std::string(buf);
}

std::string bool_const::str() {
    return std::string(this->value ? "True" : "False");
}

node *list::__add__(node *rhs) {
    if (!rhs->is_list())
        error("list add error");
    list *plist = new(allocator) list();
    node_list *rhs_list = rhs->list_value();
    for (node_list::iterator i = this->begin(); i != this->end(); i++)
        plist->append(*i);
    for (node_list::iterator i = rhs_list->begin(); i != rhs_list->end(); i++)
        plist->append(*i);
    return plist;
}

node *list::__mul__(node *rhs) {
    if (!rhs->is_int_const())
        error("list mul error");
    list *plist = new(allocator) list();
    for (int_t x = rhs->int_value(); x > 0; x--)
        for (node_list::iterator i = this->begin(); i != this->end(); i++)
            plist->append(*i);
    return plist;
}

node *list::getattr(const char *key) {
    if (!strcmp(key, "append"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_list_append));
    else if (!strcmp(key, "index"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_list_index));
    else if (!strcmp(key, "pop"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_list_pop));
    error("list has no attribute %s", key);
}

node *dict::getattr(const char *key) {
    if (!strcmp(key, "get"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_dict_get));
    else if (!strcmp(key, "keys"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_dict_keys));
    error("dict has no attribute %s", key);
}

node *set::getattr(const char *key) {
    if (!strcmp(key, "add"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_set_add));
    error("set has no attribute %s", key);
}

node *string_const::getattr(const char *key) {
    if (!strcmp(key, "__class__"))
        return &builtin_class_str;
    else if (!strcmp(key, "join"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_str_join));
    else if (!strcmp(key, "upper"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_str_upper));
    else if (!strcmp(key, "startswith"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_str_startswith));
    error("str has no attribute %s", key);
}

// This entire function is very stupidly implemented.
node *string_const::__mod__(node *rhs) {
    std::ostringstream new_string;
    // HACK for now...
    if (!rhs->is_list()) {
        list *l = new(allocator) list();
        l->append(rhs);
        rhs = l;
    }
    int_t args = 0;
    for (const char *c = value.c_str(); *c; c++) {
        if (*c == '%') {
            char fmt_buf[64], buf[64];
            char *fmt = fmt_buf;
            *fmt++ = '%';
            c++;
            // Copy over formatting data: only numbers allowed as modifiers now
            while (*c && isdigit(*c))
                *fmt++ = *c++;

            if (fmt - fmt_buf >= sizeof(buf))
                error("I do believe you've made a terrible mistake whilst formatting a string!");
            if (args >= rhs->len())
                error("not enough arguments for string format");
            node *arg = rhs->__getitem__(args++);
            if (*c == 's') {
                *fmt++ = 's';
                *fmt = 0;
                sprintf(buf, fmt_buf, arg->str().c_str());
            }
            else if (*c == 'd' || *c == 'i' || *c == 'X') {
                *fmt++ = *c;
                *fmt = 0;
                sprintf(buf, fmt_buf, arg->int_value());
            }
            else if (*c == 'c') {
                *fmt++ = 'c';
                *fmt = 0;
                int_t char_value;
                if (arg->is_string())
                    char_value = (unsigned char)arg->string_value()[0];
                else
                    char_value = arg->int_value();
                sprintf(buf, fmt_buf, char_value);
            }
            else
                error("bad format specifier '%c' in \"%s\"", *c, value.c_str());
            new_string << buf;
        }
        else
            new_string << *c;
    }
    return new(allocator) string_const(new_string.str());
}

node *string_const::__add__(node *rhs) {
    if (!rhs->is_string())
        error("bad argument to str.add");
    std::string new_string = this->value + rhs->string_value();
    return new(allocator) string_const(new_string);
}

node *string_const::__mul__(node *rhs) {
    if (!rhs->is_int_const() || rhs->int_value() < 0)
        error("bad argument to str.mul");
    std::string new_string;
    for (int_t i = 0; i < rhs->int_value(); i++)
        new_string += this->value;
    return new(allocator) string_const(new_string);
}

node *builtin_dict_get(context *ctx, list *args, dict *kwargs) {
    if (args->len() != 3) // just assume 3 args for now...
        error("bad number of arguments to dict.get()");
    node *self = args->__getitem__(0);
    node *key = args->__getitem__(1);

    node *value = ((dict *)self)->lookup(key);
    if (!value)
        value = args->__getitem__(2);

    return value;
}

node *builtin_dict_keys(context *ctx, list *args, dict *kwargs) {
    if (args->len() != 1)
        error("bad number of arguments to dict.keys()");
    dict *self = (dict *)args->__getitem__(0);

    list *plist = new(allocator) list();
    for (node_dict::iterator i = self->begin(); i != self->end(); i++)
        plist->append(i->second.first);

    return plist;
}

node *builtin_fread(context *ctx, list *args, dict *kwargs) {
    node *f = args->__getitem__(0);
    node *len = args->__getitem__(1);
    if (!f->is_file() || !len->is_int_const())
        error("bad arguments to fread()");
    return ((file *)f)->read(len->int_value());
}

node *builtin_isinstance(context *ctx, list *args, dict *kwargs) {
    node *obj = args->__getitem__(0);
    node *arg_class = args->__getitem__(1);

    node *obj_class = obj->getattr("__class__");
    return create_bool_const(obj_class == arg_class);
}

node *builtin_len(context *ctx, list *args, dict *kwargs) {
    return args->__getitem__(0)->__len__();
}

node *builtin_list_append(context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    node *item = args->__getitem__(1);

    ((list *)self)->append(item);

    return &none_singleton;
}

node *builtin_list_index(context *ctx, list *args, dict *kwargs) {
    if (args->len() != 2)
        error("bad number of arguments to list.index()");
    node *self = args->__getitem__(0);
    node *key = args->__getitem__(1);

    for (int_t i = 0; i < self->len(); i++)
        if (self->__getitem__(i)->__eq__(key)->bool_value())
            return new(allocator) int_const(i);
    error("item not found in list");
    return &none_singleton;
}

node *builtin_list_pop(context *ctx, list *args, dict *kwargs) {
    list *self = (list *)args->__getitem__(0);

    return self->pop();
}

node *builtin_open(context *ctx, list *args, dict *kwargs) {
    node *path = args->__getitem__(0);
    node *mode = args->__getitem__(1);
    if (!path->is_string() || !mode->is_string())
        error("bad arguments to open()");
    file *f = new(allocator) file(path->string_value().c_str(), mode->string_value().c_str());
    return f;
}

node *builtin_ord(context *ctx, list *args, dict *kwargs) {
    node *arg = args->__getitem__(0);
    if (!arg->is_string() || arg->len() != 1)
        error("bad arguments to ord()");
    return new(allocator) int_const((unsigned char)arg->string_value()[0]);
}

node *builtin_print(context *ctx, list *args, dict *kwargs) {
    std::string new_string;
    for (int_t i = 0; i < args->len(); i++) {
        if (i)
            new_string += " ";
        node *s = args->__getitem__(i);
        new_string += s->str();
    }
    printf("%s\n", new_string.c_str());
}

node *builtin_print_nonl(context *ctx, list *args, dict *kwargs) {
    node *s = args->__getitem__(0);
    printf("%s", s->str().c_str());
}

node *builtin_range(context *ctx, list *args, dict *kwargs) {
    list *new_list = new(allocator) list();
    int_t start = 0, end, step = 1;

    if (args->len() == 1)
        end = args->__getitem__(0)->int_value();
    else if (args->len() == 2) {
        start = args->__getitem__(0)->int_value();
        end = args->__getitem__(1)->int_value();
    }
    else if (args->len() == 3) {
        start = args->__getitem__(0)->int_value();
        end = args->__getitem__(1)->int_value();
        step = args->__getitem__(2)->int_value();
    }
    else
        error("too many arguments to range()");

    for (int_t s = start; step > 0 ? (s < end) : (s > end); s += step)
        new_list->append(new(allocator) int_const(s));
    return new_list;
}

node *builtin_reversed(context *ctx, list *args, dict *kwargs) {
    node *item = args->__getitem__(0);
    if (!item->is_list())
        error("cannot call reversed on non-list");
    list *plist = (list *)item;
    // sigh, I hate c++
    node_list new_list;
    new_list.resize(plist->len());
    std::reverse_copy(plist->begin(), plist->end(), new_list.begin());

    return new(allocator) list(new_list);
}

node *builtin_set(context *ctx, list *args, dict *kwargs) {
    return new(allocator) set();
}

node *builtin_set_add(context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    node *item = args->__getitem__(1);

    ((set *)self)->add(item);

    return &none_singleton;
}

bool compare_nodes(node *lhs, node *rhs) {
    return lhs->_lt(rhs);
}

node *builtin_sorted(context *ctx, list *args, dict *kwargs) {
    node *item = args->__getitem__(0);
    if (!item->is_list())
        error("cannot call sorted on non-list");
    list *plist = (list *)item;
    // sigh, I hate c++
    node_list new_list;
    new_list.resize(plist->len());
    std::copy(plist->begin(), plist->end(), new_list.begin());
    std::stable_sort(new_list.begin(), new_list.end(), compare_nodes);

    return new(allocator) list(new_list);
}

node *builtin_str_join(context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    node *item = args->__getitem__(1);
    if (!self->is_string() || !item->is_list())
        error("bad arguments to str.join()");

    list *joined = (list *)item;
    std::string s;
    for (node_list::iterator i = joined->begin(); i != joined->end(); i++) {
        s += (*i)->str();
        if (i + 1 != joined->end())
            s += self->string_value();
    }
    return new(allocator) string_const(s);
}

node *builtin_str_upper(context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    if (!self->is_string())
        error("bad argument to str.upper()");
    string_const *str = (string_const *)self;

    std::string new_string;
    for (std::string::iterator c = str->begin(); c != str->end(); c++)
        new_string += toupper(*c);

    return new(allocator) string_const(new_string);
}

node *builtin_str_startswith(context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    node *prefix = args->__getitem__(1);
    if (!self->is_string() || !prefix->is_string())
        error("bad arguments to str.startswith()");

    std::string s1 = self->string_value();
    std::string s2 = prefix->string_value();
    return create_bool_const(s1.compare(0, s2.size(), s2) == 0);
}

node *builtin_zip(context *ctx, list *args, dict *kwargs) {
    if (args->len() != 2)
        error("bad arguments to zip()");
    node *list1 = args->__getitem__(0);
    node *list2 = args->__getitem__(1);

    if (!list1->is_list() || !list2->is_list() || list1->len() != list2->len())
        error("bad arguments to zip()");

    list *plist = new(allocator) list();
    for (int_t i = 0; i < list1->len(); i++) {
        list *pair = new(allocator) list();
        pair->append(list1->__getitem__(i));
        pair->append(list2->__getitem__(i));
        plist->append(pair);
    }

    return plist;
}

void init_context(context *ctx, int_t argc, char **argv) {
    ctx->store("fread", new(allocator) function_def(builtin_fread));
    ctx->store("len", new(allocator) function_def(builtin_len));
    ctx->store("isinstance", new(allocator) function_def(builtin_isinstance));
    ctx->store("open", new(allocator) function_def(builtin_open));
    ctx->store("ord", new(allocator) function_def(builtin_ord));
    ctx->store("print", new(allocator) function_def(builtin_print));
    ctx->store("print_nonl", new(allocator) function_def(builtin_print_nonl));
    ctx->store("range", new(allocator) function_def(builtin_range));
    ctx->store("reversed", new(allocator) function_def(builtin_reversed));
    ctx->store("set", new(allocator) function_def(builtin_set));
    ctx->store("sorted", new(allocator) function_def(builtin_sorted));
    ctx->store("zip", new(allocator) function_def(builtin_zip));

    ctx->store("int", &builtin_class_int);
    ctx->store("str", &builtin_class_str);

    ctx->store("__name__", new(allocator) string_const("__main__"));
    list *plist = new(allocator) list();
    for (int_t a = 0; a < argc; a++)
        plist->append(new(allocator) string_const(argv[a]));
    ctx->store("__args__", plist);
}
