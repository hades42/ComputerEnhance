#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define IMM_TO_REGISTER 0b1011
#define REGISTER_TO_REGISTER 0b100010

char *registers[2][8] = {{"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"},
                         {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"}};

int main() {
  FILE *asmFile = fopen("test", "rb");
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

        if (d) {
          printf("mov %s, %s \n", registers[w][reg], registers[w][r_m]);
        } else {
          printf("mov %s, %s \n", registers[w][r_m], registers[w][reg]);
        }
      }
    }
  }
}
