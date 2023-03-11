#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define IMM_TO_REGISTER 0b1011 // 1011

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

      // Check for Immediate To Register
      if (buffer >> 4 == IMM_TO_REGISTER) {
        bool w = buffer >> 3 & 1;
        uint8_t reg = buffer & 0b111;
        // Word
        if (w) {
          int16_t data;
          fread(&data, sizeof(int16_t), 1, asmFile);
          printf("%s %d\n", registers[w][reg], data);
        } else {
          // byte
          int8_t data;
          fread(&data, sizeof(int8_t), 1, asmFile);
          printf("%s %d\n", registers[w][reg], data);
        }
      }
    }
  }
}
