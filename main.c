#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 50 // 사용자에게서 입력받는 파일이름 크기 최댓값

#define STARTING_PC 1000 // 시작 PC 주소

#define EXIT_CODE 0xFFFFFFFF // 종료 기계어 명령어

#define MAX_LINE_COUNT 5000 // 가능한 레이블 개수 최댓값

typedef struct {
    char name[10];
    int opcode;
    int funct3;
    int funct7;
} R_Instruction;

typedef struct {
    char name[10];
    int opcode;
    int funct3;
    int funct7; // funct7 추가
} I_Instruction;

typedef struct {
    char name[10];
    int opcode;
    int funct3;
} S_Instruction;

typedef struct {
    char name[10];
    int opcode;
    int funct3;
} SB_Instruction;

typedef struct {
    char name[10];
    int opcode;
} UJ_Instruction;

typedef struct {
    char name[MAX_LINE_LENGTH];
    int pc_address;
    int instruction_index;
} Label;

Label labels[100]; // 레이블 저장 배열

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
    {"ADDI", 0x13, 0x0, 0}, // Add Immediate
    {"XORI", 0x13, 0x4, 0}, // XOR Immediate
    {"ORI", 0x13, 0x6, 0}, // OR Immediate
    {"ANDI", 0x13, 0x7, 0}, // AND Immediate
    {"SLLI", 0x13, 0x1, 0x00}, // Shift Left Logical Immediate (funct7 = 0x00)
    {"SRLI", 0x13, 0x5, 0x00}, // Shift Right Logical Immediate (funct7 = 0x00)
    {"SRAI", 0x13, 0x5, 0x20}, // Shift Right Arithmetic Immediate (funct7 = 0x20)
    {"LW", 0x03, 0x2, 0}, // Load Word
    {"JALR", 0x67, 0x0, 0} // Jump And Link Register
};

S_Instruction s_instructions = {"SW", 0x23, 0x2}; // Store Word

SB_Instruction sb_instructions[] = {
    {"BEQ", 0x63, 0x0}, // Branch if Equal
    {"BNE", 0x63, 0x1}, // Branch if Not Equal
    {"BLT", 0x63, 0x4}, // Branch if Less Than
    {"BGE", 0x63, 0x5} // Branch if Greater or Equal
};

UJ_Instruction uj_instructions = {"JAL", 0x6F}; // Jump and Link

// =====================================================================================================================
//
// Registers & Memory
//
// =====================================================================================================================

int registers[32]; // virtual register for execution
int return_pc = 0;

void initialize_registers() {
    // x0는 항상 0
    registers[0] = 0;
    // x1~x6는 1,2,3,4,5,6으로 초기화
    for (int i = 1; i <= 6; i++) {
        registers[i] = i;
    }
    // 나머지 레지스터는 0으로 초기화
    for (int i = 7; i < 32; i++) {
        registers[i] = 0;
    }
}

int memory[1024] = {0,}; // virtual memory for execution

// =====================================================================================================================
//
// Instruction Select Functions
//
// =====================================================================================================================

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

// =====================================================================================================================
//
// 각 타입의 기계어 명령어로 인코딩하는 코드
//
// =====================================================================================================================

// Encode R-type instruction
int encode_r_type(const int funct7, const int rs2, const int rs1,
                  const int funct3,
                  const int rd, const int opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

// Encode I-type instruction
int encode_i_type(const int imm, const int rs1, const int funct3,
                  const int rd, const int opcode) {
    return (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

// Encode S-type instruction
int encode_s_type(const int imm2, const int rs2, const int rs1,
                  const int funct3,
                  const int imm1, const int opcode) {
    return (imm2 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm1 << 7) | opcode;
}

// Encode SB-type instruction
int encode_sb_type(const int imm2, const int rs2, const int rs1,
                   const int funct3,
                   const int imm1, const int opcode) {
    return (imm2 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm1 << 7) | opcode;
}

// Encode UJ-type instruction
int encode_uj_type(const int imm, const int rd, const int opcode) {
    return (imm << 12) | (rd << 7) | opcode;
}

// =====================================================================================================================
//
// Utility 성격을 가지는 코드
//
// =====================================================================================================================

// Binary instruction으로 파일에 쓰기 위함
void print_binary_to_file(int n, FILE *file) {
    int bits = sizeof(n) * 8; //  int의 비트 수 (32비트 환경에서는 4바이트 * 8 = 32비트)

    for (int i = bits - 1; i >= 0; i--) {
        // 가장 왼쪽 비트부터 검사
        int mask = 1 << i; // 왼쪽으로 i만큼 시프트하여 해당 비트에 1을 위치시킴
        fprintf(file, "%d", (n & mask) ? 1 : 0); // 파일에 해당 비트가 1인지 0인지 출력
    }
    fprintf(file, "\n");
}

// S type 명령어에서 imm를 분리
void parse_imm_for_s_type_inst(const int imm, int *imm1, int *imm2) {
    // imm1에는 상위 비트 imm[11:5] 저장
    *imm1 = (imm >> 5) & 0x7F; // 0x7F는 7비트 마스크로, imm[11:5] 추출

    // imm2에는 하위 비트 imm[4:0] 저장
    *imm2 = imm & 0x1F; // 0x1F는 5비트 마스크로, imm[4:0] 추출
}

// SB 타입 명령어에서 imm을 분리
void parse_imm_for_sb_type_inst(const int imm, int *imm1, int *imm2) {
    // imm1에는 imm[12]와 imm[10:5]를 저장
    *imm1 = ((imm >> 5) & 0x3F) | ((imm >> 12) & 0x1) << 6; // 6비트와 1비트를 결합하여 imm[12:5] 추출

    // imm2에는 imm[4:1]과 imm[11]을 저장
    *imm2 = (imm & 0x1E) | ((imm >> 11) & 0x1); // 4비트와 1비트를 결합하여 imm[4:1]과 imm[11] 추출
}

int extract_bits(int imm, int high, int low) {
    int mask = (1 << (high - low + 1)) - 1;
    return (imm >> low) & mask;
}

// UJ 타입 명령어에서 imm를 파싱
int parse_imm_for_uj_type_inst(int imm) {
    imm = imm >> 1;

    // 각 비트를 추출하여 필요한 위치에 결합
    int bit_20 = extract_bits(imm, 19, 19); // imm[20]
    int bits_10_1 = extract_bits(imm, 9, 0); // imm[10-1]
    int bit_11 = extract_bits(imm, 10, 10); // imm[11]
    int bits_19_12 = extract_bits(imm, 18, 11); // imm[19-12]

    // 결합된 결과값을 저장할 변수
    int result = 0;

    // 각 비트 부분을 올바른 위치에 시프트하여 결합
    result |= (bit_20 << 19); // 20번 비트 위치에 bit_20 설정
    result |= (bits_10_1 << 9); // 10-1번 비트 위치에 bits_10_1 설정
    result |= (bit_11 << 8); // 11번 비트 위치에 bit_11 설정
    result |= (bits_19_12); // 19-12번 비트 위치에 bits_19_12 설정

    return result;
}

void fprintf_pc_into_trace_file(FILE *trace, const int *pc) {
    fprintf(trace, "%u\n", *pc);
}

// =====================================================================================================================
//
// 각 타입에 맞게 동작을 구현한 코드
//
// =====================================================================================================================

// Execution functions for R type instruction
void execute_r_type(const R_Instruction *instr, const int rd, const int rs1, const int rs2, FILE *trace, int *pc_ptr,
                    int *pc_location_ptr) {
    switch (instr->funct3) {
        // Case for ADD and SUB
        case 0x0:
            // Case for ADD
            if (instr->funct7 == 0x0) {
                registers[rd] = registers[rs1] + registers[rs2];
            }

            // Case for SUB
            else if (instr->funct7 == 0x20) {
                registers[rd] = registers[rs1] - registers[rs2];
            }

            break;

        // Case for SLL
        case 0x1:
            registers[rd] = registers[rs1] << registers[rs2];
            break;

        // Case for XOR
        case 0x4:
            registers[rd] = registers[rs1] ^ registers[rs2];
            break;

        // Case for SRL & SRA
        case 0x5: {
            int shamt = registers[rs2] & 0x1F; // Extract shift amount (lower 5 bits)

            if (instr->funct7 == 0x00) {
                // SRL (Shift Right Logical)
                registers[rd] = (uint32_t) registers[rs1] >> shamt; // Logical shift
            } else if (instr->funct7 == 0x20) {
                // SRA (Shift Right Arithmetic)
                registers[rd] = registers[rs1] >> shamt; // Arithmetic shift
            }

            break;
        }

        // Case for OR
        case 0x6:
            registers[rd] = registers[rs1] | registers[rs2];

        // Case for AND
        case 0x7:
            registers[rd] = registers[rs1] & registers[rs2];
        default:
            break;
    }

    *pc_location_ptr += 1;
    fprintf_pc_into_trace_file(trace, pc_ptr);
    *pc_ptr += 4;
}

// Execution functions for R type instruction
void execute_i_type(const I_Instruction *instr, const int rd, const int rs1, const int imm, FILE *trace,
                    int *pc_ptr, int *pc_location_ptr) {
    // Case for JARL instruction only
    if (instr->opcode == 0x67) {
        // JALR opcode

        fprintf_pc_into_trace_file(trace, pc_ptr);

        registers[rd] = registers[rs1] + imm;

        *pc_location_ptr = registers[rd];

        *pc_ptr = return_pc;
        *pc_ptr += 4;
    }

    // Case for opcode 0x13
    else if (instr->opcode == 0x13) {
        int shamt = 0;
        switch (instr->funct3) {
            case 0x0: // Case for ADDI
                registers[rd] = registers[rs1] + imm;
                break;

            case 0x4: // Case for XORI
                registers[rd] = registers[rs1] ^ imm;
                break;

            case 0x6: // Case for ORI
                registers[rd] = registers[rs1] | imm;
                break;

            case 0x7: // Case for ANDI
                registers[rd] = registers[rs1] & imm;
                break;

            case 0x1: // Case for SLLI
                shamt = imm;
                registers[rd] = registers[rs1] << shamt;
                break;

            case 0x5: // Case for SRLI & SRAI
                shamt = imm & 0x1F; // shamt is lower 5 bits of imm
                if (instr->funct7 == 0x00) {
                    // SRLI (Shift Right Logical Immediate)
                    registers[rd] = (uint32_t) registers[rs1] >> shamt;
                } else if (instr->funct7 == 0x20) {
                    // SRAI (Shift Right Arithmetic Immediate)
                    registers[rd] = (int32_t) registers[rs1] >> shamt;
                } else {
                    // Invalid instruction
                    printf("Invalid funct7 for shift instruction\n");
                }
                break;
        }

        *pc_location_ptr += 1;
        fprintf_pc_into_trace_file(trace, pc_ptr);
        *pc_ptr += 4;
    }

    // Case for opcode is 0x3
    else if (instr->opcode == 0x03) {
        // LW 명령어 처리
        switch (instr->funct3) {
            case 0x2: {
                const int address = registers[rs1] + imm;
                const int word_address = address >> 2;

                if (word_address >= 0 && word_address < 1024) {
                    registers[rd] = memory[word_address];
                } else {
                    printf("Memory access error: address out of bounds\n");
                }
                break;
            }
            default:
                printf("Unsupported funct3 for LW instruction\n");
                break;
        }

        *pc_location_ptr += 1;
        fprintf_pc_into_trace_file(trace, pc_ptr);
        *pc_ptr += 4;
    }
}

// Execution functions for S type instruction
void execute_s_type(const S_Instruction *instr, const int rs2, const int rs1,
                    const int imm, FILE *trace, int *pc_ptr, int *pc_location_ptr) {
    if (instr->funct3 == 0x2) {
        // SW 명령어 처리
        int address = registers[rs1] + imm;
        int word_address = address >> 2; // 4로 나누기

        // 메모리 범위 확인
        if (word_address >= 0 && word_address < 1024) {
            memory[word_address] = registers[rs2];
        } else {
            printf("Memory access error: address out of bounds\n");
        }
    }

    *pc_location_ptr += 1;
    fprintf_pc_into_trace_file(trace, pc_ptr);
    *pc_ptr += 4;
}


// Execution functions for SB type instruction
void execute_sb_type(const SB_Instruction *instr, const int rs1, const int rs2,
                     const int imm, FILE *trace, int *pc_ptr, int *pc_location_ptr) {
    int branch_condition_is_true = 0;

    switch (instr->funct3) {
        case 0x0: // BEQ (Branch if Equal)
            branch_condition_is_true = (registers[rs1] == registers[rs2]);
            break;

        case 0x1: // BNE (Branch if Not Equal)
            branch_condition_is_true = (registers[rs1] != registers[rs2]);
            break;

        case 0x4: // BLT (Branch if Less Than)
            // signed comparison을 위해 int32_t로 캐스팅
            branch_condition_is_true = ((int32_t) registers[rs1] < (int32_t) registers[rs2]);
            break;

        case 0x5: // BGE (Branch if Greater or Equal)
            // signed comparison을 위해 int32_t로 캐스팅
            branch_condition_is_true = ((int32_t) registers[rs1] >= (int32_t) registers[rs2]);
            break;

        default:
            printf("Invalid branch instruction funct3\n");
    }

    // 분기가 성공하면 PC를 업데이트
    if (branch_condition_is_true) {
        // imm은 이미 2를 곱한 값으로 가정 (word-aligned)
        fprintf_pc_into_trace_file(trace, pc_ptr);
        *pc_ptr = *pc_ptr + imm;

        const int size = sizeof(labels) / sizeof(labels[0]);

        for (int i = 0; i < size; i++) {
            if (*pc_ptr == labels[i].pc_address) {
                *pc_location_ptr = labels[i].instruction_index;
                break;
            }
        }
    } else {
        // 분기가 실패하면 다음 명령어로
        *pc_location_ptr += 1;
        fprintf_pc_into_trace_file(trace, pc_ptr);
        *pc_ptr = *pc_ptr + 4;
    }
}

void execute_uj_type(const UJ_Instruction *instr, const int rd, const int imm, FILE *trace, int *pc_ptr,
                     int *pc_location_ptr) {
    fprintf_pc_into_trace_file(trace, pc_ptr);
    return_pc = *pc_ptr;
    registers[rd] = *pc_location_ptr + 1; // 프로시저 호출 다음 명령어 위치
    *pc_ptr = *pc_ptr + imm;

    const int size = sizeof(labels) / sizeof(labels[0]);
    for (int i = 0; i < size; i++) {
        if (*pc_ptr == labels[i].pc_address) {
            *pc_location_ptr = labels[i].instruction_index;
            break;
        }
    }
}


// =====================================================================================================================
//
// 핵심 동작을 수행하는 함수
//
// =====================================================================================================================

int have_syntax_error_instruction(const char *filename) {
    FILE *input_file = fopen(filename, "r");
    char line[MAX_LINE_LENGTH] = {0,};

    while (fgets(line, sizeof(line), input_file)) {
        char procedure_name[MAX_LINE_LENGTH] = {0,};
        char jump_label_name[MAX_LINE_LENGTH] = {0,};
        char instruction_name[MAX_LINE_LENGTH] = {0,};
        int rd = 0, rs1 = 0, rs2 = 0, imm = 0;

        if (sscanf(line, "%s x%d, x%d, x%d", instruction_name, &rd, &rs1, &rs2) == 4) {
            const R_Instruction *r_instr = find_r_instruction(instruction_name);
            if (r_instr == NULL) {
                return 1;
            }
        } else if (sscanf(line, "%s x%d, x%d, %d", instruction_name, &rd, &rs1, &imm) == 4) {
            const I_Instruction *i_instr = find_i_instruction(instruction_name);
            if (i_instr == NULL) {
                return 1;
            }
        } else if (sscanf(line, "%s x%d, %d(x%d)", instruction_name, &rd, &imm, &rs1) == 4) {
            const I_Instruction *i_instr = find_i_instruction(instruction_name); // Return LW only
            const S_Instruction *s_instr = find_s_instruction(instruction_name); // Return SW only

            if (i_instr == NULL) {
                if (s_instr != NULL) {
                    continue;
                }
                return 1;
            }
        } else if (sscanf(line, "%s x%d, x%d, %s", instruction_name, &rs1, &rs2, jump_label_name) == 4) {
            const SB_Instruction *sb_instr = find_sb_instruction(instruction_name);
            if (sb_instr == NULL) {
                return 1;
            }
        } else if (sscanf(line, "%s x%d, %s", instruction_name, &rd, procedure_name) == 3) {
            const UJ_Instruction *uj_instr = find_uj_instruction(instruction_name);
            if (uj_instr == NULL) {
                return 1;
            }
        }
    }

    return 0;
}

void record_label(const char *filename) {
    FILE *input_file = fopen(filename, "r");

    char line[MAX_LINE_LENGTH] = {0,};
    int pc = STARTING_PC;
    int label_count = 0;
    int index = 0;

    while (fgets(line, sizeof(line), input_file)) {
        char *line_ptr = line;

        // 앞쪽 공백 제거
        while (isspace(*line_ptr)) line_ptr++;
        if (*line_ptr == '\0' || *line_ptr == '\n') continue; // 빈 줄은 건너뜀

        // 라인이 레이블인지 확인
        char *colon_ptr = strchr(line_ptr, ':');
        if (colon_ptr) {
            // 레이블임
            size_t label_len = colon_ptr - line_ptr;
            char label_name[MAX_LINE_LENGTH] = {0,};
            strncpy(label_name, line_ptr, label_len);
            label_name[label_len] = '\0';

            // 레이블 이름과 현재 pc를 labels[] 배열에 저장
            strcpy(labels[label_count].name, label_name);
            labels[label_count].pc_address = pc;
            labels[label_count].instruction_index = index;
            index++;
            label_count++;
        } else {
            index++;
            pc += 4;
        }
    }

    fclose(input_file);
}

// Add other encoding functions for S-type, SB-type, U-type, and UJ-type
void translate_assembly_instruction(const char *filename) {
    FILE *input_file = fopen(filename, "r");

    char output_file[MAX_LINE_LENGTH];
    char filename_without_extension[MAX_LINE_LENGTH];
    sscanf(filename, "%[^.]", filename_without_extension);
    snprintf(output_file, sizeof(output_file), "%s.o", filename_without_extension);

    FILE *output = fopen(output_file, "w");

    char line[MAX_LINE_LENGTH] = {0,};
    int pc = STARTING_PC;

    while (fgets(line, sizeof(line), input_file)) {
        char instruction_name[MAX_LINE_LENGTH] = {0,};
        char jump_label_name[MAX_LINE_LENGTH] = {0,};
        char procedure_name[MAX_LINE_LENGTH] = {0,};
        int rd = 0, rs1 = 0, rs2 = 0, imm = 0;
        int machine_code = 0;


        if (sscanf(line, "%s x%d, x%d, x%d", instruction_name, &rd, &rs1, &rs2) == 4) {
            const R_Instruction *r_instr = find_r_instruction(instruction_name);
            machine_code = encode_r_type(r_instr->funct7, rs2, rs1, r_instr->funct3, rd, r_instr->opcode);
        }

        // operation rd, rs1, imm12 format instruction -> I type with immediate instruction
        // operation rd, rs1, shamt format instruction -> SLLI & SRLI & SRAI instruction only
        // I 타입 명령어 처리 - immediate 값을 사용하는 경우
        else if (sscanf(line, "%s x%d, x%d, %d", instruction_name, &rd, &rs1, &imm) == 4) {
            const I_Instruction *i_instr = find_i_instruction(instruction_name);

            if (strcasecmp(instruction_name, "SLLI") == 0 ||
                strcasecmp(instruction_name, "SRLI") == 0 ||
                strcasecmp(instruction_name, "SRAI") == 0) {
                int shamt = imm;
                machine_code = encode_i_type(
                    (i_instr->funct7 << 5) | shamt,
                    rs1,
                    i_instr->funct3,
                    rd,
                    i_instr->opcode
                );
            } else {
                machine_code = encode_i_type(imm, rs1, i_instr->funct3, rd, i_instr->opcode);
            }
        }


        // operation rd, imm12(rs1) format instruction -> I type with load instruction
        // operation rs2, imm12(rs1) format instruction -> S type with store instruction
        // JALR x0, 0(x1) format instruction -> I type with jump and link instruction
        else if (sscanf(line, "%s x%d, %d(x%d)", instruction_name, &rd, &imm, &rs1) == 4) {
            int is_LW = strcasecmp(instruction_name, "LW"); // compare does instruction is LW
            int is_JARL = strcasecmp(instruction_name, "JALR");

            if (is_LW == 0) {
                const I_Instruction *i_instr = find_i_instruction(instruction_name); // return only LW instruction
                machine_code = encode_i_type(imm, rs1, i_instr->funct3, rd, i_instr->opcode);
            } else if (is_JARL == 0) {
                const I_Instruction *i_instr = find_i_instruction(instruction_name);
                machine_code = encode_i_type(imm, rs1, i_instr->funct3, rd, i_instr->opcode);
            } else {
                // SW case
                const S_Instruction *s_instr = find_s_instruction(instruction_name); // return only SW instruction

                int imm1; // imm[11:5]
                int imm2; // imm[4:0]

                parse_imm_for_s_type_inst(imm, &imm1, &imm2); // parse imm into two individual imm variables

                rs2 = rd; // SW instruction doesn't use rd. Change into rs2.

                machine_code = encode_s_type(imm1, rs2, rs1, s_instr->funct3, imm2, s_instr->opcode);
            }
        }

        // operation rs1, rs2, imm12 format instruction -> SB type
        else if (sscanf(line, "%s x%d, x%d, %s", instruction_name, &rs1, &rs2, jump_label_name) == 4) {
            const SB_Instruction *sb_instr = find_sb_instruction(instruction_name);

            const int labels_size = sizeof(labels) / sizeof(labels[0]);
            for (int i = 0; i < labels_size; i++) {
                if (strcasecmp(labels[i].name, jump_label_name) == 0) {
                    imm = labels[i].pc_address - pc;
                    break;
                }
            }

            int imm1; // imm[12:10-5]
            int imm2; // imm[4-0:11]

            parse_imm_for_sb_type_inst(imm, &imm1, &imm2);

            machine_code = encode_sb_type(imm1, rs2, rs1, sb_instr->funct3, imm2, sb_instr->opcode);
        }

        // operation rd, imm20 format instruction -> UJ type
        else if (sscanf(line, "%s x%d, %s", instruction_name, &rd, procedure_name) == 3) {
            const UJ_Instruction *uj_instr = find_uj_instruction(instruction_name);

            const int labels_size = sizeof(labels) / sizeof(labels[0]);
            for (int i = 0; i < labels_size; i++) {
                if (strcasecmp(labels[i].name, procedure_name) == 0) {
                    imm = labels[i].pc_address - pc;
                    break;
                }
            }

            imm = parse_imm_for_uj_type_inst(imm);

            machine_code = encode_uj_type(imm, rd, uj_instr->opcode);
        }

        // EXIT
        else if (sscanf(line, "%s", instruction_name) == 1) {
            if (strcasecmp(instruction_name, "EXIT") == 0) {
                machine_code = EXIT_CODE;
            } else {
                continue;
            }
        }

        // rest of the case. might be blank line and label
        else {
            continue;
        }

        // Write machine code in output file
        if (machine_code == EXIT_CODE) {
            print_binary_to_file(EXIT_CODE, output);
        } else {
            print_binary_to_file(machine_code, output);

            pc += 4; // Increment PC by 4 for each instruction
        }
    }

    fclose(input_file);
    fclose(output);

    // printf("Files %s generated successfully.\n", output_file);
}

void trace_pc(const char *filename) {
    FILE *input_file = fopen(filename, "r");

    char trace_file[MAX_LINE_LENGTH] = {0,};
    char filename_without_extension[MAX_LINE_LENGTH] = {0,};
    sscanf(filename, "%[^.]", filename_without_extension);
    snprintf(trace_file, sizeof(trace_file), "%s.trace", filename_without_extension);
    FILE *trace = fopen(trace_file, "w");

    char line[MAX_LINE_LENGTH] = {0,};
    int pc = STARTING_PC;

    char instructions[MAX_LINE_COUNT][MAX_LINE_LENGTH] = {{0,}};

    int index = 0;
    while (fgets(line, sizeof(line), input_file)) {
        if (line[0] == '\n') {
            continue;
        }
        strcpy(instructions[index], line);
        index++;
    }

    for (int pc_location = 0; pc_location < index;) {
        char instruction_name[MAX_LINE_LENGTH] = {0,};
        char jump_label_name[MAX_LINE_LENGTH] = {0,};
        char procedure_name[MAX_LINE_LENGTH] = {0,};
        int rd = 0, rs1 = 0, rs2 = 0, imm = 0;

        strcpy(line, instructions[pc_location]);

        // Execute R type instruction
        if (sscanf(line, "%s x%d, x%d, x%d", instruction_name, &rd, &rs1, &rs2) == 4) {
            const R_Instruction *r_instr = find_r_instruction(instruction_name);
            execute_r_type(r_instr, rd, rs1, rs2, trace, &pc, &pc_location);
        }

        // Execute "operation rd, rs1, imm12" format instruction
        // Execute "operation rd, rs1, shamt" format instruction
        else if (sscanf(line, "%s x%d, x%d, %d", instruction_name, &rd, &rs1, &imm) == 4) {
            const I_Instruction *i_instr = find_i_instruction(instruction_name);

            if (strcasecmp(instruction_name, "SLLI") == 0 ||
                strcasecmp(instruction_name, "SRLI") == 0 ||
                strcasecmp(instruction_name, "SRAI") == 0) {
                int shamt = imm;
                execute_i_type(i_instr, rd, rs1, shamt, trace, &pc, &pc_location); // Execute SLLI & SRLI & SRAI
            } else {
                execute_i_type(i_instr, rd, rs1, imm, trace, &pc, &pc_location); // Execute ADDI, XORI, ORI, ANDI
            }
        }

        // Execute "operation rd, imm12(rs1)" format instruction
        // Execute "operation rs2, imm12(rs1)" format instruction
        // Execute "JALR x0, 0(x1)" format instruction
        else if (sscanf(line, "%s x%d, %d(x%d)", instruction_name, &rd, &imm, &rs1) == 4) {
            int is_LW = strcasecmp(instruction_name, "LW"); // compare does instruction is LW
            int is_JARL = strcasecmp(instruction_name, "JALR");

            const I_Instruction *i_instr = find_i_instruction(instruction_name);

            if (is_LW == 0) {
                execute_i_type(i_instr, rd, rs1, imm, trace, &pc, &pc_location);
            } else if (is_JARL == 0) {
                execute_i_type(i_instr, rd, rs1, imm, trace, &pc, &pc_location);
            } else {
                // SW case
                const S_Instruction *s_instr = find_s_instruction(instruction_name); // return only SW instruction
                rs2 = rd; // SW instruction doesn't use rd. Change into rs2.
                execute_s_type(s_instr, rs2, rs1, imm, trace, &pc, &pc_location);
            }
        }

        // Execute "operation rs1, rs2, imm12" format instruction
        else if (sscanf(line, "%s x%d, x%d, %s", instruction_name, &rs1, &rs2, jump_label_name) == 4) {
            const SB_Instruction *sb_instr = find_sb_instruction(instruction_name);

            const int labels_size = sizeof(labels) / sizeof(labels[0]);
            for (int i = 0; i < labels_size; i++) {
                if (strcasecmp(labels[i].name, jump_label_name) == 0) {
                    imm = labels[i].pc_address - pc;
                    break;
                }
            }

            execute_sb_type(sb_instr, rs1, rs2, imm, trace, &pc, &pc_location);
        }

        // Execute "operation rd, imm20" format instruction
        else if (sscanf(line, "%s x%d, %s", instruction_name, &rd, procedure_name) == 3) {
            const UJ_Instruction *uj_instr = find_uj_instruction(instruction_name);

            const int labels_size = sizeof(labels) / sizeof(labels[0]);
            for (int i = 0; i < labels_size; i++) {
                if (strcasecmp(labels[i].name, procedure_name) == 0) {
                    imm = labels[i].pc_address - pc;
                    break;
                }
            }

            execute_uj_type(uj_instr, rd, imm, trace, &pc, &pc_location);
        }

        // EXIT
        else if (sscanf(line, "%s", instruction_name) == 1) {
            if (strcasecmp(instruction_name, "EXIT") == 0) {
                fprintf_pc_into_trace_file(trace, &pc);
                fclose(trace);
                printf("Files %s generated successfully.\n", trace_file);
                return;
            }

            pc_location++;
        }
    }

    fclose(trace);

    // printf("Files %s generated successfully.\n", trace_file);
}

// =====================================================================================================================
//
// 메인 함수
//
// =====================================================================================================================

int main() {
    int terminate_flag = 0;

    while (true) {
        initialize_registers(); // Need to initialize everytime when filename entered
        char filename[MAX_LINE_LENGTH] = {0,};
        printf("Enter Input File Name: ");
        scanf("%s", filename);

        terminate_flag = strcasecmp("terminate", filename);

        if (terminate_flag == 0) {
            break;
        }

        FILE *input_file = fopen(filename, "r");

        if (!input_file) {
            printf("Input file does not exist!!\n");
            continue;
        }

        const int syntax_error_flag = have_syntax_error_instruction(filename);

        if (syntax_error_flag == 1) {
            printf("Syntax Error!!\n");
        } else {
            record_label(filename);
            translate_assembly_instruction(filename);
            trace_pc(filename);
        }
    }

    return 0;
}
