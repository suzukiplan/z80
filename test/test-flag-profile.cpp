// FlagProfile test: Zilog (default) and Upd9002 (NEC
// uPD9002 Z80 emulation mode, rules R1-R7). F is loaded with PUSH BC / POP AF
// and captured with PUSH AF / POP DE right after the instruction under test.
#include "z80.hpp"

struct Probe {
    const char* name;
    unsigned short af;
    unsigned short bc;
    unsigned short hl;
    unsigned char body[8];
    int bodySize;
    unsigned char a;        // expected A (both profiles)
    unsigned char fZilog;   // expected F, Zilog
    unsigned char fUpd9002; // expected F, Upd9002
};

static const Probe probes[] = {
    {"XOR A", 0x5A00, 0x0000, 0x0000, {0xAF}, 1, 0x00, 0x44, 0x44},
    {"AND $0F", 0xFF00, 0x0000, 0x0000, {0xE6, 0x0F}, 2, 0x0F, 0x1C, 0x04},       // R1, R2
    {"BIT 0,A", 0x0100, 0x0000, 0x0000, {0xCB, 0x47}, 2, 0x01, 0x10, 0x00},       // R3
    {"BIT 0,(HL)", 0x0013, 0x0000, 0x2800, {0xCB, 0x46}, 2, 0x00, 0x7D, 0x45}, // R3
    {"BIT 7,(HL)", 0x0013, 0x0000, 0x2800, {0xCB, 0x7E}, 2, 0x00, 0xB9, 0x81}, // R3
    {"BIT 0,(IX+d)", 0x0013, 0x0000, 0x2800, {0xDD, 0xCB, 0xFF, 0x46}, 4, 0x00, 0x7D, 0x45}, // R3
    {"BIT 7,(IX+d)", 0x0013, 0x0000, 0x2800, {0xDD, 0xCB, 0xFF, 0x7E}, 4, 0x00, 0xB9, 0x81}, // R3
    {"BIT 0,(IY+d)", 0x0013, 0x0000, 0x2800, {0xFD, 0xCB, 0x01, 0x46}, 4, 0x00, 0x7D, 0x45}, // R3
    {"BIT 7,(IY+d)", 0x0013, 0x0000, 0x2800, {0xFD, 0xCB, 0x01, 0x7E}, 4, 0x00, 0xB9, 0x81}, // R3
    {"RLCA", 0x80D7, 0x0000, 0x0000, {0x07}, 1, 0x01, 0xC5, 0xD7},                // R4
    {"LDI (BC=1)", 0x00D7, 0x0001, 0x4000, {0xED, 0xA0}, 2, 0x00, 0xC1, 0xD3},    // R5
    {"LDI (BC=2)", 0x00D7, 0x0002, 0x4000, {0xED, 0xA0}, 2, 0x00, 0xC5, 0xD7},    // R5
    {"ADD HL,BC (H set)", 0x0000, 0x0001, 0x0FFF, {0x09}, 1, 0x00, 0x10, 0x00},   // R6
    {"ADD HL,BC (H kept)", 0x0010, 0x0001, 0x0001, {0x09}, 1, 0x00, 0x00, 0x10},  // R6
    {"ADC HL,BC", 0x0000, 0x0001, 0x000F, {0xED, 0x4A}, 2, 0x00, 0x00, 0x10},     // R7
    {"SBC HL,BC", 0x0000, 0x0001, 0x0010, {0xED, 0x42}, 2, 0x00, 0x02, 0x12},     // R7
    {"ADC HL,BC (C=1)", 0x0001, 0x0000, 0x000F, {0xED, 0x4A}, 2, 0x00, 0x00, 0x10}, // R7
    {"SBC HL,BC (C=1)", 0x0001, 0x0000, 0x0010, {0xED, 0x42}, 2, 0x00, 0x02, 0x12}, // R7
    {"ADC HL,FFFF (C=1)", 0x0001, 0xFFFF, 0x0000, {0xED, 0x4A}, 2, 0x00, 0x51, 0x51},
    {"SBC HL,FFFF (C=1)", 0x0001, 0xFFFF, 0x0000, {0xED, 0x42}, 2, 0x00, 0x53, 0x53},
    {"POP AF ($FFFF)", 0x0000, 0xFFFF, 0x0000, {0xC5, 0xF1}, 2, 0xFF, 0xFF, 0xD7}, // R1
    {"EX AF,AF' x2", 0x0000, 0xFFFF, 0x0000, {0xC5, 0xF1, 0x08, 0x08}, 4, 0xFF, 0xFF, 0xD7}, // R1
};

// Each probe needs fewer than 16 instructions; leave room for setup changes.
static const int instructionLimit = 32;

static bool runUntil(Z80& z80, unsigned short end)
{
    for (int instructions = 0; instructions < instructionLimit; instructions++) {
        if (z80.reg.PC == end) return true;
        z80.execute(1);
    }
    return z80.reg.PC == end;
}

static int testInstructionLimit()
{
    unsigned char memory[65536] = {0xC3, 0x00, 0x00}; // JP $0000
    Z80 z80([&memory](void* arg, unsigned short addr) { return memory[addr]; },
            [&memory](void* arg, unsigned short addr, unsigned char value) { memory[addr] = value; },
            [](void* arg, unsigned short port) { return (unsigned char)0xFF; },
            [](void* arg, unsigned short port, unsigned char value) {},
            nullptr);
    bool stopped = !runUntil(z80, 3);
    printf("%s: instruction limit rejects infinite loop\n", stopped ? "OK" : "NG");
    memory[0] = 0x00; // NOPs reach the endpoint on the final allowed instruction.
    z80.initialize();
    bool reached = runUntil(z80, (unsigned short)instructionLimit);
    printf("%s: instruction limit accepts exact boundary\n", reached ? "OK" : "NG");
    return (stopped ? 0 : 1) + (reached ? 0 : 1);
}

static int checkState(const char* name, const Z80& z80, Z80::FlagProfile profile,
                      unsigned char f, unsigned char f2)
{
    bool ok = z80.getFlagProfile() == profile && z80.reg.pair.F == f && z80.reg.back.F == f2;
    printf("%s: %s profile=%d F=$%02X F'=$%02X (expected profile=%d F=$%02X F'=$%02X)\n",
           ok ? "OK" : "NG", name, (int)z80.getFlagProfile(), z80.reg.pair.F, z80.reg.back.F,
           (int)profile, f, f2);
    return ok ? 0 : 1;
}

static int testProfileLifecycle()
{
    Z80 z80;
    int failures = checkState("default initialization", z80, Z80::FlagProfile::Zilog, 0xFF, 0x00);
    z80.reg.pair.F = 0xFF;
    z80.reg.back.F = 0xAB;
    z80.setFlagProfile(Z80::FlagProfile::Upd9002);
    failures += checkState("switch to Upd9002", z80, Z80::FlagProfile::Upd9002, 0xD7, 0x83);
    Z80 other;
    failures += checkState("independent instance", other, Z80::FlagProfile::Zilog, 0xFF, 0x00);
    z80.setFlagProfile(Z80::FlagProfile::Zilog);
    failures += checkState("switch back to Zilog", z80, Z80::FlagProfile::Zilog, 0xD7, 0x83);
    z80.reg.pair.F = 0xFF;
    z80.reg.back.F = 0xAB;
    z80.setFlagProfile(Z80::FlagProfile::Zilog);
    failures += checkState("Zilog preserves bits 5/3", z80, Z80::FlagProfile::Zilog, 0xFF, 0xAB);
    z80.initialize();
    failures += checkState("initialize Zilog", z80, Z80::FlagProfile::Zilog, 0xFF, 0x00);
    z80.setFlagProfile(Z80::FlagProfile::Upd9002);
    z80.reg.back.F = 0xFF;
    z80.initialize();
    failures += checkState("initialize keeps Upd9002", z80, Z80::FlagProfile::Upd9002, 0xD7, 0x00);
    return failures;
}

int main()
{
    unsigned char memory[65536];
    int failures = testProfileLifecycle() + testInstructionLimit();
    for (int profile = 0; profile < 2; profile++) {
        for (const Probe& probe : probes) {
            memset(memory, 0, sizeof(memory));
            // Nonzero effective address bits 5/3 also exercise the Zilog XY flags.
            memory[0x2800] = 0x80;
            unsigned char* p = memory;
            *p++ = 0x01; // LD BC,af
            *p++ = (unsigned char)probe.af;
            *p++ = (unsigned char)(probe.af >> 8);
            *p++ = 0xC5; // PUSH BC
            *p++ = 0xF1; // POP AF
            *p++ = 0x01; // LD BC,bc
            *p++ = (unsigned char)probe.bc;
            *p++ = (unsigned char)(probe.bc >> 8);
            *p++ = 0x21; // LD HL,hl
            *p++ = (unsigned char)probe.hl;
            *p++ = (unsigned char)(probe.hl >> 8);
            *p++ = 0x11; // LD DE,$5000
            *p++ = 0x00;
            *p++ = 0x50;
            memcpy(p, probe.body, (size_t)probe.bodySize);
            p += probe.bodySize;
            *p++ = 0xF5; // PUSH AF
            *p++ = 0xD1; // POP DE
            unsigned short end = (unsigned short)(p - memory);
            Z80 z80([&memory](void* arg, unsigned short addr) { return memory[addr]; },
                    [&memory](void* arg, unsigned short addr, unsigned char value) { memory[addr] = value; },
                    [](void* arg, unsigned short port) { return (unsigned char)0xFF; },
                    [](void* arg, unsigned short port, unsigned char value) {},
                    &z80);
            z80.setFlagProfile(profile ? Z80::FlagProfile::Upd9002 : Z80::FlagProfile::Zilog);
            z80.reg.SP = 0xF000;
            z80.reg.IX = 0x2801;
            z80.reg.IY = 0x27FF;
            z80.reg.WZ = 0x2800;
            if (!runUntil(z80, end)) {
                printf("NG: %s %s exceeded %d instructions: PC=$%04X expected=$%04X\n",
                       profile ? "Upd9002" : "Zilog", probe.name, instructionLimit, z80.reg.PC, end);
                failures++;
                continue;
            }
            unsigned char expectedF = profile ? probe.fUpd9002 : probe.fZilog;
            bool ok = z80.reg.pair.A == probe.a && z80.reg.pair.E == expectedF;
            printf("%s: %-8s %-20s A=$%02X F=$%02X (expected A=$%02X F=$%02X)\n", ok ? "OK" : "NG",
                   profile ? "Upd9002" : "Zilog", probe.name, z80.reg.pair.A, z80.reg.pair.E, probe.a, expectedF);
            if (!ok) failures++;
        }
    }
    return failures ? -1 : 0;
}
