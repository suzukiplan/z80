#include "z80.hpp"
#include <vector>

struct Machine {
    unsigned char memory[65536] = {};
    Z80 cpu;
    std::vector<unsigned char> expected;
    std::vector<unsigned short> callbacks;
    bool valid = true;

    Machine(const std::vector<unsigned char>& bytes, unsigned short start)
        : cpu([](void* arg, unsigned short addr) { return static_cast<Machine*>(arg)->memory[addr]; },
              [](void* arg, unsigned short addr, unsigned char value) { static_cast<Machine*>(arg)->memory[addr] = value; },
              [](void*, unsigned short) { return static_cast<unsigned char>(0); },
              [](void*, unsigned short, unsigned char) {}, this),
          expected(bytes)
    {
        for (size_t i = 0; i < bytes.size(); i++) memory[static_cast<unsigned short>(start + i)] = bytes[i];
        cpu.reg.PC = start;
        cpu.reg.SP = 0x8000;
        cpu.reg.R = 0xFE;
        cpu.reg.pair.A = 1;
        cpu.reg.IX = cpu.reg.IY = 0x4000;
        memory[0x4001] = 0x80;
    }

    static void notify(void* arg, unsigned char* bytes, int length)
    {
        auto m = static_cast<Machine*>(arg);
        m->valid = m->valid && length == static_cast<int>(m->expected.size());
        if (length == static_cast<int>(m->expected.size()))
            m->valid = m->valid && std::vector<unsigned char>(bytes, bytes + length) == m->expected;
        m->callbacks.push_back(m->cpu.reg.PC);
    }
};

static int failures = 0;
static void run(const std::vector<unsigned char>& bytes, int key, const std::vector<unsigned short>& phases, unsigned short start = 0)
{
    Machine observed(bytes, start), baseline(bytes, start);
    // A following instruction must never leak into the reported opcode.
    observed.memory[static_cast<unsigned short>(start + bytes.size())] = 0x3E;
    baseline.memory[static_cast<unsigned short>(start + bytes.size())] = 0x3E;
    observed.cpu.addBreakOperand(key, Machine::notify);
    observed.cpu.addBreakOperand(key, Machine::notify);
    std::vector<unsigned short> expectedPhases;
    for (auto pc : phases) {
        expectedPhases.push_back(pc);
        expectedPhases.push_back(pc);
    }
    int clocks = observed.cpu.execute(1);
    int expectedClocks = baseline.cpu.execute(1);
    const auto& a = observed.cpu.reg;
    const auto& b = baseline.cpu.reg;
    bool state = a.WZ == b.WZ && a.I == b.I && a.execEI == b.execEI && a.interruptVector == b.interruptVector && a.PC == b.PC && a.SP == b.SP && a.IX == b.IX && a.IY == b.IY && a.R == b.R && a.IFF == b.IFF && a.interrupt == b.interrupt &&
                 !memcmp(&a.pair, &b.pair, sizeof(a.pair)) && !memcmp(observed.memory, baseline.memory, sizeof(observed.memory));
    if (!observed.valid || observed.callbacks != expectedPhases || clocks != expectedClocks || !state) {
        printf("NG: operand %X at %04X (%zu bytes)\n", key, start, bytes.size());
        failures++;
    }
    if (bytes == std::vector<unsigned char>({0xDD, 0xFD, 0x21, 0x34, 0x12}) && (a.IY != 0x1234 || a.IX != 0x4000 || clocks != 18 || a.R != 0x81 || a.PC != static_cast<unsigned short>(start + 5))) {
        printf("NG: IY state IX=%04X IY=%04X clock=%d R=%02X PC=%04X\n", a.IX, a.IY, clocks, a.R, a.PC);
        failures++;
    }
}

int main()
{
    for (int op = 0; op < 256; op++) {
        std::vector<unsigned char> bytes{0xED, static_cast<unsigned char>(op)};
        if ((op & 0xC7) == 0x43) {
            bytes.push_back(0x00);
            bytes.push_back(0x40);
        }
        run(bytes, 0xED00 | op, {2});
    }
    run({0xDD, 0x00}, 0xDD00, {2});
    run({0xFD, 0x00}, 0xFD00, {2});
    run({0xDD, 0x3E, 0x12}, 0xDD3E, {2});
    run({0xDD, 0xFD, 0x21, 0x34, 0x12}, 0xFD21, {3});
    run({0xDD, 0xFD, 0x21, 0x34, 0x12}, 0xDDFD, {2});
    run({0xDD, 0xFD, 0x21, 0x34, 0x12}, 0xDD, {1});
    run({0xDD, 0xED, 0x00}, 0xDDED, {2});
    run({0xDD, 0xED, 0x00}, 0xED00, {3});
    for (unsigned char prefix : {0xDD, 0xFD}) {
        run({prefix, 0xCB, 1, 6}, (prefix << 8) | 0xCB, {2});
        run({prefix, 0xCB, 1, 6}, (prefix << 16) | 0xCB06, {4});
    }
    std::vector<unsigned char> longCode(40, 0xDD);
    longCode.push_back(0x3E);
    longCode.push_back(0x12);
    std::vector<unsigned short> phases;
    for (unsigned short pc = 2; pc <= 40; pc++) phases.push_back(pc);
    run(longCode, 0xDDDD, phases);
    run(longCode, 0xDD3E, {41});
    run({0xDD, 0xFD, 0x21, 0x34, 0x12}, 0xFD21, {0}, 0xFFFD);
    run({0xDD, 0xCB, 1, 6}, 0xDDCB06, {2}, 0xFFFE);
    run({0xED, 0x43, 0, 0x40}, 0xED43, {0}, 0xFFFE);
    Machine continuous({0xDD, 0xED, 0x00}, 0);
    continuous.cpu.wtc.fetch = continuous.cpu.wtc.fetchM = continuous.cpu.wtc.read = 1;
    continuous.cpu.addBreakOperand(0xED, 0x00, Machine::notify);
    continuous.cpu.setConsumeClockCallback([](void* arg, int) { static_cast<Machine*>(arg)->cpu.requestBreak(); });
    continuous.cpu.execute();
    if (!continuous.valid || continuous.callbacks != std::vector<unsigned short>({3}) || continuous.cpu.reg.PC != 3 || continuous.cpu.reg.R != 0x81) failures++;
    printf("%s: operand breakpoint regression tests\n", failures ? "NG" : "OK");
    return failures ? 1 : 0;
}
