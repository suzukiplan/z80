#include <ctype.h>
#include "z80.hpp"

// 利用プログラム側で MMU (Memory Management Unit) を実装します。
// ミニマムでは 64KB の RAM と 256バイト の I/O ポートが必要です。
class MMU
{
  public:
    unsigned char RAM[0x10000];
    unsigned char IO[0x100];

    // コンストラクタで0クリアしておく
    MMU()
    {
        memset(&RAM, 0, sizeof(RAM));
        memset(&IO, 0, sizeof(IO));
    }
};

// CPUからメモリ読み込み要求発生時のコールバック
unsigned char readByte(void* arg, unsigned short addr)
{
    return ((MMU*)arg)->RAM[addr];
}

// CPUからメモリ書き込み要求発生時のコールバック
void writeByte(void* arg, unsigned short addr, unsigned char value)
{
    if (addr < 0x2000) {
        // 0x2000以下は検証プログラム用ROM扱いにする（CPUからの書き込みは禁止しておく）
        return;
    }
    ((MMU*)arg)->RAM[addr] = value;
}

// 入力命令（IN）発生時のコールバック
unsigned char inPort(void* arg, unsigned char port)
{
    return ((MMU*)arg)->IO[port];
}

// 出力命令（OUT）発生時のコールバック
void outPort(void* arg, unsigned char port, unsigned char value)
{
    ((MMU*)arg)->IO[port] = value;
}

// 16進数文字列かチェック（デバッグコマンド用）
bool isHexDigit(char c)
{
    if (isdigit(c)) return true;
    char uc = toupper(c);
    return 'A' <= uc && uc <= 'F';
}

// 16進数文字列を数値に変換（デバッグコマンド用）
int hexToInt(const char* cp)
{
    int result = 0;
    while (isHexDigit(*cp)) {
        result <<= 4;
        switch (toupper(*cp)) {
            case '0': break;
            case '1': result += 1; break;
            case '2': result += 2; break;
            case '3': result += 3; break;
            case '4': result += 4; break;
            case '5': result += 5; break;
            case '6': result += 6; break;
            case '7': result += 7; break;
            case '8': result += 8; break;
            case '9': result += 9; break;
            case 'A': result += 10; break;
            case 'B': result += 11; break;
            case 'C': result += 12; break;
            case 'D': result += 13; break;
            case 'E': result += 14; break;
            case 'F': result += 15; break;
            default: result >>= 4; return result;
        }
        cp++;
    }
    return result;
}

static void extractProgram(MMU& mmu);

int main()
{
    // MMUインスタンスを作成
    MMU mmu;
    extractProgram(mmu);

    // CPUインスタンスを作成
    // コールバック、コールバック引数、デバッグ出力設定を行う
    // コールバック引数: コールバックに渡す任意の引数（ここでは、MMUインスタンスのポインタを指定）
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu);

    // デバッグメッセージを標準出力
    z80.setDebugMessage([](void* arg, const char* message) -> void {
        time_t t1 = time(NULL);
        struct tm* t2 = localtime(&t1);
        printf("%04d.%02d.%02d %02d:%02d:%02d %s\n",
               t2->tm_year + 1900, t2->tm_mon, t2->tm_mday, t2->tm_hour,
               t2->tm_min, t2->tm_sec, message);
    });

    /*
    // Use break point:
    z80.setBreakPoint(0x008E, [](void* arg) -> void {
        printf("Detect break point! (PUSH ENTER TO CONTINUE)");
        char buf[80];
        fgets(buf, sizeof(buf), stdin);
    });
    */

    /*
    // You can detect the timing of clock consume by following:
    z80.setConsumeClockCallback([](void* arg, int clock) -> void {
        printf("consumed: %dHz\n", clock);
    });
    */

    // レジスタ初期値を設定（未設定時は0）
    z80.reg.pair.A = 0x12;
    z80.reg.pair.B = 0x34;
    z80.reg.pair.L = 0x01;
    z80.reg.IY = 1;

    // ステップ実行
    char cmd[1024];
    int clocks = 0;
    printf("> ");
    while (fgets(cmd, sizeof(cmd), stdin)) {
        if (isdigit(cmd[0])) {
            // 数字が入力されたらそのクロック数CPUを実行
            int hz = atoi(cmd);
            hz = z80.execute(hz);
            if (hz < 0) break;
            clocks += hz;
        } else if ('R' == toupper(cmd[0])) {
            // レジスタダンプ
            z80.registerDump();
        } else if ('M' == toupper(cmd[0])) {
            // メモリダンプ
            int addr = 0;
            for (char* cp = &cmd[1]; *cp; cp++) {
                if (isHexDigit(*cp)) {
                    addr = hexToInt(cp);
                    break;
                }
            }
            addr &= 0xFFFF;
            printf("[%04X] %02X %02X %02X %02X - %02X %02X %02X %02X - %02X %02X %02X %02X - %02X %02X %02X %02X\n", addr,
                   mmu.RAM[addr],
                   mmu.RAM[(addr + 1) & 0xFFFF],
                   mmu.RAM[(addr + 2) & 0xFFFF],
                   mmu.RAM[(addr + 3) & 0xFFFF],
                   mmu.RAM[(addr + 4) & 0xFFFF],
                   mmu.RAM[(addr + 5) & 0xFFFF],
                   mmu.RAM[(addr + 6) & 0xFFFF],
                   mmu.RAM[(addr + 7) & 0xFFFF],
                   mmu.RAM[(addr + 8) & 0xFFFF],
                   mmu.RAM[(addr + 9) & 0xFFFF],
                   mmu.RAM[(addr + 10) & 0xFFFF],
                   mmu.RAM[(addr + 11) & 0xFFFF],
                   mmu.RAM[(addr + 12) & 0xFFFF],
                   mmu.RAM[(addr + 13) & 0xFFFF],
                   mmu.RAM[(addr + 14) & 0xFFFF],
                   mmu.RAM[(addr + 15) & 0xFFFF]);
        } else if ('\r' == cmd[0] || '\n' == cmd[0]) {
            break;
        }
        printf("> ");
    }
    printf("executed %dHz\n", clocks);
    return 0;
}

// ハンドアセンブルした検証用プログラムをRAMに展開
static void extractProgram(MMU& mmu)
{
    unsigned short addr = 0;
    mmu.RAM[addr++] = 0b01000111; // LD B, A
    mmu.RAM[addr++] = 0b00001110; // LD C, $56
    mmu.RAM[addr++] = 0x56;
    mmu.RAM[addr++] = 0b01010110; // LD D, (HL)
    mmu.RAM[addr++] = 0b11011101; // LD E, (IX+4)
    mmu.RAM[addr++] = 0b01011110;
    mmu.RAM[addr++] = 4;
    mmu.RAM[addr++] = 0b11111101; // LD H, (IY+4)
    mmu.RAM[addr++] = 0b01100110;
    mmu.RAM[addr++] = 4;
    mmu.RAM[addr++] = 0b01110000; // LD (HL), B
    mmu.RAM[addr++] = 0b11011101; // LD (IX+7), A
    mmu.RAM[addr++] = 0b01110111;
    mmu.RAM[addr++] = 7;
    mmu.RAM[addr++] = 0b11111101; // LD (IY+7), C
    mmu.RAM[addr++] = 0b01110001;
    mmu.RAM[addr++] = 7;
    mmu.RAM[addr++] = 0b00110110; // LD (HL), 123
    mmu.RAM[addr++] = 123;
    mmu.RAM[addr++] = 0b11011101; // LD (IX+9), 100
    mmu.RAM[addr++] = 0b00110110;
    mmu.RAM[addr++] = 9;
    mmu.RAM[addr++] = 100;
    mmu.RAM[addr++] = 0b11111101; // LD (IY+9), 200
    mmu.RAM[addr++] = 0b00110110;
    mmu.RAM[addr++] = 9;
    mmu.RAM[addr++] = 200;
    mmu.RAM[addr++] = 0b00001010; // LD A, (BC)
    mmu.RAM[addr++] = 0b00011010; // LD A, (DE)
    mmu.RAM[addr++] = 0b00111010; // LD A, ($1234)
    mmu.RAM[addr++] = 0x34;
    mmu.RAM[addr++] = 0x12;
    mmu.RAM[addr++] = 0b00000010; // LD (BC), A
    mmu.RAM[addr++] = 0b00010010; // LD (DE), A
    mmu.RAM[addr++] = 0b00110010; // LD ($5678), A
    mmu.RAM[addr++] = 0x78;
    mmu.RAM[addr++] = 0x56;
    mmu.RAM[addr++] = 0b11101101; // LD A, I
    mmu.RAM[addr++] = 0b01010111;
    mmu.RAM[addr++] = 0b11101101; // LD I, A
    mmu.RAM[addr++] = 0b01000111;
    mmu.RAM[addr++] = 0b11101101; // LD A, R
    mmu.RAM[addr++] = 0b01011111;
    mmu.RAM[addr++] = 0b11101101; // LD R, A
    mmu.RAM[addr++] = 0b01001111;
    mmu.RAM[addr++] = 0b00000001; // LD BC, $ABCD
    mmu.RAM[addr++] = 0xCD;
    mmu.RAM[addr++] = 0xAB;
    mmu.RAM[addr++] = 0b11011101; // LD IX, $1234
    mmu.RAM[addr++] = 0b00100001;
    mmu.RAM[addr++] = 0x34;
    mmu.RAM[addr++] = 0x12;
    mmu.RAM[addr++] = 0b11111101; // LD IY, $5678
    mmu.RAM[addr++] = 0b00100001;
    mmu.RAM[addr++] = 0x78;
    mmu.RAM[addr++] = 0x56;
    mmu.RAM[addr++] = 0b00101010; // LD HL, ($1234)
    mmu.RAM[addr++] = 0x34;
    mmu.RAM[addr++] = 0x12;
    mmu.RAM[addr++] = 0b11101101; // LD SP, ($0011)
    mmu.RAM[addr++] = 0b01111011;
    mmu.RAM[addr++] = 0x11;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11011101; // LD IX, ($0002)
    mmu.RAM[addr++] = 0b00101010;
    mmu.RAM[addr++] = 0x02;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11111101; // LD IY, ($0004)
    mmu.RAM[addr++] = 0b00101010;
    mmu.RAM[addr++] = 0x04;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b00100010; // LD ($0010), HL
    mmu.RAM[addr++] = 0x10;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11101101; // LD ($0020), DE
    mmu.RAM[addr++] = 0b01010011;
    mmu.RAM[addr++] = 0x20;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11011101; // LD ($0008), IX
    mmu.RAM[addr++] = 0b00100010;
    mmu.RAM[addr++] = 0x08;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11111101; // LD ($0018), IY
    mmu.RAM[addr++] = 0b00100010;
    mmu.RAM[addr++] = 0x18;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11111001; // LD SP, HL
    mmu.RAM[addr++] = 0b11011101; // LD SP, IX
    mmu.RAM[addr++] = 0b11111001;
    mmu.RAM[addr++] = 0b11111101; // LD SP, IY
    mmu.RAM[addr++] = 0b11111001;
    mmu.RAM[addr++] = 0b11101101; // LDI
    mmu.RAM[addr++] = 0b10100000;
    mmu.RAM[addr++] = 0b00000001; // LD BC, $0005
    mmu.RAM[addr++] = 0x05;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11101101; // LDIR
    mmu.RAM[addr++] = 0b10110000;
    mmu.RAM[addr++] = 0b11101101; // LDD
    mmu.RAM[addr++] = 0b10101000;
    mmu.RAM[addr++] = 0b00000001; // LD BC, $0005
    mmu.RAM[addr++] = 0x05;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b11101101; // LDDR
    mmu.RAM[addr++] = 0b10111000;
    mmu.RAM[addr++] = 0b11101011; // EX DE, HL
    mmu.RAM[addr++] = 0b00001000; // EX AF, AF'
    mmu.RAM[addr++] = 0b11011001; // EXX
    mmu.RAM[addr++] = 0b11100011; // EX (SP), HL
    mmu.RAM[addr++] = 0b11011101; // EX (SP), IX
    mmu.RAM[addr++] = 0b11100011;
    mmu.RAM[addr++] = 0b11111101; // EX (SP), IY
    mmu.RAM[addr++] = 0b11100011;
    mmu.RAM[addr++] = 0b00110001; // LD SP, $0000
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0x00;
    mmu.RAM[addr++] = 0b00000001; // LD BC, $ABCD
    mmu.RAM[addr++] = 0xCD;
    mmu.RAM[addr++] = 0xAB;
    mmu.RAM[addr++] = 0b11000101; // PUSH BC
    mmu.RAM[addr++] = 0b00000001; // LD BC, $1234
    mmu.RAM[addr++] = 0x34;
    mmu.RAM[addr++] = 0x12;
    mmu.RAM[addr++] = 0b11000001; // POP BC
    mmu.RAM[addr++] = 0b11110101; // PUSH AF
    mmu.RAM[addr++] = 0b11110001; // POP AF
    mmu.RAM[addr++] = 0b11011101; // PUSH IX
    mmu.RAM[addr++] = 0b11100101;
    mmu.RAM[addr++] = 0b11111101; // PUSH IY
    mmu.RAM[addr++] = 0b11100101;
    mmu.RAM[addr++] = 0b11011101; // POP IX
    mmu.RAM[addr++] = 0b11100001;
    mmu.RAM[addr++] = 0b11111101; // POP IY
    mmu.RAM[addr++] = 0b11100001;
    mmu.RAM[addr++] = 0b00111110; // LD A, $AA
    mmu.RAM[addr++] = 0xAA;
    mmu.RAM[addr++] = 0b00000111; // RLCA
    mmu.RAM[addr++] = 0b00010111; // RLA
    mmu.RAM[addr++] = 0b11001011; // RLC A
    mmu.RAM[addr++] = 0b00000111;
    mmu.RAM[addr++] = 0b11001011; // RL A
    mmu.RAM[addr++] = 0b00010111;
    mmu.RAM[addr++] = 0b00100001; // LD HL, $FFFD
    mmu.RAM[addr++] = 0xFD;
    mmu.RAM[addr++] = 0xFF;
    mmu.RAM[addr++] = 0b11001011; // RLC (HL)
    mmu.RAM[addr++] = 0b00000110;
    mmu.RAM[addr++] = 0b11001011; // RL (HL)
    mmu.RAM[addr++] = 0b00010110;
    mmu.RAM[addr++] = 0b11011101; // RLC (IX+$12)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x12;
    mmu.RAM[addr++] = 0b00000110;
    mmu.RAM[addr++] = 0b11011101; // RL (IX+$34)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x34;
    mmu.RAM[addr++] = 0b00010110;
    mmu.RAM[addr++] = 0b11111101; // RLC (IY+$56)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x56;
    mmu.RAM[addr++] = 0b00000110;
    mmu.RAM[addr++] = 0b11111101; // RL (IY+$78)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x78;
    mmu.RAM[addr++] = 0b00010110;
    mmu.RAM[addr++] = 0b00001111; // RRCA
    mmu.RAM[addr++] = 0b00011111; // RRA
    mmu.RAM[addr++] = 0b11001011; // RRC A
    mmu.RAM[addr++] = 0b00001111;
    mmu.RAM[addr++] = 0b11001011; // RR A
    mmu.RAM[addr++] = 0b00011111;
    mmu.RAM[addr++] = 0b11001011; // RRC (HL)
    mmu.RAM[addr++] = 0b00001110;
    mmu.RAM[addr++] = 0b11001011; // RR (HL)
    mmu.RAM[addr++] = 0b00011110;
    mmu.RAM[addr++] = 0b11011101; // RRC (IX+$12)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x12;
    mmu.RAM[addr++] = 0b00001110;
    mmu.RAM[addr++] = 0b11011101; // RR (IX+$34)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x34;
    mmu.RAM[addr++] = 0b00011110;
    mmu.RAM[addr++] = 0b11111101; // RRC (IY+$56)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x56;
    mmu.RAM[addr++] = 0b00001110;
    mmu.RAM[addr++] = 0b11111101; // RR (IY+$78)
    mmu.RAM[addr++] = 0b11001011;
    mmu.RAM[addr++] = 0x78;
    mmu.RAM[addr++] = 0b00011110;
}