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
    {"ADD", 0x33, 0x0, 0x00}, // Addition
    {"SUB", 0x33, 0x0, 0x20}, // Subtraction
    {"SLL", 0x33, 0x1, 0x00}, // Shift Left Logical
    {"XOR", 0x33, 0x4, 0x00}, // XOR (Exclusive OR)
    {"SRL", 0x33, 0x5, 0x00}, // Shift Right Logical
    {"SRA", 0x33, 0x5, 0x20}, // Shift Right Arithmetic
    {"OR", 0x33, 0x6, 0x00}, // OR (Logical OR)
    {"AND", 0x33, 0x7, 0x00} // AND (Logical AND)
};

I_Instruction i_instructions[] = {
    {"ADDI", 0x13, 0x0}, // Add Immediate
    {"XORI", 0x13, 0x4}, // XOR Immediate
    {"ORI", 0x13, 0x6}, // OR Immediate
    {"ANDI", 0x13, 0x7}, // AND Immediate

    // TODO: SLLI, SRLI, SRAI instruction have func7...?
    {"SLLI", 0x13, 0x1}, // Shift Left Logical Immediate
    {"SRLI", 0x13, 0x5}, // Shift Right Logical Immediate
    {"SRAI", 0x13, 0x5}, // Shift Right Arithmetic Immediate

    {"LW", 0x03, 0x2}, // Load Word
    {"JALR", 0x67, 0x0} // Jump And Link Register
};

S_Instruction s_instructions = {"SW", 0x23, 0x2}; // Store Word

SB_Instruction sb_instructions[] = {
    {"BEQ", 0x63, 0x0}, // Branch if Equal
    {"BNE", 0x63, 0x1}, // Branch if Not Equal
    {"BLT", 0x63, 0x4}, // Branch if Less Than
    {"BGE", 0x63, 0x5} // Branch if Greater or Equal
};

UJ_Instruction uj_instructions = {"JAL", 0x6F}; // Jump and Link

R_Instruction *find_r_instruction(const char *name) {
    for (int i = 0; i < sizeof(r_instructions) / sizeof(R_Instruction); i++) {
        if (strcasecmp(name, r_instructions[i].name) == 0) {
            return &r_instructions[i];
        }
    }
    return NULL;
}

I_Instruction *find_i_instruction(const char *name) {
    for (int i = 0; i < sizeof(i_instructions) / sizeof(I_Instruction); i++) {
        if (strcasecmp(name, i_instructions[i].name) == 0) {
            return &i_instructions[i];
        }
    }
    return NULL;
}

S_Instruction *find_s_instruction(const char *name) {
    return &s_instructions;
}

SB_Instruction *find_sb_instruction(const char *name) {
    for (int i = 0; i < sizeof(sb_instructions) / sizeof(SB_Instruction); i++) {
        if (strcasecmp(name, sb_instructions[i].name) == 0) {
            return &sb_instructions[i];
        }
    }
    return NULL;
}

UJ_Instruction *find_uj_instruction(const char *name) {
    return &uj_instructions;
}

// Encode R-type instruction
unsigned int encode_r_type(const unsigned int funct7, const unsigned int rs2, const unsigned int rs1,
                           const unsigned int funct3,
                           const unsigned int rd, const unsigned int opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

// Encode I-type instruction
unsigned int encode_i_type(const unsigned int imm, const unsigned int rs1, const unsigned int funct3,
                           const unsigned int rd, const unsigned int opcode) {
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

// S type 명령어에서 imm를 분리
void parse_imm_for_s_type_inst(const unsigned int imm, unsigned int *imm1, unsigned int *imm2) {
    // imm1에는 상위 비트 imm[11:5] 저장
    *imm1 = (imm >> 5) & 0x7F; // 0x7F는 7비트 마스크로, imm[11:5] 추출

    // imm2에는 하위 비트 imm[4:0] 저장
    *imm2 = imm & 0x1F; // 0x1F는 5비트 마스크로, imm[4:0] 추출
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
    char filename_without_extension[MAX_LINE_LENGTH];
    sscanf(filename, "%[^.]", filename_without_extension);
    snprintf(output_file, sizeof(output_file), "%s.o", filename_without_extension);
    snprintf(trace_file, sizeof(trace_file), "%s.trace", filename_without_extension);

    FILE *output = fopen(output_file, "w");
    FILE *trace = fopen(trace_file, "w");

    char line[MAX_LINE_LENGTH];
    unsigned int pc = STARTING_PC;

    while (fgets(line, sizeof(line), input_file)) {
        char instruction_name[10];
        unsigned int rd = 0, rs1 = 0, rs2 = 0, imm = 0;
        unsigned int machine_code = 0;

        // operation rd, rs1, rs2 format instruction -> R type
        if (sscanf(line, "%s x%u, x%u, x%u", instruction_name, &rd, &rs1, &rs2) == 4) {
            const R_Instruction *r_instr = find_r_instruction(instruction_name);
            machine_code = encode_r_type(r_instr->funct7, rs2, rs1, r_instr->funct3, rd, r_instr->opcode);
        }

        // operation rd, rs1, imm12 format instruction -> I type with immediate instruction
        else if (sscanf(line, "%s x%u, x%u, %u", instruction_name, &rd, &rs1, &imm) == 4) {
            const I_Instruction *i_instr = find_i_instruction(instruction_name);
            machine_code = encode_i_type(imm, rs1, i_instr->funct3, rd, i_instr->opcode);
        }

        // operation rd, imm12(rs1) format instruction -> I type with load instruction
        // operation rs2, imm12(rs1) format instruction -> S type with store instruction
        else if (sscanf(line, "%s x%u, %u(x%u)", instruction_name, &rd, &imm, &rs1) == 4) {
            int result = strcasecmp(instruction_name, "LW"); // compare does instruction is LW

            if (result == 0) {
                // LW case
                const I_Instruction *i_instr = find_i_instruction(instruction_name); // return only LW instruction
                machine_code = encode_i_type(imm, rs1, i_instr->funct3, rd, i_instr->opcode);
            } else {
                // SW case
                const S_Instruction *s_instr = find_s_instruction(instruction_name); // return only SW instruction

                unsigned int imm1; // imm[11:5]
                unsigned int imm2; // imm[4:0]

                parse_imm_for_s_type_inst(imm, &imm1, &imm2); // parse imm into two individual imm variables

                rs2 = rd; // SW instruction doesn't use rd

                machine_code = encode_s_type(imm1, rs2, rs1, s_instr->funct3, imm2, s_instr->opcode);
            }
        }

        // operation rs1, rs2, imm12 format instruction -> SB type
        else if (sscanf(line, "%s x%u, x%u, &imm", instruction_name, &rs1, &rs2, &imm) == 4) {
            const SB_Instruction *sb_instr = find_sb_instruction(instruction_name);
            // TODO: imm를 이진수로 표현했을 때, imm를 imm1 & imm2로 분리하는 코드를 작성
            // TODO: 라벨로 오는 항목들을 어떻게 명령어로 변환할 지 연구
        }

        // operation rd, imm20 format instruction -> UJ type
        else if (sscanf(line, "%s x%u, x%u", instruction_name, &rd, &imm) == 4) {
            const UJ_Instruction *uj_instr = find_uj_instruction(instruction_name);
            machine_code = encode_uj_type(imm, rd, uj_instr->opcode);
        } else {
            // TODO: 잘못된 형식의 명령어를 읽어올 때 처리
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
