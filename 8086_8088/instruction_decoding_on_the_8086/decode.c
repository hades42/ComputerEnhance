#include <stdio.h>
#include <stdlib.h>

#define GET_MOV 0xFC   // 6 bits (11111100)
#define CHECK_MOV 0x88 // 6 bits (10001000)
#define GET_D 0x2      // 1 bit  (00000010)
#define GET_W 0x1      // 1 bit  (00000001)
#define GET_MOD 0xC0   // 2 bits (11000000)
#define GET_REG 0x38   // 3 bits (00111000)
#define GET_RM 0x7     // 3 bits (00000111)

char *namesByte[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char *namesWord[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

char *programName;
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

  /*only need to read 2 byte as a time becuase each instruction is 2 byte*/
  FILE *asmFile = fopen(programName, "rb");
  char buffer[2];
  char ahihi = GET_D;
  if (asmFile) {
    printf("bits 16\n\n");
    while (!feof(asmFile)) {
      buffer[0] = fgetc(asmFile);
      buffer[1] = fgetc(asmFile);

      char isMov = (buffer[0] & GET_MOV) == CHECK_MOV;
      if (isMov) {
        printf("mov ");
        char movd = buffer[0] & GET_D;
        char movw = buffer[0] & GET_W;
        char mod = buffer[1] & GET_MOD;
        char reg = (buffer[1] & GET_REG) >> 3;
        char rm = buffer[1] & GET_RM;

        if (movw) {
          // W = 1 => namesByte
          if (movd) {
            printf("%s, %s", namesByte[reg], namesByte[rm]);
          } else {
            printf("%s, %s", namesByte[rm], namesByte[reg]);
          }
        } else {
          // W = 0 => namesWord
          if (movd) {
            printf("%s, %s", namesWord[reg], namesWord[rm]);
          } else {
            printf("%s, %s", namesWord[rm], namesWord[reg]);
          }
        }
      }
      printf("\n");
    }
  }
  fclose(asmFile);
  return 0;
}
