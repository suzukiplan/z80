#include "z80.hpp"
#include <ctype.h>

class MMU
{
  public:
    unsigned char RAM[0x10000];
    unsigned char IO[0x100];
    Z80* cpu;
    MMU()
    {
        memset(&RAM, 0, sizeof(RAM));
        memset(&IO, 0, sizeof(IO));
    }
};

unsigned char readByte(void* arg, unsigned short addr)
{
    return ((MMU*)arg)->RAM[addr];
}

void writeByte(void* arg, unsigned short addr, unsigned char value)
{
    if (addr < 0x2000) {
        return;
    }
    ((MMU*)arg)->RAM[addr] = value;
}

unsigned char inPort(void* arg, unsigned char port)
{
    return ((MMU*)arg)->IO[port];
}

void outPort(void* arg, unsigned char port, unsigned char value)
{
    ((MMU*)arg)->IO[port] = value;
}

static int testNumber;
static int expectClocks;
static FILE* file;
void executeTest(Z80* cpu, MMU* mmu, unsigned char op1, unsigned char op2, unsigned char op3, unsigned char op4, int clocks)
{
    expectClocks = clocks;
    mmu->RAM[0] = op1;
    mmu->RAM[1] = op2;
    mmu->RAM[2] = op3;
    mmu->RAM[3] = op4;
    cpu->reg.PC = 0;
    int actualClocks = cpu->execute(1);
    if (expectClocks != actualClocks) {
        fprintf(file, "TEST FAILED! (expected=%dHz, actual=%dHz)\n", expectClocks, actualClocks);
        fprintf(stdout, "TEST FAILED! (expected=%dHz, actual=%dHz)\n", expectClocks, actualClocks);
        exit(255);
    }
    cpu->reg.IFF = 0;
}

int main(int argc, char* argv[])
{
    file = fopen("test-clock.txt", "w");
    fprintf(file, "===== CLOCK CYCLE TEST =====\n");
    fprintf(stdout, "===== CLOCK CYCLE TEST =====\n");
    MMU mmu;
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu);
    mmu.cpu = &z80;
    z80.setDebugMessage([](void* arg, const char* msg) {
        testNumber++;
        fprintf(file, "TEST#%03d: %2dHz %s\n", testNumber, expectClocks, msg);
        fprintf(stdout, "TEST#%03d: %2dHz %s\n", testNumber, expectClocks, msg);
    });

    // checked clocks from https://userweb.alles.or.jp/chunichidenko/nd3setumeisyo/nd3_z80meirei.pdf
    executeTest(&z80, &mmu, 0b01000111, 0, 0, 0, 4);                 // LD B, A
    executeTest(&z80, &mmu, 0b01010110, 0, 0, 0, 7);                 // LD D, (HL)
    executeTest(&z80, &mmu, 0b01110000, 0, 0, 0, 7);                 // LD (HL), B
    executeTest(&z80, &mmu, 0b00001110, 0x56, 0, 0, 7);              // LD C, $56
    executeTest(&z80, &mmu, 0b00110110, 123, 0, 0, 10);              // LD (HL), 123
    executeTest(&z80, &mmu, 0b00001010, 0, 0, 0, 7);                 // LD A, (BC)
    executeTest(&z80, &mmu, 0b00011010, 0, 0, 0, 7);                 // LD A, (DE)
    executeTest(&z80, &mmu, 0b00111010, 0x34, 0x12, 0, 13);          // LD A, ($1234)
    executeTest(&z80, &mmu, 0b00000010, 0x34, 0x12, 0, 7);           // LD (BC), A
    executeTest(&z80, &mmu, 0b00010010, 0x34, 0x12, 0, 7);           // LD (DE), A
    executeTest(&z80, &mmu, 0b00110010, 0x78, 0x56, 0, 13);          // LD ($5678), A
    executeTest(&z80, &mmu, 0b11011101, 0b01011110, 4, 0, 19);       // LD E, (IX+4)
    executeTest(&z80, &mmu, 0b11111101, 0b01100110, 4, 0, 19);       // LD H, (IY+4)
    executeTest(&z80, &mmu, 0b11011101, 0b01110111, 7, 0, 19);       // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11111101, 0b01110001, 7, 0, 19);       // LD (IY+7), C
    executeTest(&z80, &mmu, 0b11011101, 0b00110110, 9, 100, 19);     // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11111101, 0b00110110, 9, 200, 19);     // LD (IY+7), C
    executeTest(&z80, &mmu, 0b00000001, 0xCD, 0xAB, 0, 10);          // LD BC, $ABCD
    executeTest(&z80, &mmu, 0b00010001, 0xCD, 0xAB, 0, 10);          // LD DE, $ABCD
    executeTest(&z80, &mmu, 0b00100001, 0xCD, 0xAB, 0, 10);          // LD HL, $ABCD
    executeTest(&z80, &mmu, 0b00110001, 0xCD, 0xAB, 0, 10);          // LD SP, $ABCD
    executeTest(&z80, &mmu, 0b11011101, 0b00100001, 0x34, 0x12, 14); // LD IX, $1234
    executeTest(&z80, &mmu, 0b11111101, 0b00100001, 0x78, 0x56, 14); // LD IY, $5678
    executeTest(&z80, &mmu, 0b00101010, 0x34, 0x12, 0, 16);          // LD HL, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x4B, 0x34, 0x12, 20);             // LD BC, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x5B, 0x34, 0x12, 20);             // LD DE, ($1234)
    executeTest(&z80, &mmu, 0b11101101, 0b01111011, 0x11, 0x00, 20); // LD SP, ($0011)
    executeTest(&z80, &mmu, 0b11011101, 0b00101010, 0x02, 0x00, 20); // LD IX, ($0002)
    executeTest(&z80, &mmu, 0b11111101, 0b00101010, 0x04, 0x00, 20); // LD IY, ($0004)
    executeTest(&z80, &mmu, 0x22, 0x34, 0x12, 0, 16);                // LD ($1234), HL
    executeTest(&z80, &mmu, 0xED, 0x43, 0x34, 0x12, 20);             // LD ($1234), BC
    executeTest(&z80, &mmu, 0xED, 0x53, 0x34, 0x12, 20);             // LD ($1234), DE
    executeTest(&z80, &mmu, 0b11101101, 0x73, 0x11, 0x00, 20);       // LD ($0011), SP
    executeTest(&z80, &mmu, 0b11011101, 0b00100010, 0x02, 0x00, 20); // LD ($0002), IX
    executeTest(&z80, &mmu, 0b11111101, 0b00100010, 0x04, 0x00, 20); // LD ($0004), IY
    executeTest(&z80, &mmu, 0b11111001, 0, 0, 0, 6);                 // LD SP, HL
    executeTest(&z80, &mmu, 0b11011101, 0b11111001, 0, 0, 10);       // LD SP, IX
    executeTest(&z80, &mmu, 0b11111101, 0b11111001, 0, 0, 10);       // LD SP, IY
    executeTest(&z80, &mmu, 0xC5, 0, 0, 0, 11);                      // PUSH BC
    executeTest(&z80, &mmu, 0xD5, 0, 0, 0, 11);                      // PUSH DE
    executeTest(&z80, &mmu, 0xE5, 0, 0, 0, 11);                      // PUSH HL
    executeTest(&z80, &mmu, 0xF5, 0, 0, 0, 11);                      // PUSH AF
    executeTest(&z80, &mmu, 0xDD, 0xE5, 0, 0, 15);                   // PUSH IX
    executeTest(&z80, &mmu, 0xFD, 0xE5, 0, 0, 15);                   // PUSH IX
    executeTest(&z80, &mmu, 0xC1, 0, 0, 0, 10);                      // POP BC
    executeTest(&z80, &mmu, 0xD1, 0, 0, 0, 10);                      // POP DE
    executeTest(&z80, &mmu, 0xE1, 0, 0, 0, 10);                      // POP HL
    executeTest(&z80, &mmu, 0xF1, 0, 0, 0, 10);                      // POP AF
    executeTest(&z80, &mmu, 0xDD, 0xE1, 0, 0, 14);                   // POP IX
    executeTest(&z80, &mmu, 0xFD, 0xE1, 0, 0, 14);                   // POP IX
    executeTest(&z80, &mmu, 0xEB, 0, 0, 0, 4);                       // EX DE, HL
    executeTest(&z80, &mmu, 0x08, 0, 0, 0, 4);                       // EX AF, AF'
    executeTest(&z80, &mmu, 0xD9, 0, 0, 0, 4);                       // EXX
    executeTest(&z80, &mmu, 0xE3, 0, 0, 0, 19);                      // EX (SP), HL
    executeTest(&z80, &mmu, 0xDD, 0xE3, 0, 0, 23);                   // EX (SP), IX
    executeTest(&z80, &mmu, 0xFD, 0xE3, 0, 0, 23);                   // EX (SP), IY
    executeTest(&z80, &mmu, 0xED, 0xA0, 0, 0, 16);                   // LDI
    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB0, 0, 0, 21);                   // LDIR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB0, 0, 0, 16);                   // LDIR (--BC == 0)
    executeTest(&z80, &mmu, 0xED, 0xA8, 0, 0, 16);                   // LDD
    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB8, 0, 0, 21);                   // LDDR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB8, 0, 0, 16);                   // LDDR (--BC == 0)
    executeTest(&z80, &mmu, 0xED, 0xA1, 0, 0, 16);                   // CPI
    z80.reg.pair.A = 123;                                            // (setup register for test)
    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB1, 0, 0, 21);                   // CPIR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB1, 0, 0, 16);                   // CPIR (--BC == 0)
    executeTest(&z80, &mmu, 0xED, 0xA9, 0, 0, 16);                   // CPD
    z80.reg.pair.A = 123;                                            // (setup register for test)
    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB9, 0, 0, 21);                   // CPDR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB9, 0, 0, 16);                   // CPDR (--BC == 0)
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 4);                       // ADD A, B
    executeTest(&z80, &mmu, 0xC6, 9, 0, 0, 7);                       // ADD A, n
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 7);                       // ADD A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 19);                   // ADD A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 19);                   // ADD A, (IX+d)
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 4);                       // ADC A, B
    executeTest(&z80, &mmu, 0xCE, 9, 0, 0, 7);                       // ADC A, n
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 7);                       // ADC A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 19);                   // ADC A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 19);                   // ADC A, (IX+d)
    executeTest(&z80, &mmu, 0x90, 0, 0, 0, 4);                       // SUB A, B
    executeTest(&z80, &mmu, 0xD6, 9, 0, 0, 7);                       // SUB A, n
    executeTest(&z80, &mmu, 0x96, 0, 0, 0, 7);                       // SUB A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x96, 5, 0, 19);                   // SUB A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x96, 5, 0, 19);                   // SUB A, (IX+d)
    executeTest(&z80, &mmu, 0x98, 0, 0, 0, 4);                       // SBC A, B
    executeTest(&z80, &mmu, 0xDE, 9, 0, 0, 7);                       // SBC A, n
    executeTest(&z80, &mmu, 0x9E, 0, 0, 0, 7);                       // SBC A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x9E, 5, 0, 19);                   // SBC A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x9E, 5, 0, 19);                   // SBC A, (IX+d)
    executeTest(&z80, &mmu, 0xA0, 0, 0, 0, 4);                       // AND A, B
    executeTest(&z80, &mmu, 0xE6, 9, 0, 0, 7);                       // AND A, n
    executeTest(&z80, &mmu, 0xA6, 0, 0, 0, 7);                       // AND A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xA6, 5, 0, 19);                   // AND A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xA6, 5, 0, 19);                   // AND A, (IX+d)
    executeTest(&z80, &mmu, 0xB0, 0, 0, 0, 4);                       // OR A, B
    executeTest(&z80, &mmu, 0xF6, 9, 0, 0, 7);                       // OR A, n
    executeTest(&z80, &mmu, 0xB6, 0, 0, 0, 7);                       // OR A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xB6, 5, 0, 19);                   // OR A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xB6, 5, 0, 19);                   // OR A, (IX+d)
    executeTest(&z80, &mmu, 0xA8, 0, 0, 0, 4);                       // XOR A, B
    executeTest(&z80, &mmu, 0xEE, 9, 0, 0, 7);                       // XOR A, n
    executeTest(&z80, &mmu, 0xAE, 0, 0, 0, 7);                       // XOR A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xAE, 5, 0, 19);                   // XOR A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xAE, 5, 0, 19);                   // XOR A, (IX+d)
    executeTest(&z80, &mmu, 0xB8, 0, 0, 0, 4);                       // CP A, B
    executeTest(&z80, &mmu, 0xFE, 9, 0, 0, 7);                       // CP A, n
    executeTest(&z80, &mmu, 0xBE, 0, 0, 0, 7);                       // CP A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xBE, 5, 0, 19);                   // CP A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xBE, 5, 0, 19);                   // CP A, (IX+d)
    executeTest(&z80, &mmu, 0x04, 0, 0, 0, 4);                       // INC B
    executeTest(&z80, &mmu, 0x34, 0, 0, 0, 11);                      // INC (HL)
    executeTest(&z80, &mmu, 0xDD, 0x34, 3, 0, 23);                   // INC (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x34, 6, 0, 23);                   // INC (IY+d)
    executeTest(&z80, &mmu, 0x05, 0, 0, 0, 4);                       // DEC B
    executeTest(&z80, &mmu, 0x35, 0, 0, 0, 11);                      // DEC (HL)
    executeTest(&z80, &mmu, 0xDD, 0x35, 3, 0, 23);                   // DEC (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x35, 6, 0, 23);                   // DEC (IY+d)
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 4);                       // DAA
    executeTest(&z80, &mmu, 0x2F, 0, 0, 0, 4);                       // CPL
    executeTest(&z80, &mmu, 0xED, 0x44, 0, 0, 8);                    // NEG
    executeTest(&z80, &mmu, 0x3F, 0, 0, 0, 4);                       // CCF
    executeTest(&z80, &mmu, 0x37, 0, 0, 0, 4);                       // SCF
    executeTest(&z80, &mmu, 0x00, 0, 0, 0, 4);                       // NOP
    executeTest(&z80, &mmu, 0b01110110, 0, 0, 0, 4);                 // HALT
    z80.reg.IFF = 0;                                                 // (setup register for test)
    executeTest(&z80, &mmu, 0x09, 0, 0, 0, 11);                      // ADD HL, BC
    executeTest(&z80, &mmu, 0x19, 0, 0, 0, 11);                      // ADD HL, DE
    executeTest(&z80, &mmu, 0x29, 0, 0, 0, 11);                      // ADD HL, HL
    executeTest(&z80, &mmu, 0x39, 0, 0, 0, 11);                      // ADD HL, SP
    executeTest(&z80, &mmu, 0xED, 0x4A, 0, 0, 15);                   // ADC HL, BC
    executeTest(&z80, &mmu, 0xED, 0x5A, 0, 0, 15);                   // ADC HL, DE
    executeTest(&z80, &mmu, 0xED, 0x6A, 0, 0, 15);                   // ADC HL, HL
    executeTest(&z80, &mmu, 0xED, 0x7A, 0, 0, 15);                   // ADC HL, SP
    executeTest(&z80, &mmu, 0xED, 0x42, 0, 0, 15);                   // SBC HL, BC
    executeTest(&z80, &mmu, 0xED, 0x52, 0, 0, 15);                   // SBC HL, DE
    executeTest(&z80, &mmu, 0xED, 0x62, 0, 0, 15);                   // SBC HL, HL
    executeTest(&z80, &mmu, 0xED, 0x72, 0, 0, 15);                   // SBC HL, SP
    executeTest(&z80, &mmu, 0xDD, 0x09, 0, 0, 15);                   // ADD IX, BC
    executeTest(&z80, &mmu, 0xDD, 0x19, 0, 0, 15);                   // ADD IX, DE
    executeTest(&z80, &mmu, 0xDD, 0x29, 0, 0, 15);                   // ADD IX, HL
    executeTest(&z80, &mmu, 0xDD, 0x39, 0, 0, 15);                   // ADD IX, SP
    executeTest(&z80, &mmu, 0xFD, 0x09, 0, 0, 15);                   // ADD IY, BC
    executeTest(&z80, &mmu, 0xFD, 0x19, 0, 0, 15);                   // ADD IY, DE
    executeTest(&z80, &mmu, 0xFD, 0x29, 0, 0, 15);                   // ADD IY, HL
    executeTest(&z80, &mmu, 0xFD, 0x39, 0, 0, 15);                   // ADD IY, SP
    executeTest(&z80, &mmu, 0x03, 0, 0, 0, 6);                       // INC BC
    executeTest(&z80, &mmu, 0x13, 0, 0, 0, 6);                       // INC DE
    executeTest(&z80, &mmu, 0x23, 0, 0, 0, 6);                       // INC HL
    executeTest(&z80, &mmu, 0x33, 0, 0, 0, 6);                       // INC SP
    executeTest(&z80, &mmu, 0xDD, 0x23, 0, 0, 10);                   // INC HL
    executeTest(&z80, &mmu, 0xFD, 0x23, 0, 0, 10);                   // INC HL
    executeTest(&z80, &mmu, 0x0B, 0, 0, 0, 6);                       // DEC BC
    executeTest(&z80, &mmu, 0x1B, 0, 0, 0, 6);                       // DEC DE
    executeTest(&z80, &mmu, 0x2B, 0, 0, 0, 6);                       // DEC HL
    executeTest(&z80, &mmu, 0x3B, 0, 0, 0, 6);                       // DEC SP
    executeTest(&z80, &mmu, 0xDD, 0x2B, 0, 0, 10);                   // DEC HL
    executeTest(&z80, &mmu, 0xFD, 0x2B, 0, 0, 10);                   // DEC HL
    executeTest(&z80, &mmu, 0x07, 0, 0, 0, 4);                       // RLCA
    executeTest(&z80, &mmu, 0x17, 0, 0, 0, 4);                       // RLA
    executeTest(&z80, &mmu, 0x0F, 0, 0, 0, 4);                       // RRCA
    executeTest(&z80, &mmu, 0x1F, 0, 0, 0, 4);                       // RRA
    executeTest(&z80, &mmu, 0xCB, 0x00, 0, 0, 8);                    // RLC B
    executeTest(&z80, &mmu, 0xCB, 0x06, 0, 0, 15);                   // RLC (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x06, 23);              // RLC (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x06, 23);              // RLC (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x10, 0, 0, 8);                    // RL B
    executeTest(&z80, &mmu, 0xCB, 0x16, 0, 0, 15);                   // RL (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x16, 23);              // RL (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x16, 23);              // RL (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x08, 0, 0, 8);                    // RRC B
    executeTest(&z80, &mmu, 0xCB, 0x0E, 0, 0, 15);                   // RRC (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x0E, 23);              // RRC (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x0E, 23);              // RRC (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x18, 0, 0, 8);                    // RR B
    executeTest(&z80, &mmu, 0xCB, 0x1E, 0, 0, 15);                   // RR (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x1E, 23);              // RR (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x1E, 23);              // RR (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x20, 0, 0, 8);                    // SLA B
    executeTest(&z80, &mmu, 0xCB, 0x26, 0, 0, 15);                   // SLA (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x26, 23);              // SLA (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x26, 23);              // SLA (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x28, 0, 0, 8);                    // SRA B
    executeTest(&z80, &mmu, 0xCB, 0x2E, 0, 0, 15);                   // SRA (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x2E, 23);              // SRA (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x2E, 23);              // SRA (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x38, 0, 0, 8);                    // SRL B
    executeTest(&z80, &mmu, 0xCB, 0x3E, 0, 0, 15);                   // SRL (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x3E, 23);              // SRL (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x3E, 23);              // SRL (IY+123)
    executeTest(&z80, &mmu, 0xED, 0x6F, 0, 0, 18);                   // RLD
    executeTest(&z80, &mmu, 0xED, 0x67, 0, 0, 18);                   // RRD
    executeTest(&z80, &mmu, 0xCB, 0b01000000, 0, 0, 8);              // BIT b, r
    executeTest(&z80, &mmu, 0xCB, 0b01000110, 0, 0, 12);             // BIT b, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 129, 0b01000110, 20);        // BIT b, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 129, 0b01000110, 20);        // BIT b, (IY+d)
    executeTest(&z80, &mmu, 0xCB, 0b11000000, 0, 0, 8);              // SET b, r
    executeTest(&z80, &mmu, 0xCB, 0b11000110, 0, 0, 15);             // SET b, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 129, 0b11000110, 23);        // SET b, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 129, 0b11000110, 23);        // SET b, (IY+d)
    executeTest(&z80, &mmu, 0xCB, 0b10000000, 0, 0, 8);              // RES b, r
    executeTest(&z80, &mmu, 0xCB, 0b10000110, 0, 0, 15);             // RES b, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 129, 0b10000110, 23);        // RES b, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 129, 0b10000110, 23);        // RES b, (IY+d)
    executeTest(&z80, &mmu, 0xC3, 0x34, 0x12, 0, 10);                // JP nn
    executeTest(&z80, &mmu, 0xC2, 0x34, 0x12, 0, 10);                // JP NZ nn
    executeTest(&z80, &mmu, 0xCA, 0x34, 0x12, 0, 10);                // JP Z nn
    executeTest(&z80, &mmu, 0xD2, 0x34, 0x12, 0, 10);                // JP NC nn
    executeTest(&z80, &mmu, 0xDA, 0x34, 0x12, 0, 10);                // JP C nn
    executeTest(&z80, &mmu, 0xE2, 0x34, 0x12, 0, 10);                // JP PO nn
    executeTest(&z80, &mmu, 0xEA, 0x34, 0x12, 0, 10);                // JP PE nn
    executeTest(&z80, &mmu, 0xF2, 0x34, 0x12, 0, 10);                // JP P nn
    executeTest(&z80, &mmu, 0xFA, 0x34, 0x12, 0, 10);                // JP M nn
    executeTest(&z80, &mmu, 0x18, 79, 0, 0, 12);                     // JR e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x38, 79, 0, 0, 7);                      // JR C, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x38, 79, 0, 0, 12);                     // JR C, e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x30, 79, 0, 0, 12);                     // JR NC, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x30, 79, 0, 0, 7);                      // JR NC, e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x28, 79, 0, 0, 7);                      // JR Z, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x28, 79, 0, 0, 12);                     // JR Z, e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x20, 79, 0, 0, 12);                     // JR NC, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x20, 79, 0, 0, 7);                      // JR NC, e
    executeTest(&z80, &mmu, 0xE9, 0, 0, 0, 4);                       // JP (HL)
    executeTest(&z80, &mmu, 0xDD, 0xE9, 0, 0, 8);                    // JP (IX)
    executeTest(&z80, &mmu, 0xFD, 0xE9, 0, 0, 8);                    // JP (IY)
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x10, 78, 0, 0, 13);                     // DJNZ
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x10, 78, 0, 0, 8);                      // DJNZ
    executeTest(&z80, &mmu, 0xCD, 0x34, 0x12, 0, 17);                // CALL nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xC4, 0x34, 0x12, 0, 17);                // CALL NZ, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xC4, 0x34, 0x12, 0, 10);                // CALL NZ, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xCC, 0x34, 0x12, 0, 10);                // CALL Z, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xCC, 0x34, 0x12, 0, 17);                // CALL Z, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xD4, 0x34, 0x12, 0, 17);                // CALL NC, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xD4, 0x34, 0x12, 0, 10);                // CALL CZ, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xDC, 0x34, 0x12, 0, 10);                // CALL C, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xDC, 0x34, 0x12, 0, 17);                // CALL C, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xE4, 0x34, 0x12, 0, 17);                // CALL PO, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xE4, 0x34, 0x12, 0, 10);                // CALL PO, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xEC, 0x34, 0x12, 0, 10);                // CALL PE, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xEC, 0x34, 0x12, 0, 17);                // CALL PE, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xF4, 0x34, 0x12, 0, 17);                // CALL P, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xF4, 0x34, 0x12, 0, 10);                // CALL P, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xFC, 0x34, 0x12, 0, 10);                // CALL M, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xFC, 0x34, 0x12, 0, 17);                // CALL M, nn
    executeTest(&z80, &mmu, 0xC9, 0, 0, 0, 10);                      // RET
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 11);                      // RET NZ
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 5);                       // RET NZ
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xC8, 0, 0, 0, 5);                       // RET Z
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xC8, 0, 0, 0, 11);                      // RET Z
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xD0, 0, 0, 0, 11);                      // RET NC
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xD0, 0, 0, 0, 5);                       // RET NC
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xD8, 0, 0, 0, 5);                       // RET C
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xD8, 0, 0, 0, 11);                      // RET C
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xE0, 0, 0, 0, 11);                      // RET PO
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xE0, 0, 0, 0, 5);                       // RET PO
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xE8, 0, 0, 0, 5);                       // RET PE
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xE8, 0, 0, 0, 11);                      // RET PE
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xF0, 0, 0, 0, 11);                      // RET P
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xF0, 0, 0, 0, 5);                       // RET P
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xF8, 0, 0, 0, 5);                       // RET M
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xF8, 0, 0, 0, 11);                      // RET M
    executeTest(&z80, &mmu, 0xED, 0x4D, 0, 0, 14);                   // RETI
    executeTest(&z80, &mmu, 0xED, 0x45, 0, 0, 14);                   // RETN
    executeTest(&z80, &mmu, 0xC7, 0, 0, 0, 11);                      // RST 0
    executeTest(&z80, &mmu, 0xDB, 1, 0, 0, 11);                      // IN (n)
    executeTest(&z80, &mmu, 0xED, 0x40, 0, 0, 12);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x48, 0, 0, 12);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x50, 0, 0, 12);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x58, 0, 0, 12);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x60, 0, 0, 12);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x68, 0, 0, 12);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x78, 0, 0, 12);                   // IN r, (C)
    executeTest(&z80, &mmu, 0xED, 0xA2, 0, 0, 16);                   // INI
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB2, 0, 0, 21);                   // INIR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB2, 0, 0, 16);                   // INIR
    executeTest(&z80, &mmu, 0xED, 0xAA, 0, 0, 16);                   // IND
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xBA, 0, 0, 21);                   // INDR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xBA, 0, 0, 16);                   // INDR
    executeTest(&z80, &mmu, 0xD3, 0, 0, 0, 11);                      // OUT (n), A
    executeTest(&z80, &mmu, 0xED, 0x41, 0, 0, 12);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x49, 0, 0, 12);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x51, 0, 0, 12);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x59, 0, 0, 12);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x61, 0, 0, 12);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x69, 0, 0, 12);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x79, 0, 0, 12);                   // OUT (C), r
    executeTest(&z80, &mmu, 0xED, 0xA3, 0, 0, 16);                   // OUTI
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB3, 0, 0, 21);                   // OTIR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB3, 0, 0, 16);                   // OTIR
    executeTest(&z80, &mmu, 0xED, 0xAB, 0, 0, 16);                   // OUTD
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xBB, 0, 0, 21);                   // OTDR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xBB, 0, 0, 16);                   // OTDR
    executeTest(&z80, &mmu, 0b11110011, 0, 0, 0, 4);                 // DI
    executeTest(&z80, &mmu, 0b11111011, 0, 0, 0, 4);                 // EI
    executeTest(&z80, &mmu, 0b11101101, 0b01000110, 0, 0, 8);        // IM 0
    executeTest(&z80, &mmu, 0b11101101, 0b01010110, 0, 0, 8);        // IM 1
    executeTest(&z80, &mmu, 0b11101101, 0b01011110, 0, 0, 8);        // IM 2

    // undocumented instructions
    executeTest(&z80, &mmu, 0xDD, 0x24, 0, 0, 8);       // INC IXH
    executeTest(&z80, &mmu, 0xFD, 0x24, 0, 0, 8);       // INC IYH
    executeTest(&z80, &mmu, 0xDD, 0x2C, 0, 0, 8);       // INC IXL
    executeTest(&z80, &mmu, 0xFD, 0x2C, 0, 0, 8);       // INC IYL
    executeTest(&z80, &mmu, 0xDD, 0x25, 0, 0, 8);       // DEC IXH
    executeTest(&z80, &mmu, 0xFD, 0x25, 0, 0, 8);       // DEC IYH
    executeTest(&z80, &mmu, 0xDD, 0x2D, 0, 0, 8);       // DEC IXL
    executeTest(&z80, &mmu, 0xFD, 0x2D, 0, 0, 8);       // DEC IYL
    executeTest(&z80, &mmu, 0xDD, 0x26, 123, 0, 11);    // LD IXH, n
    executeTest(&z80, &mmu, 0xFD, 0x26, 123, 0, 11);    // LD IXH, n
    executeTest(&z80, &mmu, 0xDD, 0x2E, 123, 0, 11);    // LD IXL, n
    executeTest(&z80, &mmu, 0xFD, 0x2E, 123, 0, 11);    // LD IXL, n
    executeTest(&z80, &mmu, 0xDD, 0x67, 0, 0, 8);       // LD IXH, A
    executeTest(&z80, &mmu, 0xDD, 0x60, 0, 0, 8);       // LD IXH, B
    executeTest(&z80, &mmu, 0xDD, 0x61, 0, 0, 8);       // LD IXH, C
    executeTest(&z80, &mmu, 0xDD, 0x62, 0, 0, 8);       // LD IXH, D
    executeTest(&z80, &mmu, 0xDD, 0x63, 0, 0, 8);       // LD IXH, E
    executeTest(&z80, &mmu, 0xDD, 0x64, 0, 0, 8);       // LD IXH, IXH
    executeTest(&z80, &mmu, 0xDD, 0x65, 0, 0, 8);       // LD IXH, IXL
    executeTest(&z80, &mmu, 0xDD, 0x6F, 0, 0, 8);       // LD IXL, A
    executeTest(&z80, &mmu, 0xDD, 0x68, 0, 0, 8);       // LD IXL, B
    executeTest(&z80, &mmu, 0xDD, 0x69, 0, 0, 8);       // LD IXL, C
    executeTest(&z80, &mmu, 0xDD, 0x6A, 0, 0, 8);       // LD IXL, D
    executeTest(&z80, &mmu, 0xDD, 0x6B, 0, 0, 8);       // LD IXL, E
    executeTest(&z80, &mmu, 0xDD, 0x6C, 0, 0, 8);       // LD IXL, IXH
    executeTest(&z80, &mmu, 0xDD, 0x6D, 0, 0, 8);       // LD IXL, IXL
    executeTest(&z80, &mmu, 0xFD, 0x67, 0, 0, 8);       // LD IYH, A
    executeTest(&z80, &mmu, 0xFD, 0x60, 0, 0, 8);       // LD IYH, B
    executeTest(&z80, &mmu, 0xFD, 0x61, 0, 0, 8);       // LD IYH, C
    executeTest(&z80, &mmu, 0xFD, 0x62, 0, 0, 8);       // LD IYH, D
    executeTest(&z80, &mmu, 0xFD, 0x63, 0, 0, 8);       // LD IYH, E
    executeTest(&z80, &mmu, 0xFD, 0x64, 0, 0, 8);       // LD IYH, IYH
    executeTest(&z80, &mmu, 0xFD, 0x65, 0, 0, 8);       // LD IYH, IYL
    executeTest(&z80, &mmu, 0xFD, 0x6F, 0, 0, 8);       // LD IYL, A
    executeTest(&z80, &mmu, 0xFD, 0x68, 0, 0, 8);       // LD IYL, B
    executeTest(&z80, &mmu, 0xFD, 0x69, 0, 0, 8);       // LD IYL, C
    executeTest(&z80, &mmu, 0xFD, 0x6A, 0, 0, 8);       // LD IYL, D
    executeTest(&z80, &mmu, 0xFD, 0x6B, 0, 0, 8);       // LD IYL, E
    executeTest(&z80, &mmu, 0xFD, 0x6C, 0, 0, 8);       // LD IYL, IYH
    executeTest(&z80, &mmu, 0xFD, 0x6D, 0, 0, 8);       // LD IYL, IYL
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 8);       // ADD A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 8);       // ADD A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 8);       // ADD A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 8);       // ADD A, IYL
    executeTest(&z80, &mmu, 0xDD, 0x7C, 0, 0, 8);       // LD A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x7D, 0, 0, 8);       // LD A, IXL
    executeTest(&z80, &mmu, 0xDD, 0x44, 0, 0, 8);       // LD B, IXH
    executeTest(&z80, &mmu, 0xDD, 0x45, 0, 0, 8);       // LD B, IXL
    executeTest(&z80, &mmu, 0xDD, 0x4C, 0, 0, 8);       // LD C, IXH
    executeTest(&z80, &mmu, 0xDD, 0x4D, 0, 0, 8);       // LD C, IXL
    executeTest(&z80, &mmu, 0xDD, 0x54, 0, 0, 8);       // LD D, IXH
    executeTest(&z80, &mmu, 0xDD, 0x55, 0, 0, 8);       // LD D, IXL
    executeTest(&z80, &mmu, 0xDD, 0x5C, 0, 0, 8);       // LD E, IXH
    executeTest(&z80, &mmu, 0xDD, 0x5D, 0, 0, 8);       // LD E, IXL
    executeTest(&z80, &mmu, 0xFD, 0x7C, 0, 0, 8);       // LD A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x7D, 0, 0, 8);       // LD A, IYL
    executeTest(&z80, &mmu, 0xFD, 0x44, 0, 0, 8);       // LD B, IYH
    executeTest(&z80, &mmu, 0xFD, 0x45, 0, 0, 8);       // LD B, IYL
    executeTest(&z80, &mmu, 0xFD, 0x4C, 0, 0, 8);       // LD C, IYH
    executeTest(&z80, &mmu, 0xFD, 0x4D, 0, 0, 8);       // LD C, IYL
    executeTest(&z80, &mmu, 0xFD, 0x54, 0, 0, 8);       // LD D, IYH
    executeTest(&z80, &mmu, 0xFD, 0x55, 0, 0, 8);       // LD D, IYL
    executeTest(&z80, &mmu, 0xFD, 0x5C, 0, 0, 8);       // LD E, IYH
    executeTest(&z80, &mmu, 0xFD, 0x5D, 0, 0, 8);       // LD E, IYL
    executeTest(&z80, &mmu, 0xDD, 0x8C, 0, 0, 8);       // ADC A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x8D, 0, 0, 8);       // ADC A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x8C, 0, 0, 8);       // ADC A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x8D, 0, 0, 8);       // ADC A, IYL
    executeTest(&z80, &mmu, 0xDD, 0x94, 0, 0, 8);       // SUB A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x95, 0, 0, 8);       // SUB A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x94, 0, 0, 8);       // SUB A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x95, 0, 0, 8);       // SUB A, IYL
    executeTest(&z80, &mmu, 0xDD, 0x9C, 0, 0, 8);       // SBC A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x9D, 0, 0, 8);       // SBC A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x9C, 0, 0, 8);       // SBC A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x9D, 0, 0, 8);       // SBC A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xA4, 0, 0, 8);       // AND A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xA5, 0, 0, 8);       // AND A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xA4, 0, 0, 8);       // AND A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xA5, 0, 0, 8);       // AND A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xB4, 0, 0, 8);       // OR A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xB5, 0, 0, 8);       // OR A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xB4, 0, 0, 8);       // OR A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xB5, 0, 0, 8);       // OR A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xAC, 0, 0, 8);       // XOR A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xAD, 0, 0, 8);       // XOR A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xAC, 0, 0, 8);       // XOR A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xAD, 0, 0, 8);       // XOR A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xBC, 0, 0, 8);       // CP A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xBD, 0, 0, 8);       // CP A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xBC, 0, 0, 8);       // CP A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xBD, 0, 0, 8);       // CP A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x07, 23); // RLC (IX+d) with LD A
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x00, 23); // RLC (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x01, 23); // RLC (IX+d) with LD C
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x02, 23); // RLC (IX+d) with LD D
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x03, 23); // RLC (IX+d) with LD E
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x04, 23); // RLC (IX+d) with LD H
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x05, 23); // RLC (IX+d) with LD L
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x08, 23); // RRC (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x10, 23); // RL (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x18, 23); // RR (IX+d) with LD B

    return 0;
}
