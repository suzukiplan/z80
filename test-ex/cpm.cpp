#include "../z80.hpp"

// CP/M Emulator (minimum implementation)
class CPM {
  public:
    char lineBuffer[0x101];
    unsigned char linePointer = 0;
    unsigned char memory[0x10000];
    bool halted;
    bool error;
    void (*lineCallback)(CPM*, char*);

    bool init(char* cimPath) {
        // read cim file
        FILE* fp = fopen(cimPath, "rb");
        if (!fp) {
            printf("File not found: %s\n", cimPath);
            return false;
        }
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        if (size < 1 || 0xFFFF - 0x100 < size) {
            printf("Invalid cim file size: %s\n", cimPath);
            fclose(fp);
            return false;
        }
        memset(memory, 0, sizeof(memory));
        fseek(fp, 0, SEEK_SET);
        if (size != fread(memory + 0x100, 1, size, fp)) {
            printf("Cannot read file: %s\n", cimPath);
            fclose(fp);
            return false;
        }
        fclose(fp);
        initBios();
        halted = false;
        error = false;
        clearLineBuffer();
        lineCallback = NULL;
        return true;
    }

    void initBios() {
        const unsigned char bios0000[] = { 0xc3, 0x03, 0xff, 0x00, 0x00, 0xc3, 0x06, 0xfe };
        const unsigned char biosFE06[] = { 0x79, 0xfe, 0x02, 0x28, 0x05, 0xfe, 0x09, 0x28, 0x05, 0x76, 0x7b, 0xd3, 0x00, 0xc9, 0x1a, 0xfe, 0x24, 0xc8, 0xd3, 0x00, 0x13, 0x18, 0xf7 };
        const unsigned char biosFF03[] = { 0x76 };
        memcpy(&memory[0x0000], bios0000, sizeof(bios0000));
        memcpy(&memory[0xFE06], biosFE06, sizeof(biosFE06));
        memcpy(&memory[0xFF03], biosFF03, sizeof(biosFF03));
    }

    unsigned char readMemory(unsigned short addr) { return memory[addr]; }
    void writeMemory(unsigned short addr, unsigned char value) { memory[addr] = value; }

    unsigned char inPort(unsigned char port) {
        printf("Unimplemented Input Port $0x%02X\n", port);
        return 0x00;
    }

    void outPort(unsigned char port, unsigned char value) {
        if (0x00 == port) {
            putc(value, stdout);
            if (value != '\n') {
                lineBuffer[linePointer++] = value;
            } else {
                if (lineCallback) {
                    lineCallback(this, lineBuffer);
                }
                clearLineBuffer();
            }
        } else {
            printf("Unimplemented Output Port $0x%02X <- $%02X\n", port, value);
        }
    }

    void clearLineBuffer() {
        memset(lineBuffer, 0, sizeof(lineBuffer));
        linePointer = 0;
    }
};

// bridge static functions
static inline unsigned char readMemory(void* arg, unsigned short addr) { return ((CPM*)arg)->readMemory(addr); }
static inline void writeMemory(void* arg, unsigned short addr, unsigned char value) { ((CPM*)arg)->writeMemory(addr, value); }
static inline unsigned char inPort(void* arg, unsigned char port) { return ((CPM*)arg)->inPort(port); }
static inline void outPort(void* arg, unsigned char port, unsigned char value) { ((CPM*)arg)->outPort(port, value); }

int main(int argc, char* argv[])
{
    if (argc < 2) {
        puts("usage: cpm path/to/file.cim");
        return 1;
    }
    CPM cpm;
    Z80 z80(readMemory, writeMemory, inPort, outPort, &cpm);
    if (!cpm.init(argv[1])) {
        puts("Cannot initialized");
        return -1;
    }
    z80.reg.PC = 0x0100;
    z80.addBreakOperand(0x76, [](void* arg) {
        ((CPM*)arg)->halted = true;
    });
    cpm.lineCallback = [](CPM* cpm, char* line) {
        if (strstr(line, "ERROR")) {
            cpm->halted = true;
            cpm->error = true;
        }
    };
    while (true) {
        z80.execute(100);
        if (cpm.halted) {
            printf("CPM halted at $0x%04X\n", z80.reg.PC);
            break;
        }
    }
    return cpm.error ? -1 : 0;
}