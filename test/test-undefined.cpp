#include "z80.hpp"
#include <initializer_list>

struct Machine {
    unsigned char memory[65536] = {};
    Z80 cpu;

    Machine(std::initializer_list<unsigned char> bytes)
        : cpu([](void* arg, unsigned short addr) { return static_cast<Machine*>(arg)->memory[addr]; },
              [](void* arg, unsigned short addr, unsigned char value) { static_cast<Machine*>(arg)->memory[addr] = value; },
              [](void*, unsigned short) { return static_cast<unsigned char>(0); },
              [](void*, unsigned short, unsigned char) {}, this)
    {
        unsigned short addr = 0;
        for (auto byte : bytes) memory[addr++] = byte;
        cpu.reg.pair.A = 1;
        cpu.reg.pair.F = 0;
        cpu.reg.R = 0xFE;
        cpu.reg.SP = 0x8000;
        memory[0x8000] = 0x34;
        memory[0x8001] = 0x12;
    }
};

static int failures = 0;
static void check(bool condition, const char* name, int opcode)
{
    if (!condition) {
        printf("NG: %s (opcode %02X)\n", name, opcode);
        failures++;
    }
}

int main()
{
    // Both builds must restore IFF1 from IFF2 for the documented RETN.
    for (int iff = 0; iff < 4; iff++) {
        Machine m{0xED, 0x45};
        m.cpu.reg.IFF = static_cast<unsigned char>((iff & 1 ? 0x01 : 0) | (iff & 2 ? 0x04 : 0));
        int clocks = m.cpu.execute(1);
        check(clocks == 14 && m.cpu.reg.PC == 0x1234 && m.cpu.reg.SP == 0x8002 && !!(m.cpu.reg.IFF & 0x01) == !!(iff & 2), "RETN IFF restore", iff);
    }
#ifdef Z80_NO_EXCEPTION
    for (int opcode = 0; opcode < 256; opcode++) {
        // Every undefined ED encoding outside the alias region is a NOP.
        bool block = (opcode >= 0xA0 && opcode <= 0xA3) || (opcode >= 0xA8 && opcode <= 0xAB) || (opcode >= 0xB0 && opcode <= 0xB3) || (opcode >= 0xB8 && opcode <= 0xBB);
        if ((opcode >= 0x40 && opcode <= 0x7F && opcode != 0x77 && opcode != 0x7F) || block) continue;
        Machine m{0xED, static_cast<unsigned char>(opcode)};
        m.cpu.reg.pair.F = 0xA5;
        check(m.cpu.execute(1) == 8 && m.cpu.reg.PC == 2 && m.cpu.reg.R == 0x80 && m.cpu.reg.pair.A == 1 && m.cpu.reg.pair.F == 0xA5, "ED NOP", opcode);
    }
    const unsigned char negCases[][3] = {{0x00, 0x00, 0x42}, {0x01, 0xFF, 0xBB}, {0x7F, 0x81, 0x93}, {0x80, 0x80, 0x87}, {0xFF, 0x01, 0x13}};
    for (int opcode = 0x44; opcode <= 0x7C; opcode += 8) {
        for (const auto& probe : negCases) {
            Machine m{0xED, static_cast<unsigned char>(opcode)};
            m.cpu.reg.pair.A = probe[0];
            check(m.cpu.execute(1) == 8 && m.cpu.reg.PC == 2 && m.cpu.reg.R == 0x80 && m.cpu.reg.pair.A == probe[1] && m.cpu.reg.pair.F == probe[2], "NEG aliases", opcode);
        }
    }
    for (int opcode = 0x45; opcode <= 0x7D; opcode += 8) {
        if (opcode == 0x4D) continue; // RETI has a distinct encoding.
        for (int iff = 0; iff < 4; iff++) {
            Machine m{0xED, static_cast<unsigned char>(opcode)};
            m.cpu.reg.IFF = static_cast<unsigned char>((iff & 1 ? 0x01 : 0) | (iff & 2 ? 0x04 : 0));
            check(m.cpu.execute(1) == 14 && m.cpu.reg.PC == 0x1234 && m.cpu.reg.SP == 0x8002 && !!(m.cpu.reg.IFF & 0x01) == !!(iff & 2), "RETN aliases", opcode);
        }
    }
    const unsigned char modes[] = {0, 0, 1, 2, 0, 0, 1, 2};
    for (int i = 0; i < 8; i++) {
        Machine m{0xED, static_cast<unsigned char>(0x46 + 8 * i)};
        m.cpu.reg.interrupt = 3;
        check(m.cpu.execute(1) == 8 && (m.cpu.reg.interrupt & 3) == modes[i], "IM aliases", 0x46 + 8 * i);
    }
    for (unsigned char prefix : {0xDD, 0xFD}) {
        Machine nop{prefix, 0x00};
        check(nop.cpu.execute(1) == 8 && nop.cpu.reg.PC == 2 && nop.cpu.reg.R == 0x80, "index NOP", prefix);
        Machine load{prefix, 0x01, 0x34, 0x12};
        check(load.cpu.execute(1) == 14 && load.cpu.reg.PC == 4 && load.cpu.reg.pair.B == 0x12 && load.cpu.reg.pair.C == 0x34, "ignored prefix LD BC", prefix);
        Machine jump{prefix, 0x18, 0x02};
        check(jump.cpu.execute(1) == 16 && jump.cpu.reg.PC == 5, "ignored prefix JR", prefix);
        Machine halt{prefix, 0x76};
        check(halt.cpu.execute(1) == 8 && halt.cpu.reg.PC == 2 && (halt.cpu.reg.IFF & 0x80), "index HALT", prefix);
        Machine ed{prefix, 0xED, 0x4C};
        check(ed.cpu.execute(1) == 12 && ed.cpu.reg.PC == 3 && ed.cpu.reg.R == 0x81 && ed.cpu.reg.pair.A == 0xFF, "index ED", prefix);
        Machine index{0xDD, 0xFD, prefix, 0x21, 0x34, 0x12};
        check(index.cpu.execute(1) == 22 && index.cpu.reg.PC == 6 && index.cpu.reg.R == 0x82 && (prefix == 0xDD ? index.cpu.reg.IX : index.cpu.reg.IY) == 0x1234 && (prefix == 0xDD ? index.cpu.reg.IY : index.cpu.reg.IX) == 0, "last index prefix wins", prefix);
        Machine cb{prefix, 0xCB, 0x01, 0x06}; // RLC (IX/IY+1)
        cb.cpu.reg.IX = cb.cpu.reg.IY = 0x4000;
        cb.memory[0x4001] = 0x80;
        check(cb.cpu.execute(1) == 23 && cb.cpu.reg.PC == 4 && cb.cpu.reg.R == 0x80 && cb.memory[0x4001] == 1, "indexed CB", prefix);
    }
    Machine repeated{};
    for (int i = 0; i < 1024; i++) repeated.memory[i] = i % 2 ? 0xFD : 0xDD;
    check(repeated.cpu.execute(1) == 4100 && repeated.cpu.reg.PC == 1025 && repeated.cpu.reg.R == 0xFF,
          "repeated prefixes", 0);
    Machine wrap{};
    wrap.cpu.reg.PC = 0xFFFF;
    wrap.memory[0xFFFF] = 0xFD;
    check(wrap.cpu.execute(1) == 8 && wrap.cpu.reg.PC == 1 && wrap.cpu.reg.R == 0x80, "PC wrap", 0);
    Machine continuous{0xDD, 0xED, 0x00};
    continuous.cpu.setConsumeClockCallback([](void* arg, int) { static_cast<Machine*>(arg)->cpu.requestBreak(); });
    continuous.cpu.execute();
    check(continuous.cpu.reg.PC == 3 && continuous.cpu.reg.R == 0x81, "execute without budget", 0);
    Machine waits{0xDD, 0xED, 0x00};
    waits.cpu.wtc.fetch = 1;
    waits.cpu.wtc.fetchM = 1;
    check(waits.cpu.execute(1) == 15 && waits.cpu.reg.R == 0x81, "prefix fetch waits", 0);
    Machine cb{0xCB, 0x00};
    check(cb.cpu.execute(1) == 8 && cb.cpu.reg.R == 0x80, "CB refresh", 0);
#else
    const unsigned char unknown[][2] = {{0xED, 0x00}, {0xED, 0xA4}, {0xED, 0x4C}, {0xDD, 0x00}, {0xFD, 0x00}};
    for (const auto& bytes : unknown) {
        Machine m{bytes[0], bytes[1]};
        bool caught = false;
        try {
            m.cpu.execute(1);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        check(caught, "default unknown-opcode exception", m.memory[1]);
    }
#endif
    printf("%s: undefined opcode regression tests\n", failures ? "NG" : "OK");
    return failures ? 1 : 0;
}
