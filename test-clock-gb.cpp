#include "z80.hpp"
#include <ctype.h>

class MMU
{
  public:
    unsigned char RAM[0x10000];
    Z80* cpu;
    MMU() { memset(&RAM, 0, sizeof(RAM)); }
};
unsigned char readByte(void* arg, unsigned short addr) { return ((MMU*)arg)->RAM[addr]; }
void writeByte(void* arg, unsigned short addr, unsigned char value) { ((MMU*)arg)->RAM[addr] = value; }

static int testNumber;
static int expectClocks;
static FILE* file;
void executeTest(Z80* cpu, MMU* mmu, unsigned char op1, unsigned char op2, unsigned char op3, unsigned char op4, unsigned char flagBeofre, int clocks)
{
    expectClocks = clocks;
    mmu->RAM[0] = op1;
    mmu->RAM[1] = op2;
    mmu->RAM[2] = op3;
    mmu->RAM[3] = op4;
    cpu->reg.PC = 0;
    cpu->reg.pair.F = flagBeofre;
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
    file = fopen("test-clock-gb.txt", "w");
    fprintf(file, "===== CLOCK CYCLE TEST =====\n");
    fprintf(stdout, "===== CLOCK CYCLE TEST =====\n");
    MMU mmu;
    Z80 z80(readByte, writeByte, NULL, NULL, &mmu);
    mmu.cpu = &z80;
    z80.setDebugMessage([](void* arg, const char* msg) {
        testNumber++;
        fprintf(file, "TEST#%03d: %2dHz %s\n", testNumber, expectClocks, msg);
        fprintf(stdout, "TEST#%03d: %2dHz %s\n", testNumber, expectClocks, msg);
    });

    // checked clocks from https://pastraiser.com/cpu/gameboy/gameboy_opcodes.html
    executeTest(&z80, &mmu, 0x00, 0, 0, 0, 0, 4);        // NOP
    executeTest(&z80, &mmu, 0x01, 0x34, 0x12, 0, 0, 12); // LD BC,d16
    executeTest(&z80, &mmu, 0x02, 0, 0, 0, 0, 8);        // LD (BC),A
    executeTest(&z80, &mmu, 0x03, 0, 0, 0, 0, 8);        // INC BC
    executeTest(&z80, &mmu, 0x04, 0, 0, 0, 0, 4);        // INC B
    executeTest(&z80, &mmu, 0x05, 0, 0, 0, 0, 4);        // DEC B
    executeTest(&z80, &mmu, 0x06, 123, 0, 0, 0, 8);      // LD B,d8
    executeTest(&z80, &mmu, 0x07, 0, 0, 0, 0, 4);        // RLCA
    executeTest(&z80, &mmu, 0x08, 0xCD, 0xAB, 0, 0, 20); // LD (a16),SP
    executeTest(&z80, &mmu, 0x09, 0, 0, 0, 0, 8);        // ADD HL,BC
    executeTest(&z80, &mmu, 0x0A, 0, 0, 0, 0, 8);        // LD A,(BC)
    executeTest(&z80, &mmu, 0x0B, 0, 0, 0, 0, 8);        // DEC BC
    executeTest(&z80, &mmu, 0x0C, 0, 0, 0, 0, 4);        // INC C
    executeTest(&z80, &mmu, 0x0D, 0, 0, 0, 0, 4);        // DEC C
    executeTest(&z80, &mmu, 0x0E, 98, 0, 0, 0, 8);       // LD C,d8
    executeTest(&z80, &mmu, 0x0F, 0, 0, 0, 0, 4);        // RRCA
    executeTest(&z80, &mmu, 0x10, 0, 0, 0, 0, 4);        // STOP
    executeTest(&z80, &mmu, 0x11, 0x34, 0x12, 0, 0, 12); // LD DE,d16
    executeTest(&z80, &mmu, 0x12, 0xCD, 0, 0, 0, 8);     // LD (DE),A
    executeTest(&z80, &mmu, 0x13, 0, 0, 0, 0, 8);        // INC DE
    executeTest(&z80, &mmu, 0x14, 0, 0, 0, 0, 4);        // INC D
    executeTest(&z80, &mmu, 0x15, 0, 0, 0, 0, 4);        // DEC D
    executeTest(&z80, &mmu, 0x16, 3, 0, 0, 0, 8);        // LD D,d8
    executeTest(&z80, &mmu, 0x17, 0, 0, 0, 0, 4);        // RLA
    executeTest(&z80, &mmu, 0x18, 0x80, 0, 0, 0, 12);    // JR r8
    executeTest(&z80, &mmu, 0x19, 3, 0, 0, 0, 8);        // ADD HL,DE
    executeTest(&z80, &mmu, 0x1A, 3, 0, 0, 0, 8);        // LD A,(DE)
    executeTest(&z80, &mmu, 0x1B, 3, 0, 0, 0, 8);        // DEC DE
    executeTest(&z80, &mmu, 0x1C, 0, 0, 0, 0, 4);        // INC D
    executeTest(&z80, &mmu, 0x1D, 0, 0, 0, 0, 4);        // DEC D
    executeTest(&z80, &mmu, 0x1E, 12, 0, 0, 0, 8);       // LD E,d8
    executeTest(&z80, &mmu, 0x1F, 0, 0, 0, 0, 4);        // RRA
    executeTest(&z80, &mmu, 0x20, 0, 0, 0, 0x40, 8);     // JR NZ,r8
    executeTest(&z80, &mmu, 0x20, 0, 0, 0, 0x00, 12);    // JR NZ,r8
    executeTest(&z80, &mmu, 0x21, 0x34, 0x12, 0, 0, 12); // LD HL,d16
    executeTest(&z80, &mmu, 0x22, 0, 0, 0, 0, 8);        // LD (HL+),A
    executeTest(&z80, &mmu, 0x23, 0, 0, 0, 0, 8);        // INC HL
    executeTest(&z80, &mmu, 0x24, 0, 0, 0, 0, 4);        // INC H
    executeTest(&z80, &mmu, 0x25, 0, 0, 0, 0, 4);        // DEC H
    executeTest(&z80, &mmu, 0x26, 0x33, 0, 0, 0, 8);     // LD H,d8
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0, 4);        // DAA
    executeTest(&z80, &mmu, 0x28, 0, 0, 0, 0x00, 8);     // JR Z,r8
    executeTest(&z80, &mmu, 0x28, 0, 0, 0, 0x40, 12);    // JR Z,r8
    executeTest(&z80, &mmu, 0x29, 0, 0, 0, 0, 8);        // ADD HL,HL
    executeTest(&z80, &mmu, 0x2A, 0, 0, 0, 0, 8);        // LD A,(HL+)
    executeTest(&z80, &mmu, 0x2B, 0, 0, 0, 0, 8);        // DEC HL
    executeTest(&z80, &mmu, 0x2C, 0, 0, 0, 0, 4);        // INC L
    executeTest(&z80, &mmu, 0x2D, 0, 0, 0, 0, 4);        // DEC L
    executeTest(&z80, &mmu, 0x2E, 12, 0, 0, 0, 8);       // LD L,d8
    executeTest(&z80, &mmu, 0x2F, 0, 0, 0, 0, 4);        // CPL
    executeTest(&z80, &mmu, 0x30, 0, 0, 0, 0x01, 8);     // JR NC,r8
    executeTest(&z80, &mmu, 0x30, 0, 0, 0, 0x00, 12);    // JR NC,r8
    executeTest(&z80, &mmu, 0x31, 0x22, 0x11, 0, 0, 12); // LD SP,d16
    executeTest(&z80, &mmu, 0x32, 0, 0, 0, 0, 8);        // LD (HL-),A
    executeTest(&z80, &mmu, 0x33, 0, 0, 0, 0, 8);        // INC SP
    executeTest(&z80, &mmu, 0x34, 0, 0, 0, 0, 12);       // INC (HL)
    executeTest(&z80, &mmu, 0x35, 0, 0, 0, 0, 12);       // DEC (HL)
    executeTest(&z80, &mmu, 0x36, 99, 0, 0, 0, 12);      // LD (HL),d8
    executeTest(&z80, &mmu, 0x37, 0, 0, 0, 0, 4);        // SCF
    executeTest(&z80, &mmu, 0x38, 0, 0, 0, 0x00, 8);     // JR C,r8
    executeTest(&z80, &mmu, 0x38, 0, 0, 0, 0x01, 12);    // JR C,r8
    executeTest(&z80, &mmu, 0x39, 0, 0, 0, 0, 8);        // ADD HL,SP
    executeTest(&z80, &mmu, 0x3A, 0, 0, 0, 0, 8);        // LD A,(HL-)
    executeTest(&z80, &mmu, 0x3B, 0, 0, 0, 0, 8);        // DEC SP
    executeTest(&z80, &mmu, 0x3C, 0, 0, 0, 0, 4);        // INC A
    executeTest(&z80, &mmu, 0x3D, 0, 0, 0, 0, 4);        // DEC A
    executeTest(&z80, &mmu, 0x3E, 55, 0, 0, 0, 8);       // LD A,d8
    executeTest(&z80, &mmu, 0x3F, 0, 0, 0, 0, 4);        // CCF
    executeTest(&z80, &mmu, 0x40, 0, 0, 0, 0, 4);        // LD B,B
    executeTest(&z80, &mmu, 0x41, 0, 0, 0, 0, 4);        // LD B,C
    executeTest(&z80, &mmu, 0x42, 0, 0, 0, 0, 4);        // LD B,D
    executeTest(&z80, &mmu, 0x43, 0, 0, 0, 0, 4);        // LD B,E
    executeTest(&z80, &mmu, 0x44, 0, 0, 0, 0, 4);        // LD B,H
    executeTest(&z80, &mmu, 0x45, 0, 0, 0, 0, 4);        // LD B,L
    executeTest(&z80, &mmu, 0x46, 0, 0, 0, 0, 8);        // LD B,(HL)
    executeTest(&z80, &mmu, 0x47, 0, 0, 0, 0, 4);        // LD B,A
    executeTest(&z80, &mmu, 0x48, 0, 0, 0, 0, 4);        // LD C,B
    executeTest(&z80, &mmu, 0x49, 0, 0, 0, 0, 4);        // LD C,C
    executeTest(&z80, &mmu, 0x4A, 0, 0, 0, 0, 4);        // LD C,D
    executeTest(&z80, &mmu, 0x4B, 0, 0, 0, 0, 4);        // LD C,E
    executeTest(&z80, &mmu, 0x4C, 0, 0, 0, 0, 4);        // LD C,H
    executeTest(&z80, &mmu, 0x4D, 0, 0, 0, 0, 4);        // LD C,L
    executeTest(&z80, &mmu, 0x4E, 0, 0, 0, 0, 8);        // LD C,(HL)
    executeTest(&z80, &mmu, 0x4F, 0, 0, 0, 0, 4);        // LD C,A
    executeTest(&z80, &mmu, 0x50, 0, 0, 0, 0, 4);        // LD D,B
    executeTest(&z80, &mmu, 0x51, 0, 0, 0, 0, 4);        // LD D,C
    executeTest(&z80, &mmu, 0x52, 0, 0, 0, 0, 4);        // LD D,D
    executeTest(&z80, &mmu, 0x53, 0, 0, 0, 0, 4);        // LD D,E
    executeTest(&z80, &mmu, 0x54, 0, 0, 0, 0, 4);        // LD D,H
    executeTest(&z80, &mmu, 0x55, 0, 0, 0, 0, 4);        // LD D,L
    executeTest(&z80, &mmu, 0x56, 0, 0, 0, 0, 8);        // LD D,(HL)
    executeTest(&z80, &mmu, 0x57, 0, 0, 0, 0, 4);        // LD D,A
    executeTest(&z80, &mmu, 0x58, 0, 0, 0, 0, 4);        // LD E,B
    executeTest(&z80, &mmu, 0x59, 0, 0, 0, 0, 4);        // LD E,C
    executeTest(&z80, &mmu, 0x5A, 0, 0, 0, 0, 4);        // LD E,D
    executeTest(&z80, &mmu, 0x5B, 0, 0, 0, 0, 4);        // LD E,E
    executeTest(&z80, &mmu, 0x5C, 0, 0, 0, 0, 4);        // LD E,H
    executeTest(&z80, &mmu, 0x5D, 0, 0, 0, 0, 4);        // LD E,L
    executeTest(&z80, &mmu, 0x5E, 0, 0, 0, 0, 8);        // LD E,(HL)
    executeTest(&z80, &mmu, 0x5F, 0, 0, 0, 0, 4);        // LD E,A
    executeTest(&z80, &mmu, 0x60, 0, 0, 0, 0, 4);        // LD H,B
    executeTest(&z80, &mmu, 0x61, 0, 0, 0, 0, 4);        // LD H,C
    executeTest(&z80, &mmu, 0x62, 0, 0, 0, 0, 4);        // LD H,D
    executeTest(&z80, &mmu, 0x63, 0, 0, 0, 0, 4);        // LD H,E
    executeTest(&z80, &mmu, 0x64, 0, 0, 0, 0, 4);        // LD H,H
    executeTest(&z80, &mmu, 0x65, 0, 0, 0, 0, 4);        // LD H,L
    executeTest(&z80, &mmu, 0x66, 0, 0, 0, 0, 8);        // LD H,(HL)
    executeTest(&z80, &mmu, 0x67, 0, 0, 0, 0, 4);        // LD H,A
    executeTest(&z80, &mmu, 0x68, 0, 0, 0, 0, 4);        // LD L,B
    executeTest(&z80, &mmu, 0x69, 0, 0, 0, 0, 4);        // LD L,C
    executeTest(&z80, &mmu, 0x6A, 0, 0, 0, 0, 4);        // LD L,D
    executeTest(&z80, &mmu, 0x6B, 0, 0, 0, 0, 4);        // LD L,E
    executeTest(&z80, &mmu, 0x6C, 0, 0, 0, 0, 4);        // LD L,H
    executeTest(&z80, &mmu, 0x6D, 0, 0, 0, 0, 4);        // LD L,L
    executeTest(&z80, &mmu, 0x6E, 0, 0, 0, 0, 8);        // LD L,(HL)
    executeTest(&z80, &mmu, 0x6F, 0, 0, 0, 0, 4);        // LD L,A
    executeTest(&z80, &mmu, 0x70, 0, 0, 0, 0, 8);        // LD (HL),B
    executeTest(&z80, &mmu, 0x71, 0, 0, 0, 0, 8);        // LD (HL),C
    executeTest(&z80, &mmu, 0x72, 0, 0, 0, 0, 8);        // LD (HL),D
    executeTest(&z80, &mmu, 0x73, 0, 0, 0, 0, 8);        // LD (HL),E
    executeTest(&z80, &mmu, 0x74, 0, 0, 0, 0, 8);        // LD (HL),H
    executeTest(&z80, &mmu, 0x75, 0, 0, 0, 0, 8);        // LD (HL),L
    executeTest(&z80, &mmu, 0x76, 0, 0, 0, 0, 4);        // HALT
    executeTest(&z80, &mmu, 0x77, 0, 0, 0, 0, 8);        // LD (HL),A
    executeTest(&z80, &mmu, 0x78, 0, 0, 0, 0, 4);        // LD A,B
    executeTest(&z80, &mmu, 0x79, 0, 0, 0, 0, 4);        // LD A,C
    executeTest(&z80, &mmu, 0x7A, 0, 0, 0, 0, 4);        // LD A,D
    executeTest(&z80, &mmu, 0x7B, 0, 0, 0, 0, 4);        // LD A,E
    executeTest(&z80, &mmu, 0x7C, 0, 0, 0, 0, 4);        // LD A,H
    executeTest(&z80, &mmu, 0x7D, 0, 0, 0, 0, 4);        // LD A,L
    executeTest(&z80, &mmu, 0x7E, 0, 0, 0, 0, 8);        // LD A,(HL)
    executeTest(&z80, &mmu, 0x7F, 0, 0, 0, 0, 4);        // LD A,A
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0, 4);        // ADD A,B
    executeTest(&z80, &mmu, 0x81, 0, 0, 0, 0, 4);        // ADD A,C
    executeTest(&z80, &mmu, 0x82, 0, 0, 0, 0, 4);        // ADD A,D
    executeTest(&z80, &mmu, 0x83, 0, 0, 0, 0, 4);        // ADD A,E
    executeTest(&z80, &mmu, 0x84, 0, 0, 0, 0, 4);        // ADD A,H
    executeTest(&z80, &mmu, 0x85, 0, 0, 0, 0, 4);        // ADD A,L
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0, 8);        // ADD A,(HL)
    executeTest(&z80, &mmu, 0x87, 0, 0, 0, 0, 4);        // ADD A,A
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0, 4);        // ADC A,B
    executeTest(&z80, &mmu, 0x89, 0, 0, 0, 0, 4);        // ADC A,C
    executeTest(&z80, &mmu, 0x8A, 0, 0, 0, 0, 4);        // ADC A,D
    executeTest(&z80, &mmu, 0x8B, 0, 0, 0, 0, 4);        // ADC A,E
    executeTest(&z80, &mmu, 0x8C, 0, 0, 0, 0, 4);        // ADC A,H
    executeTest(&z80, &mmu, 0x8D, 0, 0, 0, 0, 4);        // ADC A,L
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0, 8);        // ADC A,(HL)
    executeTest(&z80, &mmu, 0x8F, 0, 0, 0, 0, 4);        // ADC A,A
    executeTest(&z80, &mmu, 0x90, 0, 0, 0, 0, 4);        // SUB A,B
    executeTest(&z80, &mmu, 0x91, 0, 0, 0, 0, 4);        // SUB A,C
    executeTest(&z80, &mmu, 0x92, 0, 0, 0, 0, 4);        // SUB A,D
    executeTest(&z80, &mmu, 0x93, 0, 0, 0, 0, 4);        // SUB A,E
    executeTest(&z80, &mmu, 0x94, 0, 0, 0, 0, 4);        // SUB A,H
    executeTest(&z80, &mmu, 0x95, 0, 0, 0, 0, 4);        // SUB A,L
    executeTest(&z80, &mmu, 0x96, 0, 0, 0, 0, 8);        // SUB A,(HL)
    executeTest(&z80, &mmu, 0x97, 0, 0, 0, 0, 4);        // SUB A,A
    executeTest(&z80, &mmu, 0x98, 0, 0, 0, 0, 4);        // SBC A,B
    executeTest(&z80, &mmu, 0x99, 0, 0, 0, 0, 4);        // SBC A,C
    executeTest(&z80, &mmu, 0x9A, 0, 0, 0, 0, 4);        // SBC A,D
    executeTest(&z80, &mmu, 0x9B, 0, 0, 0, 0, 4);        // SBC A,E
    executeTest(&z80, &mmu, 0x9C, 0, 0, 0, 0, 4);        // SBC A,H
    executeTest(&z80, &mmu, 0x9D, 0, 0, 0, 0, 4);        // SBC A,L
    executeTest(&z80, &mmu, 0x9E, 0, 0, 0, 0, 8);        // SBC A,(HL)
    executeTest(&z80, &mmu, 0x9F, 0, 0, 0, 0, 4);        // SBC A,A
    executeTest(&z80, &mmu, 0xA0, 0, 0, 0, 0, 4);        // AND A,B
    executeTest(&z80, &mmu, 0xA1, 0, 0, 0, 0, 4);        // AND A,C
    executeTest(&z80, &mmu, 0xA2, 0, 0, 0, 0, 4);        // AND A,D
    executeTest(&z80, &mmu, 0xA3, 0, 0, 0, 0, 4);        // AND A,E
    executeTest(&z80, &mmu, 0xA4, 0, 0, 0, 0, 4);        // AND A,H
    executeTest(&z80, &mmu, 0xA5, 0, 0, 0, 0, 4);        // AND A,L
    executeTest(&z80, &mmu, 0xA6, 0, 0, 0, 0, 8);        // AND A,(HL)
    executeTest(&z80, &mmu, 0xA7, 0, 0, 0, 0, 4);        // AND A,A
    executeTest(&z80, &mmu, 0xA8, 0, 0, 0, 0, 4);        // XOR A,B
    executeTest(&z80, &mmu, 0xA9, 0, 0, 0, 0, 4);        // XOR A,C
    executeTest(&z80, &mmu, 0xAA, 0, 0, 0, 0, 4);        // XOR A,D
    executeTest(&z80, &mmu, 0xAB, 0, 0, 0, 0, 4);        // XOR A,E
    executeTest(&z80, &mmu, 0xAC, 0, 0, 0, 0, 4);        // XOR A,H
    executeTest(&z80, &mmu, 0xAD, 0, 0, 0, 0, 4);        // XOR A,L
    executeTest(&z80, &mmu, 0xAE, 0, 0, 0, 0, 8);        // XOR A,(HL)
    executeTest(&z80, &mmu, 0xAF, 0, 0, 0, 0, 4);        // XOR A,A
    executeTest(&z80, &mmu, 0xB0, 0, 0, 0, 0, 4);        // OR A,B
    executeTest(&z80, &mmu, 0xB1, 0, 0, 0, 0, 4);        // OR A,C
    executeTest(&z80, &mmu, 0xB2, 0, 0, 0, 0, 4);        // OR A,D
    executeTest(&z80, &mmu, 0xB3, 0, 0, 0, 0, 4);        // OR A,E
    executeTest(&z80, &mmu, 0xB4, 0, 0, 0, 0, 4);        // OR A,H
    executeTest(&z80, &mmu, 0xB5, 0, 0, 0, 0, 4);        // OR A,L
    executeTest(&z80, &mmu, 0xB6, 0, 0, 0, 0, 8);        // OR A,(HL)
    executeTest(&z80, &mmu, 0xB7, 0, 0, 0, 0, 4);        // OR A,A
    executeTest(&z80, &mmu, 0xB8, 0, 0, 0, 0, 4);        // CP A,B
    executeTest(&z80, &mmu, 0xB9, 0, 0, 0, 0, 4);        // CP A,C
    executeTest(&z80, &mmu, 0xBA, 0, 0, 0, 0, 4);        // CP A,D
    executeTest(&z80, &mmu, 0xBB, 0, 0, 0, 0, 4);        // CP A,E
    executeTest(&z80, &mmu, 0xBC, 0, 0, 0, 0, 4);        // CP A,H
    executeTest(&z80, &mmu, 0xBD, 0, 0, 0, 0, 4);        // CP A,L
    executeTest(&z80, &mmu, 0xBE, 0, 0, 0, 0, 8);        // CP A,(HL)
    executeTest(&z80, &mmu, 0xBF, 0, 0, 0, 0, 4);        // CP A,A
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 0x40, 8);     // RET NZ
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 0x00, 20);    // RET NZ
    // TODO: execute other tests...

    return 0;
}
