#include "z80.hpp"

int main()
{
    unsigned short expectIndex = 0;
    int expectAF[] = {
        0xFFFF, 0xFFFF, 0xFFFF,
        0x00FF, 0x00FF, 0x00FF,
        0x00FF, 0x00FF, 0x00FF,
        0x00FF, 0x00FF, 0x00FF,
        0x00FF, 0x00FF, 0x00FF,
        0x00FF, 0x00FF, 0x00FF,
        0x00FF, 0x00FF, 0x00FF, 0x00FF,
        -1};
    int expectBC[] = {
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0003,
        0x8003, 0x8003, 0x8003,
        0x8003, 0x8003, 0x8003,
        0x8003, 0x8003, 0x8003,
        0x8003, 0x8003, 0x8003, 0x8003,
        -1};
    int expectDE[] = {
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0004,
        0x8004, 0x8004, 0x8004,
        0x8004, 0x8004, 0x8004,
        0x8004, 0x8004, 0x8004, 0x8004,
        -1};
    int expectHL[] = {
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0005,
        0x8005, 0x8005, 0x8005,
        0x8005, 0x8005, 0x8005, 0x8005,
        -1};
    int expectPC[] = {
        0x0000, 0x0001, 0x0002,
        0x0003, 0x0004, 0x0005,
        0x0006, 0x0007, 0x0008,
        0x0009, 0x000A, 0x000B,
        0x000C, 0x000D, 0x000E,
        0x000F, 0x0010, 0x0011,
        0x0012, 0x0013, 0x0014, 0x0015,
        -1};
    int expectSP[] = {
        0xFFFF, 0xFFFF, 0xFFFF,
        0xFFFF, 0xFFFF, 0xFFFF,
        0xFFFF, 0xFFFF, 0xFFFF,
        0xFFFF, 0xFFFF, 0xFFFF,
        0xFFFF, 0xFFFF, 0xFFFF,
        0xFFFF, 0xFFFF, 0xFF06,
        0x8006, 0x8006, 0x8006, 0x8006,
        -1};
    int expectIX[] = {
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        -1};
    int expectIY[] = {
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000,
        -1};
    unsigned char rom[256] = {
        0x3A, 0x01, 0x80, // LD A, (NN)
        0x32, 0x02, 0x80, // LD (NN), A
        0x01, 0x03, 0x80, // LD BC, NN
        0x11, 0x04, 0x80, // LD DE, NN
        0x21, 0x05, 0x80, // LD HL, NN
        0x31, 0x06, 0x80, // LD SP, NN
        0xDD, 0x36, 0x7F, 0xBB, // LD (IX+d), N
        0x00, // NOP (end test)
    };
    Z80 z80([=, &expectIndex](void* arg, unsigned short addr) {
        if (addr < 0x100 && -1 != expectAF[expectIndex]) {
            unsigned short sp = ((Z80*)arg)->reg.SP;
            unsigned short pc = ((Z80*)arg)->reg.PC;
            unsigned short af = (unsigned short)(((Z80*)arg)->reg.pair.A * 256 + ((Z80*)arg)->reg.pair.F);
            unsigned short bc = (unsigned short)(((Z80*)arg)->reg.pair.B * 256 + ((Z80*)arg)->reg.pair.C);
            unsigned short de = (unsigned short)(((Z80*)arg)->reg.pair.D * 256 + ((Z80*)arg)->reg.pair.E);
            unsigned short hl = (unsigned short)(((Z80*)arg)->reg.pair.H * 256 + ((Z80*)arg)->reg.pair.L);
            unsigned short ix = ((Z80*)arg)->reg.IX;
            unsigned short iy = ((Z80*)arg)->reg.IY;
            printf("Read memory ... $%04X AF=%04X, BC=%04X, DE=%04X, HL=%04X, IX=%04X, IY=%04X, SP=%04X, PC=%04X\n", addr, af, bc, de, hl, ix, iy, sp, pc);
            if (af != expectAF[expectIndex] || 
                bc != expectBC[expectIndex] ||
                de != expectDE[expectIndex] ||
                hl != expectHL[expectIndex] ||
                ix != expectIX[expectIndex] ||
                iy != expectIY[expectIndex] ||
                sp != expectSP[expectIndex] ||
                pc != expectPC[expectIndex]) {
                puts("unexpected!");
                printf("expected:             AF=%04X, BC=%04X, DE=%04X, HL=%04X, IX=%04X, IY=%04X, SP=%04X, PC=%04X\n",
                    expectAF[expectIndex], 
                    expectBC[expectIndex], 
                    expectDE[expectIndex], 
                    expectHL[expectIndex], 
                    expectIX[expectIndex], 
                    expectIY[expectIndex], 
                    expectSP[expectIndex], 
                    expectPC[expectIndex]);
                exit(-1);
            }
            expectIndex++;
            return rom[addr];
        } 
        return (unsigned char)0x00;
    }, [](void* arg, unsigned char addr, unsigned char value) {
    }, [](void* arg, unsigned short port) {
        return 0x00;
    }, [](void* arg, unsigned short port, unsigned char value) {
    }, &z80);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
    z80.addBreakOperand(0x00, [](void* arg, unsigned char* op, int len) { exit(0); });
    z80.execute(INT_MAX);
    return 0;
}
