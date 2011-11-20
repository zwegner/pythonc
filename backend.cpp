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

class list;
class string_const;

typedef int64_t int_t;

typedef std::pair<node *, node *> node_pair;
typedef std::map<int_t, node_pair> node_dict;
typedef std::map<int_t, node *> node_set;
typedef std::vector<node *> node_list;

inline node *create_bool_const(bool b);

class node {
private:
public:
    node() { }
    const char *node_type() { return type()->type_name(); }

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
    virtual bool is_str() { return false; }
    virtual bool bool_value() { error("bool_value unimplemented for %s", this->node_type()); return false; }
    virtual int_t int_value() { error("int_value unimplemented for %s", this->node_type()); return 0; }
    virtual std::string str_value() { error("str_value unimplemented for %s", this->node_type()); return NULL; }
    virtual const char *c_str() { error("c_str unimplemented for %s", this->node_type()); }

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

#define UNIMP_UNOP(NAME) \
    virtual node *__##NAME##__() { error(#NAME " unimplemented for %s", this->node_type()); return NULL; }

    UNIMP_UNOP(invert)
    UNIMP_UNOP(pos)
    UNIMP_UNOP(neg)
    UNIMP_UNOP(abs)

    // By default, in-place ops map to regular ops
#define IN_PLACE_OP(name) \
    virtual node *__i##name##__(node *rhs) { return __##name##__(rhs); }

    IN_PLACE_OP(add)
    IN_PLACE_OP(and)
    IN_PLACE_OP(floordiv)
    IN_PLACE_OP(lshift)
    IN_PLACE_OP(mod)
    IN_PLACE_OP(mul)
    IN_PLACE_OP(or)
    IN_PLACE_OP(pow)
    IN_PLACE_OP(rshift)
    IN_PLACE_OP(sub)
    IN_PLACE_OP(truediv)
    IN_PLACE_OP(xor)

    node *__contains__(node *rhs);
    node *__len__();
    node *__hash__();
    node *__getattr__(node *rhs);
    node *__ncontains__(node *rhs);
    node *__not__();
    node *__is__(node *rhs);
    node *__isnot__(node *rhs);
    node *__repr__();
    node *__str__();

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
    virtual bool contains(node *rhs) { error("contains unimplemented for %s", this->node_type()); }
    virtual int_t len() { error("len unimplemented for %s", this->node_type()); return 0; }
    virtual node *getattr(const char *key);
    virtual int_t hash() { error("hash unimplemented for %s", this->node_type()); return 0; }
    virtual std::string repr();
    virtual std::string str() { return repr(); }
    virtual node *type() = 0;
    virtual const char *type_name() { error("type_name unimplemented for %s", this->node_type()); }
};

class builtin_class: public node {
public:
    virtual const char *type_name() = 0;

    MARK_LIVE_SINGLETON_FN

    virtual node *getattr(const char *key);

    virtual std::string repr() {
        return std::string("<class '") + this->type_name() + "'>";
    }
    virtual node *type();
};

#define BUILTIN_HIDDEN_CLASS(name) \
class name##_class: public builtin_class { \
public: \
    virtual const char *type_name() { return #name; } \
}; \
name##_class builtin_class_##name;
LIST_BUILTIN_HIDDEN_CLASSES(BUILTIN_HIDDEN_CLASS)
#undef BUILTIN_HIDDEN_CLASS

#define BUILTIN_CLASS(name) \
class name##_class: public builtin_class { \
public: \
    virtual const char *type_name() { return #name; } \
    virtual node *getattr(const char *key); \
    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs); \
}; \
name##_class builtin_class_##name;
LIST_BUILTIN_CLASSES(BUILTIN_CLASS)
#undef BUILTIN_CLASS

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

    MARK_LIVE_SINGLETON_FN

    virtual bool is_none() { return true; }
    virtual bool bool_value() { return false; }

    virtual bool _eq(node *rhs);
    virtual bool _ne(node *rhs) { return !_eq(rhs); }

    virtual int_t hash() { return 0; }
    virtual std::string repr() { return std::string("None"); }
    virtual node *type() { return &builtin_class_NoneType; }
};

class int_const : public node {
public:
    int_t value;

    explicit int_const(int_t v): value(v) {}

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
    virtual node *type() { return &builtin_class_int; }
};

class int_const_singleton : public int_const {
public:
    int_const_singleton(int_t value) : int_const(value) { }

    MARK_LIVE_SINGLETON_FN
};

class bool_const: public node {
public:
    bool value;

    explicit bool_const(bool v): value(v) {}

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
    virtual node *type() { return &builtin_class_bool; }
};

class string_const : public node {
public:
    std::string value;

    class str_iter: public node {
    private:
        string_const *parent;
        std::string::iterator it;

    public:
        str_iter(string_const *s) {
            this->parent = s;
            it = s->value.begin();
        }

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
        virtual node *type() { return &builtin_class_str_iterator; }
    };

    explicit string_const(const char *x): value(x) {}
    explicit string_const(std::string x): value(x) {}

    MARK_LIVE_FN

    virtual bool is_str() { return true; }
    virtual std::string str_value() { return this->value; }
    virtual bool bool_value() { return this->value.length() != 0; }
    virtual const char *c_str() { return this->value.c_str(); }

#define STRING_OP(NAME, OP) \
    virtual bool _##NAME(node *rhs) { \
        if (rhs->is_str()) \
            return this->str_value() OP rhs->str_value(); \
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
        for (auto it = this->value.begin(); it != this->value.end(); ++it) {
            hashkey ^= *it;
            hashkey *= 1099511628211ll;
        }
        return hashkey;
    }
    virtual int_t len() { return this->value.length(); }
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
        for (auto it = this->value.begin(); it != this->value.end(); ++it) {
            char c = *it;
            if (c == '\'')
                has_single_quotes = true;
            else if (c == '"')
                has_double_quotes = true;
        }
        bool use_double_quotes = has_single_quotes && !has_double_quotes;
        std::string s(use_double_quotes ? "\"" : "'");
        for (auto it = this->value.begin(); it != this->value.end(); ++it) {
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
    virtual node *type() { return &builtin_class_str; }
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
public:
    std::vector<uint8_t> value;

    class bytes_iter: public node {
    private:
        bytes *parent;
        std::vector<uint8_t>::iterator it;

    public:
        bytes_iter(bytes *b) {
            this->parent = b;
            it = b->value.begin();
        }

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
        virtual node *type() { return &builtin_class_bytes_iterator; }
    };

    bytes() {}
    explicit bytes(size_t len): value(len) {}
    bytes(size_t len, const uint8_t *data): value(len) {
        memcpy(&value[0], data, len);
    }

    MARK_LIVE_FN

    void append(uint8_t x) { this->value.push_back(x); }

    virtual bool bool_value() { return this->value.size() != 0; }

    virtual int_t len() { return this->value.size(); }
    virtual node *type() { return &builtin_class_bytes; }

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

class list: public node {
public:
    node_list items;

    class list_iter: public node {
    private:
        list *parent;
        node_list::iterator it;

    public:
        list_iter(list *l) {
            this->parent = l;
            it = l->items.begin();
        }

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
        virtual node *type() { return &builtin_class_list_iterator; }
    };

    list() {}
    explicit list(int_t n): items(n) {}

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (size_t i = 0; i < this->items.size(); i++)
                this->items[i]->mark_live();
        }
    }

    int_t index(int_t base) {
        int_t size = items.size();
        if ((base >= size) || (base < -size))
            error("list index out of range");
        if (base < 0)
            base += size;
        return base;
    }
    node *pop(int_t idx) {
        idx = this->index(idx);
        node *popped = this->items[idx];
        items.erase(this->items.begin() + idx);
        return popped;
    }

    virtual bool is_list() { return true; }
    virtual bool bool_value() { return this->items.size() != 0; }

    virtual node *__add__(node *rhs);
    virtual node *__mul__(node *rhs);
    virtual node *__iadd__(node *rhs);
    virtual node *__imul__(node *rhs);

    virtual bool _eq(node *rhs);
    virtual bool _ne(node *rhs) { return !_eq(rhs); }

    virtual bool contains(node *key) {
        for (size_t i = 0; i < this->items.size(); i++) {
            if (this->items[i]->_eq(key))
                return true;
        }
        return false;
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
    virtual int_t len() { return this->items.size(); }
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
            new_list->items.push_back(items[lo]);
        return new_list;
    }
    virtual std::string repr() {
        std::string new_string = "[";
        bool first = true;
        for (auto it = this->items.begin(); it != this->items.end(); ++it) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += (*it)->repr();
        }
        new_string += "]";
        return new_string;
    }
    virtual node *type() { return &builtin_class_list; }
    virtual node *__iter__() { return new(allocator) list_iter(this); }
};

class tuple: public node {
public:
    node_list items;

    class tuple_iter: public node {
    private:
        tuple *parent;
        node_list::iterator it;

    public:
        tuple_iter(tuple *t) {
            this->parent = t;
            it = t->items.begin();
        }

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
        virtual node *type() { return &builtin_class_tuple_iterator; }
    };

    tuple() {}
    explicit tuple(int_t n): items(n) {}

    virtual bool is_tuple() { return true; }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (size_t i = 0; i < this->items.size(); i++)
                this->items[i]->mark_live();
        }
    }

    int_t index(int_t base) {
        int_t size = items.size();
        if ((base >= size) || (base < -size))
            error("tuple index out of range");
        if (base < 0)
            base += size;
        return base;
    }

    virtual bool bool_value() { return this->items.size() != 0; }

    virtual node *__add__(node *rhs);
    virtual node *__mul__(node *rhs);

    virtual bool _eq(node *rhs);
    virtual bool _ne(node *rhs) { return !_eq(rhs); }

    virtual bool contains(node *key) {
        for (size_t i = 0; i < this->items.size(); i++) {
            if (this->items[i]->_eq(key))
                return true;
        }
        return false;
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
    virtual int_t len() { return this->items.size(); }
    virtual node *type() { return &builtin_class_tuple; }
    virtual std::string repr() {
        std::string new_string = "(";
        bool first = true;
        for (auto it = this->items.begin(); it != this->items.end(); ++it) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += (*it)->repr();
        }
        if (this->items.size() == 1)
            new_string += ",";
        new_string += ")";
        return new_string;
    }
    virtual node *__iter__() { return new(allocator) tuple_iter(this); }
};

class dict: public node {
public:
    node_dict items;

    class dict_keys_iter: public node {
    private:
        dict *parent;
        node_dict::iterator it;

    public:
        dict_keys_iter(dict *d) {
            this->parent = d;
            it = d->items.begin();
        }

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
        virtual node *type() { return &builtin_class_dict_keyiterator; }
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

        virtual void mark_live() {
            if (!allocator->mark_live<sizeof(*this)>(this))
                this->parent->mark_live();
        }

        virtual node *__iter__() { return this; }
        virtual node *next() {
            if (this->it == this->parent->items.end())
                return NULL;
            tuple *ret = new(allocator) tuple(2);
            ret->items[0] = this->it->second.first;
            ret->items[1] = this->it->second.second;
            ++this->it;
            return ret;
        }
        virtual node *type() { return &builtin_class_dict_itemiterator; }
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
        virtual node *type() { return &builtin_class_dict_valueiterator; }
    };

    dict() {}

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (auto it = this->items.begin(); it != this->items.end(); ++it) {
                it->second.first->mark_live();
                it->second.second->mark_live();
            }
        }
    }

    node *lookup(node *key) {
        int_t hashkey;
        if (key->is_int_const())
            hashkey = key->int_value();
        else
            hashkey = key->hash();
        auto it = this->items.find(hashkey);
        if (it == this->items.end())
            return NULL;
        node *k = it->second.first;
        if (!k->_eq(key))
            return NULL;
        return it->second.second;
    }
    dict *copy() {
        dict *ret = new(allocator) dict;
        for (auto it = this->items.begin(); it != this->items.end(); ++it)
            ret->items[it->first] = it->second;
        return ret;
    }

    virtual bool is_dict() { return true; }
    virtual bool bool_value() { return this->items.size() != 0; }

    virtual bool contains(node *key) {
        return this->lookup(key) != NULL;
    }
    virtual node *__getitem__(node *key) {
        node *value = this->lookup(key);
        if (value == NULL)
            error("cannot find %s in dict", key->repr().c_str());
        return value;
    }
    virtual int_t len() { return this->items.size(); }
    virtual void __setitem__(node *key, node *value) {
        items[key->hash()] = node_pair(key, value);
    }
    virtual std::string repr() {
        std::string new_string = "{";
        bool first = true;
        for (auto it = this->items.begin(); it != this->items.end(); ++it) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += it->second.first->repr() + ": " + it->second.second->repr();
        }
        new_string += "}";
        return new_string;
    }
    virtual node *type() { return &builtin_class_dict; }
    virtual node *__iter__() { return new(allocator) dict_keys_iter(this); }
};

class dict_keys: public node {
private:
    dict *parent;

public:
    dict_keys(dict *d) {
        this->parent = d;
    }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->parent->mark_live();
    }

    virtual bool bool_value() { return this->parent->items.size() != 0; }
    virtual int_t len() { return this->parent->items.size(); }
    virtual node *__iter__() { return new(allocator) dict::dict_keys_iter(this->parent); }
    virtual std::string repr() {
        std::string new_string = "dict_keys([";
        bool first = true;
        for (auto it = this->parent->items.begin(); it != this->parent->items.end(); ++it) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += it->second.first->repr();
        }
        new_string += "])";
        return new_string;
    }
    virtual node *type() { return &builtin_class_dict_keys; }
};

class dict_items: public node {
private:
    dict *parent;

public:
    dict_items(dict *d) {
        this->parent = d;
    }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->parent->mark_live();
    }

    virtual bool bool_value() { return this->parent->items.size() != 0; }
    virtual int_t len() { return this->parent->items.size(); }
    virtual node *__iter__() { return new(allocator) dict::dict_items_iter(this->parent); }
    virtual std::string repr() {
        std::string new_string = "dict_items([";
        bool first = true;
        for (auto it = this->parent->items.begin(); it != this->parent->items.end(); ++it) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += "(";
            new_string += it->second.first->repr();
            new_string += ", ";
            new_string += it->second.second->repr();
            new_string += ")";
        }
        new_string += "])";
        return new_string;
    }
    virtual node *type() { return &builtin_class_dict_items; }
};

class dict_values: public node {
private:
    dict *parent;

public:
    dict_values(dict *d) {
        this->parent = d;
    }

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->parent->mark_live();
    }

    virtual bool bool_value() { return this->parent->items.size() != 0; }
    virtual int_t len() { return this->parent->items.size(); }
    virtual node *__iter__() { return new(allocator) dict::dict_values_iter(this->parent); }
    virtual std::string repr() {
        std::string new_string = "dict_values([";
        bool first = true;
        for (auto it = this->parent->items.begin(); it != this->parent->items.end(); ++it) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += it->second.second->repr();
        }
        new_string += "])";
        return new_string;
    }
    virtual node *type() { return &builtin_class_dict_values; }
};

class set: public node {
public:
    node_set items;

    class set_iter: public node {
    private:
        set *parent;
        node_set::iterator it;

    public:
        set_iter(set *s) {
            this->parent = s;
            it = s->items.begin();
        }

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
        virtual node *type() { return &builtin_class_set_iterator; }
    };

    set() {}

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            for (auto it = this->items.begin(); it != this->items.end(); ++it)
                it->second->mark_live();
        }
    }

    node *lookup(node *key) {
        auto it = this->items.find(key->hash());
        if ((it == this->items.end()) || !it->second->_eq(key))
            return NULL;
        return it->second;
    }
    void add(node *key) {
        items[key->hash()] = key;
    }
    void discard(node *key) {
        auto it = this->items.find(key->hash());
        if ((it == this->items.end()) || !it->second->_eq(key))
            return;
        this->items.erase(it);
    }
    void remove(node *key) {
        auto it = this->items.find(key->hash());
        if ((it == this->items.end()) || !it->second->_eq(key))
            error("element not in set");
        this->items.erase(it);
    }
    set *copy() {
        set *ret = new(allocator) set;
        for (auto it = this->items.begin(); it != this->items.end(); ++it)
            ret->items[it->first] = it->second;
        return ret;
    }

    virtual bool is_set() { return true; }
    virtual bool bool_value() { return this->items.size() != 0; }

    virtual node *__or__(node *rhs);
    virtual node *__ior__(node *rhs);
    virtual node *__sub__(node *rhs);
    virtual node *__isub__(node *rhs);

    virtual bool _eq(node *rhs);
    virtual bool _ne(node *rhs) { return !_eq(rhs); }

    virtual bool contains(node *key) {
        return this->lookup(key) != NULL;
    }
    virtual int_t len() { return this->items.size(); }
    virtual std::string repr() {
        if (!this->items.size())
            return "set()";
        std::string new_string = "{";
        bool first = true;
        for (auto it = this->items.begin(); it != this->items.end(); ++it) {
            if (!first)
                new_string += ", ";
            first = false;
            new_string += it->second->repr();
        }
        new_string += "}";
        return new_string;
    }
    virtual node *type() { return &builtin_class_set; }
    virtual node *__iter__() { return new(allocator) set_iter(this); }
};

class object: public node {
public:
    dict *items;

    object() { this->items = new(allocator) dict; }

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
    virtual bool _eq(node *rhs) { return this == rhs; }
    virtual bool _ne(node *rhs) { return this != rhs; }
};

class file: public node {
public:
    FILE *f;

    file(const char *path, const char *mode) {
        f = fopen(path, mode);
        if (!f)
            error("%s: file not found", path);
    }

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
    virtual node *type() { return &builtin_class_file; }
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

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this))
            this->iter->mark_live();
    }

    virtual node *__iter__() { return this; }
    virtual node *next() {
        node *item = this->iter->next();
        if (!item)
            return NULL;
        tuple *ret = new(allocator) tuple(2);
        ret->items[0] = new(allocator) int_const(this->i++);
        ret->items[1] = item;
        return ret;
    }

    virtual node *type() { return &builtin_class_enumerate; }
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
        virtual node *type() { return &builtin_class_range_iterator; }
    };

    int_t start, end, step;

public:
    range(int_t start, int_t end, int_t step) {
        this->start = start;
        this->end = end;
        this->step = step;
    }
 
    MARK_LIVE_FN

    virtual node *__iter__() { return new(allocator) range_iter(this); }

    virtual node *type() { return &builtin_class_range; }
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

    virtual node *type() { return &builtin_class_reversed; }
};

class zip: public node {
private:
    node *iter1;
    node *iter2;

public:
    zip(node *i1, node *i2): iter1(i1), iter2(i2) {}

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
        tuple *ret = new(allocator) tuple(2);
        ret->items[0] = item1;
        ret->items[1] = item2;
        return ret;
    }

    virtual node *type() { return &builtin_class_zip; }
};

typedef node *(*fptr)(context *globals, context *parent_ctx, tuple *args, dict *kwargs);

class bound_method : public node {
private:
    node *self;
    node *function;

public:
    bound_method(node *s, node *f): self(s), function(f) {}

    virtual void mark_live() {
        if (!allocator->mark_live<sizeof(*this)>(this)) {
            this->self->mark_live();
            this->function->mark_live();
        }
    }

    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs) {
        int_t len = args->items.size();
        tuple *new_args = new(allocator) tuple(len + 1);
        new_args->items[0] = this->self;
        for (int_t i = 0; i < len; i++)
            new_args->items[i+1] = args->items[i];
        return this->function->__call__(globals, ctx, new_args, kwargs);
    }
    virtual node *type() { return &builtin_class_bound_method; }
};

class function_def : public node {
private:
    fptr base_function;

public:
    explicit function_def(fptr f): base_function(f) {}

    MARK_LIVE_FN

    virtual bool is_function() { return true; }

    virtual node *__call__(context *globals, context *ctx, tuple *args, dict *kwargs) {
        return this->base_function(globals, ctx, args, kwargs);
    }
    virtual node *type() { return &builtin_class_function; }
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
        for (auto it = items->items.begin(); it != items->items.end(); ++it) {
            if (it->second.second->is_function())
                obj->__setattr__(it->second.first, new(allocator) bound_method(obj, it->second.second));
        }

        int_t len = args->items.size();
        tuple *new_args = new(allocator) tuple(len + 1);
        new_args->items[0] = obj;
        for (int_t i = 0; i < len; i++)
            new_args->items[i+1] = args->items[i];
        init->__call__(globals, ctx, new_args, kwargs);
        return obj;
    }
    virtual node *getattr(const char *attr) {
        return this->load(attr);
    }
    virtual std::string repr() {
        return std::string("<class '") + this->name + "'>";
    }
    virtual node *type() { return &builtin_class_type; }
};

bool_const bool_singleton_True(true);
bool_const bool_singleton_False(false);
none_const none_singleton(0);

inline node *create_bool_const(bool b) {
    return b ? &bool_singleton_True : &bool_singleton_False;
}

// Builtin classes
class builtin_method_def: public function_def {
public:
    builtin_method_def(fptr base_function): function_def(base_function) {}

    MARK_LIVE_SINGLETON_FN

    virtual node *type() { return &builtin_class_builtin_function_or_method; } // XXX reports as "method_descriptor"
};

#define BUILTIN_METHOD(class_name, method_name) builtin_method_def builtin_method_##class_name##_##method_name(wrapped_builtin_##class_name##_##method_name);
LIST_BUILTIN_CLASS_METHODS(BUILTIN_METHOD)
#undef BUILTIN_METHOD

inline node *bool_init(node *arg) {
    if (!arg)
        return &bool_singleton_False;
    return create_bool_const(arg->bool_value());
}

inline node *bytes_init(node *arg) {
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

inline node *dict_init(node *arg) {
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

inline node *enumerate_init(node *arg) {
    node *iter = arg->__iter__();
    return new(allocator) enumerate(iter);
}

inline node *int_init(node *arg0, node *arg1) {
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
    if (arg0->is_str()) {
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

inline node *list_init(node *arg) {
    list *ret = new(allocator) list;
    if (!arg)
        return ret;
    node *iter = arg->__iter__();
    while (node *item = iter->next())
        ret->items.push_back(item);
    return ret;
}

inline node *range_init(node *arg0, node *arg1, node *arg2) {
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

// XXX This will actually work on dictionaries if they have keys of 0..len-1.
// Logically speaking it doesn't make sense to have reversed() of a dictionary
// do anything, but the Python docs imply that __len__ and __getitem__ are
// sufficient.  This seems like a documentation error.
inline node *reversed_init(node *arg) {
    int_t len = arg->len();
    return new(allocator) reversed(arg, len);
}

inline node *set_init(node *arg) {
    set *ret = new(allocator) set;
    if (!arg)
        return ret;
    node *iter = arg->__iter__();
    while (node *item = iter->next())
        ret->add(item);
    return ret;
}

inline node *str_init(node *arg) {
    if (!arg)
        return new(allocator) string_const("");
    return arg->__str__();
}

inline node *tuple_init(node *arg) {
    tuple *ret = new(allocator) tuple;
    if (!arg)
        return ret;
    node *iter = arg->__iter__();
    while (node *item = iter->next())
        ret->items.push_back(item);
    return ret;
}

inline node *type_init(node *arg) {
    return arg->type();
}

inline node *zip_init(node *arg0, node *arg1) {
    node *iter1 = arg0->__iter__();
    node *iter2 = arg1->__iter__();
    return new(allocator) zip(iter1, iter2);
}

#define GET_METHOD(class_name, method_name) \
    if (!strcmp(key, #method_name)) return &builtin_method_##class_name##_##method_name;
#define DEFINE_GETATTR(class_name) \
    node *class_name##_class::getattr(const char *key) {  \
        LIST_##class_name##_CLASS_METHODS(GET_METHOD) \
        return builtin_class::getattr(key); \
    }
LIST_BUILTIN_CLASSES(DEFINE_GETATTR)
#undef GET_METHOD
#undef DEFINE_GETATTR


node *node::__contains__(node *rhs) {
    return create_bool_const(this->contains(rhs));
}

node *node::__getattr__(node *key) {
    if (!key->is_str())
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

node *node::__ncontains__(node *rhs) {
    return create_bool_const(!this->contains(rhs));
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

std::string node::repr() {
    char buf[256];
    sprintf(buf, "<%s object at %p>", this->type()->type_name(), this);
    return buf;
}

// Don't call node::getattr as a fallback -- this will result in infinite recursion
node *builtin_class::getattr(const char *key) {
    if (!strcmp(key, "__name__"))
        return new(allocator) string_const(this->type_name());
    if (!strcmp(key, "__class__"))
        return type();
    error("%s has no attribute %s", type_name(), key);
}

bool none_const::_eq(node *rhs) {
    return (this == rhs);
}

std::string int_const::repr() {
    char buf[32];
    sprintf(buf, "%" PRId64, this->value);
    return std::string(buf);
}

std::string bool_const::repr() {
    return std::string(this->value ? "True" : "False");
}

node *list::__add__(node *rhs_arg) {
    if (!rhs_arg->is_list())
        error("list add error");
    list *rhs = (list *)rhs_arg;
    int_t self_len = this->items.size();
    int_t rhs_len = rhs->items.size();
    list *ret = new(allocator) list(self_len + rhs_len);
    for (int_t i = 0; i < self_len; i++)
        ret->items[i] = this->items[i];
    for (int_t i = 0; i < rhs_len; i++)
        ret->items[i + self_len] = rhs->items[i];
    return ret;
}

node *list::__mul__(node *rhs_arg) {
    if (!rhs_arg->is_int_const())
        error("list mul error");
    int_t rhs = ((int_const *)rhs_arg)->value;
    if (rhs <= 0)
        return new(allocator) list;
    int_t self_len = this->items.size();
    list *ret = new(allocator) list(self_len * rhs);
    node **items = &ret->items[0];
    do {
        for (int_t i = 0; i < self_len; i++)
            *items++ = this->items[i];
    } while (--rhs);
    return ret;
}

node *list::__iadd__(node *rhs_arg) {
    if (!rhs_arg->is_list())
        error("list add error");
    list *rhs = (list *)rhs_arg;
    for (auto it = rhs->items.begin(); it != rhs->items.end(); ++it)
        this->items.push_back(*it);
    return this;
}

node *list::__imul__(node *rhs) {
    if (!rhs->is_int_const())
        error("list mul error");
    int_t len = this->items.size();
    for (int_t x = rhs->int_value() - 1; x > 0; x--) {
        for (int_t i = 0; i < len; i++)
            this->items.push_back(this->items[i]);
    }
    return this;
}

bool list::_eq(node *rhs_arg) {
    if (!rhs_arg->is_list())
        return false;
    list *rhs = (list *)rhs_arg;
    int_t len = this->items.size();
    int_t rhs_len = rhs->items.size();
    if (len != rhs_len)
        return false;
    for (int_t i = 0; i < len; i++) {
        if (!this->items[i]->_eq(rhs->items[i]))
            return false;
    }
    return true;
}

node *tuple::__add__(node *rhs_arg) {
    if (!rhs_arg->is_tuple())
        error("tuple add error");
    tuple *rhs = (tuple *)rhs_arg;
    int_t self_len = this->items.size();
    int_t rhs_len = rhs->items.size();
    tuple *ret = new(allocator) tuple(self_len + rhs_len);
    for (int_t i = 0; i < self_len; i++)
        ret->items[i] = this->items[i];
    for (int_t i = 0; i < rhs_len; i++)
        ret->items[i + self_len] = rhs->items[i];
    return ret;
}

node *tuple::__mul__(node *rhs_arg) {
    if (!rhs_arg->is_int_const())
        error("tuple mul error");
    int_t rhs = ((int_const *)rhs_arg)->value;
    if (rhs <= 0)
        return new(allocator) tuple;
    int_t self_len = this->items.size();
    tuple *ret = new(allocator) tuple(self_len * rhs);
    node **items = &ret->items[0];
    do {
        for (int_t i = 0; i < self_len; i++)
            *items++ = this->items[i];
    } while (--rhs);
    return ret;
}

bool tuple::_eq(node *rhs_arg) {
    if (!rhs_arg->is_tuple())
        return false;
    tuple *rhs = (tuple *)rhs_arg;
    int_t len = this->items.size();
    int_t rhs_len = rhs->items.size();
    if (len != rhs_len)
        return false;
    for (int_t i = 0; i < len; i++) {
        if (!this->items[i]->_eq(rhs->items[i]))
            return false;
    }
    return true;
}

node *set::__or__(node *rhs_arg) {
    if (!rhs_arg->is_set())
        error("set or error");
    set *rhs = (set *)rhs_arg;
    set *ret = this->copy();
    for (auto it = rhs->items.begin(); it != rhs->items.end(); ++it)
        ret->add(it->second);
    return ret;
}

node *set::__ior__(node *rhs_arg) {
    if (!rhs_arg->is_set())
        error("set or error");
    set *rhs = (set *)rhs_arg;
    for (auto it = rhs->items.begin(); it != rhs->items.end(); ++it)
        this->add(it->second);
    return this;
}

node *set::__sub__(node *rhs_arg) {
    if (!rhs_arg->is_set())
        error("set sub error");
    set *rhs = (set *)rhs_arg;
    set *ret = this->copy();
    for (auto it = rhs->items.begin(); it != rhs->items.end(); ++it)
        ret->discard(it->second);
    return ret;
}

node *set::__isub__(node *rhs_arg) {
    if (!rhs_arg->is_set())
        error("set sub error");
    set *rhs = (set *)rhs_arg;
    for (auto it = rhs->items.begin(); it != rhs->items.end(); ++it)
        this->discard(it->second);
    return this;
}

bool set::_eq(node *rhs_arg) {
    if (!rhs_arg->is_set())
        return false;
    set *rhs = (set *)rhs_arg;
    int_t len = this->items.size();
    int_t rhs_len = rhs->items.size();
    if (len != rhs_len)
        return false;
    for (auto it = this->items.begin(), it2 = rhs->items.begin(); it != this->items.end(); ++it, ++it2) {
        if (it->first != it2->first)
            return false;
        if (!it->second->_eq(it2->second))
            return false;
    }
    return true;
}

// This entire function is very stupidly implemented.
node *string_const::__mod__(node *rhs_arg) {
    std::ostringstream new_string;
    int_t rhs_len;
    node **rhs_items;
    if (rhs_arg->is_tuple()) {
        tuple *rhs = (tuple *)rhs_arg;
        rhs_len = rhs->items.size();
        rhs_items = &rhs->items[0];
    }
    else {
        rhs_len = 1;
        rhs_items = &rhs_arg;
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
            if (args >= rhs_len)
                error("not enough arguments for string format");
            node *arg = rhs_items[args++];
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
                if (arg->is_str())
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
    if (!rhs->is_str())
        error("bad argument to str.add");
    std::string new_string = this->value + rhs->str_value();
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

    MARK_LIVE_SINGLETON_FN

    virtual std::string repr() {
        return std::string("<built-in function ") + this->name + ">";
    }
    virtual node *type() { return &builtin_class_builtin_function_or_method; }
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

inline node *builtin_dict_clear(dict *self) {
    self->items.clear();
    return &none_singleton;
}

inline node *builtin_dict_copy(dict *self) {
    return self->copy();
}

inline node *builtin_dict_get(dict *self, node *arg0, node *arg1) {
    node *value = self->lookup(arg0);
    if (!value)
        value = arg1;
    return value;
}

inline node *builtin_dict_keys(dict *self) {
    return new(allocator) dict_keys(self);
}

inline node *builtin_dict_items(dict *self) {
    return new(allocator) dict_items(self);
}

inline node *builtin_dict_values(dict *self) {
    return new(allocator) dict_values(self);
}

inline node *builtin_file_read(file *self, node *arg) {
    if (!arg->is_int_const())
        error("bad arguments to file.read()");
    return self->read(arg->int_value());
}

inline node *builtin_file_write(file *self, node *arg) {
    if (!arg->is_str())
        error("bad arguments to file.write()");
    self->write((string_const *)arg);
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

inline node *builtin_list_append(list *self, node *arg) {
    self->items.push_back(arg);
    return &none_singleton;
}

inline node *builtin_list_count(list *self, node *arg) {
    int_t n = 0;
    int_t len = self->items.size();
    for (int_t i = 0; i < len; i++) {
        if (self->items[i]->_eq(arg))
            n++;
    }
    return new(allocator) int_const(n);
}

inline node *builtin_list_extend(list *self, node *arg) {
    node *iter = arg->__iter__();
    while (node *item = iter->next())
        self->items.push_back(item);
    return &none_singleton;
}

inline node *builtin_list_index(list *self, node *arg) {
    int_t len = self->items.size();
    for (int_t i = 0; i < len; i++) {
        if (self->items[i]->_eq(arg))
            return new(allocator) int_const(i);
    }
    error("item not found in list");
}

inline node *builtin_list_insert(list *self, node *arg0_arg, node *arg1) {
    if (!arg0_arg->is_int_const())
        error("bad argument to list.insert()");
    int_t arg0 = arg0_arg->int_value();
    int_t len = self->items.size();
    if ((arg0 < 0) || (arg0 > len))
        error("bad argument to list.insert()");
    self->items.insert(self->items.begin() + arg0, arg1);
    return &none_singleton;
}

inline node *builtin_list_pop(node *self_arg, node *arg) {
    if (!self_arg->is_list())
        error("bad argument to list.pop()");
    list *self = (list *)self_arg;
    if (arg && !arg->is_int_const())
        error("bad argument to list.pop()");
    int_t idx = arg ? arg->int_value() : -1;
    return self->pop(idx);
}

inline node *builtin_list_remove(list *self, node *arg) {
    int_t len = self->items.size();
    for (int_t i = 0; i < len; i++) {
        if (self->items[i]->_eq(arg)) {
            self->items.erase(self->items.begin() + i);
            return &none_singleton;
        }
    }
    error("item not found in list");
}

inline node *builtin_list_reverse(list *self) {
    int_t len = self->items.size();
    for (int_t i = 0; i < len / 2; i++) {
        node *temp1 = self->items[i];
        node *temp2 = self->items[len - 1 - i];
        self->items[i] = temp2;
        self->items[len - 1 - i] = temp1;
    }
    return &none_singleton;
}

static bool compare_nodes(node *lhs, node *rhs) {
    return lhs->_lt(rhs);
}

inline node *builtin_list_sort(list *self) {
    std::stable_sort(self->items.begin(), self->items.end(), compare_nodes);
    return &none_singleton;
}

inline node *builtin_max(node *arg) {
    node *iter = arg->__iter__();
    node *ret = iter->next();
    if (!ret)
        error("max() expects non-empty iterable");
    while (node *item = iter->next()) {
        if (item->_gt(ret))
            ret = item;
    }
    return ret;
}

inline node *builtin_min(node *arg) {
    node *iter = arg->__iter__();
    node *ret = iter->next();
    if (!ret)
        error("min() expects non-empty iterable");
    while (node *item = iter->next()) {
        if (item->_lt(ret))
            ret = item;
    }
    return ret;
}

inline node *builtin_open(node *arg0, node *arg1) {
    if (!arg0->is_str() || !arg1->is_str())
        error("bad arguments to open()");
    return new(allocator) file(arg0->c_str(), arg1->c_str());
}

inline node *builtin_ord(node *arg) {
    if (!arg->is_str() || arg->len() != 1)
        error("bad arguments to ord()");
    return new(allocator) int_const((unsigned char)arg->c_str()[0]);
}

inline node *builtin_print(tuple *args) {
    std::string new_string;
    int_t args_len = args->items.size();
    for (int_t i = 0; i < args_len; i++) {
        if (i)
            new_string += " ";
        node *s = args->items[i];
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

inline node *builtin_set_add(set *self, node *arg) {
    self->add(arg);
    return &none_singleton;
}

inline node *builtin_set_clear(set *self) {
    self->items.clear();
    return &none_singleton;
}

inline node *builtin_set_copy(set *self) {
    return self->copy();
}

inline node *builtin_set_difference_update(tuple *args) {
    int_t args_len = args->items.size();
    if (args_len < 1)
        error("bad argument to set.difference_update()");
    node *self_arg = args->items[0];
    if (!self_arg->is_set())
        error("bad argument to set.difference_update()");
    set *self = (set *)self_arg;
    for (int_t i = 1; i < args_len; i++) {
        node *iter = args->items[i]->__iter__();
        while (node *item = iter->next())
            self->discard(item);
    }
    return &none_singleton;
}

inline node *builtin_set_discard(set *self, node *arg) {
    self->discard(arg);
    return &none_singleton;
}

inline node *builtin_set_remove(set *self, node *arg) {
    self->remove(arg);
    return &none_singleton;
}

inline node *builtin_set_update(tuple *args) {
    int_t args_len = args->items.size();
    if (args_len < 1)
        error("bad argument to set.update()");
    node *self_arg = args->items[0];
    if (!self_arg->is_set())
        error("bad argument to set.update()");
    set *self = (set *)self_arg;
    for (int_t i = 1; i < args_len; i++) {
        node *iter = args->items[i]->__iter__();
        while (node *item = iter->next())
            self->add(item);
    }
    return &none_singleton;
}

inline node *builtin_sorted(node *arg) {
    node *iter = arg->__iter__();
    node_list new_list;
    while (node *item = iter->next())
        new_list.push_back(item);
    std::stable_sort(new_list.begin(), new_list.end(), compare_nodes);
    int_t size = new_list.size();
    list *ret = new(allocator) list(size);
    memcpy(&ret->items[0], &new_list[0], size*sizeof(node *));
    return ret;
}

inline node *builtin_str_join(string_const *self, node *arg) {
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
    if (!self_arg->is_str() || !arg->is_str() || (arg->len() != 1))
        error("bad argument to str.split()");
    string_const *self = (string_const *)self_arg;
    // XXX Implement correct behavior for this too--delimiter strings can have len>1
    char split = arg->c_str()[0];
    list *ret = new(allocator) list;
    std::string s;
    for (auto it = self->value.begin(); it != self->value.end(); ++it) {
        char c = *it;
        if (c == split) {
            ret->items.push_back(new(allocator) string_const(s));
            s.clear();
        }
        else {
            s += c;
        }
    }
    ret->items.push_back(new(allocator) string_const(s));
    return ret;
}

inline node *builtin_str_startswith(string_const *self, node *arg) {
    if (!arg->is_str())
        error("bad arguments to str.startswith()");
    std::string s1 = self->str_value();
    std::string s2 = arg->str_value();
    return create_bool_const(s1.compare(0, s2.size(), s2) == 0);
}

inline node *builtin_str_upper(string_const *self) {
    std::string new_string;
    for (auto it = self->value.begin(); it != self->value.end(); ++it)
        new_string += toupper(*it);
    return new(allocator) string_const(new_string);
}

inline node *builtin_tuple_count(tuple *self, node *arg) {
    int_t n = 0;
    int_t len = self->items.size();
    for (int_t i = 0; i < len; i++) {
        if (self->items[i]->_eq(arg))
            n++;
    }
    return new(allocator) int_const(n);
}

inline node *builtin_tuple_index(tuple *self, node *arg) {
    int_t len = self->items.size();
    for (int_t i = 0; i < len; i++) {
        if (self->items[i]->_eq(arg))
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
        plist->items.push_back(new(allocator) string_const(argv[a]));
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
