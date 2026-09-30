// Undefined and redundant opcodes with -DZ80_NO_EXCEPTION.
// Every ED xx, DD xx and FD xx must execute without calling a null handler,
// and follow the Zilog Z80: undefined ED opcodes are two-byte NOPs, the
// NEG/RETN/IM duplicates behave as the base instruction, and a DD/FD prefix
// before an opcode that does not use HL is ignored. (Without
// Z80_NO_EXCEPTION such opcodes throw; see test-unknown.)
#ifndef Z80_NO_EXCEPTION
#define Z80_NO_EXCEPTION
#endif
#include "z80.hpp"

static unsigned char memory[65536];
static int failures = 0;

static void check(bool ok, const char* what)
{
    printf("%s: %s\n", ok ? "OK" : "NG", what);
    if (!ok) failures++;
}

struct Machine {
    Z80 z80;
    int clocks;
    Machine() : z80([](void*, unsigned short a) { return memory[a]; },
                    [](void*, unsigned short a, unsigned char v) { memory[a] = v; },
                    [](void*, unsigned short) { return (unsigned char)0x5A; },
                    [](void*, unsigned short, unsigned char) {},
                    this),
                clocks(0)
    {
        z80.setConsumeClockCallback([](void* arg, int c) { ((Machine*)arg)->clocks += c; });
    }
};

// Load code at 0100h, set registers, run one instruction.
static Machine* run(const unsigned char* code, int size)
{
    static Machine* m = nullptr;
    delete m;
    memset(memory, 0, sizeof(memory));
    memcpy(&memory[0x0100], code, (size_t)size);
    memory[0x8000] = 0x34; // return address for RETN duplicates
    memory[0x8001] = 0x12;
    m = new Machine();
    m->z80.reg.PC = 0x0100;
    m->z80.reg.SP = 0x8000;
    m->z80.reg.pair.A = 0x12;
    m->z80.reg.pair.F = 0x00;
    m->z80.reg.pair.B = 0x34;
    m->z80.reg.pair.C = 0x56;
    m->z80.reg.pair.H = 0x40;
    m->z80.reg.pair.L = 0x00;
    m->z80.reg.IX = 0x5000;
    m->z80.reg.IY = 0x6000;
    m->clocks = 0;
    m->z80.execute(1);
    return m;
}

int main()
{
    // 1. no table entry is missing
    for (int prefix = 0; prefix < 3; prefix++) {
        const unsigned char p = prefix == 0 ? 0xED : prefix == 1 ? 0xDD : 0xFD;
        for (int op = 0; op < 256; op++) {
            unsigned char code[4] = {p, (unsigned char)op, 0x00, 0x00};
            run(code, 4);
        }
    }
    check(true, "all ED/DD/FD opcodes executed");

    // 2. undefined ED opcodes are two-byte NOPs taking 8 clocks
    const unsigned char undefinedEd[] = {0x00, 0x3F, 0x77, 0x7F, 0x80, 0xA4, 0xBF, 0xC0, 0xFF};
    for (unsigned char op : undefinedEd) {
        unsigned char code[2] = {0xED, op};
        Machine* m = run(code, 2);
        char what[64];
        snprintf(what, sizeof(what), "ED %02X is a NOP", op);
        check(m->z80.reg.PC == 0x0102 && m->z80.reg.pair.A == 0x12 && m->z80.reg.pair.F == 0x00 &&
                  m->z80.reg.pair.B == 0x34 && m->z80.reg.SP == 0x8000 && m->clocks == 8,
              what);
    }

    // 3. duplicates
    const unsigned char negs[] = {0x44, 0x4C, 0x54, 0x5C, 0x64, 0x6C, 0x74, 0x7C};
    for (unsigned char op : negs) {
        unsigned char code[2] = {0xED, op};
        Machine* m = run(code, 2);
        char what[64];
        snprintf(what, sizeof(what), "ED %02X is NEG", op);
        check(m->z80.reg.pair.A == 0xEE && m->z80.reg.PC == 0x0102, what);
    }
    const unsigned char retns[] = {0x45, 0x55, 0x5D, 0x65, 0x6D, 0x75, 0x7D};
    for (unsigned char op : retns) {
        unsigned char code[2] = {0xED, op};
        Machine* m = run(code, 2);
        char what[64];
        snprintf(what, sizeof(what), "ED %02X is RETN", op);
        check(m->z80.reg.PC == 0x1234 && m->z80.reg.SP == 0x8002, what);
    }
    const struct {
        unsigned char op;
        int mode;
    } ims[] = {{0x46, 0}, {0x4E, 0}, {0x66, 0}, {0x6E, 0}, {0x56, 1}, {0x76, 1}, {0x5E, 2}, {0x7E, 2}};
    for (const auto& im : ims) {
        unsigned char code[2] = {0xED, im.op};
        Machine* m = run(code, 2);
        char what[64];
        snprintf(what, sizeof(what), "ED %02X is IM %d", im.op, im.mode);
        check((m->z80.reg.interrupt & 3) == im.mode && m->z80.reg.PC == 0x0102, what);
    }

    // 4. ignored DD/FD prefixes
    {
        unsigned char code[] = {0xDD, 0x00};
        Machine* m = run(code, 2);
        check(m->z80.reg.PC == 0x0102 && m->clocks == 8, "DD 00 is NOP with 8 clocks");
    }
    {
        unsigned char code[] = {0xFD, 0x3C};
        Machine* m = run(code, 2);
        check(m->z80.reg.pair.A == 0x13 && m->z80.reg.PC == 0x0102, "FD 3C is INC A");
    }
    {
        unsigned char code[] = {0xDD, 0x06, 0x99};
        Machine* m = run(code, 3);
        check(m->z80.reg.pair.B == 0x99 && m->z80.reg.PC == 0x0103, "DD 06 nn is LD B,nn");
    }
    {
        unsigned char code[] = {0xDD, 0xFD, 0x21, 0x78, 0x56};
        Machine* m = run(code, 5);
        check(m->z80.reg.IY == 0x5678 && m->z80.reg.IX == 0x5000 && m->z80.reg.PC == 0x0105,
              "DD FD 21 nn nn is LD IY,nn (last prefix wins)");
    }
    {
        unsigned char code[] = {0xFD, 0xED, 0x44};
        Machine* m = run(code, 3);
        check(m->z80.reg.pair.A == 0xEE && m->z80.reg.PC == 0x0103, "FD ED 44 is NEG");
    }
    return failures ? -1 : 0;
}
