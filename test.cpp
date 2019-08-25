#include "z80.hpp"

unsigned char RAM[0x10000];
unsigned char IO[0x100];

unsigned char readByte(unsigned short addr)
{
    return RAM[addr];
}

void writeByte(unsigned short addr, unsigned char value)
{
    RAM[addr] = value;
}

unsigned char inPort(unsigned char port)
{
    return IO[port];
}

void outPort(unsigned char port, unsigned char value)
{
    IO[port] = value;
}

int main()
{
    Z80 z80(readByte, writeByte, inPort, outPort, true);
    z80.execute(32);
    return 0;
}