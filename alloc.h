////////////////////////////////////////////////////////////////////////////////
//
// Pythonc memory allocator
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

#include <stddef.h>

#define ARENA_BLOCK_SIZE (4096)
class arena_block {
public:
    unsigned char data[ARENA_BLOCK_SIZE];
    void *curr;
    arena_block *next;
    arena_block() {
        this->curr = this->data;
    }
    void *get_bytes(uint64_t bytes) {
        // Should only happen if allocation is bigger than block size
        if (bytes > this->bytes_left()) {
            printf("unable to allocate memory!");
            exit(1);
        }
        void *b = this->curr;
        this->curr += bytes;
        return b;
    }
    uint64_t bytes_left() {
        return ARENA_BLOCK_SIZE - ((unsigned char *)this->curr - this->data);
    }
};

class arena {
private:
    arena_block *head;

public:
    arena() {
        this->head = new arena_block();
    }

    void *allocate(int64_t bytes) {
        if (this->head->bytes_left() < bytes) {
            arena_block *nblock = new arena_block();
            nblock->next = this->head;
            this->head = nblock;
        }
        return this->head->get_bytes(bytes);
    }
};

template <class T>
class alloc;

template <>
class alloc<void> {
public:
    typedef void *pointer;
    typedef const void *const_pointer;
    typedef void value_type;
    template <class U> struct rebind {
        typedef alloc<U> other;
    };
};

template <class T>
class alloc {
private:
    static arena a;

public:
    typedef T value_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef ptrdiff_t difference_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef size_t size_type;
    template <class U>
    struct rebind {
        typedef alloc<U> other;
    };

    alloc() {}
    alloc(const alloc &x) {}
    template <class U>
    alloc(const alloc<U> &x) {}
    ~alloc() {}

    T *address(T &x) const { return &x; }
    const T* address(const T &x) const { return &x; }

    T *allocate(size_t bytes, alloc<void>::const_pointer hint=0) {
        return (T *)a.allocate(bytes * sizeof(T));
    }
    void deallocate(T *p, size_t n) {
        // XXX would be nice :)
    }
    size_t max_size() const {
        return size_t(-1) / sizeof(value_type);
    }

    void construct(T *p, const T &val) { new(static_cast<void *>(p)) T(val); }
    void destroy(T *p) { p->~T(); }
};

template <typename T> arena alloc<T>::a;

template <typename T, typename U>
bool operator==(const alloc<T>&, const alloc<U>) {
    return true;
}

template <typename T, typename U>
bool operator!=(const alloc<T>&, const alloc<U>) {
    return false;
}
