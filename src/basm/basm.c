#define BASM_UTILS
#define BASM_CREATE

#include "libbasm.h"

Basm basm = {0};
MManager manager = {0};

static void usage(FILE *stream, const char *program) {
    fprintf(stream, "Usage: %s [-o <output>] <input>\n", program);
}

int main(int argc, char **argv) {
    char *program = shift(&argc, &argv);
    char *output_file_path = "a";
    char *input_file_path = NULL;

    while (argc > 0) {
        char *flag = shift(&argc, &argv);
        if (strcmp(flag, "-o") == 0) {
            if (argc == 0) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: No argument is provided for flag '%s'\n", flag);
                return 1;
            }

            output_file_path = shift(&argc, &argv);
        } else {
            if (input_file_path != NULL) {
                usage(stderr, program);
                fprintf(stderr, "ERROR: Unknown flag '%s'\n", flag);
                return 1;
            }

            input_file_path = flag;
        }
    }

    if (input_file_path == NULL) {
        usage(stderr, program);
        fprintf(stderr, "ERROR: No input file specified\n");
        return 1;
    }

    basm_translate_source(cstr_as_sv(input_file_path), &basm, &manager);

    if (!basm.has_entry) {
        fprintf(stderr,
                "%s: ERROR: Entry point for the program is not provided. Use preprocessor directive %%entry to provide the entry point:\n",
                input_file_path);
        fprintf(stderr, "    %%entry main\n");
        fprintf(stderr, "    %%entry 654\n");
        return 1;
    }

    size_t written_size = basm_save_to_file(&basm, output_file_path);


    printf("%zd bytes of memory used\n", manager.arena_size);
    printf("%zd bytes written to file\n", written_size);
    printf("Entry point at 0x%08X\n", (uint32_t) basm.entry);

    return 0;
}
