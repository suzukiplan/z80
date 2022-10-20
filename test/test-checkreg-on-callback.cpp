#include "z80.hpp"

int main()
{
    unsigned short expectIndex = 0;
    int expectPC[] = {0x0000, 0x0001, 0x0002, -1 };
    int expectSP[] = {0xFFFF, 0xFFFF, 0xFFFF, -1 };
    unsigned char rom[256] = {
        0x3A, 0x01, 0x80, // LD A, (NN)
    };
    Z80 z80([=, &expectIndex](void* arg, unsigned short addr) {
        if (addr < 0x100) {
            unsigned short sp = ((Z80*)arg)->reg.SP;
            unsigned short pc = ((Z80*)arg)->reg.PC;
            printf("Read memory ... $%04X SP=%04X, PC=%04X <expect: SP=%04X, PC=%04X>\n", addr, sp, pc, expectSP[expectIndex], expectPC[expectIndex]);
            if (sp != expectSP[expectIndex] || pc != expectPC[expectIndex]) {
                puts("unexpected!");
                // 
                //exit(-1);
            }
            expectIndex++;
            return rom[addr];
        } 
        return (unsigned char)0xFF;
    }, [](void* arg, unsigned char addr, unsigned char value) {
    }, [](void* arg, unsigned short port) {
        return 0x00;
    }, [](void* arg, unsigned short port, unsigned char value) {
    }, &z80);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
    z80.execute(1);
    return 0;
}
