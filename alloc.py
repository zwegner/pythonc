################################################################################
##
## Pythonc--Python to C++ translator
##
## Copyright 2011 Zach Wegner
##
## This file is part of Pythonc.
##
## Pythonc is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Pythonc is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Pythonc.  If not, see <http://www.gnu.org/licenses/>.
##
################################################################################

def write_allocator(f):
    block_size = 1 << 14
    chunk_size = 1 << 21

    f.write("""
#define BLOCK_SIZE (%s)
#define CHUNK_SIZE (%s)

typedef unsigned char byte;

uint32_t bitscan64(uint64_t r) {
   asm ("bsfq %%0, %%0" : "=r" (r) : "0" (r));
   return r;
}

template <uint64_t obj_size>
class arena_block {
public:
    // Static head pointer, one per arena block size.
    static arena_block<obj_size> *head;
    static const uint64_t capacity = BLOCK_SIZE - sizeof(void *);
    static const uint64_t n_objects = (capacity * 8 / (obj_size * 8 + 1));
    static const uint64_t n_live = (n_objects + 63) / 64;

    byte data[obj_size * n_objects];
    uint64_t live_bits[n_live];
    arena_block<obj_size> *next_block;
    byte padding[capacity - (sizeof(data) + sizeof(live_bits))];

    void init() {
        assert(sizeof(arena_block<obj_size>) == BLOCK_SIZE);
        assert(sizeof(this->live_bits) * 8 >= n_objects);
        mark_dead();
    }
    void mark_dead() {
        for (uint32_t t = 0; t < n_live - 1; t++)
            this->live_bits[t] = 0;
        // For the last chunk of live bits, some bits could represent objects past
        // the end of the block, so make sure their live bits are always set,
        // and thus not be used
        this->live_bits[n_live - 1] = -1ull << (n_objects & 63);
    }
    void *get_obj() {
        for (uint32_t t = 0; t < n_live; t++) {
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
    bool mark_live(void *object) {
        uint32_t idx = ((uint64_t)object & (BLOCK_SIZE - 1)) / obj_size;
        uint32_t t = idx / 64;
        uint64_t bit = 1ull << (idx & 63);
        bool already_live = (this->live_bits[t] & bit) != 0ull;
        this->live_bits[t] |= bit;
        return already_live;
    }
};

template<uint64_t x> arena_block<x> *arena_block<x>::head;

// Silly preprocessor stuff for "cleanly" handling multiple block sizes.
#define FOR_EACH_OBJ_SIZE(x) x(16) x(24) x(32) x(56)

class arena {
private:
    byte *chunk_start, *chunk_end;

public:
    arena() {
        this->get_new_chunk();
#define INIT_BLOCK(size) \\
        arena_block<size>::head = this->new_block<size>();

        FOR_EACH_OBJ_SIZE(INIT_BLOCK)
#undef INIT_BLOCK
    }

    void get_new_chunk() {
        this->chunk_start = new byte[CHUNK_SIZE];
        this->chunk_end = chunk_start + CHUNK_SIZE; 
        // Align the start of the chunk
        this->chunk_start = (byte *)(((uint64_t)this->chunk_start + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1));
    }

    template<size_t bytes>
    arena_block<bytes> *new_block() {
        if (this->chunk_end - this->chunk_start < BLOCK_SIZE)
            this->get_new_chunk();

        arena_block<bytes> *block = (arena_block<bytes> *)this->chunk_start;
        this->chunk_start += BLOCK_SIZE;
        block->init();
        return block;
    }

    template<size_t bytes>
    void *allocate() {
        arena_block<bytes> *block = arena_block<bytes>::head;
        void *p = block->get_obj();
        if (!p) {
            block = this->new_block<bytes>();
            block->next_block = arena_block<bytes>::head;
            arena_block<bytes>::head = block;
            p = block->get_obj();
        }
        return p;
    }
    void mark_dead() {
#define MARK_DEAD(size) \\
        for (arena_block<size> *p = arena_block<size>::head; p; p = p->next_block) \\
            p->mark_dead();

        FOR_EACH_OBJ_SIZE(MARK_DEAD)
#undef MARK_DEAD
    }
    template<size_t bytes>
    bool mark_live(void *object) {
        void *block = (void *)((uint64_t)object & ~(BLOCK_SIZE - 1));
        return ((arena_block<bytes> *)block)->mark_live(object);
    }
};

arena allocator[1];

inline void *operator new(const size_t bytes, arena *a) {
    switch (bytes) {
#define OBJ_CASE(size) case size: return a->allocate<size>();
        FOR_EACH_OBJ_SIZE(OBJ_CASE)
        default:
            printf("bad obj size %%" PRIu64 "\\n", bytes);
            exit(1);
    }
}
""" % (block_size, chunk_size))
