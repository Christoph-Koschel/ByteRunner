#define BASM_UTILS
#define BASM_VM

#include "libbasm.h"

ByteRunner br = {0};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./dbrasm <program>\n");
        fprintf(stderr, "ERROR: no program provided\n");
        return 1;
    }

    const char *program_path = argv[1];

    br_load_program_from_file(&br, program_path);

    for (uint64_t i = 0; i < br.program_size; i++) {
        printf("%s", inst_asm_name(br.program[i].type));
        if (inst_has_operand(br.program[i].type)) {
            printf(" %ld", br.program[i].operand.as_i64);
        }
        printf("\n");
    }

    return 0;
}
