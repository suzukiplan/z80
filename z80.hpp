// SUZUKI PLAN - Z80 Emulator
// Copyright 2019, SUZUKI PLAN. (MIT License)
#ifndef INCLUDE_Z80_HPP
#define INCLUDE_Z80_HPP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <functional>

class Z80
{
  public: // Interface data types
    struct RegisterPair {
        unsigned char A;
        unsigned char F;
        unsigned char B;
        unsigned char C;
        unsigned char D;
        unsigned char E;
        unsigned char H;
        unsigned char L;
    };

    struct Register {
        struct RegisterPair pair;
        struct RegisterPair back;
        unsigned short PC;
        unsigned short SP;
        unsigned short IX;
        unsigned short IY;
        unsigned char R;
        unsigned char I;
        unsigned char IFF;
        int interruptMode;
        bool isHalt;
    } reg;

  private: // Internal variables
    struct Callback {
        std::function<unsigned char(void* arg, unsigned short addr)> read;
        std::function<void(void* arg, unsigned short addr, unsigned char value)>
            write;
        std::function<unsigned char(void* arg, unsigned char port)> in;
        std::function<void(void* arg, unsigned char port, unsigned char value)> out;
        void* arg;
    } CB;
    FILE* debugStream;

  public: // utility functions
    inline void log(const char* format, ...)
    {
        if (!debugStream)
            return;
        char buf[1024];
        va_list args;
        va_start(args, format);
        vsprintf(buf, format, args);
        va_end(args);
        time_t t1 = time(NULL);
        struct tm* t2 = localtime(&t1);
        fprintf(debugStream, "%04d.%02d.%02d %02d:%02d:%02d %s\n",
                t2->tm_year + 1900, t2->tm_mon, t2->tm_mday, t2->tm_hour,
                t2->tm_min, t2->tm_sec, buf);
    }

  private: // Internal functions
    inline unsigned short getAF(RegisterPair* pair)
    {
        unsigned short result = pair->A;
        result <<= 8;
        result |= pair->F;
        return result;
    }

    inline unsigned short getBC(RegisterPair* pair)
    {
        unsigned short result = pair->B;
        result <<= 8;
        result |= pair->C;
        return result;
    }

    inline unsigned short getDE(RegisterPair* pair)
    {
        unsigned short result = pair->D;
        result <<= 8;
        result |= pair->E;
        return result;
    }

    inline unsigned short getHL(RegisterPair* pair)
    {
        unsigned short result = pair->H;
        result <<= 8;
        result |= pair->L;
        return result;
    }

    static inline int NOP(Z80* ctx)
    {
        ctx->log("[%04X] NOP", ctx->reg.PC);
        ctx->reg.PC++;
        return 4;
    }

    static inline int HALT(Z80* ctx)
    {
        ctx->log("[%04X] HALT", ctx->reg.PC);
        ctx->reg.isHalt = true;
        ctx->reg.PC++;
        return 4;
    }

    static inline int DI(Z80* ctx)
    {
        ctx->log("[%04X] DI", ctx->reg.PC);
        ctx->reg.IFF = 1;
        ctx->reg.PC++;
        return 4;
    }

    static inline int EI(Z80* ctx)
    {
        ctx->log("[%04X] EI", ctx->reg.PC);
        ctx->reg.IFF = 1;
        ctx->reg.PC++;
        return 4;
    }

    static inline int IM(Z80* ctx)
    {
        unsigned char mode = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        switch (mode) {
            case 0b01000110:
                ctx->log("[%04X] IM 0", ctx->reg.PC);
                ctx->reg.interruptMode = 0;
                break;
            case 0b01010110:
                ctx->log("[%04X] IM 1", ctx->reg.PC);
                ctx->reg.interruptMode = 1;
                break;
            case 0b01011110:
                ctx->log("[%04X] IM 2", ctx->reg.PC);
                ctx->reg.interruptMode = 2;
                break;
            default:
                ctx->log("unknown IM: $%02X", mode);
                return -1;
        }
        ctx->reg.PC += 2;
        return 8;
    }

    inline unsigned char* getRegisterPointer(unsigned char r)
    {
        switch (r) {
            case 0b111: return &reg.pair.A;
            case 0b000: return &reg.pair.B;
            case 0b001: return &reg.pair.C;
            case 0b010: return &reg.pair.D;
            case 0b011: return &reg.pair.E;
            case 0b100: return &reg.pair.H;
            case 0b101: return &reg.pair.L;
        }
        log("detected an unknown register number: $%02X", r);
        return NULL;
    }

    inline char* registerDump(unsigned char r)
    {
        static char A[16];
        static char B[16];
        static char C[16];
        static char D[16];
        static char E[16];
        static char H[16];
        static char L[16];
        static char unknown[2] = "?";
        switch (r) {
            case 0b111: sprintf(A, "A<$%02X>", reg.pair.A); return A;
            case 0b000: sprintf(B, "B<$%02X>", reg.pair.B); return B;
            case 0b001: sprintf(C, "C<$%02X>", reg.pair.C); return C;
            case 0b010: sprintf(D, "D<$%02X>", reg.pair.D); return D;
            case 0b011: sprintf(E, "E<$%02X>", reg.pair.E); return E;
            case 0b100: sprintf(H, "H<$%02X>", reg.pair.H); return H;
            case 0b101: sprintf(L, "L<$%02X>", reg.pair.L); return L;
            default: return unknown;
        }
    }

    inline char* registerPairDump(unsigned char ptn)
    {
        static char BC[16];
        static char DE[16];
        static char HL[16];
        static char SP[16];
        static char unknown[2] = "?";
        switch (ptn & 0b11) {
            case 0b00: sprintf(BC, "BC<$%02X%02X>", reg.pair.B, reg.pair.C); return BC;
            case 0b01: sprintf(DE, "DE<$%02X%02X>", reg.pair.D, reg.pair.E); return DE;
            case 0b10: sprintf(HL, "HL<$%02X%02X>", reg.pair.H, reg.pair.L); return HL;
            case 0b11: sprintf(SP, "SP<$%04X>", reg.SP); return SP;
            default: return unknown;
        }
    }

    // Load Reg. r1 with Reg. r2
    inline int LD_R1_R2(unsigned char r1, unsigned char r2)
    {
        unsigned char* r1p = getRegisterPointer(r1);
        unsigned char* r2p = getRegisterPointer(r2);
        if (debugStream) {
            log("[%04X] LD %s, %s", reg.PC, registerDump(r1), registerDump(r2));
        }
        if (r1p && r2p) *r1p = *r2p;
        reg.PC += 1;
        return 4;
    }

    // Load Reg. r with value n
    inline int LD_R_N(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char n = CB.read(CB.arg, reg.PC + 1);
        if (debugStream) {
            log("[%04X] LD %s, $%02X", reg.PC, registerDump(r), n);
        }
        if (rp) *rp = n;
        reg.PC += 2;
        return 7;
    }

    // Load Reg. r with location (HL)
    inline int LD_R_HL(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char n = CB.read(CB.arg, getHL(&reg.pair));
        if (debugStream) {
            log("[%04X] LD %s, (%s) = $%02X", reg.PC, registerDump(r), registerPairDump(0b10), n);
        }
        if (rp) *rp = n;
        reg.PC += 1;
        return 7;
    }

    int (*opSet1[256])(Z80* ctx);

  public: // API functions
    Z80(std::function<unsigned char(void* arg, unsigned short addr)> read,
        std::function<void(void* arg, unsigned short addr, unsigned char value)> write,
        std::function<unsigned char(void* arg, unsigned char port)> in,
        std::function<void(void* arg, unsigned char port, unsigned char value)> out,
        void* arg,
        FILE* debugStream = NULL)
    {
        this->CB.read = read;
        this->CB.write = write;
        this->CB.in = in;
        this->CB.out = out;
        this->CB.arg = arg;
        this->debugStream = debugStream;
        ::memset(&reg, 0, sizeof(reg));
        ::memset(&opSet1, 0, sizeof(opSet1));
        opSet1[0b00000000] = NOP;
        opSet1[0b01110110] = HALT;
        opSet1[0b11110011] = DI;
        opSet1[0b11111011] = EI;
        opSet1[0b11101101] = IM;
    }

    ~Z80() {}

    bool execute(int clock)
    {
        while (0 < clock && !reg.isHalt) {
            int operandNumber = CB.read(CB.arg, reg.PC);
            int (*op)(Z80*) = opSet1[operandNumber];
            int consume = -1;
            if (NULL == op) {
                if ((operandNumber & 0b11000111) == 0b00000110) {
                    consume = LD_R_N((operandNumber & 0b00111000) >> 3);
                } else if ((operandNumber & 0b11000111) == 0b01000110) {
                    consume = LD_R_HL((operandNumber & 0b00111000) >> 3);
                } else if ((operandNumber & 0b11000000) == 0b01000000) {
                    consume = LD_R1_R2((operandNumber & 0b00111000) >> 3, operandNumber & 0b00000111);
                }
            } else {
                consume = op(this);
            }
            if (consume < 0) {
                log("detected an invalid operand: $%02X", operandNumber);
                return false;
            }
            clock -= consume;
        }
        return true;
    }

    void executeTick4MHz() { execute(4194304 / 60); }

    void executeTick8MHz() { execute(8388608 / 60); }

    void registerDump()
    {
        log("===== REGISTER DUMP : START =====");
        log("%s %s %s %s %s %s %s", registerDump(0b111), registerDump(0b000), registerDump(0b001), registerDump(0b010), registerDump(0b011), registerDump(0b100), registerDump(0b101));
        log("PC<$%04X> SP<$%04X> IX<$%04X> IY<$%04X>", reg.PC, reg.SP, reg.IX, reg.IY);
        log("===== REGISTER DUMP : END =====");
    }
};

#endif // INCLUDE_Z80_HPP