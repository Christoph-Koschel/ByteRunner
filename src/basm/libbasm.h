// define BASM_CREATE for implementing create function
// define BASM_VM for implementing the vm

#ifndef BYTERUNNER_LIBBR_H
#define BYTERUNNER_LIBBR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

// FROM https://stackoverflow.com/a/3312896
#if defined(__GNUC__) || defined(__clang__)
# define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#elif defined(_MSC_VER)
# define PACK(__Declaration__) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#else
# error "Packed attributes for struct is not implmented for this compiler. This may result in a program working incorrectly"
# define PACK
#endif

#define BR_STACK_CAPACITY 1024
#define BR_WORD_SIZE 8
#define BR_PROGRAM_CAPACITY 1024
#define BR_LABEL_CAPACITY 1024
#define BR_UNRESOLVED_JMPS_CAPACITY 1024
#define BR_NATIVE_CAPACITY 1024
#define BR_MEMORY_CAPACITY (640 * 1000)
#define BR_ASSEMBLY_MEMORY_CAPACITY (1000 * 1000 * 1000)

#define BR_FILE_MAGIC 0x5242
#define BR_FILE_VERSION 1

#define WORD_U64(u64)((Word) { .as_u64 = u64 })
#define WORD_I64(i64)((Word) { .as_i64 = i64 })
#define WORD_F64(f64)((Word) { .as_f64 = f64 })
#define WORD_PTR(ptr)((Word) { .as_ptr = ptr })

#define BR_ASSEMBLY_COMMENT ';'
#define BR_ASSEMBLY_PREPROCESSOR '%'
#define BR_ASSEMBLY_MAX_INCLUDE_LEVEL 10

#define BINARY_OP(br, in, out, op)                                                               \
{                                                                                                \
    if ((br)->stack_size < 2) {                                                                  \
        return ERR_STACK_UNDERFLOW;                                                              \
    }                                                                                            \
    (br)->stack[(br)->stack_size - 2].as_##out =                                                 \
        (br)->stack[(br)->stack_size - 2].as_##in op (br)->stack[(br)->stack_size - 1].as_##in;  \
    (br)->stack_size--;                                                                          \
    (br)->ip++;                                                                                  \
    break;                                                                                       \
}

#define CAST_OP(br, from, to, cast)                                                              \
{                                                                                                \
    if ((br)->stack_size < 1) {                                                                  \
        return ERR_STACK_UNDERFLOW;                                                              \
    }                                                                                            \
    (br)->stack[(br)->stack_size - 1].as_##to = cast (br)->stack[(br)->stack_size - 1].as_##from;\
    (br)->ip++;                                                                                  \
    break;                                                                                       \
}

/// ========================================
/// STRING VIEW
/// ========================================
/// region

typedef struct {
    size_t count;
    const char *data;
} StringView;

StringView cstr_as_sv(const char *cstr);

StringView sv_trim_left(StringView sv);

StringView sv_trim_right(StringView sv);

StringView sv_trim(StringView sv);

StringView sv_chop_by_delim(StringView *sv, char delim);

int sv_eq(StringView a, StringView b);

int sv_to_int(StringView sv);

char *shift(int *argc, char ***argv);

/// endregion
/// ========================================
/// BR & BASM Structures
/// ========================================
/// region

typedef enum {
    ERR_OK = 0,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_ILLEGAL_INS,
    ERR_DIV_BY_ZERO,
    ERR_ILLEGAL_INS_ACCESS,
    ERR_ILLEGAL_OPERAND,
    ERR_ILLEGAL_MEMORY_ACCESS
} Err;

typedef uint64_t InstAddr;
typedef uint64_t MemoryAddr;

typedef union {
    int64_t as_i64;
    uint64_t as_u64;
    double as_f64;
    void *as_ptr;
} Word;

static_assert(sizeof(Word) == BR_WORD_SIZE, "The Word union is expected to be 64 bits");

typedef enum {
    INST_NOP = 0,
    INST_DUP,
    INST_SWAP,
    INST_PUSH,
    INST_POP,

    INST_PLUSI,
    INST_MINUSI,
    INST_MULTI,
    INST_DIVI,
    INST_MODI,
    INST_GEI,
    INST_LEI,
    INST_LI,
    INST_NEI,
    INST_GI,
    INST_EQI,

    INST_PLUSF,
    INST_MINUSF,
    INST_MULTF,
    INST_DIVF,
    INST_GEF,
    INST_GF,
    INST_LEF,
    INST_LF,
    INST_NEF,
    INST_EQF,

    INST_ANDB,
    INST_ORB,
    INST_XOR,
    INST_SHR,
    INST_SHL,
    INST_NOTB,

    INST_CALL,
    INST_INT,
    INST_JMP,
    INST_JMP_IF,
    INST_RET,

    INST_READ8,
    INST_READ16,
    INST_READ32,
    INST_READ64,

    INST_WRITE8,
    INST_WRITE16,
    INST_WRITE32,
    INST_WRITE64,

    INST_I2F,
    INST_I2U,
    INST_U2F,
    INST_U2I,
    INST_F2I,
    INST_F2U,

    INST_NOT,
    INST_HALT,
    SIZE
} InstType;

typedef struct {
    InstType type;
    Word operand;
} Inst;

typedef struct ByteRunner ByteRunner;

typedef Err (*Br_Native)(ByteRunner *);

struct ByteRunner {
    Word stack[BR_STACK_CAPACITY];
    uint64_t stack_size;

    Inst program[BR_PROGRAM_CAPACITY];
    uint64_t program_size;
    InstAddr ip;

    Br_Native natives[BR_NATIVE_CAPACITY];
    size_t natives_size;

    uint8_t memory[BR_MEMORY_CAPACITY];

    int halt;
};

typedef struct {
    StringView name;
    Word word;
} Label;

typedef struct {
    InstAddr addr;
    StringView label;
} UnresolvedJmp;

typedef struct {
    Label labels[BR_LABEL_CAPACITY];
    size_t labels_size;

    UnresolvedJmp unresolved_jmps[BR_UNRESOLVED_JMPS_CAPACITY];
    size_t unresolved_jmp_size;

    char arena[BR_ASSEMBLY_MEMORY_CAPACITY];
    size_t arena_size;

    Inst program[BR_PROGRAM_CAPACITY];
    size_t program_size;
    InstAddr entry;
    int has_entry;

    uint8_t memory[BR_MEMORY_CAPACITY];
    size_t memory_size;
    size_t memory_capacity;

    size_t inc_level;
} Basm;

PACK(struct BasmFileMeta {
         uint16_t magic;
         uint16_t version;
         uint64_t entry;
         uint64_t program_size;
         uint64_t memory_size;
         uint64_t memory_capacity;
     });

typedef struct BasmFileMeta BasmFileMeta;

/// endregion
/// ========================================
/// BASM functions
/// ========================================
/// region

void *basm_alloc(Basm *basm, size_t size);

size_t basm_save_to_file(Basm *basm, const char *file_path);

int basm_resolve_label(const Basm *basm, StringView name, Word *output);

int basm_bind_label(Basm *basm, StringView name, Word word);

void basm_bind_unresolved(Basm *basm, InstAddr addr, StringView label);

Word basm_push_string_to_memory(Basm *basm, StringView sv);

int basm_translate_literal(Basm *basm, StringView sv, Word *output);

void basm_translate_source(StringView input_file_path, Basm *basm);

StringView basm_slurp_file(Basm *basm, StringView file_path);

// endregion
/// ========================================
/// BR functions
/// ========================================
/// region

int inst_has_operand(InstType type);

const char *inst_asm_name(InstType type);

int inst_by_name(StringView *name, InstType *output);

const char *err_as_cstr(Err err);

Err br_execute_inst(ByteRunner *br);

Err br_execute_program(ByteRunner *br, int limit);

void br_push_native(ByteRunner *br, Br_Native native);

void br_dump_stack(FILE *stream, const ByteRunner *br);

void br_load_program_from_file(ByteRunner *br, const char *file_path);

/// endregion
#endif
#ifdef BASM_UTILS

void *basm_alloc(Basm *basm, size_t size) {
    assert(basm->arena_size + size <= BR_ASSEMBLY_MEMORY_CAPACITY);

    void *result = basm->arena + basm->arena_size;
    basm->arena_size += size;
    return result;
}

const char *inst_asm_name(InstType type) {
    switch (type) {
        case INST_NOP:
            return "nop";
        case INST_DUP:
            return "dup";
        case INST_SWAP:
            return "swap";
        case INST_POP:
            return "pop";
        case INST_PUSH:
            return "push";
        case INST_PLUSI:
            return "plusi";
        case INST_MINUSI:
            return "minusi";
        case INST_MULTI:
            return "multi";
        case INST_DIVI:
            return "divi";
        case INST_MODI:
            return "modi";
        case INST_PLUSF:
            return "plusf";
        case INST_MINUSF:
            return "minusf";
        case INST_MULTF:
            return "multf";
        case INST_DIVF:
            return "divf";
        case INST_EQF:
            return "eqf";
        case INST_GEF:
            return "gef";
        case INST_NEF:
            return "nef";
        case INST_GEI:
            return "gei";
        case INST_CALL:
            return "call";
        case INST_INT:
            return "int";
        case INST_JMP:
            return "jmp";
        case INST_JMP_IF:
            return "jmpif";
        case INST_RET:
            return "ret";
        case INST_EQI:
            return "eqi";
        case INST_NOT:
            return "not";
        case INST_HALT:
            return "halt";
        case INST_ANDB:
            return "andb";
        case INST_ORB:
            return "orb";
        case INST_XOR:
            return "xor";
        case INST_SHR:
            return "shr";
        case INST_SHL:
            return "shl";
        case INST_NOTB:
            return "notb";
        case INST_READ8:
            return "read8";
        case INST_READ16:
            return "read16";
        case INST_READ32:
            return "read32";
        case INST_READ64:
            return "read64";
        case INST_WRITE8:
            return "write8";
        case INST_WRITE16:
            return "write16";
        case INST_WRITE32:
            return "write32";
        case INST_WRITE64:
            return "write64";
        case INST_LEI:
            return "lei";
        case INST_LI:
            return "li";
        case INST_NEI:
            return "nei";
        case INST_GI:
            return "gi";
        case INST_GF:
            return "gf";
        case INST_LEF:
            return "lef";
        case INST_LF:
            return "lf";
        case INST_I2F:
            return "i2f";
        case INST_I2U:
            return "i2u";
        case INST_U2F:
            return "u2f";
        case INST_U2I:
            return "u2i";
        case INST_F2I:
            return "f2i";
        case INST_F2U:
            return "f2u";
        case SIZE:
        default:
            assert(0 && "inst_asm_name: Unreachable");
            return "";
    }
}

int inst_by_name(StringView *name, InstType *output) {
    for (InstType type = (InstType) 0; type < SIZE; type++) {
        if (sv_eq(*name, cstr_as_sv(inst_asm_name(type)))) {
            *output = type;
            return 1;
        }
    }

    return 0;
}

int inst_has_operand(InstType type) {
    switch (type) {
        case INST_NOP:
        case INST_POP:
        case INST_PLUSI:
        case INST_MINUSI:
        case INST_MULTI:
        case INST_DIVI:
        case INST_MODI:
        case INST_GEI:
        case INST_PLUSF:
        case INST_MINUSF:
        case INST_MULTF:
        case INST_DIVF:
        case INST_EQF:
        case INST_GEF:
        case INST_EQI:
        case INST_NOT:
        case INST_HALT:
        case INST_RET:
        case INST_ANDB:
        case INST_ORB:
        case INST_XOR:
        case INST_SHR:
        case INST_SHL:
        case INST_NOTB:
        case INST_READ8:
        case INST_READ16:
        case INST_READ32:
        case INST_READ64:
        case INST_WRITE8:
        case INST_WRITE16:
        case INST_WRITE32:
        case INST_WRITE64:
        case INST_LEI:
        case INST_LI:
        case INST_NEI:
        case INST_GI:
        case INST_GF:
        case INST_LEF:
        case INST_LF:
        case INST_NEF:
        case INST_I2F:
        case INST_I2U:
        case INST_U2F:
        case INST_U2I:
        case INST_F2I:
        case INST_F2U:
            return 0;

        case INST_DUP:
        case INST_SWAP:
        case INST_PUSH:
        case INST_CALL:
        case INST_INT:
        case INST_JMP:
        case INST_JMP_IF:
            return 1;
        case SIZE:
        default:
            assert(0 && "inst_has_operand: Unreachable");
            return 0;
    }
}

Word basm_push_string_to_memory(Basm *basm, StringView sv) {
    assert(basm->memory_size + sv.count <= BR_MEMORY_CAPACITY);

    Word result = WORD_U64(basm->memory_size);
    memcpy(basm->memory + basm->memory_size, sv.data, sv.count);
    basm->memory_size += sv.count;

    if (basm->memory_size > basm->memory_capacity) {
        basm->memory_capacity = basm->memory_size;
    }

    return result;
}

/// ========================================
/// STRING VIEW
/// ========================================
/// region

StringView cstr_as_sv(const char *cstr) {
    return (StringView) {
            .count = strlen(cstr),
            .data = cstr
    };
}

StringView sv_trim_left(StringView sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i++;
    }

    return (StringView) {
            .count = sv.count - i,
            .data = sv.data + i
    };
}

StringView sv_trim_right(StringView sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i++;
    }

    return (StringView) {
            .count = sv.count - i,
            .data = sv.data
    };
}

StringView sv_trim(StringView sv) {
    return sv_trim_right(sv_trim_left(sv));
}

StringView sv_chop_by_delim(StringView *sv, char delim) {
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i++;
    }

    StringView result = {
            .count = i,
            .data = sv->data
    };

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data += i + 1;
    } else {
        sv->count -= i;
        sv->data += i;
    }

    return result;
}

int sv_eq(StringView a, StringView b) {
    if (a.count != b.count) {
        return 0;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

int sv_to_int(StringView sv) {
    int result = 0;


    for (size_t i = 0; i < sv.count && isdigit(sv.data[i]); i++) {
        result = result * 10 + sv.data[i] - '0';
    }

    return result;
}

char *shift(int *argc, char ***argv) {
    assert(*argc > 0);

    char *result = **argv;
    *argv += 1;
    *argc -= 1;

    return result;
}

/// endregion
#endif
#ifdef BASM_CREATE

StringView basm_slurp_file(Basm *basm, StringView file_path) {
    char *cstr = basm_alloc(basm, file_path.count + 1);
    if (cstr == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for file path: %.*s\n",
                (int) file_path.count,
                file_path.data);
        exit(1);
    }

    memcpy(cstr, file_path.data, file_path.count);
    cstr[file_path.count] = '\0';

    FILE *f = fopen(cstr, "r");

    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not read file '%s' : %s\n", cstr, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0L, SEEK_END) < 0) {
        fprintf(stderr, "ERROR: Could not read file '%.*s' : %s\n",
                (int) file_path.count,
                file_path.data,
                strerror(errno));
        exit(1);
    }

    long m = ftell(f);
    if (m < 0) {
        fprintf(stderr, "ERROR: Could not read file '%s' : %s\n", cstr, strerror(errno));
        exit(1);
    }

    if (fseek(f, 0L, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR: Could not read file '%s' : %s\n", cstr, strerror(errno));
        exit(1);
    }

    char *buffer = basm_alloc(basm, (size_t) m + 1);
    if (buffer == NULL) {
        fclose(f);
        fprintf(stderr, "ERROR: Could not allocate memory for file '%s' : %s\n", cstr,
                strerror(errno));
        exit(1);
    }

    size_t len = fread(buffer, 1, (size_t) m, f);
    buffer[len] = '\0';

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (buffer[i] != '\r') {
            buffer[j] = buffer[i];
            j++;
        }
    }
    buffer[j] = '\0';
    fclose(f);
    return (StringView) {
            .count = j,
            .data = buffer
    };
}

int basm_translate_literal(Basm *basm, StringView sv, Word *output) {
    if (sv.count >= 2 && *sv.data == '"' && sv.data[sv.count - 1] == '"') {
        sv.data += 1;
        sv.count -= 2;
        *output = basm_push_string_to_memory(basm, sv);
    } else {
        char cstr[sv.count + 1];
        char *endptr = 0;
        Word result = {0};

        memcpy(cstr, sv.data, sv.count);
        cstr[sv.count] = '\0';

        result.as_u64 = strtoull(cstr, &endptr, 10);
        if ((size_t) (endptr - cstr) != sv.count) {
            result.as_f64 = strtod(cstr, &endptr);
            if ((size_t) (endptr - cstr) != sv.count) {
                return 0;
            }
        }

        *output = result;
    }
    return 1;
}

void basm_translate_source(StringView input_file_path, Basm *basm) {
    StringView original_source = basm_slurp_file(basm, input_file_path);
    StringView source = original_source;
    StringView entry_label = {0};
    Word entry = {0};
    basm->program_size = 0;
    int line_number = 0;

    // Parse pre-processor directives, instructions and store labels
    while (source.count > 0) {
        StringView line = sv_trim(sv_chop_by_delim(&source, '\n'));
        line = sv_trim(sv_chop_by_delim(&line, BR_ASSEMBLY_COMMENT));
        line_number++;

        if (line.count > 0) {
            StringView token = sv_trim(sv_chop_by_delim(&line, ' '));

            // Pre-processor
            if (token.count > 0 && *token.data == BR_ASSEMBLY_PREPROCESSOR) {
                token.count -= 1;
                token.data += 1;
                if (sv_eq(token, cstr_as_sv("define"))) {
                    line = sv_trim(line);
                    StringView label = sv_chop_by_delim(&line, ' ');
                    if (label.count > 0) {
                        line = sv_trim(line);
                        StringView value = line;
                        Word word = {0};
                        if (!basm_translate_literal(basm, value, &word)) {
                            fprintf(stderr,
                                    "%.*s:%d: ERROR: `%.*s` is not a number\n",
                                    (int) input_file_path.count,
                                    input_file_path.data,
                                    line_number,
                                    (int) value.count,
                                    value.data);
                            exit(1);
                        }

                        if (!basm_bind_label(basm, label, word)) {
                            fprintf(stderr,
                                    "%.*s:%d: ERROR: label `%.*s` is already defined\n",
                                    (int) input_file_path.count,
                                    input_file_path.data,
                                    line_number,
                                    (int) label.count,
                                    label.data);
                            exit(1);
                        }
                    } else {
                        fprintf(stderr, "%.*s:%d: ERROR: Pre-processor name is not provided\n",
                                (int) input_file_path.count,
                                input_file_path.data,
                                line_number);
                        exit(1);
                    }
                } else if (sv_eq(token, cstr_as_sv("include"))) {
                    line = sv_trim(line);
                    if (line.count > 0) {
                        if (*line.data == '"' && line.data[line.count - 1] == '"') {
                            line.data++;
                            line.count -= 2;

                            if (basm->inc_level + 1 >= BR_ASSEMBLY_MAX_INCLUDE_LEVEL) {
                                fprintf(stderr, "%.*s:%d: ERROR: Exceeded maximum include level\n",
                                        (int) input_file_path.count,
                                        input_file_path.data,
                                        line_number);
                                exit(1);
                            }
                            basm->inc_level++;
                            basm_translate_source(line, basm);
                            basm->inc_level--;
                        } else {
                            fprintf(stderr,
                                    "%.*s:%d: ERROR: Pre-processor include path has to be surrounded with quotation marks\n",
                                    (int) input_file_path.count,
                                    input_file_path.data,
                                    line_number);
                            exit(1);
                        }
                    } else {
                        fprintf(stderr,
                                "%.*s:%d: ERROR: Pre-processor include path is not provided\n",
                                (int) input_file_path.count,
                                input_file_path.data,
                                line_number);
                        exit(1);
                    }
                } else if (sv_eq(token, cstr_as_sv("entry"))) {
                    line = sv_trim(line);
                    if (line.count > 0) {
                        if (basm_translate_literal(basm, line, &entry)) {
                            basm->entry = entry.as_u64;
                            basm->has_entry = 1;
                        } else {
                            if (basm_resolve_label(basm, line, &entry)) {
                                basm->entry = entry.as_u64;
                                basm->has_entry = 1;
                            } else {
                                entry_label = line;
                            }
                        }
                    } else {
                        fprintf(stderr, "%.*s:%d: ERROR: Pre-processor entry address is not provided\n",
                                (int) input_file_path.count,
                                input_file_path.data,
                                line_number);
                        exit(1);
                    }
                } else {
                    fprintf(stderr, "%.*s:%d: ERROR: Unknown pre-processor directive '%.*s'\n",
                            (int) input_file_path.count,
                            input_file_path.data,
                            line_number,
                            (int) token.count,
                            token.data);
                    exit(1);
                }
            } else {
                // Label
                if (token.count > 0 && token.data[token.count - 1] == ':') {
                    StringView label = {
                            .count = token.count - 1,
                            .data = token.data
                    };

                    if (!basm_bind_label(basm, label, WORD_U64(basm->program_size))) {
                        fprintf(stderr, "%.*s:%d: ERROR: Label '%.*s' is already defined\n",
                                (int) input_file_path.count,
                                input_file_path.data,
                                line_number,
                                (int) label.count,
                                label.data);
                        exit(1);

                    }

                    token = sv_trim(sv_chop_by_delim(&line, ' '));
                }

                // Instruction
                if (token.count > 0) {
                    StringView operand = line;
                    InstType inst_type = INST_NOP;

                    if (inst_by_name(&token, &inst_type)) {
                        assert(basm->program_size < BR_PROGRAM_CAPACITY);
                        basm->program[basm->program_size].type = inst_type;

                        if (inst_has_operand(inst_type)) {
                            if (operand.count == 0) {
                                fprintf(stderr,
                                        "%.*s:%d: ERROR: Instruction '%.*s' requires an operand\n",
                                        (int) input_file_path.count,
                                        input_file_path.data,
                                        line_number,
                                        (int) token.count,
                                        token.data);
                                exit(1);
                            }

                            if (!basm_translate_literal(basm, operand,
                                                        &basm->program[basm->program_size].operand)) {
                                basm_bind_unresolved(basm, basm->program_size, operand);
                            }
                        }

                        basm->program_size++;
                    } else {
                        fprintf(stderr, "%.*s:%d: ERROR: Unknown instruction '%.*s'\n",
                                (int) input_file_path.count,
                                input_file_path.data,
                                line_number,
                                (int) token.count,
                                token.data);
                        exit(1);
                    }
                }
            }
        }
    }

    // Replace label parameters with actual offset
    for (size_t i = 0; i < basm->unresolved_jmp_size; i++) {
        StringView label = basm->unresolved_jmps[i].label;

        if (!basm_resolve_label(basm, label, &basm->program[basm->unresolved_jmps[i].addr].operand)) {
            fprintf(stderr, "%.*s: ERROR: Unknown label '%.*s'\n",
                    (int) input_file_path.count,
                    input_file_path.data,
                    (int) label.count,
                    label.data);
            exit(1);
        }
    }

    // Replace entry point label with instruction offset
    if (!basm->has_entry && entry_label.count != 0) {
        if (basm_resolve_label(basm, entry_label, &entry)) {
            basm->entry = entry.as_u64;
            basm->has_entry = 1;
        } else {
            fprintf(stderr, "%.*s: ERROR: Unknown label '%.*s'\n",
                    (int) input_file_path.count,
                    input_file_path.data,
                    (int) entry_label.count,
                    entry_label.data);
            exit(1);
        }
    }
}

int basm_resolve_label(const Basm *basm, StringView name, Word *output) {
    for (size_t i = 0; i < basm->labels_size; i++) {
        if (sv_eq(basm->labels[i].name, name)) {
            *output = basm->labels[i].word;
            return 1;
        }
    }

    return 0;
}

int basm_bind_label(Basm *basm, StringView name, Word word) {
    assert(basm->labels_size < BR_LABEL_CAPACITY);

    Word ignore = {0};
    if (basm_resolve_label(basm, name, &ignore)) {
        return 0;
    }

    basm->labels[basm->labels_size++] = (Label) {.name = name, .word = word};
    return 1;
}

void basm_bind_unresolved(Basm *basm, InstAddr addr, StringView label) {
    assert(basm->unresolved_jmp_size < BR_LABEL_CAPACITY);
    basm->unresolved_jmps[basm->unresolved_jmp_size++] = (UnresolvedJmp) {.addr = addr, .label = label};
}

size_t basm_save_to_file(Basm *basm, const char *file_path) {
    FILE *f = fopen(file_path, "wb");
    size_t written_size = 0;
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file '%s' : %s\n", file_path, strerror(errno));
        exit(1);
    }

    BasmFileMeta meta = {
            .magic = BR_FILE_MAGIC,
            .version = BR_FILE_VERSION,
            .program_size = basm->program_size,
            .memory_size = basm->memory_size,
            .memory_capacity = basm->memory_capacity,
            .entry = basm->entry
    };

    written_size += fwrite(&meta, sizeof(meta), 1, f) * sizeof(meta);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file '%s' : %s\n", file_path, strerror(errno));
        exit(1);
    }

    written_size += fwrite(basm->program, sizeof(basm->program[0]), basm->program_size, f) * sizeof(basm->program[0]);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file '%s' : %s\n", file_path, strerror(errno));
        exit(1);
    }

    written_size += fwrite(basm->memory, sizeof(basm->memory[0]), basm->memory_size, f) * sizeof(basm->memory[0]);
    if (ferror(f)) {
        fprintf(stderr, "ERROR: Could not write to file '%s' : %s\n", file_path, strerror(errno));
        exit(1);
    }

    fclose(f);
    return written_size;
}

#endif
#ifdef BASM_VM

const char *err_as_cstr(Err err) {
    switch (err) {
        case ERR_OK:
            return "ERR_OK";
        case ERR_STACK_OVERFLOW:
            return "ERR_STACK_OVERFLOW";
        case ERR_STACK_UNDERFLOW:
            return "ERR_STACK_UNDERFLOW";
        case ERR_ILLEGAL_INS:
            return "ERR_ILLEGAL_INS";
        case ERR_DIV_BY_ZERO:
            return "ERR_DIV_BY_ZERO";
        case ERR_ILLEGAL_INS_ACCESS:
            return "ERR_ILLEGAL_INS_ACCESS";
        case ERR_ILLEGAL_OPERAND:
            return "ERR_ILLEGAL_OPERAND";
        case ERR_ILLEGAL_MEMORY_ACCESS:
            return "ERR_ILLEGAL_MEMORY_ACCESS";
        default:
            assert(0 && "err_as_cstr: Unreachable");
            break;
    }
}

void br_load_program_from_file(ByteRunner *br, const char *file_path) {
    FILE *f = fopen(file_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: Could not open file '%s' : %s\n", file_path, strerror(errno));
        exit(1);
    }

    BasmFileMeta meta = {0};
    size_t n = fread(&meta, sizeof(meta), 1, f);
    if (n < 1) {
        fprintf(stderr, "ERROR: Could not read meta data from file '%s': %s\n", file_path, strerror(errno));
        exit(1);
    }

    if (meta.magic != BR_FILE_MAGIC) {
        fprintf(stderr, "ERROR: '%s' does not appear to be a valid file. "
                        "Unexpected magic %04X. Expected %04X\n", file_path, meta.magic, BR_FILE_MAGIC);
        exit(1);
    }

    if (meta.version != BR_FILE_VERSION) {
        fprintf(stderr, "ERROR: Unsupported version %d, Expected version %d\n", meta.version, BR_FILE_VERSION);
        exit(1);
    }

    if (meta.program_size > BR_PROGRAM_CAPACITY) {
        fprintf(stderr,
                "ERROR: '%s': program section is too large. The file contains %" PRIu64 " program instructions. But the capacity is %"  PRIu64 "\n",
                file_path, meta.program_size, (uint64_t) BR_PROGRAM_CAPACITY);
        exit(1);
    }

    br->ip = meta.entry;

    if (meta.memory_capacity > BR_MEMORY_CAPACITY) {
        fprintf(stderr,
                "ERROR: '%s': memory section is too large. The file wants %" PRIu64 "bytes. But the capacity is %"  PRIu64 "bytes \n",
                file_path, meta.memory_capacity, (uint64_t) BR_MEMORY_CAPACITY);
        exit(1);
    }

    if (meta.memory_size > meta.memory_capacity) {
        fprintf(stderr,
                "ERROR: '%s': memory size %" PRIu64 " is greater than the declared memory capacity %" PRIu64 "\n",
                file_path, meta.memory_size, meta.memory_capacity);
        exit(1);
    }

    br->program_size = fread(br->program, sizeof(br->program[0]), meta.program_size, f);

    if (br->program_size != meta.program_size) {
        fprintf(stderr, "ERROR: '%s', read %zd program instructions, but expected %" PRIu64 "\n",
                file_path, br->program_size, meta.program_size);
    }

    n = fread(br->memory, sizeof(br->memory[0]), meta.memory_size, f);

    if (n != meta.memory_size) {
        fprintf(stderr, "ERROR: '%s', read %zd bytes of memory section, but expected %" PRIu64 " bytes\n",
                file_path, n, meta.memory_size);
    }


    fclose(f);
}

void br_push_native(ByteRunner *br, Br_Native native) {
    assert(br->natives_size < BR_NATIVE_CAPACITY);
    br->natives[br->natives_size++] = native;
}

void br_dump_stack(FILE *stream, const ByteRunner *br) {
    fprintf(stream, "Stack:\n");
    if (br->stack_size > 0) {
        for (InstAddr i = 0; i < br->stack_size; i++) {
            fprintf(stream, "  u64: %-20" PRIu64 "|   i64: %-20" PRId64 "|   f64: %-25lf|   ptr: %-20p\n",
                    br->stack[i].as_u64,
                    br->stack[i].as_i64,
                    br->stack[i].as_f64,
                    br->stack[i].as_ptr
            );
        }
    } else {
        fprintf(stream, "  [EMPTY]\n");
    }

}

Err br_execute_program(ByteRunner *br, int limit) {
    while (limit != 0 && !br->halt) {
        Err err = br_execute_inst(br);
        if (err != ERR_OK) {
            return err;
        }

        if (limit > 0) {
            limit--;
        }
    }

    return ERR_OK;
}

Err br_execute_inst(ByteRunner *br) {
    if (br->ip >= br->program_size) {
        return ERR_ILLEGAL_INS_ACCESS;
    }

    Inst inst = br->program[br->ip];

    switch (inst.type) {
        case INST_NOP:
            br->ip++;
            break;
        case INST_PUSH:
            if (br->stack_size >= BR_STACK_CAPACITY) {
                return ERR_STACK_OVERFLOW;
            }
            br->stack[br->stack_size++] = inst.operand;
            br->ip++;
            break;
        case INST_POP:
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }

            br->stack_size--;
            br->ip++;
            break;
        case INST_NOTB:
            if (br->stack_size - inst.operand.as_u64 < 1) {
                return ERR_STACK_UNDERFLOW;
            }

            br->stack[br->stack_size - 1].as_u64 = ~br->stack[br->stack_size - 1].as_u64;
            br->ip++;
            break;
        case INST_ANDB: BINARY_OP(br, u64, u64, &)
        case INST_ORB: BINARY_OP(br, u64, u64, |)
        case INST_XOR: BINARY_OP(br, u64, u64, ^)
        case INST_SHR: BINARY_OP(br, u64, u64, >>)
        case INST_SHL: BINARY_OP(br, u64, u64, <<)
        case INST_PLUSI: BINARY_OP(br, u64, u64, +)
        case INST_MINUSI: BINARY_OP(br, u64, u64, -)
        case INST_MULTI: BINARY_OP(br, u64, u64, *)
        case INST_DIVI:
            if (br->stack[br->stack_size - 1].as_u64 == 0) {
                return ERR_DIV_BY_ZERO;
            }
            BINARY_OP(br, u64, u64, /)
        case INST_MODI: BINARY_OP(br, u64, u64, %)
        case INST_EQI: BINARY_OP(br, u64, u64, ==)
        case INST_GEI: BINARY_OP(br, u64, u64, >=)
        case INST_GI: BINARY_OP(br, u64, u64, >)
        case INST_LEI: BINARY_OP(br, u64, u64, <=)
        case INST_LI: BINARY_OP(br, u64, u64, <)
        case INST_NEI: BINARY_OP(br, u64, u64, !=)
        case INST_PLUSF: BINARY_OP(br, f64, f64, +)
        case INST_MINUSF: BINARY_OP(br, f64, f64, -)
        case INST_MULTF: BINARY_OP(br, f64, f64, *)
        case INST_DIVF: BINARY_OP(br, f64, f64, /)
        case INST_EQF: BINARY_OP(br, f64, u64, ==)
        case INST_GEF: BINARY_OP(br, f64, u64, >=)
        case INST_GF: BINARY_OP(br, f64, u64, >)
        case INST_LEF: BINARY_OP(br, f64, u64, <=)
        case INST_LF: BINARY_OP(br, f64, u64, <)
        case INST_NEF: BINARY_OP(br, f64, u64, !=)
        case INST_CALL:
            if (br->stack_size >= BR_STACK_CAPACITY) {
                return ERR_STACK_OVERFLOW;
            }

            br->stack[br->stack_size++].as_u64 = br->ip;
            br->ip = inst.operand.as_u64;
            break;
        case INST_INT:
            if (inst.operand.as_u64 > br->natives_size) {
                return ERR_ILLEGAL_OPERAND;
            }
            br->natives[inst.operand.as_u64](br);
            br->ip++;
            break;
        case INST_JMP:
            br->ip = inst.operand.as_u64;
            break;
        case INST_JMP_IF:
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }

            if (br->stack[br->stack_size - 1].as_u64) {
                br->ip = inst.operand.as_u64;
            } else {
                br->ip++;
            }
            br->stack_size--;

            break;
        case INST_RET:
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }

            br->ip = br->stack[br->stack_size - 1].as_u64 + 1;
            br->stack_size--;
            break;
        case INST_NOT:
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }

            br->stack[br->stack_size - 1].as_u64 = !br->stack[br->stack_size - 1].as_u64;
            br->ip++;
            break;
        case INST_DUP:
            if (br->stack_size >= BR_STACK_CAPACITY) {
                return ERR_STACK_OVERFLOW;
            }

            if (br->stack_size - inst.operand.as_u64 <= 0) {
                return ERR_STACK_UNDERFLOW;
            }

            br->stack[br->stack_size] = br->stack[br->stack_size - 1 - inst.operand.as_u64];
            br->stack_size++;
            br->ip++;
            break;
        case INST_SWAP:
            if (inst.operand.as_u64 >= br->stack_size) {
                return ERR_STACK_UNDERFLOW;
            }

            const uint64_t a = br->stack_size - 1;
            const uint64_t b = br->stack_size - 1 - inst.operand.as_u64;
            Word t = br->stack[a];
            br->stack[a] = br->stack[b];
            br->stack[b] = t;
            br->ip++;
            break;
        case INST_HALT:
            br->halt = 1;
            break;
        case INST_READ8: {
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }
            MemoryAddr addr = br->stack[br->stack_size - 1].as_u64;
            if (addr >= BR_MEMORY_CAPACITY) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            br->stack[br->stack_size - 1].as_u64 = br->memory[addr];
            br->ip++;
            break;
        }
        case INST_READ16: {
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }
            MemoryAddr addr = br->stack[br->stack_size - 1].as_u64;
            if (addr >= BR_MEMORY_CAPACITY - 1) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            br->stack[br->stack_size - 1].as_u64 = *(uint16_t *) &br->memory[addr];
            br->ip++;
            break;
        }
        case INST_READ32: {
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }
            MemoryAddr addr = br->stack[br->stack_size - 1].as_u64;
            if (addr >= BR_MEMORY_CAPACITY - 3) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            br->stack[br->stack_size - 1].as_u64 = *(uint32_t *) &br->memory[addr];
            br->ip++;
            break;
        }
        case INST_READ64: {
            if (br->stack_size < 1) {
                return ERR_STACK_UNDERFLOW;
            }
            MemoryAddr addr = br->stack[br->stack_size - 1].as_u64;
            if (addr >= BR_MEMORY_CAPACITY - 7) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            br->stack[br->stack_size - 1].as_u64 = *(uint64_t *) &br->memory[addr];
            br->ip++;
            break;
        }
        case INST_WRITE8: {
            if (br->stack_size < 2) {
                return ERR_STACK_UNDERFLOW;
            }

            const MemoryAddr addr = br->stack[br->stack_size - 2].as_u64;
            if (addr >= BR_MEMORY_CAPACITY) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }

            br->memory[addr] = (uint8_t) br->stack[br->stack_size - 1].as_u64;
            br->stack_size -= 2;
            br->ip++;
            break;
        }
        case INST_WRITE16: {
            if (br->stack_size < 2) {
                return ERR_STACK_UNDERFLOW;
            }

            const MemoryAddr addr = br->stack[br->stack_size - 2].as_u64;
            if (addr >= BR_MEMORY_CAPACITY) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }

            *(uint16_t *) &br->memory[addr] = (uint16_t) br->stack[br->stack_size - 1].as_u64;
            br->stack_size -= 2;
            br->ip++;
            break;
        }
        case INST_WRITE32: {
            if (br->stack_size < 2) {
                return ERR_STACK_UNDERFLOW;
            }

            const MemoryAddr addr = br->stack[br->stack_size - 2].as_u64;
            if (addr >= BR_MEMORY_CAPACITY) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }

            *(uint32_t *) &br->memory[addr] = (uint32_t) br->stack[br->stack_size - 1].as_u64;
            br->stack_size -= 2;
            br->ip++;
            break;
        }
        case INST_WRITE64: {
            if (br->stack_size < 2) {
                return ERR_STACK_UNDERFLOW;
            }

            const MemoryAddr addr = br->stack[br->stack_size - 2].as_u64;
            if (addr >= BR_MEMORY_CAPACITY) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }

            *(uint64_t *) &br->memory[addr] = br->stack[br->stack_size - 1].as_u64;
            br->stack_size -= 2;
            br->ip++;
            break;
        }
        case INST_I2F: CAST_OP(br, i64, f64, (double))
        case INST_I2U: CAST_OP(br, i64, u64, (uint64_t))
        case INST_U2F: CAST_OP(br, u64, f64, (double))
        case INST_U2I: CAST_OP(br, u64, i64, (int32_t))
        case INST_F2I: CAST_OP(br, f64, i64, (int64_t))
        case INST_F2U: CAST_OP(br, f64, u64, (uint64_t) (int64_t))
        case SIZE:
        default:
            return ERR_ILLEGAL_INS;
    }

    return ERR_OK;
}

#endif
