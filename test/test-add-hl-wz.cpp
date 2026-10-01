// ADD HL,rr must leave the original HL + 1 in WZ, which BIT (HL) observes.
#include "z80.hpp"
#include <array>
#include <functional>

using Memory = std::array<unsigned char, 65536>;

// C++11 callable adapters accept the API context without relying on void pointers
// or generic lambdas; the memory is supplied directly by the test fixture.
struct MemoryAccess {
    Memory& memory;

    template <typename Context>
    unsigned char operator()(Context, unsigned short addr) const
    {
        return memory[addr];
    }

    template <typename Context>
    void operator()(Context, unsigned short addr, unsigned char value) const
    {
        memory[addr] = value;
    }
};

static void setupMemory(Z80& z80, Memory& memory)
{
    z80.setupCallback(
        MemoryAccess{memory},
        MemoryAccess{memory},
        std::bind([]() { return static_cast<unsigned char>(0xFF); }),
        std::bind([]() {
            // No I/O devices are attached to this memory-only test machine.
        }),
        nullptr);
}

struct Probe {
    unsigned short hl;
    unsigned short wz;
};

static int runProbe(const Probe& probe, int profile, int rp, const char* name)
{
    Memory memory = {};
    memory[0x8000] = (unsigned char)(0x09 + rp * 0x10);
    memory[0x8001] = 0xCB; // BIT 0,(HL)
    memory[0x8002] = 0x46;
    Z80 z80;
    setupMemory(z80, memory);
    if (profile) z80.setFlagProfile(Z80::FlagProfile::Upd9002);
    z80.reg.PC = 0x8000;
    z80.reg.pair.H = (unsigned char)(probe.hl >> 8);
    z80.reg.pair.L = (unsigned char)probe.hl;
    z80.reg.pair.B = 0x12;
    z80.reg.pair.D = 0x12;
    z80.reg.pair.C = 0x34;
    z80.reg.pair.E = 0x34;
    z80.reg.SP = 0x1234;
    z80.reg.WZ = 0xABCD;
    z80.reg.pair.F = 0;
    unsigned int sum = (unsigned int)probe.hl + (rp == 2 ? probe.hl : 0x1234u);
    z80.execute(1);
    auto actualHL = (unsigned short)((z80.reg.pair.H << 8) | z80.reg.pair.L);
    bool addOK = actualHL == (unsigned short)sum && z80.reg.WZ == probe.wz;
    unsigned short actualWZ = z80.reg.WZ;
    // Zero memory sets Z/PV; XY must come from WZ, not the new HL.
    z80.execute(1);
    auto expectedF = (unsigned char)((profile ? 0x44 : 0x54)
        | (profile ? 0 : ((probe.wz >> 8) & 0x28)) | (sum > 0xFFFFu ? 1 : 0));
    if (!addOK || z80.reg.pair.F != expectedF || z80.reg.WZ != probe.wz) {
        printf("NG: profile=%d ADD HL,%s HL=$%04X -> HL=$%04X WZ=$%04X; BIT F=$%02X"
               " (expected HL=$%04X WZ=$%04X F=$%02X)\n",
               profile, name, probe.hl, actualHL, actualWZ, z80.reg.pair.F,
               (unsigned short)sum, probe.wz, expectedF);
        return 1;
    }
    return 0;
}

int main()
{
    const std::array<Probe, 5> probes = {{{0x27FF, 0x2800}, {0x2800, 0x2801}, {0x07FF, 0x0800},
                                        {0xFFFF, 0x0000}, {0x0000, 0x0001}}};
    const std::array<const char*, 4> names = {{"BC", "DE", "HL", "SP"}};
    int failures = 0;
    int cases = 0;
    for (int profile = 0; profile < 2; profile++) {
        for (int rp = 0; rp < 4; rp++) {
            for (const auto& probe : probes) {
                failures += runProbe(probe, profile, rp, names[(size_t)rp]);
                cases++;
            }
        }
    }
    printf("%s: %d of %d cases failed\n", failures ? "NG" : "OK", failures, cases);
    return failures ? 1 : 0;
}
