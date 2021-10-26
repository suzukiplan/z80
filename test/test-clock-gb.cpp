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
    executeTest(&z80, &mmu, 0x00, 0, 0, 0, 0, 4);           // NOP
    executeTest(&z80, &mmu, 0x01, 0x34, 0x12, 0, 0, 12);    // LD BC,d16
    executeTest(&z80, &mmu, 0x02, 0, 0, 0, 0, 8);           // LD (BC),A
    executeTest(&z80, &mmu, 0x03, 0, 0, 0, 0, 8);           // INC BC
    executeTest(&z80, &mmu, 0x04, 0, 0, 0, 0, 4);           // INC B
    executeTest(&z80, &mmu, 0x05, 0, 0, 0, 0, 4);           // DEC B
    executeTest(&z80, &mmu, 0x06, 123, 0, 0, 0, 8);         // LD B,d8
    executeTest(&z80, &mmu, 0x07, 0, 0, 0, 0, 4);           // RLCA
    executeTest(&z80, &mmu, 0x08, 0xCD, 0xAB, 0, 0, 20);    // LD (a16),SP
    executeTest(&z80, &mmu, 0x09, 0, 0, 0, 0, 8);           // ADD HL,BC
    executeTest(&z80, &mmu, 0x0A, 0, 0, 0, 0, 8);           // LD A,(BC)
    executeTest(&z80, &mmu, 0x0B, 0, 0, 0, 0, 8);           // DEC BC
    executeTest(&z80, &mmu, 0x0C, 0, 0, 0, 0, 4);           // INC C
    executeTest(&z80, &mmu, 0x0D, 0, 0, 0, 0, 4);           // DEC C
    executeTest(&z80, &mmu, 0x0E, 98, 0, 0, 0, 8);          // LD C,d8
    executeTest(&z80, &mmu, 0x0F, 0, 0, 0, 0, 4);           // RRCA
    executeTest(&z80, &mmu, 0x10, 0, 0, 0, 0, 4);           // STOP
    executeTest(&z80, &mmu, 0x11, 0x34, 0x12, 0, 0, 12);    // LD DE,d16
    executeTest(&z80, &mmu, 0x12, 0xCD, 0, 0, 0, 8);        // LD (DE),A
    executeTest(&z80, &mmu, 0x13, 0, 0, 0, 0, 8);           // INC DE
    executeTest(&z80, &mmu, 0x14, 0, 0, 0, 0, 4);           // INC D
    executeTest(&z80, &mmu, 0x15, 0, 0, 0, 0, 4);           // DEC D
    executeTest(&z80, &mmu, 0x16, 3, 0, 0, 0, 8);           // LD D,d8
    executeTest(&z80, &mmu, 0x17, 0, 0, 0, 0, 4);           // RLA
    executeTest(&z80, &mmu, 0x18, 0x80, 0, 0, 0, 12);       // JR r8
    executeTest(&z80, &mmu, 0x19, 3, 0, 0, 0, 8);           // ADD HL,DE
    executeTest(&z80, &mmu, 0x1A, 3, 0, 0, 0, 8);           // LD A,(DE)
    executeTest(&z80, &mmu, 0x1B, 3, 0, 0, 0, 8);           // DEC DE
    executeTest(&z80, &mmu, 0x1C, 0, 0, 0, 0, 4);           // INC D
    executeTest(&z80, &mmu, 0x1D, 0, 0, 0, 0, 4);           // DEC D
    executeTest(&z80, &mmu, 0x1E, 12, 0, 0, 0, 8);          // LD E,d8
    executeTest(&z80, &mmu, 0x1F, 0, 0, 0, 0, 4);           // RRA
    executeTest(&z80, &mmu, 0x20, 0, 0, 0, 0x40, 8);        // JR NZ,r8
    executeTest(&z80, &mmu, 0x20, 0, 0, 0, 0x00, 12);       // JR NZ,r8
    executeTest(&z80, &mmu, 0x21, 0x34, 0x12, 0, 0, 12);    // LD HL,d16
    executeTest(&z80, &mmu, 0x22, 0, 0, 0, 0, 8);           // LD (HL+),A
    executeTest(&z80, &mmu, 0x23, 0, 0, 0, 0, 8);           // INC HL
    executeTest(&z80, &mmu, 0x24, 0, 0, 0, 0, 4);           // INC H
    executeTest(&z80, &mmu, 0x25, 0, 0, 0, 0, 4);           // DEC H
    executeTest(&z80, &mmu, 0x26, 0x33, 0, 0, 0, 8);        // LD H,d8
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0, 4);           // DAA
    executeTest(&z80, &mmu, 0x28, 0, 0, 0, 0x00, 8);        // JR Z,r8
    executeTest(&z80, &mmu, 0x28, 0, 0, 0, 0x40, 12);       // JR Z,r8
    executeTest(&z80, &mmu, 0x29, 0, 0, 0, 0, 8);           // ADD HL,HL
    executeTest(&z80, &mmu, 0x2A, 0, 0, 0, 0, 8);           // LD A,(HL+)
    executeTest(&z80, &mmu, 0x2B, 0, 0, 0, 0, 8);           // DEC HL
    executeTest(&z80, &mmu, 0x2C, 0, 0, 0, 0, 4);           // INC L
    executeTest(&z80, &mmu, 0x2D, 0, 0, 0, 0, 4);           // DEC L
    executeTest(&z80, &mmu, 0x2E, 12, 0, 0, 0, 8);          // LD L,d8
    executeTest(&z80, &mmu, 0x2F, 0, 0, 0, 0, 4);           // CPL
    executeTest(&z80, &mmu, 0x30, 0, 0, 0, 0x01, 8);        // JR NC,r8
    executeTest(&z80, &mmu, 0x30, 0, 0, 0, 0x00, 12);       // JR NC,r8
    executeTest(&z80, &mmu, 0x31, 0x22, 0x11, 0, 0, 12);    // LD SP,d16
    executeTest(&z80, &mmu, 0x32, 0, 0, 0, 0, 8);           // LD (HL-),A
    executeTest(&z80, &mmu, 0x33, 0, 0, 0, 0, 8);           // INC SP
    executeTest(&z80, &mmu, 0x34, 0, 0, 0, 0, 12);          // INC (HL)
    executeTest(&z80, &mmu, 0x35, 0, 0, 0, 0, 12);          // DEC (HL)
    executeTest(&z80, &mmu, 0x36, 99, 0, 0, 0, 12);         // LD (HL),d8
    executeTest(&z80, &mmu, 0x37, 0, 0, 0, 0, 4);           // SCF
    executeTest(&z80, &mmu, 0x38, 0, 0, 0, 0x00, 8);        // JR C,r8
    executeTest(&z80, &mmu, 0x38, 0, 0, 0, 0x01, 12);       // JR C,r8
    executeTest(&z80, &mmu, 0x39, 0, 0, 0, 0, 8);           // ADD HL,SP
    executeTest(&z80, &mmu, 0x3A, 0, 0, 0, 0, 8);           // LD A,(HL-)
    executeTest(&z80, &mmu, 0x3B, 0, 0, 0, 0, 8);           // DEC SP
    executeTest(&z80, &mmu, 0x3C, 0, 0, 0, 0, 4);           // INC A
    executeTest(&z80, &mmu, 0x3D, 0, 0, 0, 0, 4);           // DEC A
    executeTest(&z80, &mmu, 0x3E, 55, 0, 0, 0, 8);          // LD A,d8
    executeTest(&z80, &mmu, 0x3F, 0, 0, 0, 0, 4);           // CCF
    executeTest(&z80, &mmu, 0x40, 0, 0, 0, 0, 4);           // LD B,B
    executeTest(&z80, &mmu, 0x41, 0, 0, 0, 0, 4);           // LD B,C
    executeTest(&z80, &mmu, 0x42, 0, 0, 0, 0, 4);           // LD B,D
    executeTest(&z80, &mmu, 0x43, 0, 0, 0, 0, 4);           // LD B,E
    executeTest(&z80, &mmu, 0x44, 0, 0, 0, 0, 4);           // LD B,H
    executeTest(&z80, &mmu, 0x45, 0, 0, 0, 0, 4);           // LD B,L
    executeTest(&z80, &mmu, 0x46, 0, 0, 0, 0, 8);           // LD B,(HL)
    executeTest(&z80, &mmu, 0x47, 0, 0, 0, 0, 4);           // LD B,A
    executeTest(&z80, &mmu, 0x48, 0, 0, 0, 0, 4);           // LD C,B
    executeTest(&z80, &mmu, 0x49, 0, 0, 0, 0, 4);           // LD C,C
    executeTest(&z80, &mmu, 0x4A, 0, 0, 0, 0, 4);           // LD C,D
    executeTest(&z80, &mmu, 0x4B, 0, 0, 0, 0, 4);           // LD C,E
    executeTest(&z80, &mmu, 0x4C, 0, 0, 0, 0, 4);           // LD C,H
    executeTest(&z80, &mmu, 0x4D, 0, 0, 0, 0, 4);           // LD C,L
    executeTest(&z80, &mmu, 0x4E, 0, 0, 0, 0, 8);           // LD C,(HL)
    executeTest(&z80, &mmu, 0x4F, 0, 0, 0, 0, 4);           // LD C,A
    executeTest(&z80, &mmu, 0x50, 0, 0, 0, 0, 4);           // LD D,B
    executeTest(&z80, &mmu, 0x51, 0, 0, 0, 0, 4);           // LD D,C
    executeTest(&z80, &mmu, 0x52, 0, 0, 0, 0, 4);           // LD D,D
    executeTest(&z80, &mmu, 0x53, 0, 0, 0, 0, 4);           // LD D,E
    executeTest(&z80, &mmu, 0x54, 0, 0, 0, 0, 4);           // LD D,H
    executeTest(&z80, &mmu, 0x55, 0, 0, 0, 0, 4);           // LD D,L
    executeTest(&z80, &mmu, 0x56, 0, 0, 0, 0, 8);           // LD D,(HL)
    executeTest(&z80, &mmu, 0x57, 0, 0, 0, 0, 4);           // LD D,A
    executeTest(&z80, &mmu, 0x58, 0, 0, 0, 0, 4);           // LD E,B
    executeTest(&z80, &mmu, 0x59, 0, 0, 0, 0, 4);           // LD E,C
    executeTest(&z80, &mmu, 0x5A, 0, 0, 0, 0, 4);           // LD E,D
    executeTest(&z80, &mmu, 0x5B, 0, 0, 0, 0, 4);           // LD E,E
    executeTest(&z80, &mmu, 0x5C, 0, 0, 0, 0, 4);           // LD E,H
    executeTest(&z80, &mmu, 0x5D, 0, 0, 0, 0, 4);           // LD E,L
    executeTest(&z80, &mmu, 0x5E, 0, 0, 0, 0, 8);           // LD E,(HL)
    executeTest(&z80, &mmu, 0x5F, 0, 0, 0, 0, 4);           // LD E,A
    executeTest(&z80, &mmu, 0x60, 0, 0, 0, 0, 4);           // LD H,B
    executeTest(&z80, &mmu, 0x61, 0, 0, 0, 0, 4);           // LD H,C
    executeTest(&z80, &mmu, 0x62, 0, 0, 0, 0, 4);           // LD H,D
    executeTest(&z80, &mmu, 0x63, 0, 0, 0, 0, 4);           // LD H,E
    executeTest(&z80, &mmu, 0x64, 0, 0, 0, 0, 4);           // LD H,H
    executeTest(&z80, &mmu, 0x65, 0, 0, 0, 0, 4);           // LD H,L
    executeTest(&z80, &mmu, 0x66, 0, 0, 0, 0, 8);           // LD H,(HL)
    executeTest(&z80, &mmu, 0x67, 0, 0, 0, 0, 4);           // LD H,A
    executeTest(&z80, &mmu, 0x68, 0, 0, 0, 0, 4);           // LD L,B
    executeTest(&z80, &mmu, 0x69, 0, 0, 0, 0, 4);           // LD L,C
    executeTest(&z80, &mmu, 0x6A, 0, 0, 0, 0, 4);           // LD L,D
    executeTest(&z80, &mmu, 0x6B, 0, 0, 0, 0, 4);           // LD L,E
    executeTest(&z80, &mmu, 0x6C, 0, 0, 0, 0, 4);           // LD L,H
    executeTest(&z80, &mmu, 0x6D, 0, 0, 0, 0, 4);           // LD L,L
    executeTest(&z80, &mmu, 0x6E, 0, 0, 0, 0, 8);           // LD L,(HL)
    executeTest(&z80, &mmu, 0x6F, 0, 0, 0, 0, 4);           // LD L,A
    executeTest(&z80, &mmu, 0x70, 0, 0, 0, 0, 8);           // LD (HL),B
    executeTest(&z80, &mmu, 0x71, 0, 0, 0, 0, 8);           // LD (HL),C
    executeTest(&z80, &mmu, 0x72, 0, 0, 0, 0, 8);           // LD (HL),D
    executeTest(&z80, &mmu, 0x73, 0, 0, 0, 0, 8);           // LD (HL),E
    executeTest(&z80, &mmu, 0x74, 0, 0, 0, 0, 8);           // LD (HL),H
    executeTest(&z80, &mmu, 0x75, 0, 0, 0, 0, 8);           // LD (HL),L
    executeTest(&z80, &mmu, 0x76, 0, 0, 0, 0, 4);           // HALT
    executeTest(&z80, &mmu, 0x77, 0, 0, 0, 0, 8);           // LD (HL),A
    executeTest(&z80, &mmu, 0x78, 0, 0, 0, 0, 4);           // LD A,B
    executeTest(&z80, &mmu, 0x79, 0, 0, 0, 0, 4);           // LD A,C
    executeTest(&z80, &mmu, 0x7A, 0, 0, 0, 0, 4);           // LD A,D
    executeTest(&z80, &mmu, 0x7B, 0, 0, 0, 0, 4);           // LD A,E
    executeTest(&z80, &mmu, 0x7C, 0, 0, 0, 0, 4);           // LD A,H
    executeTest(&z80, &mmu, 0x7D, 0, 0, 0, 0, 4);           // LD A,L
    executeTest(&z80, &mmu, 0x7E, 0, 0, 0, 0, 8);           // LD A,(HL)
    executeTest(&z80, &mmu, 0x7F, 0, 0, 0, 0, 4);           // LD A,A
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0, 4);           // ADD A,B
    executeTest(&z80, &mmu, 0x81, 0, 0, 0, 0, 4);           // ADD A,C
    executeTest(&z80, &mmu, 0x82, 0, 0, 0, 0, 4);           // ADD A,D
    executeTest(&z80, &mmu, 0x83, 0, 0, 0, 0, 4);           // ADD A,E
    executeTest(&z80, &mmu, 0x84, 0, 0, 0, 0, 4);           // ADD A,H
    executeTest(&z80, &mmu, 0x85, 0, 0, 0, 0, 4);           // ADD A,L
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0, 8);           // ADD A,(HL)
    executeTest(&z80, &mmu, 0x87, 0, 0, 0, 0, 4);           // ADD A,A
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0, 4);           // ADC A,B
    executeTest(&z80, &mmu, 0x89, 0, 0, 0, 0, 4);           // ADC A,C
    executeTest(&z80, &mmu, 0x8A, 0, 0, 0, 0, 4);           // ADC A,D
    executeTest(&z80, &mmu, 0x8B, 0, 0, 0, 0, 4);           // ADC A,E
    executeTest(&z80, &mmu, 0x8C, 0, 0, 0, 0, 4);           // ADC A,H
    executeTest(&z80, &mmu, 0x8D, 0, 0, 0, 0, 4);           // ADC A,L
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0, 8);           // ADC A,(HL)
    executeTest(&z80, &mmu, 0x8F, 0, 0, 0, 0, 4);           // ADC A,A
    executeTest(&z80, &mmu, 0x90, 0, 0, 0, 0, 4);           // SUB A,B
    executeTest(&z80, &mmu, 0x91, 0, 0, 0, 0, 4);           // SUB A,C
    executeTest(&z80, &mmu, 0x92, 0, 0, 0, 0, 4);           // SUB A,D
    executeTest(&z80, &mmu, 0x93, 0, 0, 0, 0, 4);           // SUB A,E
    executeTest(&z80, &mmu, 0x94, 0, 0, 0, 0, 4);           // SUB A,H
    executeTest(&z80, &mmu, 0x95, 0, 0, 0, 0, 4);           // SUB A,L
    executeTest(&z80, &mmu, 0x96, 0, 0, 0, 0, 8);           // SUB A,(HL)
    executeTest(&z80, &mmu, 0x97, 0, 0, 0, 0, 4);           // SUB A,A
    executeTest(&z80, &mmu, 0x98, 0, 0, 0, 0, 4);           // SBC A,B
    executeTest(&z80, &mmu, 0x99, 0, 0, 0, 0, 4);           // SBC A,C
    executeTest(&z80, &mmu, 0x9A, 0, 0, 0, 0, 4);           // SBC A,D
    executeTest(&z80, &mmu, 0x9B, 0, 0, 0, 0, 4);           // SBC A,E
    executeTest(&z80, &mmu, 0x9C, 0, 0, 0, 0, 4);           // SBC A,H
    executeTest(&z80, &mmu, 0x9D, 0, 0, 0, 0, 4);           // SBC A,L
    executeTest(&z80, &mmu, 0x9E, 0, 0, 0, 0, 8);           // SBC A,(HL)
    executeTest(&z80, &mmu, 0x9F, 0, 0, 0, 0, 4);           // SBC A,A
    executeTest(&z80, &mmu, 0xA0, 0, 0, 0, 0, 4);           // AND A,B
    executeTest(&z80, &mmu, 0xA1, 0, 0, 0, 0, 4);           // AND A,C
    executeTest(&z80, &mmu, 0xA2, 0, 0, 0, 0, 4);           // AND A,D
    executeTest(&z80, &mmu, 0xA3, 0, 0, 0, 0, 4);           // AND A,E
    executeTest(&z80, &mmu, 0xA4, 0, 0, 0, 0, 4);           // AND A,H
    executeTest(&z80, &mmu, 0xA5, 0, 0, 0, 0, 4);           // AND A,L
    executeTest(&z80, &mmu, 0xA6, 0, 0, 0, 0, 8);           // AND A,(HL)
    executeTest(&z80, &mmu, 0xA7, 0, 0, 0, 0, 4);           // AND A,A
    executeTest(&z80, &mmu, 0xA8, 0, 0, 0, 0, 4);           // XOR A,B
    executeTest(&z80, &mmu, 0xA9, 0, 0, 0, 0, 4);           // XOR A,C
    executeTest(&z80, &mmu, 0xAA, 0, 0, 0, 0, 4);           // XOR A,D
    executeTest(&z80, &mmu, 0xAB, 0, 0, 0, 0, 4);           // XOR A,E
    executeTest(&z80, &mmu, 0xAC, 0, 0, 0, 0, 4);           // XOR A,H
    executeTest(&z80, &mmu, 0xAD, 0, 0, 0, 0, 4);           // XOR A,L
    executeTest(&z80, &mmu, 0xAE, 0, 0, 0, 0, 8);           // XOR A,(HL)
    executeTest(&z80, &mmu, 0xAF, 0, 0, 0, 0, 4);           // XOR A,A
    executeTest(&z80, &mmu, 0xB0, 0, 0, 0, 0, 4);           // OR A,B
    executeTest(&z80, &mmu, 0xB1, 0, 0, 0, 0, 4);           // OR A,C
    executeTest(&z80, &mmu, 0xB2, 0, 0, 0, 0, 4);           // OR A,D
    executeTest(&z80, &mmu, 0xB3, 0, 0, 0, 0, 4);           // OR A,E
    executeTest(&z80, &mmu, 0xB4, 0, 0, 0, 0, 4);           // OR A,H
    executeTest(&z80, &mmu, 0xB5, 0, 0, 0, 0, 4);           // OR A,L
    executeTest(&z80, &mmu, 0xB6, 0, 0, 0, 0, 8);           // OR A,(HL)
    executeTest(&z80, &mmu, 0xB7, 0, 0, 0, 0, 4);           // OR A,A
    executeTest(&z80, &mmu, 0xB8, 0, 0, 0, 0, 4);           // CP A,B
    executeTest(&z80, &mmu, 0xB9, 0, 0, 0, 0, 4);           // CP A,C
    executeTest(&z80, &mmu, 0xBA, 0, 0, 0, 0, 4);           // CP A,D
    executeTest(&z80, &mmu, 0xBB, 0, 0, 0, 0, 4);           // CP A,E
    executeTest(&z80, &mmu, 0xBC, 0, 0, 0, 0, 4);           // CP A,H
    executeTest(&z80, &mmu, 0xBD, 0, 0, 0, 0, 4);           // CP A,L
    executeTest(&z80, &mmu, 0xBE, 0, 0, 0, 0, 8);           // CP A,(HL)
    executeTest(&z80, &mmu, 0xBF, 0, 0, 0, 0, 4);           // CP A,A
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 0x40, 8);        // RET NZ
    executeTest(&z80, &mmu, 0xC0, 0, 0, 0, 0x00, 20);       // RET NZ
    executeTest(&z80, &mmu, 0xC1, 0, 0, 0, 0, 12);          // POP BC
    executeTest(&z80, &mmu, 0xC2, 0, 0, 0, 0x40, 12);       // JP NZ,a16
    executeTest(&z80, &mmu, 0xC2, 0, 0, 0, 0x00, 16);       // JP NZ,a16
    executeTest(&z80, &mmu, 0xC3, 0x34, 0x12, 0, 0, 16);    // JP HL
    executeTest(&z80, &mmu, 0xC4, 0, 0, 0, 0x40, 12);       // CALL NZ,a16
    executeTest(&z80, &mmu, 0xC4, 0, 0, 0, 0x00, 24);       // CALL NZ,a16
    executeTest(&z80, &mmu, 0xC5, 0, 0, 0, 0, 12);          // POP BC
    executeTest(&z80, &mmu, 0xC6, 255, 0, 0, 0, 8);         // ADD A,d8
    executeTest(&z80, &mmu, 0xC7, 0, 0, 0, 0, 16);          // RST 00H
    executeTest(&z80, &mmu, 0xC8, 0, 0, 0, 0x00, 8);        // RET N
    executeTest(&z80, &mmu, 0xC8, 0, 0, 0, 0x40, 20);       // RET Z
    executeTest(&z80, &mmu, 0xC9, 0, 0, 0, 0, 16);          // RET
    executeTest(&z80, &mmu, 0xCA, 0, 0, 0, 0x00, 12);       // JP Z,a16
    executeTest(&z80, &mmu, 0xCA, 0, 0, 0, 0x40, 16);       // JP Z,a16
    executeTest(&z80, &mmu, 0xCB, 0x00, 0, 0, 0, 8);        // RLC B
    executeTest(&z80, &mmu, 0xCB, 0x01, 0, 0, 0, 8);        // RLC C
    executeTest(&z80, &mmu, 0xCB, 0x02, 0, 0, 0, 8);        // RLC D
    executeTest(&z80, &mmu, 0xCB, 0x03, 0, 0, 0, 8);        // RLC E
    executeTest(&z80, &mmu, 0xCB, 0x04, 0, 0, 0, 8);        // RLC H
    executeTest(&z80, &mmu, 0xCB, 0x05, 0, 0, 0, 8);        // RLC L
    executeTest(&z80, &mmu, 0xCB, 0x06, 0, 0, 0, 16);       // RLC (HL)
    executeTest(&z80, &mmu, 0xCB, 0x07, 0, 0, 0, 8);        // RLC A
    executeTest(&z80, &mmu, 0xCB, 0x08, 0, 0, 0, 8);        // RRC B
    executeTest(&z80, &mmu, 0xCB, 0x09, 0, 0, 0, 8);        // RRC C
    executeTest(&z80, &mmu, 0xCB, 0x0A, 0, 0, 0, 8);        // RRC D
    executeTest(&z80, &mmu, 0xCB, 0x0B, 0, 0, 0, 8);        // RRC E
    executeTest(&z80, &mmu, 0xCB, 0x0C, 0, 0, 0, 8);        // RRC H
    executeTest(&z80, &mmu, 0xCB, 0x0D, 0, 0, 0, 8);        // RRC L
    executeTest(&z80, &mmu, 0xCB, 0x0E, 0, 0, 0, 16);       // RRC (HL)
    executeTest(&z80, &mmu, 0xCB, 0x0F, 0, 0, 0, 8);        // RRC A
    executeTest(&z80, &mmu, 0xCB, 0x10, 0, 0, 0, 8);        // RL B
    executeTest(&z80, &mmu, 0xCB, 0x11, 0, 0, 0, 8);        // RL C
    executeTest(&z80, &mmu, 0xCB, 0x12, 0, 0, 0, 8);        // RL D
    executeTest(&z80, &mmu, 0xCB, 0x13, 0, 0, 0, 8);        // RL E
    executeTest(&z80, &mmu, 0xCB, 0x14, 0, 0, 0, 8);        // RL H
    executeTest(&z80, &mmu, 0xCB, 0x15, 0, 0, 0, 8);        // RL L
    executeTest(&z80, &mmu, 0xCB, 0x16, 0, 0, 0, 16);       // RL (HL)
    executeTest(&z80, &mmu, 0xCB, 0x17, 0, 0, 0, 8);        // RL A
    executeTest(&z80, &mmu, 0xCB, 0x18, 0, 0, 0, 8);        // RR B
    executeTest(&z80, &mmu, 0xCB, 0x19, 0, 0, 0, 8);        // RR C
    executeTest(&z80, &mmu, 0xCB, 0x1A, 0, 0, 0, 8);        // RR D
    executeTest(&z80, &mmu, 0xCB, 0x1B, 0, 0, 0, 8);        // RR E
    executeTest(&z80, &mmu, 0xCB, 0x1C, 0, 0, 0, 8);        // RR H
    executeTest(&z80, &mmu, 0xCB, 0x1D, 0, 0, 0, 8);        // RR L
    executeTest(&z80, &mmu, 0xCB, 0x1E, 0, 0, 0, 16);       // RR (HL)
    executeTest(&z80, &mmu, 0xCB, 0x1F, 0, 0, 0, 8);        // RR A
    executeTest(&z80, &mmu, 0xCB, 0x20, 0, 0, 0, 8);        // SLA B
    executeTest(&z80, &mmu, 0xCB, 0x21, 0, 0, 0, 8);        // SLA C
    executeTest(&z80, &mmu, 0xCB, 0x22, 0, 0, 0, 8);        // SLA D
    executeTest(&z80, &mmu, 0xCB, 0x23, 0, 0, 0, 8);        // SLA E
    executeTest(&z80, &mmu, 0xCB, 0x24, 0, 0, 0, 8);        // SLA H
    executeTest(&z80, &mmu, 0xCB, 0x25, 0, 0, 0, 8);        // SLA L
    executeTest(&z80, &mmu, 0xCB, 0x26, 0, 0, 0, 16);       // SLA (HL)
    executeTest(&z80, &mmu, 0xCB, 0x27, 0, 0, 0, 8);        // SLA A
    executeTest(&z80, &mmu, 0xCB, 0x28, 0, 0, 0, 8);        // SRA B
    executeTest(&z80, &mmu, 0xCB, 0x29, 0, 0, 0, 8);        // SRA C
    executeTest(&z80, &mmu, 0xCB, 0x2A, 0, 0, 0, 8);        // SRA D
    executeTest(&z80, &mmu, 0xCB, 0x2B, 0, 0, 0, 8);        // SRA E
    executeTest(&z80, &mmu, 0xCB, 0x2C, 0, 0, 0, 8);        // SRA H
    executeTest(&z80, &mmu, 0xCB, 0x2D, 0, 0, 0, 8);        // SRA L
    executeTest(&z80, &mmu, 0xCB, 0x2E, 0, 0, 0, 16);       // SRA (HL)
    executeTest(&z80, &mmu, 0xCB, 0x2F, 0, 0, 0, 8);        // SRA A
    executeTest(&z80, &mmu, 0xCB, 0x30, 0, 0, 0, 8);        // SWAP B
    executeTest(&z80, &mmu, 0xCB, 0x31, 0, 0, 0, 8);        // SWAP C
    executeTest(&z80, &mmu, 0xCB, 0x32, 0, 0, 0, 8);        // SWAP D
    executeTest(&z80, &mmu, 0xCB, 0x33, 0, 0, 0, 8);        // SWAP E
    executeTest(&z80, &mmu, 0xCB, 0x34, 0, 0, 0, 8);        // SWAP H
    executeTest(&z80, &mmu, 0xCB, 0x35, 0, 0, 0, 8);        // SWAP L
    executeTest(&z80, &mmu, 0xCB, 0x36, 0, 0, 0, 16);       // SWAP (HL)
    executeTest(&z80, &mmu, 0xCB, 0x37, 0, 0, 0, 8);        // SWAP A
    executeTest(&z80, &mmu, 0xCB, 0x38, 0, 0, 0, 8);        // SRL B
    executeTest(&z80, &mmu, 0xCB, 0x39, 0, 0, 0, 8);        // SRL C
    executeTest(&z80, &mmu, 0xCB, 0x3A, 0, 0, 0, 8);        // SRL D
    executeTest(&z80, &mmu, 0xCB, 0x3B, 0, 0, 0, 8);        // SRL E
    executeTest(&z80, &mmu, 0xCB, 0x3C, 0, 0, 0, 8);        // SRL H
    executeTest(&z80, &mmu, 0xCB, 0x3D, 0, 0, 0, 8);        // SRL L
    executeTest(&z80, &mmu, 0xCB, 0x3E, 0, 0, 0, 16);       // SRL (HL)
    executeTest(&z80, &mmu, 0xCB, 0x3F, 0, 0, 0, 8);        // SRL A
    executeTest(&z80, &mmu, 0xCB, 0x40, 0, 0, 0, 8);        // BIT 0, B
    executeTest(&z80, &mmu, 0xCB, 0x41, 0, 0, 0, 8);        // BIT 0, C
    executeTest(&z80, &mmu, 0xCB, 0x42, 0, 0, 0, 8);        // BIT 0, D
    executeTest(&z80, &mmu, 0xCB, 0x43, 0, 0, 0, 8);        // BIT 0, E
    executeTest(&z80, &mmu, 0xCB, 0x44, 0, 0, 0, 8);        // BIT 0, H
    executeTest(&z80, &mmu, 0xCB, 0x45, 0, 0, 0, 8);        // BIT 0, L
    executeTest(&z80, &mmu, 0xCB, 0x46, 0, 0, 0, 12);       // BIT 0, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x47, 0, 0, 0, 8);        // BIT 0, A
    executeTest(&z80, &mmu, 0xCB, 0x48, 0, 0, 0, 8);        // BIT 1, B
    executeTest(&z80, &mmu, 0xCB, 0x49, 0, 0, 0, 8);        // BIT 1, C
    executeTest(&z80, &mmu, 0xCB, 0x4A, 0, 0, 0, 8);        // BIT 1, D
    executeTest(&z80, &mmu, 0xCB, 0x4B, 0, 0, 0, 8);        // BIT 1, E
    executeTest(&z80, &mmu, 0xCB, 0x4C, 0, 0, 0, 8);        // BIT 1, H
    executeTest(&z80, &mmu, 0xCB, 0x4D, 0, 0, 0, 8);        // BIT 1, L
    executeTest(&z80, &mmu, 0xCB, 0x4E, 0, 0, 0, 12);       // BIT 1, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x4F, 0, 0, 0, 8);        // BIT 1, A
    executeTest(&z80, &mmu, 0xCB, 0x50, 0, 0, 0, 8);        // BIT 2, B
    executeTest(&z80, &mmu, 0xCB, 0x51, 0, 0, 0, 8);        // BIT 2, C
    executeTest(&z80, &mmu, 0xCB, 0x52, 0, 0, 0, 8);        // BIT 2, D
    executeTest(&z80, &mmu, 0xCB, 0x53, 0, 0, 0, 8);        // BIT 2, E
    executeTest(&z80, &mmu, 0xCB, 0x54, 0, 0, 0, 8);        // BIT 2, H
    executeTest(&z80, &mmu, 0xCB, 0x55, 0, 0, 0, 8);        // BIT 2, L
    executeTest(&z80, &mmu, 0xCB, 0x56, 0, 0, 0, 12);       // BIT 2, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x57, 0, 0, 0, 8);        // BIT 2, A
    executeTest(&z80, &mmu, 0xCB, 0x58, 0, 0, 0, 8);        // BIT 3, B
    executeTest(&z80, &mmu, 0xCB, 0x59, 0, 0, 0, 8);        // BIT 3, C
    executeTest(&z80, &mmu, 0xCB, 0x5A, 0, 0, 0, 8);        // BIT 3, D
    executeTest(&z80, &mmu, 0xCB, 0x5B, 0, 0, 0, 8);        // BIT 3, E
    executeTest(&z80, &mmu, 0xCB, 0x5C, 0, 0, 0, 8);        // BIT 3, H
    executeTest(&z80, &mmu, 0xCB, 0x5D, 0, 0, 0, 8);        // BIT 3, L
    executeTest(&z80, &mmu, 0xCB, 0x5E, 0, 0, 0, 12);       // BIT 3, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x5F, 0, 0, 0, 8);        // BIT 3, A
    executeTest(&z80, &mmu, 0xCB, 0x60, 0, 0, 0, 8);        // BIT 4, B
    executeTest(&z80, &mmu, 0xCB, 0x61, 0, 0, 0, 8);        // BIT 4, C
    executeTest(&z80, &mmu, 0xCB, 0x62, 0, 0, 0, 8);        // BIT 4, D
    executeTest(&z80, &mmu, 0xCB, 0x63, 0, 0, 0, 8);        // BIT 4, E
    executeTest(&z80, &mmu, 0xCB, 0x64, 0, 0, 0, 8);        // BIT 4, H
    executeTest(&z80, &mmu, 0xCB, 0x65, 0, 0, 0, 8);        // BIT 4, L
    executeTest(&z80, &mmu, 0xCB, 0x66, 0, 0, 0, 12);       // BIT 4, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x67, 0, 0, 0, 8);        // BIT 4, A
    executeTest(&z80, &mmu, 0xCB, 0x68, 0, 0, 0, 8);        // BIT 5, B
    executeTest(&z80, &mmu, 0xCB, 0x69, 0, 0, 0, 8);        // BIT 5, C
    executeTest(&z80, &mmu, 0xCB, 0x6A, 0, 0, 0, 8);        // BIT 5, D
    executeTest(&z80, &mmu, 0xCB, 0x6B, 0, 0, 0, 8);        // BIT 5, E
    executeTest(&z80, &mmu, 0xCB, 0x6C, 0, 0, 0, 8);        // BIT 5, H
    executeTest(&z80, &mmu, 0xCB, 0x6D, 0, 0, 0, 8);        // BIT 5, L
    executeTest(&z80, &mmu, 0xCB, 0x6E, 0, 0, 0, 12);       // BIT 5, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x6F, 0, 0, 0, 8);        // BIT 5, A
    executeTest(&z80, &mmu, 0xCB, 0x70, 0, 0, 0, 8);        // BIT 6, B
    executeTest(&z80, &mmu, 0xCB, 0x71, 0, 0, 0, 8);        // BIT 6, C
    executeTest(&z80, &mmu, 0xCB, 0x72, 0, 0, 0, 8);        // BIT 6, D
    executeTest(&z80, &mmu, 0xCB, 0x73, 0, 0, 0, 8);        // BIT 6, E
    executeTest(&z80, &mmu, 0xCB, 0x74, 0, 0, 0, 8);        // BIT 6, H
    executeTest(&z80, &mmu, 0xCB, 0x75, 0, 0, 0, 8);        // BIT 6, L
    executeTest(&z80, &mmu, 0xCB, 0x76, 0, 0, 0, 12);       // BIT 6, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x77, 0, 0, 0, 8);        // BIT 6, A
    executeTest(&z80, &mmu, 0xCB, 0x78, 0, 0, 0, 8);        // BIT 7, B
    executeTest(&z80, &mmu, 0xCB, 0x79, 0, 0, 0, 8);        // BIT 7, C
    executeTest(&z80, &mmu, 0xCB, 0x7A, 0, 0, 0, 8);        // BIT 7, D
    executeTest(&z80, &mmu, 0xCB, 0x7B, 0, 0, 0, 8);        // BIT 7, E
    executeTest(&z80, &mmu, 0xCB, 0x7C, 0, 0, 0, 8);        // BIT 7, H
    executeTest(&z80, &mmu, 0xCB, 0x7D, 0, 0, 0, 8);        // BIT 7, L
    executeTest(&z80, &mmu, 0xCB, 0x7E, 0, 0, 0, 12);       // BIT 7, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x7F, 0, 0, 0, 8);        // BIT 7, A
    executeTest(&z80, &mmu, 0xCB, 0x80, 0, 0, 0, 8);        // RES 0, B
    executeTest(&z80, &mmu, 0xCB, 0x81, 0, 0, 0, 8);        // RES 0, C
    executeTest(&z80, &mmu, 0xCB, 0x82, 0, 0, 0, 8);        // RES 0, D
    executeTest(&z80, &mmu, 0xCB, 0x83, 0, 0, 0, 8);        // RES 0, E
    executeTest(&z80, &mmu, 0xCB, 0x84, 0, 0, 0, 8);        // RES 0, H
    executeTest(&z80, &mmu, 0xCB, 0x85, 0, 0, 0, 8);        // RES 0, L
    executeTest(&z80, &mmu, 0xCB, 0x86, 0, 0, 0, 16);       // RES 0, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x87, 0, 0, 0, 8);        // RES 0, A
    executeTest(&z80, &mmu, 0xCB, 0x88, 0, 0, 0, 8);        // RES 1, B
    executeTest(&z80, &mmu, 0xCB, 0x89, 0, 0, 0, 8);        // RES 1, C
    executeTest(&z80, &mmu, 0xCB, 0x8A, 0, 0, 0, 8);        // RES 1, D
    executeTest(&z80, &mmu, 0xCB, 0x8B, 0, 0, 0, 8);        // RES 1, E
    executeTest(&z80, &mmu, 0xCB, 0x8C, 0, 0, 0, 8);        // RES 1, H
    executeTest(&z80, &mmu, 0xCB, 0x8D, 0, 0, 0, 8);        // RES 1, L
    executeTest(&z80, &mmu, 0xCB, 0x8E, 0, 0, 0, 16);       // RES 1, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x8F, 0, 0, 0, 8);        // RES 1, A
    executeTest(&z80, &mmu, 0xCB, 0x90, 0, 0, 0, 8);        // RES 2, B
    executeTest(&z80, &mmu, 0xCB, 0x91, 0, 0, 0, 8);        // RES 2, C
    executeTest(&z80, &mmu, 0xCB, 0x92, 0, 0, 0, 8);        // RES 2, D
    executeTest(&z80, &mmu, 0xCB, 0x93, 0, 0, 0, 8);        // RES 2, E
    executeTest(&z80, &mmu, 0xCB, 0x94, 0, 0, 0, 8);        // RES 2, H
    executeTest(&z80, &mmu, 0xCB, 0x95, 0, 0, 0, 8);        // RES 2, L
    executeTest(&z80, &mmu, 0xCB, 0x96, 0, 0, 0, 16);       // RES 2, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x97, 0, 0, 0, 8);        // RES 2, A
    executeTest(&z80, &mmu, 0xCB, 0x98, 0, 0, 0, 8);        // RES 3, B
    executeTest(&z80, &mmu, 0xCB, 0x99, 0, 0, 0, 8);        // RES 3, C
    executeTest(&z80, &mmu, 0xCB, 0x9A, 0, 0, 0, 8);        // RES 3, D
    executeTest(&z80, &mmu, 0xCB, 0x9B, 0, 0, 0, 8);        // RES 3, E
    executeTest(&z80, &mmu, 0xCB, 0x9C, 0, 0, 0, 8);        // RES 3, H
    executeTest(&z80, &mmu, 0xCB, 0x9D, 0, 0, 0, 8);        // RES 3, L
    executeTest(&z80, &mmu, 0xCB, 0x9E, 0, 0, 0, 16);       // RES 3, (HL)
    executeTest(&z80, &mmu, 0xCB, 0x9F, 0, 0, 0, 8);        // RES 3, A
    executeTest(&z80, &mmu, 0xCB, 0xA0, 0, 0, 0, 8);        // RES 4, B
    executeTest(&z80, &mmu, 0xCB, 0xA1, 0, 0, 0, 8);        // RES 4, C
    executeTest(&z80, &mmu, 0xCB, 0xA2, 0, 0, 0, 8);        // RES 4, D
    executeTest(&z80, &mmu, 0xCB, 0xA3, 0, 0, 0, 8);        // RES 4, E
    executeTest(&z80, &mmu, 0xCB, 0xA4, 0, 0, 0, 8);        // RES 4, H
    executeTest(&z80, &mmu, 0xCB, 0xA5, 0, 0, 0, 8);        // RES 4, L
    executeTest(&z80, &mmu, 0xCB, 0xA6, 0, 0, 0, 16);       // RES 4, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xA7, 0, 0, 0, 8);        // RES 4, A
    executeTest(&z80, &mmu, 0xCB, 0xA8, 0, 0, 0, 8);        // RES 5, B
    executeTest(&z80, &mmu, 0xCB, 0xA9, 0, 0, 0, 8);        // RES 5, C
    executeTest(&z80, &mmu, 0xCB, 0xAA, 0, 0, 0, 8);        // RES 5, D
    executeTest(&z80, &mmu, 0xCB, 0xAB, 0, 0, 0, 8);        // RES 5, E
    executeTest(&z80, &mmu, 0xCB, 0xAC, 0, 0, 0, 8);        // RES 5, H
    executeTest(&z80, &mmu, 0xCB, 0xAD, 0, 0, 0, 8);        // RES 5, L
    executeTest(&z80, &mmu, 0xCB, 0xAE, 0, 0, 0, 16);       // RES 5, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xAF, 0, 0, 0, 8);        // RES 5, A
    executeTest(&z80, &mmu, 0xCB, 0xB0, 0, 0, 0, 8);        // RES 6, B
    executeTest(&z80, &mmu, 0xCB, 0xB1, 0, 0, 0, 8);        // RES 6, C
    executeTest(&z80, &mmu, 0xCB, 0xB2, 0, 0, 0, 8);        // RES 6, D
    executeTest(&z80, &mmu, 0xCB, 0xB3, 0, 0, 0, 8);        // RES 6, E
    executeTest(&z80, &mmu, 0xCB, 0xB4, 0, 0, 0, 8);        // RES 6, H
    executeTest(&z80, &mmu, 0xCB, 0xB5, 0, 0, 0, 8);        // RES 6, L
    executeTest(&z80, &mmu, 0xCB, 0xB6, 0, 0, 0, 16);       // RES 6, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xB7, 0, 0, 0, 8);        // RES 6, A
    executeTest(&z80, &mmu, 0xCB, 0xB8, 0, 0, 0, 8);        // RES 7, B
    executeTest(&z80, &mmu, 0xCB, 0xB9, 0, 0, 0, 8);        // RES 7, C
    executeTest(&z80, &mmu, 0xCB, 0xBA, 0, 0, 0, 8);        // RES 7, D
    executeTest(&z80, &mmu, 0xCB, 0xBB, 0, 0, 0, 8);        // RES 7, E
    executeTest(&z80, &mmu, 0xCB, 0xBC, 0, 0, 0, 8);        // RES 7, H
    executeTest(&z80, &mmu, 0xCB, 0xBD, 0, 0, 0, 8);        // RES 7, L
    executeTest(&z80, &mmu, 0xCB, 0xBE, 0, 0, 0, 16);       // RES 7, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xBF, 0, 0, 0, 8);        // RES 7, A
    executeTest(&z80, &mmu, 0xCB, 0xC0, 0, 0, 0, 8);        // SET 0, B
    executeTest(&z80, &mmu, 0xCB, 0xC1, 0, 0, 0, 8);        // SET 0, C
    executeTest(&z80, &mmu, 0xCB, 0xC2, 0, 0, 0, 8);        // SET 0, D
    executeTest(&z80, &mmu, 0xCB, 0xC3, 0, 0, 0, 8);        // SET 0, E
    executeTest(&z80, &mmu, 0xCB, 0xC4, 0, 0, 0, 8);        // SET 0, H
    executeTest(&z80, &mmu, 0xCB, 0xC5, 0, 0, 0, 8);        // SET 0, L
    executeTest(&z80, &mmu, 0xCB, 0xC6, 0, 0, 0, 16);       // SET 0, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xC7, 0, 0, 0, 8);        // SET 0, A
    executeTest(&z80, &mmu, 0xCB, 0xC8, 0, 0, 0, 8);        // SET 1, B
    executeTest(&z80, &mmu, 0xCB, 0xC9, 0, 0, 0, 8);        // SET 1, C
    executeTest(&z80, &mmu, 0xCB, 0xCA, 0, 0, 0, 8);        // SET 1, D
    executeTest(&z80, &mmu, 0xCB, 0xCB, 0, 0, 0, 8);        // SET 1, E
    executeTest(&z80, &mmu, 0xCB, 0xCC, 0, 0, 0, 8);        // SET 1, H
    executeTest(&z80, &mmu, 0xCB, 0xCD, 0, 0, 0, 8);        // SET 1, L
    executeTest(&z80, &mmu, 0xCB, 0xCE, 0, 0, 0, 16);       // SET 1, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xCF, 0, 0, 0, 8);        // SET 1, A
    executeTest(&z80, &mmu, 0xCB, 0xD0, 0, 0, 0, 8);        // SET 2, B
    executeTest(&z80, &mmu, 0xCB, 0xD1, 0, 0, 0, 8);        // SET 2, C
    executeTest(&z80, &mmu, 0xCB, 0xD2, 0, 0, 0, 8);        // SET 2, D
    executeTest(&z80, &mmu, 0xCB, 0xD3, 0, 0, 0, 8);        // SET 2, E
    executeTest(&z80, &mmu, 0xCB, 0xD4, 0, 0, 0, 8);        // SET 2, H
    executeTest(&z80, &mmu, 0xCB, 0xD5, 0, 0, 0, 8);        // SET 2, L
    executeTest(&z80, &mmu, 0xCB, 0xD6, 0, 0, 0, 16);       // SET 2, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xD7, 0, 0, 0, 8);        // SET 2, A
    executeTest(&z80, &mmu, 0xCB, 0xD8, 0, 0, 0, 8);        // SET 3, B
    executeTest(&z80, &mmu, 0xCB, 0xD9, 0, 0, 0, 8);        // SET 3, C
    executeTest(&z80, &mmu, 0xCB, 0xDA, 0, 0, 0, 8);        // SET 3, D
    executeTest(&z80, &mmu, 0xCB, 0xDB, 0, 0, 0, 8);        // SET 3, E
    executeTest(&z80, &mmu, 0xCB, 0xDC, 0, 0, 0, 8);        // SET 3, H
    executeTest(&z80, &mmu, 0xCB, 0xDD, 0, 0, 0, 8);        // SET 3, L
    executeTest(&z80, &mmu, 0xCB, 0xDE, 0, 0, 0, 16);       // SET 3, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xDF, 0, 0, 0, 8);        // SET 3, A
    executeTest(&z80, &mmu, 0xCB, 0xE0, 0, 0, 0, 8);        // SET 4, B
    executeTest(&z80, &mmu, 0xCB, 0xE1, 0, 0, 0, 8);        // SET 4, C
    executeTest(&z80, &mmu, 0xCB, 0xE2, 0, 0, 0, 8);        // SET 4, D
    executeTest(&z80, &mmu, 0xCB, 0xE3, 0, 0, 0, 8);        // SET 4, E
    executeTest(&z80, &mmu, 0xCB, 0xE4, 0, 0, 0, 8);        // SET 4, H
    executeTest(&z80, &mmu, 0xCB, 0xE5, 0, 0, 0, 8);        // SET 4, L
    executeTest(&z80, &mmu, 0xCB, 0xE6, 0, 0, 0, 16);       // SET 4, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xE7, 0, 0, 0, 8);        // SET 4, A
    executeTest(&z80, &mmu, 0xCB, 0xE8, 0, 0, 0, 8);        // SET 5, B
    executeTest(&z80, &mmu, 0xCB, 0xE9, 0, 0, 0, 8);        // SET 5, C
    executeTest(&z80, &mmu, 0xCB, 0xEA, 0, 0, 0, 8);        // SET 5, D
    executeTest(&z80, &mmu, 0xCB, 0xEB, 0, 0, 0, 8);        // SET 5, E
    executeTest(&z80, &mmu, 0xCB, 0xEC, 0, 0, 0, 8);        // SET 5, H
    executeTest(&z80, &mmu, 0xCB, 0xED, 0, 0, 0, 8);        // SET 5, L
    executeTest(&z80, &mmu, 0xCB, 0xEE, 0, 0, 0, 16);       // SET 5, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xEF, 0, 0, 0, 8);        // SET 5, A
    executeTest(&z80, &mmu, 0xCB, 0xF0, 0, 0, 0, 8);        // SET 6, B
    executeTest(&z80, &mmu, 0xCB, 0xF1, 0, 0, 0, 8);        // SET 6, C
    executeTest(&z80, &mmu, 0xCB, 0xF2, 0, 0, 0, 8);        // SET 6, D
    executeTest(&z80, &mmu, 0xCB, 0xF3, 0, 0, 0, 8);        // SET 6, E
    executeTest(&z80, &mmu, 0xCB, 0xF4, 0, 0, 0, 8);        // SET 6, H
    executeTest(&z80, &mmu, 0xCB, 0xF5, 0, 0, 0, 8);        // SET 6, L
    executeTest(&z80, &mmu, 0xCB, 0xF6, 0, 0, 0, 16);       // SET 6, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xF7, 0, 0, 0, 8);        // SET 6, A
    executeTest(&z80, &mmu, 0xCB, 0xF8, 0, 0, 0, 8);        // SET 7, B
    executeTest(&z80, &mmu, 0xCB, 0xF9, 0, 0, 0, 8);        // SET 7, C
    executeTest(&z80, &mmu, 0xCB, 0xFA, 0, 0, 0, 8);        // SET 7, D
    executeTest(&z80, &mmu, 0xCB, 0xFB, 0, 0, 0, 8);        // SET 7, E
    executeTest(&z80, &mmu, 0xCB, 0xFC, 0, 0, 0, 8);        // SET 7, H
    executeTest(&z80, &mmu, 0xCB, 0xFD, 0, 0, 0, 8);        // SET 7, L
    executeTest(&z80, &mmu, 0xCB, 0xFE, 0, 0, 0, 16);       // SET 7, (HL)
    executeTest(&z80, &mmu, 0xCB, 0xFF, 0, 0, 0, 8);        // SET 7, A
    executeTest(&z80, &mmu, 0xCC, 0x34, 0x12, 0, 0x00, 12); // CALL Z,a16
    executeTest(&z80, &mmu, 0xCC, 0x34, 0x12, 0, 0x40, 24); // CALL Z,a16
    executeTest(&z80, &mmu, 0xCD, 0x34, 0x12, 0, 0, 24);    // CALL a16
    executeTest(&z80, &mmu, 0xCE, 0xFF, 0, 0, 0, 8);        // ADC A,d8
    executeTest(&z80, &mmu, 0xCF, 0, 0, 0, 0, 16);          // RST 08H
    executeTest(&z80, &mmu, 0xD0, 0, 0, 0, 0x01, 8);        // RET NC
    executeTest(&z80, &mmu, 0xD0, 0, 0, 0, 0x00, 20);       // RET NC
    executeTest(&z80, &mmu, 0xD1, 0, 0, 0, 0, 12);          // POP DE
    executeTest(&z80, &mmu, 0xD2, 0, 0, 0, 0x01, 12);       // JP NC,a16
    executeTest(&z80, &mmu, 0xD2, 0, 0, 0, 0x00, 16);       // JP NC,a16
    executeTest(&z80, &mmu, 0xD3, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xD4, 0, 0, 0, 0x01, 12);       // CALL NC,a16
    executeTest(&z80, &mmu, 0xD4, 0, 0, 0, 0x00, 24);       // CALL NC,a16
    executeTest(&z80, &mmu, 0xD5, 0, 0, 0, 0, 12);          // PUSH DE
    executeTest(&z80, &mmu, 0xD6, 0xFF, 0, 0, 0, 8);        // SUB d8
    executeTest(&z80, &mmu, 0xD7, 0, 0, 0, 0, 16);          // RST 10H
    executeTest(&z80, &mmu, 0xD8, 0, 0, 0, 0x00, 8);        // RET C
    executeTest(&z80, &mmu, 0xD8, 0, 0, 0, 0x01, 20);       // RET C
    executeTest(&z80, &mmu, 0xD9, 0, 0, 0, 0, 16);          // RETI
    executeTest(&z80, &mmu, 0xDA, 0, 0, 0, 0x00, 12);       // JP C,a16
    executeTest(&z80, &mmu, 0xDA, 0, 0, 0, 0x01, 16);       // JP C,a16
    executeTest(&z80, &mmu, 0xDB, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xDC, 0, 0, 0, 0x00, 12);       // CALL C,a16
    executeTest(&z80, &mmu, 0xDC, 0, 0, 0, 0x01, 24);       // CALL C,a16
    executeTest(&z80, &mmu, 0xDD, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xDE, 0x88, 0, 0, 0, 8);        // SBC A,d8
    executeTest(&z80, &mmu, 0xDF, 0, 0, 0, 0, 16);          // RST 18H
    executeTest(&z80, &mmu, 0xE0, 0x33, 0, 0, 0, 12);       // LDH (a8),A
    executeTest(&z80, &mmu, 0xE1, 0, 0, 0, 0, 12);          // POP HL
    executeTest(&z80, &mmu, 0xE2, 0, 0, 0, 0, 8);           // LD (C),A
    executeTest(&z80, &mmu, 0xE3, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xE4, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xE5, 0, 0, 0, 0, 12);          // PUSH HL
    executeTest(&z80, &mmu, 0xE6, 0xAA, 0, 0, 0, 8);        // AND d8
    executeTest(&z80, &mmu, 0xE7, 0, 0, 0, 0, 16);          // RST 20H
    executeTest(&z80, &mmu, 0xE8, -5, 0, 0, 0, 16);         // ADD SP,r8
    executeTest(&z80, &mmu, 0xE9, 0, 0, 0, 0, 4);           // JP (HL)
    executeTest(&z80, &mmu, 0xEA, 0x34, 0x12, 0, 0, 16);    // LD (a16),A
    executeTest(&z80, &mmu, 0xEB, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xEC, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xED, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xEE, 0xAA, 0, 0, 0, 8);        // XOR d8
    executeTest(&z80, &mmu, 0xEF, 0, 0, 0, 0, 16);          // RST 28H
    executeTest(&z80, &mmu, 0xF0, 0x33, 0, 0, 0, 12);       // LDH A,(a8)
    executeTest(&z80, &mmu, 0xF1, 0, 0, 0, 0, 12);          // POP AF
    executeTest(&z80, &mmu, 0xF2, 0, 0, 0, 0, 8);           // LD A,(C)
    executeTest(&z80, &mmu, 0xF3, 0, 0, 0, 0, 4);           // DI
    executeTest(&z80, &mmu, 0xF4, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xF5, 0, 0, 0, 0, 12);          // PUSH AF
    executeTest(&z80, &mmu, 0xF6, 0xAA, 0, 0, 0, 8);        // OR d8
    executeTest(&z80, &mmu, 0xF7, 0, 0, 0, 0, 16);          // RST 30H
    executeTest(&z80, &mmu, 0xF8, 0x11, 0, 0, 0, 12);       // LD HL,SP+r8
    executeTest(&z80, &mmu, 0xF9, 0, 0, 0, 0, 8);           // LD SP,HL
    executeTest(&z80, &mmu, 0xFA, 0x34, 0x12, 0, 0, 16);    // LD A,(a16)
    executeTest(&z80, &mmu, 0xFB, 0, 0, 0, 0, 4);           // EI
    executeTest(&z80, &mmu, 0xFC, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xFD, 0, 0, 0, 0, 4);           // n/a
    executeTest(&z80, &mmu, 0xFE, 0xAA, 0, 0, 0, 8);        // CP d8
    executeTest(&z80, &mmu, 0xFF, 0, 0, 0, 0, 16);          // RST 38H
    // TODO: execute other tests...

    return 0;
}
