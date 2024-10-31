#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 사용자에게서 입력받는 파일이름 크기 최댓값
#define MAX_LINE_LENGTH 100

// 시작 PC 주소
#define STARTING_PC 1000

// 종료 기계어 명령어
#define EXIT_CODE 0xFFFFFFFF

typedef struct {
    char name[10];
    unsigned int opcode;
    unsigned int funct3;
    unsigned int funct7;
} R_Instruction;

typedef struct {
    char name[10];
    unsigned int opcode;
    unsigned int funct3;
} I_Instruction;

typedef struct {
    char name[10];
    unsigned int opcode;
    unsigned int funct3;
} S_Instruction;

typedef struct {
    char name[10];
    unsigned int opcode;
    unsigned int funct3;
} SB_Instruction;

typedef struct {
    char name[10];
    unsigned int opcode;
} UJ_Instruction;

R_Instruction r_instructions[] = {
    {"ADD", 0x33, 0x0, 0x00},
    {"SUB", 0x33, 0x0, 0x20},
    {"SLL", 0x33, 0x1, 0x00},
    {"XOR", 0x33, 0x4, 0x00},
    {"SRL", 0x33, 0x5, 0x00},
    {"SRA", 0x33, 0x5, 0x20},
    {"OR", 0x33, 0x6, 0x00},
    {"AND", 0x33, 0x7, 0x00}
};

// Utility function to find the instruction by name
R_Instruction *find_instruction(const char *name) {
    for (int i = 0; i < sizeof(r_instructions) / sizeof(R_Instruction); i++) {
        if (strcasecmp(name, r_instructions[i].name) == 0) {
            return &r_instructions[i];
        }
    }
    return NULL;
}

// Encode R-type instruction
unsigned int encode_r_type(const unsigned int funct7, const unsigned int rs2, const unsigned int rs1,
                           const unsigned int funct3,
                           const unsigned int rd, const unsigned int opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

// Encode I-type instruction
unsigned int encode_i_type(const unsigned int imm, const unsigned int rs1, const unsigned int funct3,
                           const unsigned int rd, const int opcode) {
    return (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

// Encode S-type instruction
unsigned int encode_s_type(const unsigned int imm2, const unsigned int rs2, const unsigned int rs1,
                           const unsigned int funct3,
                           const unsigned int imm1, const unsigned int opcode) {
    return (imm2 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm1 << 7) | opcode;
}

// Encode SB-type instruction
unsigned int encode_sb_type(const unsigned int imm2, const unsigned int rs2, const unsigned int rs1,
                            const unsigned int funct3,
                            const unsigned int imm1, const unsigned int opcode) {
    return (imm2 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm1 << 7) | opcode;
}

// Encode UJ-type instruction
unsigned int encode_uj_type(const unsigned int imm, const unsigned int rd, const unsigned int opcode) {
    return (imm << 12) | (rd << 7) | opcode;
}

// Binary instruction으로 파일에 쓰기 위함
void print_binary_to_file(unsigned int n, FILE *file) {
    int bits = sizeof(n) * 8; // unsigned int의 비트 수 (32비트 환경에서는 4바이트 * 8 = 32비트)

    for (int i = bits - 1; i >= 0; i--) {
        // 가장 왼쪽 비트부터 검사
        unsigned int mask = 1 << i; // 왼쪽으로 i만큼 시프트하여 해당 비트에 1을 위치시킴
        fprintf(file, "%d", (n & mask) ? 1 : 0); // 파일에 해당 비트가 1인지 0인지 출력
    }
    fprintf(file, "\n");
}

// Add other encoding functions for S-type, SB-type, U-type, and UJ-type
void process_file(const char *filename) {
    FILE *input_file = fopen(filename, "r");
    if (!input_file) {
        printf("Input file does not exist!!\n");
        return;
    }

    char output_file[MAX_LINE_LENGTH];
    char trace_file[MAX_LINE_LENGTH];
    snprintf(output_file, sizeof(output_file), "%s.o", filename);
    snprintf(trace_file, sizeof(trace_file), "%s.trace", filename);

    FILE *output = fopen(output_file, "w");
    FILE *trace = fopen(trace_file, "w");

    char line[MAX_LINE_LENGTH];
    unsigned int pc = STARTING_PC;

    while (fgets(line, sizeof(line), input_file)) {
        char instruction_name[10];
        unsigned int rd, rs1, rs2;
        sscanf(line, "%s x%u, x%u, x%u", instruction_name, &rd, &rs1, &rs2);
        R_Instruction *r_instr = find_instruction(instruction_name);

        if (r_instr == NULL) {
            printf("Syntax Error!!\n");
            fclose(output);
            fclose(trace);
            fclose(input_file);
            return;
        }

        unsigned int machine_code = 0;

        // Determine the instruction type and encode accordingly
        if (strcasecmp(instruction_name, "ADD") == 0
            || strcasecmp(instruction_name, "SUB") == 0
            || strcasecmp(instruction_name, "SLL") == 0
            || strcasecmp(instruction_name, "XOR") == 0
            || strcasecmp(instruction_name, "SRL") == 0
            || strcasecmp(instruction_name, "SRA") == 0
            || strcasecmp(instruction_name, "OR") == 0
            || strcasecmp(instruction_name, "AND") == 0) {
            // Case for R-type
            machine_code = encode_r_type(
                r_instr->funct7,
                rs2,
                rs1,
                r_instr->funct3,
                rd,
                r_instr->opcode);
        }
        // TODO: Add cases for other types: I_TYPE, S_TYPE, SB_TYPE, U_TYPE, UJ_TYPE

        // TODO: Handle EXIT instruction
        if (strcmp(instruction_name, "EXIT") == 0) {
            machine_code = EXIT_CODE;
        }

        // Write machine code and PC to files
        print_binary_to_file(machine_code, output);
        fprintf(trace, "%u\n", pc);

        pc += 4; // Increment PC by 4 for each instruction
    }

    fclose(input_file);
    fclose(output);
    fclose(trace);

    // FIXME: 과제 제출 시 해당 코드 제거 요망, 개발 간 확인용
    printf("Files %s and %s generated successfully.\n", output_file, trace_file);
}

int main() {
    char filename[MAX_LINE_LENGTH];
    printf("Enter Input File Name: ");
    scanf("%s", filename);

    process_file(filename);

    return 0;
}
