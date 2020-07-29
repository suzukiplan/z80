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
    executeTest(&z80, &mmu, 0x00, 0, 0, 0, 4);        // NOP
    executeTest(&z80, &mmu, 0x01, 0x34, 0x12, 0, 12); // LD BC,d16
    executeTest(&z80, &mmu, 0x02, 0, 0, 0, 8);        // LD (BC),A
    executeTest(&z80, &mmu, 0x03, 0, 0, 0, 8);        // INC BC
    executeTest(&z80, &mmu, 0x04, 0, 0, 0, 4);        // INC B
    executeTest(&z80, &mmu, 0x05, 0, 0, 0, 4);        // DEC B
    executeTest(&z80, &mmu, 0x06, 123, 0, 0, 8);      // LD B,d8
    executeTest(&z80, &mmu, 0x07, 0, 0, 0, 4);        // RLCA
    executeTest(&z80, &mmu, 0x08, 0xCD, 0xAB, 0, 20); // LD (a16),SP
    executeTest(&z80, &mmu, 0x09, 0, 0, 0, 8);        // ADD HL,BC
    executeTest(&z80, &mmu, 0x0A, 0, 0, 0, 8);        // LD A,(BC)
    executeTest(&z80, &mmu, 0x0B, 0, 0, 0, 8);        // DEC BC
    executeTest(&z80, &mmu, 0x0C, 0, 0, 0, 4);        // INC C
    executeTest(&z80, &mmu, 0x0D, 0, 0, 0, 4);        // DEC C
    executeTest(&z80, &mmu, 0x0E, 98, 0, 0, 8);       // LD C,d8
    executeTest(&z80, &mmu, 0x0F, 0, 0, 0, 4);        // RRCA
    executeTest(&z80, &mmu, 0x10, 0, 0, 0, 4);        // STOP
    executeTest(&z80, &mmu, 0x11, 0x34, 0x12, 0, 12); // LD DE,d16
    executeTest(&z80, &mmu, 0x12, 0xCD, 0, 0, 8);     // LD (DE),A
    executeTest(&z80, &mmu, 0x13, 0, 0, 0, 8);        // INC DE
    executeTest(&z80, &mmu, 0x14, 0, 0, 0, 4);        // INC D
    executeTest(&z80, &mmu, 0x15, 0, 0, 0, 4);        // DEC D
    executeTest(&z80, &mmu, 0x16, 3, 0, 0, 8);        // LD D,d8
    executeTest(&z80, &mmu, 0x17, 0, 0, 0, 4);        // RLA
    executeTest(&z80, &mmu, 0x18, 0x80, 0, 0, 12);    // JR r8
    executeTest(&z80, &mmu, 0x19, 3, 0, 0, 8);        // ADD HL,DE
    executeTest(&z80, &mmu, 0x1A, 3, 0, 0, 8);        // LD A,(DE)
    executeTest(&z80, &mmu, 0x1B, 3, 0, 0, 8);        // DEC DE
    executeTest(&z80, &mmu, 0x1C, 0, 0, 0, 4);        // INC D
    executeTest(&z80, &mmu, 0x1D, 0, 0, 0, 4);        // DEC D
    executeTest(&z80, &mmu, 0x1E, 12, 0, 0, 8);       // LD E,d8
    executeTest(&z80, &mmu, 0x1F, 0, 0, 0, 4);        // RRA
    // TODO: execute other tests...

    return 0;
}
