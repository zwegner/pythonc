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
    assert(0);
}

class node;
class dict;
class list;
class context;
class string_const;

typedef int64_t int_t;

typedef std::pair<node *, node *> node_pair;
typedef std::map<const char *, node *> symbol_table;
typedef std::map<int_t, node_pair> node_dict;
typedef std::map<int_t, node *> node_set;
typedef std::vector<node *> node_list;

node *builtin_dict_get(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_dict_keys(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_fread(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_isinstance(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_len(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_list_append(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_list_index(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_list_pop(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_open(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_ord(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_print(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_print_nonl(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_range(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_reversed(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_set(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_set_add(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_sorted(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_str_join(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_str_upper(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_str_startswith(context *globals, context *ctx, list *args, dict *kwargs);
node *builtin_zip(context *globals, context *ctx, list *args, dict *kwargs);

inline node *create_bool_const(bool b);

class node {
private:
public:
    node() { }
    virtual const char *node_type() { return "node"; }

    virtual void mark_live() { error("mark_live unimplemented for %s", this->node_type()); }
#define MARK_LIVE_FN \
    virtual void mark_live() { allocator->mark_live(this, sizeof(*this)); }
#define MARK_LIVE_SINGLETON_FN virtual void mark_live() { }

    virtual bool is_bool() { return false; }
    virtual bool is_dict() { return false; }
    virtual bool is_file() { return false; }
    virtual bool is_function() { return false; }
    virtual bool is_int_const() { return false; }
    virtual bool is_list() { return false; }
    virtual bool is_none() { return false; }
    virtual bool is_set() { return false; }
    virtual bool is_string() { return false; }
    virtual bool bool_value() { error("bool_value unimplemented for %s", this->node_type()); return false; }
    virtual int_t int_value() { error("int_value unimplemented for %s", this->node_type()); return 0; }
    virtual std::string string_value() { error("string_value unimplemented for %s", this->node_type()); return NULL; }
    virtual node_list *list_value() { error("list_value unimplemented for %s", this->node_type()); return NULL; }

#define UNIMP_OP(NAME) \
    virtual node *__##NAME##__(node *rhs) { error(#NAME " unimplemented for %s", this->node_type()); return NULL; }

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
    virtual bool _##NAME(node *rhs) { error(#NAME " unimplemented for %s", this->node_type()); return false; } \
    node *__##NAME##__(node *rhs) { return create_bool_const(this->_##NAME(rhs)); }

    UNIMP_CMP_OP(eq)
    UNIMP_CMP_OP(ne)
    UNIMP_CMP_OP(lt)
    UNIMP_CMP_OP(le)
    UNIMP_CMP_OP(gt)
    UNIMP_CMP_OP(ge)

    UNIMP_OP(contains)

#define UNIMP_UNOP(NAME) \
    virtual node *__##NAME##__() { error(#NAME " unimplemented for %s", this->node_type()); return NULL; }

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

    virtual node *__call__(context *globals, context *ctx, list *args, dict *kwargs) { error("call unimplemented for %s", this->node_type()); return NULL; }
    virtual void __delitem__(node *rhs) { error("delitem unimplemented for %s", this->node_type()); }
    virtual node *__getitem__(node *rhs) { error("getitem unimplemented for %s", this->node_type()); return NULL; }
    virtual node *__getitem__(int index) { error("getitem unimplemented for %s", this->node_type()); return NULL; }
    virtual void __setattr__(node *rhs, node *key) { error("setattr unimplemented for %s", this->node_type()); }
    virtual void __setitem__(node *key, node *value) { error("setitem unimplemented for %s", this->node_type()); }
    virtual node *__slice__(node *start, node *end, node *step) { error("slice unimplemented for %s", this->node_type()); return NULL; }

    // unwrapped versions
    virtual int_t len() { error("len unimplemented for %s", this->node_type()); return 0; }
    virtual node *getattr(const char *rhs) { error("getattr unimplemented (%s) for %s", rhs, this->node_type()); return NULL; }
    virtual int_t hash() { error("hash unimplemented for %s", this->node_type()); return 0; }
    virtual std::string str() { error("str unimplemented for %s", this->node_type()); return NULL; }
};

class context {
private:
    symbol_table symbols;
    context *parent_ctx;

public:
    context() {
        this->parent_ctx = NULL;
    }
    context(context *parent_ctx) {
        this->parent_ctx = parent_ctx;
    }

    void mark_live(bool free_ctx) {
        if (!free_ctx)
            for (symbol_table::const_iterator i = this->symbols.begin(); i != this->symbols.end(); i++)
                i->second->mark_live();
        if (this->parent_ctx)
            this->parent_ctx->mark_live(false);
    }

    void store(const char *name, node *obj) {
        this->symbols[name] = obj;
    }
    node *load(const char *name) {
        symbol_table::const_iterator v = this->symbols.find(name);
        if (v == this->symbols.end())
            error("cannot find '%s' in symbol table", name);
        return v->second;
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
    none_const(int_t value) { }
    const char *node_type() { return "none"; }

    MARK_LIVE_SINGLETON_FN

    virtual bool is_none() { return true; }
    virtual bool bool_value() { return false; }

    virtual bool _eq(node *rhs);
    virtual int_t hash() { return 0; }
    virtual std::string str() { return std::string("None"); }
};

class int_const : public node {
private:
    int_t value;

public:
    int_const(int_t value) {
        this->value = value;
    }
    const char *node_type() { return "int"; }

    MARK_LIVE_FN

    virtual bool is_int_const() { return true; }
    virtual int_t int_value() { return this->value; }
    virtual bool bool_value() { return this->value != 0; }

#define INT_OP(NAME, OP) \
    virtual int_t _##NAME(node *rhs) { \
        return this->int_value() OP rhs->int_value(); \
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
        return this->int_value() OP rhs->int_value(); \
    } \

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

class int_const_singleton : public int_const {
public:
    int_const_singleton(int_t value) : int_const(value) { }

    MARK_LIVE_SINGLETON_FN
};

class bool_const : public node {
private:
    bool value;

public:
    bool_const(bool value) {
        this->value = value;
    }
    const char *node_type() { return "bool"; }

    MARK_LIVE_SINGLETON_FN

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

#define BOOL_INT_CHECK_OP(NAME, OP) \
    virtual node *__##NAME##__(node *rhs) { \
        if (rhs->is_bool()) \
            return new(allocator) bool_const((bool)(this->int_value() OP rhs->int_value())); \
        else if (rhs->is_int_const()) \
            return new(allocator) int_const(this->int_value() OP rhs->int_value()); \
        error(#NAME " error in bool"); \
        return NULL; \
    }

    BOOL_INT_CHECK_OP(and, &)
    BOOL_INT_CHECK_OP(or, |)
    BOOL_INT_CHECK_OP(xor, ^)

#define BOOL_OP(NAME, OP) \
    virtual bool _##NAME(node *rhs) { \
        if (rhs->is_int_const() || rhs->is_bool()) \
            return this->int_value() OP rhs->int_value(); \
        error(#NAME " error in bool"); \
        return false; \
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
    string_const(std::string value) {
        this->value = value;
    }
    const char *node_type() { return "str"; }

    MARK_LIVE_FN

    virtual bool is_string() { return true; }
    virtual std::string string_value() { return this->value; }
    virtual bool bool_value() { return this->len() != 0; }

    std::string::iterator begin() { return value.begin(); }
    std::string::iterator end() { return value.end(); }

#define STRING_OP(NAME, OP) \
    virtual bool _##NAME(node *rhs) { \
        if (rhs->is_string()) \
            return this->string_value() OP rhs->string_value(); \
        error(#NAME " unimplemented"); \
        return false; \
    } \

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

class string_const_singleton : public string_const {
private:
    int_t hashkey;

public:
    string_const_singleton(std::string value, int_t hashkey) : string_const(value), hashkey(hashkey) { }

    MARK_LIVE_SINGLETON_FN

    virtual int_t hash() {
        return this->hashkey;
    }
};

class list : public node {
private:
    node_list items;

public:
    list() { }
    list(node_list &l) : items(l) { }
    const char *node_type() { return "list"; }

    virtual void mark_live() {
        if (!allocator->mark_live(this, sizeof(*this))) {
            for (size_t i = 0; i < this->items.size(); i++)
                this->items[i]->mark_live();
        }
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
    virtual bool bool_value() { return this->len() != 0; }

    virtual node *__add__(node *rhs);
    virtual node *__mul__(node *rhs);

    virtual node *__contains__(node *key) {
        bool found = false;
        for (size_t i = 0; i < this->items.size(); i++)
            if (this->items[i]->_eq(key)) {
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
    dict() { }
    const char *node_type() { return "dict"; }

    virtual void mark_live() {
        if (!allocator->mark_live(this, sizeof(*this))) {
            for (node_dict::iterator i = this->items.begin(); i != this->items.end(); i++) {
                i->second.first->mark_live();
                i->second.second->mark_live();
            }
        }
    }

    node *lookup(node *key) {
        int_t hashkey;
        if (key->is_int_const())
            hashkey = key->int_value();
        else
            hashkey = key->hash();
        node_dict::const_iterator v = this->items.find(hashkey);
        if (v == this->items.end())
            return NULL;
        node *k = v->second.first;
        if (!k->_eq(key))
            return NULL;
        return v->second.second;
    }
    node_dict::iterator begin() { return items.begin(); }
    node_dict::iterator end() { return items.end(); }

    virtual bool is_dict() { return true; }
    virtual bool bool_value() { return this->len() != 0; }

    virtual node *__contains__(node *key) {
        return create_bool_const(this->lookup(key) != NULL);
    }
    virtual node *__getitem__(node *key) {
        node *value = this->lookup(key);
        if (value == NULL)
            error("cannot find '%s' in dict", key->str().c_str());
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
    set() { }
    const char *node_type() { return "set"; }

    virtual void mark_live() {
        if (!allocator->mark_live(this, sizeof(*this))) {
            for (node_set::iterator i = this->items.begin(); i != this->items.end(); i++)
                i->second->mark_live();
        }
    }

    node *lookup(node *key) {
        int_t hashkey;
        if (key->is_int_const())
            hashkey = key->int_value();
        else
            hashkey = key->hash();
        node_set::const_iterator v = this->items.find(hashkey);
        if (v == this->items.end() || !v->second->_eq(key))
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
    virtual bool bool_value() { return this->len() != 0; }

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
    dict *items;

public:
    object() {
        this->items = new(allocator) dict();
    }
    const char *node_type() { return "object"; }

    virtual void mark_live() {
        if (!allocator->mark_live(this, sizeof(*this)))
            this->items->mark_live();
    }

    virtual bool bool_value() { return true; }

    virtual node *getattr(const char *key) {
        return items->__getitem__(new(allocator) string_const(key));
    }
    virtual void __setattr__(node *key, node *value) {
        items->__setitem__(key, value);
    }
    virtual bool _eq(node *rhs) {
        return this == rhs;
    }
};

class file : public node {
private:
    FILE *f;

public:
    file(const char *path, const char *mode) {
        f = fopen(path, mode);
        if (!f)
            error("%s: file not found", path);
    }
    const char *node_type() { return "file"; }

    MARK_LIVE_FN

    node *read(int_t len) {
        static char buf[64*1024];
        size_t ret = fread(buf, 1, len, this->f);
        std::string s(buf, ret);
        return new(allocator) string_const(s);
    }

    virtual bool is_file() { return true; }
};

typedef node *(*fptr)(context *globals, context *parent_ctx, list *args, dict *kwargs);

class bound_method : public node {
private:
    node *self;
    node *function;

public:
    bound_method(node *self, node *function) {
        this->self = self;
        this->function = function;
    }
    const char *node_type() { return "bound_method"; }

    virtual void mark_live() {
        if (!allocator->mark_live(this, sizeof(*this))) {
            this->self->mark_live();
            this->function->mark_live();
        }
    }

    virtual bool is_function() { return true; } // XXX is it?

    virtual node *__call__(context *globals, context *ctx, list *args, dict *kwargs) {
        if (!args->is_list())
            error("call with non-list args?");
        ((list *)args)->prepend(this->self);
        return this->function->__call__(globals, ctx, args, kwargs);
    }
};

class function_def : public node {
private:
    fptr base_function;

public:
    function_def(fptr base_function) {
        this->base_function = base_function;
    }
    const char *node_type() { return "function"; }

    MARK_LIVE_FN

    virtual bool is_function() { return true; }

    virtual node *__call__(context *globals, context *ctx, list *args, dict *kwargs) {
        return this->base_function(globals, ctx, args, kwargs);
    }
};

class class_def : public node {
private:
    std::string name;
    dict *items;

public:
    class_def(std::string name, void (*creator)(class_def *)) {
        this->name = name;
        this->items = new(allocator) dict();
        creator(this);
    }
    const char *node_type() { return "class"; }

    virtual void mark_live() {
        if (!allocator->mark_live(this, sizeof(*this)))
            this->items->mark_live();
    }

    node *load(const char *name) {
        return items->__getitem__(new(allocator) string_const(name));
    }
    void store(const char *name, node *value) {
        items->__setitem__(new(allocator) string_const(name), value);
    }

    virtual node *__call__(context *globals, context *ctx, list *args, dict *kwargs) {
        node *init = this->load("__init__");
        node *obj = new(allocator) object();

        obj->__setattr__(new(allocator) string_const("__class__"), this);

        // Create bound methods
        for (node_dict::iterator i = items->begin(); i != items->end(); i++)
            if (i->second.second->is_function())
                obj->__setattr__(i->second.first, new(allocator) bound_method(obj, i->second.second));

        ((list *)args)->prepend(obj);
        context *call_ctx = new(allocator) context(ctx);
        init->__call__(globals, call_ctx, args, kwargs);
        return obj;
    }
    virtual node *getattr(const char *attr) {
        return this->load(attr);
    }
};

class class_def_singleton : public class_def {
public:
    class_def_singleton(std::string name, void (*creator)(class_def *)) : class_def(name, creator) { }

    MARK_LIVE_SINGLETON_FN
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

class_def_singleton builtin_class_int("int", _int__create_);
class_def_singleton builtin_class_str("str", _str__create_);

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
    return create_bool_const(!this->bool_value());
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

bool none_const::_eq(node *rhs) {
    return (this == rhs);
}

node *int_const::getattr(const char *key) {
    if (!strcmp(key, "__class__"))
        return &builtin_class_int;
    error("int has no attribute %s", key);
    return NULL;
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
    return NULL;
}

node *dict::getattr(const char *key) {
    if (!strcmp(key, "get"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_dict_get));
    else if (!strcmp(key, "keys"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_dict_keys));
    error("dict has no attribute %s", key);
    return NULL;
}

node *set::getattr(const char *key) {
    if (!strcmp(key, "add"))
        return new(allocator) bound_method(this, new(allocator) function_def(builtin_set_add));
    error("set has no attribute %s", key);
    return NULL;
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
    return NULL;
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

            if ((unsigned)(fmt - fmt_buf) >= sizeof(buf))
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

node *builtin_dict_get(context *globals, context *ctx, list *args, dict *kwargs) {
    if (args->len() != 3) // just assume 3 args for now...
        error("bad number of arguments to dict.get()");
    node *self = args->__getitem__(0);
    node *key = args->__getitem__(1);

    node *value = ((dict *)self)->lookup(key);
    if (!value)
        value = args->__getitem__(2);

    return value;
}

node *builtin_dict_keys(context *globals, context *ctx, list *args, dict *kwargs) {
    if (args->len() != 1)
        error("bad number of arguments to dict.keys()");
    dict *self = (dict *)args->__getitem__(0);

    list *plist = new(allocator) list();
    for (node_dict::iterator i = self->begin(); i != self->end(); i++)
        plist->append(i->second.first);

    return plist;
}

node *builtin_fread(context *globals, context *ctx, list *args, dict *kwargs) {
    node *f = args->__getitem__(0);
    node *len = args->__getitem__(1);
    if (!f->is_file() || !len->is_int_const())
        error("bad arguments to fread()");
    return ((file *)f)->read(len->int_value());
}

node *builtin_isinstance(context *globals, context *ctx, list *args, dict *kwargs) {
    node *obj = args->__getitem__(0);
    node *arg_class = args->__getitem__(1);

    node *obj_class = obj->getattr("__class__");
    return create_bool_const(obj_class == arg_class);
}

node *builtin_len(context *globals, context *ctx, list *args, dict *kwargs) {
    return args->__getitem__(0)->__len__();
}

node *builtin_list_append(context *globals, context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    node *item = args->__getitem__(1);

    ((list *)self)->append(item);

    return &none_singleton;
}

node *builtin_list_index(context *globals, context *ctx, list *args, dict *kwargs) {
    if (args->len() != 2)
        error("bad number of arguments to list.index()");
    node *self = args->__getitem__(0);
    node *key = args->__getitem__(1);

    for (int_t i = 0; i < self->len(); i++)
        if (self->__getitem__(i)->_eq(key))
            return new(allocator) int_const(i);
    error("item not found in list");
    return &none_singleton;
}

node *builtin_list_pop(context *globals, context *ctx, list *args, dict *kwargs) {
    list *self = (list *)args->__getitem__(0);

    return self->pop();
}

node *builtin_open(context *globals, context *ctx, list *args, dict *kwargs) {
    node *path = args->__getitem__(0);
    node *mode = args->__getitem__(1);
    if (!path->is_string() || !mode->is_string())
        error("bad arguments to open()");
    file *f = new(allocator) file(path->string_value().c_str(), mode->string_value().c_str());
    return f;
}

node *builtin_ord(context *globals, context *ctx, list *args, dict *kwargs) {
    node *arg = args->__getitem__(0);
    if (!arg->is_string() || arg->len() != 1)
        error("bad arguments to ord()");
    return new(allocator) int_const((unsigned char)arg->string_value()[0]);
}

node *builtin_print(context *globals, context *ctx, list *args, dict *kwargs) {
    std::string new_string;
    for (int_t i = 0; i < args->len(); i++) {
        if (i)
            new_string += " ";
        node *s = args->__getitem__(i);
        new_string += s->str();
    }
    printf("%s\n", new_string.c_str());
    return &none_singleton;
}

node *builtin_print_nonl(context *globals, context *ctx, list *args, dict *kwargs) {
    node *s = args->__getitem__(0);
    printf("%s", s->str().c_str());
    return &none_singleton;
}

node *builtin_range(context *globals, context *ctx, list *args, dict *kwargs) {
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

node *builtin_reversed(context *globals, context *ctx, list *args, dict *kwargs) {
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

node *builtin_set(context *globals, context *ctx, list *args, dict *kwargs) {
    return new(allocator) set();
}

node *builtin_set_add(context *globals, context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    node *item = args->__getitem__(1);

    ((set *)self)->add(item);

    return &none_singleton;
}

bool compare_nodes(node *lhs, node *rhs) {
    return lhs->_lt(rhs);
}

node *builtin_sorted(context *globals, context *ctx, list *args, dict *kwargs) {
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

node *builtin_str_join(context *globals, context *ctx, list *args, dict *kwargs) {
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

node *builtin_str_upper(context *globals, context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    if (!self->is_string())
        error("bad argument to str.upper()");
    string_const *str = (string_const *)self;

    std::string new_string;
    for (std::string::iterator c = str->begin(); c != str->end(); c++)
        new_string += toupper(*c);

    return new(allocator) string_const(new_string);
}

node *builtin_str_startswith(context *globals, context *ctx, list *args, dict *kwargs) {
    node *self = args->__getitem__(0);
    node *prefix = args->__getitem__(1);
    if (!self->is_string() || !prefix->is_string())
        error("bad arguments to str.startswith()");

    std::string s1 = self->string_value();
    std::string s2 = prefix->string_value();
    return create_bool_const(s1.compare(0, s2.size(), s2) == 0);
}

node *builtin_zip(context *globals, context *ctx, list *args, dict *kwargs) {
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

void collect_garbage(context *ctx, node *ret_val) {
    static int gc_tick = 0;
    if (++gc_tick > 128) {
        gc_tick = 0;

        allocator->mark_dead();

        ctx->mark_live(ret_val != NULL);

        if (ret_val)
            ret_val->mark_live();
    }
}
