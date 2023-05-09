#define BASM_UTILS
#define BASM_CREATE
#define BASM_VM

#include "libbasm.h"
#include "wrapper.h"

MManager manager = {0};
Basm basm = {0};

void basm_translate_file(const char *input_file_path, const char *output_file_path) {
    basm_translate_source(cstr_as_sv(input_file_path), &basm, &manager);
    basm_save_to_file(&basm, output_file_path);
}
