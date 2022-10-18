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

    {
        puts("=== 8bit port mode ===");
        Z80 z80([&rom](void* arg, unsigned short addr) {
            return rom[addr & 0xFF];
        }, [](void* arg, unsigned char addr, unsigned char value) {
            // nothing to do
        }, [](void* arg, unsigned short port) {
            printf("IN port A <- $%02X%02X\n",
                ((Z80*)arg)->reg.pair.B, // the top half (A8 through A15) of the address bus (reg.B)
                port // the bottom half (A0 through A7) of the address bus to select the I/O device at one of 256 possible ports
            );
            return 0x00;
        }, [](void* arg, unsigned short port, unsigned char value) {
            printf("OUT port $%02X%02X <- $%02X\n",
                ((Z80*)arg)->reg.pair.B, // the top half (A8 through A15) of the address bus (reg.B)
                port, // the bottom half (A0 through A7) of the address bus to select the I/O device at one of 256 possible ports
                value
            );
        }, &z80, false);
        z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
        z80.execute(50);
    }

    {
        puts("=== 16bit port mode ===");
        Z80 z80([&rom](void* arg, unsigned short addr) {
            return rom[addr & 0xFF];
        }, [](void* arg, unsigned char addr, unsigned char value) {
            // nothing to do
        }, [](void* arg, unsigned short port) {
            printf("IN port A <- $%04X\n",
                port // the full (A0 through A15) of the address bus to select the I/O device at one of 65536 possible ports
            );
            return 0;
        }, [](void* arg, unsigned short port, unsigned char value) {
            printf("OUT port $%04X <- $%02X\n",
                port, // the full (A0 through A15) of the address bus to select the I/O device at one of 65536 possible ports
                value
            );
        }, &z80, true);
        z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
        z80.execute(50);
    }
    return 0;
}
