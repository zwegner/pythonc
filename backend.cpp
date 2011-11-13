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

__attribute((noreturn)) void error(const char *msg, ...) {
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
class tuple;
class context;
class string_const;

typedef int64_t int_t;

typedef std::pair<node *, node *> node_pair;
typedef std::map<int_t, node_pair> node_dict;
typedef std::map<int_t, node *> node_set;
typedef std::vector<node *> node_list;

#define LIST_dict_CLASS_METHODS(x) \
    x(dict, get) \
    x(dict, keys) \
    x(dict, items) \
    x(dict, values) \

#define LIST_file_CLASS_METHODS(x) \
    x(file, read) \
    x(file, write) \

#define LIST_list_CLASS_METHODS(x) \
    x(list, append) \
    x(list, count) \
    x(list, extend) \
    x(list, index) \
    x(list, pop) \

#define LIST_set_CLASS_METHODS(x) \
    x(set, add) \
    x(set, update) \

#define LIST_str_CLASS_METHODS(x) \
    x(str, join) \
    x(str, split) \
    x(str, startswith) \
    x(str, upper) \

#define LIST_tuple_CLASS_METHODS(x) \
    x(tuple, count) \
    x(tuple, index) \

// XXX There is still some ugliness with file objects, so they're not included
#define LIST_BUILTIN_CLASSES_WITH_METHODS(x) \
    x(dict) \
    x(list) \
    x(set) \
    x(str) \
    x(tuple) \

#define LIST_BUILTIN_CLASS_METHODS(x) \
    LIST_dict_CLASS_METHODS(x) \
    LIST_file_CLASS_METHODS(x) \
    LIST_list_CLASS_METHODS(x) \
    LIST_set_CLASS_METHODS(x) \
    LIST_str_CLASS_METHODS(x) \
    LIST_tuple_CLASS_METHODS(x) \

#define BUILTIN_FUNCTION(name) \
    node *wrapped_builtin_##name(context *globals, context *ctx, tuple *args, dict *kwargs);
LIST_BUILTIN_FUNCTIONS(BUILTIN_FUNCTION)
#undef BUILTIN_FUNCTION

#define BUILTIN_METHOD(class_name, method_name) \
    node *wrapped_builtin_##class_name##_##method_name(context *globals, context *ctx, tuple *args, dict *kwargs);
LIST_BUILTIN_CLASS_METHODS(BUILTIN_METHOD)
#undef BUILTIN_METHOD

inline node *create_bool_const(bool b);

class node {
private:
public:
    node() { }
    virtual const char *node_type() { return "node"; }

    virtual void mark_live() { error("mark_live unimplemented for %s", this->node_type()); }
#define MARK_LIVE_FN \
    virtual void mark_live() { allocator->mark_live<sizeof(*this)>(this); }
#define MARK_LIVE_SINGLETON_FN virtual void mark_live() { }

    virtual bool is_bool() { return false; }
    virtual bool is_dict() { return false; }
    virtual bool is_file() { return false; }
    virtual bool is_function() { return false; }
    virtual bool is_int_const() { return false; }
    virtual bool is_list() { return false; }
    virtual bool is_tuple() { return false; }
    virtual bool is_none() { return false; }
    virtual bool is_set() { return false; }
    virtual bool is_string() { return false; }
    virtual bool bool_value() { error("bool_value unimplemented for %s", this->node_type()); return false; }
    virtual int_t int_value() { error("int_value unimplemented for %s", this->node_type()); return 0; }
    virtual std::string string_value() { error("string_value unimplemented for %s", this->node_type()); return NULL; }
    virtual const char *c_str() { error("c_str unimplemented for %s", this->node_type()); }
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
    UNIMP_UNOP(abs)

    node *__len__();
    node *__hash__();
    node *__getattr__(node *rhs);
    node *__not__();
    node *__is__(node *rhs);
    node *__isnot__(node *rhs);
    node *__repr__();
    node *__str__();

    virtual node *__ncontains__(node *rhs) { return this->__contains__(rhs)->__not__(); }

    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs) { error("call unimplemented for %s", this->node_type()); return NULL; }
    virtual void __delitem__(node *rhs) { error("delitem unimplemented for %s", this->node_type()); }
    virtual node *__getitem__(node *rhs) { error("getitem unimplemented for %s", this->node_type()); return NULL; }
    virtual node *__getitem__(int index) { error("getitem unimplemented for %s", this->node_type()); return NULL; }
    virtual node *__iter__() { error("iter unimplemented for %s", this->node_type()); }
    virtual node *next() { error("next unimplemented for %s", this->node_type()); }
    virtual void __setattr__(node *rhs, node *key) { error("setattr unimplemented for %s", this->node_type()); }
    virtual void __setitem__(node *key, node *value) { error("setitem unimplemented for %s", this->node_type()); }
    virtual node *__slice__(node *start, node *end, node *step) { error("slice unimplemented for %s", this->node_type()); return NULL; }

    // unwrapped versions
    virtual int_t len() { error("len unimplemented for %s", this->node_type()); return 0; }
    virtual node *getattr(const char *key);
    virtual int_t hash() { error("hash unimplemented for %s", this->node_type()); return 0; }
    virtual std::string repr() { error("repr unimplemented for %s", this->node_type()); return NULL; }
    virtual std::string str() { return repr(); }
    virtual node *type() { error("type unimplemented for %s", this->node_type()); }
};

class context {
private:
    uint32_t sym_len;
    node **symbols;
    context *parent_ctx;

public:
    context(uint32_t size, node **symbols) {
        this->parent_ctx = NULL;
        this->symbols = symbols;
        this->sym_len = size;
    }
    context(context *parent_ctx, uint32_t size, node **symbols) {
        this->parent_ctx = parent_ctx;
        this->symbols = symbols;
        this->sym_len = size;
    }

    void mark_live(bool free_ctx) {
        if (!free_ctx)
            for (uint32_t i = 0; i < this->sym_len; i++)
                if (this->symbols[i])
                    this->symbols[i]->mark_live();
        if (this->parent_ctx)
            this->parent_ctx->mark_live(false);
    }

    void store(uint32_t idx, node *obj) {
        this->symbols[idx] = obj;
    }
    node *load(uint32_t idx) {
        node *ret = this->symbols[idx];
        if (!ret)
            error("name is not defined");
        return ret;
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
    virtual std::string repr() { return std::string("None"); }
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
    INT_UNOP(pos, +)
    INT_UNOP(neg, -)

    virtual node *__abs__() {
        int_t ret = (this->value >= 0) ? this->value : -this->value;
        return new(allocator) int_const(ret);
    }

    virtual int_t hash() { return this->value; }
    virtual std::string repr();
    virtual node *type();
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
    virtual std::string repr();
    virtual node *type();
};

class string_const : public node {
private:
    class str_iter: public node {
    private:
        string_const *parent;
        std::string::iterator it;

    public:
        str_iter(string_const *s) {
            this->parent = s;
            it = s->value.begin();
        }
        const char *node_type() { return "str_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->value.end())
                return NULL;
            char ret[2];
            ret[0] = *this->it;
            ret[1] = 0;
            ++this->it;
            return new(allocator) string_const(ret);
        }
    };

    std::string value;

public:
    string_const(const char *x): value(x) {}
    string_const(std::string x): value(x) {}
    const char *node_type() { return "str"; }

    MARK_LIVE_FN

    virtual bool is_string() { return true; }
    virtual std::string string_value() { return this->value; }
    virtual bool bool_value() { return this->len() != 0; }
    virtual const char *c_str() { return this->value.c_str(); }

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
        for (auto c = this->begin(); c != this->end(); c++) {
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
    virtual std::string repr() {
        bool has_single_quotes = false;
        bool has_double_quotes = false;
        for (auto it = this->begin(); it != this->end(); ++it) {
            char c = *it;
            if (c == '\'')
                has_single_quotes = true;
            else if (c == '"')
                has_double_quotes = true;
        }
        bool use_double_quotes = has_single_quotes && !has_double_quotes;
        std::string s(use_double_quotes ? "\"" : "'");
        for (auto it = this->begin(); it != this->end(); ++it) {
            char c = *it;
            if (c == '\n')
                s += "\\n";
            else if (c == '\r')
                s += "\\r";
            else if (c == '\t')
                s += "\\t";
            else if (c == '\\')
                s += "\\\\";
            else if ((c == '\'') && !use_double_quotes)
                s += "\\'";
            else
                s += c;
        }
        s += use_double_quotes ? "\"" : "'";
        return s;
    }
    virtual std::string str() { return this->value; }
    virtual node *type();
    virtual node *__iter__() { return new(allocator) str_iter(this); }
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

class bytes: public node {
private:
    class bytes_iter: public node {
    private:
        bytes *parent;
        std::vector<uint8_t>::iterator it;

    public:
        bytes_iter(bytes *b) {
            this->parent = b;
            it = b->value.begin();
        }
        const char *node_type() { return "bytes_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->value.end())
                return NULL;
            uint8_t ret = *this->it;
            ++this->it;
            return new(allocator) int_const(ret);
        }
    };

    std::vector<uint8_t> value;

public:
    bytes() {}
    bytes(size_t len, const uint8_t *data): value(len) {
        memcpy(&value[0], data, len);
    }

    const char *node_type() { return "bytes"; }

    MARK_LIVE_FN

    void append(uint8_t x) { this->value.push_back(x); }

    virtual bool bool_value() { return this->len() != 0; }

    virtual int_t len() { return this->value.size(); }
    virtual node *type();

    virtual std::string repr() {
        std::string s("b'");
        for (size_t i = 0; i < this->value.size(); i++) {
            uint8_t x = this->value[i];
            char buf[5];
            if (x == '\n')
                s += "\\n";
            else if (x == '\r')
                s += "\\r";
            else if (x == '\t')
                s += "\\t";
            else if (x == '\'')
                s += "\\'";
            else if (x == '\\')
                s += "\\\\";
            else if ((x >= 0x20) && (x < 0x7F)) {
                buf[0] = x;
                buf[1] = 0;
                s += std::string(buf);
            }
            else {
                buf[0] = '\\';
                buf[1] = 'x';
                buf[2] = "0123456789abcdef"[x >> 4];
                buf[3] = "0123456789abcdef"[x & 15];
                buf[4] = 0;
                s += std::string(buf);
            }
        }
        return s + "'";
    }
    virtual node *__iter__() { return new(allocator) bytes_iter(this); }
};

class bytes_singleton: public bytes {
public:
    bytes_singleton(size_t len, const uint8_t *data): bytes(len, data) {}

    MARK_LIVE_SINGLETON_FN
};

class list : public node {
private:
    class list_iter: public node {
    private:
        list *parent;
        node_list::iterator it;

    public:
        list_iter(list *l) {
            this->parent = l;
            it = l->items.begin();
        }
        const char *node_type() { return "list_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->items.end())
                return NULL;
            node *ret = *this->it;
            ++this->it;
            return ret;
        }
    };

    node_list items;

public:
    list() { }
    list(int_t n, node **items): items(n) {
        for (int_t i = 0; i < n; i++)
            this->items[i] = items[i];
    }
    const char *node_type() { return "list"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (size_t i = 0; i < this->items.size(); i++)
                this->items[i]->mark_live();
        }
    }

    void append(node *obj) {
        items.push_back(obj);
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
        int_t size = items.size();
        if ((base >= size) || (base < -size))
            error("list index out of range");
        if (base < 0)
            base += size;
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
        auto f = items.begin() + this->index(rhs->int_value());
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
        list *new_list = new(allocator) list;
        for (; st > 0 ? (lo < hi) : (lo > hi); lo += st)
            new_list->append(items[lo]);
        return new_list;
    }
    virtual std::string repr() {
        std::string new_string = "[";
        bool first = true;
        for (auto i = this->items.begin(); i != this->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += (*i)->repr();
        }
        new_string += "]";
        return new_string;
    }
    virtual node *type();
    virtual node *__iter__() { return new(allocator) list_iter(this); }
};

class tuple: public node {
private:
    class tuple_iter: public node {
    private:
        tuple *parent;
        node_list::iterator it;

    public:
        tuple_iter(tuple *t) {
            this->parent = t;
            it = t->items.begin();
        }
        const char *node_type() { return "tuple_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->items.end())
                return NULL;
            node *ret = *this->it;
            ++this->it;
            return ret;
        }
    };

    node_list items;

public:
    tuple() { }
    tuple(int_t n, node **items): items(n) {
        for (int_t i = 0; i < n; i++)
            this->items[i] = items[i];
    }
    const char *node_type() { return "tuple"; }
    virtual bool is_tuple() { return true; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (size_t i = 0; i < this->items.size(); i++)
                this->items[i]->mark_live();
        }
    }

    void append(node *obj) { this->items.push_back(obj); }

    int_t index(int_t base) {
        int_t size = items.size();
        if ((base >= size) || (base < -size))
            error("tuple index out of range");
        if (base < 0)
            base += size;
        return base;
    }

    virtual bool bool_value() { return this->len() != 0; }

    virtual node *__add__(node *rhs);
    virtual node *__mul__(node *rhs);

    virtual node *__contains__(node *key) {
        bool found = false;
        for (size_t i = 0; i < this->items.size(); i++) {
            if (this->items[i]->_eq(key)) {
                found = true;
                break;
            }
        }
        return create_bool_const(found);
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
    virtual node *type();
    virtual std::string repr() {
        std::string new_string = "(";
        bool first = true;
        for (auto i = this->items.begin(); i != this->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += (*i)->repr();
        }
        if (this->items.size() == 1)
            new_string += ",";
        new_string += ")";
        return new_string;
    }
    virtual node *__iter__() { return new(allocator) tuple_iter(this); }
};

class dict : public node {
private:
    class dict_keys_iter: public node {
    private:
        dict *parent;
        node_dict::iterator it;

    public:
        dict_keys_iter(dict *d) {
            this->parent = d;
            it = d->items.begin();
        }
        const char *node_type() { return "dict_keys_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->items.end())
                return NULL;
            node *ret = this->it->second.first;
            ++this->it;
            return ret;
        }
    };
    class dict_items_iter: public node {
    private:
        dict *parent;
        node_dict::iterator it;

    public:
        dict_items_iter(dict *d) {
            this->parent = d;
            it = d->items.begin();
        }
        const char *node_type() { return "dict_items_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->items.end())
                return NULL;
            node *pair[2] = {this->it->second.first, this->it->second.second};
            node *ret = new(allocator) tuple(2, pair);
            ++this->it;
            return ret;
        }
    };
    class dict_values_iter: public node {
    private:
        dict *parent;
        node_dict::iterator it;

    public:
        dict_values_iter(dict *d) {
            this->parent = d;
            it = d->items.begin();
        }
        const char *node_type() { return "dict_values_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->items.end())
                return NULL;
            node *ret = this->it->second.second;
            ++this->it;
            return ret;
        }
    };

    node_dict items;

public:
    dict() { }
    const char *node_type() { return "dict"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (auto i = this->items.begin(); i != this->items.end(); i++) {
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
            error("cannot find %s in dict", key->repr().c_str());
        return value;
    }
    virtual int_t len() {
        return this->items.size();
    }
    virtual void __setitem__(node *key, node *value) {
        items[key->hash()] = node_pair(key, value);
    }
    virtual std::string repr() {
        std::string new_string = "{";
        bool first = true;
        for (auto i = this->items.begin(); i != this->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += i->second.first->repr() + ": " + i->second.second->repr();
        }
        new_string += "}";
        return new_string;
    }
    virtual node *type();
    virtual node *__iter__() { return new(allocator) dict_keys_iter(this); }

    friend class dict_keys;
    friend class dict_items;
    friend class dict_values;
};

class dict_keys: public node {
private:
    dict *parent;

public:
    dict_keys(dict *d) {
        this->parent = d;
    }
    const char *node_type() { return "dict_keys"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->parent->mark_live();
    }

    virtual int_t len() { return this->parent->items.size(); }
    virtual node *__iter__() { return new(allocator) dict::dict_keys_iter(this->parent); }
    virtual std::string repr() {
        std::string new_string = "dict_keys([";
        bool first = true;
        for (auto i = this->parent->items.begin(); i != this->parent->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += i->second.first->repr();
        }
        new_string += "])";
        return new_string;
    }
};

class dict_items: public node {
private:
    dict *parent;

public:
    dict_items(dict *d) {
        this->parent = d;
    }
    const char *node_type() { return "dict_items"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->parent->mark_live();
    }

    virtual int_t len() { return this->parent->items.size(); }
    virtual node *__iter__() { return new(allocator) dict::dict_items_iter(this->parent); }
    virtual std::string repr() {
        std::string new_string = "dict_items([";
        bool first = true;
        for (auto i = this->parent->items.begin(); i != this->parent->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += "(";
            new_string += i->second.first->repr();
            new_string += ", ";
            new_string += i->second.second->repr();
            new_string += ")";
        }
        new_string += "])";
        return new_string;
    }
};

class dict_values: public node {
private:
    dict *parent;

public:
    dict_values(dict *d) {
        this->parent = d;
    }
    const char *node_type() { return "dict_values"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->parent->mark_live();
    }

    virtual int_t len() { return this->parent->items.size(); }
    virtual node *__iter__() { return new(allocator) dict::dict_values_iter(this->parent); }
    virtual std::string repr() {
        std::string new_string = "dict_values([";
        bool first = true;
        for (auto i = this->parent->items.begin(); i != this->parent->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += i->second.second->repr();
        }
        new_string += "])";
        return new_string;
    }
};

class set : public node {
private:
    class set_iter: public node {
    private:
        set *parent;
        node_set::iterator it;

    public:
        set_iter(set *s) {
            this->parent = s;
            it = s->items.begin();
        }
        const char *node_type() { return "set_iter"; }

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->items.end())
                return NULL;
            node *ret = this->it->second;
            ++this->it;
            return ret;
        }
    };

    node_set items;

public:
    set() { }
    const char *node_type() { return "set"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (auto i = this->items.begin(); i != this->items.end(); i++)
                i->second->mark_live();
        }
    }

    node *lookup(node *key) {
        node_set::const_iterator v = this->items.find(key->hash());
        if (v == this->items.end() || !v->second->_eq(key))
            return NULL;
        return v->second;
    }
    void add(node *key) {
        items[key->hash()] = key;
    }

    virtual bool is_set() { return true; }
    virtual bool bool_value() { return this->len() != 0; }

    virtual node *__contains__(node *key) {
        return create_bool_const(this->lookup(key) != NULL);
    }
    virtual int_t len() {
        return this->items.size();
    }
    virtual std::string repr() {
        if (!this->items.size())
            return "set()";
        std::string new_string = "{";
        bool first = true;
        for (auto i = this->items.begin(); i != this->items.end(); i++) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += i->second->repr();
        }
        new_string += "}";
        return new_string;
    }
    virtual node *type();
    virtual node *__iter__() { return new(allocator) set_iter(this); }
};

class object : public node {
private:
    dict *items;

public:
    object() {
        this->items = new(allocator) dict;
    }
    const char *node_type() { return "object"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->items->mark_live();
    }

    virtual bool bool_value() { return true; }

    virtual node *getattr(const char *key) {
        return items->__getitem__(new(allocator) string_const(key));
    }
    virtual node *type() {
        return this->getattr("__class__");
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
    void write(string_const *data) {
        size_t len = data->len();
        const char *buf = data->c_str();
        (void)fwrite(buf, 1, len, this->f);
    }

    virtual bool is_file() { return true; }

    virtual node *getattr(const char *key);
};

class enumerate: public node {
private:
    node *iter;
    int_t i;

public:
    enumerate(node *iter) {
        this->iter = iter;
        this->i = 0;
    }
    const char *node_type() { return "enumerate"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->iter->mark_live();
    }

    virtual node *__iter__() { return this; }
    virtual node *next() {
        node *item = this->iter->next();
        if (!item)
            return NULL;
        node *pair[2];
        pair[0] = new(allocator) int_const(this->i++);
        pair[1] = item;
        return new(allocator) tuple(2, pair);
    }

    virtual node *type();
    virtual std::string repr() { return "<enumerate object>"; }
};

class range: public node {
private:
    class range_iter: public node {
    private:
        int_t start, end, step;

    public:
        range_iter(range *r) {
            this->start = r->start;
            this->end = r->end;
            this->step = r->step;
        }
        const char *node_type() { return "range_iter"; }
 
        MARK_LIVE_FN

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (step > 0) {
                if (this->start >= this->end)
                    return NULL;
            }
            else {
                if (this->start <= this->end)
                    return NULL;
            }
            node *ret = new(allocator) int_const(this->start);
            this->start += this->step;
            return ret;
        }
    };

    int_t start, end, step;

public:
    range(int_t start, int_t end, int_t step) {
        this->start = start;
        this->end = end;
        this->step = step;
    }
    const char *node_type() { return "range"; }
 
    MARK_LIVE_FN

    virtual node *__iter__() { return new(allocator) range_iter(this); }

    virtual node *type();
    virtual std::string repr() {
        char buf[128];
        if (step == 1) {
            sprintf(buf, "range(%ld, %ld)", this->start, this->end);
        }
        else {
            sprintf(buf, "range(%ld, %ld, %ld)", this->start, this->end, this->step);
        }
        return buf;
    }
};

class reversed: public node {
private:
    node *parent;
    int_t i;
    int_t len;

public:
    reversed(node *parent, int_t len) {
        this->parent = parent;
        this->i = 0;
        this->len = len;
    }
    const char *node_type() { return "reversed"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->parent->mark_live();
    }

    virtual node *__iter__() { return this; }
    virtual node *next() {
        if (i >= len)
            return NULL;
        int_t cur = this->i++;
        return this->parent->__getitem__(this->len - 1 - cur);
    }

    virtual std::string repr() { return "<reversed object>"; }
};

class zip: public node {
private:
    node *iter1;
    node *iter2;

public:
    zip(node *iter1, node *iter2) {
        this->iter1 = iter1;
        this->iter2 = iter2;
    }
    const char *node_type() { return "zip"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            this->iter1->mark_live();
            this->iter2->mark_live();
        }
    }

    virtual node *__iter__() { return this; }
    virtual node *next() {
        node *item1 = this->iter1->next();
        node *item2 = this->iter2->next();
        if (!item1 || !item2)
            return NULL;
        node *pair[2];
        pair[0] = item1;
        pair[1] = item2;
        return new(allocator) tuple(2, pair);
    }

    virtual node *type();
    virtual std::string repr() { return "<zip object>"; }
};

typedef node *(*fptr)(context *globals, context *parent_ctx, tuple *args, dict *kwargs);

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
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            this->self->mark_live();
            this->function->mark_live();
        }
    }

    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs) {
        int_t len = args->len();
        node *new_args[len + 1];
        new_args[0] = this->self;
        for (int_t i = 0; i < len; i++)
            new_args[i+1] = args->__getitem__(i);
        args = new(allocator) tuple(len + 1, new_args);
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

    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs) {
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
        this->items = new(allocator) dict;
        creator(this);
    }
    const char *node_type() { return "class"; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->items->mark_live();
    }

    node *load(const char *name) {
        return items->__getitem__(new(allocator) string_const(name));
    }
    void store(const char *name, node *value) {
        items->__setitem__(new(allocator) string_const(name), value);
    }

    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs) {
        node *init = this->load("__init__");
        node *obj = new(allocator) object();

        obj->__setattr__(new(allocator) string_const("__class__"), this);

        // Create bound methods
        for (auto i = items->begin(); i != items->end(); i++) {
            if (i->second.second->is_function())
                obj->__setattr__(i->second.first, new(allocator) bound_method(obj, i->second.second));
        }

        int_t len = args->len();
        node *new_args[len + 1];
        new_args[0] = obj;
        for (int_t i = 0; i < len; i++)
            new_args[i+1] = args->__getitem__(i);
        args = new(allocator) tuple(len + 1, new_args);
        init->__call__(globals, ctx, args, kwargs);
        return obj;
    }
    virtual node *getattr(const char *attr) {
        return this->load(attr);
    }
    virtual std::string repr() {
        return std::string("<class '") + this->name + "'>";
    }
};

bool_const bool_singleton_True(true);
bool_const bool_singleton_False(false);
none_const none_singleton(0);

inline node *create_bool_const(bool b) {
    return b ? &bool_singleton_True : &bool_singleton_False;
}

#define CHECK_MIN_MAX_ARGS(name, min_args, max_args) \
    if (args->len() < min_args) \
        error("too few arguments to " name "()"); \
    if (args->len() > max_args) \
        error("too many arguments to " name "()")

// Builtin classes
class builtin_method_def: public function_def {
public:
    builtin_method_def(fptr base_function): function_def(base_function) {}

    MARK_LIVE_SINGLETON_FN
};

#define BUILTIN_METHOD(class_name, method_name) builtin_method_def builtin_method_##class_name##_##method_name(wrapped_builtin_##class_name##_##method_name);
LIST_BUILTIN_CLASS_METHODS(BUILTIN_METHOD)
#undef BUILTIN_METHOD

class builtin_class: public node {
public:
    virtual const char *node_type() { return "type"; }

    virtual const char *type_name() = 0;

    MARK_LIVE_SINGLETON_FN

    // Don't call node::getattr as a fallback -- this will result in infinite recursion
    virtual node *getattr(const char *key) {
        if (!strcmp(key, "__name__"))
            return new(allocator) string_const(this->type_name());
        if (!strcmp(key, "__class__"))
            return type();
        error("%s has no attribute %s", type_name(), key);
    }

    virtual std::string repr() {
        return std::string("<class '") + this->type_name() + "'>";
    }
    virtual node *type();
};

class bool_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        if (!arg)
            return &bool_singleton_False;
        return create_bool_const(arg->bool_value());
    }
};

class bytes_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        bytes *ret = new(allocator) bytes;
        if (!arg)
            return ret;
        if (arg->is_int_const()) {
            int_t value = arg->int_value();
            if (value < 0)
                error("negative count");
            for (int_t i = 0; i < value; i++)
                ret->append(0);
            return ret;
        }
        node *iter = arg->__iter__();
        while (node *item = iter->next()) {
            int_t i = item->int_value();
            if ((i < 0) || (i >= 256))
                error("invalid byte value");
            ret->append(i);
        }
        return ret;
    }
};

class dict_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *getattr(const char *key);
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        dict *ret = new(allocator) dict;
        if (!arg)
            return ret;
        node *iter = arg->__iter__();
        while (node *item = iter->next()) {
            if (item->len() != 2)
                error("dictionary update sequence must have length 2");
            node *key = item->__getitem__(0);
            node *value = item->__getitem__(1);
            ret->__setitem__(key, value);
        }
        return ret;
    }
};

class enumerate_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        node *iter = arg->__iter__();
        return new(allocator) enumerate(iter);
    }
};

class int_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg0, node *arg1) {
        if (!arg0)
            return new(allocator) int_const(0);
        if (arg0->is_int_const()) {
            if (arg1)
                error("int() cannot accept a base when passed an int");
            return arg0;
        }
        if (arg0->is_bool()) {
            if (arg1)
                error("int() cannot accept a base when passed a bool");
            return new(allocator) int_const(arg0->int_value());
        }
        if (arg0->is_string()) {
            int_t base = 10;
            if (arg1) {
                if (!arg1->is_int_const())
                    error("base must be an int");
                base = arg1->int_value();
                if ((base < 0) || (base == 1) || (base > 36))
                    error("base must be 0 or 2-36");
                if (base == 0)
                    error("base 0 unsupported at present");
            }
            const char *s = arg0->c_str();
            while (isspace(*s))
                continue;
            int_t sign = 1;
            if (*s == '-') {
                s++;
                sign = -1;
            }
            else if (*s == '+')
                s++;
            int_t value = 0;
            for (;;) {
                int_t digit;
                char c = *s++;
                if (c == 0)
                    break;
                if ((c >= '0') && (c <= '9'))
                    digit = c - '0';
                else if ((c >= 'a') && (c <= 'z'))
                    digit = c - 'a' + 10;
                else if ((c >= 'A') && (c <= 'Z'))
                    digit = c - 'A' + 10;
                else
                    error("unexpected digit");
                if (digit >= base)
                    error("digit not valid in base");
                value = value*base + digit;
            }
            return new(allocator) int_const(sign*value);
        }
        error("don't know how to handle argument to int()");
    }
};

class list_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *getattr(const char *key);
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        list *ret = new(allocator) list;
        if (!arg)
            return ret;
        node *iter = arg->__iter__();
        while (node *item = iter->next())
            ret->append(item);
        return ret;
    }
};

class range_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg0, node *arg1, node *arg2) {
        int_t start = 0, end, step = 1;
        if (!arg1)
            end = arg0->int_value();
        else {
            start = arg0->int_value();
            end = arg1->int_value();
            if (arg2)
                step = arg2->int_value();
        }
        return new(allocator) range(start, end, step);
    }
};

class reversed_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);

    // XXX This will actually work on dictionaries if they have keys of 0..len-1.
    // Logically speaking it doesn't make sense to have reversed() of a dictionary
    // do anything, but the Python docs imply that __len__ and __getitem__ are
    // sufficient.  This seems like a documentation error.
    inline node *call(node *arg) {
        int_t len = arg->len();
        return new(allocator) reversed(arg, len);
    }
};

class set_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *getattr(const char *key);
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        set *ret = new(allocator) set;
        if (!arg)
            return ret;
        node *iter = arg->__iter__();
        while (node *item = iter->next())
            ret->add(item);
        return ret;
    }
};

class str_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *getattr(const char *key);
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        if (!arg)
            return new(allocator) string_const("");
        return arg->__str__();
    }
};

class tuple_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *getattr(const char *key);
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        tuple *ret = new(allocator) tuple;
        if (!arg)
            return ret;
        node *iter = arg->__iter__();
        while (node *item = iter->next())
            ret->append(item);
        return ret;
    }
};

class type_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg) {
        return arg->type();
    }
};

class zip_class: public builtin_class {
public:
    virtual const char *type_name();
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs);
    inline node *call(node *arg0, node *arg1) {
        node *iter1 = arg0->__iter__();
        node *iter2 = arg1->__iter__();
        return new(allocator) zip(iter1, iter2);
    }
};

#define GET_METHOD(class_name, method_name) \
    if (!strcmp(key, #method_name)) return &builtin_method_##class_name##_##method_name;
#define DEFINE_GETATTR(class_name) \
    node *class_name##_class::getattr(const char *key) {  \
        LIST_##class_name##_CLASS_METHODS(GET_METHOD) \
        return builtin_class::getattr(key); \
    }
LIST_BUILTIN_CLASSES_WITH_METHODS(DEFINE_GETATTR)
#undef GET_METHOD
#undef DEFINE_GETATTR


#define BUILTIN_CLASS(name) \
    const char *name##_class::type_name() { return #name; } \
    name##_class builtin_class_##name;
LIST_BUILTIN_CLASSES(BUILTIN_CLASS)
#undef BUILTIN_CLASS

node *node::__getattr__(node *key) {
    if (!key->is_string())
        error("getattr with non-string");
    return this->getattr(key->c_str());
}

node *node::getattr(const char *key) {
    if (!strcmp(key, "__class__"))
        return type();
    return new(allocator) bound_method(this, type()->getattr(key));
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

node *node::__repr__() {
    return new(allocator) string_const(this->repr());
}

node *node::__str__() {
    return new(allocator) string_const(this->str());
}

bool none_const::_eq(node *rhs) {
    return (this == rhs);
}

std::string int_const::repr() {
    char buf[32];
    sprintf(buf, "%" PRId64, this->value);
    return std::string(buf);
}

node *int_const::type() {
    return &builtin_class_int;
}

std::string bool_const::repr() {
    return std::string(this->value ? "True" : "False");
}

node *bool_const::type() {
    return &builtin_class_bool;
}

node *list::__add__(node *rhs) {
    if (!rhs->is_list())
        error("list add error");
    list *plist = new(allocator) list;
    node_list *rhs_list = rhs->list_value();
    for (auto i = this->begin(); i != this->end(); i++)
        plist->append(*i);
    for (auto i = rhs_list->begin(); i != rhs_list->end(); i++)
        plist->append(*i);
    return plist;
}

node *list::__mul__(node *rhs) {
    if (!rhs->is_int_const())
        error("list mul error");
    list *plist = new(allocator) list;
    for (int_t x = rhs->int_value(); x > 0; x--) {
        for (auto i = this->begin(); i != this->end(); i++)
            plist->append(*i);
    }
    return plist;
}

node *list::type() {
    return &builtin_class_list;
}

node *tuple::__add__(node *rhs) {
    if (!rhs->is_tuple())
        error("tuple add error");
    tuple *ret = new(allocator) tuple;
    tuple *rhs_tuple = (tuple *)rhs;
    for (auto it = this->items.begin(); it != this->items.end(); ++it)
        ret->append(*it);
    for (auto it = rhs_tuple->items.begin(); it != rhs_tuple->items.end(); ++it)
        ret->append(*it);
    return ret;
}

node *tuple::__mul__(node *rhs) {
    if (!rhs->is_int_const())
        error("list mul error");
    tuple *ret = new(allocator) tuple;
    for (int_t x = rhs->int_value(); x > 0; x--) {
        for (auto it = this->items.begin(); it != this->items.end(); ++it)
            ret->append(*it);
    }
    return ret;
}

node *tuple::type() {
    return &builtin_class_tuple;
}

node *dict::type() {
    return &builtin_class_dict;
}

node *set::type() {
    return &builtin_class_set;
}

node *string_const::type() {
    return &builtin_class_str;
}

// This entire function is very stupidly implemented.
node *string_const::__mod__(node *rhs) {
    std::ostringstream new_string;
    if (!rhs->is_tuple()) {
        node *tuple_item[1] = {rhs};
        tuple *t = new(allocator) tuple(1, tuple_item);
        rhs = t;
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
                *fmt++ = 'l';
                *fmt++ = 'l';
                *fmt++ = *c;
                *fmt = 0;
                sprintf(buf, fmt_buf, arg->int_value());
            }
            else if (*c == 'c') {
                *fmt++ = 'c';
                *fmt = 0;
                int_t char_value;
                if (arg->is_string())
                    char_value = (unsigned char)arg->c_str()[0];
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

node *file::getattr(const char *key) {
    if (!strcmp(key, "read"))
        return new(allocator) bound_method(this, &builtin_method_file_read);
    if (!strcmp(key, "write"))
        return new(allocator) bound_method(this, &builtin_method_file_write);
    error("file has no attribute %s", key);
}

node *bytes::type() {
    return &builtin_class_bytes;
}

node *enumerate::type() {
    return &builtin_class_enumerate;
}

node *range::type() {
    return &builtin_class_range;
}

node *zip::type() {
    return &builtin_class_zip;
}

node *builtin_class::type() {
    return &builtin_class_type;
}

////////////////////////////////////////////////////////////////////////////////
// Builtins ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class builtin_function_def: public function_def {
private:
    const char *name;

public:
    builtin_function_def(const char *name, fptr base_function): function_def(base_function) {
        this->name = name;
    }
    const char *node_type() { return "builtin_function"; }

    MARK_LIVE_SINGLETON_FN

    virtual std::string repr() {
        return std::string("<built-in function ") + this->name + ">";
    }
};

inline node *builtin_abs(node *arg) {
    return arg->__abs__();
}

inline node *builtin_all(node *arg) {
    node *iter = arg->__iter__();
    while (node *item = iter->next()) {
        if (!item->bool_value())
            return &bool_singleton_False;
    }
    return &bool_singleton_True;
}

inline node *builtin_any(node *arg) {
    node *iter = arg->__iter__();
    while (node *item = iter->next()) {
        if (item->bool_value())
            return &bool_singleton_True;
    }
    return &bool_singleton_False;
}

inline node *builtin_dict_get(node *self_arg, node *arg0, node *arg1) {
    if (!self_arg->is_dict())
        error("bad argument to dict.get()");
    dict *self = (dict *)self_arg;
    node *value = self->lookup(arg0);
    if (!value)
        value = arg1;
    return value;
}

inline node *builtin_dict_keys(node *self_arg) {
    if (!self_arg->is_dict())
        error("bad argument to dict.keys()");
    dict *self = (dict *)self_arg;
    return new(allocator) dict_keys(self);
}

inline node *builtin_dict_items(node *self_arg) {
    if (!self_arg->is_dict())
        error("bad argument to dict.items()");
    dict *self = (dict *)self_arg;
    return new(allocator) dict_items(self);
}

inline node *builtin_dict_values(node *self_arg) {
    if (!self_arg->is_dict())
        error("bad argument to dict.values()");
    dict *self = (dict *)self_arg;
    return new(allocator) dict_values(self);
}

inline node *builtin_file_read(node *self_arg, node *arg) {
    if (!self_arg->is_file() || !arg->is_int_const())
        error("bad arguments to file.read()");
    return ((file *)self_arg)->read(arg->int_value());
}

inline node *builtin_file_write(node *self_arg, node *arg) {
    if (!self_arg->is_file() || !arg->is_string())
        error("bad arguments to file.write()");
    ((file *)self_arg)->write((string_const *)arg);
    return &none_singleton;
}

inline node *builtin_isinstance(node *arg0, node *arg1) {
    node *obj_class = arg0->type();
    return create_bool_const(obj_class == arg1);
}

inline node *builtin_iter(node *arg) {
    return arg->__iter__();
}

inline node *builtin_len(node *arg) {
    return arg->__len__();
}

inline node *builtin_list_append(node *self_arg, node *arg) {
    if (!self_arg->is_list())
        error("bad argument to list.append()");
    list *self = (list *)self_arg;
    self->append(arg);
    return &none_singleton;
}

inline node *builtin_list_count(node *self_arg, node *arg) {
    if (!self_arg->is_list())
        error("bad argument to list.count()");
    list *self = (list *)self_arg;
    int_t n = 0;
    for (int_t i = 0; i < self->len(); i++) {
        if (self->__getitem__(i)->_eq(arg))
            n++;
    }
    return new(allocator) int_const(n);
}

inline node *builtin_list_extend(node *self_arg, node *arg) {
    if (!self_arg->is_list())
        error("bad argument to list.extend()");
    list *self = (list *)self_arg;
    node *iter = arg->__iter__();
    while (node *item = iter->next())
        self->append(item);
    return &none_singleton;
}

inline node *builtin_list_index(node *self_arg, node *arg) {
    if (!self_arg->is_list())
        error("bad argument to list.index()");
    list *self = (list *)self_arg;
    for (int_t i = 0; i < self->len(); i++) {
        if (self->__getitem__(i)->_eq(arg))
            return new(allocator) int_const(i);
    }
    error("item not found in list");
}

inline node *builtin_list_pop(node *self_arg) {
    if (!self_arg->is_list())
        error("bad argument to list.pop()");
    list *self = (list *)self_arg;
    return self->pop();
}

inline node *builtin_open(node *arg0, node *arg1) {
    if (!arg0->is_string() || !arg1->is_string())
        error("bad arguments to open()");
    return new(allocator) file(arg0->c_str(), arg1->c_str());
}

inline node *builtin_ord(node *arg) {
    if (!arg->is_string() || arg->len() != 1)
        error("bad arguments to ord()");
    return new(allocator) int_const((unsigned char)arg->c_str()[0]);
}

inline node *builtin_print(tuple *args) {
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

inline node *builtin_print_nonl(node *arg) {
    printf("%s", arg->str().c_str());
    return &none_singleton;
}

inline node *builtin_repr(node *arg) {
    return arg->__repr__();
}

inline node *builtin_set_add(node *self_arg, node *arg) {
    if (!self_arg->is_set())
        error("bad argument to set.add()");
    set *self = (set *)self_arg;
    self->add(arg);
    return &none_singleton;
}

inline node *builtin_set_update(node *self_arg, node *arg) {
    if (!self_arg->is_set())
        error("bad argument to set.update()");
    set *self = (set *)self_arg;
    node *iter = arg->__iter__();
    while (node *item = iter->next())
        self->add(item);
    return &none_singleton;
}

static bool compare_nodes(node *lhs, node *rhs) {
    return lhs->_lt(rhs);
}

inline node *builtin_sorted(node *arg) {
    node *iter = arg->__iter__();
    node_list new_list;
    while (node *item = iter->next())
        new_list.push_back(item);
    std::stable_sort(new_list.begin(), new_list.end(), compare_nodes);
    return new(allocator) list(new_list.size(), &new_list[0]);
}

inline node *builtin_str_join(node *self_arg, node *arg) {
    if (!self_arg->is_string())
        error("bad arguments to str.join()");
    string_const *self = (string_const *)self_arg;
    node *iter = arg->__iter__();
    std::string s;
    bool first = true;
    while (node *item = iter->next()) {
        if (first)
            first = false;
        else
            s += self->c_str();
        s += item->str();
    }
    return new(allocator) string_const(s);
}

inline node *builtin_str_split(node *self_arg, node *arg) {
    // XXX Implement correct behavior for missing separator (not the same as ' ')
    if (!arg || (arg == &none_singleton))
        arg = new(allocator) string_const(" ");
    if (!self_arg->is_string() || !arg->is_string() || (arg->len() != 1))
        error("bad argument to str.split()");
    string_const *self = (string_const *)self_arg;
    // XXX Implement correct behavior for this too--delimiter strings can have len>1
    char split = arg->c_str()[0];
    list *ret = new(allocator) list;
    std::string s;
    for (auto c = self->begin(); c != self->end(); ++c) {
        if (*c == split) {
            ret->append(new(allocator) string_const(s));
            s.clear();
        }
        else {
            s += *c;
        }
    }
    ret->append(new(allocator) string_const(s));
    return ret;
}

inline node *builtin_str_upper(node *self_arg) {
    if (!self_arg->is_string())
        error("bad argument to str.upper()");
    string_const *self = (string_const *)self_arg;
    std::string new_string;
    for (auto it = self->begin(); it != self->end(); ++it)
        new_string += toupper(*it);
    return new(allocator) string_const(new_string);
}

inline node *builtin_str_startswith(node *self_arg, node *arg) {
    if (!self_arg->is_string() || !arg->is_string())
        error("bad arguments to str.startswith()");
    std::string s1 = self_arg->string_value();
    std::string s2 = arg->string_value();
    return create_bool_const(s1.compare(0, s2.size(), s2) == 0);
}

inline node *builtin_tuple_count(node *self_arg, node *arg) {
    if (!self_arg->is_tuple())
        error("bad argument to tuple.count()");
    tuple *self = (tuple *)self_arg;
    int_t n = 0;
    for (int_t i = 0; i < self->len(); i++) {
        if (self->__getitem__(i)->_eq(arg))
            n++;
    }
    return new(allocator) int_const(n);
}

inline node *builtin_tuple_index(node *self_arg, node *arg) {
    if (!self_arg->is_tuple())
        error("bad argument to tuple.index()");
    tuple *self = (tuple *)self_arg;
    for (int_t i = 0; i < self->len(); i++) {
        if (self->__getitem__(i)->_eq(arg))
            return new(allocator) int_const(i);
    }
    error("item not found in tuple");
}

#define BUILTIN_FUNCTION(name) builtin_function_def builtin_function_##name(#name, wrapped_builtin_##name);
LIST_BUILTIN_FUNCTIONS(BUILTIN_FUNCTION)
#undef BUILTIN_FUNCTION

void init_context(context *ctx, int_t argc, char **argv) {
#define BUILTIN_FUNCTION(name) ctx->store(sym_id_##name, &builtin_function_##name);
LIST_BUILTIN_FUNCTIONS(BUILTIN_FUNCTION)
#undef BUILTIN_FUNCTION

#define BUILTIN_CLASS(name) ctx->store(sym_id_##name, &builtin_class_##name);
LIST_BUILTIN_CLASSES(BUILTIN_CLASS)
#undef BUILTIN_CLASS

    ctx->store(sym_id___name__, new(allocator) string_const("__main__"));
    list *plist = new(allocator) list;
    for (int_t a = 0; a < argc; a++)
        plist->append(new(allocator) string_const(argv[a]));
    ctx->store(sym_id___args__, plist);
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
