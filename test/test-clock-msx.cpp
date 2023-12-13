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

unsigned char inPort(void* arg, unsigned short port)
{
    return ((MMU*)arg)->IO[port];
}

void outPort(void* arg, unsigned short port, unsigned char value)
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
    file = fopen("test-clock-msx.txt", "w");
    fprintf(file, "===== CLOCK CYCLE TEST for MSX =====\n");
    fprintf(stdout, "===== CLOCK CYCLE TEST for MSX =====\n");
    MMU mmu;
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu);
    z80.wtc.fetch = 1;
    z80.wtc.fetchM = 1;
    mmu.cpu = &z80;
    z80.setDebugMessage([](void* arg, const char* msg) {
        testNumber++;
        fprintf(file, "TEST#%03d: %2dHz %s\n", testNumber, expectClocks, msg);
        fprintf(stdout, "TEST#%03d: %2dHz %s\n", testNumber, expectClocks, msg);
    });

    // checked clocks from https://userweb.alles.or.jp/chunichidenko/nd3setumeisyo/nd3_z80meirei.pdf
    executeTest(&z80, &mmu, 0b01000111, 0, 0, 0, 5);                 // LD B, A
    executeTest(&z80, &mmu, 0b01010110, 0, 0, 0, 8);                 // LD D, (HL)
    executeTest(&z80, &mmu, 0b01110000, 0, 0, 0, 8);                 // LD (HL), B
    executeTest(&z80, &mmu, 0b00001110, 0x56, 0, 0, 8);              // LD C, $56
    executeTest(&z80, &mmu, 0b00110110, 123, 0, 0, 11);              // LD (HL), 123
    executeTest(&z80, &mmu, 0b00001010, 0, 0, 0, 8);                 // LD A, (BC)
    executeTest(&z80, &mmu, 0b00011010, 0, 0, 0, 8);                 // LD A, (DE)
    executeTest(&z80, &mmu, 0b00111010, 0x34, 0x12, 0, 14);          // LD A, ($1234)
    executeTest(&z80, &mmu, 0b00000010, 0x34, 0x12, 0, 8);           // LD (BC), A
    executeTest(&z80, &mmu, 0b00010010, 0x34, 0x12, 0, 8);           // LD (DE), A
    executeTest(&z80, &mmu, 0b00110010, 0x78, 0x56, 0, 14);          // LD ($5678), A
    executeTest(&z80, &mmu, 0b11011101, 0b01011110, 4, 0, 21);       // LD E, (IX+4)
    executeTest(&z80, &mmu, 0b11111101, 0b01100110, 4, 0, 21);       // LD H, (IY+4)
    executeTest(&z80, &mmu, 0b11011101, 0b01110111, 7, 0, 21);       // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11111101, 0b01110001, 7, 0, 21);       // LD (IY+7), C
    executeTest(&z80, &mmu, 0b11011101, 0b00110110, 9, 100, 21);     // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11111101, 0b00110110, 9, 200, 21);     // LD (IY+7), C
    executeTest(&z80, &mmu, 0b00000001, 0xCD, 0xAB, 0, 11);          // LD BC, $ABCD
    executeTest(&z80, &mmu, 0b00010001, 0xCD, 0xAB, 0, 11);          // LD DE, $ABCD
    executeTest(&z80, &mmu, 0b00100001, 0xCD, 0xAB, 0, 11);          // LD HL, $ABCD
    executeTest(&z80, &mmu, 0b00110001, 0xCD, 0xAB, 0, 11);          // LD SP, $ABCD
    executeTest(&z80, &mmu, 0b11011101, 0b00100001, 0x34, 0x12, 16); // LD IX, $1234
    executeTest(&z80, &mmu, 0b11111101, 0b00100001, 0x78, 0x56, 16); // LD IY, $5678
    executeTest(&z80, &mmu, 0b00101010, 0x34, 0x12, 0, 17);          // LD HL, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x4B, 0x34, 0x12, 22);             // LD BC, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x5B, 0x34, 0x12, 22);             // LD DE, ($1234)
    executeTest(&z80, &mmu, 0b11101101, 0b01111011, 0x11, 0x00, 22); // LD SP, ($0011)
    executeTest(&z80, &mmu, 0b11011101, 0b00101010, 0x02, 0x00, 22); // LD IX, ($0002)
    executeTest(&z80, &mmu, 0b11111101, 0b00101010, 0x04, 0x00, 22); // LD IY, ($0004)
    executeTest(&z80, &mmu, 0x22, 0x34, 0x12, 0, 17);                // LD ($1234), HL
    executeTest(&z80, &mmu, 0xED, 0x43, 0x34, 0x12, 22);             // LD ($1234), BC
    executeTest(&z80, &mmu, 0xED, 0x53, 0x34, 0x12, 22);             // LD ($1234), DE
    executeTest(&z80, &mmu, 0b11101101, 0x73, 0x11, 0x00, 22);       // LD ($0011), SP
    executeTest(&z80, &mmu, 0b11011101, 0b00100010, 0x02, 0x00, 22); // LD ($0002), IX
    executeTest(&z80, &mmu, 0b11111101, 0b00100010, 0x04, 0x00, 22); // LD ($0004), IY
    executeTest(&z80, &mmu, 0b11111001, 0, 0, 0, 7);                 // LD SP, HL
    executeTest(&z80, &mmu, 0b11011101, 0b11111001, 0, 0, 12);       // LD SP, IX
    executeTest(&z80, &mmu, 0b11111101, 0b11111001, 0, 0, 12);       // LD SP, IY
    executeTest(&z80, &mmu, 0xC5, 0, 0, 0, 12);                      // PUSH BC
    executeTest(&z80, &mmu, 0xD5, 0, 0, 0, 12);                      // PUSH DE
    executeTest(&z80, &mmu, 0xE5, 0, 0, 0, 12);                      // PUSH HL
    executeTest(&z80, &mmu, 0xF5, 0, 0, 0, 12);                      // PUSH AF
    executeTest(&z80, &mmu, 0xDD, 0xE5, 0, 0, 17);                   // PUSH IX
    executeTest(&z80, &mmu, 0xFD, 0xE5, 0, 0, 17);                   // PUSH IX
    executeTest(&z80, &mmu, 0xC1, 0, 0, 0, 11);                      // POP BC
    executeTest(&z80, &mmu, 0xD1, 0, 0, 0, 11);                      // POP DE
    executeTest(&z80, &mmu, 0xE1, 0, 0, 0, 11);                      // POP HL
    executeTest(&z80, &mmu, 0xF1, 0, 0, 0, 11);                      // POP AF
    executeTest(&z80, &mmu, 0xDD, 0xE1, 0, 0, 16);                   // POP IX
    executeTest(&z80, &mmu, 0xFD, 0xE1, 0, 0, 16);                   // POP IX
    executeTest(&z80, &mmu, 0xEB, 0, 0, 0, 5);                       // EX DE, HL
    executeTest(&z80, &mmu, 0x08, 0, 0, 0, 5);                       // EX AF, AF'
    executeTest(&z80, &mmu, 0xD9, 0, 0, 0, 5);                       // EXX
    executeTest(&z80, &mmu, 0xE3, 0, 0, 0, 20);                      // EX (SP), HL
    executeTest(&z80, &mmu, 0xDD, 0xE3, 0, 0, 25);                   // EX (SP), IX
    executeTest(&z80, &mmu, 0xFD, 0xE3, 0, 0, 25);                   // EX (SP), IY
    executeTest(&z80, &mmu, 0xED, 0xA0, 0, 0, 18);                   // LDI

    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB0, 0, 0, 23);                   // LDIR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB0, 0, 0, 18);                   // LDIR (--BC == 0)
    executeTest(&z80, &mmu, 0xED, 0xA8, 0, 0, 18);                   // LDD
    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB8, 0, 0, 23);                   // LDDR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB8, 0, 0, 18);                   // LDDR (--BC == 0)
    executeTest(&z80, &mmu, 0xED, 0xA1, 0, 0, 18);                   // CPI
    z80.reg.pair.A = 123;                                            // (setup register for test)
    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB1, 0, 0, 23);                   // CPIR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB1, 0, 0, 18);                   // CPIR (--BC == 0)
    executeTest(&z80, &mmu, 0xED, 0xA9, 0, 0, 18);                   // CPD
    z80.reg.pair.A = 123;                                            // (setup register for test)
    z80.reg.pair.B = 0;                                              // (setup register for test)
    z80.reg.pair.C = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB9, 0, 0, 23);                   // CPDR (--BC != 0)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB9, 0, 0, 18);                   // CPDR (--BC == 0)

    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 5);                       // ADD A, B
    executeTest(&z80, &mmu, 0xC6, 9, 0, 0, 8);                       // ADD A, n
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 8);                       // ADD A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 21);                   // ADD A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 21);                   // ADD A, (IX+d)
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 5);                       // ADC A, B
    executeTest(&z80, &mmu, 0xCE, 9, 0, 0, 8);                       // ADC A, n
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 8);                       // ADC A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 21);                   // ADC A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 21);                   // ADC A, (IX+d)
    executeTest(&z80, &mmu, 0x90, 0, 0, 0, 5);                       // SUB A, B
    executeTest(&z80, &mmu, 0xD6, 9, 0, 0, 8);                       // SUB A, n
    executeTest(&z80, &mmu, 0x96, 0, 0, 0, 8);                       // SUB A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x96, 5, 0, 21);                   // SUB A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x96, 5, 0, 21);                   // SUB A, (IX+d)
    executeTest(&z80, &mmu, 0x98, 0, 0, 0, 5);                       // SBC A, B
    executeTest(&z80, &mmu, 0xDE, 9, 0, 0, 8);                       // SBC A, n
    executeTest(&z80, &mmu, 0x9E, 0, 0, 0, 8);                       // SBC A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0x9E, 5, 0, 21);                   // SBC A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x9E, 5, 0, 21);                   // SBC A, (IX+d)
    executeTest(&z80, &mmu, 0xA0, 0, 0, 0, 5);                       // AND A, B
    executeTest(&z80, &mmu, 0xE6, 9, 0, 0, 8);                       // AND A, n
    executeTest(&z80, &mmu, 0xA6, 0, 0, 0, 8);                       // AND A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xA6, 5, 0, 21);                   // AND A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xA6, 5, 0, 21);                   // AND A, (IX+d)
    executeTest(&z80, &mmu, 0xB0, 0, 0, 0, 5);                       // OR A, B
    executeTest(&z80, &mmu, 0xF6, 9, 0, 0, 8);                       // OR A, n
    executeTest(&z80, &mmu, 0xB6, 0, 0, 0, 8);                       // OR A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xB6, 5, 0, 21);                   // OR A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xB6, 5, 0, 21);                   // OR A, (IX+d)
    executeTest(&z80, &mmu, 0xA8, 0, 0, 0, 5);                       // XOR A, B
    executeTest(&z80, &mmu, 0xEE, 9, 0, 0, 8);                       // XOR A, n
    executeTest(&z80, &mmu, 0xAE, 0, 0, 0, 8);                       // XOR A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xAE, 5, 0, 21);                   // XOR A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xAE, 5, 0, 21);                   // XOR A, (IX+d)
    executeTest(&z80, &mmu, 0xB8, 0, 0, 0, 5);                       // CP A, B
    executeTest(&z80, &mmu, 0xFE, 9, 0, 0, 8);                       // CP A, n
    executeTest(&z80, &mmu, 0xBE, 0, 0, 0, 8);                       // CP A, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xBE, 5, 0, 21);                   // CP A, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xBE, 5, 0, 21);                   // CP A, (IX+d)
    executeTest(&z80, &mmu, 0x04, 0, 0, 0, 5);                       // INC B
    executeTest(&z80, &mmu, 0x34, 0, 0, 0, 12);                      // INC (HL)
    executeTest(&z80, &mmu, 0xDD, 0x34, 3, 0, 25);                   // INC (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x34, 6, 0, 25);                   // INC (IY+d)
    executeTest(&z80, &mmu, 0x05, 0, 0, 0, 5);                       // DEC B
    executeTest(&z80, &mmu, 0x35, 0, 0, 0, 12);                      // DEC (HL)
    executeTest(&z80, &mmu, 0xDD, 0x35, 3, 0, 25);                   // DEC (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0x35, 6, 0, 25);                   // DEC (IY+d)
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 5);                       // DAA
    executeTest(&z80, &mmu, 0x2F, 0, 0, 0, 5);                       // CPL
    executeTest(&z80, &mmu, 0xED, 0x44, 0, 0, 10);                   // NEG
    executeTest(&z80, &mmu, 0x3F, 0, 0, 0, 5);                       // CCF
    executeTest(&z80, &mmu, 0x37, 0, 0, 0, 5);                       // SCF
    executeTest(&z80, &mmu, 0x00, 0, 0, 0, 5);                       // NOP
    executeTest(&z80, &mmu, 0b01110110, 0, 0, 0, 5);                 // HALT
    z80.reg.IFF = 0;                                                 // (setup register for test)
    executeTest(&z80, &mmu, 0x09, 0, 0, 0, 12);                      // ADD HL, BC
    executeTest(&z80, &mmu, 0x19, 0, 0, 0, 12);                      // ADD HL, DE
    executeTest(&z80, &mmu, 0x29, 0, 0, 0, 12);                      // ADD HL, HL
    executeTest(&z80, &mmu, 0x39, 0, 0, 0, 12);                      // ADD HL, SP
    executeTest(&z80, &mmu, 0xED, 0x4A, 0, 0, 17);                   // ADC HL, BC
    executeTest(&z80, &mmu, 0xED, 0x5A, 0, 0, 17);                   // ADC HL, DE
    executeTest(&z80, &mmu, 0xED, 0x6A, 0, 0, 17);                   // ADC HL, HL
    executeTest(&z80, &mmu, 0xED, 0x7A, 0, 0, 17);                   // ADC HL, SP
    executeTest(&z80, &mmu, 0xED, 0x42, 0, 0, 17);                   // SBC HL, BC
    executeTest(&z80, &mmu, 0xED, 0x52, 0, 0, 17);                   // SBC HL, DE
    executeTest(&z80, &mmu, 0xED, 0x62, 0, 0, 17);                   // SBC HL, HL
    executeTest(&z80, &mmu, 0xED, 0x72, 0, 0, 17);                   // SBC HL, SP
    executeTest(&z80, &mmu, 0xDD, 0x09, 0, 0, 17);                   // ADD IX, BC
    executeTest(&z80, &mmu, 0xDD, 0x19, 0, 0, 17);                   // ADD IX, DE
    executeTest(&z80, &mmu, 0xDD, 0x29, 0, 0, 17);                   // ADD IX, HL
    executeTest(&z80, &mmu, 0xDD, 0x39, 0, 0, 17);                   // ADD IX, SP
    executeTest(&z80, &mmu, 0xFD, 0x09, 0, 0, 17);                   // ADD IY, BC
    executeTest(&z80, &mmu, 0xFD, 0x19, 0, 0, 17);                   // ADD IY, DE
    executeTest(&z80, &mmu, 0xFD, 0x29, 0, 0, 17);                   // ADD IY, HL
    executeTest(&z80, &mmu, 0xFD, 0x39, 0, 0, 17);                   // ADD IY, SP
    executeTest(&z80, &mmu, 0x03, 0, 0, 0, 7);                       // INC BC
    executeTest(&z80, &mmu, 0x13, 0, 0, 0, 7);                       // INC DE
    executeTest(&z80, &mmu, 0x23, 0, 0, 0, 7);                       // INC HL
    executeTest(&z80, &mmu, 0x33, 0, 0, 0, 7);                       // INC SP
    executeTest(&z80, &mmu, 0xDD, 0x23, 0, 0, 12);                   // INC HL
    executeTest(&z80, &mmu, 0xFD, 0x23, 0, 0, 12);                   // INC HL
    executeTest(&z80, &mmu, 0x0B, 0, 0, 0, 7);                       // DEC BC
    executeTest(&z80, &mmu, 0x1B, 0, 0, 0, 7);                       // DEC DE
    executeTest(&z80, &mmu, 0x2B, 0, 0, 0, 7);                       // DEC HL
    executeTest(&z80, &mmu, 0x3B, 0, 0, 0, 7);                       // DEC SP
    executeTest(&z80, &mmu, 0xDD, 0x2B, 0, 0, 12);                   // DEC HL
    executeTest(&z80, &mmu, 0xFD, 0x2B, 0, 0, 12);                   // DEC HL
    executeTest(&z80, &mmu, 0x07, 0, 0, 0, 5);                       // RLCA
    executeTest(&z80, &mmu, 0x17, 0, 0, 0, 5);                       // RLA
    executeTest(&z80, &mmu, 0x0F, 0, 0, 0, 5);                       // RRCA
    executeTest(&z80, &mmu, 0x1F, 0, 0, 0, 5);                       // RRA
    executeTest(&z80, &mmu, 0xCB, 0x00, 0, 0, 10);                   // RLC B
    executeTest(&z80, &mmu, 0xCB, 0x06, 0, 0, 17);                   // RLC (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x06, 25);              // RLC (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x06, 25);              // RLC (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x10, 0, 0, 10);                   // RL B
    executeTest(&z80, &mmu, 0xCB, 0x16, 0, 0, 17);                   // RL (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x16, 25);              // RL (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x16, 25);              // RL (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x08, 0, 0, 10);                   // RRC B
    executeTest(&z80, &mmu, 0xCB, 0x0E, 0, 0, 17);                   // RRC (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x0E, 25);              // RRC (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x0E, 25);              // RRC (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x18, 0, 0, 10);                   // RR B
    executeTest(&z80, &mmu, 0xCB, 0x1E, 0, 0, 17);                   // RR (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x1E, 25);              // RR (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x1E, 25);              // RR (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x20, 0, 0, 10);                   // SLA B
    executeTest(&z80, &mmu, 0xCB, 0x26, 0, 0, 17);                   // SLA (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x26, 25);              // SLA (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x26, 25);              // SLA (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x28, 0, 0, 10);                   // SRA B
    executeTest(&z80, &mmu, 0xCB, 0x2E, 0, 0, 17);                   // SRA (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x2E, 25);              // SRA (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x2E, 25);              // SRA (IY+123)
    executeTest(&z80, &mmu, 0xCB, 0x38, 0, 0, 10);                    // SRL B
    executeTest(&z80, &mmu, 0xCB, 0x3E, 0, 0, 17);                   // SRL (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x3E, 25);              // SRL (IX+123)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 123, 0x3E, 25);              // SRL (IY+123)
    executeTest(&z80, &mmu, 0xED, 0x6F, 0, 0, 20);                   // RLD
    executeTest(&z80, &mmu, 0xED, 0x67, 0, 0, 20);                   // RRD
    executeTest(&z80, &mmu, 0xCB, 0b01000000, 0, 0, 10);             // BIT b, r
    executeTest(&z80, &mmu, 0xCB, 0b01000110, 0, 0, 14);             // BIT b, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 129, 0b01000110, 22);        // BIT b, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 129, 0b01000110, 22);        // BIT b, (IY+d)
    executeTest(&z80, &mmu, 0xCB, 0b11000000, 0, 0, 10);             // SET b, r
    executeTest(&z80, &mmu, 0xCB, 0b11000110, 0, 0, 17);             // SET b, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 129, 0b11000110, 25);        // SET b, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 129, 0b11000110, 25);        // SET b, (IY+d)
    executeTest(&z80, &mmu, 0xCB, 0b10000000, 0, 0, 10);             // RES b, r
    executeTest(&z80, &mmu, 0xCB, 0b10000110, 0, 0, 17);             // RES b, (HL)
    executeTest(&z80, &mmu, 0xDD, 0xCB, 129, 0b10000110, 25);        // RES b, (IX+d)
    executeTest(&z80, &mmu, 0xFD, 0xCB, 129, 0b10000110, 25);        // RES b, (IY+d)

    executeTest(&z80, &mmu, 0xC3, 0x34, 0x12, 0, 11);                // JP nn
    executeTest(&z80, &mmu, 0xC2, 0x34, 0x12, 0, 11);                // JP NZ nn
    executeTest(&z80, &mmu, 0xCA, 0x34, 0x12, 0, 11);                // JP Z nn
    executeTest(&z80, &mmu, 0xD2, 0x34, 0x12, 0, 11);                // JP NC nn
    executeTest(&z80, &mmu, 0xDA, 0x34, 0x12, 0, 11);                // JP C nn
    executeTest(&z80, &mmu, 0xE2, 0x34, 0x12, 0, 11);                // JP PO nn
    executeTest(&z80, &mmu, 0xEA, 0x34, 0x12, 0, 11);                // JP PE nn
    executeTest(&z80, &mmu, 0xF2, 0x34, 0x12, 0, 11);                // JP P nn
    executeTest(&z80, &mmu, 0xFA, 0x34, 0x12, 0, 11);                // JP M nn
    executeTest(&z80, &mmu, 0x18, 79, 0, 0, 13);                     // JR e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x38, 79, 0, 0, 8);                      // JR C, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x38, 79, 0, 0, 13);                     // JR C, e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x30, 79, 0, 0, 13);                     // JR NC, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x30, 79, 0, 0, 8);                      // JR NC, e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x28, 79, 0, 0, 8);                      // JR Z, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x28, 79, 0, 0, 13);                     // JR Z, e
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x20, 79, 0, 0, 13);                     // JR NC, e
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x20, 79, 0, 0, 8);                      // JR NC, e
    executeTest(&z80, &mmu, 0xE9, 0, 0, 0, 5);                       // JP (HL)
    executeTest(&z80, &mmu, 0xDD, 0xE9, 0, 0, 10);                   // JP (IX)
    executeTest(&z80, &mmu, 0xFD, 0xE9, 0, 0, 10);                   // JP (IY)
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0x10, 78, 0, 0, 14);                     // DJNZ
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0x10, 78, 0, 0, 9);                      // DJNZ
    executeTest(&z80, &mmu, 0xCD, 0x34, 0x12, 0, 18);                // CALL nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xC4, 0x34, 0x12, 0, 18);                // CALL NZ, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xC4, 0x34, 0x12, 0, 11);                // CALL NZ, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xCC, 0x34, 0x12, 0, 11);                // CALL Z, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xCC, 0x34, 0x12, 0, 18);                // CALL Z, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xD4, 0x34, 0x12, 0, 18);                // CALL NC, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xD4, 0x34, 0x12, 0, 11);                // CALL CZ, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xDC, 0x34, 0x12, 0, 11);                // CALL C, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xDC, 0x34, 0x12, 0, 18);                // CALL C, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xE4, 0x34, 0x12, 0, 18);                // CALL PO, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xE4, 0x34, 0x12, 0, 11);                // CALL PO, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xEC, 0x34, 0x12, 0, 11);                // CALL PE, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xEC, 0x34, 0x12, 0, 18);                // CALL PE, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xF4, 0x34, 0x12, 0, 18);                // CALL P, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xF4, 0x34, 0x12, 0, 11);                // CALL P, nn
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xFC, 0x34, 0x12, 0, 11);                // CALL M, nn
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xFC, 0x34, 0x12, 0, 18);                // CALL M, nn
    executeTest(&z80, &mmu, 0xC9, 0, 0, 0, 11);                      // RET
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 12);                      // RET NZ
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 6);                       // RET NZ
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xC8, 0, 0, 0, 6);                       // RET Z
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xC8, 0, 0, 0, 12);                      // RET Z
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xD0, 0, 0, 0, 12);                      // RET NC
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xD0, 0, 0, 0, 6);                       // RET NC
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xD8, 0, 0, 0, 6);                       // RET C
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xD8, 0, 0, 0, 12);                      // RET C
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xE0, 0, 0, 0, 12);                      // RET PO
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xE0, 0, 0, 0, 6);                       // RET PO
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xE8, 0, 0, 0, 6);                       // RET PE
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xE8, 0, 0, 0, 12);                      // RET PE
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xF0, 0, 0, 0, 12);                      // RET P
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xF0, 0, 0, 0, 6);                       // RET P
    z80.reg.pair.F = 0;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xF8, 0, 0, 0, 6);                       // RET M
    z80.reg.pair.F = 0xFF;                                           // (setup register for test)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xF8, 0, 0, 0, 12);                      // RET M
    executeTest(&z80, &mmu, 0xED, 0x4D, 0, 0, 16);                   // RETI
    executeTest(&z80, &mmu, 0xED, 0x45, 0, 0, 16);                   // RETN
    executeTest(&z80, &mmu, 0xC7, 0, 0, 0, 12);                      // RST 0

    executeTest(&z80, &mmu, 0xDB, 1, 0, 0, 12);                      // IN (n)
    executeTest(&z80, &mmu, 0xED, 0x40, 0, 0, 14);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x48, 0, 0, 14);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x50, 0, 0, 14);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x58, 0, 0, 14);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x60, 0, 0, 14);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x68, 0, 0, 14);                   // IN r, (C)
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x78, 0, 0, 14);                   // IN r, (C)
    executeTest(&z80, &mmu, 0xED, 0xA2, 0, 0, 18);                   // INI
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB2, 0, 0, 23);                   // INIR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB2, 0, 0, 18);                   // INIR
    executeTest(&z80, &mmu, 0xED, 0xAA, 0, 0, 18);                   // IND
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xBA, 0, 0, 23);                   // INDR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xBA, 0, 0, 18);                   // INDR
    executeTest(&z80, &mmu, 0xD3, 0, 0, 0, 12);                      // OUT (n), A
    executeTest(&z80, &mmu, 0xED, 0x41, 0, 0, 14);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x49, 0, 0, 14);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x51, 0, 0, 14);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x59, 0, 0, 14);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x61, 0, 0, 14);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x69, 0, 0, 14);                   // OUT (C), r
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0x79, 0, 0, 14);                   // OUT (C), r
    executeTest(&z80, &mmu, 0xED, 0xA3, 0, 0, 18);                   // OUTI
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xB3, 0, 0, 23);                   // OTIR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xB3, 0, 0, 18);                   // OTIR
    executeTest(&z80, &mmu, 0xED, 0xAB, 0, 0, 18);                   // OUTD
    z80.reg.pair.B = 2;                                              // (setup register for test)
    executeTest(&z80, &mmu, 0xED, 0xBB, 0, 0, 23);                   // OTDR
    testNumber--;                                                    // (decrement test number)
    executeTest(&z80, &mmu, 0xED, 0xBB, 0, 0, 18);                   // OTDR

    executeTest(&z80, &mmu, 0b11110011, 0, 0, 0, 5);                 // DI
    executeTest(&z80, &mmu, 0b11111011, 0, 0, 0, 5);                 // EI
    executeTest(&z80, &mmu, 0b11101101, 0b01000110, 0, 0, 10);       // IM 0
    executeTest(&z80, &mmu, 0b11101101, 0b01010110, 0, 0, 10);       // IM 1
    executeTest(&z80, &mmu, 0b11101101, 0b01011110, 0, 0, 10);       // IM 2

    // undocumented instructions
    executeTest(&z80, &mmu, 0xDD, 0x24, 0, 0, 10);      // INC IXH
    executeTest(&z80, &mmu, 0xFD, 0x24, 0, 0, 10);      // INC IYH
    executeTest(&z80, &mmu, 0xDD, 0x2C, 0, 0, 10);      // INC IXL
    executeTest(&z80, &mmu, 0xFD, 0x2C, 0, 0, 10);      // INC IYL
    executeTest(&z80, &mmu, 0xDD, 0x25, 0, 0, 10);      // DEC IXH
    executeTest(&z80, &mmu, 0xFD, 0x25, 0, 0, 10);      // DEC IYH
    executeTest(&z80, &mmu, 0xDD, 0x2D, 0, 0, 10);      // DEC IXL
    executeTest(&z80, &mmu, 0xFD, 0x2D, 0, 0, 10);      // DEC IYL
    executeTest(&z80, &mmu, 0xDD, 0x26, 123, 0, 13);    // LD IXH, n
    executeTest(&z80, &mmu, 0xFD, 0x26, 123, 0, 13);    // LD IXH, n
    executeTest(&z80, &mmu, 0xDD, 0x2E, 123, 0, 13);    // LD IXL, n
    executeTest(&z80, &mmu, 0xFD, 0x2E, 123, 0, 13);    // LD IXL, n
    executeTest(&z80, &mmu, 0xDD, 0x67, 0, 0, 10);       // LD IXH, A
    executeTest(&z80, &mmu, 0xDD, 0x60, 0, 0, 10);       // LD IXH, B
    executeTest(&z80, &mmu, 0xDD, 0x61, 0, 0, 10);       // LD IXH, C
    executeTest(&z80, &mmu, 0xDD, 0x62, 0, 0, 10);       // LD IXH, D
    executeTest(&z80, &mmu, 0xDD, 0x63, 0, 0, 10);       // LD IXH, E
    executeTest(&z80, &mmu, 0xDD, 0x64, 0, 0, 10);       // LD IXH, IXH
    executeTest(&z80, &mmu, 0xDD, 0x65, 0, 0, 10);       // LD IXH, IXL
    executeTest(&z80, &mmu, 0xDD, 0x6F, 0, 0, 10);       // LD IXL, A
    executeTest(&z80, &mmu, 0xDD, 0x68, 0, 0, 10);       // LD IXL, B
    executeTest(&z80, &mmu, 0xDD, 0x69, 0, 0, 10);       // LD IXL, C
    executeTest(&z80, &mmu, 0xDD, 0x6A, 0, 0, 10);       // LD IXL, D
    executeTest(&z80, &mmu, 0xDD, 0x6B, 0, 0, 10);       // LD IXL, E
    executeTest(&z80, &mmu, 0xDD, 0x6C, 0, 0, 10);       // LD IXL, IXH
    executeTest(&z80, &mmu, 0xDD, 0x6D, 0, 0, 10);       // LD IXL, IXL
    executeTest(&z80, &mmu, 0xFD, 0x67, 0, 0, 10);       // LD IYH, A
    executeTest(&z80, &mmu, 0xFD, 0x60, 0, 0, 10);       // LD IYH, B
    executeTest(&z80, &mmu, 0xFD, 0x61, 0, 0, 10);       // LD IYH, C
    executeTest(&z80, &mmu, 0xFD, 0x62, 0, 0, 10);       // LD IYH, D
    executeTest(&z80, &mmu, 0xFD, 0x63, 0, 0, 10);       // LD IYH, E
    executeTest(&z80, &mmu, 0xFD, 0x64, 0, 0, 10);       // LD IYH, IYH
    executeTest(&z80, &mmu, 0xFD, 0x65, 0, 0, 10);       // LD IYH, IYL
    executeTest(&z80, &mmu, 0xFD, 0x6F, 0, 0, 10);       // LD IYL, A
    executeTest(&z80, &mmu, 0xFD, 0x68, 0, 0, 10);       // LD IYL, B
    executeTest(&z80, &mmu, 0xFD, 0x69, 0, 0, 10);       // LD IYL, C
    executeTest(&z80, &mmu, 0xFD, 0x6A, 0, 0, 10);       // LD IYL, D
    executeTest(&z80, &mmu, 0xFD, 0x6B, 0, 0, 10);       // LD IYL, E
    executeTest(&z80, &mmu, 0xFD, 0x6C, 0, 0, 10);       // LD IYL, IYH
    executeTest(&z80, &mmu, 0xFD, 0x6D, 0, 0, 10);       // LD IYL, IYL
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 10);       // ADD A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 10);       // ADD A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 10);       // ADD A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 10);       // ADD A, IYL
    executeTest(&z80, &mmu, 0xDD, 0x7C, 0, 0, 10);       // LD A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x7D, 0, 0, 10);       // LD A, IXL
    executeTest(&z80, &mmu, 0xDD, 0x44, 0, 0, 10);       // LD B, IXH
    executeTest(&z80, &mmu, 0xDD, 0x45, 0, 0, 10);       // LD B, IXL
    executeTest(&z80, &mmu, 0xDD, 0x4C, 0, 0, 10);       // LD C, IXH
    executeTest(&z80, &mmu, 0xDD, 0x4D, 0, 0, 10);       // LD C, IXL
    executeTest(&z80, &mmu, 0xDD, 0x54, 0, 0, 10);       // LD D, IXH
    executeTest(&z80, &mmu, 0xDD, 0x55, 0, 0, 10);       // LD D, IXL
    executeTest(&z80, &mmu, 0xDD, 0x5C, 0, 0, 10);       // LD E, IXH
    executeTest(&z80, &mmu, 0xDD, 0x5D, 0, 0, 10);       // LD E, IXL
    executeTest(&z80, &mmu, 0xFD, 0x7C, 0, 0, 10);       // LD A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x7D, 0, 0, 10);       // LD A, IYL
    executeTest(&z80, &mmu, 0xFD, 0x44, 0, 0, 10);       // LD B, IYH
    executeTest(&z80, &mmu, 0xFD, 0x45, 0, 0, 10);       // LD B, IYL
    executeTest(&z80, &mmu, 0xFD, 0x4C, 0, 0, 10);       // LD C, IYH
    executeTest(&z80, &mmu, 0xFD, 0x4D, 0, 0, 10);       // LD C, IYL
    executeTest(&z80, &mmu, 0xFD, 0x54, 0, 0, 10);       // LD D, IYH
    executeTest(&z80, &mmu, 0xFD, 0x55, 0, 0, 10);       // LD D, IYL
    executeTest(&z80, &mmu, 0xFD, 0x5C, 0, 0, 10);       // LD E, IYH
    executeTest(&z80, &mmu, 0xFD, 0x5D, 0, 0, 10);       // LD E, IYL
    executeTest(&z80, &mmu, 0xDD, 0x8C, 0, 0, 10);       // ADC A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x8D, 0, 0, 10);       // ADC A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x8C, 0, 0, 10);       // ADC A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x8D, 0, 0, 10);       // ADC A, IYL
    executeTest(&z80, &mmu, 0xDD, 0x94, 0, 0, 10);       // SUB A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x95, 0, 0, 10);       // SUB A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x94, 0, 0, 10);       // SUB A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x95, 0, 0, 10);       // SUB A, IYL
    executeTest(&z80, &mmu, 0xDD, 0x9C, 0, 0, 10);       // SBC A, IXH
    executeTest(&z80, &mmu, 0xDD, 0x9D, 0, 0, 10);       // SBC A, IXL
    executeTest(&z80, &mmu, 0xFD, 0x9C, 0, 0, 10);       // SBC A, IYH
    executeTest(&z80, &mmu, 0xFD, 0x9D, 0, 0, 10);       // SBC A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xA4, 0, 0, 10);       // AND A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xA5, 0, 0, 10);       // AND A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xA4, 0, 0, 10);       // AND A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xA5, 0, 0, 10);       // AND A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xB4, 0, 0, 10);       // OR A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xB5, 0, 0, 10);       // OR A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xB4, 0, 0, 10);       // OR A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xB5, 0, 0, 10);       // OR A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xAC, 0, 0, 10);       // XOR A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xAD, 0, 0, 10);       // XOR A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xAC, 0, 0, 10);       // XOR A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xAD, 0, 0, 10);       // XOR A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xBC, 0, 0, 10);       // CP A, IXH
    executeTest(&z80, &mmu, 0xDD, 0xBD, 0, 0, 10);       // CP A, IXL
    executeTest(&z80, &mmu, 0xFD, 0xBC, 0, 0, 10);       // CP A, IYH
    executeTest(&z80, &mmu, 0xFD, 0xBD, 0, 0, 10);       // CP A, IYL
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x07, 25); // RLC (IX+d) with LD A
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x00, 25); // RLC (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x01, 25); // RLC (IX+d) with LD C
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x02, 25); // RLC (IX+d) with LD D
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x03, 25); // RLC (IX+d) with LD E
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x04, 25); // RLC (IX+d) with LD H
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x05, 25); // RLC (IX+d) with LD L
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x08, 25); // RRC (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x10, 25); // RL (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x18, 25); // RR (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x20, 25); // SLA (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x28, 25); // SRA (IX+d) with LD B
    executeTest(&z80, &mmu, 0xCB, 0x30, 0, 0, 10);       // SLL B
    executeTest(&z80, &mmu, 0xCB, 0x31, 0, 0, 10);       // SLL C
    executeTest(&z80, &mmu, 0xCB, 0x32, 0, 0, 10);       // SLL D
    executeTest(&z80, &mmu, 0xCB, 0x33, 0, 0, 10);       // SLL E
    executeTest(&z80, &mmu, 0xCB, 0x34, 0, 0, 10);       // SLL H
    executeTest(&z80, &mmu, 0xCB, 0x35, 0, 0, 10);       // SLL L
    executeTest(&z80, &mmu, 0xCB, 0x36, 0, 0, 17);      // SLL (HL)
    executeTest(&z80, &mmu, 0xCB, 0x37, 0, 0, 10);       // SLL A
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x30, 25); // SLL (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x38, 25); // SRL (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x80, 25); // RES 0, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x88, 25); // RES 1, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x90, 25); // RES 2, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0x98, 25); // RES 3, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xA0, 25); // RES 4, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xA8, 25); // RES 5, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xB0, 25); // RES 6, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xB8, 25); // RES 7, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xC0, 25); // SET 0, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xC8, 25); // SET 1, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xD0, 25); // SET 2, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xD8, 25); // SET 3, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xE0, 25); // SET 4, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xE8, 25); // SET 5, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xF0, 25); // SET 6, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xDD, 0xCB, 123, 0xF8, 25); // SET 7, (IX+d) with LD B
    executeTest(&z80, &mmu, 0xED, 0x70, 0, 0, 14);      // IN F,(C)
    executeTest(&z80, &mmu, 0xED, 0x71, 0, 0, 14);      // OUT F,(C)

    return 0;
}
