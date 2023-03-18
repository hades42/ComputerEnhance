#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define IMM_TO_REGISTER 0b1011
#define REGISTER_TO_REGISTER 0b100010
#define IMM_TO_REGISTER_MEM 0b1100011
#define MEM_TO_ACCUMULATOR 0b101000

char *registers[2][8] = {{"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
                         {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}};

char *address_calculation[] = {
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx",
};
char *programName;

int16_t read_sign_extended(FILE *input, bool w) {
  if (w) {
    int16_t output;
    fread(&output, sizeof(int16_t), 1, input);
    return output;
  } else {
    int8_t output;
    fread(&output, sizeof(int8_t), 1, input);
    return output;
  }
}

int16_t read_disp(uint8_t mod, FILE *input) {
  if (mod) {
    return read_sign_extended(input, mod == 0b10);
  } else {
    return 0;
  }
}

void outputEffectiveAddress(uint8_t mod, uint8_t r_m, FILE *input) {
  if (mod == 0b00 && r_m == 0b110) {
    int16_t displ;
    fread(&displ, sizeof(int16_t), 1, input);
    printf("[%d]", displ);
  } else {
    int16_t displ = read_disp(mod, input);
    if (displ) {
      if (displ > 0) {
        printf("[%s + %d]", address_calculation[r_m], displ);
      } else if (displ < 0) {
        printf("[%s - %d]", address_calculation[r_m], -displ);
      }
    } else {
      printf("[%s]", address_calculation[r_m]);
    }
  }
}

int parseParam(int argc, char *argv[]) {
  programName = argv[1];
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [FILE]...\n", programName);
    return 0;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  if (!parseParam(argc, argv)) {
    exit(EXIT_FAILURE);
  }

  FILE *asmFile = fopen(programName, "rb");
  uint8_t buffer;
  if (asmFile) {
    printf("bits 16\n\n");
    // Read each line
    while (!feof(asmFile)) {
      // First Byte
      buffer = fgetc(asmFile);

      // Decode Immediate To Register
      if (buffer >> 4 == IMM_TO_REGISTER) {
        bool w = buffer >> 3 & 1;
        uint8_t reg = buffer & 0b111;
        // Word
        if (w) {
          // Want to read signed number because there are negative numbers
          int16_t data;
          fread(&data, sizeof(int16_t), 1, asmFile);
          printf("mov %s, %d\n", registers[w][reg], data);
        } else {
          // byte
          int8_t data;
          fread(&data, sizeof(int8_t), 1, asmFile);
          printf("mov %s, %d\n", registers[w][reg], data);
        }
      } else if (buffer >> 2 == REGISTER_TO_REGISTER) {
        // Decode REGISTER_TO_REGISTER
        bool w = buffer & 1;
        bool d = buffer >> 1 & 1;

        uint8_t nextBuffer;
        fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);

        uint8_t mod = nextBuffer >> 6;
        uint8_t reg = nextBuffer >> 3 & 0b111;
        uint8_t r_m = nextBuffer & 0b111;

        if (mod == 0b11) {
          // Register mode
          if (d) {
            printf("mov %s, %s \n", registers[w][reg], registers[w][r_m]);
          } else {
            printf("mov %s, %s \n", registers[w][r_m], registers[w][reg]);
          }
        } else {
          if (d) {
            printf("mov %s, ", registers[w][reg]);
            outputEffectiveAddress(mod, r_m, asmFile);
            printf("\n");
          } else {
            printf("mov ");
            outputEffectiveAddress(mod, r_m, asmFile);
            printf(", %s\n", registers[w][reg]);
          }
        }

      } else if (buffer >> 1 == IMM_TO_REGISTER_MEM) {
        bool w = buffer & 1;

        uint8_t nextBuffer;
        fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);

        uint8_t mod = nextBuffer >> 6;
        uint8_t r_m = nextBuffer & 0b111;
        if (mod == 0b11) {
          int16_t data = read_sign_extended(asmFile, w);
          printf("mov %s, %d\n", registers[w][r_m], data);
        } else {
          printf("mov ");
          outputEffectiveAddress(mod, r_m, asmFile);

          if (w == 0) {
            int16_t data = read_sign_extended(asmFile, w);
            printf(", byte %d\n", data);
          } else {
            int16_t data = read_sign_extended(asmFile, w);
            printf(", word %d\n", data);
          }
        }
      } else if (buffer >> 2 == MEM_TO_ACCUMULATOR) {
        bool w = buffer & 1;
        bool d = buffer >> 1 & 1;
        if (d) {
          // Memory to accumulator
          if (w) {
            // 16bits
            uint16_t data;
            fread(&data, sizeof(uint16_t), 1, asmFile);
            printf("mov [%d], %s\n", data, registers[w][0]);
          } else {
            // 8 bits
            uint8_t data;
            fread(&data, sizeof(uint8_t), 1, asmFile);
            printf("mov [%d], %s\n", data, registers[w][0]);
          }
        } else {
          // Memory to accumulator
          if (w) {
            // 16bits
            uint16_t data;
            fread(&data, sizeof(uint16_t), 1, asmFile);
            printf("mov %s, [%d]\n", registers[w][0], data);
          } else {
            // 8 bits
            uint8_t data;
            fread(&data, sizeof(uint8_t), 1, asmFile);
            printf("mov %s, [%d]\n", registers[w][0], data);
          }
        }
      }
    }
  }
}
