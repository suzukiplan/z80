#include "z80.hpp"

int main()
{
    unsigned char rom[256] = {
        0x01, 0x10, 0x03, // LD BC, $0310
        0xED, 0xA2,       // INI
        0x01, 0x20, 0x02, // LD BC, $0220
        0xED, 0xB2,       // INIR
        0x01, 0x30, 0x03, // LD BC, $0330
        0xED, 0xAA,       // IND
        0x01, 0x40, 0x02, // LD BC, $0240
        0xED, 0xBA,       // INDR
        0x01, 0x50, 0x03, // LD BC, $0350
        0xED, 0xA3,       // OUTI
        0x01, 0x60, 0x02, // LD BC, $0260
        0xED, 0xB3,       // OTIR
        0x01, 0x70, 0x03, // LD BC, $0370
        0xED, 0xAB,       // OUTD
        0x01, 0x80, 0x02, // LD BC, $0280
        0xED, 0xBB,       // OTDR
    };
    unsigned short expectPorts[] = {
        0x0310,
        0x0220,
        0x0120,
        0x0330,
        0x0240,
        0x0140,
        0x0250,
        0x0160,
        0x0060,
        0x0270,
        0x0180,
        0x0080,
    };
    int expectPortIndex = 0;
    Z80 z80([&rom](void* arg, unsigned short addr) {
        return rom[addr & 0xFF];
    }, [](void* arg, unsigned char addr, unsigned char value) {
        // nothing to do
    }, [&](void* arg, unsigned short port) {
        printf("IN port A <- $%04X\n",
            port // the full (A0 through A15) of the address bus to select the I/O device at one of 65536 possible ports
        );
        if (port != expectPorts[expectPortIndex++]) {
            puts("UNEXPECTED!");
            exit(-1);
        }
        return 0;
    }, [](void* arg, unsigned short port, unsigned char value) {
        printf("OUT port $%04X <- $%02X\n",
            port, // the full (A0 through A15) of the address bus to select the I/O device at one of 65536 possible ports
            value
        );
    }, &z80, true);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
    z80.addBreakOperand(0x00, [](void* arg, unsigned char* operands, int size) {
        exit(0);
    });
    z80.execute(INT_MAX);
    return 0;
}
