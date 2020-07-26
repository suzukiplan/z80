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

const char* statusText(unsigned char f, char* buf)
{
    int bit[8];
    bit[7] = (f & 0b10000000) ? 1 : 0;
    bit[6] = (f & 0b01000000) ? 1 : 0;
    bit[5] = (f & 0b00100000) ? 1 : 0;
    bit[4] = (f & 0b00010000) ? 1 : 0;
    bit[3] = (f & 0b00001000) ? 1 : 0;
    bit[2] = (f & 0b00000100) ? 1 : 0;
    bit[1] = (f & 0b00000010) ? 1 : 0;
    bit[0] = (f & 0b00000001) ? 1 : 0;
    sprintf(buf, "%d%d%d%d%d%d%d%d", bit[7], bit[6], bit[5], bit[4], bit[3], bit[2], bit[1], bit[0]);
    return buf;
}

const char* statusText1(unsigned char f)
{
    static char buf[32];
    return statusText(f, buf);
}

const char* statusText2(unsigned char f)
{
    static char buf[32];
    return statusText(f, buf);
}

const char* statusText3(unsigned char f)
{
    static char buf[32];
    return statusText(f, buf);
}

static int testNumber;
static int prev;
static int expect;
static FILE* file;
void executeTest(Z80* cpu, MMU* mmu, unsigned char op1, unsigned char op2, unsigned char op3, unsigned char op4, unsigned char prevF, unsigned char expectF)
{
    mmu->RAM[0] = op1;
    mmu->RAM[1] = op2;
    mmu->RAM[2] = op3;
    mmu->RAM[3] = op4;
    cpu->reg.PC = 0;
    cpu->reg.pair.F = prevF;
    prev = prevF;
    expect = expectF;
    cpu->execute(1);
    if (expectF != cpu->reg.pair.F) {
        fprintf(file, "                      SZ*H*PNC        SZ*H*PNC          SZ*H*PNC\n");
        fprintf(file, "TEST FAILED! (F: prev=%s, after=%s, excpect=%s)\n", statusText1(prevF), statusText2(cpu->reg.pair.F), statusText3(expectF));
        fprintf(stdout, "                      SZ*H*PNC        SZ*H*PNC          SZ*H*PNC\n");
        fprintf(stdout, "TEST FAILED! (F: prev=%s, after=%s, excpect=%s)\n", statusText1(prevF), statusText2(cpu->reg.pair.F), statusText3(expectF));
        exit(255);
    }
    cpu->reg.IFF = 0;
}

void check(const char* what, int e, int a)
{
    if (e != a) {
        fprintf(file, "> %s is incorrect: expected=$%X, actual=$%X\n", what, e, a);
        fprintf(stdout, "> %s is incorrect: expected=$%X, actual=$%X\n", what, e, a);
        exit(255);
    }
}

int main(int argc, char* argv[])
{
    file = fopen("test-status.txt", "w");
    fprintf(file, "===== STATUS CONDITION TEST =====\n");
    fprintf(stdout, "===== STATUS CONDITION TEST =====\n");
    MMU mmu;
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu);
    mmu.cpu = &z80;
    z80.setDebugMessage([](void* arg, const char* msg) {
        testNumber++;
        fprintf(file, "TEST#%03d: <SZYHXPNC> %s -> %s %s\n", testNumber, statusText1(prev), statusText2(expect), msg);
        fprintf(stdout, "TEST#%03d: <SZYHXPNC> %s -> %s %s\n", testNumber, statusText1(prev), statusText2(expect), msg);
    });

    executeTest(&z80, &mmu, 0b01000111, 0, 0, 0, 0, 0);                      // LD B, A
    executeTest(&z80, &mmu, 0b01000111, 0, 0, 0, 0xff, 0xff);                // LD B, A
    executeTest(&z80, &mmu, 0b01000111, 0, 0, 0, 0, 0);                      // LD B, A
    executeTest(&z80, &mmu, 0b01000111, 0, 0, 0, 0xff, 0xff);                // LD B, A
    executeTest(&z80, &mmu, 0b01010110, 0, 0, 0, 0, 0);                      // LD D, (HL)
    executeTest(&z80, &mmu, 0b01010110, 0, 0, 0, 0xff, 0xff);                // LD D, (HL)
    executeTest(&z80, &mmu, 0b01110000, 0, 0, 0, 0, 0);                      // LD (HL), B
    executeTest(&z80, &mmu, 0b01110000, 0, 0, 0, 0xff, 0xff);                // LD (HL), B
    executeTest(&z80, &mmu, 0b00001110, 0x56, 0, 0, 0, 0);                   // LD C, $56
    executeTest(&z80, &mmu, 0b00001110, 0x56, 0, 0, 0xff, 0xff);             // LD C, $56
    executeTest(&z80, &mmu, 0b00110110, 123, 0, 0, 0, 0);                    // LD (HL), 123
    executeTest(&z80, &mmu, 0b00110110, 123, 0, 0, 0xff, 0xff);              // LD (HL), 123
    executeTest(&z80, &mmu, 0b00001010, 0, 0, 0, 0, 0);                      // LD A, (BC)
    executeTest(&z80, &mmu, 0b00001010, 0, 0, 0, 0xff, 0xff);                // LD A, (BC)
    executeTest(&z80, &mmu, 0b00011010, 0, 0, 0, 0, 0);                      // LD A, (DE)
    executeTest(&z80, &mmu, 0b00011010, 0, 0, 0, 0xff, 0xff);                // LD A, (DE)
    executeTest(&z80, &mmu, 0b00111010, 0x34, 0x12, 0, 0, 0);                // LD A, ($1234)
    executeTest(&z80, &mmu, 0b00111010, 0x34, 0x12, 0, 0xff, 0xff);          // LD A, ($1234)
    executeTest(&z80, &mmu, 0b00000010, 0x34, 0x12, 0, 0, 0);                // LD (BC), A
    executeTest(&z80, &mmu, 0b00000010, 0x34, 0x12, 0, 0xff, 0xff);          // LD (BC), A
    executeTest(&z80, &mmu, 0b00010010, 0x34, 0x12, 0, 0, 0);                // LD (DE), A
    executeTest(&z80, &mmu, 0b00010010, 0x34, 0x12, 0, 0xff, 0xff);          // LD (DE), A
    executeTest(&z80, &mmu, 0b00110010, 0x78, 0x56, 0, 0, 0);                // LD ($5678), A
    executeTest(&z80, &mmu, 0b00110010, 0x78, 0x56, 0, 0xff, 0xff);          // LD ($5678), A
    executeTest(&z80, &mmu, 0b11011101, 0b01011110, 4, 0, 0, 0);             // LD E, (IX+4)
    executeTest(&z80, &mmu, 0b11011101, 0b01011110, 4, 0, 0xff, 0xff);       // LD E, (IX+4)
    executeTest(&z80, &mmu, 0b11111101, 0b01100110, 4, 0, 0, 0);             // LD H, (IY+4)
    executeTest(&z80, &mmu, 0b11111101, 0b01100110, 4, 0, 0xff, 0xff);       // LD H, (IY+4)
    executeTest(&z80, &mmu, 0b11011101, 0b01110111, 7, 0, 0, 0);             // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11011101, 0b01110111, 7, 0, 0xff, 0xff);       // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11111101, 0b01110001, 7, 0, 0, 0);             // LD (IY+7), C
    executeTest(&z80, &mmu, 0b11111101, 0b01110001, 7, 0, 0xff, 0xff);       // LD (IY+7), C
    executeTest(&z80, &mmu, 0b11011101, 0b00110110, 9, 100, 0, 0);           // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11011101, 0b00110110, 9, 100, 0xff, 0xff);     // LD (IX+7), A
    executeTest(&z80, &mmu, 0b11111101, 0b00110110, 9, 200, 0, 0);           // LD (IY+7), C
    executeTest(&z80, &mmu, 0b11111101, 0b00110110, 9, 200, 0xff, 0xff);     // LD (IY+7), C
    executeTest(&z80, &mmu, 0b00000001, 0xCD, 0xAB, 0, 0, 0);                // LD BC, $ABCD
    executeTest(&z80, &mmu, 0b00000001, 0xCD, 0xAB, 0, 0xff, 0xff);          // LD BC, $ABCD
    executeTest(&z80, &mmu, 0b00010001, 0xCD, 0xAB, 0, 0, 0);                // LD DE, $ABCD
    executeTest(&z80, &mmu, 0b00010001, 0xCD, 0xAB, 0, 0xff, 0xff);          // LD DE, $ABCD
    executeTest(&z80, &mmu, 0b00100001, 0xCD, 0xAB, 0, 0, 0);                // LD HL, $ABCD
    executeTest(&z80, &mmu, 0b00100001, 0xCD, 0xAB, 0, 0xff, 0xff);          // LD HL, $ABCD
    executeTest(&z80, &mmu, 0b00110001, 0xCD, 0xAB, 0, 0, 0);                // LD SP, $ABCD
    executeTest(&z80, &mmu, 0b00110001, 0xCD, 0xAB, 0, 0xff, 0xff);          // LD SP, $ABCD
    executeTest(&z80, &mmu, 0b11011101, 0b00100001, 0x34, 0x12, 0, 0);       // LD IX, $1234
    executeTest(&z80, &mmu, 0b11011101, 0b00100001, 0x34, 0x12, 0xff, 0xff); // LD IX, $1234
    executeTest(&z80, &mmu, 0b11111101, 0b00100001, 0x78, 0x56, 0, 0);       // LD IY, $5678
    executeTest(&z80, &mmu, 0b11111101, 0b00100001, 0x78, 0x56, 0xff, 0xff); // LD IY, $5678
    executeTest(&z80, &mmu, 0b00101010, 0x34, 0x12, 0, 0, 0);                // LD HL, ($1234)
    executeTest(&z80, &mmu, 0b00101010, 0x34, 0x12, 0, 0xff, 0xff);          // LD HL, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x4B, 0x34, 0x12, 0, 0);                   // LD BC, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x4B, 0x34, 0x12, 0xff, 0xff);             // LD BC, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x5B, 0x34, 0x12, 0, 0);                   // LD DE, ($1234)
    executeTest(&z80, &mmu, 0xED, 0x5B, 0x34, 0x12, 0xff, 0xff);             // LD DE, ($1234)
    executeTest(&z80, &mmu, 0b11101101, 0b01111011, 0x11, 0x00, 0, 0);       // LD SP, ($0011)
    executeTest(&z80, &mmu, 0b11101101, 0b01111011, 0x11, 0x00, 0xff, 0xff); // LD SP, ($0011)
    executeTest(&z80, &mmu, 0b11011101, 0b00101010, 0x02, 0x00, 0, 0);       // LD IX, ($0002)
    executeTest(&z80, &mmu, 0b11011101, 0b00101010, 0x02, 0x00, 0xff, 0xff); // LD IX, ($0002)
    executeTest(&z80, &mmu, 0b11111101, 0b00101010, 0x04, 0x00, 0, 0);       // LD IY, ($0004)
    executeTest(&z80, &mmu, 0b11111101, 0b00101010, 0x04, 0x00, 0xff, 0xff); // LD IY, ($0004)
    executeTest(&z80, &mmu, 0x22, 0x34, 0x12, 0, 0xff, 0xff);                // LD ($1234), HL
    executeTest(&z80, &mmu, 0x22, 0x34, 0x12, 0, 0, 0);                      // LD ($1234), HL
    executeTest(&z80, &mmu, 0xED, 0x43, 0x34, 0x12, 0xff, 0xff);             // LD ($1234), BC
    executeTest(&z80, &mmu, 0xED, 0x43, 0x34, 0x12, 0, 0);                   // LD ($1234), BC
    executeTest(&z80, &mmu, 0xED, 0x53, 0x34, 0x12, 0xff, 0xff);             // LD ($1234), DE
    executeTest(&z80, &mmu, 0xED, 0x53, 0x34, 0x12, 0, 0);                   // LD ($1234), DE
    executeTest(&z80, &mmu, 0b11101101, 0x73, 0x11, 0x00, 0xff, 0xff);       // LD ($0011), SP
    executeTest(&z80, &mmu, 0b11101101, 0x73, 0x11, 0x00, 0, 0);             // LD ($0011), SP
    executeTest(&z80, &mmu, 0b11011101, 0b00100010, 0x02, 0x00, 0xff, 0xff); // LD ($0002), IX
    executeTest(&z80, &mmu, 0b11011101, 0b00100010, 0x02, 0x00, 0, 0);       // LD ($0002), IX
    executeTest(&z80, &mmu, 0b11111101, 0b00100010, 0x04, 0x00, 0xff, 0xff); // LD ($0004), IY
    executeTest(&z80, &mmu, 0b11111101, 0b00100010, 0x04, 0x00, 0, 0);       // LD ($0004), IY
    executeTest(&z80, &mmu, 0b11111001, 0, 0, 0, 0, 0);                      // LD SP, HL
    executeTest(&z80, &mmu, 0b11111001, 0, 0, 0, 0xff, 0xff);                // LD SP, HL
    executeTest(&z80, &mmu, 0b11011101, 0b11111001, 0, 0, 0, 0);             // LD SP, IX
    executeTest(&z80, &mmu, 0b11011101, 0b11111001, 0, 0, 0xff, 0xff);       // LD SP, IX
    executeTest(&z80, &mmu, 0b11111101, 0b11111001, 0, 0, 0, 0);             // LD SP, IY
    executeTest(&z80, &mmu, 0b11111101, 0b11111001, 0, 0, 0xff, 0xff);       // LD SP, IY
    z80.reg.SP = 0xFFFF;                                                     // setup register for test
    executeTest(&z80, &mmu, 0xC5, 0, 0, 0, 0, 0);                            // PUSH BC
    executeTest(&z80, &mmu, 0xC5, 0, 0, 0, 0xff, 0xff);                      // PUSH BC
    executeTest(&z80, &mmu, 0xD5, 0, 0, 0, 0, 0);                            // PUSH DE
    executeTest(&z80, &mmu, 0xD5, 0, 0, 0, 0xff, 0xff);                      // PUSH DE
    executeTest(&z80, &mmu, 0xE5, 0, 0, 0, 0, 0);                            // PUSH HL
    executeTest(&z80, &mmu, 0xE5, 0, 0, 0, 0xff, 0xff);                      // PUSH HL
    executeTest(&z80, &mmu, 0xF5, 0, 0, 0, 12, 12);                          // PUSH AF
    executeTest(&z80, &mmu, 0xF5, 0, 0, 0, 34, 34);                          // PUSH AF
    executeTest(&z80, &mmu, 0xDD, 0xE5, 0, 0, 0, 0);                         // PUSH IX
    executeTest(&z80, &mmu, 0xDD, 0xE5, 0, 0, 0xff, 0xff);                   // PUSH IX
    executeTest(&z80, &mmu, 0xFD, 0xE5, 0, 0, 0, 0);                         // PUSH IX
    executeTest(&z80, &mmu, 0xFD, 0xE5, 0, 0, 0xff, 0xff);                   // PUSH IX
    executeTest(&z80, &mmu, 0xFD, 0xE1, 0, 0, 0, 0);                         // POP IX
    executeTest(&z80, &mmu, 0xFD, 0xE1, 0, 0, 0xff, 0xff);                   // POP IX
    executeTest(&z80, &mmu, 0xDD, 0xE1, 0, 0, 0, 0);                         // POP IX
    executeTest(&z80, &mmu, 0xDD, 0xE1, 0, 0, 0xff, 0xff);                   // POP IX
    executeTest(&z80, &mmu, 0xF1, 0, 0, 0, 0, 34);                           // POP AF
    executeTest(&z80, &mmu, 0xF1, 0, 0, 0, 0xff, 12);                        // POP AF
    executeTest(&z80, &mmu, 0xE1, 0, 0, 0, 0, 0);                            // POP HL
    executeTest(&z80, &mmu, 0xE1, 0, 0, 0, 0xff, 0xff);                      // POP HL
    executeTest(&z80, &mmu, 0xD1, 0, 0, 0, 0, 0);                            // POP DE
    executeTest(&z80, &mmu, 0xD1, 0, 0, 0, 0xff, 0xff);                      // POP DE
    executeTest(&z80, &mmu, 0xC1, 0, 0, 0, 0, 0);                            // POP BC
    executeTest(&z80, &mmu, 0xC1, 0, 0, 0, 0xff, 0xff);                      // POP BC
    executeTest(&z80, &mmu, 0xEB, 0, 0, 0, 0, 0);                            // EX DE, HL
    executeTest(&z80, &mmu, 0xEB, 0, 0, 0, 0xff, 0xff);                      // EX DE, HL
    z80.reg.back.F = 99;                                                     // setup register for test
    executeTest(&z80, &mmu, 0x08, 0, 0, 0, 88, 99);                          // EX AF, AF'
    executeTest(&z80, &mmu, 0x08, 0, 0, 0, 77, 88);                          // EX AF, AF'
    executeTest(&z80, &mmu, 0xD9, 0, 0, 0, 0, 0);                            // EXX
    executeTest(&z80, &mmu, 0xD9, 0, 0, 0, 0xff, 0xff);                      // EXX
    executeTest(&z80, &mmu, 0xE3, 0, 0, 0, 0, 0);                            // EX (SP), HL
    executeTest(&z80, &mmu, 0xE3, 0, 0, 0, 0xff, 0xff);                      // EX (SP), HL
    executeTest(&z80, &mmu, 0xDD, 0xE3, 0, 0, 0, 0);                         // EX (SP), IX
    executeTest(&z80, &mmu, 0xDD, 0xE3, 0, 0, 0xff, 0xff);                   // EX (SP), IX
    executeTest(&z80, &mmu, 0xFD, 0xE3, 0, 0, 0, 0);                         // EX (SP), IY
    executeTest(&z80, &mmu, 0xFD, 0xE3, 0, 0, 0xff, 0xff);                   // EX (SP), IY
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xA0, 0, 0, 0xFF, 0b11000101);             // LDI
    executeTest(&z80, &mmu, 0xED, 0xA0, 0, 0, 0xFF, 0b11000001);             // LDI
    executeTest(&z80, &mmu, 0xED, 0xA0, 0, 0, 0x00, 0b00000100);             // LDI
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xB0, 0, 0, 0xFF, 0b11000101);             // LDIR
    executeTest(&z80, &mmu, 0xED, 0xB0, 0, 0, 0xFF, 0b11000001);             // LDIR
    executeTest(&z80, &mmu, 0xED, 0xB0, 0, 0, 0x00, 0b00000100);             // LDIR
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xA8, 0, 0, 0xFF, 0b11000101);             // LDD
    executeTest(&z80, &mmu, 0xED, 0xA8, 0, 0, 0xFF, 0b11000001);             // LDD
    executeTest(&z80, &mmu, 0xED, 0xA8, 0, 0, 0x00, 0b00000100);             // LDD
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xB8, 0, 0, 0xFF, 0b11000101);             // LDDR
    executeTest(&z80, &mmu, 0xED, 0xB8, 0, 0, 0xFF, 0b11000001);             // LDDR
    executeTest(&z80, &mmu, 0xED, 0xB8, 0, 0, 0x00, 0b00000100);             // LDDR
    z80.reg.pair.A = 0x11;                                                   // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    mmu.RAM[0x100] = 0x22;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xED, 0xA1, 0, 0, 0x00, 0b10111010);             // CPI
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xA1, 0, 0, 0x00, 0b10111110);             // CPI
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xA1, 0, 0, 0xFF, 0b10111111);             // CPI
    z80.reg.pair.A = 0x11;                                                   // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    mmu.RAM[0x100] = 0x22;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xED, 0xB1, 0, 0, 0x00, 0b10111010);             // CPIR
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xB1, 0, 0, 0x00, 0b10111110);             // CPIR
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xB1, 0, 0, 0xFF, 0b10111111);             // CPIR
    z80.reg.pair.A = 0x11;                                                   // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    mmu.RAM[0x100] = 0x22;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xED, 0xA9, 0, 0, 0x00, 0b10111010);             // CPD
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xA9, 0, 0, 0x00, 0b10111110);             // CPD
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xA9, 0, 0, 0xFF, 0b10111111);             // CPD
    z80.reg.pair.A = 0x11;                                                   // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    z80.reg.pair.C = 2;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    mmu.RAM[0x100] = 0x22;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xED, 0xB9, 0, 0, 0x00, 0b10111010);             // CPDR
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xB9, 0, 0, 0x00, 0b10111110);             // CPDR
    z80.reg.pair.C = 1;                                                      // setup register for test
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xED, 0xB9, 0, 0, 0xFF, 0b10111111);             // CPDR
    z80.reg.pair.A = 0;                                                      // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0x00, 0b01000000);                // ADD A, B
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    z80.reg.pair.B = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0x00, 0b00010101);                // ADD A, B
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    z80.reg.pair.B = 0x80;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0x00, 0b10000000);                // ADD A, B
    z80.reg.pair.A = 0;                                                      // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0xFF, 0b01000000);                // ADD A, B
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    z80.reg.pair.B = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0xFF, 0b00010101);                // ADD A, B
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    z80.reg.pair.B = 0x80;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x80, 0, 0, 0, 0xFF, 0b10000000);                // ADD A, B
    z80.reg.pair.B = 0xFF;                                                   // setup register for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xC6, 0x00, 0, 0, 0x00, 0b01000000);             // ADD A, n
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xC6, 0x88, 0, 0, 0x00, 0b00010101);             // ADD A, n
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xC6, 0x80, 0, 0, 0x00, 0b10000000);             // ADD A, n
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xC6, 0, 0, 0, 0xFF, 0b01000000);                // ADD A, n
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xC6, 0x88, 0, 0, 0xFF, 0b00010101);             // ADD A, n
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xC6, 0x80, 0, 0, 0xFF, 0b10000000);             // ADD A, n
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    mmu.RAM[0x100] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0x00, 0b01000000);                // ADD A, (HL)
    mmu.RAM[0x100] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0x00, 0b00010101);                // ADD A, (HL)
    mmu.RAM[0x100] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0x00, 0b10000000);                // ADD A, (HL)
    mmu.RAM[0x100] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0xFF, 0b01000000);                // ADD A, (HL)
    mmu.RAM[0x100] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0xFF, 0b00010101);                // ADD A, (HL)
    mmu.RAM[0x100] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x86, 0, 0, 0, 0xFF, 0b10000000);                // ADD A, (HL)
    z80.reg.IX = 0x200;                                                      // setup register for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 0x00, 0b01000000);             // ADD A, (IX+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 0x00, 0b00010101);             // ADD A, (IX+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 0x00, 0b10000000);             // ADD A, (IX+d)
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 0xFF, 0b01000000);             // ADD A, (IX+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 0xFF, 0b00010101);             // ADD A, (IX+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x86, 5, 0, 0xFF, 0b10000000);             // ADD A, (IX+d)
    z80.reg.IY = 0x200;                                                      // setup register for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 0x00, 0b01000000);             // ADD A, (IY+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 0x00, 0b00010101);             // ADD A, (IY+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 0x00, 0b10000000);             // ADD A, (IY+d)
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 0xFF, 0b01000000);             // ADD A, (IY+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 0xFF, 0b00010101);             // ADD A, (IY+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x86, 5, 0, 0xFF, 0b10000000);             // ADD A, (IY+d)
    z80.reg.pair.A = 0;                                                      // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0x00, 0b01000000);                // ADC A, B
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    z80.reg.pair.B = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0x00, 0b00010101);                // ADC A, B
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    z80.reg.pair.B = 0x80;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0x00, 0b10000000);                // ADC A, B
    z80.reg.pair.A = 0;                                                      // setup register for test
    z80.reg.pair.B = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0xFF, 0b00000000);                // ADC A, B
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    z80.reg.pair.B = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0xFF, 0b00010101);                // ADC A, B
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    z80.reg.pair.B = 0x80;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x88, 0, 0, 0, 0xFF, 0b10000000);                // ADC A, B
    z80.reg.pair.B = 0xFF;                                                   // setup register for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xCE, 0x00, 0, 0, 0x00, 0b01000000);             // ADC A, n
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xCE, 0x88, 0, 0, 0x00, 0b00010101);             // ADC A, n
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xCE, 0x80, 0, 0, 0x00, 0b10000000);             // ADC A, n
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xCE, 0, 0, 0, 0xFF, 0b00000000);                // ADC A, n
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xCE, 0x88, 0, 0, 0xFF, 0b00010101);             // ADC A, n
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xCE, 0x80, 0, 0, 0xFF, 0b10000000);             // ADC A, n
    z80.reg.pair.H = 0x01;                                                   // setup register for test
    z80.reg.pair.L = 0x00;                                                   // setup register for test
    mmu.RAM[0x100] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0x00, 0b01000000);                // ADC A, (HL)
    mmu.RAM[0x100] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0x00, 0b00010101);                // ADC A, (HL)
    mmu.RAM[0x100] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0x00, 0b10000000);                // ADC A, (HL)
    mmu.RAM[0x100] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0xFF, 0b00000000);                // ADC A, (HL)
    mmu.RAM[0x100] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0xFF, 0b00010101);                // ADC A, (HL)
    mmu.RAM[0x100] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0x8E, 0, 0, 0, 0xFF, 0b10000000);                // ADC A, (HL)
    z80.reg.IX = 0x200;                                                      // setup register for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 0x00, 0b01000000);             // ADC A, (IX+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 0x00, 0b00010101);             // ADC A, (IX+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 0x00, 0b10000000);             // ADC A, (IX+d)
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 0xFF, 0b00000000);             // ADC A, (IX+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 0xFF, 0b00010101);             // ADC A, (IX+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x8E, 5, 0, 0xFF, 0b10000000);             // ADC A, (IX+d)
    z80.reg.IY = 0x200;                                                      // setup register for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 0x00, 0b01000000);             // ADC A, (IY+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 0x00, 0b00010101);             // ADC A, (IY+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 0x00, 0b10000000);             // ADC A, (IY+d)
    mmu.RAM[0x205] = 0x00;                                                   // setup RAM for test
    z80.reg.pair.A = 0;                                                      // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 0xFF, 0b00000000);             // ADC A, (IY+d)
    mmu.RAM[0x205] = 0x88;                                                   // setup RAM for test
    z80.reg.pair.A = 0x88;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 0xFF, 0b00010101);             // ADC A, (IY+d)
    mmu.RAM[0x205] = 0x80;                                                   // setup RAM for test
    z80.reg.pair.A = 0x00;                                                   // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x8E, 5, 0, 0xFF, 0b10000000);             // ADC A, (IY+d)

    // test DAA (increment)
    executeTest(&z80, &mmu, 0x3E, 0x99, 0, 0, 0b00000000, 0b00000000); // LD A, $99
    executeTest(&z80, &mmu, 0x3C, 0, 0, 0, 0b00000000, 0b10001000);    // INC A
    check("A", 0x9A, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b10001000, 0b01011101);    // DAA
    check("A", 0x00, z80.reg.pair.A);                                  // check register result

    // test DAA (addition / not carry & not half)
    executeTest(&z80, &mmu, 0x3E, 0x12, 0, 0, 0b00000000, 0b00000000); // LD A, $12
    executeTest(&z80, &mmu, 0xC6, 0x34, 0, 0, 0b00000000, 0b00000000); // ADD A, $34
    check("A", 0x46, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b00000000, 0b00000000);    // DAA
    check("A", 0x46, z80.reg.pair.A);                                  // check register result

    // test DAA (addition / not carry & half)
    executeTest(&z80, &mmu, 0x3E, 0x14, 0, 0, 0b00000000, 0b00000000); // LD A, $14
    executeTest(&z80, &mmu, 0xC6, 0x39, 0, 0, 0b00000000, 0b00001000); // ADD A, $34
    check("A", 0x4D, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b00001000, 0b00011100);    // DAA
    check("A", 0x53, z80.reg.pair.A);                                  // check register result

    // test DAA (addition / carry & not half)
    executeTest(&z80, &mmu, 0x3E, 0x72, 0, 0, 0b00000000, 0b00000000); // LD A, $72
    executeTest(&z80, &mmu, 0xC6, 0x77, 0, 0, 0b00000000, 0b10101100); // ADD A, $77
    check("A", 0xE9, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b10101100, 0b00101001);    // DAA
    check("A", 0x49, z80.reg.pair.A);                                  // check register result

    // test DAA (addition / carry & half - case 1)
    executeTest(&z80, &mmu, 0x3E, 0x67, 0, 0, 0b00000000, 0b00000000); // LD A, $67
    executeTest(&z80, &mmu, 0xC6, 0x55, 0, 0, 0b00000000, 0b10101100); // ADD A, $55
    check("A", 0xBC, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b10101100, 0b00111101);    // DAA
    check("A", 0x22, z80.reg.pair.A);                                  // check register result

    // test DAA (addition / carry & half - case 2)
    executeTest(&z80, &mmu, 0x3E, 0x67, 0, 0, 0b00000000, 0b00000000); // LD A, $67
    executeTest(&z80, &mmu, 0xC6, 0x33, 0, 0, 0b00000000, 0b10001100); // ADD A, $33
    check("A", 0x9A, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b10001100, 0b01011101);    // DAA
    check("A", 0x00, z80.reg.pair.A);                                  // check register result

    // test DAA (decrement)
    executeTest(&z80, &mmu, 0x3E, 0, 0, 0, 0b00000000, 0b00000000); // LD A, $00
    executeTest(&z80, &mmu, 0xD6, 1, 0, 0, 0b00000000, 0b10111011); // SUB A, $01
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b10111011, 0b10111111); // DAA
    check("A", 0x99, z80.reg.pair.A);                               // check register result

    // test DAA (substract / not carry & not half)
    executeTest(&z80, &mmu, 0x3E, 0x55, 0, 0, 0b00000000, 0b00000000); // LD A, $55
    executeTest(&z80, &mmu, 0xD6, 0x23, 0, 0, 0b00000000, 0b00100010); // SUB A, $23
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b00100010, 0b00100010);    // DAA
    check("A", 0x32, z80.reg.pair.A);                                  // check register result

    // test DAA (substract / not carry & half)
    executeTest(&z80, &mmu, 0x3E, 0x35, 0, 0, 0b00000000, 0b00000000); // LD A, $35
    executeTest(&z80, &mmu, 0xD6, 0x06, 0, 0, 0b00000000, 0b00111010); // SUB A, $06
    check("A", 0x2F, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b00111010, 0b00111010);    // DAA
    check("A", 0x29, z80.reg.pair.A);                                  // check register result

    // test DAA (substract / carry & not half)
    executeTest(&z80, &mmu, 0x3E, 0x35, 0, 0, 0b00000000, 0b00000000); // LD A, $35
    executeTest(&z80, &mmu, 0xD6, 0x40, 0, 0, 0b00000000, 0b10100011); // SUB A, $40
    check("A", 0xF5, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b10100011, 0b10100111);    // DAA
    check("A", 0x95, z80.reg.pair.A);                                  // check register result

    // test DAA (substract / carry & half)
    executeTest(&z80, &mmu, 0x3E, 0x35, 0, 0, 0b00000000, 0b00000000); // LD A, $35
    executeTest(&z80, &mmu, 0xD6, 0x56, 0, 0, 0b00000000, 0b10011011); // SUB A, $56
    check("A", 0xDF, z80.reg.pair.A);                                  // check register result
    executeTest(&z80, &mmu, 0x27, 0, 0, 0, 0b10011011, 0b00011011);    // DAA
    check("A", 0x79, z80.reg.pair.A);                                  // check register result

    // test CPL
    z80.reg.pair.A = 0b10110100;                                    // setup register for test
    executeTest(&z80, &mmu, 0x2F, 0, 0, 0, 0b00000000, 0b00011010); // CPL
    check("A", 0b01001011, z80.reg.pair.A);                         // check register result
    z80.reg.pair.A = 0b10110100;                                    // setup register for test
    executeTest(&z80, &mmu, 0x2F, 0, 0, 0, 0b11111111, 0b11011111); // CPL
    check("A", 0b01001011, z80.reg.pair.A);                         // check register result

    puts("tests INC/DEC IXH/IXL");
    z80.reg.IX = 0xFFFF;                                               // setup register for test
    z80.reg.IY = 0x0000;                                               // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x24, 0, 0, 0b00101010, 0b01010000); // INC IXH
    check("IX", 0x00FF, z80.reg.IX);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x25, 0, 0, 0b00000000, 0b10111010); // DEC IXH
    check("IX", 0xFFFF, z80.reg.IX);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x2C, 0, 0, 0b00101010, 0b01010000); // INC IXL
    check("IX", 0xFF00, z80.reg.IX);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x2D, 0, 0, 0b00000000, 0b10111010); // DEC IXL
    check("IX", 0xFFFF, z80.reg.IX);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result

    puts("tests INC/DEC IYH/IYL");
    z80.reg.IX = 0x0000;                                               // setup register for test
    z80.reg.IY = 0xFFFF;                                               // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x24, 0, 0, 0b00101010, 0b01010000); // INC IYH
    check("IY", 0x00FF, z80.reg.IY);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x25, 0, 0, 0b00000000, 0b10111010); // DEC IYH
    check("IY", 0xFFFF, z80.reg.IY);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x2C, 0, 0, 0b00101010, 0b01010000); // INC IYL
    check("IY", 0xFF00, z80.reg.IY);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x2D, 0, 0, 0b00000000, 0b10111010); // DEC IYL
    check("IY", 0xFFFF, z80.reg.IY);                                   // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result

    puts("tests LD IXH/IXL/IYH/IYL, n");
    z80.reg.IX = 0x1234;                                                  // setup register for test
    z80.reg.IY = 0x4321;                                                  // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x26, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, $00
    executeTest(&z80, &mmu, 0xDD, 0x26, 0x00, 0, 0b11111111, 0b11111111); // LD IXH, $00
    check("IX", 0x0034, z80.reg.IX);                                      // check register result
    check("PC", 3, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x2E, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, $00
    executeTest(&z80, &mmu, 0xDD, 0x2E, 0x00, 0, 0b11111111, 0b11111111); // LD IXL, $00
    check("IX", 0x0000, z80.reg.IX);                                      // check register result
    check("PC", 3, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x26, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, $00
    executeTest(&z80, &mmu, 0xFD, 0x26, 0x00, 0, 0b11111111, 0b11111111); // LD IYH, $00
    check("IY", 0x0021, z80.reg.IY);                                      // check register result
    check("PC", 3, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x2E, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, $00
    executeTest(&z80, &mmu, 0xFD, 0x2E, 0x00, 0, 0b11111111, 0b11111111); // LD IYL, $00
    check("IY", 0x0000, z80.reg.IY);                                      // check register result
    check("PC", 3, z80.reg.PC);                                           // check register result

    puts("tests LD IXH, A/B/C/D/E/IXH/IXL");
    z80.reg.IX = 0x1234;                                                  // setup register for test
    z80.reg.IY = 0x4321;                                                  // setup register for test
    z80.reg.pair.A = 0x0A;                                                // setup register for test
    z80.reg.pair.B = 0x0B;                                                // setup register for test
    z80.reg.pair.C = 0x0C;                                                // setup register for test
    z80.reg.pair.D = 0x0D;                                                // setup register for test
    z80.reg.pair.E = 0x0E;                                                // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x67, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, A
    check("IX", 0x0A34, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x60, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, B
    check("IX", 0x0B34, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x61, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, C
    check("IX", 0x0C34, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x62, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, D
    check("IX", 0x0D34, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x63, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, E
    check("IX", 0x0E34, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x64, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, IXH
    check("IX", 0x0E34, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x65, 0x00, 0, 0b00000000, 0b00000000); // LD IXH, IXL
    check("IX", 0x03434, z80.reg.IX);                                     // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    puts("tests LD IXL, A/B/C/D/E/IXH/IXL");
    executeTest(&z80, &mmu, 0xDD, 0x6F, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, A
    check("IX", 0x340A, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x68, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, B
    check("IX", 0x340B, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x69, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, C
    check("IX", 0x340C, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x6A, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, D
    check("IX", 0x340D, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x6B, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, E
    check("IX", 0x340E, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x6C, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, IXH
    check("IX", 0x3434, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xDD, 0x6D, 0x00, 0, 0b00000000, 0b00000000); // LD IXL, IXL
    check("IX", 0x3434, z80.reg.IX);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    puts("tests LD IYH, A/B/C/D/E/IYH/IYL");
    executeTest(&z80, &mmu, 0xFD, 0x67, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, A
    check("IY", 0x0A21, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x60, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, B
    check("IY", 0x0B21, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x61, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, C
    check("IY", 0x0C21, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x62, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, D
    check("IY", 0x0D21, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x63, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, E
    check("IY", 0x0E21, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x64, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, IYH
    check("IY", 0x0E21, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x65, 0x00, 0, 0b00000000, 0b00000000); // LD IYH, IYL
    check("IY", 0x2121, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    puts("tests LD IYL, A/B/C/D/E/IYH/IYL");
    executeTest(&z80, &mmu, 0xFD, 0x6F, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, A
    check("IY", 0x210A, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x68, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, B
    check("IY", 0x210B, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x69, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, C
    check("IY", 0x210C, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x6A, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, D
    check("IY", 0x210D, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x6B, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, E
    check("IY", 0x210E, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x6C, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, IYH
    check("IY", 0x2121, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result
    executeTest(&z80, &mmu, 0xFD, 0x6D, 0x00, 0, 0b00000000, 0b00000000); // LD IYL, IYL
    check("IY", 0x2121, z80.reg.IY);                                      // check register result
    check("PC", 2, z80.reg.PC);                                           // check register result

    puts("tests ADD A, IXH");
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IX = 0x0000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 0x00, 0b01000000); // ADD A, IXH
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IX = 0x8800;                                         // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 0x00, 0b00010101); // ADD A, IXH
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IX = 0x8000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 0x00, 0b10000000); // ADD A, IXH
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IX = 0x0000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 0xFF, 0b01000000); // ADD A, IXH
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IX = 0x8800;                                         // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 0xFF, 0b00010101); // ADD A, IXH
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IX = 0x8000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x84, 0, 0, 0xFF, 0b10000000); // ADD A, IXH

    puts("tests ADD A, IXL");
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IX = 0x0000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 0x00, 0b01000000); // ADD A, IXL
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IX = 0x88;                                           // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 0x00, 0b00010101); // ADD A, IXL
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IX = 0x80;                                           // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 0x00, 0b10000000); // ADD A, IXL
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IX = 0x00;                                           // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 0xFF, 0b01000000); // ADD A, IXL
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IX = 0x88;                                           // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 0xFF, 0b00010101); // ADD A, IXL
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IX = 0x80;                                           // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x85, 0, 0, 0xFF, 0b10000000); // ADD A, IXL

    puts("tests ADD A, IYH");
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IY = 0x0000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 0x00, 0b01000000); // ADD A, IYH
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IY = 0x8800;                                         // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 0x00, 0b00010101); // ADD A, IYH
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IY = 0x8000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 0x00, 0b10000000); // ADD A, IYH
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IY = 0x0000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 0xFF, 0b01000000); // ADD A, IYH
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IY = 0x8800;                                         // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 0xFF, 0b00010101); // ADD A, IYH
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IY = 0x8000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x84, 0, 0, 0xFF, 0b10000000); // ADD A, IYH

    puts("tests ADD A, IYL");
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IY = 0x0000;                                         // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 0x00, 0b01000000); // ADD A, IYL
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IY = 0x88;                                           // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 0x00, 0b00010101); // ADD A, IYL
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IY = 0x80;                                           // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 0x00, 0b10000000); // ADD A, IYL
    z80.reg.pair.A = 0;                                          // setup register for test
    z80.reg.IY = 0x00;                                           // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 0xFF, 0b01000000); // ADD A, IYL
    z80.reg.pair.A = 0x88;                                       // setup register for test
    z80.reg.IY = 0x88;                                           // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 0xFF, 0b00010101); // ADD A, IYL
    z80.reg.pair.A = 0x00;                                       // setup register for test
    z80.reg.IY = 0x80;                                           // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x85, 0, 0, 0xFF, 0b10000000); // ADD A, IYL

    puts("tests LD A/B/C/D/E, IXH");
    z80.reg.IX = 0x1234;                                               // setup register for test
    z80.reg.IY = 0x4321;                                               // setup register for test
    z80.reg.pair.A = 0x0A;                                             // setup register for test
    z80.reg.pair.B = 0x0B;                                             // setup register for test
    z80.reg.pair.C = 0x0C;                                             // setup register for test
    z80.reg.pair.D = 0x0D;                                             // setup register for test
    z80.reg.pair.E = 0x0E;                                             // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x7C, 0, 0, 0b00000000, 0b00000000); // LD A, IXH
    check("A", 0x12, z80.reg.pair.A);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x44, 0, 0, 0b00000000, 0b00000000); // LD B, IXH
    check("B", 0x12, z80.reg.pair.B);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x4C, 0, 0, 0b00000000, 0b00000000); // LD C, IXH
    check("C", 0x12, z80.reg.pair.C);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x54, 0, 0, 0b00000000, 0b00000000); // LD D, IXH
    check("D", 0x12, z80.reg.pair.D);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x5C, 0, 0, 0b00000000, 0b00000000); // LD E, IXH
    check("E", 0x12, z80.reg.pair.E);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result

    puts("tests LD A/B/C/D/E, IXL");
    z80.reg.IX = 0x1234;                                               // setup register for test
    z80.reg.IY = 0x4321;                                               // setup register for test
    z80.reg.pair.A = 0x0A;                                             // setup register for test
    z80.reg.pair.B = 0x0B;                                             // setup register for test
    z80.reg.pair.C = 0x0C;                                             // setup register for test
    z80.reg.pair.D = 0x0D;                                             // setup register for test
    z80.reg.pair.E = 0x0E;                                             // setup register for test
    executeTest(&z80, &mmu, 0xDD, 0x7D, 0, 0, 0b00000000, 0b00000000); // LD A, IXL
    check("A", 0x34, z80.reg.pair.A);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x45, 0, 0, 0b00000000, 0b00000000); // LD B, IXL
    check("B", 0x34, z80.reg.pair.B);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x4D, 0, 0, 0b00000000, 0b00000000); // LD C, IXL
    check("C", 0x34, z80.reg.pair.C);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x55, 0, 0, 0b00000000, 0b00000000); // LD D, IXL
    check("D", 0x34, z80.reg.pair.D);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xDD, 0x5D, 0, 0, 0b00000000, 0b00000000); // LD E, IXL
    check("E", 0x34, z80.reg.pair.E);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result

    puts("tests LD A/B/C/D/E, IYH");
    z80.reg.IX = 0x1234;                                               // setup register for test
    z80.reg.IY = 0x4321;                                               // setup register for test
    z80.reg.pair.A = 0x0A;                                             // setup register for test
    z80.reg.pair.B = 0x0B;                                             // setup register for test
    z80.reg.pair.C = 0x0C;                                             // setup register for test
    z80.reg.pair.D = 0x0D;                                             // setup register for test
    z80.reg.pair.E = 0x0E;                                             // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x7C, 0, 0, 0b00000000, 0b00000000); // LD A, IYH
    check("A", 0x43, z80.reg.pair.A);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x44, 0, 0, 0b00000000, 0b00000000); // LD B, IYH
    check("B", 0x43, z80.reg.pair.B);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x4C, 0, 0, 0b00000000, 0b00000000); // LD C, IYH
    check("C", 0x43, z80.reg.pair.C);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x54, 0, 0, 0b00000000, 0b00000000); // LD D, IYH
    check("D", 0x43, z80.reg.pair.D);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x5C, 0, 0, 0b00000000, 0b00000000); // LD E, IYH
    check("E", 0x43, z80.reg.pair.E);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result

    puts("tests LD A/B/C/D/E, IYL");
    z80.reg.IX = 0x1234;                                               // setup register for test
    z80.reg.IY = 0x4321;                                               // setup register for test
    z80.reg.pair.A = 0x0A;                                             // setup register for test
    z80.reg.pair.B = 0x0B;                                             // setup register for test
    z80.reg.pair.C = 0x0C;                                             // setup register for test
    z80.reg.pair.D = 0x0D;                                             // setup register for test
    z80.reg.pair.E = 0x0E;                                             // setup register for test
    executeTest(&z80, &mmu, 0xFD, 0x7D, 0, 0, 0b00000000, 0b00000000); // LD A, IYL
    check("A", 0x21, z80.reg.pair.A);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x45, 0, 0, 0b00000000, 0b00000000); // LD B, IYL
    check("B", 0x21, z80.reg.pair.B);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x4D, 0, 0, 0b00000000, 0b00000000); // LD C, IYL
    check("C", 0x21, z80.reg.pair.C);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x55, 0, 0, 0b00000000, 0b00000000); // LD D, IYL
    check("D", 0x21, z80.reg.pair.D);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result
    executeTest(&z80, &mmu, 0xFD, 0x5D, 0, 0, 0b00000000, 0b00000000); // LD E, IYL
    check("E", 0x21, z80.reg.pair.E);                                  // check register result
    check("PC", 2, z80.reg.PC);                                        // check register result

    //         7 6 5 4 3 2   1 0
    // status: S Z * H * P/V N C

    /*
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
*/
    return 0;
}
