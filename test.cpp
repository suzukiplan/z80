#include <ctype.h>
#include "z80.hpp"

// 利用プログラム側で MMU (Memory Management Unit) を実装します。
// ミニマムでは 64KB の RAM と 256バイト の I/O ポートが必要です。
class MMU
{
  public:
    unsigned char RAM[0x10000];
    unsigned char IO[0x100];
    Z80* cpu;

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

int main(int argc, char* argv[])
{
    int autoExec = 1;
    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], "-c")) {
            autoExec = 0;
        } else if (0 == strcmp(argv[i], "-a")) {
            autoExec = 1;
        }
    }

    // MMUインスタンスを作成
    MMU mmu;
    extractProgram(mmu);

    // CPUインスタンスを作成
    // コールバック、コールバック引数、デバッグ出力設定を行う
    // コールバック引数: コールバックに渡す任意の引数（ここでは、MMUインスタンスのポインタを指定）
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu);
    mmu.cpu = &z80;

    // デバッグメッセージを標準出力
    z80.setDebugMessage([](void* arg, const char* message) -> void {
        time_t t1 = time(NULL);
        struct tm* t2 = localtime(&t1);
        printf("%04d.%02d.%02d %02d:%02d:%02d %s\n",
               t2->tm_year + 1900, t2->tm_mon, t2->tm_mday, t2->tm_hour,
               t2->tm_min, t2->tm_sec, message);
        static FILE* fp;
        if (!fp) fp = fopen("test-result.txt", "w");
        fprintf(fp, "%s\n", message);
    });

    /*
    // Use break point:
    z80.setBreakPoint(0x008E, [](void* arg) -> void {
        printf("Detect break point! (PUSH ENTER TO CONTINUE)");
        char buf[80];
        fgets(buf, sizeof(buf), stdin);
    });
    */

    // autoExecモードの場合はNOPを検出したらレジスタを出力してプログラムを終了
    if (autoExec) {
        z80.setBreakOperand(0x00, [](void* arg) -> void {
            ((MMU*)arg)->cpu->registerDump();
            exit(0);
        });
    }

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

    if (autoExec) {
        // 通常は面倒なのでNOPを検出するまで実行し続ける
        while (0 < z80.executeTick4MHz()) {
            puts("--- proceed 1 frame (1/60sec) of 4MHz ---");
        }
    } else {
        // ステップ実行 (細かい命令毎のテスト用)
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
    }
    return 0;
}

// ハンドアセンブルした検証用プログラムをRAMに展開
#define PG mmu.RAM[addr++]
static void extractProgram(MMU& mmu)
{
    unsigned short addr = 0;
    PG = 0b01000111; // LD B, A
    PG = 0b00001110; // LD C, $56
    PG = 0x56;
    PG = 0b01010110; // LD D, (HL)
    PG = 0b11011101; // LD E, (IX+4)
    PG = 0b01011110;
    PG = 4;
    PG = 0b11111101; // LD H, (IY+4)
    PG = 0b01100110;
    PG = 4;
    PG = 0b01110000; // LD (HL), B
    PG = 0b11011101; // LD (IX+7), A
    PG = 0b01110111;
    PG = 7;
    PG = 0b11111101; // LD (IY+7), C
    PG = 0b01110001;
    PG = 7;
    PG = 0b00110110; // LD (HL), 123
    PG = 123;
    PG = 0b11011101; // LD (IX+9), 100
    PG = 0b00110110;
    PG = 9;
    PG = 100;
    PG = 0b11111101; // LD (IY+9), 200
    PG = 0b00110110;
    PG = 9;
    PG = 200;
    PG = 0b00001010; // LD A, (BC)
    PG = 0b00011010; // LD A, (DE)
    PG = 0b00111010; // LD A, ($1234)
    PG = 0x34;
    PG = 0x12;
    PG = 0b00000010; // LD (BC), A
    PG = 0b00010010; // LD (DE), A
    PG = 0b00110010; // LD ($5678), A
    PG = 0x78;
    PG = 0x56;
    PG = 0b11101101; // LD A, I
    PG = 0b01010111;
    PG = 0b11101101; // LD I, A
    PG = 0b01000111;
    PG = 0b11101101; // LD A, R
    PG = 0b01011111;
    PG = 0b11101101; // LD R, A
    PG = 0b01001111;
    PG = 0b00000001; // LD BC, $ABCD
    PG = 0xCD;
    PG = 0xAB;
    PG = 0b11011101; // LD IX, $1234
    PG = 0b00100001;
    PG = 0x34;
    PG = 0x12;
    PG = 0b11111101; // LD IY, $5678
    PG = 0b00100001;
    PG = 0x78;
    PG = 0x56;
    PG = 0b00101010; // LD HL, ($1234)
    PG = 0x34;
    PG = 0x12;
    PG = 0b11101101; // LD SP, ($0011)
    PG = 0b01111011;
    PG = 0x11;
    PG = 0x00;
    PG = 0b11011101; // LD IX, ($0002)
    PG = 0b00101010;
    PG = 0x02;
    PG = 0x00;
    PG = 0b11111101; // LD IY, ($0004)
    PG = 0b00101010;
    PG = 0x04;
    PG = 0x00;
    PG = 0b00100010; // LD ($0010), HL
    PG = 0x10;
    PG = 0x00;
    PG = 0b11101101; // LD ($0020), DE
    PG = 0b01010011;
    PG = 0x20;
    PG = 0x00;
    PG = 0b11011101; // LD ($0008), IX
    PG = 0b00100010;
    PG = 0x08;
    PG = 0x00;
    PG = 0b11111101; // LD ($0018), IY
    PG = 0b00100010;
    PG = 0x18;
    PG = 0x00;
    PG = 0b11111001; // LD SP, HL
    PG = 0b11011101; // LD SP, IX
    PG = 0b11111001;
    PG = 0b11111101; // LD SP, IY
    PG = 0b11111001;
    PG = 0b11101101; // LDI
    PG = 0b10100000;
    PG = 0b00000001; // LD BC, $0005
    PG = 0x05;
    PG = 0x00;
    PG = 0b11101101; // LDIR
    PG = 0b10110000;
    PG = 0b11101101; // LDD
    PG = 0b10101000;
    PG = 0b00000001; // LD BC, $0005
    PG = 0x05;
    PG = 0x00;
    PG = 0b11101101; // LDDR
    PG = 0b10111000;
    PG = 0b11101011; // EX DE, HL
    PG = 0b00001000; // EX AF, AF'
    PG = 0b11011001; // EXX
    PG = 0b11100011; // EX (SP), HL
    PG = 0b11011101; // EX (SP), IX
    PG = 0b11100011;
    PG = 0b11111101; // EX (SP), IY
    PG = 0b11100011;
    PG = 0b00110001; // LD SP, $0000
    PG = 0x00;
    PG = 0x00;
    PG = 0b00000001; // LD BC, $ABCD
    PG = 0xCD;
    PG = 0xAB;
    PG = 0b11000101; // PUSH BC
    PG = 0b00000001; // LD BC, $1234
    PG = 0x34;
    PG = 0x12;
    PG = 0b11000001; // POP BC
    PG = 0b11110101; // PUSH AF
    PG = 0b11110001; // POP AF
    PG = 0b11011101; // PUSH IX
    PG = 0b11100101;
    PG = 0b11111101; // PUSH IY
    PG = 0b11100101;
    PG = 0b11011101; // POP IX
    PG = 0b11100001;
    PG = 0b11111101; // POP IY
    PG = 0b11100001;
    PG = 0b00111110; // LD A, $AA
    PG = 0xAA;
    PG = 0b00000111; // RLCA
    PG = 0b00010111; // RLA
    PG = 0b11001011; // RLC A
    PG = 0b00000111;
    PG = 0b11001011; // RL A
    PG = 0b00010111;
    PG = 0b00100001; // LD HL, $FFFD
    PG = 0xFD;
    PG = 0xFF;
    PG = 0b11001011; // RLC (HL)
    PG = 0b00000110;
    PG = 0b11001011; // RL (HL)
    PG = 0b00010110;
    PG = 0b11011101; // RLC (IX+$12)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00000110;
    PG = 0b11011101; // RL (IX+$34)
    PG = 0b11001011;
    PG = 0x34;
    PG = 0b00010110;
    PG = 0b11111101; // RLC (IY+$56)
    PG = 0b11001011;
    PG = 0x56;
    PG = 0b00000110;
    PG = 0b11111101; // RL (IY+$78)
    PG = 0b11001011;
    PG = 0x78;
    PG = 0b00010110;
    PG = 0b00001111; // RRCA
    PG = 0b00011111; // RRA
    PG = 0b11001011; // RRC A
    PG = 0b00001111;
    PG = 0b11001011; // RR A
    PG = 0b00011111;
    PG = 0b11001011; // RRC (HL)
    PG = 0b00001110;
    PG = 0b11001011; // RR (HL)
    PG = 0b00011110;
    PG = 0b11011101; // RRC (IX+$12)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00001110;
    PG = 0b11011101; // RR (IX+$34)
    PG = 0b11001011;
    PG = 0x34;
    PG = 0b00011110;
    PG = 0b11111101; // RRC (IY+$56)
    PG = 0b11001011;
    PG = 0x56;
    PG = 0b00001110;
    PG = 0b11111101; // RR (IY+$78)
    PG = 0b11001011;
    PG = 0x78;
    PG = 0b00011110;
    PG = 0b11001011; // SLA A
    PG = 0b00100111;
    PG = 0b11001011; // SLA (HL)
    PG = 0b00100110;
    PG = 0b11011101; // SLA (IX+$12)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00100110;
    PG = 0b11111101; // SLA (IY+$34)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00100110;
    PG = 0b11001011; // SRA A
    PG = 0b00101111;
    PG = 0b11001011; // SRA (HL)
    PG = 0b00101110;
    PG = 0b11011101; // SRA (IX+$12)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00101110;
    PG = 0b11111101; // SRA (IY+$34)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00101110;
    PG = 0b11001011; // SRL A
    PG = 0b00111111;
    PG = 0b11001011; // SRL (HL)
    PG = 0b00111110;
    PG = 0b11011101; // SRL (IX+$12)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00111110;
    PG = 0b11111101; // SRL (IY+$34)
    PG = 0b11001011;
    PG = 0x12;
    PG = 0b00111110;
    PG = 0b10000000; // ADD A, B
    PG = 0b11000110; // ADD A, 127
    PG = 0x7F;
    PG = 0b10000110; // ADD A, (HL)
    PG = 0b11011101; // ADD A, (IX+$12)
    PG = 0b10000110;
    PG = 0x12;
    PG = 0b11111101; // ADD A, (IY+$34)
    PG = 0b10000110;
    PG = 0x34;
    PG = 0b10001000; // ACD A, B
    PG = 0b11001110; // ACD A, 127
    PG = 0x7F;
    PG = 0b10001110; // ACD A, (HL)
    PG = 0b11011101; // ACD A, (IX+$12)
    PG = 0b10001110;
    PG = 0x12;
    PG = 0b11111101; // ACD A, (IY+$34)
    PG = 0b10001110;
    PG = 0x34;
    PG = 0b00111100; // INC A
    PG = 0b00110100; // INC (HL)
    PG = 0b11011101; // INC (IX+$12)
    PG = 0b00110100;
    PG = 0x12;
    PG = 0b11111101; // INC (IY+$34)
    PG = 0b00110100;
    PG = 0x34;
    PG = 0b10010000; // SUB A, B
    PG = 0b11010110; // SUB A, 127
    PG = 0x7F;
    PG = 0b10010110; // SUB A, (HL)
    PG = 0b11011101; // SUB A, (IX+$12)
    PG = 0b10010110;
    PG = 0x12;
    PG = 0b11111101; // SUB A, (IY+$34)
    PG = 0b10010110;
    PG = 0x34;
    PG = 0b10011000; // SBC A, B
    PG = 0b11011110; // SBC A, 127
    PG = 0x7F;
    PG = 0b10011110; // SBC A, (HL)
    PG = 0b11011101; // SBC A, (IX+$12)
    PG = 0b10011110;
    PG = 0x12;
    PG = 0b11111101; // SBC A, (IY+$34)
    PG = 0b10011110;
    PG = 0x34;
}