#include "z80.hpp"

int main()
{
    unsigned char rom[256] = {
        0x01, 0x34, 0x12, // LD BC, $1234
        0x3E, 0x01,       // LD A, $01
        0xED, 0x79,       // OUT (C), A
        0xED, 0x78,       // IN A, (C)
        0xc3, 0x00, 0x00, // JMP $0000
    };

    Z80 z80([&rom](void* arg, unsigned short addr) {
        return rom[addr & 0xFF];
    }, [](void* arg, unsigned char addr, unsigned char value) {
        ((Z80*)arg)->requestBreak();
    }, [](void* arg, unsigned short port) {
        return 0x00;
    }, [](void* arg, unsigned short port, unsigned char value) {
        putc(value, stdout);
    }, &z80);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
    for (unsigned short addr = 0; addr < 0x8000; addr++) {
        for (int i = 0; i < 10; i++) {
            z80.addBreakPoint(addr, [=](void* arg) {
                printf("break 0x%04X (%d)\n", addr, i);
            });
        }
    }
    for (int operand = 0; operand < 65536; operand++) {
        unsigned char op1 = (unsigned char)((operand & 0xFF00) >> 8);
        unsigned char op2 = (unsigned char)(operand & 0xFF);
        for (int i = 0; i < 10; i++) {
            z80.addBreakOperand(op1, op2, [=](void* arg, const unsigned char* opcode, int opcodeLength) {
                char buf[1024];
                buf[0] = '\0';
                for (int j = 0; j < opcodeLength; j++) {
                    char hex[8];
                    if (j) {
                        snprintf(hex, sizeof(hex), ",%02X", opcode[j]);
                    } else {
                        snprintf(hex, sizeof(hex), "%02X", opcode[j]);
                    }
                    strcat(buf, hex);
                }
                printf("break#%d op1=%02X, op2=%02X (len=%d) ... opcode=%s\n", i, op1, op2, opcodeLength, buf);
            });
        }
    }
    z80.execute(50);
    puts("\n===== remove break point 3 and 7 =====\n");
    z80.removeBreakPoint(3);
    z80.removeBreakPoint(7);
    z80.execute(50);
    puts("\n===== remove all break points =====\n");
    z80.removeAllBreakPoints();
    z80.execute(50);
    puts("\n===== remove break operand ED =====\n");
    z80.removeBreakOperand(0xED);
    z80.execute(50);
    puts("\n===== remove all break operands =====\n");
    z80.removeAllBreakOperands();
    z80.execute(50);
    return 0;
}
