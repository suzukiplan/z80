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
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

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
        unsigned char interrupt;        // NI-- --mm (N: NMI, I: IRQ, mm: mode)
        unsigned short interruptVector; // interrupt vector for IRQ
        unsigned short interruptAddrN;  // interrupt address for NMI
        unsigned char consumeClockCounter;
        unsigned char execEI;
        unsigned char reserved1;
        unsigned char reserved2;
    } reg;

  private: // Internal functions & variables
    // flag setter
    inline void clearAllFlags() { reg.pair.F = 0; }
    inline void clearAllFlagsWithKeepCarry() { reg.pair.F &= 0b00000001; }
    inline void setFlagS(bool on) { on ? reg.pair.F |= 0b10000000 : reg.pair.F &= 0b01111111; }
    inline void setFlagZ(bool on) { on ? reg.pair.F |= 0b01000000 : reg.pair.F &= 0b10111111; }
    inline void setFlagY(bool on) { on ? reg.pair.F |= 0b00100000 : reg.pair.F &= 0b11011111; }
    inline void setFlagH(bool on) { on ? reg.pair.F |= 0b00010000 : reg.pair.F &= 0b11101111; }
    inline void setFlagX(bool on) { on ? reg.pair.F |= 0b00001000 : reg.pair.F &= 0b11110111; }
    inline void setFlagPV(bool on) { on ? reg.pair.F |= 0b00000100 : reg.pair.F &= 0b11111011; }
    inline void setFlagN(bool on) { on ? reg.pair.F |= 0b00000010 : reg.pair.F &= 0b11111101; }
    inline void setFlagC(bool on) { on ? reg.pair.F |= 0b00000001 : reg.pair.F &= 0b11111110; }

    inline void setFlagsXY(unsigned char value)
    {
        setFlagX(value & 0b00001000 ? true : false);
        setFlagY(value & 0b00100000 ? true : false);
    }

    // flag checker
    inline bool isFlagS() { return reg.pair.F & 0b10000000; }
    inline bool isFlagZ() { return reg.pair.F & 0b01000000; }
    inline bool isFlagH() { return reg.pair.F & 0b00010000; }
    inline bool isFlagPV() { return reg.pair.F & 0b00000100; }
    inline bool isFlagN() { return reg.pair.F & 0b00000010; }
    inline bool isFlagC() { return reg.pair.F & 0b00000001; }

    inline unsigned char IFF1() { return 0b00000001; }
    inline unsigned char IFF2() { return 0b00000100; }
    inline unsigned char IFF_IRQ() { return 0b00100000; }
    inline unsigned char IFF_NMI() { return 0b01000000; }
    inline unsigned char IFF_HALT() { return 0b10000000; }

    class BreakPoint
    {
      public:
        unsigned short addr;
        void (*callback)(void* arg);
        BreakPoint(unsigned short addr, void (*callback)(void* arg))
        {
            this->addr = addr;
            this->callback = callback;
        }
    };

    class BreakOperand
    {
      public:
        unsigned char operandNumber;
        void (*callback)(void* arg);
        BreakOperand(unsigned char operandNumber, void (*callback)(void* arg))
        {
            this->operandNumber = operandNumber;
            this->callback = callback;
        }
    };

    struct Callback {
        unsigned char (*read)(void* arg, unsigned short addr);
        void (*write)(void* arg, unsigned short addr, unsigned char value);
        unsigned char (*in)(void* arg, unsigned char port);
        void (*out)(void* arg, unsigned char port, unsigned char value);
        void (*debugMessage)(void* arg, const char* message);
        void (*consumeClock)(void* arg, int clock);
        std::vector<BreakPoint*> breakPoints;
        std::vector<BreakOperand*> breakOperands;
        void* arg;
    } CB;

    bool requestBreakFlag;

    inline void checkBreakPoint()
    {
        if (!CB.breakPoints.empty()) {
            for (auto bp : CB.breakPoints) {
                if (bp->addr == reg.PC) {
                    bp->callback(CB.arg);
                }
            }
        }
    }

    inline void checkBreakOperand(unsigned char operandNumber)
    {
        if (!CB.breakOperands.empty()) {
            for (auto bo : CB.breakOperands) {
                if (bo->operandNumber == operandNumber) {
                    bo->callback(CB.arg);
                }
            }
        }
    }

    inline void log(const char* format, ...)
    {
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

    inline unsigned short getRP(unsigned char rp)
    {
        switch (rp & 0b11) {
            case 0b00: return (reg.pair.B << 8) + reg.pair.C;
            case 0b01: return (reg.pair.D << 8) + reg.pair.E;
            case 0b10: return (reg.pair.H << 8) + reg.pair.L;
            default: return reg.SP;
        }
    }

    inline void setRP(unsigned char rp, unsigned short value)
    {
        unsigned char h = (value & 0xFF00) >> 8;
        unsigned char l = value & 0x00FF;
        switch (rp & 0b11) {
            case 0b00: {
                reg.pair.B = h;
                reg.pair.C = l;
                break;
            }
            case 0b01: {
                reg.pair.D = h;
                reg.pair.E = l;
                break;
            }
            case 0b10: {
                reg.pair.H = h;
                reg.pair.L = l;
                break;
            }
            default: reg.SP = value;
        }
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

    inline unsigned char readByte(unsigned short addr, int clock = 4)
    {
        unsigned char byte = CB.read(CB.arg, addr);
        consumeClock(clock);
        return byte;
    }

    inline void writeByte(unsigned short addr, unsigned char value, int clock = 4)
    {
        CB.write(CB.arg, addr, value);
        consumeClock(clock);
    }

    inline unsigned char inPort(unsigned char port, int clock = 4)
    {
        unsigned char byte = CB.in(CB.arg, port);
        consumeClock(clock);
        return byte;
    }

    inline void outPort(unsigned char port, unsigned char value, int clock = 4)
    {
        CB.out(CB.arg, port, value);
        consumeClock(clock);
    }

    static inline int NOP(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] NOP", ctx->reg.PC);
        ctx->reg.PC++;
        return 0;
    }

    static inline int HALT(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] HALT", ctx->reg.PC);
        ctx->reg.IFF |= ctx->IFF_HALT();
        ctx->reg.PC++;
        return 0;
    }

    static inline int DI(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] DI", ctx->reg.PC);
        ctx->reg.IFF &= ~(ctx->IFF1() | ctx->IFF2());
        ctx->reg.PC++;
        return 0;
    }

    static inline int EI(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] EI", ctx->reg.PC);
        ctx->reg.IFF |= ctx->IFF1() | ctx->IFF2();
        ctx->reg.PC++;
        ctx->reg.execEI = 1;
        return 0;
    }

    inline int IM(unsigned char interrptMode)
    {
        if (isDebug()) log("[%04X] IM %d", reg.PC, interrptMode);
        reg.interrupt &= 0b11111100;
        reg.interrupt |= interrptMode & 0b11;
        reg.PC += 2;
        return 0;
    }

    inline int LD_A_I()
    {
        if (isDebug()) log("[%04X] LD A<$%02X>, I<$%02X>", reg.PC, reg.pair.A, reg.I);
        reg.pair.A = reg.I;
        setFlagPV(reg.IFF & IFF1() ? true : false);
        reg.PC += 2;
        return consumeClock(1);
    }

    inline int LD_I_A()
    {
        if (isDebug()) log("[%04X] LD I<$%02X>, A<$%02X>", reg.PC, reg.I, reg.pair.A);
        reg.I = reg.pair.A;
        reg.PC += 2;
        return consumeClock(1);
    }

    inline int LD_A_R()
    {
        if (isDebug()) log("[%04X] LD A<$%02X>, R<$%02X>", reg.PC, reg.pair.A, reg.R);
        reg.pair.A = reg.R;
        setFlagPV(reg.IFF & IFF1() ? true : false);
        reg.PC += 2;
        return consumeClock(1);
    }

    inline int LD_R_A()
    {
        if (isDebug()) log("[%04X] LD R<$%02X>, A<$%02X>", reg.PC, reg.R, reg.pair.A);
        reg.R = reg.pair.A;
        reg.PC += 2;
        return consumeClock(1);
    }

    static inline int EXTRA(Z80* ctx)
    {
        unsigned char mode = ctx->readByte(ctx->reg.PC + 1);
        switch (mode) {
            case 0b01000110: return ctx->IM(0);
            case 0b01010110: return ctx->IM(1);
            case 0b01011110: return ctx->IM(2);
            case 0b01010111: return ctx->LD_A_I();
            case 0b01000111: return ctx->LD_I_A();
            case 0b01011111: return ctx->LD_A_R();
            case 0b01001111: return ctx->LD_R_A();
            case 0b10100000: return ctx->LDI();
            case 0b10110000: return ctx->LDIR();
            case 0b10101000: return ctx->LDD();
            case 0b10111000: return ctx->LDDR();
            case 0b01000100: return ctx->NEG();
            case 0b10100001: return ctx->CPI();
            case 0b10110001: return ctx->CPIR();
            case 0b10101001: return ctx->CPD();
            case 0b10111001: return ctx->CPDR();
            case 0b01001101: return ctx->RETI();
            case 0b01000101: return ctx->RETN();
            case 0b10100010: return ctx->INI();
            case 0b10110010: return ctx->INIR();
            case 0b10101010: return ctx->IND();
            case 0b10111010: return ctx->INDR();
            case 0b10100011: return ctx->OUTI();
            case 0b10110011: return ctx->OUTIR();
            case 0b10101011: return ctx->OUTD();
            case 0b10111011: return ctx->OUTDR();
            case 0b01101111: return ctx->RLD();
            case 0b01100111: return ctx->RRD();
        }
        switch (mode & 0b11001111) {
            case 0b01001011: return ctx->LD_RP_ADDR((mode & 0b00110000) >> 4);
            case 0b01000011: return ctx->LD_ADDR_RP((mode & 0b00110000) >> 4);
            case 0b01001010: return ctx->ADC_HL_RP((mode & 0b00110000) >> 4);
            case 0b01000010: return ctx->SBC_HL_RP((mode & 0b00110000) >> 4);
        }
        switch (mode & 0b11000111) {
            case 0b01000000: return ctx->IN_R_C((mode & 0b00111000) >> 3);
            case 0b01000001: return ctx->OUT_C_R((mode & 0b00111000) >> 3);
        }
        if (ctx->isDebug()) ctx->log("unknown EXTRA: $%02X", mode);
        return -1;
    }

    // operand of using IX (first byte is 0b11011101)
    static inline int OP_IX(Z80* ctx)
    {
        unsigned char op2 = ctx->readByte(ctx->reg.PC + 1);
        switch (op2) {
            case 0b00100010: return ctx->LD_ADDR_IX();
            case 0b00100011: return ctx->INC_IX_reg();
            case 0b00101010: return ctx->LD_IX_ADDR();
            case 0b00101011: return ctx->DEC_IX_reg();
            case 0b00110110: return ctx->LD_IX_N();
            case 0b00100001: return ctx->LD_IX_NN();
            case 0b00110100: return ctx->INC_IX();
            case 0b00110101: return ctx->DEC_IX();
            case 0b10000110: return ctx->ADD_A_IX();
            case 0b10001110: return ctx->ADC_A_IX();
            case 0b10010110: return ctx->SUB_A_IX();
            case 0b10011110: return ctx->SBC_A_IX();
            case 0b10100110: return ctx->AND_IX();
            case 0b10101110: return ctx->XOR_IX();
            case 0b10110110: return ctx->OR_IX();
            case 0b10111110: return ctx->CP_IX();
            case 0b11100001: return ctx->POP_IX();
            case 0b11100011: return ctx->EX_SP_IX();
            case 0b11100101: return ctx->PUSH_IX();
            case 0b11101001: return ctx->JP_IX();
            case 0b11111001: return ctx->LD_SP_IX();
            case 0b11001011: {
                unsigned char op3 = ctx->readByte(ctx->reg.PC + 2);
                unsigned char op4 = ctx->readByte(ctx->reg.PC + 3);
                switch (op4) {
                    case 0b00000110: return ctx->RLC_IX(op3);
                    case 0b00001110: return ctx->RRC_IX(op3);
                    case 0b00010110: return ctx->RL_IX(op3);
                    case 0b00011110: return ctx->RR_IX(op3);
                    case 0b00100110: return ctx->SLA_IX(op3);
                    case 0b00101110: return ctx->SRA_IX(op3);
                    case 0b00111110: return ctx->SRL_IX(op3);
                }
                switch (op4 & 0b11000111) {
                    case 0b01000110: return ctx->BIT_IX(op3, (op4 & 0b00111000) >> 3);
                    case 0b11000110: return ctx->SET_IX(op3, (op4 & 0b00111000) >> 3);
                    case 0b10000110: return ctx->RES_IX(op3, (op4 & 0b00111000) >> 3);
                }
            }
        }
        if ((op2 & 0b11000111) == 0b01000110) {
            return ctx->LD_R_IX((op2 & 0b00111000) >> 3);
        } else if ((op2 & 0b11001111) == 0b00001001) {
            return ctx->ADD_IX_RP((op2 & 0b00110000) >> 4);
        } else if ((op2 & 0b11111000) == 0b01110000) {
            return ctx->LD_IX_R(op2 & 0b00000111);
        }
        if (ctx->isDebug()) ctx->log("detected an unknown operand: 0b11011101 - $%02X", op2);
        return -1;
    }

    // operand of using IY (first byte is 0b11111101)
    static inline int OP_IY(Z80* ctx)
    {
        unsigned char op2 = ctx->readByte(ctx->reg.PC + 1);
        switch (op2) {
            case 0b00100010: return ctx->LD_ADDR_IY();
            case 0b00100011: return ctx->INC_IY_reg();
            case 0b00101010: return ctx->LD_IY_ADDR();
            case 0b00101011: return ctx->DEC_IY_reg();
            case 0b00110110: return ctx->LD_IY_N();
            case 0b00100001: return ctx->LD_IY_NN();
            case 0b00110100: return ctx->INC_IY();
            case 0b00110101: return ctx->DEC_IY();
            case 0b10000110: return ctx->ADD_A_IY();
            case 0b10001110: return ctx->ADC_A_IY();
            case 0b10010110: return ctx->SUB_A_IY();
            case 0b10011110: return ctx->SBC_A_IY();
            case 0b10100110: return ctx->AND_IY();
            case 0b10101110: return ctx->XOR_IY();
            case 0b10110110: return ctx->OR_IY();
            case 0b10111110: return ctx->CP_IY();
            case 0b11100001: return ctx->POP_IY();
            case 0b11100011: return ctx->EX_SP_IY();
            case 0b11100101: return ctx->PUSH_IY();
            case 0b11101001: return ctx->JP_IY();
            case 0b11111001: return ctx->LD_SP_IY();
            case 0b11001011: {
                unsigned char op3 = ctx->readByte(ctx->reg.PC + 2);
                unsigned char op4 = ctx->readByte(ctx->reg.PC + 3);
                switch (op4) {
                    case 0b00000110: return ctx->RLC_IY(op3);
                    case 0b00001110: return ctx->RRC_IY(op3);
                    case 0b00010110: return ctx->RL_IY(op3);
                    case 0b00011110: return ctx->RR_IY(op3);
                    case 0b00100110: return ctx->SLA_IY(op3);
                    case 0b00101110: return ctx->SRA_IY(op3);
                    case 0b00111110: return ctx->SRL_IY(op3);
                }
                switch (op4 & 0b11000111) {
                    case 0b01000110: return ctx->BIT_IY(op3, (op4 & 0b00111000) >> 3);
                    case 0b11000110: return ctx->SET_IY(op3, (op4 & 0b00111000) >> 3);
                    case 0b10000110: return ctx->RES_IY(op3, (op4 & 0b00111000) >> 3);
                }
            }
        }
        if ((op2 & 0b11000111) == 0b01000110) {
            return ctx->LD_R_IY((op2 & 0b00111000) >> 3);
        } else if ((op2 & 0b11001111) == 0b00001001) {
            return ctx->ADD_IY_RP((op2 & 0b00110000) >> 4);
        } else if ((op2 & 0b11111000) == 0b01110000) {
            return ctx->LD_IY_R(op2 & 0b00000111);
        }
        if (ctx->isDebug()) ctx->log("detected an unknown operand: 11111101 - $%02X", op2);
        return -1;
    }

    // operand of using other register (first byte is 0b11001011)
    static inline int OP_R(Z80* ctx)
    {
        unsigned char op2 = ctx->readByte(ctx->reg.PC + 1);
        switch (op2) {
            case 0b00000110: return ctx->RLC_HL();
            case 0b00010110: return ctx->RL_HL();
            case 0b00001110: return ctx->RRC_HL();
            case 0b00011110: return ctx->RR_HL();
            case 0b00100110: return ctx->SLA_HL();
            case 0b00101110: return ctx->SRA_HL();
            case 0b00111110: return ctx->SRL_HL();
        }
        switch (op2 & 0b11111000) {
            case 0b00000000: return ctx->RLC_R(op2 & 0b00000111);
            case 0b00010000: return ctx->RL_R(op2 & 0b00000111);
            case 0b00001000: return ctx->RRC_R(op2 & 0b00000111);
            case 0b00011000: return ctx->RR_R(op2 & 0b00000111);
            case 0b00100000: return ctx->SLA_R(op2 & 0b00000111);
            case 0b00101000: return ctx->SRA_R(op2 & 0b00000111);
            case 0b00111000: return ctx->SRL_R(op2 & 0b00000111);
        }
        switch (op2 & 0b11000111) {
            case 0b01000110: return ctx->BIT_HN((op2 & 0b00111000) >> 3);
            case 0b11000110: return ctx->SET_HN((op2 & 0b00111000) >> 3);
            case 0b10000110: return ctx->RES_HN((op2 & 0b00111000) >> 3);
        }
        switch (op2 & 0b11000000) {
            case 0b01000000: return ctx->BIT_R(op2 & 0b00000111, (op2 & 0b00111000) >> 3);
            case 0b11000000: return ctx->SET_R(op2 & 0b00000111, (op2 & 0b00111000) >> 3);
            case 0b10000000: return ctx->RES_R(op2 & 0b00000111, (op2 & 0b00111000) >> 3);
        }
        if (ctx->isDebug()) ctx->log("detected an unknown operand: 11001011 - $%02X", op2);
        return -1;
    }

    // Load location (HL) with value n
    static inline int LD_HL_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned short hl = ctx->getHL();
        if (ctx->isDebug()) ctx->log("[%04X] LD (HL<$%04X>), $%02X", ctx->reg.PC, hl, n);
        ctx->writeByte(hl, n, 3);
        ctx->reg.PC += 2;
        return 0;
    }

    // Load Acc. wth location (BC)
    static inline int LD_A_BC(Z80* ctx)
    {
        unsigned short addr = ctx->getBC();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] LD A, (BC<$%02X%02X>) = $%02X", ctx->reg.PC, ctx->reg.pair.B, ctx->reg.pair.C, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC++;
        return 0;
    }

    // Load Acc. wth location (DE)
    static inline int LD_A_DE(Z80* ctx)
    {
        unsigned short addr = ctx->getDE();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] LD A, (DE<$%02X%02X>) = $%02X", ctx->reg.PC, ctx->reg.pair.D, ctx->reg.pair.E, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC++;
        return 0;
    }

    // Load Acc. wth location (nn)
    static inline int LD_A_NN(Z80* ctx)
    {
        unsigned short addr = ctx->readByte(ctx->reg.PC + 1, 3);
        addr += ctx->readByte(ctx->reg.PC + 2, 3) << 8;
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] LD A, ($%04X) = $%02X", ctx->reg.PC, addr, n);
        ctx->reg.pair.A = n;
        ctx->reg.PC += 3;
        return 0;
    }

    // Load location (BC) wtih Acc.
    static inline int LD_BC_A(Z80* ctx)
    {
        unsigned short addr = ctx->getBC();
        unsigned char n = ctx->reg.pair.A;
        if (ctx->isDebug()) ctx->log("[%04X] LD (BC<$%02X%02X>), A<$%02X>", ctx->reg.PC, ctx->reg.pair.B, ctx->reg.pair.C, n);
        ctx->writeByte(addr, n, 3);
        ctx->reg.PC++;
        return 0;
    }

    // Load location (DE) wtih Acc.
    static inline int LD_DE_A(Z80* ctx)
    {
        unsigned short addr = ctx->getDE();
        unsigned char n = ctx->reg.pair.A;
        if (ctx->isDebug()) ctx->log("[%04X] LD (DE<$%02X%02X>), A<$%02X>", ctx->reg.PC, ctx->reg.pair.D, ctx->reg.pair.E, n);
        ctx->writeByte(addr, n, 3);
        ctx->reg.PC++;
        return 0;
    }

    // Load location (nn) with Acc.
    static inline int LD_NN_A(Z80* ctx)
    {
        unsigned short addr = ctx->readByte(ctx->reg.PC + 1, 3);
        addr += ctx->readByte(ctx->reg.PC + 2, 3) << 8;
        unsigned char n = ctx->reg.pair.A;
        if (ctx->isDebug()) ctx->log("[%04X] LD ($%04X), A<$%02X>", ctx->reg.PC, addr, n);
        ctx->writeByte(addr, n, 3);
        ctx->reg.PC += 3;
        return 0;
    }

    // Load HL with location (nn).
    static inline int LD_HL_ADDR(Z80* ctx)
    {
        unsigned char nL = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char nH = ctx->readByte(ctx->reg.PC + 2, 3);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = ctx->readByte(addr, 3);
        unsigned char h = ctx->readByte(addr + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] LD HL<$%04X>, ($%04X) = $%02X%02X", ctx->reg.PC, ctx->getHL(), addr, h, l);
        ctx->reg.pair.L = l;
        ctx->reg.pair.H = h;
        ctx->reg.PC += 3;
        return 0;
    }

    // Load location (nn) with HL.
    static inline int LD_ADDR_HL(Z80* ctx)
    {
        unsigned char nL = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char nH = ctx->readByte(ctx->reg.PC + 2, 3);
        unsigned short addr = (nH << 8) + nL;
        if (ctx->isDebug()) ctx->log("[%04X] LD ($%04X), %s", ctx->reg.PC, addr, ctx->registerPairDump(0b10));
        ctx->writeByte(addr, ctx->reg.pair.L, 3);
        ctx->writeByte(addr + 1, ctx->reg.pair.H, 3);
        ctx->reg.PC += 3;
        return 0;
    }

    // Load SP with HL.
    static inline int LD_SP_HL(Z80* ctx)
    {
        unsigned short value = ctx->getHL();
        if (ctx->isDebug()) ctx->log("[%04X] LD %s, HL<$%04X>", ctx->reg.PC, ctx->registerPairDump(0b11), value);
        ctx->reg.SP = value;
        ctx->reg.PC++;
        return ctx->consumeClock(2);
    }

    // Exchange H and L with D and E
    static inline int EX_DE_HL(Z80* ctx)
    {
        unsigned short de = ctx->getDE();
        unsigned short hl = ctx->getHL();
        if (ctx->isDebug()) ctx->log("[%04X] EX %s, %s", ctx->reg.PC, ctx->registerPairDump(0b01), ctx->registerPairDump(0b10));
        ctx->setDE(hl);
        ctx->setHL(de);
        ctx->reg.PC++;
        return 0;
    }

    // Exchange A and F with A' and F'
    static inline int EX_AF_AF2(Z80* ctx)
    {
        unsigned short af = ctx->getAF();
        unsigned short af2 = ctx->getAF2();
        if (ctx->isDebug()) ctx->log("[%04X] EX AF<$%02X%02X>, AF'<$%02X%02X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.pair.F, ctx->reg.back.A, ctx->reg.back.F);
        ctx->setAF(af2);
        ctx->setAF2(af);
        ctx->reg.PC++;
        return 0;
    }

    static inline int EX_SP_HL(Z80* ctx)
    {
        unsigned char l = ctx->readByte(ctx->reg.SP);
        unsigned char h = ctx->readByte(ctx->reg.SP + 1);
        unsigned short hl = ctx->getHL();
        if (ctx->isDebug()) ctx->log("[%04X] EX (SP<$%04X>) = $%02X%02X, HL<$%04X>", ctx->reg.PC, ctx->reg.SP, h, l, hl);
        ctx->writeByte(ctx->reg.SP, ctx->reg.pair.L);
        ctx->writeByte(ctx->reg.SP + 1, ctx->reg.pair.H, 3);
        ctx->reg.pair.L = l;
        ctx->reg.pair.H = h;
        ctx->reg.PC++;
        return 0;
    }

    static inline int EXX(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] EXX", ctx->reg.PC);
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
        return 0;
    }

    static inline int PUSH_AF(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] PUSH AF<$%02X%02X> <SP:$%04X>", ctx->reg.PC, ctx->reg.pair.A, ctx->reg.pair.F, ctx->reg.SP);
        ctx->writeByte(--ctx->reg.SP, ctx->reg.pair.A);
        ctx->writeByte(--ctx->reg.SP, ctx->reg.pair.F, 3);
        ctx->reg.PC++;
        return 0;
    }

    static inline int POP_AF(Z80* ctx)
    {
        unsigned short sp = ctx->reg.SP;
        unsigned char l = ctx->readByte(ctx->reg.SP++, 3);
        unsigned char h = ctx->readByte(ctx->reg.SP++, 3);
        if (ctx->isDebug()) ctx->log("[%04X] POP AF <SP:$%04X> = $%02X%02X", ctx->reg.PC, sp, h, l);
        ctx->reg.pair.F = l;
        ctx->reg.pair.A = h;
        ctx->reg.PC++;
        return 0;
    }

    static inline int RLCA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a7 = ctx->reg.pair.A & 0x80 ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] RLCA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b01111111;
        ctx->reg.pair.A <<= 1;
        ctx->reg.pair.A |= a7; // differ with RLA
        ctx->setFlagC(a7 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->setFlagsXY(ctx->reg.pair.A);
        ctx->reg.PC++;
        return 0;
    }

    static inline int RRCA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a0 = ctx->reg.pair.A & 0x01;
        if (ctx->isDebug()) ctx->log("[%04X] RRCA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b11111110;
        ctx->reg.pair.A >>= 1;
        ctx->reg.pair.A |= a0 ? 0x80 : 0; // differ with RRA
        ctx->setFlagC(a0 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->setFlagsXY(ctx->reg.pair.A);
        ctx->reg.PC++;
        return 0;
    }

    static inline int RLA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a7 = ctx->reg.pair.A & 0x80 ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] RLA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b01111111;
        ctx->reg.pair.A <<= 1;
        ctx->reg.pair.A |= c; // differ with RLCA
        ctx->setFlagC(a7 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->setFlagsXY(ctx->reg.pair.A);
        ctx->reg.PC++;
        return 0;
    }

    static inline int RRA(Z80* ctx)
    {
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        unsigned char a0 = ctx->reg.pair.A & 0x01;
        if (ctx->isDebug()) ctx->log("[%04X] RRA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, c ? "ON" : "OFF");
        ctx->reg.pair.A &= 0b11111110;
        ctx->reg.pair.A >>= 1;
        ctx->reg.pair.A |= c ? 0x80 : 0x00; // differ with RLCA
        ctx->setFlagC(a0 ? true : false);
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->setFlagsXY(ctx->reg.pair.A);
        ctx->reg.PC++;
        return 0;
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
        if (isDebug()) log("detected an unknown register number: $%02X", r);
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

    inline char* conditionDump(unsigned char c)
    {
        static char CN[4];
        switch (c) {
            case 0b000: strcpy(CN, "NZ"); break;
            case 0b001: strcpy(CN, "Z"); break;
            case 0b010: strcpy(CN, "NC"); break;
            case 0b011: strcpy(CN, "C"); break;
            case 0b100: strcpy(CN, "PO"); break;
            case 0b101: strcpy(CN, "PE"); break;
            case 0b110: strcpy(CN, "P"); break;
            case 0b111: strcpy(CN, "M"); break;
            default: strcpy(CN, "??");
        }
        return CN;
    }

    inline char* relativeDump(signed char e)
    {
        static char buf[80];
        if (e < 0) {
            sprintf(buf, "$%04X - %d = $%04X", reg.PC, -e, reg.PC + e);
        } else {
            sprintf(buf, "$%04X + %d = $%04X", reg.PC, e, reg.PC + e);
        }
        return buf;
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
        if (isDebug()) log("[%04X] LD %s, %s", reg.PC, registerDump(r1), registerDump(r2));
        if (r1p && r2p) *r1p = *r2p;
        reg.PC += 1;
        return 0;
    }

    // Load Reg. r with value n
    inline int LD_R_N(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char n = readByte(reg.PC + 1, 3);
        if (isDebug()) log("[%04X] LD %s, $%02X", reg.PC, registerDump(r), n);
        if (rp) *rp = n;
        reg.PC += 2;
        return 0;
    }

    // Load Reg. r with location (HL)
    inline int LD_R_HL(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char n = readByte(getHL(), 3);
        if (isDebug()) log("[%04X] LD %s, (%s) = $%02X", reg.PC, registerDump(r), registerPairDump(0b10), n);
        if (rp) *rp = n;
        reg.PC += 1;
        return 0;
    }

    // Load Reg. r with location (IX+d)
    inline int LD_R_IX(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        signed char d = readByte(reg.PC + 2);
        unsigned char n = readByte((reg.IX + d) & 0xFFFF);
        if (isDebug()) log("[%04X] LD %s, (IX<$%04X>+$%02X) = $%02X", reg.PC, registerDump(r), reg.IX, d, n);
        if (rp) *rp = n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Load Reg. r with location (IY+d)
    inline int LD_R_IY(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        signed char d = readByte(reg.PC + 2);
        unsigned char n = readByte((reg.IY + d) & 0xFFFF);
        if (isDebug()) log("[%04X] LD %s, (IY<$%04X>+$%02X) = $%02X", reg.PC, registerDump(r), reg.IY, d, n);
        if (rp) *rp = n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Load location (HL) with Reg. r
    inline int LD_HL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned short addr = getHL();
        if (isDebug()) log("[%04X] LD (%s), %s", reg.PC, registerPairDump(0b10), registerDump(r));
        if (rp) writeByte(addr, *rp, 3);
        reg.PC += 1;
        return 0;
    }

    // 	Load location (IX+d) with Reg. r
    inline int LD_IX_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        if (isDebug()) log("[%04X] LD (IX<$%04X>+$%02X), %s", reg.PC, reg.IX, d, registerDump(r));
        if (rp) writeByte(addr, *rp);
        reg.PC += 3;
        return consumeClock(3);
    }

    // 	Load location (IY+d) with Reg. r
    inline int LD_IY_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        if (isDebug()) log("[%04X] LD (IY<$%04X>+$%02X), %s", reg.PC, reg.IY, d, registerDump(r));
        if (rp) writeByte(addr, *rp);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Load location (IX+d) with value n
    inline int LD_IX_N()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned char n = readByte(reg.PC + 3);
        unsigned short addr = reg.IX + d;
        if (isDebug()) log("[%04X] LD (IX<$%04X>+$%02X), $%02X", reg.PC, reg.IX, d, n);
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // Load location (IY+d) with value n
    inline int LD_IY_N()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned char n = readByte(reg.PC + 3);
        unsigned short addr = reg.IY + d;
        if (isDebug()) log("[%04X] LD (IY<$%04X>+$%02X), $%02X", reg.PC, reg.IY, d, n);
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // Load Reg. pair rp with value nn.
    inline int LD_RP_NN(unsigned char rp)
    {
        unsigned char nL = readByte(reg.PC + 1, 3);
        unsigned char nH = readByte(reg.PC + 2, 3);
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
                if (isDebug()) log("[%04X] LD SP<$%04X>, $%02X%02X", reg.PC, reg.SP, nH, nL);
                reg.SP = (nH << 8) + nL;
                reg.PC += 3;
                return 0;
            default:
                if (isDebug()) log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        if (isDebug()) log("[%04X] LD %s, $%02X%02X", reg.PC, registerPairDump(rp), nH, nL);
        *rH = nH;
        *rL = nL;
        reg.PC += 3;
        return 0;
    }

    inline int LD_IX_NN()
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        if (isDebug()) log("[%04X] LD IX, $%02X%02X", reg.PC, nH, nL);
        reg.IX = (nH << 8) + nL;
        reg.PC += 4;
        return 0;
    }

    inline int LD_IY_NN()
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        if (isDebug()) log("[%04X] LD IY, $%02X%02X", reg.PC, nH, nL);
        reg.IY = (nH << 8) + nL;
        reg.PC += 4;
        return 0;
    }

    // Load Reg. pair rp with location (nn)
    inline int LD_RP_ADDR(unsigned char rp)
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = readByte(addr, 3);
        unsigned char h = readByte(addr + 1, 3);
        if (isDebug()) log("[%04X] LD %s, ($%02X%02X) = $%02X%02X", reg.PC, registerPairDump(rp), nH, nL, h, l);
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
                if (isDebug()) log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        reg.PC += 4;
        return 0;
    }

    // Load location (nn) with Reg. pair rp.
    inline int LD_ADDR_RP(unsigned char rp)
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] LD ($%04X), %s", reg.PC, addr, registerPairDump(rp));
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
                if (isDebug()) log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        writeByte(addr, l, 3);
        writeByte(addr + 1, h, 3);
        reg.PC += 4;
        return 0;
    }

    // Load IX with location (nn)
    inline int LD_IX_ADDR()
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = readByte(addr, 3);
        unsigned char h = readByte(addr + 1, 3);
        if (isDebug()) log("[%04X] LD IX<$%04X>, ($%02X%02X) = $%02X%02X", reg.PC, reg.IX, nH, nL, h, l);
        reg.IX = (h << 8) + l;
        reg.PC += 4;
        return 0;
    }

    // Load IY with location (nn)
    inline int LD_IY_ADDR()
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        unsigned short addr = (nH << 8) + nL;
        unsigned char l = readByte(addr, 3);
        unsigned char h = readByte(addr + 1, 3);
        if (isDebug()) log("[%04X] LD IY<$%04X>, ($%02X%02X) = $%02X%02X", reg.PC, reg.IY, nH, nL, h, l);
        reg.IY = (h << 8) + l;
        reg.PC += 4;
        return 0;
    }

    inline int LD_ADDR_IX()
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] LD ($%04X), IX<$%04X>", reg.PC, addr, reg.IX);
        unsigned char l = reg.IX & 0x00FF;
        unsigned char h = (reg.IX & 0xFF00) >> 8;
        writeByte(addr, l, 3);
        writeByte(addr + 1, h, 3);
        reg.PC += 4;
        return 0;
    }

    inline int LD_ADDR_IY()
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] LD ($%04X), IY<$%04X>", reg.PC, addr, reg.IY);
        unsigned char l = reg.IY & 0x00FF;
        unsigned char h = (reg.IY & 0xFF00) >> 8;
        writeByte(addr, l, 3);
        writeByte(addr + 1, h, 3);
        reg.PC += 4;
        return 0;
    }

    // Load SP with IX.
    inline int LD_SP_IX()
    {
        unsigned short value = reg.IX;
        if (isDebug()) log("[%04X] LD %s, IX<$%04X>", reg.PC, registerPairDump(0b11), value);
        reg.SP = value;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Load SP with IY.
    inline int LD_SP_IY()
    {
        unsigned short value = reg.IY;
        if (isDebug()) log("[%04X] LD %s, IY<$%04X>", reg.PC, registerPairDump(0b11), value);
        reg.SP = value;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Load location (DE) with Loacation (HL), increment DE, HL, decrement BC
    inline int LDI(bool isRepeat = false)
    {
        if (isDebug()) log("[%04X] %s ... %s, %s, %s", reg.PC, isRepeat ? "LDIR" : "LDI", registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
        unsigned short bc = getBC();
        unsigned short de = getDE();
        unsigned short hl = getHL();
        unsigned char n = readByte(hl);
        writeByte(de, n);
        de++;
        hl++;
        bc--;
        setBC(bc);
        setDE(de);
        setHL(hl);
        setFlagN(false);
        setFlagH(false);
        setFlagPV(bc != 0);
        setFlagsXY((reg.pair.A + (int)n) & 0xFF); // NOTE: undocumented
        reg.PC += 2;
        return 0;
    }

    inline void endCheckForRepeatLD()
    {
        if (0 != getBC()) {
            reg.PC -= 2;
            consumeClock(5);
        }
    }

    // Load location (DE) with Loacation (HL), increment DE, HL, decrement BC and repeat until BC=0.
    inline int LDIR()
    {
        LDI(true);
        endCheckForRepeatLD();
        return 0;
    }

    // Load location (DE) with Loacation (HL), decrement DE, HL, decrement BC
    inline int LDD(bool isRepeat = false)
    {
        if (isDebug()) log("[%04X] %s ... %s, %s, %s", reg.PC, isRepeat ? "LDDR" : "LDD", registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
        unsigned short bc = getBC();
        unsigned short de = getDE();
        unsigned short hl = getHL();
        unsigned char n = readByte(hl);
        writeByte(de, n);
        de--;
        hl--;
        bc--;
        setBC(bc);
        setDE(de);
        setHL(hl);
        setFlagN(false);
        setFlagH(false);
        setFlagPV(bc != 0);
        setFlagsXY(reg.pair.A + n); // NOTE: undocumented
        reg.PC += 2;
        return 0;
    }

    // Load location (DE) with Loacation (HL), decrement DE, HL, decrement BC and repeat until BC=0.
    inline int LDDR()
    {
        LDD(true);
        endCheckForRepeatLD();
        return 0;
    }

    // Exchange stack top with IX
    inline int EX_SP_IX()
    {
        unsigned char l = readByte(reg.SP);
        unsigned char h = readByte(reg.SP + 1);
        unsigned char i = (reg.IX & 0xFF00) >> 8;
        unsigned char x = reg.IX & 0x00FF;
        if (isDebug()) log("[%04X] EX (SP<$%04X>) = $%02X%02X, IX<$%04X>", reg.PC, reg.SP, h, l, reg.IX);
        writeByte(reg.SP, x);
        writeByte(reg.SP + 1, i, 3);
        reg.IX = (h << 8) + l;
        reg.PC += 2;
        return 0;
    }

    // Exchange stack top with IY
    inline int EX_SP_IY()
    {
        unsigned char l = readByte(reg.SP);
        unsigned char h = readByte(reg.SP + 1);
        unsigned char i = (reg.IY & 0xFF00) >> 8;
        unsigned char y = reg.IY & 0x00FF;
        if (isDebug()) log("[%04X] EX (SP<$%04X>) = $%02X%02X, IY<$%04X>", reg.PC, reg.SP, h, l, reg.IY);
        writeByte(reg.SP, y);
        writeByte(reg.SP + 1, i, 3);
        reg.IY = (h << 8) + l;
        reg.PC += 2;
        return 0;
    }

    // Push Reg. on Stack.
    inline int PUSH_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] PUSH %s <SP:$%04X>", reg.PC, registerPairDump(rp), reg.SP);
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
                if (isDebug()) log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        writeByte(--reg.SP, h);
        writeByte(--reg.SP, l, 3);
        reg.PC++;
        return 0;
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
                if (isDebug()) log("invalid register pair has specified: $%02X", rp);
                return -1;
        }
        if (isDebug()) log("[%04X] POP %s <SP:$%04X> = $%02X%02X", reg.PC, registerPairDump(rp), sp, *h, *l);
        *l = readByte(reg.SP++, 3);
        *h = readByte(reg.SP++, 3);
        reg.PC++;
        return 0;
    }

    // Push Reg. IX on Stack.
    inline int PUSH_IX()
    {
        if (isDebug()) log("[%04X] PUSH IX<$%04X> <SP:$%04X>", reg.PC, reg.IX, reg.SP);
        unsigned char h = (reg.IX & 0xFF00) >> 8;
        unsigned char l = reg.IX & 0x00FF;
        writeByte(--reg.SP, h);
        writeByte(--reg.SP, l, 3);
        reg.PC += 2;
        return 0;
    }

    // Pop Reg. IX from Stack.
    inline int POP_IX()
    {
        unsigned short sp = reg.SP;
        unsigned char l = readByte(reg.SP++, 3);
        unsigned char h = readByte(reg.SP++, 3);
        if (isDebug()) log("[%04X] POP IX <SP:$%04X> = $%02X%02X", reg.PC, sp, h, l);
        reg.IX = (h << 8) + l;
        reg.PC += 2;
        return 0;
    }

    // Push Reg. IY on Stack.
    inline int PUSH_IY()
    {
        if (isDebug()) log("[%04X] PUSH IY<$%04X> <SP:$%04X>", reg.PC, reg.IY, reg.SP);
        unsigned char h = (reg.IY & 0xFF00) >> 8;
        unsigned char l = reg.IY & 0x00FF;
        writeByte(--reg.SP, h);
        writeByte(--reg.SP, l, 3);
        reg.PC += 2;
        return 0;
    }

    // Pop Reg. IY from Stack.
    inline int POP_IY()
    {
        unsigned short sp = reg.SP;
        unsigned char l = readByte(reg.SP++, 3);
        unsigned char h = readByte(reg.SP++, 3);
        if (isDebug()) log("[%04X] POP IY <SP:$%04X> = $%02X%02X", reg.PC, sp, h, l);
        reg.IY = (h << 8) + l;
        reg.PC += 2;
        return 0;
    }

    // Rotate register Left Circular
    inline int RLC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r7 = *rp & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RLC %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b01111111;
        *rp <<= 1;
        *rp |= r7; // differ with RL
        setFlagC(r7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(*rp);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return 0;
    }

    // Rotate Left register
    inline int RL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r7 = *rp & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RL %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b01111111;
        *rp <<= 1;
        *rp |= c; // differ with RLC
        setFlagC(r7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(*rp);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return 0;
    }

    // Shift operand register left Arithmetic
    inline int SLA_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r7 = *rp & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] SLA %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b01111111;
        *rp <<= 1;
        setFlagC(r7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(*rp);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return 0;
    }

    // Rotate register Right Circular
    inline int RRC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r0 = *rp & 0x01;
        if (isDebug()) log("[%04X] RRC %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b11111110;
        *rp >>= 1;
        *rp |= r0 ? 0x80 : 0; // differ with RR
        setFlagC(r0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(*rp);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return 0;
    }

    // Rotate Right register
    inline int RR_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r0 = *rp & 0x01;
        if (isDebug()) log("[%04X] RR %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b11111110;
        *rp >>= 1;
        *rp |= c ? 0x80 : 0; // differ with RRC
        setFlagC(r0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(*rp);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return 0;
    }

    // Shift operand register Right Arithmetic
    inline int SRA_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r0 = *rp & 0x01;
        unsigned char r7 = *rp & 0x80;
        if (isDebug()) log("[%04X] SRA %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b11111110;
        *rp >>= 1;
        r7 ? * rp |= 0x80 : * rp &= 0x7F;
        setFlagC(r0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(*rp);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return 0;
    }

    // Shift operand register Right Logical
    inline int SRL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char r0 = *rp & 0x01;
        if (isDebug()) log("[%04X] SRL %s <C:%s>", reg.PC, registerDump(r), c ? "ON" : "OFF");
        *rp &= 0b11111110;
        *rp >>= 1;
        setFlagC(r0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(*rp);
        setFlagS((*rp & 0x80) != 0);
        setFlagZ(*rp == 0);
        setFlagPV(isEvenNumberBits(*rp));
        reg.PC += 2;
        return 0;
    }

    // Rotate memory (HL) Left Circular
    inline int RLC_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RLC (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= n7; // differ with RL (HL)
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return 0;
    }

    // Rotate Left memory
    inline int RL_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RL (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= c; // differ with RLC (HL)
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return 0;
    }

    // Shift operand location (HL) left Arithmetic
    inline int SLA_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] SLA (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return 0;
    }

    // Rotate memory (HL) Right Circular
    inline int RRC_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] RRC (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= n0 ? 0x80 : 0; // differ with RR (HL)
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return 0;
    }

    // Rotate Right memory
    inline int RR_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] RR (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= c ? 0x80 : 0; // differ with RRC (HL)
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return 0;
    }

    // Shift operand location (HL) Right Arithmetic
    inline int SRA_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        unsigned char n7 = n & 0x80;
        if (isDebug()) log("[%04X] SRA (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n7 ? n |= 0x80 : n &= 0x7F;
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return 0;
    }

    // Shift operand location (HL) Right Logical
    inline int SRL_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] SRL (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 2;
        return 0;
    }

    // Rotate memory (IX+d) Left Circular
    inline int RLC_IX(signed char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RLC (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= n7; // differ with RL (IX+d)
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Rotate memory (IX+d) Right Circular
    inline int RRC_IX(signed char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] RRC (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= n0 ? 0x80 : 0; // differ with RR (IX+d)
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Rotate Left memory
    inline int RL_IX(signed char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RL (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= c; // differ with RLC (IX+d)
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IX+d) left Arithmetic
    inline int SLA_IX(signed char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] SLA (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Rotate Right memory
    inline int RR_IX(signed char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] RR (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= c ? 0x80 : 0; // differ with RRC (IX+d)
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IX+d) Right Arithmetic
    inline int SRA_IX(signed char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        unsigned char n7 = n & 0x80;
        if (isDebug()) log("[%04X] SRA (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n7 ? n |= 0x80 : n &= 0x7F;
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IX+d) Right Logical
    inline int SRL_IX(signed char d)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] SRL (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Rotate memory (IY+d) Left Circular
    inline int RLC_IY(signed char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RLC (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= n7; // differ with RL (IX+d)
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Rotate memory (IY+d) Right Circular
    inline int RRC_IY(signed char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] RRC (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= n0 ? 0x80 : 0; // differ with RR (IX+d)
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Rotate Left memory
    inline int RL_IY(signed char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] RL (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        n |= c; // differ with RLC (IY+d)
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IY+d) left Arithmetic
    inline int SLA_IY(signed char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n7 = n & 0x80 ? 1 : 0;
        if (isDebug()) log("[%04X] SLA (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b01111111;
        n <<= 1;
        writeByte(addr, n, 3);
        setFlagC(n7 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Rotate Right memory
    inline int RR_IY(signed char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] RR (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n |= c ? 0x80 : 0; // differ with RRC (IY+d)
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IY+d) Right Arithmetic
    inline int SRA_IY(signed char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        unsigned char n7 = n & 0x80;
        if (isDebug()) log("[%04X] SRA (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        n7 ? n |= 0x80 : n &= 0x7F;
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IY+d) Right Logical
    inline int SRL_IY(signed char d)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        unsigned char n0 = n & 0x01;
        if (isDebug()) log("[%04X] SRL (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, c ? "ON" : "OFF");
        n &= 0b11111110;
        n >>= 1;
        writeByte(addr, n, 3);
        setFlagC(n0 ? true : false);
        setFlagH(false);
        setFlagN(false);
        setFlagsXY(n);
        setFlagS((n & 0x80) != 0);
        setFlagZ(n == 0);
        setFlagPV(isEvenNumberBits(n));
        reg.PC += 4;
        return 0;
    }

    inline void setFlagByAddition(unsigned char before, unsigned char addition, bool withCarry = false)
    {
        int result = before + addition + (withCarry ? (isFlagC() ? 1 : 0) : 0);
        int carryBits = before ^ addition ^ result;
        unsigned char finalResult = (unsigned char)result;
        clearAllFlags();
        setFlagZ(finalResult == 0);
        setFlagS(finalResult & 0x80 ? true : false);
        setFlagsXY(finalResult);
        setFlagC(carryBits & 0x100 ? true : false);
        setFlagH(carryBits & 0x10 ? true : false);
        setFlagPV((((carryBits << 1) ^ carryBits) & 0x100) != 0);
    }

    inline void setFlagByIncrement(unsigned char before)
    {
        unsigned char n = before++;
        clearAllFlagsWithKeepCarry();
        setFlagZ(n == 0);
        setFlagS(n & 0x80 ? true : false);
        setFlagsXY(n);
        setFlagH(n & 0x0F ? false : true);
        setFlagPV(n == 0x80);
    }

    // Add Reg. r to Acc.
    inline int ADD_A_R(unsigned char r)
    {
        if (isDebug()) log("[%04X] ADD %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        setFlagByAddition(reg.pair.A, *rp);
        reg.pair.A += *rp;
        reg.PC += 1;
        return 0;
    }

    // Add value n to Acc.
    static inline int ADD_A_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] ADD %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->setFlagByAddition(ctx->reg.pair.A, n);
        ctx->reg.pair.A += n;
        ctx->reg.PC += 2;
        return 0;
    }

    // Add location (HL) to Acc.
    static inline int ADD_A_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] ADD %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->setFlagByAddition(ctx->reg.pair.A, n);
        ctx->reg.pair.A += n;
        ctx->reg.PC += 1;
        return 0;
    }

    // Add location (IX+d) to Acc.
    inline int ADD_A_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] ADD %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        setFlagByAddition(reg.pair.A, n);
        reg.pair.A += n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Add location (IY+d) to Acc.
    inline int ADD_A_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] ADD %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        setFlagByAddition(reg.pair.A, n);
        reg.pair.A += n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Add Resister with carry
    inline int ADC_A_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char c = isFlagC() ? 1 : 0;
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] ADC %s, %s <C:%s>", reg.PC, registerDump(0b111), registerDump(r), c ? "ON" : "OFF");
        setFlagByAddition(reg.pair.A, *rp, true);
        reg.pair.A += c + *rp;
        reg.PC += 1;
        return 0;
    }

    // Add immediate with carry
    static inline int ADC_A_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] ADC %s, $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), n, c ? "ON" : "OFF");
        ctx->setFlagByAddition(ctx->reg.pair.A, n, true);
        ctx->reg.pair.A += c + n;
        ctx->reg.PC += 2;
        return 0;
    }

    // Add memory with carry
    static inline int ADC_A_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] ADC %s, (%s) = $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n, c ? "ON" : "OFF");
        ctx->setFlagByAddition(ctx->reg.pair.A, n, true);
        ctx->reg.pair.A += c + n;
        ctx->reg.PC += 1;
        return 0;
    }

    // Add memory with carry
    inline int ADC_A_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        setFlagByAddition(reg.pair.A, n, true);
        reg.pair.A += c + n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Add memory with carry
    inline int ADC_A_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        setFlagByAddition(reg.pair.A, n, true);
        reg.pair.A += c + n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Increment Register
    inline int INC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] INC %s", reg.PC, registerDump(r));
        setFlagByIncrement(*rp);
        (*rp)++;
        reg.PC += 1;
        return 0;
    }

    // Increment location (HL)
    static inline int INC_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr);
        if (ctx->isDebug()) ctx->log("[%04X] INC (%s) = $%02X", ctx->reg.PC, ctx->registerPairDump(0b10), n);
        ctx->setFlagByIncrement(n);
        ctx->writeByte(addr, n + 1, 3);
        ctx->reg.PC += 1;
        return 0;
    }

    // Increment location (IX+d)
    inline int INC_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] INC (IX+d<$%04X>) = $%02X", reg.PC, addr, n);
        setFlagByIncrement(n);
        writeByte(addr, n + 1);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Increment location (IY+d)
    inline int INC_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] INC (IY+d<$%04X>) = $%02X", reg.PC, addr, n);
        setFlagByIncrement(n);
        writeByte(addr, n + 1);
        reg.PC += 3;
        return consumeClock(3);
    }

    inline void setFlagBySubstract(unsigned char before, unsigned char substract, bool withCarry = false)
    {
        int result = before - substract - (withCarry ? (isFlagC() ? 1 : 0) : 0);
        int carryBits = before ^ substract ^ result;
        unsigned char finalResult = (unsigned char)result;
        clearAllFlags();
        setFlagN(true);
        setFlagZ(finalResult == 0);
        setFlagS(finalResult & 0x80 ? true : false);
        setFlagsXY(finalResult);
        setFlagC(carryBits & 0x100 ? true : false);
        setFlagH(carryBits & 0x10);
        setFlagPV((((carryBits << 1) ^ carryBits) & 0x100) != 0);
    }

    inline void setFlagByDecrement(unsigned char before)
    {
        unsigned char n = before--;
        clearAllFlagsWithKeepCarry();
        setFlagN(true);
        setFlagZ(n == 0);
        setFlagS(n & 0x80);
        setFlagsXY(n);
        setFlagH((n & 0x0F) == 0x0F);
        setFlagPV(n == 0x7F);
    }

    // Substract Register
    inline int SUB_A_R(unsigned char r)
    {
        if (isDebug()) log("[%04X] SUB %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        setFlagBySubstract(reg.pair.A, *rp);
        reg.pair.A -= *rp;
        reg.PC += 1;
        return 0;
    }

    // Substract immediate
    static inline int SUB_A_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] SUB %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->setFlagBySubstract(ctx->reg.pair.A, n);
        ctx->reg.pair.A -= n;
        ctx->reg.PC += 2;
        return 0;
    }

    // Substract memory
    static inline int SUB_A_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] SUB %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->setFlagBySubstract(ctx->reg.pair.A, n);
        ctx->reg.pair.A -= n;
        ctx->reg.PC += 1;
        return 0;
    }

    // Substract memory
    inline int SUB_A_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SUB %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        setFlagBySubstract(reg.pair.A, n);
        reg.pair.A -= n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Substract memory
    inline int SUB_A_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SUB %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        setFlagBySubstract(reg.pair.A, n);
        reg.pair.A -= n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Substract Resister with carry
    inline int SBC_A_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char c = isFlagC() ? 1 : 0;
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] SBC %s, %s <C:%s>", reg.PC, registerDump(0b111), registerDump(r), c ? "ON" : "OFF");
        setFlagBySubstract(reg.pair.A, *rp, true);
        reg.pair.A -= c + *rp;
        reg.PC += 1;
        return 0;
    }

    // Substract immediate with carry
    static inline int SBC_A_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] SBC %s, $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), n, c ? "ON" : "OFF");
        ctx->setFlagBySubstract(ctx->reg.pair.A, n, true);
        ctx->reg.pair.A -= c + n;
        ctx->reg.PC += 2;
        return 0;
    }

    // Substract memory with carry
    static inline int SBC_A_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] SBC %s, (%s) = $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n, c ? "ON" : "OFF");
        ctx->setFlagBySubstract(ctx->reg.pair.A, n, true);
        ctx->reg.pair.A -= c + n;
        ctx->reg.PC += 1;
        return 0;
    }

    // Substract memory with carry
    inline int SBC_A_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        setFlagBySubstract(reg.pair.A, n, true);
        reg.pair.A -= c + n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Substract memory with carry
    inline int SBC_A_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        setFlagBySubstract(reg.pair.A, n, true);
        reg.pair.A -= c + n;
        reg.PC += 3;
        return consumeClock(3);
    }

    // Decrement Register
    inline int DEC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] DEC %s", reg.PC, registerDump(r));
        setFlagByDecrement(*rp);
        (*rp)--;
        reg.PC += 1;
        return 0;
    }

    // Decrement location (HL)
    static inline int DEC_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr);
        if (ctx->isDebug()) ctx->log("[%04X] DEC (%s) = $%02X", ctx->reg.PC, ctx->registerPairDump(0b10), n);
        ctx->setFlagByDecrement(n);
        ctx->writeByte(addr, n - 1, 3);
        ctx->reg.PC += 1;
        return 0;
    }

    // Decrement location (IX+d)
    inline int DEC_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] DEC (IX+d<$%04X>) = $%02X", reg.PC, addr, n);
        setFlagByDecrement(n);
        writeByte(addr, n - 1);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Decrement location (IY+d)
    inline int DEC_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] DEC (IY+d<$%04X>) = $%02X", reg.PC, addr, n);
        setFlagByDecrement(n);
        writeByte(addr, n - 1);
        reg.PC += 3;
        return consumeClock(3);
    }

    inline void setFlagByAdd16(unsigned short before, unsigned short addition)
    {
        unsigned int result32u = before;
        result32u += addition;
        setFlagH(0x00FF < (before & 0x00FF) + (addition & 0x00FF));
        setFlagN(true);
        setFlagC(65535 < result32u);
    }

    inline void setFlagByAdc16(unsigned short before, unsigned short addition)
    {
        unsigned short result16 = before + addition;
        signed int result32s = (signed short)before;
        result32s += (signed short)addition;
        setFlagByAdd16(before, addition);
        setFlagS(result16 & 0x8000 ? true : false);
        setFlagZ(result16 == 0);
        setFlagPV(result32s < -32768 || 32767 < result32s);
    }

    // Add register pair to H and L
    inline int ADD_HL_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] ADD %s, %s", reg.PC, registerPairDump(0b10), registerPairDump(rp));
        unsigned short hl = getHL();
        unsigned short nn = getRP(rp);
        setFlagByAdd16(hl, nn);
        setHL(hl + nn);
        reg.PC++;
        return consumeClock(7);
    }

    // Add with carry register pair to HL
    inline int ADC_HL_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] ADC %s, %s <C:%s>", reg.PC, registerPairDump(0b10), registerPairDump(rp), isFlagC() ? "ON" : "OFF");
        unsigned short hl = getHL();
        unsigned short nn = getRP(rp);
        unsigned char c = isFlagC() ? 1 : 0;
        setFlagByAdc16(hl, c + nn);
        setHL(hl + c + nn);
        reg.PC += 2;
        return consumeClock(7);
    }

    // Add register pair to IX
    inline int ADD_IX_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] ADD IX<$%04X>, %s", reg.PC, reg.IX, registerPairDump(rp));
        unsigned short nn = getRP(rp);
        setFlagByAdd16(reg.IX, nn);
        reg.IX += nn;
        reg.PC += 2;
        return consumeClock(7);
    }

    // Add register pair to IY
    inline int ADD_IY_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] ADD IY<$%04X>, %s", reg.PC, reg.IY, registerPairDump(rp));
        unsigned short nn = getRP(rp);
        setFlagByAdd16(reg.IY, nn);
        reg.IY += nn;
        reg.PC += 2;
        return consumeClock(7);
    }

    // Increment register pair
    inline int INC_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] INC %s", reg.PC, registerPairDump(rp));
        unsigned short nn = getRP(rp);
        setRP(rp, nn + 1);
        reg.PC++;
        return consumeClock(2);
    }

    // Increment IX
    inline int INC_IX_reg()
    {
        if (isDebug()) log("[%04X] INC IX<$%04X>", reg.PC, reg.IX);
        reg.IX++;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Increment IY
    inline int INC_IY_reg()
    {
        if (isDebug()) log("[%04X] INC IY<$%04X>", reg.PC, reg.IY);
        reg.IY++;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Decrement register pair
    inline int DEC_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] DEC %s", reg.PC, registerPairDump(rp));
        unsigned short nn = getRP(rp);
        setRP(rp, nn - 1);
        reg.PC++;
        return consumeClock(2);
    }

    // Decrement IX
    inline int DEC_IX_reg()
    {
        if (isDebug()) log("[%04X] DEC IX<$%04X>", reg.PC, reg.IX);
        reg.IX--;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Decrement IY
    inline int DEC_IY_reg()
    {
        if (isDebug()) log("[%04X] DEC IY<$%04X>", reg.PC, reg.IY);
        reg.IY--;
        reg.PC += 2;
        return consumeClock(2);
    }

    inline void setFlagBySbc16(unsigned short before, unsigned short substract)
    {
        unsigned short result16 = before - substract;
        unsigned int result32u = before;
        result32u -= substract;
        signed int result32s = (signed short)before;
        result32s += (signed short)substract;
        setFlagH((0x00FF & (before & 0xFF00) - (substract & 0xFF00)) == 0); // TODO: これで正しいのだろうか？
        setFlagN(false);
        setFlagC(65535 < result32u);
        setFlagS(result16 & 0x8000 ? true : false);
        setFlagZ(result16 == 0);
        setFlagPV(result32s < -32768 || 32767 < result32s);
    }

    // Subtract register pair from HL with carry
    inline int SBC_HL_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] SBC %s, %s <C:%s>", reg.PC, registerPairDump(0b10), registerPairDump(rp), isFlagC() ? "ON" : "OFF");
        unsigned short hl = getHL();
        unsigned short nn = getRP(rp);
        unsigned char c = isFlagC() ? 1 : 0;
        setFlagBySbc16(hl, c + nn);
        setHL(hl - c - nn);
        reg.PC += 2;
        return consumeClock(7);
    }

    inline void setFlagByLogical()
    {
        clearAllFlags();
        setFlagZ(reg.pair.A == 0);
        setFlagS(reg.pair.A & 0x80 ? true : false);
        setFlagsXY(reg.pair.A);
        setFlagPV(isEvenNumberBits(reg.pair.A));
    }

    // AND Register
    inline int AND_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] AND %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        reg.pair.A &= *rp;
        setFlagByLogical();
        setFlagH(true);
        reg.PC++;
        return 0;
    }

    // AND immediate
    static inline int AND_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] AND %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->reg.pair.A &= n;
        ctx->setFlagByLogical();
        ctx->setFlagH(true);
        ctx->reg.PC += 2;
        return 0;
    }

    // AND Memory
    static inline int AND_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] AND %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->reg.pair.A &= n;
        ctx->setFlagByLogical();
        ctx->setFlagH(true);
        ctx->reg.PC++;
        return 0;
    }

    // AND Memory
    inline int AND_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] AND %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A & n);
        reg.pair.A &= n;
        setFlagByLogical();
        setFlagH(true);
        reg.PC += 3;
        return consumeClock(3);
    }

    // AND Memory
    inline int AND_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] AND %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A & n);
        reg.pair.A &= n;
        setFlagByLogical();
        setFlagH(true);
        reg.PC += 3;
        return consumeClock(3);
    }

    // OR Register
    inline int OR_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] OR %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        reg.pair.A |= *rp;
        setFlagByLogical();
        reg.PC++;
        return 0;
    }

    // OR immediate
    static inline int OR_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] OR %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->reg.pair.A |= n;
        ctx->setFlagByLogical();
        ctx->reg.PC += 2;
        return 0;
    }

    // OR Memory
    static inline int OR_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] OR %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), ctx->reg.pair.A | n);
        ctx->reg.pair.A |= n;
        ctx->setFlagByLogical();
        ctx->reg.PC++;
        return 0;
    }

    // OR Memory
    inline int OR_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] OR %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A | n);
        reg.pair.A |= n;
        setFlagByLogical();
        reg.PC += 3;
        return consumeClock(3);
    }

    // OR Memory
    inline int OR_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] OR %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A | n);
        reg.pair.A |= n;
        setFlagByLogical();
        reg.PC += 3;
        return consumeClock(3);
    }

    // XOR Reigster
    inline int XOR_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] XOR %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        reg.pair.A ^= *rp;
        setFlagByLogical();
        reg.PC++;
        return 0;
    }

    // XOR immediate
    static inline int XOR_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] XOR %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->reg.pair.A ^= n;
        ctx->setFlagByLogical();
        ctx->reg.PC += 2;
        return 0;
    }

    // XOR Memory
    static inline int XOR_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] XOR %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), ctx->reg.pair.A ^ n);
        ctx->reg.pair.A ^= n;
        ctx->setFlagByLogical();
        ctx->reg.PC++;
        return 0;
    }

    // XOR Memory
    inline int XOR_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] XOR %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A ^ n);
        reg.pair.A ^= n;
        setFlagByLogical();
        reg.PC += 3;
        return consumeClock(3);
    }

    // XOR Memory
    inline int XOR_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] XOR %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A ^ n);
        reg.pair.A ^= n;
        setFlagByLogical();
        reg.PC += 3;
        return consumeClock(3);
    }

    // Complement acc. (1's Comp.)
    static inline int CPL(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] CPL %s", ctx->reg.PC, ctx->registerDump(0b111));
        ctx->reg.pair.A = ~ctx->reg.pair.A;
        ctx->setFlagH(true);
        ctx->setFlagN(true);
        ctx->reg.PC++;
        return 0;
    }

    // Negate Acc. (2's Comp.)
    inline int NEG()
    {
        if (isDebug()) log("[%04X] NEG %s", reg.PC, registerDump(0b111));
        setFlagBySubstract(0, reg.pair.A);
        reg.pair.A = 0 - reg.pair.A;
        reg.PC += 2;
        return 0;
    }

    //　Complement Carry Flag
    static inline int CCF(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] CCF <C:%s -> %s>", ctx->reg.PC, ctx->isFlagC() ? "ON" : "OFF", !ctx->isFlagC() ? "ON" : "OFF");
        ctx->setFlagH(ctx->isFlagC());
        ctx->setFlagN(false);
        ctx->setFlagC(!ctx->isFlagC());
        ctx->reg.PC++;
        return 0;
    }

    // Set Carry Flag
    static inline int SCF(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] SCF <C:%s -> ON>", ctx->reg.PC, ctx->isFlagC() ? "ON" : "OFF");
        ctx->setFlagH(false);
        ctx->setFlagN(false);
        ctx->setFlagC(true);
        ctx->reg.PC++;
        return 0;
    }

    // Test BIT b of register r
    inline int BIT_R(unsigned char r, unsigned char bit)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] BIT %s of bit-%d", reg.PC, registerDump(r), bit);
        unsigned char n = 0;
        switch (bit) {
            case 0: n = *rp & 0b00000001; break;
            case 1: n = *rp & 0b00000010; break;
            case 2: n = *rp & 0b00000100; break;
            case 3: n = *rp & 0b00001000; break;
            case 4: n = *rp & 0b00010000; break;
            case 5: n = *rp & 0b00100000; break;
            case 6: n = *rp & 0b01000000; break;
            case 7: n = *rp & 0b10000000; break;
        }
        clearAllFlagsWithKeepCarry();
        setFlagZ(n ? false : true);
        setFlagPV(isFlagZ());
        if (n && 7 == bit) setFlagS(true);
        setFlagsXY(*rp);
        setFlagH(true);
        reg.PC += 2;
        return 0;
    }

    // Test BIT b of lacation (HL)
    inline int BIT_HN(unsigned char bit)
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        clearAllFlagsWithKeepCarry();
        setFlagsXY(n);
        if (isDebug()) log("[%04X] BIT (%s) = $%02X of bit-%d", reg.PC, registerPairDump(0b10), n, bit);
        switch (bit) {
            case 0: n &= 0b00000001; break;
            case 1: n &= 0b00000010; break;
            case 2: n &= 0b00000100; break;
            case 3: n &= 0b00001000; break;
            case 4: n &= 0b00010000; break;
            case 5: n &= 0b00100000; break;
            case 6: n &= 0b01000000; break;
            case 7: n &= 0b10000000; break;
        }
        setFlagZ(n ? false : true);
        setFlagPV(isFlagZ());
        if (n && 7 == bit) setFlagS(true);
        setFlagH(true);
        reg.PC += 2;
        return 0;
    }

    // Test BIT b of lacation (IX+d)
    inline int BIT_IX(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        clearAllFlagsWithKeepCarry();
        setFlagsXY(n);
        if (isDebug()) log("[%04X] BIT (IX+d<$%04X>) = $%02X of bit-%d", reg.PC, addr, n, bit);
        switch (bit) {
            case 0: n &= 0b00000001; break;
            case 1: n &= 0b00000010; break;
            case 2: n &= 0b00000100; break;
            case 3: n &= 0b00001000; break;
            case 4: n &= 0b00010000; break;
            case 5: n &= 0b00100000; break;
            case 6: n &= 0b01000000; break;
            case 7: n &= 0b10000000; break;
        }
        setFlagZ(n ? false : true);
        setFlagPV(isFlagZ());
        if (n && 7 == bit) setFlagS(true);
        setFlagH(true);
        reg.PC += 4;
        return 0;
    }

    // Test BIT b of lacation (IY+d)
    inline int BIT_IY(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        clearAllFlagsWithKeepCarry();
        setFlagsXY(n);
        if (isDebug()) log("[%04X] BIT (IY+d<$%04X>) = $%02X of bit-%d", reg.PC, addr, n, bit);
        switch (bit) {
            case 0: n &= 0b00000001; break;
            case 1: n &= 0b00000010; break;
            case 2: n &= 0b00000100; break;
            case 3: n &= 0b00001000; break;
            case 4: n &= 0b00010000; break;
            case 5: n &= 0b00100000; break;
            case 6: n &= 0b01000000; break;
            case 7: n &= 0b10000000; break;
        }
        setFlagZ(n ? false : true);
        setFlagPV(isFlagZ());
        if (n && 7 == bit) setFlagS(true);
        setFlagH(true);
        reg.PC += 4;
        return 0;
    }

    // SET bit b of register r
    inline int SET_R(unsigned char r, unsigned char bit)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] SET %s of bit-%d", reg.PC, registerDump(r), bit);
        switch (bit) {
            case 0: *rp |= 0b00000001; break;
            case 1: *rp |= 0b00000010; break;
            case 2: *rp |= 0b00000100; break;
            case 3: *rp |= 0b00001000; break;
            case 4: *rp |= 0b00010000; break;
            case 5: *rp |= 0b00100000; break;
            case 6: *rp |= 0b01000000; break;
            case 7: *rp |= 0b10000000; break;
        }
        reg.PC += 2;
        return 0;
    }

    // SET bit b of lacation (HL)
    inline int SET_HN(unsigned char bit)
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SET (%s) = $%02X of bit-%d", reg.PC, registerPairDump(0b10), n, bit);
        switch (bit) {
            case 0: n |= 0b00000001; break;
            case 1: n |= 0b00000010; break;
            case 2: n |= 0b00000100; break;
            case 3: n |= 0b00001000; break;
            case 4: n |= 0b00010000; break;
            case 5: n |= 0b00100000; break;
            case 6: n |= 0b01000000; break;
            case 7: n |= 0b10000000; break;
        }
        writeByte(addr, n, 3);
        reg.PC += 2;
        return 0;
    }

    // SET bit b of lacation (IX+d)
    inline int SET_IX(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SET (IX+d<$%04X>) = $%02X of bit-%d", reg.PC, addr, n, bit);
        switch (bit) {
            case 0: n |= 0b00000001; break;
            case 1: n |= 0b00000010; break;
            case 2: n |= 0b00000100; break;
            case 3: n |= 0b00001000; break;
            case 4: n |= 0b00010000; break;
            case 5: n |= 0b00100000; break;
            case 6: n |= 0b01000000; break;
            case 7: n |= 0b10000000; break;
        }
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // SET bit b of lacation (IY+d)
    inline int SET_IY(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SET (IY+d<$%04X>) = $%02X of bit-%d", reg.PC, addr, n, bit);
        switch (bit) {
            case 0: n |= 0b00000001; break;
            case 1: n |= 0b00000010; break;
            case 2: n |= 0b00000100; break;
            case 3: n |= 0b00001000; break;
            case 4: n |= 0b00010000; break;
            case 5: n |= 0b00100000; break;
            case 6: n |= 0b01000000; break;
            case 7: n |= 0b10000000; break;
        }
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // RESET bit b of register r
    inline int RES_R(unsigned char r, unsigned char bit)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] RES %s of bit-%d", reg.PC, registerDump(r), bit);
        switch (bit) {
            case 0: *rp &= 0b11111110; break;
            case 1: *rp &= 0b11111101; break;
            case 2: *rp &= 0b11111011; break;
            case 3: *rp &= 0b11110111; break;
            case 4: *rp &= 0b11101111; break;
            case 5: *rp &= 0b11011111; break;
            case 6: *rp &= 0b10111111; break;
            case 7: *rp &= 0b01111111; break;
        }
        reg.PC += 2;
        return 0;
    }

    // RESET bit b of lacation (HL)
    inline int RES_HN(unsigned char bit)
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RES (%s) = $%02X of bit-%d", reg.PC, registerPairDump(0b10), n, bit);
        switch (bit) {
            case 0: n &= 0b11111110; break;
            case 1: n &= 0b11111101; break;
            case 2: n &= 0b11111011; break;
            case 3: n &= 0b11110111; break;
            case 4: n &= 0b11101111; break;
            case 5: n &= 0b11011111; break;
            case 6: n &= 0b10111111; break;
            case 7: n &= 0b01111111; break;
        }
        writeByte(addr, n, 3);
        reg.PC += 2;
        return 0;
    }

    // RESET bit b of lacation (IX+d)
    inline int RES_IX(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RES (IX+d<$%04X>) = $%02X of bit-%d", reg.PC, addr, n, bit);
        switch (bit) {
            case 0: n &= 0b11111110; break;
            case 1: n &= 0b11111101; break;
            case 2: n &= 0b11111011; break;
            case 3: n &= 0b11110111; break;
            case 4: n &= 0b11101111; break;
            case 5: n &= 0b11011111; break;
            case 6: n &= 0b10111111; break;
            case 7: n &= 0b01111111; break;
        }
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // RESET bit b of lacation (IY+d)
    inline int RES_IY(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RES (IY+d<$%04X>) = $%02X of bit-%d", reg.PC, addr, n, bit);
        switch (bit) {
            case 0: n &= 0b11111110; break;
            case 1: n &= 0b11111101; break;
            case 2: n &= 0b11111011; break;
            case 3: n &= 0b11110111; break;
            case 4: n &= 0b11101111; break;
            case 5: n &= 0b11011111; break;
            case 6: n &= 0b10111111; break;
            case 7: n &= 0b01111111; break;
        }
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // common procedure of CPI/CPD
    inline void cp_internal(bool isIncrement)
    {
        unsigned short hl = getHL();
        unsigned short bc = getBC();
        unsigned char n = readByte(hl);
        int result = reg.pair.A - (int)n;
        int carryBits = reg.pair.A ^ n ^ result;
        unsigned char finalResult = (unsigned char)result;
        hl += isIncrement ? 1 : -1;
        bc--;
        setHL(hl);
        setBC(bc);
        clearAllFlags();
        setFlagN(true);
        setFlagZ(finalResult == 0);
        setFlagS(finalResult & 0x80 ? true : false);
        setFlagH(carryBits & 0x10 ? true : false);
        setFlagPV(bc != 0);
        setFlagsXY(reg.pair.A - n - (isFlagH() ? 1 : 0));
        reg.PC += 2;
    }

    // Compare location (HL) and A, increment HL and decrement BC
    inline int CPI(bool isRepeat = false)
    {
        if (isDebug()) log("[%04X] %s ... %s, %s, %s", reg.PC, isRepeat ? "CPIR" : "CPI", registerDump(0b111), registerPairDump(0b10), registerPairDump(0b00));
        cp_internal(true);
        return consumeClock(4);
    }

    inline void endCheckForRepeatCP()
    {
        if (!isFlagZ() && 0 != getBC()) {
            reg.PC -= 2;
            consumeClock(5);
        }
    }

    // Compare location (HL) and A, increment HL, decrement BC repeat until BC=0.
    inline int CPIR()
    {
        CPI(true);
        endCheckForRepeatCP();
        return 0;
    }

    // Compare location (HL) and A, decrement HL and decrement BC
    inline int CPD(bool isRepeat = false)
    {
        if (isDebug()) log("[%04X] %s ... %s, %s, %s", reg.PC, isRepeat ? "CPDR" : "CPD", registerDump(0b111), registerPairDump(0b10), registerPairDump(0b00));
        cp_internal(false);
        return consumeClock(4);
    }

    // Compare location (HL) and A, decrement HL, decrement BC repeat until BC=0.
    inline int CPDR()
    {
        CPD(true);
        endCheckForRepeatCP();
        return 0;
    }

    // Compare Register
    inline int CP_R(unsigned char r)
    {
        if (isDebug()) log("[%04X] CP %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        setFlagBySubstract(reg.pair.A, *rp);
        reg.PC += 1;
        return 0;
    }

    // Compare immediate
    static inline int CP_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] CP %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->setFlagBySubstract(ctx->reg.pair.A, n);
        ctx->reg.PC += 2;
        return 0;
    }

    // Compare memory
    static inline int CP_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] CP %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->setFlagBySubstract(ctx->reg.pair.A, n);
        ctx->reg.PC += 1;
        return 0;
    }

    // Compare memory
    inline int CP_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] CP %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        setFlagBySubstract(reg.pair.A, n);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Compare memory
    inline int CP_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] CP %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        setFlagBySubstract(reg.pair.A, n);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Jump
    static inline int JP_NN(Z80* ctx)
    {
        unsigned char nL = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char nH = ctx->readByte(ctx->reg.PC + 2, 3);
        unsigned short addr = (nH << 8) + nL;
        if (ctx->isDebug()) ctx->log("[%04X] JP $%04X", ctx->reg.PC, addr);
        ctx->reg.PC = addr;
        return 0;
    }

    // Conditional Jump
    inline int JP_C_NN(unsigned char c)
    {
        unsigned char nL = readByte(reg.PC + 1, 3);
        unsigned char nH = readByte(reg.PC + 2, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] JP %s, $%04X", reg.PC, conditionDump(c), addr);
        bool jump;
        switch (c) {
            case 0b000: jump = isFlagZ() ? false : true; break;
            case 0b001: jump = isFlagZ() ? true : false; break;
            case 0b010: jump = isFlagC() ? false : true; break;
            case 0b011: jump = isFlagC() ? true : false; break;
            case 0b100: jump = isFlagPV() ? false : true; break;
            case 0b101: jump = isFlagPV() ? true : false; break;
            case 0b110: jump = isFlagS() ? false : true; break;
            case 0b111: jump = isFlagS() ? true : false; break;
            default: jump = false;
        }
        if (jump) {
            reg.PC = addr;
        } else {
            reg.PC += 3;
        }
        return 0;
    }

    // Jump Relative to PC+e
    static inline int JR_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1) + 2;
        if (ctx->isDebug()) ctx->log("[%04X] JR %s", ctx->reg.PC, ctx->relativeDump(e));
        ctx->reg.PC += e;
        return ctx->consumeClock(4);
    }

    // Jump Relative to PC+e, if carry
    static inline int JR_C_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3) + 2;
        bool execute = ctx->isFlagC();
        if (ctx->isDebug()) ctx->log("[%04X] JR C, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += execute ? e : 2;
        if (execute) ctx->consumeClock(5);
        return 0;
    }

    // Jump Relative to PC+e, if not carry
    static inline int JR_NC_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3) + 2;
        bool execute = !ctx->isFlagC();
        if (ctx->isDebug()) ctx->log("[%04X] JR NC, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += execute ? e : 2;
        if (execute) ctx->consumeClock(5);
        return 0;
    }

    // Jump Relative to PC+e, if zero
    static inline int JR_Z_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3) + 2;
        bool execute = ctx->isFlagZ();
        if (ctx->isDebug()) ctx->log("[%04X] JR Z, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += execute ? e : 2;
        if (execute) ctx->consumeClock(5);
        return 0;
    }

    // Jump Relative to PC+e, if zero
    static inline int JR_NZ_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3) + 2;
        bool execute = !ctx->isFlagZ();
        if (ctx->isDebug()) ctx->log("[%04X] JR NZ, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += execute ? e : 2;
        if (execute) ctx->consumeClock(5);
        return 0;
    }

    // Jump to HL
    static inline int JP_HL(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] JP %s", ctx->reg.PC, ctx->registerPairDump(0b10));
        ctx->reg.PC = ctx->getHL();
        return 0;
    }

    // Jump to IX
    inline int JP_IX()
    {
        if (isDebug()) log("[%04X] JP IX<$%04X>", reg.PC, reg.IX);
        reg.PC = reg.IX;
        return 0;
    }

    // Jump to IY
    inline int JP_IY()
    {
        if (isDebug()) log("[%04X] JP IY<$%04X>", reg.PC, reg.IY);
        reg.PC = reg.IY;
        return 0;
    }

    // 	Decrement B and Jump relative if B=0
    static inline int DJNZ_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1) + 2;
        if (ctx->isDebug()) ctx->log("[%04X] DJNZ %s (%s)", ctx->reg.PC, ctx->relativeDump(e), ctx->registerDump(0b000));
        ctx->reg.pair.B--;
        if (ctx->reg.pair.B) {
            ctx->reg.PC += e;
            ctx->consumeClock(5);
        } else {
            ctx->reg.PC += 2;
        }
        return 0;
    }

    // Call
    static inline int CALL_NN(Z80* ctx)
    {
        unsigned char nL = ctx->readByte(ctx->reg.PC + 1);
        unsigned char nH = ctx->readByte(ctx->reg.PC + 2, 3);
        unsigned short addr = (nH << 8) + nL;
        if (ctx->isDebug()) ctx->log("[%04X] CALL $%04X (%s)", ctx->reg.PC, addr, ctx->registerPairDump(0b11));
        ctx->reg.PC += 3;
        unsigned char pcL = ctx->reg.PC & 0x00FF;
        unsigned char pcH = (ctx->reg.PC & 0xFF00) >> 8;
        ctx->writeByte(ctx->reg.SP - 1, pcH, 3);
        ctx->writeByte(ctx->reg.SP - 2, pcL, 3);
        ctx->reg.SP -= 2;
        ctx->reg.PC = addr;
        return 0;
    }

    // Return
    static inline int RET(Z80* ctx)
    {
        unsigned char nL = ctx->readByte(ctx->reg.SP, 3);
        unsigned char nH = ctx->readByte(ctx->reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (ctx->isDebug()) ctx->log("[%04X] RET to $%04X (%s)", ctx->reg.PC, addr, ctx->registerPairDump(0b11));
        ctx->reg.SP += 2;
        ctx->reg.PC = addr;
        return 0;
    }

    // Call with condition
    inline int CALL_C_NN(unsigned char c)
    {
        bool execute;
        switch (c) {
            case 0b000: execute = isFlagZ() ? false : true; break;
            case 0b001: execute = isFlagZ() ? true : false; break;
            case 0b010: execute = isFlagC() ? false : true; break;
            case 0b011: execute = isFlagC() ? true : false; break;
            case 0b100: execute = isFlagPV() ? false : true; break;
            case 0b101: execute = isFlagPV() ? true : false; break;
            case 0b110: execute = isFlagS() ? false : true; break;
            case 0b111: execute = isFlagS() ? true : false; break;
            default: execute = false;
        }
        unsigned char nL = readByte(reg.PC + 1, 3);
        unsigned char nH = readByte(reg.PC + 2, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] CALL %s, $%04X (%s) <execute:%s>", reg.PC, conditionDump(c), addr, registerPairDump(0b11), execute ? "YES" : "NO");
        reg.PC += 3;
        if (execute) {
            unsigned char pcL = reg.PC & 0x00FF;
            unsigned char pcH = (reg.PC & 0xFF00) >> 8;
            writeByte(reg.SP - 1, pcH);
            writeByte(reg.SP - 2, pcL, 3);
            reg.SP -= 2;
            reg.PC = addr;
        }
        return 0;
    }

    // Return with condition
    inline int RET_C(unsigned char c)
    {
        bool execute;
        switch (c) {
            case 0b000: execute = isFlagZ() ? false : true; break;
            case 0b001: execute = isFlagZ() ? true : false; break;
            case 0b010: execute = isFlagC() ? false : true; break;
            case 0b011: execute = isFlagC() ? true : false; break;
            case 0b100: execute = isFlagPV() ? false : true; break;
            case 0b101: execute = isFlagPV() ? true : false; break;
            case 0b110: execute = isFlagS() ? false : true; break;
            case 0b111: execute = isFlagS() ? true : false; break;
            default: execute = false;
        }
        if (!execute) {
            if (isDebug()) log("[%04X] RET %s <execute:NO>", reg.PC, conditionDump(c));
            reg.PC++;
            return consumeClock(1);
        }
        unsigned char nL = readByte(reg.SP);
        unsigned char nH = readByte(reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] RET %s to $%04X (%s) <execute:YES>", reg.PC, conditionDump(c), addr, registerPairDump(0b11));
        reg.SP += 2;
        reg.PC = addr;
        return 0;
    }

    // Return from interrupt
    inline int RETI()
    {
        unsigned char nL = readByte(reg.SP, 3);
        unsigned char nH = readByte(reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] RETI to $%04X (%s)", reg.PC, addr, registerPairDump(0b11));
        reg.SP += 2;
        reg.PC = addr;
        reg.IFF &= ~IFF_IRQ();
        return 0;
    }

    // Return from non maskable interrupt
    inline int RETN()
    {
        unsigned char nL = readByte(reg.SP, 3);
        unsigned char nH = readByte(reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] RETN to $%04X (%s)", reg.PC, addr, registerPairDump(0b11));
        reg.SP += 2;
        reg.PC = addr;
        if (!((reg.IFF & IFF1()) && (reg.IFF & IFF2()))) {
            reg.IFF |= IFF1();
        } else {
            if (reg.IFF & IFF2()) {
                reg.IFF |= IFF1();
            } else {
                reg.IFF &= ~IFF1();
            }
        }
        return 0;
    }

    // Interrupt
    inline int RST(unsigned char t)
    {
        unsigned short addr = t * 8;
        unsigned char pcH = (reg.PC & 0xFF00) >> 8;
        unsigned char pcL = reg.PC & 0x00FF;
        if (isDebug()) log("[%04X] RST $%04X (%s)", reg.PC, addr, registerPairDump(0b11));
        writeByte(reg.SP - 1, pcH);
        writeByte(reg.SP - 2, pcL, 3);
        reg.SP -= 2;
        reg.PC = addr;
        return 0;
    }

    // Input a byte form device n to accu.
    static inline int IN_A_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char i = ctx->inPort(n);
        if (ctx->isDebug()) ctx->log("[%04X] IN %s, ($%02X) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), n, i);
        ctx->reg.pair.A = i;
        ctx->reg.PC += 2;
        return 0;
    }

    // Input a byte form device (C) to register.
    inline int IN_R_C(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        unsigned char i = inPort(reg.pair.C);
        if (isDebug()) log("[%04X] IN %s, (%s) = $%02X", reg.PC, registerDump(r), registerDump(0b001), i);
        *rp = i;
        clearAllFlagsWithKeepCarry();
        setFlagZ(i == 0);
        setFlagS(i & 0x80 ? true : false);
        setFlagPV(isEvenNumberBits(i));
        setFlagsXY(i);
        reg.PC += 2;
        return 0;
    }

    inline void decrementB_forRepeatIO()
    {
        setFlagByDecrement(reg.pair.B);
        reg.pair.B--;
    }

    inline void endCheckForRepeatIO()
    {
        if (0 != reg.pair.B) {
            reg.PC -= 2;
            consumeClock(5);
        }
    }

    // Load location (HL) with input from port (C); or increment HL and decrement B
    inline int INI(bool isRepeat = false)
    {
        unsigned char i = inPort(reg.pair.C);
        unsigned short hl = getHL();
        if (isDebug()) log("[%04X] %s ... (%s) <- p(%s) = $%02X [%s]", reg.PC, isRepeat ? "INIR" : "INI", registerPairDump(0b10), registerDump(0b001), i, registerDump(0b000));
        writeByte(hl, i);
        decrementB_forRepeatIO();
        setHL(hl + 1);
        setFlagN(i & 0x80 ? true : false);                                             // NOTE: undocumented
        setFlagC(0xFF < i + ((reg.pair.C + 1) & 0xFF));                                // NOTE: undocumented
        setFlagH(isFlagC());                                                           // NOTE: undocumented
        setFlagPV(i + (((reg.pair.C + 1) & 0xFF) & 0x07) ^ reg.pair.B ? true : false); // NOTE: undocumented
        reg.PC += 2;
        return 0;
    }

    // Load location (HL) with input from port (C), increment HL and decrement B, repeat until B=0
    inline int INIR()
    {
        INI(true);
        endCheckForRepeatIO();
        return 0;
    }

    // Load location (HL) with input from port (C); or decrement HL and B
    inline int IND(bool isRepeat = false)
    {
        unsigned char i = inPort(reg.pair.C);
        unsigned short hl = getHL();
        if (isDebug()) log("[%04X] %s ... (%s) <- p(%s) = $%02X [%s]", reg.PC, isRepeat ? "INDR" : "IND", registerPairDump(0b10), registerDump(0b001), i, registerDump(0b000));
        writeByte(hl, i);
        decrementB_forRepeatIO();
        setHL(hl - 1);
        setFlagN(i & 0x80 ? true : false);                                             // NOTE: undocumented
        setFlagC(0xFF < i + ((reg.pair.C + 1) & 0xFF));                                // NOTE: undocumented
        setFlagH(isFlagC());                                                           // NOTE: undocumented
        setFlagPV(i + (((reg.pair.C + 1) & 0xFF) & 0x07) ^ reg.pair.B ? true : false); // NOTE: undocumented
        reg.PC += 2;
        return 0;
    }

    // Load location (HL) with input from port (C), decrement HL and B, repeat until B=0
    inline int INDR()
    {
        IND(true);
        endCheckForRepeatIO();
        return 0;
    }

    // Load Output port (n) with Acc.
    static inline int OUT_N_A(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] OUT ($%02X), %s", ctx->reg.PC, n, ctx->registerDump(0b111));
        ctx->outPort(n, ctx->reg.pair.A);
        ctx->reg.PC += 2;
        return 0;
    }

    // Output a byte to device (C) form register.
    inline int OUT_C_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (!rp) {
            if (isDebug()) log("specified an unknown register (%d)", r);
            return -1;
        }
        if (isDebug()) log("[%04X] OUT (%s), %s", reg.PC, registerDump(0b001), registerDump(r));
        outPort(reg.pair.C, *rp);
        reg.PC += 2;
        return 0;
    }

    // Load Output port (C) with location (HL), increment HL and decrement B
    inline int OUTI(bool isRepeat = false)
    {
        unsigned short hl = getHL();
        unsigned char o = readByte(hl);
        if (isDebug()) log("[%04X] %s ... p(%s) <- (%s) <$%02x> [%s]", reg.PC, isRepeat ? "OTIR" : "OUTI", registerDump(0b001), registerPairDump(0b10), o, registerDump(0b000));
        outPort(reg.pair.C, o);
        decrementB_forRepeatIO();
        setHL(hl + 1);
        setFlagN(o & 0x80 ? true : false);                 // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagC(reg.pair.L + o > 0xFF);                   // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagH(isFlagC());                               // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagPV(((reg.pair.L + o) & 0x07) ^ reg.pair.B); // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        reg.PC += 2;
        return 0;
    }

    // Load output port (C) with location (HL), increment HL and decrement B, repeat until B=0
    inline int OUTIR()
    {
        OUTI(true);
        endCheckForRepeatIO();
        return 0;
    }

    // Load Output port (C) with location (HL), decrement HL and B
    inline int OUTD(bool isRepeat = false)
    {
        unsigned short hl = getHL();
        if (isDebug()) log("[%04X] %s ... p(%s) <- (%s) [%s]", reg.PC, isRepeat ? "OTDR" : "OUTD", registerDump(0b001), registerPairDump(0b10), registerDump(0b000));
        unsigned char o = readByte(hl);
        outPort(reg.pair.C, o);
        decrementB_forRepeatIO();
        setHL(hl - 1);
        setFlagS(o & 0x80 ? true : false);                 // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagC(reg.pair.L + o > 0xFF);                   // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagH(isFlagC());                               // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagPV(((reg.pair.L + o) & 0x07) ^ reg.pair.B); // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        reg.PC += 2;
        return 0;
    }

    // Load output port (C) with location (HL), decrement HL and  B, repeat until B=0
    inline int OUTDR()
    {
        OUTD(true);
        endCheckForRepeatIO();
        return 0;
    }

    // Decimal Adjust Accumulator
    static inline int DAA(Z80* ctx)
    {
        unsigned char aH = (ctx->reg.pair.A & 0b11110000) >> 4;
        unsigned char aL = ctx->reg.pair.A & 0b00001111;
        unsigned beforeA = ctx->reg.pair.A;
        bool beforeCarry = ctx->isFlagC();
        if (ctx->isFlagN()) {
            if (!ctx->isFlagC()) {
                if (!ctx->isFlagH()) {
                    if (aH < 9) {
                        ctx->setFlagC(false);
                        if (aL < 10) {
                            ;
                        } else {
                            ctx->reg.pair.A += 0x06;
                        }
                    } else if (aH == 9) {
                        if (aL < 10) {
                            ctx->setFlagC(false);
                        } else {
                            ctx->reg.pair.A += 0x66;
                            ctx->setFlagC(true);
                        }
                    } else {
                        ctx->setFlagC(true);
                        if (aL < 10) {
                            ctx->reg.pair.A += 0x60;
                        } else {
                            ctx->reg.pair.A += 0x66;
                        }
                    }
                } else {
                    if (aH < 9) {
                        ctx->setFlagC(false);
                        ctx->reg.pair.A += 0x06;
                    } else if (aH == 9) {
                        if (aL < 10) {
                            ctx->reg.pair.A += 0x06;
                            ctx->setFlagC(false);
                        } else {
                            ctx->reg.pair.A += 0x66;
                            ctx->setFlagC(true);
                        }
                    } else {
                        ctx->setFlagC(true);
                        ctx->reg.pair.A += 0x66;
                    }
                }
            } else {
                ctx->setFlagC(true);
                if (!ctx->isFlagH()) {
                    if (aL < 10) {
                        ctx->reg.pair.A += 0x60;
                    } else {
                        ctx->reg.pair.A += 0x66;
                    }
                } else {
                    ctx->reg.pair.A += 0x66;
                }
            }
        } else {
            if (!ctx->isFlagC()) {
                if (!ctx->isFlagH()) {
                    if (aH < 9) {
                        ctx->setFlagC(false);
                        if (aL < 10) {
                            ;
                        } else {
                            ctx->reg.pair.A += 0xFA;
                        }
                    } else if (aH == 9) {
                        if (aL < 10) {
                            ctx->setFlagC(false);
                        } else {
                            ctx->reg.pair.A += 0x9A;
                            ctx->setFlagC(true);
                        }
                    } else {
                        ctx->setFlagC(true);
                        if (aL < 10) {
                            ctx->reg.pair.A += 0xA0;
                        } else {
                            ctx->reg.pair.A += 0x9A;
                        }
                    }
                } else {
                    if (aH < 9) {
                        ctx->setFlagC(false);
                        ctx->reg.pair.A += 0xFA;
                    } else if (aH == 9) {
                        if (aL < 10) {
                            ctx->reg.pair.A += 0xFA;
                            ctx->setFlagC(false);
                        } else {
                            ctx->reg.pair.A += 0x9A;
                            ctx->setFlagC(true);
                        }
                    } else {
                        ctx->setFlagC(true);
                        ctx->reg.pair.A += 0x9A;
                    }
                }
            } else {
                ctx->setFlagC(true);
                if (!ctx->isFlagH()) {
                    if (aL < 10) {
                        ctx->reg.pair.A += 0xA0;
                    } else {
                        ctx->reg.pair.A += 0x9A;
                    }
                } else {
                    ctx->reg.pair.A += 0x9A;
                }
            }
        }
        if (ctx->isDebug()) ctx->log("[%04X] DAA ... A: $%02X -> $%02X, flag-n: %s, flag-h: %s, flag-c: %s -> %s", ctx->reg.PC, beforeA, ctx->reg.pair.A, ctx->isFlagN() ? "ON" : "OFF", ctx->isFlagH() ? "ON" : "OFF", beforeCarry ? "ON" : "OFF", ctx->isFlagC() ? "ON" : "OFF");
        ctx->reg.PC++;
        return 0;
    }

    // Rotate digit Left and right between Acc. and location (HL)
    inline int RLD()
    {
        unsigned short hl = getHL();
        unsigned char beforeN = readByte(hl);
        unsigned char nH = (beforeN & 0b11110000) >> 4;
        unsigned char nL = beforeN & 0b00001111;
        unsigned char aH = (reg.pair.A & 0b11110000) >> 4;
        unsigned char aL = reg.pair.A & 0b00001111;
        unsigned char beforeA = reg.pair.A;
        unsigned char afterA = (aH << 4) | nH;
        unsigned char afterN = (nL << 4) | aL;
        if (isDebug()) log("[%04X] RLD ... A: $%02X -> $%02X, ($%04X): $%02X -> $%02X", reg.PC, beforeA, afterA, hl, beforeN, afterN);
        reg.pair.A = afterA;
        writeByte(hl, afterN);
        setFlagS(reg.pair.A & 0x80 ? true : false);
        setFlagZ(reg.pair.A == 0);
        setFlagH(false);
        setFlagPV(isEvenNumberBits(reg.pair.A));
        setFlagN(false);
        reg.PC += 2;
        return consumeClock(2);
    }

    // Rotate digit Right and right between Acc. and location (HL)
    inline int RRD()
    {
        unsigned short hl = getHL();
        unsigned char beforeN = readByte(hl);
        unsigned char nH = (beforeN & 0b11110000) >> 4;
        unsigned char nL = beforeN & 0b00001111;
        unsigned char aH = (reg.pair.A & 0b11110000) >> 4;
        unsigned char aL = reg.pair.A & 0b00001111;
        unsigned char beforeA = reg.pair.A;
        unsigned char afterA = (aH << 4) | nL;
        unsigned char afterN = (aL << 4) | nH;
        if (isDebug()) log("[%04X] RRD ... A: $%02X -> $%02X, ($%04X): $%02X -> $%02X", reg.PC, beforeA, afterA, hl, beforeN, afterN);
        reg.pair.A = afterA;
        writeByte(hl, afterN);
        setFlagS(reg.pair.A & 0x80 ? true : false);
        setFlagZ(reg.pair.A == 0);
        setFlagH(false);
        setFlagPV(isEvenNumberBits(reg.pair.A));
        setFlagN(false);
        reg.PC += 2;
        return consumeClock(2);
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
        opSet1[0b00010000] = DJNZ_E;
        opSet1[0b00010010] = LD_DE_A;
        opSet1[0b00010111] = RLA;
        opSet1[0b00011000] = JR_E;
        opSet1[0b00011010] = LD_A_DE;
        opSet1[0b00011111] = RRA;
        opSet1[0b00100000] = JR_NZ_E;
        opSet1[0b00100010] = LD_ADDR_HL;
        opSet1[0b00100111] = DAA;
        opSet1[0b00101000] = JR_Z_E;
        opSet1[0b00101010] = LD_HL_ADDR;
        opSet1[0b00101111] = CPL;
        opSet1[0b00110000] = JR_NC_E;
        opSet1[0b00110010] = LD_NN_A;
        opSet1[0b00110100] = INC_HL;
        opSet1[0b00110101] = DEC_HL;
        opSet1[0b00110110] = LD_HL_N;
        opSet1[0b00110111] = SCF;
        opSet1[0b00111000] = JR_C_E;
        opSet1[0b00111010] = LD_A_NN;
        opSet1[0b00111111] = CCF;
        opSet1[0b01110110] = HALT;
        opSet1[0b10000110] = ADD_A_HL;
        opSet1[0b10001110] = ADC_A_HL;
        opSet1[0b10010110] = SUB_A_HL;
        opSet1[0b10011110] = SBC_A_HL;
        opSet1[0b10100110] = AND_HL;
        opSet1[0b10101110] = XOR_HL;
        opSet1[0b10110110] = OR_HL;
        opSet1[0b10111110] = CP_HL;
        opSet1[0b11000011] = JP_NN;
        opSet1[0b11000110] = ADD_A_N;
        opSet1[0b11001001] = RET;
        opSet1[0b11001011] = OP_R;
        opSet1[0b11001101] = CALL_NN;
        opSet1[0b11001110] = ADC_A_N;
        opSet1[0b11010011] = OUT_N_A;
        opSet1[0b11010110] = SUB_A_N;
        opSet1[0b11011011] = IN_A_N;
        opSet1[0b11011110] = SBC_A_N;
        opSet1[0b11011001] = EXX;
        opSet1[0b11011101] = OP_IX;
        opSet1[0b11100011] = EX_SP_HL;
        opSet1[0b11100110] = AND_N;
        opSet1[0b11101001] = JP_HL;
        opSet1[0b11101011] = EX_DE_HL;
        opSet1[0b11101101] = EXTRA;
        opSet1[0b11101110] = XOR_N;
        opSet1[0b11110001] = POP_AF;
        opSet1[0b11110011] = DI;
        opSet1[0b11110101] = PUSH_AF;
        opSet1[0b11110110] = OR_N;
        opSet1[0b11111001] = LD_SP_HL;
        opSet1[0b11111011] = EI;
        opSet1[0b11111101] = OP_IY;
        opSet1[0b11111110] = CP_N;
    }

    inline void checkInterrupt()
    {
        // Interrupt processing is not executed by the instruction immediately after executing EI.
        if (reg.execEI) {
            return;
        }
        // check interrupt flag
        if (reg.interrupt & 0b10000000) {
            // execute NMI
            if (reg.IFF & IFF_NMI()) {
                return;
            }
            reg.interrupt &= 0b01111111;
            reg.IFF &= ~IFF_HALT();
            if (isDebug()) log("EXECUTE NMI: $%04X", reg.interruptAddrN);
            reg.R++;
            reg.IFF |= IFF_NMI();
            reg.IFF &= ~IFF1();
            unsigned char pcL = reg.PC & 0x00FF;
            unsigned char pcH = (reg.PC & 0xFF00) >> 8;
            writeByte(reg.SP - 1, pcH);
            writeByte(reg.SP - 2, pcL);
            reg.SP -= 2;
            reg.PC = reg.interruptAddrN;
            consumeClock(11);
        } else if (reg.interrupt & 0b01000000) {
            // execute IRQ
            if (!(reg.IFF & IFF1())) {
                return;
            }
            reg.interrupt &= 0b10111111;
            reg.IFF &= ~IFF_HALT();
            reg.IFF |= IFF_IRQ();
            reg.IFF &= ~(IFF1() | IFF2());
            switch (reg.interrupt & 0b00000011) {
                case 0: // mode 0
                    if (isDebug()) log("EXECUTE INT MODE1 (RST TO $%04X)", reg.interruptVector * 8);
                    if (reg.interruptVector == 0xCD) {
                        consumeClock(7);
                    }
                    RST(reg.interruptVector);
                    break;
                case 1: // mode 1 (13Hz)
                    if (isDebug()) log("EXECUTE INT MODE1 (RST TO $0038)");
                    consumeClock(1);
                    RST(7);
                    break;
                case 2: { // mode 2
                    unsigned char pcL = reg.PC & 0x00FF;
                    unsigned char pcH = (reg.PC & 0xFF00) >> 8;
                    writeByte(reg.SP - 1, pcH);
                    writeByte(reg.SP - 2, pcL);
                    reg.SP -= 2;
                    unsigned short addr = reg.I;
                    addr <<= 8;
                    addr |= reg.interruptVector;
                    unsigned short pc = readByte(addr);
                    pc += ((unsigned short)readByte(addr)) << 8;
                    if (isDebug()) log("EXECUTE INT MODE2: ($%04X) = $%04X", addr, pc);
                    reg.PC = pc;
                    consumeClock(3);
                    break;
                }
            }
        }
    }

  public: // API functions
    Z80(unsigned char (*read)(void* arg, unsigned short addr),
        void (*write)(void* arg, unsigned short addr, unsigned char value),
        unsigned char (*in)(void* arg, unsigned char port),
        void (*out)(void* arg, unsigned char port, unsigned char value),
        void* arg)
    {
        ::memset(&CB, 0, sizeof(CB));
        this->CB.read = read;
        this->CB.write = write;
        this->CB.in = in;
        this->CB.out = out;
        this->CB.arg = arg;
        ::memset(&reg, 0, sizeof(reg));
        setupOpSet1();
    }

    ~Z80() {}

    void setDebugMessage(void (*debugMessage)(void*, const char*) = NULL)
    {
        CB.debugMessage = debugMessage;
    }

    inline bool isDebug()
    {
        return CB.debugMessage != NULL;
    }

    void addBreakPoint(unsigned short addr, void (*callback)(void*) = NULL)
    {
        CB.breakPoints.push_back(new BreakPoint(addr, callback));
    }

    void removeBreakPoint(void (*callback)(void*))
    {
        int index = 0;
        for (auto bp : CB.breakPoints) {
            if (bp->callback == callback) {
                CB.breakPoints.erase(CB.breakPoints.begin() + index);
                delete bp;
                return;
            }
            index++;
        }
    }

    void removeAllBreakPoints()
    {
        for (auto bp : CB.breakPoints) delete bp;
        CB.breakPoints.clear();
    }

    void addBreakOperand(unsigned char operandNumber, void (*callback)(void*) = NULL)
    {
        CB.breakOperands.push_back(new BreakOperand(operandNumber, callback));
    }

    void removeBreakOperand(void (*callback)(void*))
    {
        int index = 0;
        for (auto bo : CB.breakOperands) {
            if (bo->callback == callback) {
                CB.breakOperands.erase(CB.breakOperands.begin() + index);
                delete bo;
                return;
            }
            index++;
        }
    }

    void removeAllBreakOperands()
    {
        for (auto bo : CB.breakOperands) delete bo;
        CB.breakOperands.clear();
    }

    void setConsumeClockCallback(void (*consumeClock)(void*, int) = NULL)
    {
        CB.consumeClock = consumeClock;
    }

    void requestBreak()
    {
        requestBreakFlag = true;
    }

    void generateIRQ(unsigned char vector)
    {
        reg.interrupt |= 0b01000000;
        reg.interruptVector = vector;
    }

    void generateNMI(unsigned short addr)
    {
        reg.interrupt |= 0b10000000;
        reg.interruptAddrN = addr;
    }

    inline int execute(int clock)
    {
        int executed = 0;
        requestBreakFlag = false;
        reg.consumeClockCounter = 0;
        while (0 < clock && !requestBreakFlag) {
            // execute NOP while halt
            if (reg.IFF & IFF_HALT()) {
                reg.execEI = 0;
                readByte(reg.PC); // NOTE: read and discard (to be consumed 4Hz)
            } else {
                checkBreakPoint();
                reg.execEI = 0;
                int operandNumber = readByte(reg.PC);
                checkBreakOperand(operandNumber);
                int (*op)(Z80*) = opSet1[operandNumber];
                int ret = -1;
                if (NULL == op) {
                    // execute an operand that register type has specified in the first byte.
                    if ((operandNumber & 0b11111000) == 0b01110000) {
                        ret = LD_HL_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11001111) == 0b00001001) {
                        ret = ADD_HL_RP((operandNumber & 0b00110000) >> 4);
                    } else if ((operandNumber & 0b11000111) == 0b00000110) {
                        ret = LD_R_N((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11001111) == 0b00000001) {
                        ret = LD_RP_NN((operandNumber & 0b00110000) >> 4);
                    } else if ((operandNumber & 0b11001111) == 0b11000101) {
                        ret = PUSH_RP((operandNumber & 0b00110000) >> 4);
                    } else if ((operandNumber & 0b11001111) == 0b11000001) {
                        ret = POP_RP((operandNumber & 0b00110000) >> 4);
                    } else if ((operandNumber & 0b11001111) == 0b00000011) {
                        ret = INC_RP((operandNumber & 0b00110000) >> 4);
                    } else if ((operandNumber & 0b11001111) == 0b00001011) {
                        ret = DEC_RP((operandNumber & 0b00110000) >> 4);
                    } else if ((operandNumber & 0b11000111) == 0b01000110) {
                        ret = LD_R_HL((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11000111) == 0b00000100) {
                        ret = INC_R((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11000111) == 0b00000101) {
                        ret = DEC_R((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11000111) == 0b11000010) {
                        ret = JP_C_NN((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11000111) == 0b11000100) {
                        ret = CALL_C_NN((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11000111) == 0b11000000) {
                        ret = RET_C((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11000111) == 0b11000111) {
                        ret = RST((operandNumber & 0b00111000) >> 3);
                    } else if ((operandNumber & 0b11000000) == 0b01000000) {
                        ret = LD_R1_R2((operandNumber & 0b00111000) >> 3, operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10000000) {
                        ret = ADD_A_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10001000) {
                        ret = ADC_A_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10010000) {
                        ret = SUB_A_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10011000) {
                        ret = SBC_A_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10100000) {
                        ret = AND_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10110000) {
                        ret = OR_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10101000) {
                        ret = XOR_R(operandNumber & 0b00000111);
                    } else if ((operandNumber & 0b11111000) == 0b10111000) {
                        ret = CP_R(operandNumber & 0b00000111);
                    }
                } else {
                    // execute an operand that the first byte is fixed.
                    ret = op(this);
                }
                if (ret < 0) {
                    if (isDebug()) log("[%04X] detected an invalid operand: $%02X", reg.PC, operandNumber);
                    return 0;
                }
            }
            executed += reg.consumeClockCounter;
            clock -= reg.consumeClockCounter;
            reg.consumeClockCounter = 0;
            checkInterrupt();
        }
        return executed;
    }

    int executeTick4MHz() { return execute(4194304 / 60); }

    int executeTick8MHz() { return execute(8388608 / 60); }

    void registerDump()
    {
        if (isDebug()) log("===== REGISTER DUMP : START =====");
        if (isDebug()) log("PAIR: %s %s %s %s %s %s %s", registerDump(0b111), registerDump(0b000), registerDump(0b001), registerDump(0b010), registerDump(0b011), registerDump(0b100), registerDump(0b101));
        if (isDebug()) log("PAIR: F<$%02X> ... S:%s, Z:%s, H:%s, P/V:%s, N:%s, C:%s",
                           reg.pair.F,
                           isFlagS() ? "ON" : "OFF",
                           isFlagZ() ? "ON" : "OFF",
                           isFlagH() ? "ON" : "OFF",
                           isFlagPV() ? "ON" : "OFF",
                           isFlagN() ? "ON" : "OFF",
                           isFlagC() ? "ON" : "OFF");
        if (isDebug()) log("BACK: %s %s %s %s %s %s %s F'<$%02X>", registerDump2(0b111), registerDump2(0b000), registerDump2(0b001), registerDump2(0b010), registerDump2(0b011), registerDump2(0b100), registerDump2(0b101), reg.back.F);
        if (isDebug()) log("PC<$%04X> SP<$%04X> IX<$%04X> IY<$%04X>", reg.PC, reg.SP, reg.IX, reg.IY);
        if (isDebug()) log("R<$%02X> I<$%02X> IFF<$%02X>", reg.R, reg.I, reg.IFF);
        if (isDebug()) log("isHalt: %s, interrupt: $%02X", reg.IFF & IFF_HALT() ? "YES" : "NO", reg.interrupt);
        if (isDebug()) log("===== REGISTER DUMP : END =====");
    }
};

#endif // INCLUDE_Z80_HPP
