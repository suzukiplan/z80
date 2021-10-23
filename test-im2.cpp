#include "z80.hpp"

static unsigned char romPage00[256] = {
    0x3e, 0x80,         // LD A, $80
    0xed, 0x47,         // LD I, A
    0xed, 0x5e,         // IM 2
    0xfb,               // EI
    0x00,               // NOP
    0xc3, 0x07, 0x00,   // JMP $0007
};

static unsigned char romPage80[256] = {
    0x02, 0x80,         // interrupt vector mode 2: $8002
    0xed, 0x4d,         // RETI
};

static unsigned char ram[256];


unsigned char readMemory(void* ctx, unsigned short addr) {
    switch (addr & 0xFF00) {
        case 0x0000: return romPage00[addr & 0x00FF];
        case 0x8000: return romPage80[addr & 0x00FF];
        case 0xFF00: return ram[addr & 0x00FF];
        default: return 0xFF;
    }
}

void writeMemory(void* ctx, unsigned short addr, unsigned char value) {
    ram[addr & 0x00FF] = value;
}

unsigned char readPort(void* ctx, unsigned char port) { return 0x00; }
void writePort(void* ctx, unsigned char port, unsigned char value) { }

int main()
{
    Z80 z80(readMemory, writeMemory, readPort, writePort, NULL);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
    z80.execute(100);
    z80.generateIRQ(0);
    z80.execute(100);
    return 0;
}

