/**
 * SUZUKI PLAN - Z80 Emulator
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 * 
 * Copyright (c) 2019 Yoji Suzuki.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
#ifndef INCLUDE_Z80_HPP
#define INCLUDE_Z80_HPP
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <time.h>

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
        unsigned long consumeClockCounter;
        unsigned short PC;
        unsigned short SP;
        unsigned short IX;
        unsigned short IY;
        unsigned char R;
        unsigned char I;
        unsigned char IFF;
        unsigned char reserved;
        int interruptMode;
        bool isHalt;
    } reg;

  private: // Internal functions & variables
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

    struct Callback {
        unsigned char (*read)(void* arg, unsigned short addr);
        void (*write)(void* arg, unsigned short addr, unsigned char value);
        unsigned char (*in)(void* arg, unsigned char port);
        void (*out)(void* arg, unsigned char port, unsigned char value);
        void (*debugMessage)(void* arg, const char* message);
        void (*consumeClock)(void* arg, int clock);
        void (*breakPoint)(void* arg);
        void* arg;
        unsigned short breakPointAddress;
    } CB;

    inline void log(const char* format, ...)
    {
        if (!CB.debugMessage) {
            return;
        }
        char buf[1024];
        va_list args;
        va_start(args, format);
        vsprintf(buf, format, args);
        va_end(args);
        CB.debugMessage(CB.arg, buf);
    }

    inline unsigned short getAF()
    {
        unsigned short result = reg.pair.A;
        result <<= 8;
        result |= reg.pair.F;
        return result;
    }

    inline void setAF(unsigned short value)
    {
        reg.pair.A = (value & 0xFF00) >> 8;
        reg.pair.F = (value & 0x00FF);
    }

    inline unsigned short getAF2()
    {
        unsigned short result = reg.back.A;
        result <<= 8;
        result |= reg.back.F;
        return result;
    }

    inline void setAF2(unsigned short value)
    {
        reg.back.A = (value & 0xFF00) >> 8;
        reg.back.F = (value & 0x00FF);
    }

    inline unsigned short getBC()
    {
        unsigned short result = reg.pair.B;
        result <<= 8;
        result |= reg.pair.C;
        return result;
    }

    inline void setBC(unsigned short value)
    {
        reg.pair.B = (value & 0xFF00) >> 8;
        reg.pair.C = (value & 0x00FF);
    }

    inline unsigned short getBC2()
    {
        unsigned short result = reg.back.B;
        result <<= 8;
        result |= reg.back.C;
        return result;
    }

    inline void setBC2(unsigned short value)
    {
        reg.back.B = (value & 0xFF00) >> 8;
        reg.back.C = (value & 0x00FF);
    }

    inline unsigned short getDE()
    {
        unsigned short result = reg.pair.D;
        result <<= 8;
        result |= reg.pair.E;
        return result;
    }

    inline void setDE(unsigned short value)
    {
        reg.pair.D = (value & 0xFF00) >> 8;
        reg.pair.E = (value & 0x00FF);
    }

    inline unsigned short getDE2()
    {
        unsigned short result = reg.back.D;
        result <<= 8;
        result |= reg.back.E;
        return result;
    }

    inline void setDE2(unsigned short value)
    {
        reg.back.D = (value & 0xFF00) >> 8;
        reg.back.E = (value & 0x00FF);
    }

    inline unsigned short getHL()
    {
        unsigned short result = reg.pair.H;
        result <<= 8;
        result |= reg.pair.L;
        return result;
    }

    inline void setHL(unsigned short value)
    {
        reg.pair.H = (value & 0xFF00) >> 8;
        reg.pair.L = (value & 0x00FF);
    }

    inline unsigned short getHL2()
    {
        unsigned short result = reg.back.H;
        result <<= 8;
        result |= reg.back.L;
        return result;
    }

    inline void setHL2(unsigned short value)
    {
        reg.back.H = (value & 0xFF00) >> 8;
        reg.back.L = (value & 0x00FF);
    }

    inline bool isEvenNumberBits(unsigned char value)
    {
        int on = 0;
        int off = 0;
        value & 0b10000000 ? on++ : off++;
        value & 0b01000000 ? on++ : off++;
        value & 0b00100000 ? on++ : off++;
        value & 0b00010000 ? on++ : off++;
        value & 0b00001000 ? on++ : off++;
        value & 0b00000100 ? on++ : off++;
        value & 0b00000010 ? on++ : off++;
        value & 0b00000001 ? on++ : off++;
        return (on & 1) == 0;
    }

    inline int consumeClock(int hz)
    {
        reg.consumeClockCounter += hz;
        if (CB.consumeClock) CB.consumeClock(CB.arg, hz);
        return hz;
    }

    static inline int NOP(Z80* ctx)
    {
        ctx->log("[%04X] NOP", ctx->reg.PC);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int HALT(Z80* ctx)
    {
        ctx->log("[%04X] HALT", ctx->reg.PC);
        ctx->reg.isHalt = true;
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int DI(Z80* ctx)
    {
        ctx->log("[%04X] DI", ctx->reg.PC);
        ctx->reg.IFF = 0;
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int EI(Z80* ctx)
    {
        ctx->log("[%04X] EI", ctx->reg.PC);
        ctx->reg.IFF = 1;
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int IM(Z80* ctx)
    {
        unsigned char mode = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        switch (mode) {
            case 0b01000110:
                ctx->log("[%04X] IM 0", ctx->reg.PC);
                ctx->reg.interruptMode = 0;
                ctx->reg.PC += 2;
                return ctx->consumeClock(8);
            case 0b01010110:
                ctx->log("[%04X] IM 1", ctx->reg.PC);
                ctx->reg.interruptMode = 1;
                ctx->reg.PC += 2;
                return ctx->consumeClock(8);
            case 0b01011110:
                ctx->log("[%04X] IM 2", ctx->reg.PC);
                ctx->reg.interruptMode = 2;
                ctx->reg.PC += 2;
                return ctx->consumeClock(8);
            case 0b01010111:
                ctx->log("[%04X] LD A<$%02X>, I<$%02X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.I);
                ctx->reg.pair.A = ctx->reg.I;
                ctx->setFlagPV(ctx->reg.IFF ? true : false);
                ctx->reg.PC += 2;
                return ctx->consumeClock(9);
            case 0b01000111:
                ctx->log("[%04X] LD I<$%02X>, A<$%02X>", ctx->reg.PC, ctx->reg.I, ctx->reg.pair.A);
                ctx->reg.I = ctx->reg.pair.A;
                ctx->reg.PC += 2;
                return ctx->consumeClock(9);
            case 0b01011111:
                ctx->log("[%04X] LD A<$%02X>, R<$%02X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.R);
                ctx->reg.pair.A = ctx->reg.R;
                ctx->setFlagPV(ctx->reg.IFF ? true : false);
                ctx->reg.PC += 2;
                return ctx->consumeClock(9);
            case 0b01001111:
                ctx->log("[%04X] LD R<$%02X>, A<$%02X>", ctx->reg.PC, ctx->reg.R, ctx->reg.pair.A);
                ctx->reg.R = ctx->reg.pair.A;
                ctx->reg.PC += 2;
                return ctx->consumeClock(9);
            case 0b10100000:
                return ctx->LDI();
            case 0b10110000:
                return ctx->LDIR();
            case 0b10101000:
                return ctx->LDD();
            case 0b10111000:
                return ctx->LDDR();
            default:
                if ((mode & 0b11001111) == 0b01001011) {
                    return ctx->LD_RP_ADDR((mode & 0b00110000) >> 4);
                } else if ((mode & 0b11001111) == 0b01000011) {
                    return ctx->LD_ADDR_RP((mode & 0b00110000) >> 4);
                }
                ctx->log("unknown IM: $%02X", mode);
                return -1;
        }
    }

    // operand of using IX (first byte is 0b11011101)
    static inline int OP_IX(Z80* ctx)
    {
        unsigned char op2 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        if (op2 == 0b00110110) {
            return ctx->LD_IX_N();
        } else if (op2 == 0b00101010) {
            return ctx->LD_IX_ADDR();
        } else if (op2 == 0b00100010) {
            return ctx->LD_ADDR_IX();
        } else if (op2 == 0b00100001) {
            return ctx->LD_IX_NN();
        } else if (op2 == 0b11111001) {
            return ctx->LD_SP_IX();
        } else if (op2 == 0b11100011) {
            return ctx->EX_SP_IX();
        } else if (op2 == 0b11100101) {
            return ctx->PUSH_IX();
        } else if (op2 == 0b11100001) {
            return ctx->POP_IX();
        } else if (op2 == 0b11001011) {
            unsigned char op3 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 2);
            unsigned char op4 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 3);
            switch (op4) {
                case 0b00000110: return ctx->RLC_IX(op3);
                case 0b00001110: return ctx->RRC_IX(op3);
                case 0b00010110: return ctx->RL_IX(op3);
                case 0b00011110: return ctx->RR_IX(op3);
            }
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
        unsigned char op2 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        if (op2 == 0b00110110) {
            return ctx->LD_IY_N();
        } else if (op2 == 0b00101010) {
            return ctx->LD_IY_ADDR();
        } else if (op2 == 0b00100010) {
            return ctx->LD_ADDR_IY();
        } else if (op2 == 0b00100001) {
            return ctx->LD_IY_NN();
        } else if (op2 == 0b11111001) {
            return ctx->LD_SP_IY();
        } else if (op2 == 0b11100011) {
            return ctx->EX_SP_IY();
        } else if (op2 == 0b11100101) {
            return ctx->PUSH_IY();
        } else if (op2 == 0b11100001) {
            return ctx->POP_IY();
        } else if (op2 == 0b11001011) {
            unsigned char op3 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 2);
            unsigned char op4 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 3);
            switch (op4) {
                case 0b00000110: return ctx->RLC_IY(op3);
                case 0b00001110: return ctx->RRC_IY(op3);
                case 0b00010110: return ctx->RL_IY(op3);
                case 0b00011110: return ctx->RR_IY(op3);
            }
        } else if ((op2 & 0b11000111) == 0b01000110) {
            return ctx->LD_R_IY((op2 & 0b00111000) >> 3);
        } else if ((op2 & 0b11111000) == 0b01110000) {
            return ctx->LD_IY_R(op2 & 0b00000111);
        }
        ctx->log("detected an unknown operand: 11111101 - $%02X", op2);
        return -1;
    }

    // operand of using other register (first byte is 0b11001011)
    static inline int OP_R(Z80* ctx)
    {
        unsigned char op2 = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        if (op2 == 0b00000110) {
            return ctx->RLC_HL();
        } else if (op2 == 0b00010110) {
            return ctx->RL_HL();
        } else if (op2 == 0b00001110) {
            return ctx->RRC_HL();
        } else if (op2 == 0b00011110) {
            return ctx->RR_HL();
        } else if ((op2 & 0b11111000) == 0b00000000) {
            return ctx->RLC_R(op2 & 0b00000111);
        } else if ((op2 & 0b11111000) == 0b00010000) {
            return ctx->RL_R(op2 & 0b00000111);
        } else if ((op2 & 0b11111000) == 0b00001000) {
            return ctx->RRC_R(op2 & 0b00000111);
        } else if ((op2 & 0b11111000) == 0b00011000) {
            return ctx->RR_R(op2 & 0b00000111);
        }
        ctx->log("detected an unknown operand: 11001011 - $%02X", op2);
        return -1;
    }

    // Load location (HL) with value n
    static inline int LD_HL_N(Z80* ctx)
    {
        unsigned char n = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        unsigned short hl = ctx->getHL();
        ctx->log("[%04X] LD (HL<$%04X>), $%02X", ctx->reg.PC, hl, n);
        ctx->CB.write(ctx->CB.arg, hl, n);
        ctx->reg.PC += 2;
        return ctx->consumeClock(10);
    }

    // Load Acc. wth location (BC)
    static inline int LD_A_BC(Z80* ctx)
    {
        unsigned short addr = ctx->getBC();
        unsigned char n = ctx->CB.read(ctx->CB.arg, addr);
        ctx->log("[%04X] LD A, (BC<$%02X%02X>) = $%02X", ctx->reg.PC, ctx->reg.pair.B, ctx->reg.pair.C, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC++;
        return ctx->consumeClock(7);
    }

    // Load Acc. wth location (DE)
    static inline int LD_A_DE(Z80* ctx)
    {
        unsigned short addr = ctx->getDE();
        unsigned char n = ctx->CB.read(ctx->CB.arg, addr);
        ctx->log("[%04X] LD A, (DE<$%02X%02X>) = $%02X", ctx->reg.PC, ctx->reg.pair.D, ctx->reg.pair.E, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC++;
        return ctx->consumeClock(7);
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
        return ctx->consumeClock(13);
    }

    // Load location (BC) wtih Acc.
    static inline int LD_BC_A(Z80* ctx)
    {
        unsigned short addr = ctx->getBC();
        unsigned char n = ctx->reg.pair.A;
        ctx->log("[%04X] LD (BC<$%02X%02X>), A<$%02X>", ctx->reg.PC, ctx->reg.pair.B, ctx->reg.pair.C, n);
        ctx->CB.write(ctx->CB.arg, addr, n);
        ctx->reg.PC++;
        return ctx->consumeClock(7);
    }

    // Load location (DE) wtih Acc.
    static inline int LD_DE_A(Z80* ctx)
    {
        unsigned short addr = ctx->getDE();
        unsigned char n = ctx->reg.pair.A;
        ctx->log("[%04X] LD (DE<$%02X%02X>), A<$%02X>", ctx->reg.PC, ctx->reg.pair.D, ctx->reg.pair.E, n);
        ctx->CB.write(ctx->CB.arg, addr, n);
        ctx->reg.PC++;
        return ctx->consumeClock(7);
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
        return ctx->consumeClock(13);
    }

    // Load HL with location (nn).
    static inline int LD_HL_ADDR(Z80* ctx)
    {
        unsigned char nL = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        unsigned char nH = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 2);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = ctx->CB.read(ctx->CB.arg, addr);
        unsigned char h = ctx->CB.read(ctx->CB.arg, addr + 1);
        ctx->log("[%04X] LD HL<$%04X>, ($%04X) = $%02X%02X", ctx->reg.PC, ctx->getHL(), addr, h, l);
        ctx->reg.pair.L = l;
        ctx->reg.pair.H = h;
        ctx->reg.PC += 3;
        return ctx->consumeClock(16);
    }

    // Load location (nn) with HL.
    static inline int LD_ADDR_HL(Z80* ctx)
    {
        unsigned char nL = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 1);
        unsigned char nH = ctx->CB.read(ctx->CB.arg, ctx->reg.PC + 2);
        unsigned short addr = (nH << 8) + nL;
        ctx->log("[%04X] LD ($%04X), %s", ctx->reg.PC, addr, ctx->registerPairDump(0b10));
        ctx->CB.write(ctx->CB.arg, addr, ctx->reg.pair.L);
        ctx->CB.write(ctx->CB.arg, addr + 1, ctx->reg.pair.H);
        ctx->reg.PC += 3;
        return ctx->consumeClock(16);
    }

    // Load SP with HL.
    static inline int LD_SP_HL(Z80* ctx)
    {
        unsigned short value = ctx->getHL();
        ctx->log("[%04X] LD %s, HL<$%04X>", ctx->reg.PC, ctx->registerPairDump(0b11), value);
        ctx->reg.SP = value;
        ctx->reg.PC++;
        return ctx->consumeClock(6);
    }

    // Exchange H and L with D and E
    static inline int EX_DE_HL(Z80* ctx)
    {
        unsigned short de = ctx->getDE();
        unsigned short hl = ctx->getHL();
        ctx->log("[%04X] EX %s, %s", ctx->reg.PC, ctx->registerPairDump(0b01), ctx->registerPairDump(0b10));
        ctx->setDE(hl);
        ctx->setHL(de);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    // Exchange A and F with A' and F'
    static inline int EX_AF_AF2(Z80* ctx)
    {
        unsigned short af = ctx->getAF();
        unsigned short af2 = ctx->getAF2();
        ctx->log("[%04X] EX AF<$%02X%02X>, AF'<$%02X%02X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.pair.F, ctx->reg.back.A, ctx->reg.back.F);
        ctx->setAF(af2);
        ctx->setAF2(af);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int EX_SP_HL(Z80* ctx)
    {
        unsigned char l = ctx->CB.read(ctx->CB.arg, ctx->reg.SP);
        unsigned char h = ctx->CB.read(ctx->CB.arg, ctx->reg.SP + 1);
        unsigned short hl = ctx->getHL();
        ctx->log("[%04X] EX (SP<$%04X>) = $%02X%02X, HL<$%04X>", ctx->reg.PC, ctx->reg.SP, h, l, hl);
        ctx->CB.write(ctx->CB.arg, ctx->reg.SP, ctx->reg.pair.L);
        ctx->CB.write(ctx->CB.arg, ctx->reg.SP + 1, ctx->reg.pair.H);
        ctx->reg.pair.L = l;
        ctx->reg.pair.H = h;
        ctx->reg.PC++;
        return ctx->consumeClock(19);
    }

    static inline int EXX(Z80* ctx)
    {
        ctx->log("[%04X] EXX", ctx->reg.PC);
        unsigned short bc = ctx->getBC();
        unsigned short bc2 = ctx->getBC2();
        unsigned short de = ctx->getDE();
        unsigned short de2 = ctx->getDE2();
        unsigned short hl = ctx->getHL();
        unsigned short hl2 = ctx->getHL2();
        ctx->setBC(bc2);
        ctx->setBC2(bc);
        ctx->setDE(de2);
        ctx->setDE2(de);
        ctx->setHL(hl2);
        ctx->setHL2(hl);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int PUSH_AF(Z80* ctx)
    {
        ctx->log("[%04X] PUSH AF<$%02X%02X> <SP:$%04X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.pair.F, ctx->reg.SP);
        ctx->CB.write(ctx->CB.arg, --ctx->reg.SP, ctx->reg.pair.A);
        ctx->CB.write(ctx->CB.arg, --ctx->reg.SP, ctx->reg.pair.F);
        ctx->reg.PC++;
        return ctx->consumeClock(11);
    }

    static inline int POP_AF(Z80* ctx)
    {
        unsigned short sp = ctx->reg.SP;
        unsigned char l = ctx->CB.read(ctx->CB.arg, ctx->reg.SP++);
        unsigned char h = ctx->CB.read(ctx->CB.arg, ctx->reg.SP++);
        ctx->log("[%04X] POP AF <SP:$%04X> = $%02X%02X", ctx->reg.PC, sp, h, l);
        ctx->reg.pair.F = l;
        ctx->reg.pair.A = h;
        ctx->reg.PC++;
        return ctx->consumeClock(10);
    }

    static inline int RLCA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a7 = ctx->reg.pair.A & 0x80 ? 1 : 0;
        ctx->log("[%04X] RLCA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b01111111;
        ctx->reg.pair.A <<= 1;
        ctx->reg.pair.A |= a7; // differ with RLA
        ctx->setFlagC(a7 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int RRCA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a0 = ctx->reg.pair.A & 0x01;
        ctx->log("[%04X] RRCA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b11111110;
        ctx->reg.pair.A >>= 1;
        ctx->reg.pair.A |= a0 ? 0x80 : 0; // differ with RRA
        ctx->setFlagC(a0 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int RLA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a7 = ctx->reg.pair.A & 0x80 ? 1 : 0;
        ctx->log("[%04X] RLA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b01111111;
        ctx->reg.pair.A <<= 1;
        ctx->reg.pair.A |= c; // differ with RLCA
        ctx->setFlagC(a7 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
    }

    static inline int RRA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a0 = ctx->reg.pair.A & 0x01;
        ctx->log("[%04X] RRA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b11111110;
        ctx->reg.pair.A >>= 1;
        ctx->reg.pair.A |= c ? 0x80 : 0x00; // differ with RLCA
        ctx->setFlagC(a0 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->reg.PC++;
        return ctx->consumeClock(4);
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

    inline char* registerDump2(unsigned char r)
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
            case 0b111: sprintf(A, "A'<$%02X>", reg.back.A); return A;
            case 0b000: sprintf(B, "B'<$%02X>", reg.back.B); return B;
            case 0b001: sprintf(C, "C'<$%02X>", reg.back.C); return C;
            case 0b010: sprintf(D, "D'<$%02X>", reg.back.D); return D;
            case 0b011: sprintf(E, "E'<$%02X>", reg.back.E); return E;
            case 0b100: sprintf(H, "H'<$%02X>", reg.back.H); return H;
            case 0b101: sprintf(L, "L'<$%02X>", reg.back.L); return L;
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
        log("[%04X] LD %s, %s", reg.PC, registerDump(r1), registerDump(r2));
        if (r1p && r2p) *r1p = *r2p;
        reg.PC += 1;
        return consumeClock(4);
    }

    // Load Reg. r with value n
    inline int LD_R_N(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char n = CB.read(CB.arg, reg.PC + 1);
        log("[%04X] LD %s, $%02X", reg.PC, registerDump(r), n);
        if (rp) *rp = n;
        reg.PC += 2;
        return consumeClock(7);
    }

    // Load Reg. r with location (HL)
    inline int LD_R_HL(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char n = CB.read(CB.arg, getHL());
        log("[%04X] LD %s, (%s) = $%02X", reg.PC, registerDump(r), registerPairDump(0b10), n);
        if (rp) *rp = n;
        reg.PC += 1;
        return consumeClock(7);
    }

    // Load Reg. r with location (IX+d)
    inline int LD_R_IX(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, (reg.IX + d) & 0xFFFF);
        log("[%04X] LD %s, (IX<$%04X>+$%02X) = $%02X", reg.PC, registerDump(r), reg.IX, d, n);
        if (rp) *rp = n;
        reg.PC += 3;
        return consumeClock(19);
    }

    // Load Reg. r with location (IY+d)
    inline int LD_R_IY(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, (reg.IY + d) & 0xFFFF);
        log("[%04X] LD %s, (IY<$%04X>+$%02X) = $%02X", reg.PC, registerDump(r), reg.IY, d, n);
        if (rp) *rp = n;
        reg.PC += 3;
        return consumeClock(19);
    }

    // Load location (HL) with Reg. r
    inline int LD_HL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned short addr = getHL();
        log("[%04X] LD (%s), %s", reg.PC, registerPairDump(0b10), registerDump(r));
        if (rp) CB.write(CB.arg, addr, *rp);
        reg.PC += 1;
        return consumeClock(7);
    }

    // 	Load location (IX+d) with Reg. r
    inline int LD_IX_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned short addr = reg.IX + d;
        log("[%04X] LD (IX<$%04X>+$%02X), %s", reg.PC, reg.IX, d, registerDump(r));
        if (rp) CB.write(CB.arg, addr, *rp);
        reg.PC += 3;
        return consumeClock(19);
    }

    // 	Load location (IY+d) with Reg. r
    inline int LD_IY_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned short addr = reg.IY + d;
        log("[%04X] LD (IY<$%04X>+$%02X), %s", reg.PC, reg.IY, d, registerDump(r));
        if (rp) CB.write(CB.arg, addr, *rp);
        reg.PC += 3;
        return consumeClock(19);
    }

    // Load location (IX+d) with value n
    inline int LD_IX_N()
    {
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = reg.IX + d;
        log("[%04X] LD (IX<$%04X>+$%02X), $%02X", reg.PC, reg.IX, d, n);
        CB.write(CB.arg, addr, n);
        reg.PC += 4;
        return consumeClock(19);
    }

    // Load location (IY+d) with value n
    inline int LD_IY_N()
    {
        unsigned char d = CB.read(CB.arg, reg.PC + 2);
        unsigned char n = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = reg.IY + d;
        log("[%04X] LD (IY<$%04X>+$%02X), $%02X", reg.PC, reg.IY, d, n);
        CB.write(CB.arg, addr, n);
        reg.PC += 4;
        return consumeClock(19);
    }

    // Load Reg. pair rp with value nn.
    inline int LD_RP_NN(unsigned char rp)
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 1);
        unsigned char nH = CB.read(CB.arg, reg.PC + 2);
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
            case 0b11:
                // SP is not managed in pair structure, so calculate directly
                log("[%04X] LD SP<$%04X>, $%02X%02X", reg.PC, reg.SP, nH, nL);
                reg.SP = (nH << 8) + nL;
                reg.PC += 3;
                return consumeClock(10);
            default:
                log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        log("[%04X] LD %s, $%02X%02X", reg.PC, registerPairDump(rp), nH, nL);
        *rH = nH;
        *rL = nL;
        reg.PC += 3;
        return consumeClock(10);
    }

    inline int LD_IX_NN()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        log("[%04X] LD IX, $%02X%02X", reg.PC, nH, nL);
        reg.IX = (nH << 8) + nL;
        reg.PC += 4;
        return consumeClock(14);
    }

    inline int LD_IY_NN()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        log("[%04X] LD IY, $%02X%02X", reg.PC, nH, nL);
        reg.IY = (nH << 8) + nL;
        reg.PC += 4;
        return consumeClock(14);
    }

    // Load Reg. pair rp with location (nn)
    inline int LD_RP_ADDR(unsigned char rp)
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = CB.read(CB.arg, addr);
        unsigned char h = CB.read(CB.arg, addr + 1);
        log("[%04X] LD %s, ($%02X%02X) = $%02X%02X", reg.PC, registerPairDump(rp), nH, nL, h, l);
        switch (rp) {
            case 0b00:
                reg.pair.B = h;
                reg.pair.C = l;
                break;
            case 0b01:
                reg.pair.D = h;
                reg.pair.E = l;
                break;
            case 0b10:
                reg.pair.H = h;
                reg.pair.L = l;
                break;
            case 0b11:
                reg.SP = (h << 8) + l;
                break;
            default:
                log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        reg.PC += 4;
        return consumeClock(20);
    }

    // Load location (nn) with Reg. pair rp.
    inline int LD_ADDR_RP(unsigned char rp)
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = (nH << 8) + nL;
        log("[%04X] LD ($%04X), %s", reg.PC, addr, registerPairDump(rp));
        unsigned char l;
        unsigned char h;
        switch (rp) {
            case 0b00:
                h = reg.pair.B;
                l = reg.pair.C;
                break;
            case 0b01:
                h = reg.pair.D;
                l = reg.pair.E;
                break;
            case 0b10:
                h = reg.pair.H;
                l = reg.pair.L;
                break;
            case 0b11:
                h = (reg.SP & 0xFF00) >> 8;
                l = reg.SP & 0x00FF;
                break;
            default:
                log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        CB.write(CB.arg, addr, l);
        CB.write(CB.arg, addr + 1, h);
        reg.PC += 4;
        return consumeClock(20);
    }

    // Load IX with location (nn)
    inline int LD_IX_ADDR()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = CB.read(CB.arg, addr);
        unsigned char h = CB.read(CB.arg, addr + 1);
        log("[%04X] LD IX<$%04X>, ($%02X%02X) = $%02X%02X", reg.PC, reg.IX, nH, nL, h, l);
        reg.IX = (h << 8) + l;
        reg.PC += 4;
        return consumeClock(20);
    }

    // Load IY with location (nn)
    inline int LD_IY_ADDR()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = CB.read(CB.arg, addr);
        unsigned char h = CB.read(CB.arg, addr + 1);
        log("[%04X] LD IY<$%04X>, ($%02X%02X) = $%02X%02X", reg.PC, reg.IY, nH, nL, h, l);
        reg.IY = (h << 8) + l;
        reg.PC += 4;
        return consumeClock(20);
    }

    inline int LD_ADDR_IX()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = (nH << 8) + nL;
        log("[%04X] LD ($%04X), IX<$%04X>", reg.PC, addr, reg.IX);
        unsigned char l = reg.IX & 0x00FF;
        unsigned char h = (reg.IX & 0xFF00) >> 8;
        CB.write(CB.arg, addr, l);
        CB.write(CB.arg, addr + 1, h);
        reg.PC += 4;
        return consumeClock(20);
    }

    inline int LD_ADDR_IY()
    {
        unsigned char nL = CB.read(CB.arg, reg.PC + 2);
        unsigned char nH = CB.read(CB.arg, reg.PC + 3);
        unsigned short addr = (nH << 8) + nL;
        log("[%04X] LD ($%04X), IY<$%04X>", reg.PC, addr, reg.IY);
        unsigned char l = reg.IY & 0x00FF;
        unsigned char h = (reg.IY & 0xFF00) >> 8;
        CB.write(CB.arg, addr, l);
        CB.write(CB.arg, addr + 1, h);
        reg.PC += 4;
        return consumeClock(20);
    }

    // Load SP with IX.
    inline int LD_SP_IX()
    {
        unsigned short value = reg.IX;
        log("[%04X] LD %s, IX<$%04X>", reg.PC, registerPairDump(0b11), value);
        reg.SP = value;
        reg.PC += 2;
        return consumeClock(10);
    }

    // Load SP with IY.
    inline int LD_SP_IY()
    {
        unsigned short value = reg.IY;
        log("[%04X] LD %s, IY<$%04X>", reg.PC, registerPairDump(0b11), value);
        reg.SP = value;
        reg.PC += 2;
        return consumeClock(10);
    }

    // Load location (DE) with Loacation (HL), increment DE, HL, decrement BC
    inline int LDI()
    {
        log("[%04X] LDI ... %s, %s, %s", reg.PC, registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
        unsigned short bc = getBC();
        unsigned short de = getDE();
        unsigned short hl = getHL();
        unsigned char n = CB.read(CB.arg, hl);
        CB.write(CB.arg, de, n);
        de++;
        hl++;
        bc--;
        setBC(bc);
        setDE(de);
        setHL(hl);
        setFlagH(true);
        setFlagPV(bc != 1);
        setFlagN(false);
        reg.PC += 2;
        return consumeClock(16);
    }

    // Load location (DE) with Loacation (HL), increment DE, HL, decrement BC and repeat until BC=0.
    inline int LDIR()
    {
        int clocks = 0;
        log("[%04X] LDIR ... %s, %s, %s", reg.PC, registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
        unsigned short bc = getBC();
        unsigned short de = getDE();
        unsigned short hl = getHL();
        do {
            unsigned char n = CB.read(CB.arg, hl);
            CB.write(CB.arg, de, n);
            de++;
            hl++;
            bc--;
            clocks += 21;
            consumeClock(0 != bc ? 21 : 16);
        } while (0 != bc);
        setBC(bc);
        setDE(de);
        setHL(hl);
        setFlagH(false);
        setFlagPV(false);
        setFlagN(false);
        reg.PC += 2;
        return clocks - 5;
    }

    // Load location (DE) with Loacation (HL), decrement DE, HL, decrement BC
    inline int LDD()
    {
        log("[%04X] LDD ... %s, %s, %s", reg.PC, registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
        unsigned short bc = getBC();
        unsigned short de = getDE();
        unsigned short hl = getHL();
        unsigned char n = CB.read(CB.arg, hl);
        CB.write(CB.arg, de, n);
        de--;
        hl--;
        bc--;
        setBC(bc);
        setDE(de);
        setHL(hl);
        setFlagH(true);
        setFlagPV(bc != 1);
        setFlagN(false);
        reg.PC += 2;
        return consumeClock(16);
    }

    // Load location (DE) with Loacation (HL), decrement DE, HL, decrement BC and repeat until BC=0.
    inline int LDDR()
    {
        int clocks = 0;
        log("[%04X] LDDR ... %s, %s, %s", reg.PC, registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
        unsigned short bc = getBC();
        unsigned short de = getDE();
        unsigned short hl = getHL();
        do {
            unsigned char n = CB.read(CB.arg, hl);
            CB.write(CB.arg, de, n);
            de--;
            hl--;
            bc--;
            clocks += 21;
            consumeClock(0 != bc ? 21 : 16);
        } while (0 != bc);
        setBC(bc);
        setDE(de);
        setHL(hl);
        setFlagH(false);
        setFlagPV(false);
        setFlagN(false);
        reg.PC += 2;
        return clocks - 5;
    }

    // Exchange stack top with IX
    inline int EX_SP_IX()
    {
        unsigned char l = CB.read(CB.arg, reg.SP);
        unsigned char h = CB.read(CB.arg, reg.SP + 1);
        unsigned char i = (reg.IX & 0xFF00) >> 8;
        unsigned char x = reg.IX & 0x00FF;
        log("[%04X] EX (SP<$%04X>) = $%02X%02X, IX<$%04X>", reg.PC, reg.SP, h, l, reg.IX);
        CB.write(CB.arg, reg.SP, x);
        CB.write(CB.arg, reg.SP + 1, i);
        reg.IX = (h << 8) + l;
        reg.PC += 2;
        return consumeClock(23);
    }

    // Exchange stack top with IY
    inline int EX_SP_IY()
    {
        unsigned char l = CB.read(CB.arg, reg.SP);
        unsigned char h = CB.read(CB.arg, reg.SP + 1);
        unsigned char i = (reg.IY & 0xFF00) >> 8;
        unsigned char y = reg.IY & 0x00FF;
        log("[%04X] EX (SP<$%04X>) = $%02X%02X, IY<$%04X>", reg.PC, reg.SP, h, l, reg.IY);
        CB.write(CB.arg, reg.SP, y);
        CB.write(CB.arg, reg.SP + 1, i);
        reg.IY = (h << 8) + l;
        reg.PC += 2;
        return consumeClock(23);
    }

    // Push Reg. on Stack.
    inline int PUSH_RP(unsigned char rp)
    {
        log("[%04X] PUSH %s <SP:$%04X>", reg.PC, registerPairDump(rp), reg.SP);
        unsigned char l;
        unsigned char h;
        switch (rp) {
            case 0b00:
                h = reg.pair.B;
                l = reg.pair.C;
                break;
            case 0b01:
                h = reg.pair.D;
                l = reg.pair.E;
                break;
            case 0b10:
                h = reg.pair.H;
                l = reg.pair.L;
                break;
            default:
                log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        CB.write(CB.arg, --reg.SP, h);
        CB.write(CB.arg, --reg.SP, l);
        reg.PC++;
        return consumeClock(11);
    }

    // Push Reg. on Stack.
    inline int POP_RP(unsigned char rp)
    {
        unsigned short sp = reg.SP;
        unsigned char* l;
        unsigned char* h;
        switch (rp) {
            case 0b00:
                h = &reg.pair.B;
                l = &reg.pair.C;
                break;
            case 0b01:
                h = &reg.pair.D;
                l = &reg.pair.E;
                break;
            case 0b10:
                h = &reg.pair.H;
                l = &reg.pair.L;
                break;
            default:
                log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        log("[%04X] POP %s <SP:$%04X> = $%02X%02X", reg.PC, registerPairDump(rp), sp, *h, *l);
        *l = CB.read(CB.arg, reg.SP++);
        *h = CB.read(CB.arg, reg.SP++);
        reg.PC++;
        return consumeClock(10);
    }

    // Push Reg. IX on Stack.
    inline int PUSH_IX()
    {
        log("[%04X] PUSH IX<$%04X> <SP:$%04X>", reg.PC, reg.IX, reg.SP);
        unsigned char h = (reg.IX & 0xFF00) >> 8;
        unsigned char l = reg.IX & 0x00FF;
        CB.write(CB.arg, --reg.SP, h);
        CB.write(CB.arg, --reg.SP, l);
        reg.PC += 2;
        return consumeClock(15);
    }

    // Pop Reg. IX from Stack.
    inline int POP_IX()
    {
        unsigned short sp = reg.SP;
        unsigned char l = CB.read(CB.arg, reg.SP++);
        unsigned char h = CB.read(CB.arg, reg.SP++);
        log("[%04X] POP IX <SP:$%04X> = $%02X%02X", reg.PC, sp, h, l);
        reg.IX = (h << 8) + l;
        reg.PC += 2;
        return consumeClock(14);
    }

    // Push Reg. IY on Stack.
    inline int PUSH_IY()
    {
        log("[%04X] PUSH IY<$%04X> <SP:$%04X>", reg.PC, reg.IY, reg.SP);
        unsigned char h = (reg.IY & 0xFF00) >> 8;
        unsigned char l = reg.IY & 0x00FF;
        CB.write(CB.arg, --reg.SP, h);
        CB.write(CB.arg, --reg.SP, l);
        reg.PC += 2;
        return consumeClock(15);
    }

    // Pop Reg. IY from Stack.
    inline int POP_IY()
    {
        unsigned short sp = reg.SP;
        unsigned char l = CB.read(CB.arg, reg.SP++);
        unsigned char h = CB.read(CB.arg, reg.SP++);
        log("[%04X] POP IY <SP:$%04X> = $%02X%02X", reg.PC, sp, h, l);
        reg.IY = (h << 8) + l;
        reg.PC += 2;
        return consumeClock(14);
    }

    // Rotate register Left Circular
    inline int RLC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r7 = *rp & 0x80 ? 1 : 0;
        log("[%04X] RLC %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b01111111;
        *rp <<= 1;
        *rp |= r7; // differ with RL
        setFlagC(r7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return consumeClock(4);
    }

    // Rotate Left register
    inline int RL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r7 = *rp & 0x80 ? 1 : 0;
        log("[%04X] RL %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b01111111;
        *rp <<= 1;
        *rp |= c; // differ with RLC
        setFlagC(r7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return consumeClock(8);
    }

    // Rotate register Right Circular
    inline int RRC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r0 = *rp & 0x01;
        log("[%04X] RRC %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b11111110;
        *rp >>= 1;
        *rp |= r0 ? 0x80 : 0; // differ with RR
        setFlagC(r0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return consumeClock(4);
    }

    // Rotate Right register
    inline int RR_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r0 = *rp & 0x01;
        log("[%04X] RR %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b11111110;
        *rp >>= 1;
        *rp |= c ? 0x80 : 0; // differ with RRC
        setFlagC(r0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return consumeClock(8);
    }

    // Rotate memory (HL) Left Circular
    inline int RLC_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        log("[%04X] RLC (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= n7; // differ with RL (HL)
        CB.write(CB.arg, addr, n);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return consumeClock(15);
    }

    // Rotate Left memory
    inline int RL_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        log("[%04X] RL (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= c; // differ with RLC (HL)
        CB.write(CB.arg, addr, n);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return consumeClock(15);
    }

    // Rotate memory (HL) Right Circular
    inline int RRC_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        log("[%04X] RRC (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= n0 ? 0x80 : 0; // differ with RR (HL)
        CB.write(CB.arg, addr, n);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return consumeClock(15);
    }

    // Rotate Right memory
    inline int RR_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        log("[%04X] RR (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= c ? 0x80 : 0; // differ with RRC (HL)
        CB.write(CB.arg, addr, n);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return consumeClock(15);
    }

    // Rotate memory (IX+d) Left Circular
    inline int RLC_IX(unsigned char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        log("[%04X] RLC (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= n7; // differ with RL (IX+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return consumeClock(23);
    }

    // Rotate memory (IX+d) Right Circular
    inline int RRC_IX(unsigned char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        log("[%04X] RRC (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= n0 ? 0x80 : 0; // differ with RR (IX+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return consumeClock(23);
    }

    // Rotate Left memory
    inline int RL_IX(unsigned char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        log("[%04X] RL (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= c; // differ with RLC (IX+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return consumeClock(23);
    }

    // Rotate Right memory
    inline int RR_IX(unsigned char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        log("[%04X] RR (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= c ? 0x80 : 0; // differ with RRC (IX+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return consumeClock(23);
    }

    // Rotate memory (IY+d) Left Circular
    inline int RLC_IY(unsigned char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        log("[%04X] RLC (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= n7; // differ with RL (IX+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return consumeClock(23);
    }

    // Rotate memory (IY+d) Right Circular
    inline int RRC_IY(unsigned char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        log("[%04X] RRC (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= n0 ? 0x80 : 0; // differ with RR (IX+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return consumeClock(23);
    }

    // Rotate Left memory
    inline int RL_IY(unsigned char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        log("[%04X] RL (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= c; // differ with RLC (IY+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return consumeClock(23);
    }

    // Rotate Right memory
    inline int RR_IY(unsigned char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = CB.read(CB.arg, addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        log("[%04X] RL (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= c ? 0x80 : 0; // differ with RRC (IY+d)
        CB.write(CB.arg, addr, n);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return consumeClock(23);
    }

    int (*opSet1[256])(Z80* ctx);

    // setup the operands or operand groups that detectable in fixed single byte
    void setupOpSet1()
    {
        ::memset(&opSet1, 0, sizeof(opSet1));
        opSet1[0b00000000] = NOP;
        opSet1[0b00000010] = LD_BC_A;
        opSet1[0b00000111] = RLCA;
        opSet1[0b00001000] = EX_AF_AF2;
        opSet1[0b00001010] = LD_A_BC;
        opSet1[0b00001111] = RRCA;
        opSet1[0b00010010] = LD_DE_A;
        opSet1[0b00010111] = RLA;
        opSet1[0b00011010] = LD_A_DE;
        opSet1[0b00011111] = RRA;
        opSet1[0b00100010] = LD_ADDR_HL;
        opSet1[0b00101010] = LD_HL_ADDR;
        opSet1[0b00110110] = LD_HL_N;
        opSet1[0b00110010] = LD_NN_A;
        opSet1[0b00111010] = LD_A_NN;
        opSet1[0b01110110] = HALT;
        opSet1[0b11001011] = OP_R;
        opSet1[0b11011001] = EXX;
        opSet1[0b11011101] = OP_IX;
        opSet1[0b11100011] = EX_SP_HL;
        opSet1[0b11101011] = EX_DE_HL;
        opSet1[0b11101101] = IM;
        opSet1[0b11110001] = POP_AF;
        opSet1[0b11110011] = DI;
        opSet1[0b11110101] = PUSH_AF;
        opSet1[0b11111001] = LD_SP_HL;
        opSet1[0b11111011] = EI;
        opSet1[0b11111101] = OP_IY;
    }

  public: // API functions
    Z80(unsigned char (*read)(void* arg, unsigned short addr),
        void (*write)(void* arg, unsigned short addr, unsigned char value),
        unsigned char (*in)(void* arg, unsigned char port),
        void (*out)(void* arg, unsigned char port, unsigned char value),
        void* arg)
    {
        this->CB.read = read;
        this->CB.write = write;
        this->CB.in = in;
        this->CB.out = out;
        this->CB.arg = arg;
        ::memset(&reg, 0, sizeof(reg));
        setupOpSet1();
    }

    ~Z80() {}

    void setDebugMessage(void (*debugMessage)(void*, const char*))
    {
        CB.debugMessage = debugMessage;
    }

    void setBreakPoint(unsigned short addr, void (*breakPoint)(void*))
    {
        CB.breakPointAddress = addr;
        CB.breakPoint = breakPoint;
    }

    void setConsumeClockCallback(void (*consumeClock)(void*, int))
    {
        CB.consumeClock = consumeClock;
    }

    int execute(int clock)
    {
        int executed = 0;
        while (0 < clock && !reg.isHalt) {
            if (CB.breakPoint) {
                if (CB.breakPointAddress == reg.PC) {
                    CB.breakPoint(CB.arg);
                }
            }
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
                } else if ((operandNumber & 0b11001111) == 0b11000101) {
                    consume = PUSH_RP((operandNumber & 0b00110000) >> 4);
                } else if ((operandNumber & 0b11001111) == 0b11000001) {
                    consume = POP_RP((operandNumber & 0b00110000) >> 4);
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
        log("PAIR: %s %s %s %s %s %s %s F<$%02X>", registerDump(0b111), registerDump(0b000), registerDump(0b001), registerDump(0b010), registerDump(0b011), registerDump(0b100), registerDump(0b101), reg.pair.F);
        log("BACK: %s %s %s %s %s %s %s F'<$%02X>", registerDump2(0b111), registerDump2(0b000), registerDump2(0b001), registerDump2(0b010), registerDump2(0b011), registerDump2(0b100), registerDump2(0b101), reg.back.F);
        log("PC<$%04X> SP<$%04X> IX<$%04X> IY<$%04X>", reg.PC, reg.SP, reg.IX, reg.IY);
        log("R<$%02X> I<$%02X> IFF<$%02X>", reg.R, reg.I, reg.IFF);
        log("isHalt: %s, interruptMode: %d", reg.isHalt ? "YES" : "NO", reg.interruptMode);
        log("===== REGISTER DUMP : END =====");
    }
};

#endif // INCLUDE_Z80_HPP