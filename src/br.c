#define BASM_UTILS
#define BASM_VM

#include "./natives.c"

static void usage(FILE *stream, const char *program) {
    fprintf(stream, "Usage: %s -i <program> [-l <limit>] [-h]\n", program);
}

ByteRunner br = {0};

int main(int argc, char **argv) {
    char *program = shift(&argc, &argv);
    const char *program_file_path = NULL;
    int limit = -1;
    int debug = 0;

    while (argc > 0) {
        const char *flag = shift(&argc, &argv);
        if (strcmp(flag, "-i") == 0) {
            if (argc == 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: No argument is provided for flag '%s'\n", flag);
                return 1;
            }

            program_file_path = shift(&argc, &argv);
        } else if (strcmp(flag, "-l") == 0) {
            if (argc == 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: No argument is provided for flag '%s'\n", flag);
                return 1;
            }

            limit = atoi(shift(&argc, &argv));
        } else if (strcmp(flag, "-h") == 0) {
            usage(stdout, program);
            return 0;
        } else if (strcmp(flag, "-d") == 0) {
            debug = 1;
        } else {
            usage(stderr, program);
            fprintf(stderr, "ERROR: Unknown flag '%s'\n", flag);
        }
    }


    if (program_file_path == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: Input was not provided\n");
        return 1;
    }

    br_load_program_from_file(&br, program_file_path);
    br_push_native(&br, br_alloc);
    br_push_native(&br, br_free);
    br_push_native(&br, br_print_f64);
    br_push_native(&br, br_print_i64);
    br_push_native(&br, br_print_u64);
    br_push_native(&br, br_print_ptr);
    br_push_native(&br, br_dump_memory);
    br_push_native(&br, br_write);

    if (!debug) {
        Err err = br_execute_program(&br, limit);
        if (err != ERR_OK) {
            fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
            return 1;
        }
    } else {
        while (limit != 0 && !br.halt) {
            br_dump_stack(stdout, &br);
//            br_dump_memory(stdout, &br);
            printf("Instruction: %s %ld\n",
                   inst_asm_name(br.program[br.ip].type),
                   br.program[br.ip].operand.as_u64);
            getchar();
            Err err = br_execute_inst(&br);
            if (err != ERR_OK) {
                fprintf(stderr, "ERROR: %s\n", err_as_cstr(err));
                return 1;
            }

            if (limit > 0) {
                limit--;
            }
        }

        return ERR_OK;
    }


    return 0;
}
