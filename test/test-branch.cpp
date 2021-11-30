#include "../z80.hpp"

static unsigned char rom[0x8000];
static unsigned char ram[0x8000];
static int nest = 0;

static unsigned char readMemory(void* arg, unsigned short addr)
{
    return addr < 0x8000 ? rom[addr] : ram[addr & 0x7FFF];
}

static void writeMemory(void* arg, unsigned short addr, unsigned char value)
{
    ram[addr & 0x7FFF] = value;
}

static unsigned char inPort(void* arg, unsigned char port) { return 0xFF; }
static void outPort(void* arg, unsigned char port, unsigned char value) {}

static void printIdent()
{
    printf("%2d ", nest);
    for (int i = 0; i < nest; i++) printf("*");
    if (nest) printf(" ");
}

int main()
{
    FILE* fp = fopen("test-branch.bin", "rb");
    fread(rom, 1, sizeof(rom), fp);
    fclose(fp);
    Z80 z80(readMemory, writeMemory, inPort, outPort, &z80);
    z80.addBreakOperand(0x00, [](void* arg) { exit(0); });
    z80.setDebugMessage([](void* arg, const char* msg) {
        printIdent();
        puts(msg);
    });
    z80.addCallHandler([](void* arg) {
        printf("Executed a CALL instruction:\n");
        printf("- Branched to: $%04X\n", ((Z80*)arg)->reg.PC);
        unsigned short sp = ((Z80*)arg)->reg.SP;
        unsigned short returnAddr = ((Z80*)arg)->readByte(sp + 1);
        returnAddr <<= 8;
        returnAddr |= ((Z80*)arg)->readByte(sp);
        printf("- Return to: $%04X\n", returnAddr);
    });
    z80.addReturnHandler([](void* arg) {
        printf("Detected a RET instruction:\n");
        printf("- Branch from: $%04X\n", ((Z80*)arg)->reg.PC);
        unsigned short sp = ((Z80*)arg)->reg.SP;
        unsigned short returnAddr = ((Z80*)arg)->readByte(sp + 1);
        returnAddr <<= 8;
        returnAddr |= ((Z80*)arg)->readByte(sp);
        printf("- Return to: $%04X\n", returnAddr);
    });
    z80.addCallHandler([](void* arg) {
        nest++;
    });
    z80.addReturnHandler([](void* arg) {
        nest--;
    });
    z80.execute(0x7FFFFFFF);
    return 0;
}
