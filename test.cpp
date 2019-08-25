#include "z80.hpp"

unsigned char RAM[0x10000];
unsigned char IO[0x100];

unsigned char readByte(unsigned short addr)
{
    printf("read from $%04X: $%02X\n", addr, RAM[addr]);
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
    Z80 z80(readByte, writeByte, inPort, outPort);
    z80.executeTick4MHz();
    return 0;
}