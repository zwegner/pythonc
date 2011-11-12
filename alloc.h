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

#define BLOCK_SIZE (1 << 14)
#define CHUNK_SIZE (1 << 22)

typedef unsigned char byte;

uint32_t bitscan64(uint64_t r) {
   asm ("bsfq %0, %0" : "=r" (r) : "0" (r));
   return r;
}

template <uint64_t obj_size>
class arena_block {
public:
    static const uint64_t capacity = BLOCK_SIZE - sizeof(void *);
    static const uint64_t n_objects = (capacity * 64 / (obj_size * 64 + 8)) & ~63;
    byte data[obj_size * n_objects];
    arena_block<obj_size> *next_block;
    uint64_t live_bits[n_objects / 64];
    byte padding[capacity - (obj_size * n_objects + n_objects / 8)];

    void init() {
        assert(sizeof(arena_block<obj_size>) == BLOCK_SIZE);
        assert(sizeof(this->live_bits) * 8 == n_objects);
        mark_dead();
    }
    void mark_dead() {
        for (uint32_t t = 0; t < n_objects / 64; t++)
            this->live_bits[t] = 0;
    }
    void *get_obj() {
        for (uint32_t t = 0; t < n_objects / 64; t++) {
            uint64_t dead = ~this->live_bits[t];
            if (dead) {
                uint32_t bit = bitscan64(dead);
                uint32_t idx = t * 64 + bit;
                this->live_bits[t] |= (1ull << bit);
                byte *b = &this->data[idx * obj_size];
                return (void *)b;
            }
        }
        return NULL;
    }
    bool mark_live(uint32_t idx) {
        uint32_t t = idx / 64;
        uint64_t bit = 1ull << (idx & 63);
        bool already_live = (this->live_bits[t] & bit) != 0ull;
        this->live_bits[t] |= bit;
        return already_live;
    }
};

// Silly preprocessor stuff for "cleanly" handling multiple block sizes.
#define FOR_EACH_OBJ_SIZE(x) x(16) x(24) x(32) x(40) x(56) x(64) x(72)

class arena {
private:
#define DECL_BLOCK(size) arena_block<size> *head_##size;
    FOR_EACH_OBJ_SIZE(DECL_BLOCK)
#undef DECL_BLOCK
    byte *chunk_start, *chunk_end;

public:
    arena() {
        this->get_new_chunk();
#define INIT_BLOCK(size) \
        this->head_##size = (arena_block<size> *)this->new_block(); \
        this->head_##size->init();

        FOR_EACH_OBJ_SIZE(INIT_BLOCK)
#undef INIT_BLOCK
    }

    void get_new_chunk() {
        this->chunk_start = new byte[CHUNK_SIZE];
        this->chunk_end = chunk_start + CHUNK_SIZE; 
        // Align the start of the chunk
        this->chunk_start = (byte *)(((uint64_t)this->chunk_start + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1));
    }

    void *new_block() {
        if (this->chunk_end - this->chunk_start < BLOCK_SIZE)
            this->get_new_chunk();

        void *block = this->chunk_start;
        this->chunk_start += BLOCK_SIZE;
        return block;
    }

    template<size_t bytes>
    void *allocate() {
        switch (bytes) {
#define OBJ_CASE(size) \
            case size: { \
                arena_block<size> *block = this->head_##size; \
                void *p = block->get_obj(); \
                if (!p) { \
                    block = (arena_block<size> *)this->new_block(); \
                    block->init(); \
                    block->next_block = this->head_##size; \
                    this->head_##size = block; \
                    p = block->get_obj(); \
                } \
                return p; \
            }

            FOR_EACH_OBJ_SIZE(OBJ_CASE)
#undef OBJ_CASE
            default:
                printf("bad obj size %" PRIu64 "\n", bytes);
                exit(1);
        }
        return NULL;
    }
    void mark_dead() {
#define MARK_DEAD(size) \
        for (arena_block<size> *p = this->head_##size; p; p = p->next_block) \
            p->mark_dead();

        FOR_EACH_OBJ_SIZE(MARK_DEAD)
#undef MARK_DEAD
    }
    template<size_t bytes>
    bool mark_live(void *object) {
        uint32_t idx = ((uint64_t)object & (BLOCK_SIZE - 1)) / bytes;
        void *block = (void *)((uint64_t)object & ~(BLOCK_SIZE - 1));
        switch (bytes) {
#define OBJ_CASE(size) case size: return ((arena_block<size> *)block)->mark_live(idx);
            FOR_EACH_OBJ_SIZE(OBJ_CASE)
#undef OBJ_CASE
            default:
                printf("bad obj size %" PRIu64 "\n", bytes);
                exit(1);
        }
    }
};

arena allocator[1];

inline void *operator new(const size_t bytes, arena *a) {
    switch (bytes) {
#define OBJ_CASE(size) case size: return a->allocate<size>();
        FOR_EACH_OBJ_SIZE(OBJ_CASE)
        default:
            printf("bad obj size %" PRIu64 "\n", bytes);
            exit(1);
    }
}
