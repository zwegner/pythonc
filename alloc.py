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
    block_size_pow2 = 14
    block_size = 1 << block_size_pow2
    chunk_size = 1 << 21

    obj_sizes = [16, 24, 32, 56]

    f.write("""
#define BLOCK_SIZE (%s)
#define CHUNK_SIZE (%s)

typedef unsigned char byte;

uint32_t bitscan64(uint64_t r) {
   asm ("bsfq %%0, %%0" : "=r" (r) : "0" (r));
   return r;
}
static byte *alloc_chunk_start, *alloc_chunk_end;

static inline void alloc_chunk() {
    alloc_chunk_start = new byte[CHUNK_SIZE];
    alloc_chunk_end = alloc_chunk_start + CHUNK_SIZE; 
    // Align the start of the chunk
    alloc_chunk_start = (byte *)(((uint64_t)alloc_chunk_start + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1));
}
""" % (block_size, chunk_size))

    # Write out "templates" of allocator arena blocks based on size
    ptr_size = 8 # HACK
    capacity = block_size - ptr_size

    for obj_size in obj_sizes:
        n_objects = (capacity * 8 // (obj_size * 8 + 1))
        n_live = (n_objects + 63) // 64
        padding = capacity - (n_objects * obj_size + n_live * 8)

        f.write("""
class arena_block_{obj_size} {{
public:
    static const uint64_t obj_size = {obj_size};
    static const uint64_t n_objects = {n_objects};
    static const uint64_t n_live = {n_live};

    byte data[n_objects][obj_size];
    uint64_t live_bits[n_live];
    arena_block_{obj_size} *next_block;
    byte padding[{padding_size}];

    static arena_block_{obj_size} *head;

    static inline arena_block_{obj_size} *alloc_block() {{
        if (alloc_chunk_end - alloc_chunk_start < BLOCK_SIZE)
            alloc_chunk();

        auto p = (arena_block_{obj_size} *)alloc_chunk_start;
        alloc_chunk_start += BLOCK_SIZE;
        p->mark_dead();
        return p;
    }}

    static void *alloc_obj() {{
        auto block = arena_block_{obj_size}::head;
        void *p = block->get_next_obj();
        if (!p) {{
            block = alloc_block();
            block->next_block = arena_block_{obj_size}::head;
            arena_block_{obj_size}::head = block;
            p = block->get_next_obj();
        }}
        return p;
    }}

    void init() {{
        assert(sizeof(*this) == BLOCK_SIZE);
        assert(sizeof(this->live_bits) * 8 >= n_objects);
        mark_dead();
    }}
    void mark_dead() {{
        for (uint32_t t = 0; t < n_live - 1; t++)
            this->live_bits[t] = 0;
        // For the last chunk of live bits, some bits could represent objects past
        // the end of the block, so make sure their live bits are always set,
        // and thus not be used
        this->live_bits[n_live - 1] = -1ull << (n_objects & 63);
    }}
    void *get_next_obj() {{
        for (uint32_t t = 0; t < n_live; t++) {{
            uint64_t dead = ~this->live_bits[t];
            if (dead) {{
                uint32_t bit = bitscan64(dead);
                uint32_t idx = t * 64 + bit;
                this->live_bits[t] |= (1ull << bit);
                return (void *)this->data[idx];
            }}
        }}
        return NULL;
    }}
    bool mark_live(void *object) {{
        uint32_t idx = ((uint64_t)object & (BLOCK_SIZE - 1)) / obj_size;
        uint32_t t = idx / 64;
        uint64_t bit = 1ull << (idx & 63);
        bool already_live = (this->live_bits[t] & bit) != 0ull;
        this->live_bits[t] |= bit;
        return already_live;
    }}
}};
arena_block_{obj_size} *arena_block_{obj_size}::head;
""".format(obj_size=obj_size, n_objects=n_objects, n_live=n_live, padding_size=padding))

    def dispatch_objsize(size):
        f.write('switch (%s) {\n' % size)
        for obj_size in obj_sizes:
            f.write('    case %s: {\n' % obj_size)
            yield 'arena_block_%s' % obj_size
            f.write('    } break;\n')
        f.write('    default: assert(!"bad obj size"); return NULL;\n')
        f.write('}\n')

    f.write('class allocator {\n')
    f.write('public:\n')

    f.write('    allocator() {\n')
    for obj_size in obj_sizes:
        f.write('        arena_block_%s::head = arena_block_%s::alloc_block();\n' % (obj_size, obj_size))
        f.write('        arena_block_%s::head->next_block = NULL;\n' % obj_size)
    f.write('    }\n')

    f.write('    template<class T>\n')
    f.write('    T *alloc_obj() {\n')
    for t in dispatch_objsize('sizeof(T)'):
        f.write('        return (T *)%s::alloc_obj();\n' % (t))
    f.write('    }\n')

    f.write('    void mark_dead() {\n')
    for obj_size in obj_sizes:
        f.write('        for (auto *p = arena_block_%s::head; p; p = p->next_block)\n' % obj_size)
        f.write('            p->mark_dead();\n')
    f.write('    }\n')

    f.write('    template<size_t bytes>\n')
    f.write('    bool mark_live(void *object) {\n')
    f.write('        void *block = (void *)((uint64_t)object & ~(BLOCK_SIZE - 1));\n')
    for t in dispatch_objsize('bytes'):
        f.write('        return ((%s *)block)->mark_live(object);\n' % t)
    f.write('    }\n')

    f.write('} alloc;\n')
