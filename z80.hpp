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

    // flag setter
    inline void setFlagS(bool on) { on ? reg.pair.F |= 0b10000000 : reg.pair.F &= 0b01111111; }
    inline void setFlagZ(bool on) { on ? reg.pair.F |= 0b01000000 : reg.pair.F &= 0b10111111; }
    inline void setFlagH(bool on) { on ? reg.pair.F |= 0b00010000 : reg.pair.F &= 0b11101111; }
    inline void setFlagPV(bool on) { on ? reg.pair.F |= 0b00000100 : reg.pair.F &= 0b11111011; }
    inline void setFlagN(bool on) { on ? reg.pair.F |= 0b00000010 : reg.pair.F &= 0b11111101; }
    inline void setFlagC(bool on) { on ? reg.pair.F |= 0b00000001 : reg.pair.F &= 0b11111110; }

    // flag checker
    inline bool isFlagS() { return reg.pair.F & 0b10000000; }
    inline bool isFlagZ() { return reg.pair.F & 0b01000000; }
    inline bool isFlagH() { return reg.pair.F & 0b00010000; }
    inline bool isFlagPV() { return reg.pair.F & 0b00000100; }
    inline bool isFlagN() { return reg.pair.F & 0b00000010; }
    inline bool isFlagC() { return reg.pair.F & 0b00000001; }

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
        if (!debugStream) {
            return;
        }
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
        ctx->reg.IFF = 0;
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
                ctx->reg.PC += 2;
                return 8;
            case 0b01010110:
                ctx->log("[%04X] IM 1", ctx->reg.PC);
                ctx->reg.interruptMode = 1;
                ctx->reg.PC += 2;
                return 8;
            case 0b01011110:
                ctx->log("[%04X] IM 2", ctx->reg.PC);
                ctx->reg.interruptMode = 2;
                ctx->reg.PC += 2;
                return 8;
            case 0b01010111:
                ctx->log("[%04X] LD A<$%02X>, I<$%02X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.I);
                ctx->reg.pair.A = ctx->reg.I;
                ctx->setFlagPV(ctx->reg.IFF ? true : false);
                ctx->reg.PC += 2;
                return 9;
            case 0b01000111:
                ctx->log("[%04X] LD I<$%02X>, A<$%02X>", ctx->reg.PC, ctx->reg.I, ctx->reg.pair.A);
                ctx->reg.I = ctx->reg.pair.A;
                ctx->reg.PC += 2;
                return 9;
            case 0b01011111:
                ctx->log("[%04X] LD A<$%02X>, R<$%02X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.R);
                ctx->reg.pair.A = ctx->reg.R;
                ctx->setFlagPV(ctx->reg.IFF ? true : false);
                ctx->reg.PC += 2;
                return 9;
            case 0b01001111:
                ctx->log("[%04X] LD R<$%02X>, A<$%02X>", ctx->reg.PC, ctx->reg.R, ctx->reg.pair.A);
                ctx->reg.R = ctx->reg.pair.A;
                ctx->reg.PC += 2;
                return 9;
            default:
                ctx->log("unknown IM: $%02X", mode);
                return -1;
        }
    }

    // operand of using IX (first byte is 0b11011101)
    static inline int OP_IX(Z80* ctx)
    {
        const char op2 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        if (op2 == 0b00110110) {
            return ctx->LD_IX_N();
        } else if (op2 == 0b00100001) {
            return ctx->LD_IX_NN();
        } else if ((op2 & 0b11000111) == 0b01000110) {
            return ctx->LD_R_IX((op2 & 0b00111000) >> 3);
        } else if ((op2 & 0b11111000) == 0b01110000) {
            return ctx->LD_IX_R(op2 & 0b00000111);
        }
        ctx->log("detected an unknown operand: 0b11011101 - $%02X", op2);
        return -1;
    }

    // operand of using IY (first byte is 0b11111101)
    static inline int OP_IY(Z80* ctx)
    {
        const char op2 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        if (op2 == 0b00110110) {
            return ctx->LD_IY_N();
        } else if (op2 == 0b00100001) {
            return ctx->LD_IY_NN();
        } else if ((op2 & 0b11000111) == 0b01000110) {
            return ctx->LD_R_IY((op2 & 0b00111000) >> 3);
        } else if ((op2 & 0b11111000) == 0b01110000) {
            return ctx->LD_IY_R(op2 & 0b00000111);
        }
        ctx->log("detected an unknown operand: 11111101 - $%02X", op2);
        return -1;
    }

    // Load location (HL) with value n
    static inline int LD_HL_N(Z80* ctx)
    {
        unsigned char n = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        unsigned short hl = ctx->getHL(&ctx->reg.pair);
        ctx->log("[%04X] LD (HL<$%04X>), $%02X", ctx->reg.PC, hl, n);
        ctx->CB.write(ctx->CB.arg, hl, n);
        ctx->reg.PC += 2;
        return 10;
    }

    // Load Acc. wth location (BC)
    static inline int LD_A_BC(Z80* ctx)
    {
        unsigned short addr = ctx->getBC(&ctx->reg.pair);
        unsigned char n = ctx->CB.read(ctx->CB.arg, addr);
        ctx->log("[%04X] LD A, (BC<$%02X%02X>) = $%02X", ctx->reg.PC, ctx->reg.pair.B, ctx->reg.pair.C, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC++;
        return 7;
    }

    // Load Acc. wth location (DE)
    static inline int LD_A_DE(Z80* ctx)
    {
        unsigned short addr = ctx->getDE(&ctx->reg.pair);
        unsigned char n = ctx->CB.read(ctx->CB.arg, addr);
        ctx->log("[%04X] LD A, (DE<$%02X%02X>) = $%02X", ctx->reg.PC, ctx->reg.pair.D, ctx->reg.pair.E, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC++;
        return 7;
    }

    // Load Acc. wth location (nn)
    static inline int LD_A_NN(Z80* ctx)
    {
        unsigned short addr = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        addr += ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 2) << 8;
        unsigned char n = ctx->CB.read(ctx->CB.arg, addr);
        ctx->log("[%04X] LD A, ($%04X) = $%02X", ctx->reg.PC, addr, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC += 3;
        return 13;
    }

    // Load location (BC) wtih Acc.
    static inline int LD_BC_A(Z80* ctx)
    {
        unsigned short addr = ctx->getBC(&ctx->reg.pair);
        unsigned char n = ctx->reg.pair.A;
        ctx->log("[%04X] LD (BC<$%02X%02X>), A<$%02X>", ctx->reg.PC, ctx->reg.pair.B, ctx->reg.pair.C, n);
        ctx->CB.write(ctx->CB.arg, addr, n);
        ctx->reg.PC++;
        return 7;
    }

    // Load location (DE) wtih Acc.
    static inline int LD_DE_A(Z80* ctx)
    {
        unsigned short addr = ctx->getDE(&ctx->reg.pair);
        unsigned char n = ctx->reg.pair.A;
        ctx->log("[%04X] LD (DE<$%02X%02X>), A<$%02X>", ctx->reg.PC, ctx->reg.pair.D, ctx->reg.pair.E, n);
        ctx->CB.write(ctx->CB.arg, addr, n);
        ctx->reg.PC++;
        return 7;
    }

    // Load location (nn) with Acc.
    static inline int LD_NN_A(Z80* ctx)
    {
        unsigned short addr = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        addr += ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 2) << 8;
        unsigned char n = ctx->reg.pair.A;
        ctx->log("[%04X] LD ($%04X), A<$%02X>", ctx->reg.PC, addr, n);
        ctx->CB.write(ctx->CB.arg, addr, n);
        ctx->reg.PC += 3;
        return 13;
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

    // Load Reg. r with location (IX+d)
    inline int LD_R_IX(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, (reg.IX + d) & 0xFFFF);
        if (debugStream) {
            log("[%04X] LD %s, (IX<$%04X>+$%02X) = $%02X", reg.PC, registerDump(r), reg.IX, d, n);
        }
        if (rp) *rp = n;
        reg.PC += 3;
        return 19;
    }

    // Load Reg. r with location (IY+d)
    inline int LD_R_IY(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, (reg.IY + d) & 0xFFFF);
        if (debugStream) {
            log("[%04X] LD %s, (IY<$%04X>+$%02X) = $%02X", reg.PC, registerDump(r), reg.IY, d, n);
        }
        if (rp) *rp = n;
        reg.PC += 3;
        return 19;
    }

    // Load location (HL) with Reg. r
    inline int LD_HL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned short addr = getHL(&reg.pair);
        if (debugStream) {
            log("[%04X] LD (%s), %s", reg.PC, registerPairDump(0b10), registerDump(r));
        }
        if (rp) CB.write(CB.arg, addr, *rp);
        reg.PC += 1;
        return 7;
    }

    // 	Load location (IX+d) with Reg. r
    inline int LD_IX_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned short addr = reg.IX + d;
        if (debugStream) {
            log("[%04X] LD (IX<$%04X>+$%02X), %s", reg.PC, reg.IX, d, registerDump(r));
        }
        if (rp) CB.write(CB.arg, addr, *rp);
        reg.PC += 3;
        return 19;
    }

    // 	Load location (IY+d) with Reg. r
    inline int LD_IY_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned short addr = reg.IY + d;
        if (debugStream) {
            log("[%04X] LD (IY<$%04X>+$%02X), %s", reg.PC, reg.IY, d, registerDump(r));
        }
        if (rp) CB.write(CB.arg, addr, *rp);
        reg.PC += 3;
        return 19;
    }

    // Load location (IX+d) with value n
    inline int LD_IX_N()
    {
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = reg.IX + d;
        if (debugStream) {
            log("[%04X] LD (IX<$%04X>+$%02X), $%02X", reg.PC, reg.IX, d, n);
        }
        CB.write(CB.arg, addr, n);
        reg.PC += 4;
        return 19;
    }

    // Load location (IY+d) with value n
    inline int LD_IY_N()
    {
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = reg.IY + d;
        if (debugStream) {
            log("[%04X] LD (IY<$%04X>+$%02X), $%02X", reg.PC, reg.IY, d, n);
        }
        CB.write(CB.arg, addr, n);
        reg.PC += 4;
        return 19;
    }

    // Load Reg. pair rp with value nn.
    inline int LD_RP_NN(unsigned char rp)
    {
        unsigned char* rH;
        unsigned char* rL;
        switch (rp) {
            case 0b00:
                rH = &reg.pair.B;
                rL = &reg.pair.C;
                break;
            case 0b01:
                rH = &reg.pair.D;
                rL = &reg.pair.E;
                break;
            case 0b10:
                rH = &reg.pair.H;
                rL = &reg.pair.L;
                break;
            default:
                log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        unsigned char nL = CB.read(CB.arg, reg.PC + 1);
        unsigned char nH = CB.read(CB.arg, reg.PC + 2);
        if (debugStream) {
            log("[%04X] LD %s, $%02X%02X", reg.PC, registerPairDump(rp), nH, nL);
        }
        *rH = nH;
        *rL = nL;
        reg.PC += 3;
        return 10;
    }

    inline int LD_IX_NN()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        log("[%04X] LD IX, $%02X%02X", reg.PC, nH, nL);
        reg.IX = (nH << 8) + nL;
        reg.PC += 4;
        return 14;
    }

    inline int LD_IY_NN()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        log("[%04X] LD IY, $%02X%02X", reg.PC, nH, nL);
        reg.IY = (nH << 8) + nL;
        reg.PC += 4;
        return 14;
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
        // setup 1 byte operands
        opSet1[0b00000000] = NOP;
        opSet1[0b00000010] = LD_BC_A;
        opSet1[0b00001010] = LD_A_BC;
        opSet1[0b00010010] = LD_DE_A;
        opSet1[0b00011010] = LD_A_DE;
        opSet1[0b00110110] = LD_HL_N;
        opSet1[0b00110010] = LD_NN_A;
        opSet1[0b00111010] = LD_A_NN;
        opSet1[0b01110110] = HALT;
        opSet1[0b11011101] = OP_IX;
        opSet1[0b11101101] = IM;
        opSet1[0b11110011] = DI;
        opSet1[0b11111011] = EI;
        opSet1[0b11111101] = OP_IY;
    }

    ~Z80() {}

    int execute(int clock)
    {
        int executed = 0;
        while (0 < clock && !reg.isHalt) {
            int operandNumber = CB.read(CB.arg, reg.PC);
            int (*op)(Z80*) = opSet1[operandNumber];
            int consume = -1;
            if (NULL == op) {
                // execute an operand that register type has specified in the first byte.
                if ((operandNumber & 0b11111000) == 0b01110000) {
                    consume = LD_HL_R(operandNumber & 0b00000111);
                } else if ((operandNumber & 0b11000111) == 0b00000110) {
                    consume = LD_R_N((operandNumber & 0b00111000) >> 3);
                } else if ((operandNumber & 0b11001111) == 0b00000001) {
                    consume = LD_RP_NN((operandNumber & 0b00110000) >> 4);
                } else if ((operandNumber & 0b11000111) == 0b01000110) {
                    consume = LD_R_HL((operandNumber & 0b00111000) >> 3);
                } else if ((operandNumber & 0b11000000) == 0b01000000) {
                    consume = LD_R1_R2((operandNumber & 0b00111000) >> 3, operandNumber & 0b00000111);
                }
            } else {
                // execute an operand that the first byte is fixed.
                consume = op(this);
            }
            if (consume < 0) {
                log("detected an invalid operand: $%02X", operandNumber);
                return false;
            }
            clock -= consume;
            executed += consume;
        }
        return executed;
    }

    void executeTick4MHz() { execute(4194304 / 60); }

    void executeTick8MHz() { execute(8388608 / 60); }

    void registerDump()
    {
        log("===== REGISTER DUMP : START =====");
        log("%s %s %s %s %s %s %s", registerDump(0b111), registerDump(0b000), registerDump(0b001), registerDump(0b010), registerDump(0b011), registerDump(0b100), registerDump(0b101));
        log("PC<$%04X> SP<$%04X> IX<$%04X> IY<$%04X>", reg.PC, reg.SP, reg.IX, reg.IY);
        log("R<$%02X> I<$%02X> IFF<$%02X>", reg.R, reg.I, reg.IFF);
        log("isHalt: %s, interruptMode: %d", reg.isHalt ? "YES" : "NO", reg.interruptMode);
        log("===== REGISTER DUMP : END =====");
    }
};

#endif // INCLUDE_Z80_HPP