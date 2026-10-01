// FlagProfile test: Zilog (default) and Upd9002 (NEC
// uPD9002 Z80 emulation mode, rules R1-R13). F is loaded with PUSH BC / POP AF
// and captured with PUSH AF / POP DE right after the instruction under test.
#include "z80.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <iomanip>
#include <iostream>

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
    {"CPL", 0x0000, 0x0000, 0x0000, {0x2F}, 1, 0xFF, 0x3A, 0x00},                  // R8
    {"CPL (F=$D7)", 0x00D7, 0x0000, 0x0000, {0x2F}, 1, 0xFF, 0xFF, 0xD7},          // R8
    {"SCF (H,N set)", 0x0012, 0x0000, 0x0000, {0x37}, 1, 0x00, 0x01, 0x13},        // R9
    {"CCF (C set)", 0x0013, 0x0000, 0x0000, {0x3F}, 1, 0x00, 0x10, 0x12},          // R10
    {"CCF (C clear)", 0x0012, 0x0000, 0x0000, {0x3F}, 1, 0x00, 0x01, 0x13},        // R10
    {"DAA A=$00", 0x0000, 0x0000, 0x0000, {0x27}, 1, 0x00, 0x44, 0x40},            // R11
    {"DAA A=$7A", 0x7A00, 0x0000, 0x0000, {0x27}, 1, 0x80, 0x90, 0x94},            // R11
};
// DAA cases whose A result also differs between the profiles (R11: with H=1
// the high step needs A > $9F instead of A > $99).
struct DaaHighProbe {
    Probe probe;            // probe.a and probe.fZilog are the Zilog results
    unsigned char aUpd9002; // expected A, Upd9002
};

static const DaaHighProbe daaHighProbes[] = {
    {{"DAA A=$9A H", 0x9A10, 0x0000, 0x0000, {0x27}, 1, 0x00, 0x55, 0x90}, 0xA0},
    {{"DAA A=$9A H N", 0x9A12, 0x0000, 0x0000, {0x27}, 1, 0x34, 0x23, 0x92}, 0x94},
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

using Memory = std::array<unsigned char, 65536>;

static void setupMemory(Z80& z80, Memory& memory)
{
    // Adapt the typed memory operations to the API's unused context argument.
    z80.setupCallback(
        std::bind([&memory](unsigned short addr) { return memory[addr]; }, std::placeholders::_2),
        std::bind([&memory](unsigned short addr, unsigned char value) { memory[addr] = value; },
                  std::placeholders::_2, std::placeholders::_3),
        std::bind([]() { return static_cast<unsigned char>(0xFF); }),
        std::bind([]() {
            // No I/O devices are attached to this memory-only test machine.
        }),
        nullptr);
}

static int testInstructionLimit()
{
    Memory memory = {{0xC3, 0x00, 0x00}}; // JP $0000
    Z80 z80;
    setupMemory(z80, memory);
    bool stopped = !runUntil(z80, 3);
    std::cout << (stopped ? "OK" : "NG") << ": instruction limit rejects infinite loop\n";
    memory[0] = 0x00; // NOPs reach the endpoint on the final allowed instruction.
    z80.initialize();
    bool reached = runUntil(z80, (unsigned short)instructionLimit);
    std::cout << (reached ? "OK" : "NG") << ": instruction limit accepts exact boundary\n";
    return (stopped ? 0 : 1) + (reached ? 0 : 1);
}

static const char* profileName(Z80::FlagProfile profile)
{
    return profile == Z80::FlagProfile::Upd9002 ? "Upd9002" : "Zilog";
}

static void printFlags(unsigned char a, unsigned char f)
{
    std::cout << std::hex << std::uppercase << std::setfill('0')
              << " A=$" << std::setw(2) << static_cast<unsigned int>(a)
              << " F=$" << std::setw(2) << static_cast<unsigned int>(f)
              << std::dec << std::setfill(' ');
}

static int checkState(const char* name, const Z80& z80, Z80::FlagProfile profile,
                      unsigned char f, unsigned char f2)
{
    bool ok = z80.getFlagProfile() == profile && z80.reg.pair.F == f && z80.reg.back.F == f2;
    std::cout << (ok ? "OK" : "NG") << ": " << name
              << " profile=" << profileName(z80.getFlagProfile())
              << " F=" << static_cast<unsigned int>(z80.reg.pair.F)
              << " F'=" << static_cast<unsigned int>(z80.reg.back.F)
              << " (expected profile=" << profileName(profile)
              << " F=" << static_cast<unsigned int>(f)
              << " F'=" << static_cast<unsigned int>(f2) << ")\n";
    return ok ? 0 : 1;
}

static int testProfileLifecycle()
{
    const auto zilog = Z80::FlagProfile::Zilog;
    const auto upd9002 = Z80::FlagProfile::Upd9002;
    Z80 z80;
    int failures = checkState("default initialization", z80, zilog, 0xFF, 0x00);
    z80.reg.pair.F = 0xFF;
    z80.reg.back.F = 0xAB;
    z80.setFlagProfile(upd9002);
    failures += checkState("switch to Upd9002", z80, upd9002, 0xD7, 0x83);
    Z80 other;
    failures += checkState("independent instance", other, zilog, 0xFF, 0x00);
    z80.setFlagProfile(zilog);
    failures += checkState("switch back to Zilog", z80, zilog, 0xD7, 0x83);
    z80.reg.pair.F = 0xFF;
    z80.reg.back.F = 0xAB;
    z80.setFlagProfile(zilog);
    failures += checkState("Zilog preserves bits 5/3", z80, zilog, 0xFF, 0xAB);
    z80.initialize();
    failures += checkState("initialize Zilog", z80, zilog, 0xFF, 0x00);
    z80.setFlagProfile(upd9002);
    z80.reg.back.F = 0xFF;
    z80.initialize();
    failures += checkState("initialize keeps Upd9002", z80, upd9002, 0xD7, 0x00);
    return failures;
}

static int runProbeExpecting(const Probe& probe, Z80::FlagProfile profile, unsigned char expectedA,
                             unsigned char expectedF)
{
    Memory memory = {};
    // Nonzero effective address bits 5/3 also exercise the Zilog XY flags.
    memory[0x2800] = 0x80;
    unsigned char* p = memory.data();
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
    unsigned short end = (unsigned short)(p - memory.data());
    Z80 z80;
    setupMemory(z80, memory);
    z80.setFlagProfile(profile);
    z80.reg.SP = 0xF000;
    z80.reg.IX = 0x2801;
    z80.reg.IY = 0x27FF;
    const bool indexedBit = probe.bodySize == 4 && probe.body[1] == 0xCB
        && (probe.body[0] == 0xDD || probe.body[0] == 0xFD)
        && (probe.body[3] & 0xC0) == 0x40;
    // BIT (HL) uses the existing WZ; indexed BIT must establish its own WZ.
    z80.reg.WZ = indexedBit ? 0x1000 : 0x2800;
    if (!runUntil(z80, end)) {
        std::cout << "NG: " << profileName(profile) << " " << probe.name
                  << " exceeded " << instructionLimit << " instructions: PC=" << z80.reg.PC
                  << " expected=" << end << "\n";
        return 1;
    }
    bool ok = z80.reg.pair.A == expectedA && z80.reg.pair.E == expectedF;
    if (indexedBit && z80.reg.WZ != 0x2800) {
        std::cout << "NG: " << profileName(profile) << " " << probe.name
                  << " WZ=" << z80.reg.WZ << " (expected " << 0x2800 << ")\n";
        ok = false;
    }
    std::cout << (ok ? "OK" : "NG") << ": " << profileName(profile) << " " << probe.name;
    printFlags(z80.reg.pair.A, z80.reg.pair.E);
    std::cout << " (expected";
    printFlags(expectedA, expectedF);
    std::cout << ")\n";
    return ok ? 0 : 1;
}

static int runProbe(const Probe& probe, Z80::FlagProfile profile)
{
    const bool upd9002 = profile == Z80::FlagProfile::Upd9002;
    return runProbeExpecting(probe, profile, probe.a, upd9002 ? probe.fUpd9002 : probe.fZilog);
}

static int runDaaHighProbe(const DaaHighProbe& daa, Z80::FlagProfile profile)
{
    const bool upd9002 = profile == Z80::FlagProfile::Upd9002;
    return runProbeExpecting(daa.probe, profile, upd9002 ? daa.aUpd9002 : daa.probe.a,
                             upd9002 ? daa.probe.fUpd9002 : daa.probe.fZilog);
}

// R12/R13: DD/FD CB d xx with a register operand (low three bits != 6).
struct IndexedRegisterProbe {
    const char* name;
    unsigned char prefix;
    unsigned char op;
    unsigned char zilogB, zilogMemory, zilogF;
    unsigned char upd9002B, upd9002Memory, upd9002F;
};

static const IndexedRegisterProbe indexedRegisterProbes[] = {
    // B=$0B and indexed memory=$81 before the instruction; A=F=$00
    {"RES 0,(IX+2),B", 0xDD, 0x80, 0x80, 0x80, 0x00, 0x0A, 0x81, 0x00},
    {"SET 7,(IX+2),B", 0xDD, 0xF8, 0x81, 0x81, 0x00, 0x8B, 0x81, 0x00},
    {"BIT 0,(IX+2) [B]", 0xDD, 0x40, 0x0B, 0x81, 0x10, 0x0B, 0x81, 0x00}, // Zilog tests memory, Upd9002 tests B
    // Bit 1 differs between B and memory, exposing an incorrect operand source.
    {"BIT 1,(IX+2) [B]", 0xDD, 0x48, 0x0B, 0x81, 0x54, 0x0B, 0x81, 0x00},
    {"BIT 1,(IY+2) [B]", 0xFD, 0x48, 0x0B, 0x81, 0x54, 0x0B, 0x81, 0x00},
    {"BIT 2,(IX+2) [B]", 0xDD, 0x50, 0x0B, 0x81, 0x54, 0x0B, 0x81, 0x44},
};

static int runIndexedRegisterProbe(const IndexedRegisterProbe& probe, Z80::FlagProfile profile)
{
    Memory memory = {};
    const std::array<unsigned char, 8> code = {{0x06, 0x0B, probe.prefix, 0xCB, 0x02, probe.op, 0xF5, 0xD1}}; // LD B,$0B; op; PUSH AF; POP DE
    std::copy(code.begin(), code.end(), memory.begin());
    memory[0x5002] = 0x81;
    Z80 z80;
    setupMemory(z80, memory);
    z80.setFlagProfile(profile);
    z80.reg.SP = 0xF000;
    z80.reg.IX = 0x5000;
    z80.reg.IY = 0x5000;
    z80.reg.pair.A = 0x00;
    z80.reg.pair.F = 0x00;
    if (!runUntil(z80, (unsigned short)code.size())) {
        std::cout << "NG: " << profileName(profile) << " " << probe.name << " did not finish\n";
        return 1;
    }
    const bool upd9002 = profile == Z80::FlagProfile::Upd9002;
    const unsigned char b = upd9002 ? probe.upd9002B : probe.zilogB;
    const unsigned char mem = upd9002 ? probe.upd9002Memory : probe.zilogMemory;
    const unsigned char f = upd9002 ? probe.upd9002F : probe.zilogF;
    const bool ok = z80.reg.pair.B == b && memory[0x5002] == mem && z80.reg.pair.E == f;
    std::cout << (ok ? "OK" : "NG") << ": " << profileName(profile) << " " << probe.name << std::hex
              << std::uppercase << std::setfill('0') << " B=$" << std::setw(2)
              << static_cast<unsigned int>(z80.reg.pair.B) << " indexed memory=$" << std::setw(2)
              << static_cast<unsigned int>(memory[0x5002]) << " F=$" << std::setw(2)
              << static_cast<unsigned int>(z80.reg.pair.E) << " (expected B=$" << std::setw(2)
              << static_cast<unsigned int>(b) << " indexed memory=$" << std::setw(2) << static_cast<unsigned int>(mem)
              << " F=$" << std::setw(2) << static_cast<unsigned int>(f) << ")" << std::dec
              << std::setfill(' ') << "\n";
    return ok ? 0 : 1;
}

int main()
{
    int failures = testProfileLifecycle() + testInstructionLimit();
    for (int profile = 0; profile < 2; profile++) {
        for (const Probe& probe : probes) {
            failures += runProbe(probe, profile ? Z80::FlagProfile::Upd9002 : Z80::FlagProfile::Zilog);
        }
    }
    for (int profile = 0; profile < 2; profile++) {
        for (const DaaHighProbe& daa : daaHighProbes) {
            failures += runDaaHighProbe(daa, profile ? Z80::FlagProfile::Upd9002 : Z80::FlagProfile::Zilog);
        }
    }
    for (int profile = 0; profile < 2; profile++) {
        for (const IndexedRegisterProbe& probe : indexedRegisterProbes) {
            failures += runIndexedRegisterProbe(probe, profile ? Z80::FlagProfile::Upd9002 : Z80::FlagProfile::Zilog);
        }
    }
    return failures ? -1 : 0;
}
