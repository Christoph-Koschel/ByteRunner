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
