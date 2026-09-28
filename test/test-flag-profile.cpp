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
    {"RLCA", 0x80D7, 0x0000, 0x0000, {0x07}, 1, 0x01, 0xC5, 0xD7},                // R4
    {"LDI (BC=1)", 0x00D7, 0x0001, 0x4000, {0xED, 0xA0}, 2, 0x00, 0xC1, 0xD3},    // R5
    {"LDI (BC=2)", 0x00D7, 0x0002, 0x4000, {0xED, 0xA0}, 2, 0x00, 0xC5, 0xD7},    // R5
    {"ADD HL,BC (H set)", 0x0000, 0x0001, 0x0FFF, {0x09}, 1, 0x00, 0x10, 0x00},   // R6
    {"ADD HL,BC (H kept)", 0x0010, 0x0001, 0x0001, {0x09}, 1, 0x00, 0x00, 0x10},  // R6
    {"ADC HL,BC", 0x0000, 0x0001, 0x000F, {0xED, 0x4A}, 2, 0x00, 0x00, 0x10},     // R7
    {"SBC HL,BC", 0x0000, 0x0001, 0x0010, {0xED, 0x42}, 2, 0x00, 0x02, 0x12},     // R7
    {"POP AF ($FFFF)", 0x0000, 0xFFFF, 0x0000, {0xC5, 0xF1}, 2, 0xFF, 0xFF, 0xD7}, // R1
    {"EX AF,AF' x2", 0x0000, 0xFFFF, 0x0000, {0xC5, 0xF1, 0x08, 0x08}, 4, 0xFF, 0xFF, 0xD7}, // R1
};

int main()
{
    unsigned char memory[65536];
    int failures = 0;
    for (int profile = 0; profile < 2; profile++) {
        for (const Probe& probe : probes) {
            memset(memory, 0, sizeof(memory));
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
            while (z80.reg.PC != end) z80.execute(1);
            unsigned char expectedF = profile ? probe.fUpd9002 : probe.fZilog;
            bool ok = z80.reg.pair.A == probe.a && z80.reg.pair.E == expectedF;
            printf("%s: %-8s %-20s A=$%02X F=$%02X (expected A=$%02X F=$%02X)\n", ok ? "OK" : "NG",
                   profile ? "Upd9002" : "Zilog", probe.name, z80.reg.pair.A, z80.reg.pair.E, probe.a, expectedF);
            if (!ok) failures++;
        }
    }
    return failures ? -1 : 0;
}
