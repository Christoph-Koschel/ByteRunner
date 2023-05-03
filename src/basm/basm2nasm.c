#define BASM_UTILS
#define BASM_CREATE

#include "libbasm.h"
#include <inttypes.h>

static void usage(FILE *stream) {
    fprintf(stream, "Usage: basm2nasm <input.basm>\n");
}

Basm basm = {0};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(stderr);
        fprintf(stderr, "ERROR: No input provided\n");
        return 1;
    }

    basm_translate_source(cstr_as_sv(argv[1]), &basm);

    printf("bits 64\n\n");
    printf("%%define STDOUT 1\n");
    printf("%%define SYS_WRITE 1\n");
    printf("%%define SYS_EXIT 60\n");
    printf("%%define BR_STACK_CAPACITY %d\n", BR_STACK_CAPACITY);
    printf("%%define BR_WORD_SIZE %d\n", BR_WORD_SIZE);
    printf("\nsegment .text\n");
    printf("global _start\n\n");
    printf("print_i64:\n");
    printf("    ;; extracting input from the BM's stack\n");
    printf("    mov rsi, [stack_top]\n");
    printf("    sub rsi, BR_WORD_SIZE\n");
    printf("    mov rax, [rsi]\n");
    printf("    mov [stack_top], rsi\n");
    printf("    ;; rax contains the value we need to print\n");
    printf("    ;; rdi is the counter of chars\n");
    printf("    mov rdi, 0\n");
    printf("    ;; adding the new line\n");
    printf("    dec rsp\n");
    printf("    inc rdi\n");
    printf("    mov BYTE [rsp], 10\n");
    printf(".loop:\n");
    printf("    xor rdx, rdx\n");
    printf("    mov rbx, 10\n");
    printf("    div rbx\n");
    printf("    add rdx, '0'\n");
    printf("    dec rsp\n");
    printf("    inc rdi\n");
    printf("    mov [rsp], dl\n");
    printf("    cmp rax, 0\n");
    printf("    jne .loop\n");
    printf("    ;; rsp - points at the beginning of the buf\n");
    printf("    ;; rdi - contains the size of the buf\n");
    printf("    ;; printing the buffer\n");
    printf("    mov rbx, rdi\n");
    printf("    ;; write(STDOUT, buf, buf_size)\n");
    printf("    mov rax, SYS_WRITE\n");
    printf("    mov rdi, STDOUT\n");
    printf("    mov rsi, rsp\n");
    printf("    mov rdx, rbx\n");
    printf("    syscall\n");
    printf("    add rsp, rbx\n");
    printf("    ret\n");
    printf("\n_start:\n");

    size_t jmp_if_escape_count = 0;

    for (size_t i = 0; i < basm.program_size; i++) {
        Inst inst = basm.program[i];
        printf("inst_%zu:\n", i);
        switch (inst.type) {
            case INST_NOP:
                break;
            case INST_DUP:
                printf("    ;; dup %"PRIu64"\n", inst.operand.as_u64);
                printf("    mov rsi, [stack_top]\n");
                printf("    mov rdi, rsi\n");
                printf("    sub rdi, BR_WORD_SIZE * (%"PRIu64" + 1)\n", inst.operand.as_u64);
                printf("    mov rax, [rdi]\n");
                printf("    mov [rsi], rax\n");
                printf("    add rsi, BR_WORD_SIZE\n");
                printf("    mov [stack_top], rsi\n");
                break;
            case INST_SWAP:
                printf("    ;; swap %"PRIu64"\n", inst.operand.as_u64);
                printf("    mov rsi, [stack_top]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rdi, rsi\n");
                printf("    sub rdi, BR_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
                printf("    mov rax, [rsi]\n");
                printf("    mov rbx, [rdi]\n");
                printf("    mov [rdi], rax\n");
                printf("    mov [rsi], rbx\n");
                break;
            case INST_PUSH:
                printf("    ;; push %"PRIu64"\n", inst.operand.as_u64);
                printf("    mov rsi, [stack_top]\n");
                printf("    mov QWORD [rsi], %"PRIu64"\n", inst.operand.as_u64);
                printf("    add QWORD [stack_top], BR_WORD_SIZE\n");
                break;
            case INST_POP:
                break;
            case INST_PLUSI:
                printf("    ;; plusi\n");
                printf("    mov rsi, [stack_top]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rbx, [rsi]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rax, [rsi]\n");
                printf("    add rax, rbx\n");
                printf("    mov [rsi], rax\n");
                printf("    add rsi, BR_WORD_SIZE\n");
                printf("    mov [stack_top], rsi\n");
                break;
            case INST_MINUSI:
                printf("    ;; minusi\n");
                printf("    mov rsi, [stack_top]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rbx, [rsi]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rax, [rsi]\n");
                printf("    sub rax, rbx\n");
                printf("    mov [rsi], rax\n");
                printf("    add rsi, BR_WORD_SIZE\n");
                printf("    mov [stack_top], rsi\n");
                break;
            case INST_MULTI:
                break;
            case INST_DIVI:
                break;
            case INST_MODI:
                break;
            case INST_GEI:
                break;
            case INST_LEI:
                break;
            case INST_LI:
                break;
            case INST_NEI:
                break;
            case INST_GI:
                break;
            case INST_EQI:
                printf("    ;; eqi\n");
                printf("    mov rsi, [stack_top]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rbx, [rsi]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rax, [rsi]\n");
                printf("    cmp rax, rbx\n");
                printf("    mov rax, 0\n");
                printf("    setz al\n");
                printf("    mov [rsi], rax\n");
                printf("    add rsi, BR_WORD_SIZE\n");
                printf("    mov [stack_top], rsi\n");
                break;
            case INST_PLUSF:
                break;
            case INST_MINUSF:
                break;
            case INST_MULTF:
                break;
            case INST_DIVF:
                break;
            case INST_GEF:
                break;
            case INST_GF:
                break;
            case INST_LEF:
                break;
            case INST_LF:
                break;
            case INST_NEF:
                break;
            case INST_EQF:
                break;
            case INST_ANDB:
                break;
            case INST_ORB:
                break;
            case INST_XOR:
                break;
            case INST_SHR:
                break;
            case INST_SHL:
                break;
            case INST_NOTB:
                break;
            case INST_CALL:
                break;
            case INST_INT:
                if (inst.operand.as_u64 == 3) {
                    printf("    ; -- int --\n");
                    printf("    call print_i64\n");
                }
                break;
            case INST_JMP:
                break;
            case INST_JMP_IF:
                printf("    ;; jmp_if %"PRIu64"\n", inst.operand.as_u64);
                printf("    mov rsi, [stack_top]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rax, [rsi]\n");
                printf("    mov [stack_top], rsi\n");
                printf("    cmp rax, 0\n");
                printf("    je jmp_if_escape_%zu\n", jmp_if_escape_count);
                printf("    mov rdi, inst_map\n");
                printf("    add rdi, BR_WORD_SIZE * %"PRIu64"\n", inst.operand.as_u64);
                printf("    jmp [rdi]\n");
                printf("jmp_if_escape_%zu:\n", jmp_if_escape_count);
                jmp_if_escape_count += 1;
                break;
            case INST_RET:
                break;
            case INST_READ8:
                break;
            case INST_READ16:
                break;
            case INST_READ32:
                break;
            case INST_READ64:
                break;
            case INST_WRITE8:
                break;
            case INST_WRITE16:
                break;
            case INST_WRITE32:
                break;
            case INST_WRITE64:
                break;
            case INST_I2F:
                break;
            case INST_I2U:
                break;
            case INST_U2F:
                break;
            case INST_U2I:
                break;
            case INST_F2I:
                break;
            case INST_F2U:
                break;
            case INST_NOT:
                printf("    ;; not\n");
                printf("    mov rsi, [stack_top]\n");
                printf("    sub rsi, BR_WORD_SIZE\n");
                printf("    mov rax, [rsi]\n");
                printf("    cmp rax, 0\n");
                printf("    mov rax, 0\n");
                printf("    setz al\n");
                printf("    mov [rsi], rax\n");
                break;
            case INST_HALT:
                printf("    ;; halt\n");
                printf("    mov rax, SYS_EXIT\n");
                printf("    mov rdi, 0\n");
                printf("    syscall\n");
                break;
            case SIZE:
            default:
                assert(0 && "Unknown instruction");
                break;
        }
    }

    printf("    ret\n");
    printf("\nsegment .bss\n");
    printf("stack: resq BR_STACK_CAPACITY\n");
    printf("\nsegment .data\n");
    printf("stack_top: dq stack\n");
    printf("inst_map: dq");
    for (size_t i = 0; i < basm.program_size; i++) {
        printf(" inst_%zu,", i);
    }
    printf("\n");

    return 0;
}
