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
#include <assert.h>

typedef unsigned char byte;

template <uint64_t block_size>
class arena_block {
public:
    static const int capacity = block_size - 2 * sizeof(void *) - sizeof(int);
    arena_block *next;
    byte *curr;
    int ref_count;
    byte data[capacity];

    arena_block() {
        // Ensure power-of-two block size, aligned address
        assert((block_size & block_size - 1) == 0);
        assert(((uint64_t)this & block_size - 1) == 0);
        this->init();
    }
    void init() {
        this->curr = this->data;
        this->ref_count = 0;
    }
    void *get_bytes(uint64_t bytes) {
        // Should only happen if allocation is bigger than block size
        if (bytes > this->bytes_left()) {
            printf("unable to allocate %" PRIu64 " bytes of memory!", bytes);
            exit(1);
        }
        byte *b = this->curr;
        this->curr += bytes;
        this->ref_count++;
        return (void *)b;
    }
    uint64_t bytes_left() {
        return capacity - (this->curr - this->data);
    }
};

#define BLOCK_SIZE (1 << 12)
#define CHUNK_SIZE (1 << 20)

class arena {
private:
    arena_block<BLOCK_SIZE> *head, *free_list;
    byte *chunk_start, *chunk_end;

public:
    arena() {
        assert(sizeof(arena_block<BLOCK_SIZE>) == BLOCK_SIZE);
        this->get_new_chunk();
        this->head = this->new_block();
        this->free_list = NULL;
    }

    void get_new_chunk() {
        this->chunk_start = (byte *)malloc(CHUNK_SIZE);
        this->chunk_end = chunk_start + CHUNK_SIZE; 
        // Align the start of the chunk
        this->chunk_start = (byte *)((uint64_t)this->chunk_start + BLOCK_SIZE - 1 & ~(BLOCK_SIZE - 1));
    }

    arena_block<BLOCK_SIZE> *new_block() {
        if (this->chunk_end - this->chunk_start < BLOCK_SIZE)
            this->get_new_chunk();

        arena_block<BLOCK_SIZE> *block = (arena_block<BLOCK_SIZE> *)this->chunk_start;
        this->chunk_start += BLOCK_SIZE;

        return new(block) arena_block<BLOCK_SIZE>();
    }

    void *allocate(uint64_t bytes) {
        if (bytes > this->head->capacity)
            return malloc(bytes);
        if (this->head->bytes_left() < bytes) {
            arena_block<BLOCK_SIZE> *block;
            if (this->free_list) {
                block = this->free_list;
                this->free_list = block->next;
                block->init();
            }
            else
                block = this->new_block();
            block->next = this->head;
            this->head = block;
        }
        return this->head->get_bytes(bytes);
    }
    void deallocate(void *p, uint64_t bytes) {
        if (bytes > this->head->capacity) {
            free(p);
            return;
        }
        arena_block<BLOCK_SIZE> *block = (arena_block<BLOCK_SIZE> *)
            ((uint64_t)p & ~(BLOCK_SIZE - 1));
        block->ref_count--;
        if (block->ref_count <= 0) {
            block->next = this->free_list;
            this->free_list = block;
        }
    }
};

arena allocator[1];

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

    T *allocate(size_t n, alloc<void>::const_pointer hint=0) {
        return (T *)allocator->allocate(n * sizeof(T));
    }
    void deallocate(T *p, size_t n) {
        allocator->deallocate(p, n * sizeof(T));
    }
    size_t max_size() const {
        return size_t(-1) / sizeof(value_type);
    }

    void construct(T *p, const T &val) { new(static_cast<void *>(p)) T(val); }
    void destroy(T *p) { p->~T(); }
};

template <typename T, typename U>
bool operator==(const alloc<T>&, const alloc<U>) {
    return true;
}

template <typename T, typename U>
bool operator!=(const alloc<T>&, const alloc<U>) {
    return false;
}

void *operator new(size_t bytes, arena *a) {
    return a->allocate(bytes);
}
