// ADC HL,rr / SBC HL,rr flag test.
// Compares every result with an independent model of the Z80 flags over
// operands that exercise the carry chain, with the carry flag both clear and
// set. Before the fix, cases with C = 1 and rr & 0x0FFF == 0x0FFF gave a
// wrong H flag, rr & 0x7FFF == 0x7FFF a wrong P/V flag, and rr == 0xFFFF
// also lost the carry.
#include "z80.hpp"

static unsigned char expectedFlags(bool sbc, unsigned short hl, unsigned short rr, int c)
{
    int result = sbc ? hl - rr - c : hl + rr + c;
    unsigned short value = (unsigned short)result;
    int signedResult = sbc ? (short)hl - (short)rr - c : (short)hl + (short)rr + c;
    int half = sbc ? (hl & 0x0FFF) - (rr & 0x0FFF) - c : (hl & 0x0FFF) + (rr & 0x0FFF) + c;
    unsigned char f = 0;
    f |= (value & 0x8000) ? 0x80 : 0;                             // S
    f |= value == 0 ? 0x40 : 0;                                   // Z
    f |= (value >> 8) & 0x28;                                     // Y, X
    f |= (half < 0 || half > 0x0FFF) ? 0x10 : 0;                  // H
    f |= (signedResult > 32767 || signedResult < -32768) ? 0x04 : 0; // P/V
    f |= sbc ? 0x02 : 0;                                          // N
    f |= (result < 0 || result > 0xFFFF) ? 0x01 : 0;              // C
    return f;
}

int main()
{
    const unsigned short values[] = {
        0x0000, 0x0001, 0x000F, 0x0010, 0x00FF, 0x0100, 0x0FFF, 0x1000, 0x3FFF, 0x7FFE,
        0x7FFF, 0x8000, 0x8001, 0xEFFF, 0xF000, 0xFFFE, 0xFFFF, 0x1234, 0xA5A5, 0x5A5A};
    unsigned char rom[4] = {0xED, 0x4A, 0x00, 0x00}; // ADC HL,BC or SBC HL,BC
    Z80 z80([&rom](void* arg, unsigned short addr) { return rom[addr & 3]; },
            [](void* arg, unsigned short addr, unsigned char value) {},
            [](void* arg, unsigned short port) { return (unsigned char)0xFF; },
            [](void* arg, unsigned short port, unsigned char value) {},
            &z80);
    int failures = 0;
    int cases = 0;
    for (int sbc = 0; sbc < 2; sbc++) {
        rom[1] = sbc ? 0x42 : 0x4A;
        for (unsigned short hl : values) {
            for (unsigned short rr : values) {
                for (int c = 0; c < 2; c++) {
                    z80.reg.PC = 0;
                    z80.reg.pair.H = (unsigned char)(hl >> 8);
                    z80.reg.pair.L = (unsigned char)hl;
                    z80.reg.pair.B = (unsigned char)(rr >> 8);
                    z80.reg.pair.C = (unsigned char)rr;
                    z80.reg.pair.F = (unsigned char)c;
                    z80.execute(1);
                    unsigned short expectedHL = (unsigned short)(sbc ? hl - rr - c : hl + rr + c);
                    unsigned short actualHL = (unsigned short)((z80.reg.pair.H << 8) | z80.reg.pair.L);
                    unsigned char expected = expectedFlags(sbc != 0, hl, rr, c);
                    cases++;
                    if (actualHL != expectedHL || z80.reg.pair.F != expected) {
                        printf("NG: %s HL=$%04X BC=$%04X C=%d -> HL=$%04X F=$%02X (expected HL=$%04X F=$%02X)\n",
                               sbc ? "SBC" : "ADC", hl, rr, c, actualHL, z80.reg.pair.F, expectedHL, expected);
                        failures++;
                    }
                }
            }
        }
    }
    if (failures) {
        printf("%d of %d cases failed\n", failures, cases);
        return -1;
    }
    printf("OK: %d cases\n", cases);
    return 0;
}
