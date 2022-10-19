#include "z80.hpp"

int main()
{
    unsigned char rom[256] = {
        0x01, 0x34, 0x12, // LD BC, $1234
        0x3E, 0x01,       // LD A, $01
        0xED, 0x79,       // OUT (C), A
        0xED, 0x78,       // IN A, (C)
        0xc3, 0x09, 0x00, // JMP $0009
    };
    Z80 z80([=](void* arg, unsigned short addr) { return rom[addr & 0xFF]; },
            [](void* arg, unsigned short addr, unsigned char value) {},
            [](void* arg, unsigned short port) { return 0x00; },
            [](void* arg, unsigned short port, unsigned char value) {
                // request break the execute function after output port operand has executed.
                ((Z80*)arg)->requestBreak();
            }, &z80);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
    z80.setConsumeClockCallback([](void* arg, int clocks) { printf("consume %dHz\n", clocks); });
    puts("===== execute(0) =====");
    printf("actualExecuteClocks = %dHz\n", z80.execute(0));
    puts("===== execute(1) =====");
    printf("actualExecuteClocks = %dHz\n", z80.execute(1));
    puts("===== execute(0x7FFFFFFF) =====");
    printf("actualExecuteClocks = %dHz\n", z80.execute(0x7FFFFFFF));
    return 0;
}
