#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define GET_MOV 0xFC   // 6 bits (11111100)
#define CHECK_MOV 0x88 // 6 bits (10001000)
#define GET_D 0x2      // 1 bit  (00000010)
#define GET_W 0x1      // 1 bit  (00000001)
#define GET_MOD 0xC0   // 2 bits (11000000)
#define GET_REG 0x38   // 3 bits (00111000)
#define GET_RM 0x7     // 3 bits (00000111)

#define REGISTER_TO_REGISTER 0b100010
#define IMMEDIATE_TO_REGISTER 0b1011
#define IMMEIDATE_TO_RESITER_MEMORY 0b1100011
#define MEM_TO_ACCUMULATOR 0b101000

char *namesWord[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char *namesByte[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char *effectiveAddress[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};

char *programName;
int parseParam(int argc, char *argv[]) {
  programName = argv[1];
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [FILE]...\n", programName);
    return 0;
  }
  return 1;
}

void outputEffectiveAddress(char w, char d, char reg, char rm, int16_t bitDisplacement) {
  char* regValue = w ? namesWord[reg] : namesByte[reg];
  if (d) {
    if (bitDisplacement > 0) {
      printf("%s, [%s + %d]", regValue, effectiveAddress[rm], bitDisplacement);
    } else if (bitDisplacement < 0) {
      printf("%s, [%s - %d]", regValue, effectiveAddress[rm], -bitDisplacement);
    } else {
      printf("%s, [%s]", regValue, effectiveAddress[rm]);
    }
  } else {
    if (bitDisplacement > 0) {
      printf("[%s + %d], %s", effectiveAddress[rm], bitDisplacement, regValue);
    } else if (bitDisplacement < 0) {
      printf("[%s - %d], %s", effectiveAddress[rm], -bitDisplacement, regValue);
    } else {
      printf("[%s], %s", effectiveAddress[rm], regValue);
    }
  }
}

void outPutJustEffectiveAddress(char mod, char rm, FILE *input) {
  int16_t bitDisplacement;
  if (mod == 0b00) {
    if (rm == 0b110) {
      fread(&bitDisplacement, sizeof(int16_t), 1, input);
      printf("[%d]", bitDisplacement);
    } else {
      printf("[%s]", effectiveAddress[rm]);
    }
  } else {
    if (mod == 0b01) {
      fread(&bitDisplacement, sizeof(int8_t), 1, input);
    } else if (mod == 0b10) {
      fread(&bitDisplacement, sizeof(int16_t), 1, input);
    }
    if (bitDisplacement > 0) {
      printf("[%s + %d]", effectiveAddress[rm], bitDisplacement);
    } else if (bitDisplacement < 0) {
      printf("[%s - %d]", effectiveAddress[rm], -bitDisplacement);
    } else {
      printf("[%s]", effectiveAddress[rm]);
    }
  }
}

int main(int argc, char *argv[]) {
  if (!parseParam(argc, argv)) {
    exit(EXIT_FAILURE);
  }

  /*only need to read 2 byte as a time becuase each instruction is 2 byte*/
  FILE *asmFile = fopen(programName, "rb");
  uint8_t buffer;
  if (asmFile) {
    printf("bits 16\n\n");
    while (!feof(asmFile)) {
      buffer = fgetc(asmFile);
      // Check for the type of Instruction encoding 
      if(buffer >> 2 == REGISTER_TO_REGISTER) {
        char isMov = (buffer & GET_MOV) == CHECK_MOV;
        char movd  = buffer & GET_D;
        char movw = buffer & GET_W;
        uint8_t nextBuffer;
        fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);
        
        // Keep getting other attribute
        char mod = nextBuffer >> 6;
        char reg = (nextBuffer & GET_REG) >> 3;
        char rm = nextBuffer & GET_RM;
        printf("mov ");

        char* regValue = (movw)? namesWord[reg] : namesByte[reg];
        char* rmValue = (movw)? namesWord[rm] : namesByte[rm];
        
        if(mod == 0b11) {
          if(movd) {
            printf("%s, %s", regValue, rmValue);
          } else {
            printf("%s, %s", rmValue, regValue);
          }
        } else {
          int16_t bitsDisplacement_16;
          int8_t bitsDisplacement_8;
          if (mod == 0b00) {
            if (rm == 0b110) {
              fread(&bitsDisplacement_16, sizeof(int16_t), 1, asmFile);
              if (movd) {
                printf("%s, [%d]", regValue, bitsDisplacement_16);
              } else {
                printf("[%d], %s", bitsDisplacement_16, regValue);
              }
            } else {
              if (movd) {
                printf("%s, [%s]", regValue, effectiveAddress[rm]);
              } else {
                printf("[%s], %s", effectiveAddress[rm], regValue);
              }
            }
          } 
          else if (mod == 0b01) {
            fread(&bitsDisplacement_8, sizeof(int8_t), 1, asmFile);
            outputEffectiveAddress(movw, movd, reg, rm, bitsDisplacement_8);
          } else if (mod == 0b10) {
            fread(&bitsDisplacement_16, sizeof(int16_t), 1, asmFile);
            outputEffectiveAddress(movw, movd, reg, rm, bitsDisplacement_16);
          }
        }
      } 
      else if (buffer >> 4 == IMMEDIATE_TO_REGISTER) {
        char movw = buffer >> 3 & 1;
        char reg = buffer & 0b111;
        // 16 bit if w == 1
        // else 8 bit
        printf("mov ");
        if(movw) {
          int16_t nextBuffer;
          fread(&nextBuffer, sizeof(int16_t), 1, asmFile);
          printf("%s, %d", namesWord[reg], nextBuffer);
        } else {
          int8_t nextBuffer; 
          fread(&nextBuffer, sizeof(int8_t), 1, asmFile);
          printf("%s, %d", namesByte[reg], nextBuffer);
        }
      }
      else if (buffer >> 1 == IMMEIDATE_TO_RESITER_MEMORY) {
        char movw = buffer & 1;

        uint8_t nextBuffer;
        fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);
        char mod = nextBuffer >> 6;
        char rm = nextBuffer & GET_RM; 

        int16_t data_16;
        int8_t data_8;
        printf("mov ");
        outPutJustEffectiveAddress(mod, rm, asmFile);
        
        if(movw) {
          fread(&data_16, sizeof(int16_t), 1, asmFile);
        } else {
          fread(&data_8, sizeof(int8_t), 1, asmFile);
        }
        printf((movw) ? ", word %d" : ", byte %d", movw ? data_16 : data_8);
      } 
      else if (buffer >> 2 == MEM_TO_ACCUMULATOR) {
        char movd = buffer & GET_D;
        char movw = buffer & GET_W;
        printf("mov ");
        if(movw) {
          int16_t data;
          fread(&data, sizeof(int16_t), 1, asmFile);
          if(movd) printf("[%d], %s", data, namesWord[0]);
          else printf("%s, [%d]", namesWord[0], data);
        } else {
          int8_t data;
          fread(&data, sizeof(int8_t), 1, asmFile);
          if(movd) printf("[%d], %s", data, namesWord[0]);
          else printf("%s, [%d]", namesWord[0], data);
        }
      }
      printf("\n");
    }
  }
  fclose(asmFile);
  return 0;
}