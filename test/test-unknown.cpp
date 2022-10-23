#include "z80.hpp"

int main()
{
    unsigned char rom[] = {
        0xED, 0x30,     // unknown ED
        0xDD, 0x1F,     // unknown DD
        0xFD, 0x3F,     // unknown FD
    };
    Z80 z80([rom](void* arg, unsigned short addr) {
        return rom[addr];
    }, [](void* arg, unsigned char addr, unsigned char value) {
    }, [](void* arg, unsigned short port) {
        return 0x00;
    }, [](void* arg, unsigned short port, unsigned char value) {
    }, &z80);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });

    try {
        z80.execute(1);
    } catch(std::runtime_error& error) {
        puts(error.what());
        if (strcmp(error.what(), "detect an unknown operand (ED,30)")) exit(-1);
    }

    try {
        z80.execute(1);
    } catch(std::runtime_error& error) {
        puts(error.what());
        if (strcmp(error.what(), "detect an unknown operand (DD,1F)")) exit(-1);
    }

    try {
        z80.execute(1);
    } catch(std::runtime_error& error) {
        puts(error.what());
        if (strcmp(error.what(), "detect an unknown operand (FD,3F)")) exit(-1);
    }
    return 0;
}
