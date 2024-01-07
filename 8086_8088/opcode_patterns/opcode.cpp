#include "jumpDecode.h"
#include "jumpDecode.cpp"
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

// For mov instruction
#define REGISTER_TO_REGISTER 0b100010
#define IMMEDIATE_TO_REGISTER 0b1011
#define IMMEIDATE_TO_RESITER_MEMORY 0b1100011
#define MEM_TO_ACCUMULATOR 0b101000

// For add instruction
#define ADD_REGISTER_TO_REGISTER 0b000000
#define ADD_IMMEDIATE_TO_REGISTER_MEMORY 0b100000
#define ADD_IMMEDIATE_TO_ACCUMULATOR 0b0000010

// For sub instruction
#define SUB_REGISTER_TO_REGISTER 0b001010
#define SUB_IMMEDIATE_TO_REGISTER_MEMORY 0b100000
#define SUB_IMMEDIATE_TO_ACCUMULATOR 0b0010110

#define CMP_REGISTER_TO_REGISTER 0b001110
#define CMP_IMMEDIATE_TO_REGISTER_MEMORY 0b100000
#define CMP_IMMEDIATE_TO_ACCUMULATOR 0b0011110

#define IS_JUMP 0b0111
#define LOOP 0b111000

char *namesWord[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char *namesByte[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char *effectiveAddress[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};
char *jumpInstruction[2][8] = {
    {"jo", "jb", "je", "jbe", "js", "jp", "jl", "jle"},
    {"jno", "jnb", "jne", "jnbe", "jns", "jnp", "jnl", "jnle"}};
char *loopInstruction[4] = {"loopnz", "loopz", "loop", "jcxz"};

char *programName;

int parseParam(int argc, char *argv[])
{
    programName = argv[1];
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [FILE]...\n", programName);
        return 0;
    }
    return 1;
}

void outputEffectiveAddress(char w, char d, char reg, char rm, int16_t bitDisplacement)
{
    char *regValue = w ? namesWord[reg] : namesByte[reg];
    if (d)
    {
        if (bitDisplacement > 0)
        {
            printf("%s, [%s + %d]", regValue, effectiveAddress[rm], bitDisplacement);
        }
        else if (bitDisplacement < 0)
        {
            printf("%s, [%s - %d]", regValue, effectiveAddress[rm], -bitDisplacement);
        }
        else
        {
            printf("%s, [%s]", regValue, effectiveAddress[rm]);
        }
    }
    else
    {
        if (bitDisplacement > 0)
        {
            printf("[%s + %d], %s", effectiveAddress[rm], bitDisplacement, regValue);
        }
        else if (bitDisplacement < 0)
        {
            printf("[%s - %d], %s", effectiveAddress[rm], -bitDisplacement, regValue);
        }
        else
        {
            printf("[%s], %s", effectiveAddress[rm], regValue);
        }
    }
}

void outPutJustEffectiveAddress(char mod, char rm, FILE *input)
{
    int16_t bitDisplacement_16;
    int8_t bitDisplacement_8;
    if (mod == 0b00)
    {
        if (rm == 0b110)
        {
            fread(&bitDisplacement_16, sizeof(int16_t), 1, input);
            printf("[%d]", bitDisplacement_16);
        }
        else
        {
            printf("[%s]", effectiveAddress[rm]);
        }
    }
    else
    {
        if (mod == 0b01)
        {
            fread(&bitDisplacement_8, sizeof(int8_t), 1, input);
            if (bitDisplacement_8 > 0)
            {
                printf("[%s + %d]", effectiveAddress[rm], bitDisplacement_8);
            }
            else if (bitDisplacement_8 < 0)
            {
                printf("[%s - %d]", effectiveAddress[rm], -bitDisplacement_8);
            }
            else
            {
                printf("[%s]", effectiveAddress[rm]);
            }
        }
        else if (mod == 0b10)
        {
            fread(&bitDisplacement_16, sizeof(int16_t), 1, input);
            if (bitDisplacement_16 > 0)
            {
                printf("[%s + %d]", effectiveAddress[rm], bitDisplacement_16);
            }
            else if (bitDisplacement_16 < 0)
            {
                printf("[%s - %d]", effectiveAddress[rm], -bitDisplacement_16);
            }
            else
            {
                printf("[%s]", effectiveAddress[rm]);
            }
        }
    }
}

void registerToRegister(uint8_t buffer, FILE *asmFile)
{
    char d = buffer & GET_D;
    char w = buffer & GET_W;
    uint8_t nextBuffer;
    fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);

    char mod = nextBuffer >> 6;
    char reg = (nextBuffer & GET_REG) >> 3;
    char rm = nextBuffer & GET_RM;

    char *regValue = w ? namesWord[reg] : namesByte[reg];
    char *rmValue = w ? namesWord[rm] : namesByte[rm];

    if (mod == 0b11)
    {
        if (d)
        {
            printf("%s, %s", regValue, rmValue);
        }
        else
        {
            printf("%s, %s", rmValue, regValue);
        }
    }
    else
    {
        int16_t bitDisplacement_16;
        int8_t bitDisplacement_8;
        if (mod == 0b00)
        {
            if (rm == 0b110)
            {
                fread(&bitDisplacement_16, sizeof(int16_t), 1, asmFile);
                if (d)
                {
                    printf("%s, [%d]", regValue, bitDisplacement_16);
                }
                else
                {
                    printf("[%d], %s", bitDisplacement_16, regValue);
                }
            }
            else
            {
                if (d)
                {
                    printf("%s, [%s]", regValue, effectiveAddress[rm]);
                }
                else
                {
                    printf("[%s], %s", effectiveAddress[rm], regValue);
                }
            }
        }
        else if (mod == 0b01)
        {
            if (d)
            {
                printf("%s, ", regValue);
                outPutJustEffectiveAddress(mod, rm, asmFile);
            }
            else
            {
                outPutJustEffectiveAddress(mod, rm, asmFile);
                printf(", %s", regValue);
            }
        }
        else if (mod == 0b10)
        {
            if (d)
            {
                printf("%s, ", regValue);
                outPutJustEffectiveAddress(mod, rm, asmFile);
            }
            else
            {
                outPutJustEffectiveAddress(mod, rm, asmFile);
                printf(", %s", regValue);
            }
        }
    }
}

void immediateToRegisterMem(uint8_t buffer, FILE *asmFile)
{
    char w = buffer & 1;
    char s = (buffer >> 1) & 1;
    uint8_t nextBuffer;
    fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);
    char mod = nextBuffer >> 6;
    char rm = nextBuffer & GET_RM;
    char reg = (nextBuffer & GET_REG) >> 3;
    if (reg == 0b000)
    {
        printf("add ");
    }
    else if (reg == 0b101)
    {
        printf("sub ");
    }
    else if (reg == 0b111)
    {
        printf("cmp ");
    }

    int16_t data_16;
    int8_t data_8;
    if (mod == 0b11)
    {
        if (w)
        {
            printf("%s", namesWord[rm]);
        }
        else
        {
            printf("%s", namesByte[rm]);
        }
    }
    else
    {
        if (w)
        {
            printf("word ");
        }
        else
        {
            printf("byte ");
        }
        outPutJustEffectiveAddress(mod, rm, asmFile);
    }
    if (s == 0 && w == 1)
    {
        fread(&data_16, sizeof(int16_t), 1, asmFile);
    }
    else
    {
        fread(&data_8, sizeof(int8_t), 1, asmFile);
    }
    printf(", %d", (s == 0 && w == 1) ? data_16 : data_8);
}

void immediateToAcc(uint8_t buffer, FILE *asmFile)
{
    char w = buffer & GET_W;
    if (w)
    {
        int16_t data;
        fread(&data, sizeof(int16_t), 1, asmFile);
        printf("%s, %d", namesWord[0], data);
    }
    else
    {
        int8_t data;
        fread(&data, sizeof(int8_t), 1, asmFile);
        printf("%s, %d", namesByte[0], data);
    }
}

int main(int argc, char *argv[])
{
    if (!parseParam(argc, argv))
    {
        exit(EXIT_FAILURE);
    }

    /*only need to read 2 byte as a time becuase each instruction is 2 byte*/
    FILE *asmFile = fopen(programName, "rb");
    uint8_t buffer;
    if (asmFile)
    {
        printf("bits 16\n\n");
        while (!feof(asmFile))
        {
            buffer = fgetc(asmFile);
            // Check for the type of Instruction encoding
            if (buffer >> 2 == REGISTER_TO_REGISTER)
            {
                char isMov = (buffer & GET_MOV) == CHECK_MOV;
                char movd = buffer & GET_D;
                char movw = buffer & GET_W;
                uint8_t nextBuffer;
                fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);

                // Keep getting other attribute
                char mod = nextBuffer >> 6;
                char reg = (nextBuffer & GET_REG) >> 3;
                char rm = nextBuffer & GET_RM;
                printf("mov ");

                char *regValue = (movw) ? namesWord[reg] : namesByte[reg];
                char *rmValue = (movw) ? namesWord[rm] : namesByte[rm];

                if (mod == 0b11)
                {
                    if (movd)
                    {
                        printf("%s, %s", regValue, rmValue);
                    }
                    else
                    {
                        printf("%s, %s", rmValue, regValue);
                    }
                }
                else
                {
                    int16_t bitsDisplacement_16;
                    int8_t bitsDisplacement_8;
                    if (mod == 0b00)
                    {
                        if (rm == 0b110)
                        {
                            fread(&bitsDisplacement_16, sizeof(int16_t), 1, asmFile);
                            if (movd)
                            {
                                printf("%s, [%d]", regValue, bitsDisplacement_16);
                            }
                            else
                            {
                                printf("[%d], %s", bitsDisplacement_16, regValue);
                            }
                        }
                        else
                        {
                            if (movd)
                            {
                                printf("%s, [%s]", regValue, effectiveAddress[rm]);
                            }
                            else
                            {
                                printf("[%s], %s", effectiveAddress[rm], regValue);
                            }
                        }
                    }
                    else if (mod == 0b01)
                    {
                        fread(&bitsDisplacement_8, sizeof(int8_t), 1, asmFile);
                        outputEffectiveAddress(movw, movd, reg, rm, bitsDisplacement_8);
                    }
                    else if (mod == 0b10)
                    {
                        fread(&bitsDisplacement_16, sizeof(int16_t), 1, asmFile);
                        outputEffectiveAddress(movw, movd, reg, rm, bitsDisplacement_16);
                    }
                }
            }
            else if (buffer >> 4 == IMMEDIATE_TO_REGISTER)
            {
                char movw = buffer >> 3 & 1;
                char reg = buffer & 0b111;
                // 16 bit if w == 1
                // else 8 bit
                printf("mov ");
                if (movw)
                {
                    int16_t nextBuffer;
                    fread(&nextBuffer, sizeof(int16_t), 1, asmFile);
                    printf("%s, %d", namesWord[reg], nextBuffer);
                }
                else
                {
                    int8_t nextBuffer;
                    fread(&nextBuffer, sizeof(int8_t), 1, asmFile);
                    printf("%s, %d", namesByte[reg], nextBuffer);
                }
            }
            else if (buffer >> 1 == IMMEIDATE_TO_RESITER_MEMORY)
            {
                char movw = buffer & 1;

                uint8_t nextBuffer;
                fread(&nextBuffer, sizeof(uint8_t), 1, asmFile);
                char mod = nextBuffer >> 6;
                char rm = nextBuffer & GET_RM;

                int16_t data_16;
                int8_t data_8;
                printf("mov ");
                outPutJustEffectiveAddress(mod, rm, asmFile);

                if (movw)
                {
                    fread(&data_16, sizeof(int16_t), 1, asmFile);
                }
                else
                {
                    fread(&data_8, sizeof(int8_t), 1, asmFile);
                }
                printf((movw) ? ", word %d" : ", byte %d", movw ? data_16 : data_8);
            }
            else if (buffer >> 2 == MEM_TO_ACCUMULATOR)
            {
                char movd = buffer & GET_D;
                char movw = buffer & GET_W;
                printf("mov ");
                if (movw)
                {
                    int16_t data;
                    fread(&data, sizeof(int16_t), 1, asmFile);
                    if (movd)
                        printf("[%d], %s", data, namesWord[0]);
                    else
                        printf("%s, [%d]", namesWord[0], data);
                }
                else
                {
                    int8_t data;
                    fread(&data, sizeof(int8_t), 1, asmFile);
                    if (movd)
                        printf("[%d], %s", data, namesWord[0]);
                    else
                        printf("%s, [%d]", namesWord[0], data);
                }
            }
            else if (buffer >> 2 == ADD_REGISTER_TO_REGISTER)
            {
                printf("add ");
                registerToRegister(buffer, asmFile);
            }
            else if (buffer >> 2 == ADD_IMMEDIATE_TO_REGISTER_MEMORY)
            {
                immediateToRegisterMem(buffer, asmFile);
            }
            else if (buffer >> 1 == ADD_IMMEDIATE_TO_ACCUMULATOR)
            {
                printf("add ");
                immediateToAcc(buffer, asmFile);
            }
            else if (buffer >> 2 == SUB_REGISTER_TO_REGISTER)
            {
                printf("sub ");
                registerToRegister(buffer, asmFile);
            }
            else if (buffer >> 2 == SUB_IMMEDIATE_TO_REGISTER_MEMORY)
            {
                immediateToRegisterMem(buffer, asmFile);
            }
            else if (buffer >> 1 == SUB_IMMEDIATE_TO_ACCUMULATOR)
            {
                printf("sub ");
                immediateToAcc(buffer, asmFile);
            }
            else if (buffer >> 2 == CMP_REGISTER_TO_REGISTER)
            {
                printf("cmp ");
                registerToRegister(buffer, asmFile);
            }
            else if (buffer >> 2 == CMP_IMMEDIATE_TO_REGISTER_MEMORY)
            {
                immediateToRegisterMem(buffer, asmFile);
            }
            else if (buffer >> 1 == CMP_IMMEDIATE_TO_ACCUMULATOR)
            {
                printf("cmp ");
                immediateToAcc(buffer, asmFile);
            }
            else if (buffer >> 4 == IS_JUMP)
            {
                // Get the next 3 bits
                uint8_t next_3 = (buffer >> 1) & 0b111;
                // Get the last 1 bit
                uint8_t last_1 = buffer & 0x1;

                char *jumpInst = jumpInstruction[last_1][next_3];
                printf("%s ", jumpInst);
                int8_t nextBuffer;
                fread(&nextBuffer, sizeof(int8_t), 1, asmFile);
                printf("$+%d", nextBuffer + 2);
            } else if (buffer >> 2 == LOOP) {
                // Get the last 2 bits
                uint8_t last_2 = buffer & 0b11;
                char* loopInst = loopInstruction[last_2];
                printf("%s ", loopInst);
                int8_t nextBuffer;
                fread(&nextBuffer, sizeof(int8_t), 1, asmFile);
                printf("$+%d", nextBuffer + 2);
            }
            printf("\n");
        }
    }
    fclose(asmFile);
    return 0;
}