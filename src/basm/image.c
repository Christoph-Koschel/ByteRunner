#define BASM_UTILS
#define BASM_VM

#include "libbasm.h"

static Err br_alloc(ByteRunner *br) {
    if (br->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    br->stack[br->stack_size - 1].as_ptr = malloc(br->stack[br->stack_size - 1].as_u64);

    return ERR_OK;
}

static Err br_free(ByteRunner *br) {
    if (br->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    free(br->stack[br->stack_size - 1].as_ptr);
    br->stack_size--;

    return ERR_OK;
}

static Err br_print_f64(ByteRunner *br) {
    if (br->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%lf\n", br->stack[br->stack_size - 1].as_f64);
    br->stack_size--;

    return ERR_OK;
}

static Err br_print_i64(ByteRunner *br) {
    if (br->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%"PRIi64"\n", br->stack[br->stack_size - 1].as_i64);
    br->stack_size--;

    return ERR_OK;
}

static Err br_print_u64(ByteRunner *br) {
    if (br->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%"PRIu64"\n", br->stack[br->stack_size - 1].as_u64);
    br->stack_size--;

    return ERR_OK;
}

static Err br_print_ptr(ByteRunner *br) {
    if (br->stack_size < 1) {
        return ERR_STACK_UNDERFLOW;
    }

    printf("%p\n", br->stack[br->stack_size - 1].as_ptr);
    br->stack_size--;

    return ERR_OK;
}

static Err br_dump_memory(ByteRunner *br) {
    if (br->stack_size < 2) {
        return ERR_STACK_UNDERFLOW;
    }

    MemoryAddr addr = br->stack[br->stack_size - 2].as_u64;
    uint64_t count = br->stack[br->stack_size - 1].as_u64;

    if (addr >= BR_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    if (addr + count < addr || addr + count >= BR_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }


    for (uint64_t i = 0; i < count; i++) {
        printf("%02X ", br->memory[i]);
    }
    printf("\n");

    br->stack_size -= 2;

    return ERR_OK;
}

static Err br_write(ByteRunner *br) {
    if (br->stack_size < 2) {
        return ERR_STACK_UNDERFLOW;
    }

    MemoryAddr addr = br->stack[br->stack_size - 2].as_u64;
    uint64_t count = br->stack[br->stack_size - 1].as_u64;

    if (addr >= BR_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    if (addr + count < addr || addr + count >= BR_MEMORY_CAPACITY) {
        return ERR_ILLEGAL_MEMORY_ACCESS;
    }

    fwrite(&br->memory[addr], sizeof(br->memory[0]), count, stdout);
    br->stack_size -= 2;

    return ERR_OK;
}


ByteRunner br = {0};

extern char _binary___code_start[];
extern char _binary___code_end[];


static size_t read_buf(void *dst, size_t item_size, size_t count, const void *buf) {
    size_t retrieved = 0;
    for (size_t i = 0; i < count; i++) {

        if (buf + item_size * count > dst + item_size * count) {
            return 0;
        }

        memcpy(dst, buf, item_size * count);
        retrieved++;
    }

    return retrieved;
}

int main(int argc, char **argv) {
    const char *program = argv[0];
    // unsigned int _binary___print_i64_size = (unsigned int) ((unsigned int) &_binary___print_i64_end -
    //                                                         (unsigned int) &_binary___print_i64_start);

    char *ptr = _binary___code_start;

    BasmFileMeta meta = {0};
    read_buf(&meta, sizeof(BasmFileMeta), 1, ptr);
    ptr += sizeof(meta);

    if (meta.magic != BR_FILE_MAGIC) {
        fprintf(stderr, "ERROR: '%s' does not appear to be a valid file. "
                        "Unexpected magic %04X. Expected %04X\n", program, meta.magic, BR_FILE_MAGIC);
        exit(1);
    }

    if (meta.version != BR_FILE_VERSION) {
        fprintf(stderr, "ERROR: Unsupported version %d, Expected version %d\n", meta.version, BR_FILE_VERSION);
        exit(1);
    }

    if (meta.program_size > BR_PROGRAM_CAPACITY) {
        fprintf(stderr,
                "ERROR: '%s': program section is too large. The file contains %" PRIu64 " program instructions. But the capacity is %"  PRIu64 "\n",
                program, meta.program_size, (uint64_t) BR_PROGRAM_CAPACITY);
        exit(1);
    }

    if (meta.memory_capacity > BR_MEMORY_CAPACITY) {
        fprintf(stderr,
                "ERROR: '%s': memory section is too large. The file wants %" PRIu64 "bytes. But the capacity is %"  PRIu64 "bytes \n",
                program, meta.memory_capacity, (uint64_t) BR_MEMORY_CAPACITY);
        exit(1);
    }

    if (meta.memory_size > meta.memory_capacity) {
        fprintf(stderr,
                "ERROR: '%s': memory size %" PRIu64 " is greater than the declared memory capacity %" PRIu64 "\n",
                program, meta.memory_size, meta.memory_capacity);
        exit(1);
    }
    br.ip = meta.entry;

    br.program_size = read_buf(&br.program, sizeof(br.program[0]), meta.program_size, ptr);
    ptr += br.program_size * sizeof(br.program[0]);

    if (br.program_size != meta.program_size) {
        fprintf(stderr, "ERROR: '%s', read %zd program instructions, but expected %" PRIu64 "\n",
                program, br.program_size, meta.program_size);
    }

    size_t n = read_buf(&br.memory, sizeof(br.memory[0]), meta.memory_size, ptr);
    if (n != meta.memory_size) {
        fprintf(stderr, "ERROR: '%s', read %zd bytes of memory section, but expected %" PRIu64 " bytes\n",
                program, n, meta.memory_size);
    }

    br_push_native(&br, br_alloc);
    br_push_native(&br, br_free);
    br_push_native(&br, br_print_f64);
    br_push_native(&br, br_print_i64);
    br_push_native(&br, br_print_u64);
    br_push_native(&br, br_print_ptr);
    br_push_native(&br, br_dump_memory);
    br_push_native(&br, br_write);

    Err err = br_execute_program(&br, -1);
    if (err != ERR_OK) {
        fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
        return 1;
    }

    return 0;
}
