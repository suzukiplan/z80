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
    struct WaitClocks {
        int fretch; // Wait T-cycle (Hz) before fetching instruction (default is 0 = no wait)
        int read;   // Wait T-cycle (Hz) before to read memory (default is 0 = no wait)
        int write;  // Wait T-cycle (Hz) before to write memory (default is 0 = no wait)
    } wtc;

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
        unsigned short interruptVector; // interrupt vector for IRQ
        unsigned short interruptAddrN;  // interrupt address for NMI
        unsigned short WZ;
        unsigned short reserved16;
        unsigned char R;
        unsigned char I;
        unsigned char IFF;
        unsigned char interrupt; // NI-- --mm (N: NMI, I: IRQ, mm: mode)
        unsigned char consumeClockCounter;
        unsigned char execEI;
        unsigned char reserved8[2];
    } reg;

    inline unsigned char flagS() { return 0b10000000; }
    inline unsigned char flagZ() { return 0b01000000; }
    inline unsigned char flagY() { return 0b00100000; }
    inline unsigned char flagH() { return 0b00010000; }
    inline unsigned char flagX() { return 0b00001000; }
    inline unsigned char flagPV() { return 0b00000100; }
    inline unsigned char flagN() { return 0b00000010; }
    inline unsigned char flagC() { return 0b00000001; }

    inline unsigned char readByte(unsigned short addr, int clock = 4)
    {
        if (wtc.read) consumeClock(wtc.read);
        unsigned char byte = CB.read(CB.arg, addr);
        consumeClock(clock);
        return byte;
    }

    inline void writeByte(unsigned short addr, unsigned char value, int clock = 4)
    {
        if (wtc.write) consumeClock(wtc.write);
        CB.write(CB.arg, addr, value);
        consumeClock(clock);
    }

  private: // Internal functions & variables
    // flag setter
    inline void setFlagS(bool on) { on ? reg.pair.F |= flagS() : reg.pair.F &= ~flagS(); }
    inline void setFlagZ(bool on) { on ? reg.pair.F |= flagZ() : reg.pair.F &= ~flagZ(); }
    inline void setFlagY(bool on) { on ? reg.pair.F |= flagY() : reg.pair.F &= ~flagY(); }
    inline void setFlagH(bool on) { on ? reg.pair.F |= flagH() : reg.pair.F &= ~flagH(); }
    inline void setFlagX(bool on) { on ? reg.pair.F |= flagX() : reg.pair.F &= ~flagX(); }
    inline void setFlagPV(bool on) { on ? reg.pair.F |= flagPV() : reg.pair.F &= ~flagPV(); }
    inline void setFlagN(bool on) { on ? reg.pair.F |= flagN() : reg.pair.F &= ~flagN(); }
    inline void setFlagC(bool on) { on ? reg.pair.F |= flagC() : reg.pair.F &= ~flagC(); }

    inline void setFlagXY(unsigned char value)
    {
        setFlagX(value & flagX());
        setFlagY(value & flagY());
    }

    // flag checker
    inline bool isFlagS() { return reg.pair.F & flagS(); }
    inline bool isFlagZ() { return reg.pair.F & flagZ(); }
    inline bool isFlagH() { return reg.pair.F & flagH(); }
    inline bool isFlagPV() { return reg.pair.F & flagPV(); }
    inline bool isFlagN() { return reg.pair.F & flagN(); }
    inline bool isFlagC() { return reg.pair.F & flagC(); }

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

    class ReturnHandler
    {
      public:
        void (*callback)(void* arg);
        ReturnHandler(void (*callback)(void* arg)) { this->callback = callback; }
    };

    inline void invokeReturnHandlers()
    {
        for (auto handler : this->CB.returnHandlers) {
            handler->callback(this->CB.arg);
        }
    }

    class CallHandler
    {
      public:
        void (*callback)(void* arg);
        CallHandler(void (*callback)(void* arg)) { this->callback = callback; }
    };

    inline void invokeCallHandlers()
    {
        for (auto handler : this->CB.callHandlers) {
            handler->callback(this->CB.arg);
        }
    }

    struct Callback {
        unsigned char (*read)(void* arg, unsigned short addr);
        void (*write)(void* arg, unsigned short addr, unsigned char value);
        unsigned char (*in)(void* arg, unsigned char port);
        void (*out)(void* arg, unsigned char port, unsigned char value);
        void (*debugMessage)(void* arg, const char* message);
        void (*consumeClock)(void* arg, int clock);
        std::vector<BreakPoint*> breakPoints;
        std::vector<BreakOperand*> breakOperands;
        std::vector<ReturnHandler*> returnHandlers;
        std::vector<CallHandler*> callHandlers;
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
            case 0b00: return getBC();
            case 0b01: return getDE();
            case 0b10: return getHL();
            default: return reg.SP;
        }
    }

    inline unsigned short getRPIX(unsigned char rp)
    {
        switch (rp & 0b11) {
            case 0b00: return (reg.pair.B << 8) + reg.pair.C;
            case 0b01: return (reg.pair.D << 8) + reg.pair.E;
            case 0b10: return reg.IX;
            default: return reg.SP;
        }
    }

    inline unsigned short getRPIY(unsigned char rp)
    {
        switch (rp & 0b11) {
            case 0b00: return (reg.pair.B << 8) + reg.pair.C;
            case 0b01: return (reg.pair.D << 8) + reg.pair.E;
            case 0b10: return reg.IY;
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

    inline unsigned char getIXH() { return (reg.IX & 0xFF00) >> 8; }
    inline unsigned char getIXL() { return reg.IX & 0x00Ff; }
    inline unsigned char getIYH() { return (reg.IY & 0xFF00) >> 8; }
    inline unsigned char getIYL() { return reg.IY & 0x00Ff; }
    inline void setIXH(unsigned char v) { reg.IX = (reg.IX & 0x00FF) + v * 256; }
    inline void setIXL(unsigned char v) { reg.IX = (reg.IX & 0xFF00) + v; }
    inline void setIYH(unsigned char v) { reg.IY = (reg.IY & 0x00FF) + v * 256; }
    inline void setIYL(unsigned char v) { reg.IY = (reg.IY & 0xFF00) + v; }

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
        setFlagPV(reg.IFF & IFF2());
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
        setFlagPV(reg.IFF & IFF1());
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
    static inline int OP_IX(Z80* ctx) { return ctx->opSetIX[ctx->readByte(ctx->reg.PC + 1)](ctx); }
    static inline int OP_IX4(Z80* ctx)
    {
        signed char op3 = ctx->readByte(ctx->reg.PC + 2);
        unsigned char op4 = ctx->readByte(ctx->reg.PC + 3);
        return ctx->opSetIX4[op4](ctx, op3);
    }

    // operand of using IY (first byte is 0b11111101)
    static inline int OP_IY(Z80* ctx) { return ctx->opSetIY[ctx->readByte(ctx->reg.PC + 1)](ctx); }
    static inline int OP_IY4(Z80* ctx)
    {
        signed char op3 = ctx->readByte(ctx->reg.PC + 2);
        unsigned char op4 = ctx->readByte(ctx->reg.PC + 3);
        return ctx->opSetIY4[op4](ctx, op3);
    }

    // operand of using other register (first byte is 0b11001011)
    static inline int OP_CB(Z80* ctx) { return ctx->opSetCB[ctx->readByte(ctx->reg.PC + 1)](ctx); }

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
            case 0b110: return &reg.pair.F;
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
        static char F[16];
        static char unknown[2];
        switch (r & 0b111) {
            case 0b111: sprintf(A, "A<$%02X>", reg.pair.A); return A;
            case 0b000: sprintf(B, "B<$%02X>", reg.pair.B); return B;
            case 0b001: sprintf(C, "C<$%02X>", reg.pair.C); return C;
            case 0b010: sprintf(D, "D<$%02X>", reg.pair.D); return D;
            case 0b011: sprintf(E, "E<$%02X>", reg.pair.E); return E;
            case 0b100: sprintf(H, "H<$%02X>", reg.pair.H); return H;
            case 0b101: sprintf(L, "L<$%02X>", reg.pair.L); return L;
            case 0b110: sprintf(F, "F<$%02X>", reg.pair.F); return F;
        }
        unknown[0] = '?';
        unknown[1] = '\0';
        return unknown;
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
            int ee = -e;
            ee -= 2;
            sprintf(buf, "$%04X - %d = $%04X", reg.PC, ee, reg.PC + e + 2);
        } else {
            sprintf(buf, "$%04X + %d = $%04X", reg.PC, e + 2, reg.PC + e + 2);
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

    inline char* registerPairDumpIX(unsigned char ptn)
    {
        static char BC[16];
        static char DE[16];
        static char IX[16];
        static char SP[16];
        static char unknown[2] = "?";
        switch (ptn & 0b11) {
            case 0b00: sprintf(BC, "BC<$%02X%02X>", reg.pair.B, reg.pair.C); return BC;
            case 0b01: sprintf(DE, "DE<$%02X%02X>", reg.pair.D, reg.pair.E); return DE;
            case 0b10: sprintf(IX, "IX<$%04X>", reg.IX); return IX;
            case 0b11: sprintf(SP, "SP<$%04X>", reg.SP); return SP;
            default: return unknown;
        }
    }

    inline char* registerPairDumpIY(unsigned char ptn)
    {
        static char BC[16];
        static char DE[16];
        static char IY[16];
        static char SP[16];
        static char unknown[2] = "?";
        switch (ptn & 0b11) {
            case 0b00: sprintf(BC, "BC<$%02X%02X>", reg.pair.B, reg.pair.C); return BC;
            case 0b01: sprintf(DE, "DE<$%02X%02X>", reg.pair.D, reg.pair.E); return DE;
            case 0b10: sprintf(IY, "IY<$%04X>", reg.IY); return IY;
            case 0b11: sprintf(SP, "SP<$%04X>", reg.SP); return SP;
            default: return unknown;
        }
    }

    // Load Reg. r1 with Reg. r2
    static inline int LD_B_B(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b000); }
    static inline int LD_B_C(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b001); }
    static inline int LD_B_D(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b010); }
    static inline int LD_B_E(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b011); }
    static inline int LD_B_B_2(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b000, 2); }
    static inline int LD_B_C_2(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b001, 2); }
    static inline int LD_B_D_2(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b010, 2); }
    static inline int LD_B_E_2(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b011, 2); }
    static inline int LD_B_H(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b100); }
    static inline int LD_B_L(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b101); }
    static inline int LD_B_A(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b111); }
    static inline int LD_C_B(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b000); }
    static inline int LD_C_C(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b001); }
    static inline int LD_C_D(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b010); }
    static inline int LD_C_E(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b011); }
    static inline int LD_B_A_2(Z80* ctx) { return ctx->LD_R1_R2(0b000, 0b111, 2); }
    static inline int LD_C_B_2(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b000, 2); }
    static inline int LD_C_C_2(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b001, 2); }
    static inline int LD_C_D_2(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b010, 2); }
    static inline int LD_C_E_2(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b011, 2); }
    static inline int LD_C_H(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b100); }
    static inline int LD_C_L(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b101); }
    static inline int LD_C_A(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b111); }
    static inline int LD_D_B(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b000); }
    static inline int LD_D_C(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b001); }
    static inline int LD_D_D(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b010); }
    static inline int LD_D_E(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b011); }
    static inline int LD_C_A_2(Z80* ctx) { return ctx->LD_R1_R2(0b001, 0b111, 2); }
    static inline int LD_D_B_2(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b000, 2); }
    static inline int LD_D_C_2(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b001, 2); }
    static inline int LD_D_D_2(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b010, 2); }
    static inline int LD_D_E_2(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b011, 2); }
    static inline int LD_D_H(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b100); }
    static inline int LD_D_L(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b101); }
    static inline int LD_D_A(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b111); }
    static inline int LD_E_B(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b000); }
    static inline int LD_E_C(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b001); }
    static inline int LD_E_D(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b010); }
    static inline int LD_E_E(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b011); }
    static inline int LD_D_A_2(Z80* ctx) { return ctx->LD_R1_R2(0b010, 0b111, 2); }
    static inline int LD_E_B_2(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b000, 2); }
    static inline int LD_E_C_2(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b001, 2); }
    static inline int LD_E_D_2(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b010, 2); }
    static inline int LD_E_E_2(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b011, 2); }
    static inline int LD_E_H(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b100); }
    static inline int LD_E_L(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b101); }
    static inline int LD_E_A(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b111); }
    static inline int LD_E_A_2(Z80* ctx) { return ctx->LD_R1_R2(0b011, 0b111, 2); }
    static inline int LD_H_B(Z80* ctx) { return ctx->LD_R1_R2(0b100, 0b000); }
    static inline int LD_H_C(Z80* ctx) { return ctx->LD_R1_R2(0b100, 0b001); }
    static inline int LD_H_D(Z80* ctx) { return ctx->LD_R1_R2(0b100, 0b010); }
    static inline int LD_H_E(Z80* ctx) { return ctx->LD_R1_R2(0b100, 0b011); }
    static inline int LD_H_H(Z80* ctx) { return ctx->LD_R1_R2(0b100, 0b100); }
    static inline int LD_H_L(Z80* ctx) { return ctx->LD_R1_R2(0b100, 0b101); }
    static inline int LD_H_A(Z80* ctx) { return ctx->LD_R1_R2(0b100, 0b111); }
    static inline int LD_L_B(Z80* ctx) { return ctx->LD_R1_R2(0b101, 0b000); }
    static inline int LD_L_C(Z80* ctx) { return ctx->LD_R1_R2(0b101, 0b001); }
    static inline int LD_L_D(Z80* ctx) { return ctx->LD_R1_R2(0b101, 0b010); }
    static inline int LD_L_E(Z80* ctx) { return ctx->LD_R1_R2(0b101, 0b011); }
    static inline int LD_L_H(Z80* ctx) { return ctx->LD_R1_R2(0b101, 0b100); }
    static inline int LD_L_L(Z80* ctx) { return ctx->LD_R1_R2(0b101, 0b101); }
    static inline int LD_L_A(Z80* ctx) { return ctx->LD_R1_R2(0b101, 0b111); }
    static inline int LD_A_B(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b000); }
    static inline int LD_A_C(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b001); }
    static inline int LD_A_D(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b010); }
    static inline int LD_A_E(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b011); }
    static inline int LD_A_B_2(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b000, 2); }
    static inline int LD_A_C_2(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b001, 2); }
    static inline int LD_A_D_2(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b010, 2); }
    static inline int LD_A_E_2(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b011, 2); }
    static inline int LD_A_H(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b100); }
    static inline int LD_A_L(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b101); }
    static inline int LD_A_A(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b111); }
    static inline int LD_A_A_2(Z80* ctx) { return ctx->LD_R1_R2(0b111, 0b111, 2); }
    inline int LD_R1_R2(unsigned char r1, unsigned char r2, int counter = 1)
    {
        unsigned char* r1p = getRegisterPointer(r1);
        unsigned char* r2p = getRegisterPointer(r2);
        if (isDebug()) log("[%04X] LD %s, %s", reg.PC, registerDump(r1), registerDump(r2));
        if (r1p && r2p) *r1p = *r2p;
        reg.PC += counter;
        return 0;
    }

    // Load Reg. r with value n
    static inline int LD_A_N(Z80* ctx) { return ctx->LD_R_N(0b111); }
    static inline int LD_B_N(Z80* ctx) { return ctx->LD_R_N(0b000); }
    static inline int LD_C_N(Z80* ctx) { return ctx->LD_R_N(0b001); }
    static inline int LD_D_N(Z80* ctx) { return ctx->LD_R_N(0b010); }
    static inline int LD_E_N(Z80* ctx) { return ctx->LD_R_N(0b011); }
    static inline int LD_H_N(Z80* ctx) { return ctx->LD_R_N(0b100); }
    static inline int LD_L_N(Z80* ctx) { return ctx->LD_R_N(0b101); }
    static inline int LD_A_N_3(Z80* ctx) { return ctx->LD_R_N(0b111, 3); }
    static inline int LD_B_N_3(Z80* ctx) { return ctx->LD_R_N(0b000, 3); }
    static inline int LD_C_N_3(Z80* ctx) { return ctx->LD_R_N(0b001, 3); }
    static inline int LD_D_N_3(Z80* ctx) { return ctx->LD_R_N(0b010, 3); }
    static inline int LD_E_N_3(Z80* ctx) { return ctx->LD_R_N(0b011, 3); }
    static inline int LD_H_N_3(Z80* ctx) { return ctx->LD_R_N(0b100, 3); }
    static inline int LD_L_N_3(Z80* ctx) { return ctx->LD_R_N(0b101, 3); }
    inline int LD_R_N(unsigned char r, int pc = 2)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char n = readByte(reg.PC + 1, 3);
        if (isDebug()) log("[%04X] LD %s, $%02X", reg.PC, registerDump(r), n);
        if (rp) *rp = n;
        reg.PC += pc;
        return 0;
    }

    // Load Reg. IX(high) with value n
    static inline int LD_IXH_N_(Z80* ctx) { return ctx->LD_IXH_N(); }
    inline int LD_IXH_N()
    {
        unsigned char n = readByte(reg.PC + 2, 3);
        if (isDebug()) log("[%04X] LD IXH, $%02X", reg.PC, n);
        setIXH(n);
        reg.PC += 3;
        return 0;
    }

    // Load Reg. IX(high) with value Reg.
    static inline int LD_IXH_A(Z80* ctx) { return ctx->LD_IXH_R(0b111); }
    static inline int LD_IXH_B(Z80* ctx) { return ctx->LD_IXH_R(0b000); }
    static inline int LD_IXH_C(Z80* ctx) { return ctx->LD_IXH_R(0b001); }
    static inline int LD_IXH_D(Z80* ctx) { return ctx->LD_IXH_R(0b010); }
    static inline int LD_IXH_E(Z80* ctx) { return ctx->LD_IXH_R(0b011); }
    inline int LD_IXH_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD IXH, %s", reg.PC, registerDump(r));
        setIXH(*rp);
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IX(high) with value IX(high)
    static inline int LD_IXH_IXH_(Z80* ctx) { return ctx->LD_IXH_IXH(); }
    inline int LD_IXH_IXH()
    {
        if (isDebug()) log("[%04X] LD IXH, IXH<$%02X>", reg.PC, getIXH());
        // nothing to do
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IX(high) with value IX(low)
    static inline int LD_IXH_IXL_(Z80* ctx) { return ctx->LD_IXH_IXL(); }
    inline int LD_IXH_IXL()
    {
        if (isDebug()) log("[%04X] LD IXH, IXL<$%02X>", reg.PC, getIXL());
        setIXH(getIXL());
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IX(low) with value n
    static inline int LD_IXL_N_(Z80* ctx) { return ctx->LD_IXL_N(); }
    inline int LD_IXL_N()
    {
        unsigned char n = readByte(reg.PC + 2, 3);
        if (isDebug()) log("[%04X] LD IXL, $%02X", reg.PC, n);
        setIXL(n);
        reg.PC += 3;
        return 0;
    }

    // Load Reg. IX(low) with value Reg.
    static inline int LD_IXL_A(Z80* ctx) { return ctx->LD_IXL_R(0b111); }
    static inline int LD_IXL_B(Z80* ctx) { return ctx->LD_IXL_R(0b000); }
    static inline int LD_IXL_C(Z80* ctx) { return ctx->LD_IXL_R(0b001); }
    static inline int LD_IXL_D(Z80* ctx) { return ctx->LD_IXL_R(0b010); }
    static inline int LD_IXL_E(Z80* ctx) { return ctx->LD_IXL_R(0b011); }
    inline int LD_IXL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD IXL, %s", reg.PC, registerDump(r));
        setIXL(*rp);
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IX(low) with value IX(high)
    static inline int LD_IXL_IXH_(Z80* ctx) { return ctx->LD_IXL_IXH(); }
    inline int LD_IXL_IXH()
    {
        if (isDebug()) log("[%04X] LD IXL, IXH<$%02X>", reg.PC, getIXH());
        setIXL(getIXH());
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IX(low) with value IX(low)
    static inline int LD_IXL_IXL_(Z80* ctx) { return ctx->LD_IXL_IXL(); }
    inline int LD_IXL_IXL()
    {
        if (isDebug()) log("[%04X] LD IXL, IXL<$%02X>", reg.PC, getIXL());
        // nothing to do
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IY(high) with value n
    static inline int LD_IYH_N_(Z80* ctx) { return ctx->LD_IYH_N(); }
    inline int LD_IYH_N()
    {
        unsigned char n = readByte(reg.PC + 2, 3);
        if (isDebug()) log("[%04X] LD IYH, $%02X", reg.PC, n);
        setIYH(n);
        reg.PC += 3;
        return 0;
    }

    // Load Reg. IY(high) with value Reg.
    static inline int LD_IYH_A(Z80* ctx) { return ctx->LD_IYH_R(0b111); }
    static inline int LD_IYH_B(Z80* ctx) { return ctx->LD_IYH_R(0b000); }
    static inline int LD_IYH_C(Z80* ctx) { return ctx->LD_IYH_R(0b001); }
    static inline int LD_IYH_D(Z80* ctx) { return ctx->LD_IYH_R(0b010); }
    static inline int LD_IYH_E(Z80* ctx) { return ctx->LD_IYH_R(0b011); }
    inline int LD_IYH_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD IYH, %s", reg.PC, registerDump(r));
        setIYH(*rp);
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IY(high) with value IY(high)
    static inline int LD_IYH_IYH_(Z80* ctx) { return ctx->LD_IYH_IYH(); }
    inline int LD_IYH_IYH()
    {
        if (isDebug()) log("[%04X] LD IYH, IYH<$%02X>", reg.PC, getIYH());
        // nothing to do
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IY(high) with value IY(low)
    static inline int LD_IYH_IYL_(Z80* ctx) { return ctx->LD_IYH_IYL(); }
    inline int LD_IYH_IYL()
    {
        if (isDebug()) log("[%04X] LD IYH, IYL<$%02X>", reg.PC, getIYL());
        setIYH(getIYL());
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IY(low) with value n
    static inline int LD_IYL_N_(Z80* ctx) { return ctx->LD_IYL_N(); }
    inline int LD_IYL_N()
    {
        unsigned char n = readByte(reg.PC + 2, 3);
        if (isDebug()) log("[%04X] LD IYL, $%02X", reg.PC, n);
        setIYL(n);
        reg.PC += 3;
        return 0;
    }

    // Load Reg. IY(low) with value Reg.
    static inline int LD_IYL_A(Z80* ctx) { return ctx->LD_IYL_R(0b111); }
    static inline int LD_IYL_B(Z80* ctx) { return ctx->LD_IYL_R(0b000); }
    static inline int LD_IYL_C(Z80* ctx) { return ctx->LD_IYL_R(0b001); }
    static inline int LD_IYL_D(Z80* ctx) { return ctx->LD_IYL_R(0b010); }
    static inline int LD_IYL_E(Z80* ctx) { return ctx->LD_IYL_R(0b011); }
    inline int LD_IYL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD IYL, %s", reg.PC, registerDump(r));
        setIYL(*rp);
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IY(low) with value IY(high)
    static inline int LD_IYL_IYH_(Z80* ctx) { return ctx->LD_IYL_IYH(); }
    inline int LD_IYL_IYH()
    {
        if (isDebug()) log("[%04X] LD IYL, IYH<$%02X>", reg.PC, getIYH());
        setIYL(getIYH());
        reg.PC += 2;
        return 0;
    }

    // Load Reg. IY(low) with value IY(low)
    static inline int LD_IYL_IYL_(Z80* ctx) { return ctx->LD_IYL_IYL(); }
    inline int LD_IYL_IYL()
    {
        if (isDebug()) log("[%04X] LD IYL, IYL<$%02X>", reg.PC, getIYL());
        // nothing to do
        reg.PC += 2;
        return 0;
    }

    // Load Reg. r with location (HL)
    static inline int LD_B_HL(Z80* ctx) { return ctx->LD_R_HL(0b000); }
    static inline int LD_C_HL(Z80* ctx) { return ctx->LD_R_HL(0b001); }
    static inline int LD_D_HL(Z80* ctx) { return ctx->LD_R_HL(0b010); }
    static inline int LD_E_HL(Z80* ctx) { return ctx->LD_R_HL(0b011); }
    static inline int LD_H_HL(Z80* ctx) { return ctx->LD_R_HL(0b100); }
    static inline int LD_L_HL(Z80* ctx) { return ctx->LD_R_HL(0b101); }
    static inline int LD_A_HL(Z80* ctx) { return ctx->LD_R_HL(0b111); }
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
    static inline int LD_A_IX(Z80* ctx) { return ctx->LD_R_IX(0b111); }
    static inline int LD_B_IX(Z80* ctx) { return ctx->LD_R_IX(0b000); }
    static inline int LD_C_IX(Z80* ctx) { return ctx->LD_R_IX(0b001); }
    static inline int LD_D_IX(Z80* ctx) { return ctx->LD_R_IX(0b010); }
    static inline int LD_E_IX(Z80* ctx) { return ctx->LD_R_IX(0b011); }
    static inline int LD_H_IX(Z80* ctx) { return ctx->LD_R_IX(0b100); }
    static inline int LD_L_IX(Z80* ctx) { return ctx->LD_R_IX(0b101); }
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

    // Load Reg. r with IXH
    static inline int LD_A_IXH(Z80* ctx) { return ctx->LD_R_IXH(0b111); }
    static inline int LD_B_IXH(Z80* ctx) { return ctx->LD_R_IXH(0b000); }
    static inline int LD_C_IXH(Z80* ctx) { return ctx->LD_R_IXH(0b001); }
    static inline int LD_D_IXH(Z80* ctx) { return ctx->LD_R_IXH(0b010); }
    static inline int LD_E_IXH(Z80* ctx) { return ctx->LD_R_IXH(0b011); }
    inline int LD_R_IXH(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD %s, IXH<$%02X>", reg.PC, registerDump(r), getIXH());
        if (rp) *rp = getIXH();
        reg.PC += 2;
        return 0;
    }

    // Load Reg. r with IXL
    static inline int LD_A_IXL(Z80* ctx) { return ctx->LD_R_IXL(0b111); }
    static inline int LD_B_IXL(Z80* ctx) { return ctx->LD_R_IXL(0b000); }
    static inline int LD_C_IXL(Z80* ctx) { return ctx->LD_R_IXL(0b001); }
    static inline int LD_D_IXL(Z80* ctx) { return ctx->LD_R_IXL(0b010); }
    static inline int LD_E_IXL(Z80* ctx) { return ctx->LD_R_IXL(0b011); }
    inline int LD_R_IXL(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD %s, IXL<$%02X>", reg.PC, registerDump(r), getIXL());
        if (rp) *rp = getIXL();
        reg.PC += 2;
        return 0;
    }

    // Load Reg. r with location (IY+d)
    static inline int LD_A_IY(Z80* ctx) { return ctx->LD_R_IY(0b111); }
    static inline int LD_B_IY(Z80* ctx) { return ctx->LD_R_IY(0b000); }
    static inline int LD_C_IY(Z80* ctx) { return ctx->LD_R_IY(0b001); }
    static inline int LD_D_IY(Z80* ctx) { return ctx->LD_R_IY(0b010); }
    static inline int LD_E_IY(Z80* ctx) { return ctx->LD_R_IY(0b011); }
    static inline int LD_H_IY(Z80* ctx) { return ctx->LD_R_IY(0b100); }
    static inline int LD_L_IY(Z80* ctx) { return ctx->LD_R_IY(0b101); }
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

    // Load Reg. r with IYH
    static inline int LD_A_IYH(Z80* ctx) { return ctx->LD_R_IYH(0b111); }
    static inline int LD_B_IYH(Z80* ctx) { return ctx->LD_R_IYH(0b000); }
    static inline int LD_C_IYH(Z80* ctx) { return ctx->LD_R_IYH(0b001); }
    static inline int LD_D_IYH(Z80* ctx) { return ctx->LD_R_IYH(0b010); }
    static inline int LD_E_IYH(Z80* ctx) { return ctx->LD_R_IYH(0b011); }
    inline int LD_R_IYH(unsigned char r)
    {
        unsigned char iyh = (reg.IY & 0xFF00) >> 8;
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD %s, IYH<$%02X>", reg.PC, registerDump(r), iyh);
        if (rp) *rp = iyh;
        reg.PC += 2;
        return 0;
    }

    // Load Reg. r with IYL
    static inline int LD_A_IYL(Z80* ctx) { return ctx->LD_R_IYL(0b111); }
    static inline int LD_B_IYL(Z80* ctx) { return ctx->LD_R_IYL(0b000); }
    static inline int LD_C_IYL(Z80* ctx) { return ctx->LD_R_IYL(0b001); }
    static inline int LD_D_IYL(Z80* ctx) { return ctx->LD_R_IYL(0b010); }
    static inline int LD_E_IYL(Z80* ctx) { return ctx->LD_R_IYL(0b011); }
    inline int LD_R_IYL(unsigned char r)
    {
        unsigned char iyl = reg.IY & 0x00FF;
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] LD %s, IYL<$%02X>", reg.PC, registerDump(r), iyl);
        if (rp) *rp = iyl;
        reg.PC += 2;
        return 0;
    }

    // Load location (HL) with Reg. r
    static inline int LD_HL_B(Z80* ctx) { return ctx->LD_HL_R(0b000); }
    static inline int LD_HL_C(Z80* ctx) { return ctx->LD_HL_R(0b001); }
    static inline int LD_HL_D(Z80* ctx) { return ctx->LD_HL_R(0b010); }
    static inline int LD_HL_E(Z80* ctx) { return ctx->LD_HL_R(0b011); }
    static inline int LD_HL_H(Z80* ctx) { return ctx->LD_HL_R(0b100); }
    static inline int LD_HL_L(Z80* ctx) { return ctx->LD_HL_R(0b101); }
    static inline int LD_HL_A(Z80* ctx) { return ctx->LD_HL_R(0b111); }
    inline int LD_HL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned short addr = getHL();
        if (isDebug()) log("[%04X] LD (%s), %s", reg.PC, registerPairDump(0b10), registerDump(r));
        writeByte(addr, *rp, 3);
        reg.PC += 1;
        return 0;
    }

    // 	Load location (IX+d) with Reg. r
    static inline int LD_IX_A(Z80* ctx) { return ctx->LD_IX_R(0b111); }
    static inline int LD_IX_B(Z80* ctx) { return ctx->LD_IX_R(0b000); }
    static inline int LD_IX_C(Z80* ctx) { return ctx->LD_IX_R(0b001); }
    static inline int LD_IX_D(Z80* ctx) { return ctx->LD_IX_R(0b010); }
    static inline int LD_IX_E(Z80* ctx) { return ctx->LD_IX_R(0b011); }
    static inline int LD_IX_H(Z80* ctx) { return ctx->LD_IX_R(0b100); }
    static inline int LD_IX_L(Z80* ctx) { return ctx->LD_IX_R(0b101); }
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
    static inline int LD_IY_A(Z80* ctx) { return ctx->LD_IY_R(0b111); }
    static inline int LD_IY_B(Z80* ctx) { return ctx->LD_IY_R(0b000); }
    static inline int LD_IY_C(Z80* ctx) { return ctx->LD_IY_R(0b001); }
    static inline int LD_IY_D(Z80* ctx) { return ctx->LD_IY_R(0b010); }
    static inline int LD_IY_E(Z80* ctx) { return ctx->LD_IY_R(0b011); }
    static inline int LD_IY_H(Z80* ctx) { return ctx->LD_IY_R(0b100); }
    static inline int LD_IY_L(Z80* ctx) { return ctx->LD_IY_R(0b101); }
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
    static inline int LD_IX_N_(Z80* ctx) { return ctx->LD_IX_N(); }
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
    static inline int LD_IY_N_(Z80* ctx) { return ctx->LD_IY_N(); }
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
    static inline int LD_BC_NN(Z80* ctx) { return ctx->LD_RP_NN(0b00); }
    static inline int LD_DE_NN(Z80* ctx) { return ctx->LD_RP_NN(0b01); }
    static inline int LD_HL_NN(Z80* ctx) { return ctx->LD_RP_NN(0b10); }
    static inline int LD_SP_NN(Z80* ctx) { return ctx->LD_RP_NN(0b11); }
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

    static inline int LD_IX_NN_(Z80* ctx) { return ctx->LD_IX_NN(); }
    inline int LD_IX_NN()
    {
        unsigned char nL = readByte(reg.PC + 2, 3);
        unsigned char nH = readByte(reg.PC + 3, 3);
        if (isDebug()) log("[%04X] LD IX, $%02X%02X", reg.PC, nH, nL);
        reg.IX = (nH << 8) + nL;
        reg.PC += 4;
        return 0;
    }

    static inline int LD_IY_NN_(Z80* ctx) { return ctx->LD_IY_NN(); }
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
        reg.WZ = addr + 1;
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
        reg.WZ = addr + 1;
        reg.PC += 4;
        return 0;
    }

    // Load IX with location (nn)
    static inline int LD_IX_ADDR_(Z80* ctx) { return ctx->LD_IX_ADDR(); }
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
    static inline int LD_IY_ADDR_(Z80* ctx) { return ctx->LD_IY_ADDR(); }
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

    static inline int LD_ADDR_IX_(Z80* ctx) { return ctx->LD_ADDR_IX(); }
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

    static inline int LD_ADDR_IY_(Z80* ctx) { return ctx->LD_ADDR_IY(); }
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
    static inline int LD_SP_IX_(Z80* ctx) { return ctx->LD_SP_IX(); }
    inline int LD_SP_IX()
    {
        unsigned short value = reg.IX;
        if (isDebug()) log("[%04X] LD %s, IX<$%04X>", reg.PC, registerPairDump(0b11), value);
        reg.SP = value;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Load SP with IY.
    static inline int LD_SP_IY_(Z80* ctx) { return ctx->LD_SP_IY(); }
    inline int LD_SP_IY()
    {
        unsigned short value = reg.IY;
        if (isDebug()) log("[%04X] LD %s, IY<$%04X>", reg.PC, registerPairDump(0b11), value);
        reg.SP = value;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Load location (DE) with Loacation (HL), increment/decrement DE, HL, decrement BC
    inline int repeatLD(bool isIncDEHL, bool isRepeat)
    {
        if (isDebug()) {
            if (isIncDEHL) {
                if (isDebug()) log("[%04X] %s ... %s, %s, %s", reg.PC, isRepeat ? "LDIR" : "LDI", registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
            } else {
                if (isDebug()) log("[%04X] %s ... %s, %s, %s", reg.PC, isRepeat ? "LDDR" : "LDD", registerPairDump(0b00), registerPairDump(0b01), registerPairDump(0b10));
            }
        }
        unsigned short bc = getBC();
        unsigned short de = getDE();
        unsigned short hl = getHL();
        unsigned char n = readByte(hl);
        writeByte(de, n);
        if (isIncDEHL) {
            de++;
            hl++;
        } else {
            de--;
            hl--;
        }
        bc--;
        setBC(bc);
        setDE(de);
        setHL(hl);
        setFlagH(false);
        setFlagPV(bc != 0);
        setFlagN(false);
        unsigned char an = reg.pair.A + n;
        setFlagY(an & 0b00000010);
        setFlagX(an & 0b00001000);
        if (isRepeat && 0 != bc) {
            consumeClock(5);
        } else {
            reg.PC += 2;
        }
        return 0;
    }
    inline int LDI() { return repeatLD(true, false); }
    inline int LDIR() { return repeatLD(true, true); }
    inline int LDD() { return repeatLD(false, false); }
    inline int LDDR() { return repeatLD(false, true); }

    // Exchange stack top with IX
    static inline int EX_SP_IX_(Z80* ctx) { return ctx->EX_SP_IX(); }
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
    static inline int EX_SP_IY_(Z80* ctx) { return ctx->EX_SP_IY(); }
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
    static inline int PUSH_BC(Z80* ctx) { return ctx->PUSH_RP(0b00); }
    static inline int PUSH_DE(Z80* ctx) { return ctx->PUSH_RP(0b01); }
    static inline int PUSH_HL(Z80* ctx) { return ctx->PUSH_RP(0b10); }
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
    static inline int POP_BC(Z80* ctx) { return ctx->POP_RP(0b00); }
    static inline int POP_DE(Z80* ctx) { return ctx->POP_RP(0b01); }
    static inline int POP_HL(Z80* ctx) { return ctx->POP_RP(0b10); }
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
        unsigned char lm = readByte(reg.SP++, 3);
        unsigned char hm = readByte(reg.SP++, 3);
        if (isDebug()) log("[%04X] POP %s <SP:$%04X> = $%02X%02X", reg.PC, registerPairDump(rp), sp, hm, lm);
        *l = lm;
        *h = hm;
        reg.PC++;
        return 0;
    }

    // Push Reg. IX on Stack.
    static inline int PUSH_IX_(Z80* ctx) { return ctx->PUSH_IX(); }
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
    static inline int POP_IX_(Z80* ctx) { return ctx->POP_IX(); }
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
    static inline int PUSH_IY_(Z80* ctx) { return ctx->PUSH_IY(); }
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
    static inline int POP_IY_(Z80* ctx) { return ctx->POP_IY(); }
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

    inline void setFlagByRotate(unsigned char n, bool carry, bool isA = false)
    {
        setFlagC(carry);
        setFlagH(false);
        setFlagN(false);
        setFlagXY(n);
        if (!isA) {
            setFlagS(n & 0x80);
            setFlagZ(0 == n);
            setFlagPV(isEvenNumberBits(n));
        }
    }

    inline unsigned char SLL(unsigned char n)
    {
        unsigned char c = n & 0x80;
        n &= 0b01111111;
        n <<= 1;
        n |= 1; // differ with SLA
        setFlagByRotate(n, c);
        return n;
    }

    inline unsigned char SLA(unsigned char n)
    {
        unsigned char c = n & 0x80 ? 1 : 0;
        n &= 0b01111111;
        n <<= 1;
        setFlagByRotate(n, c);
        return n;
    }

    inline unsigned char SRL(unsigned char n)
    {
        unsigned char n0 = n & 0x01;
        n &= 0b11111110;
        n >>= 1;
        setFlagByRotate(n, n0);
        return n;
    }

    inline unsigned char SRA(unsigned char n)
    {
        unsigned char n0 = n & 0x01;
        unsigned char n7 = n & 0x80;
        n &= 0b11111110;
        n >>= 1;
        n = n7 ? n | 0x80 : n & 0x7F;
        setFlagByRotate(n, n0);
        return n;
    }

    inline unsigned char RLC(unsigned char n, bool isA = false)
    {
        unsigned char c = n & 0x80 ? 1 : 0;
        n &= 0b01111111;
        n <<= 1;
        n |= c; // differ with RL
        setFlagByRotate(n, c, isA);
        return n;
    }

    inline unsigned char RL(unsigned char n, bool isA = false)
    {
        unsigned char c = n & 0x80 ? 1 : 0;
        n &= 0b01111111;
        n <<= 1;
        n |= isFlagC() ? 1 : 0; // differ with RLC
        setFlagByRotate(n, c, isA);
        return n;
    }

    inline unsigned char RRC(unsigned char n, bool isA = false)
    {
        unsigned char c = n & 1 ? 0x80 : 0;
        n &= 0b11111110;
        n >>= 1;
        n |= c; // differ with RR
        setFlagByRotate(n, c, isA);
        return n;
    }

    inline unsigned char RR(unsigned char n, bool isA = false)
    {
        unsigned char c = n & 1 ? 0x80 : 0;
        n &= 0b11111110;
        n >>= 1;
        n |= isFlagC() ? 0x80 : 0; // differ with RR
        setFlagByRotate(n, c, isA);
        return n;
    }

    static inline int RLCA(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] RLCA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, ctx->isFlagC() ? "ON" : "OFF");
        ctx->reg.pair.A = ctx->RLC(ctx->reg.pair.A, true);
        ctx->reg.PC++;
        return 0;
    }

    static inline int RRCA(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] RRCA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, ctx->isFlagC() ? "ON" : "OFF");
        ctx->reg.pair.A = ctx->RRC(ctx->reg.pair.A, true);
        ctx->reg.PC++;
        return 0;
    }

    static inline int RLA(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] RLA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, ctx->isFlagC() ? "ON" : "OFF");
        ctx->reg.pair.A = ctx->RL(ctx->reg.pair.A, true);
        ctx->reg.PC++;
        return 0;
    }

    static inline int RRA(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] RRA <A:$%02X, C:%s>", ctx->reg.PC, ctx->reg.pair.A, ctx->isFlagC() ? "ON" : "OFF");
        ctx->reg.pair.A = ctx->RR(ctx->reg.pair.A, true);
        ctx->reg.PC++;
        return 0;
    }

    // Rotate register Left Circular
    static inline int RLC_B(Z80* ctx) { return ctx->RLC_R(0b000); }
    static inline int RLC_C(Z80* ctx) { return ctx->RLC_R(0b001); }
    static inline int RLC_D(Z80* ctx) { return ctx->RLC_R(0b010); }
    static inline int RLC_E(Z80* ctx) { return ctx->RLC_R(0b011); }
    static inline int RLC_H(Z80* ctx) { return ctx->RLC_R(0b100); }
    static inline int RLC_L(Z80* ctx) { return ctx->RLC_R(0b101); }
    static inline int RLC_A(Z80* ctx) { return ctx->RLC_R(0b111); }
    inline int RLC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] RLC %s", reg.PC, registerDump(r));
        *rp = RLC(*rp);
        reg.PC += 2;
        return 0;
    }

    // Rotate Left register
    static inline int RL_B(Z80* ctx) { return ctx->RL_R(0b000); }
    static inline int RL_C(Z80* ctx) { return ctx->RL_R(0b001); }
    static inline int RL_D(Z80* ctx) { return ctx->RL_R(0b010); }
    static inline int RL_E(Z80* ctx) { return ctx->RL_R(0b011); }
    static inline int RL_H(Z80* ctx) { return ctx->RL_R(0b100); }
    static inline int RL_L(Z80* ctx) { return ctx->RL_R(0b101); }
    static inline int RL_A(Z80* ctx) { return ctx->RL_R(0b111); }
    inline int RL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] RL %s <C:%s>", reg.PC, registerDump(r), isFlagC() ? "ON" : "OFF");
        *rp = RL(*rp);
        reg.PC += 2;
        return 0;
    }

    // Shift operand register left Arithmetic
    static inline int SLA_B(Z80* ctx) { return ctx->SLA_R(0b000); }
    static inline int SLA_C(Z80* ctx) { return ctx->SLA_R(0b001); }
    static inline int SLA_D(Z80* ctx) { return ctx->SLA_R(0b010); }
    static inline int SLA_E(Z80* ctx) { return ctx->SLA_R(0b011); }
    static inline int SLA_H(Z80* ctx) { return ctx->SLA_R(0b100); }
    static inline int SLA_L(Z80* ctx) { return ctx->SLA_R(0b101); }
    static inline int SLA_A(Z80* ctx) { return ctx->SLA_R(0b111); }
    inline int SLA_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] SLA %s", reg.PC, registerDump(r));
        *rp = SLA(*rp);
        reg.PC += 2;
        return 0;
    }

    // Rotate register Right Circular
    static inline int RRC_B(Z80* ctx) { return ctx->RRC_R(0b000); }
    static inline int RRC_C(Z80* ctx) { return ctx->RRC_R(0b001); }
    static inline int RRC_D(Z80* ctx) { return ctx->RRC_R(0b010); }
    static inline int RRC_E(Z80* ctx) { return ctx->RRC_R(0b011); }
    static inline int RRC_H(Z80* ctx) { return ctx->RRC_R(0b100); }
    static inline int RRC_L(Z80* ctx) { return ctx->RRC_R(0b101); }
    static inline int RRC_A(Z80* ctx) { return ctx->RRC_R(0b111); }
    inline int RRC_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] RRC %s", reg.PC, registerDump(r));
        *rp = RRC(*rp);
        reg.PC += 2;
        return 0;
    }

    // Rotate Right register
    static inline int RR_B(Z80* ctx) { return ctx->RR_R(0b000); }
    static inline int RR_C(Z80* ctx) { return ctx->RR_R(0b001); }
    static inline int RR_D(Z80* ctx) { return ctx->RR_R(0b010); }
    static inline int RR_E(Z80* ctx) { return ctx->RR_R(0b011); }
    static inline int RR_H(Z80* ctx) { return ctx->RR_R(0b100); }
    static inline int RR_L(Z80* ctx) { return ctx->RR_R(0b101); }
    static inline int RR_A(Z80* ctx) { return ctx->RR_R(0b111); }
    inline int RR_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] RR %s <C:%s>", reg.PC, registerDump(r), isFlagC() ? "ON" : "OFF");
        *rp = RR(*rp);
        reg.PC += 2;
        return 0;
    }

    // Shift operand register Right Arithmetic
    static inline int SRA_B(Z80* ctx) { return ctx->SRA_R(0b000); }
    static inline int SRA_C(Z80* ctx) { return ctx->SRA_R(0b001); }
    static inline int SRA_D(Z80* ctx) { return ctx->SRA_R(0b010); }
    static inline int SRA_E(Z80* ctx) { return ctx->SRA_R(0b011); }
    static inline int SRA_H(Z80* ctx) { return ctx->SRA_R(0b100); }
    static inline int SRA_L(Z80* ctx) { return ctx->SRA_R(0b101); }
    static inline int SRA_A(Z80* ctx) { return ctx->SRA_R(0b111); }
    inline int SRA_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] SRA %s", reg.PC, registerDump(r));
        *rp = SRA(*rp);
        reg.PC += 2;
        return 0;
    }

    // Shift operand register Right Logical
    static inline int SRL_B(Z80* ctx) { return ctx->SRL_R(0b000); }
    static inline int SRL_C(Z80* ctx) { return ctx->SRL_R(0b001); }
    static inline int SRL_D(Z80* ctx) { return ctx->SRL_R(0b010); }
    static inline int SRL_E(Z80* ctx) { return ctx->SRL_R(0b011); }
    static inline int SRL_H(Z80* ctx) { return ctx->SRL_R(0b100); }
    static inline int SRL_L(Z80* ctx) { return ctx->SRL_R(0b101); }
    static inline int SRL_A(Z80* ctx) { return ctx->SRL_R(0b111); }
    inline int SRL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] SRL %s", reg.PC, registerDump(r));
        *rp = SRL(*rp);
        reg.PC += 2;
        return 0;
    }

    // Shift operand register Left Logical
    static inline int SLL_B(Z80* ctx) { return ctx->SLL_R(0b000); }
    static inline int SLL_C(Z80* ctx) { return ctx->SLL_R(0b001); }
    static inline int SLL_D(Z80* ctx) { return ctx->SLL_R(0b010); }
    static inline int SLL_E(Z80* ctx) { return ctx->SLL_R(0b011); }
    static inline int SLL_H(Z80* ctx) { return ctx->SLL_R(0b100); }
    static inline int SLL_L(Z80* ctx) { return ctx->SLL_R(0b101); }
    static inline int SLL_A(Z80* ctx) { return ctx->SLL_R(0b111); }
    inline int SLL_R(unsigned char r)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] SLL %s", reg.PC, registerDump(r));
        *rp = SLL(*rp);
        reg.PC += 2;
        return 0;
    }

    // Rotate memory (HL) Left Circular
    static inline int RLC_HL_(Z80* ctx) { return ctx->RLC_HL(); }
    inline int RLC_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RLC (HL<$%04X>) = $%02X", reg.PC, addr, n);
        writeByte(addr, RLC(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Rotate Left memory
    static inline int RL_HL_(Z80* ctx) { return ctx->RL_HL(); }
    inline int RL_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RL (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, isFlagC() ? "ON" : "OFF");
        writeByte(addr, RL(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Shift operand location (HL) left Arithmetic
    static inline int SLA_HL_(Z80* ctx) { return ctx->SLA_HL(); }
    inline int SLA_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SLA (HL<$%04X>) = $%02X", reg.PC, addr, n);
        writeByte(addr, SLA(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Rotate memory (HL) Right Circular
    static inline int RRC_HL_(Z80* ctx) { return ctx->RRC_HL(); }
    inline int RRC_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RRC (HL<$%04X>) = $%02X", reg.PC, addr, n);
        writeByte(addr, RRC(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Rotate Right memory
    static inline int RR_HL_(Z80* ctx) { return ctx->RR_HL(); }
    inline int RR_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RR (HL<$%04X>) = $%02X <C:%s>", reg.PC, addr, n, isFlagC() ? "ON" : "OFF");
        writeByte(addr, RR(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Shift operand location (HL) Right Arithmetic
    static inline int SRA_HL_(Z80* ctx) { return ctx->SRA_HL(); }
    inline int SRA_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SRA (HL<$%04X>) = $%02X", reg.PC, addr, n);
        writeByte(addr, SRA(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Shift operand location (HL) Right Logical
    static inline int SRL_HL_(Z80* ctx) { return ctx->SRL_HL(); }
    inline int SRL_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SRL (HL<$%04X>) = $%02X", reg.PC, addr, n);
        writeByte(addr, SRL(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Shift operand location (HL) Left Logical
    static inline int SLL_HL_(Z80* ctx) { return ctx->SLL_HL(); }
    inline int SLL_HL()
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SLL (HL<$%04X>) = $%02X", reg.PC, addr, n);
        writeByte(addr, SLL(n), 3);
        reg.PC += 2;
        return 0;
    }

    // Rotate memory (IX+d) Left Circular
    static inline int RLC_IX_(Z80* ctx, signed char d) { return ctx->RLC_IX(d); }
    inline int RLC_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RLC (IX+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = RLC(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Rotate memory (IX+d) Left Circular with load to Reg A/B/C/D/E/H/L/F
    static inline int RLC_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->RLC_IX_with_LD(d, 0b000); }
    static inline int RLC_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->RLC_IX_with_LD(d, 0b001); }
    static inline int RLC_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->RLC_IX_with_LD(d, 0b010); }
    static inline int RLC_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->RLC_IX_with_LD(d, 0b011); }
    static inline int RLC_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->RLC_IX_with_LD(d, 0b100); }
    static inline int RLC_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->RLC_IX_with_LD(d, 0b101); }
    static inline int RLC_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->RLC_IX_with_LD(d, 0b111); }
    inline int RLC_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RLC_IX(d, rp, buf);
    }

    // Rotate memory (IY+d) Left Circular with load to Reg A/B/C/D/E/H/L/F
    static inline int RLC_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->RLC_IY_with_LD(d, 0b000); }
    static inline int RLC_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->RLC_IY_with_LD(d, 0b001); }
    static inline int RLC_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->RLC_IY_with_LD(d, 0b010); }
    static inline int RLC_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->RLC_IY_with_LD(d, 0b011); }
    static inline int RLC_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->RLC_IY_with_LD(d, 0b100); }
    static inline int RLC_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->RLC_IY_with_LD(d, 0b101); }
    static inline int RLC_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->RLC_IY_with_LD(d, 0b111); }
    inline int RLC_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RLC_IY(d, rp, buf);
    }

    // Rotate memory (IX+d) Right Circular
    static inline int RRC_IX_(Z80* ctx, signed char d) { return ctx->RRC_IX(d); }
    inline int RRC_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RRC (IX+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = RRC(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Rotate memory (IX+d) Right Circular with load to Reg A/B/C/D/E/H/L/F
    static inline int RRC_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->RRC_IX_with_LD(d, 0b000); }
    static inline int RRC_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->RRC_IX_with_LD(d, 0b001); }
    static inline int RRC_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->RRC_IX_with_LD(d, 0b010); }
    static inline int RRC_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->RRC_IX_with_LD(d, 0b011); }
    static inline int RRC_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->RRC_IX_with_LD(d, 0b100); }
    static inline int RRC_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->RRC_IX_with_LD(d, 0b101); }
    static inline int RRC_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->RRC_IX_with_LD(d, 0b111); }
    inline int RRC_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RRC_IX(d, rp, buf);
    }

    // Rotate memory (IY+d) Right Circular with load to Reg A/B/C/D/E/H/L/F
    static inline int RRC_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->RRC_IY_with_LD(d, 0b000); }
    static inline int RRC_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->RRC_IY_with_LD(d, 0b001); }
    static inline int RRC_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->RRC_IY_with_LD(d, 0b010); }
    static inline int RRC_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->RRC_IY_with_LD(d, 0b011); }
    static inline int RRC_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->RRC_IY_with_LD(d, 0b100); }
    static inline int RRC_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->RRC_IY_with_LD(d, 0b101); }
    static inline int RRC_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->RRC_IY_with_LD(d, 0b111); }
    inline int RRC_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RRC_IY(d, rp, buf);
    }

    // Rotate Left memory
    static inline int RL_IX_(Z80* ctx, signed char d) { return ctx->RL_IX(d); }
    inline int RL_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RL (IX+d<$%04X>) = $%02X <C:%s>%s", reg.PC, addr, n, isFlagC() ? "ON" : "OFF", extraLog ? extraLog : "");
        unsigned char result = RL(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Rotate Left memory with load Reg.
    static inline int RL_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->RL_IX_with_LD(d, 0b000); }
    static inline int RL_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->RL_IX_with_LD(d, 0b001); }
    static inline int RL_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->RL_IX_with_LD(d, 0b010); }
    static inline int RL_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->RL_IX_with_LD(d, 0b011); }
    static inline int RL_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->RL_IX_with_LD(d, 0b100); }
    static inline int RL_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->RL_IX_with_LD(d, 0b101); }
    static inline int RL_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->RL_IX_with_LD(d, 0b111); }
    inline int RL_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RL_IX(d, rp, buf);
    }

    // Rotate Left memory with load Reg.
    static inline int RL_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->RL_IY_with_LD(d, 0b000); }
    static inline int RL_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->RL_IY_with_LD(d, 0b001); }
    static inline int RL_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->RL_IY_with_LD(d, 0b010); }
    static inline int RL_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->RL_IY_with_LD(d, 0b011); }
    static inline int RL_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->RL_IY_with_LD(d, 0b100); }
    static inline int RL_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->RL_IY_with_LD(d, 0b101); }
    static inline int RL_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->RL_IY_with_LD(d, 0b111); }
    inline int RL_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RL_IY(d, rp, buf);
    }

    // Rotate Right memory
    static inline int RR_IX_(Z80* ctx, signed char d) { return ctx->RR_IX(d); }
    inline int RR_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RR (IX+d<$%04X>) = $%02X <C:%s>%s", reg.PC, addr, n, isFlagC() ? "ON" : "OFF", extraLog ? extraLog : "");
        unsigned char result = RR(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Rotate Right memory with load Reg.
    static inline int RR_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->RR_IX_with_LD(d, 0b000); }
    static inline int RR_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->RR_IX_with_LD(d, 0b001); }
    static inline int RR_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->RR_IX_with_LD(d, 0b010); }
    static inline int RR_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->RR_IX_with_LD(d, 0b011); }
    static inline int RR_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->RR_IX_with_LD(d, 0b100); }
    static inline int RR_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->RR_IX_with_LD(d, 0b101); }
    static inline int RR_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->RR_IX_with_LD(d, 0b111); }
    inline int RR_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RR_IX(d, rp, buf);
    }

    // Rotate Right memory with load Reg.
    static inline int RR_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->RR_IY_with_LD(d, 0b000); }
    static inline int RR_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->RR_IY_with_LD(d, 0b001); }
    static inline int RR_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->RR_IY_with_LD(d, 0b010); }
    static inline int RR_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->RR_IY_with_LD(d, 0b011); }
    static inline int RR_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->RR_IY_with_LD(d, 0b100); }
    static inline int RR_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->RR_IY_with_LD(d, 0b101); }
    static inline int RR_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->RR_IY_with_LD(d, 0b111); }
    inline int RR_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RR_IY(d, rp, buf);
    }

    // Shift operand location (IX+d) left Arithmetic
    static inline int SLA_IX_(Z80* ctx, signed char d) { return ctx->SLA_IX(d); }
    inline int SLA_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SLA (IX+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SLA(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IX+d) left Arithmetic with load Reg.
    static inline int SLA_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->SLA_IX_with_LD(d, 0b000); }
    static inline int SLA_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->SLA_IX_with_LD(d, 0b001); }
    static inline int SLA_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->SLA_IX_with_LD(d, 0b010); }
    static inline int SLA_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->SLA_IX_with_LD(d, 0b011); }
    static inline int SLA_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->SLA_IX_with_LD(d, 0b100); }
    static inline int SLA_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->SLA_IX_with_LD(d, 0b101); }
    static inline int SLA_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->SLA_IX_with_LD(d, 0b111); }
    inline int SLA_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SLA_IX(d, rp, buf);
    }

    // Shift operand location (IY+d) left Arithmetic with load Reg.
    static inline int SLA_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->SLA_IY_with_LD(d, 0b000); }
    static inline int SLA_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->SLA_IY_with_LD(d, 0b001); }
    static inline int SLA_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->SLA_IY_with_LD(d, 0b010); }
    static inline int SLA_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->SLA_IY_with_LD(d, 0b011); }
    static inline int SLA_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->SLA_IY_with_LD(d, 0b100); }
    static inline int SLA_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->SLA_IY_with_LD(d, 0b101); }
    static inline int SLA_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->SLA_IY_with_LD(d, 0b111); }
    inline int SLA_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SLA_IY(d, rp, buf);
    }

    // Shift operand location (IX+d) Right Arithmetic
    static inline int SRA_IX_(Z80* ctx, signed char d) { return ctx->SRA_IX(d); }
    inline int SRA_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SRA (IX+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SRA(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IX+d) right Arithmetic with load Reg.
    static inline int SRA_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->SRA_IX_with_LD(d, 0b000); }
    static inline int SRA_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->SRA_IX_with_LD(d, 0b001); }
    static inline int SRA_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->SRA_IX_with_LD(d, 0b010); }
    static inline int SRA_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->SRA_IX_with_LD(d, 0b011); }
    static inline int SRA_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->SRA_IX_with_LD(d, 0b100); }
    static inline int SRA_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->SRA_IX_with_LD(d, 0b101); }
    static inline int SRA_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->SRA_IX_with_LD(d, 0b111); }
    inline int SRA_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SRA_IX(d, rp, buf);
    }

    // Shift operand location (IY+d) right Arithmetic with load Reg.
    static inline int SRA_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->SRA_IY_with_LD(d, 0b000); }
    static inline int SRA_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->SRA_IY_with_LD(d, 0b001); }
    static inline int SRA_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->SRA_IY_with_LD(d, 0b010); }
    static inline int SRA_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->SRA_IY_with_LD(d, 0b011); }
    static inline int SRA_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->SRA_IY_with_LD(d, 0b100); }
    static inline int SRA_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->SRA_IY_with_LD(d, 0b101); }
    static inline int SRA_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->SRA_IY_with_LD(d, 0b111); }
    inline int SRA_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SRA_IY(d, rp, buf);
    }

    // Shift operand location (IX+d) Right Logical
    static inline int SRL_IX_(Z80* ctx, signed char d) { return ctx->SRL_IX(d); }
    inline int SRL_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SRL (IX+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SRL(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IX+d) Right Logical with load Reg.
    static inline int SRL_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->SRL_IX_with_LD(d, 0b000); }
    static inline int SRL_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->SRL_IX_with_LD(d, 0b001); }
    static inline int SRL_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->SRL_IX_with_LD(d, 0b010); }
    static inline int SRL_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->SRL_IX_with_LD(d, 0b011); }
    static inline int SRL_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->SRL_IX_with_LD(d, 0b100); }
    static inline int SRL_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->SRL_IX_with_LD(d, 0b101); }
    static inline int SRL_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->SRL_IX_with_LD(d, 0b111); }
    inline int SRL_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SRL_IX(d, rp, buf);
    }

    // Shift operand location (IY+d) Right Logical with load Reg.
    static inline int SRL_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->SRL_IY_with_LD(d, 0b000); }
    static inline int SRL_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->SRL_IY_with_LD(d, 0b001); }
    static inline int SRL_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->SRL_IY_with_LD(d, 0b010); }
    static inline int SRL_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->SRL_IY_with_LD(d, 0b011); }
    static inline int SRL_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->SRL_IY_with_LD(d, 0b100); }
    static inline int SRL_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->SRL_IY_with_LD(d, 0b101); }
    static inline int SRL_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->SRL_IY_with_LD(d, 0b111); }
    inline int SRL_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SRL_IY(d, rp, buf);
    }

    // Shift operand location (IX+d) Left Logical
    // NOTE: this function is only for SLL_IX_with_LD
    static inline int SLL_IX_(Z80* ctx, signed char d) { return ctx->SLL_IX(d); }
    inline int SLL_IX(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SLL (IX+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SLL(n);
        writeByte(addr, result, 3);
        if (rp) *rp = result;
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IX+d) Left Logical with load Reg.
    static inline int SLL_IX_with_LD_B(Z80* ctx, signed char d) { return ctx->SLL_IX_with_LD(d, 0b000); }
    static inline int SLL_IX_with_LD_C(Z80* ctx, signed char d) { return ctx->SLL_IX_with_LD(d, 0b001); }
    static inline int SLL_IX_with_LD_D(Z80* ctx, signed char d) { return ctx->SLL_IX_with_LD(d, 0b010); }
    static inline int SLL_IX_with_LD_E(Z80* ctx, signed char d) { return ctx->SLL_IX_with_LD(d, 0b011); }
    static inline int SLL_IX_with_LD_H(Z80* ctx, signed char d) { return ctx->SLL_IX_with_LD(d, 0b100); }
    static inline int SLL_IX_with_LD_L(Z80* ctx, signed char d) { return ctx->SLL_IX_with_LD(d, 0b101); }
    static inline int SLL_IX_with_LD_A(Z80* ctx, signed char d) { return ctx->SLL_IX_with_LD(d, 0b111); }
    inline int SLL_IX_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SLL_IX(d, rp, buf);
    }

    // Shift operand location (IY+d) Left Logical
    // NOTE: this function is only for SLL_IY_with_LD
    static inline int SLL_IY_(Z80* ctx, signed char d) { return ctx->SLL_IY(d); }
    inline int SLL_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SLL (IY+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SLL(n);
        writeByte(addr, result, 3);
        if (rp) *rp = result;
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IY+d) Left Logical with load Reg.
    static inline int SLL_IY_with_LD_B(Z80* ctx, signed char d) { return ctx->SLL_IY_with_LD(d, 0b000); }
    static inline int SLL_IY_with_LD_C(Z80* ctx, signed char d) { return ctx->SLL_IY_with_LD(d, 0b001); }
    static inline int SLL_IY_with_LD_D(Z80* ctx, signed char d) { return ctx->SLL_IY_with_LD(d, 0b010); }
    static inline int SLL_IY_with_LD_E(Z80* ctx, signed char d) { return ctx->SLL_IY_with_LD(d, 0b011); }
    static inline int SLL_IY_with_LD_H(Z80* ctx, signed char d) { return ctx->SLL_IY_with_LD(d, 0b100); }
    static inline int SLL_IY_with_LD_L(Z80* ctx, signed char d) { return ctx->SLL_IY_with_LD(d, 0b101); }
    static inline int SLL_IY_with_LD_A(Z80* ctx, signed char d) { return ctx->SLL_IY_with_LD(d, 0b111); }
    inline int SLL_IY_with_LD(signed char d, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SLL_IY(d, rp, buf);
    }

    // Rotate memory (IY+d) Left Circular
    static inline int RLC_IY_(Z80* ctx, signed char d) { return ctx->RLC_IY(d); }
    inline int RLC_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RLC (IY+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = RLC(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Rotate memory (IY+d) Right Circular
    static inline int RRC_IY_(Z80* ctx, signed char d) { return ctx->RRC_IY(d); }
    inline int RRC_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RRC (IY+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = RRC(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Rotate Left memory
    static inline int RL_IY_(Z80* ctx, signed char d) { return ctx->RL_IY(d); }
    inline int RL_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RL (IY+d<$%04X>) = $%02X <C:%s>%s", reg.PC, addr, n, isFlagC() ? "ON" : "OFF", extraLog ? extraLog : "");
        unsigned char result = RL(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IY+d) left Arithmetic
    static inline int SLA_IY_(Z80* ctx, signed char d) { return ctx->SLA_IY(d); }
    inline int SLA_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SLA (IY+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SLA(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Rotate Right memory
    static inline int RR_IY_(Z80* ctx, signed char d) { return ctx->RR_IY(d); }
    inline int RR_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RR (IY+d<$%04X>) = $%02X <C:%s>%s", reg.PC, addr, n, isFlagC() ? "ON" : "OFF", extraLog ? extraLog : "");
        unsigned char result = RR(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IY+d) Right Arithmetic
    static inline int SRA_IY_(Z80* ctx, signed char d) { return ctx->SRA_IY(d); }
    inline int SRA_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SRA (IY+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SRA(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    // Shift operand location (IY+d) Right Logical
    static inline int SRL_IY_(Z80* ctx, signed char d) { return ctx->SRL_IY(d); }
    inline int SRL_IY(signed char d, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SRL (IY+d<$%04X>) = $%02X%s", reg.PC, addr, n, extraLog ? extraLog : "");
        unsigned char result = SRL(n);
        if (rp) *rp = result;
        writeByte(addr, result, 3);
        reg.PC += 4;
        return 0;
    }

    inline void addition8(int addition, int carry) { arithmetic8(false, addition, carry, true, true); }
    inline void subtract8(int subtract, int carry, bool setCarry = true, bool setResult = true) { arithmetic8(true, subtract, carry, setCarry, setResult); }
    inline void arithmetic8(bool negative, int addition, int carry, bool setCarry, bool setResult)
    {
        int before = reg.pair.A;
        int result = before + (negative ? -addition - carry : addition + carry);
        int carryX = before ^ addition ^ result;
        unsigned char finalResult = result & 0xFF;
        setFlagZ(0 == finalResult);
        setFlagN(negative);
        setFlagS(0x80 & finalResult);
        setFlagH(carryX & 0x10);
        setFlagPV(((carryX << 1) ^ carryX) & 0x100);
        if (setCarry) setFlagC(carryX & 0x100);
        if (setResult) {
            reg.pair.A = finalResult;
            setFlagXY(reg.pair.A);
        } else {
            setFlagXY(addition);
        }
    }

    inline void setFlagByIncrement(unsigned char before)
    {
        unsigned char finalResult = before + 1;
        setFlagN(false);
        setFlagZ(0 == finalResult);
        setFlagS(0x80 & finalResult);
        setFlagH((finalResult & 0x0F) == 0x00);
        setFlagPV(finalResult == 0x80);
        setFlagXY(finalResult);
    }

    inline void setFlagByDecrement(unsigned char before)
    {
        unsigned char finalResult = before - 1;
        setFlagN(true);
        setFlagZ(0 == finalResult);
        setFlagS(0x80 & finalResult);
        setFlagH((finalResult & 0x0F) == 0x0F);
        setFlagPV(finalResult == 0x7F);
        setFlagXY(finalResult);
    }

    // Add Reg. r to Acc.
    static inline int ADD_B(Z80* ctx) { return ctx->ADD_R(0b000); }
    static inline int ADD_C(Z80* ctx) { return ctx->ADD_R(0b001); }
    static inline int ADD_D(Z80* ctx) { return ctx->ADD_R(0b010); }
    static inline int ADD_E(Z80* ctx) { return ctx->ADD_R(0b011); }
    static inline int ADD_H(Z80* ctx) { return ctx->ADD_R(0b100); }
    static inline int ADD_L(Z80* ctx) { return ctx->ADD_R(0b101); }
    static inline int ADD_A(Z80* ctx) { return ctx->ADD_R(0b111); }
    static inline int ADD_B_2(Z80* ctx) { return ctx->ADD_R(0b000, 2); }
    static inline int ADD_C_2(Z80* ctx) { return ctx->ADD_R(0b001, 2); }
    static inline int ADD_D_2(Z80* ctx) { return ctx->ADD_R(0b010, 2); }
    static inline int ADD_E_2(Z80* ctx) { return ctx->ADD_R(0b011, 2); }
    static inline int ADD_A_2(Z80* ctx) { return ctx->ADD_R(0b111, 2); }
    inline int ADD_R(unsigned char r, int pc = 1)
    {
        if (isDebug()) log("[%04X] ADD %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        unsigned char* rp = getRegisterPointer(r);
        addition8(*rp, 0);
        reg.PC += pc;
        return 0;
    }

    // Add IXH to Acc.
    static inline int ADD_IXH_(Z80* ctx) { return ctx->ADD_IXH(); }
    inline int ADD_IXH()
    {
        if (isDebug()) log("[%04X] ADD %s, IXH<$%02X>", reg.PC, registerDump(0b111), getIXH());
        addition8(getIXH(), 0);
        reg.PC += 2;
        return 0;
    }

    // Add IXL to Acc.
    static inline int ADD_IXL_(Z80* ctx) { return ctx->ADD_IXL(); }
    inline int ADD_IXL()
    {
        if (isDebug()) log("[%04X] ADD %s, IXL<$%02X>", reg.PC, registerDump(0b111), getIXL());
        addition8(getIXL(), 0);
        reg.PC += 2;
        return 0;
    }

    // Add IYH to Acc.
    static inline int ADD_IYH_(Z80* ctx) { return ctx->ADD_IYH(); }
    inline int ADD_IYH()
    {
        if (isDebug()) log("[%04X] ADD %s, IYH<$%02X>", reg.PC, registerDump(0b111), getIYH());
        addition8(getIYH(), 0);
        reg.PC += 2;
        return 0;
    }

    // Add IYL to Acc.
    static inline int ADD_IYL_(Z80* ctx) { return ctx->ADD_IYL(); }
    inline int ADD_IYL()
    {
        if (isDebug()) log("[%04X] ADD %s, IYL<$%02X>", reg.PC, registerDump(0b111), getIYL());
        addition8(getIYL(), 0);
        reg.PC += 2;
        return 0;
    }

    // Add value n to Acc.
    static inline int ADD_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] ADD %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->addition8(n, 0);
        ctx->reg.PC += 2;
        return 0;
    }

    // Add location (HL) to Acc.
    static inline int ADD_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] ADD %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->addition8(n, 0);
        ctx->reg.PC += 1;
        return 0;
    }

    // Add location (IX+d) to Acc.
    static inline int ADD_IX_(Z80* ctx) { return ctx->ADD_IX(); }
    inline int ADD_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] ADD %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        addition8(n, 0);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Add location (IY+d) to Acc.
    static inline int ADD_IY_(Z80* ctx) { return ctx->ADD_IY(); }
    inline int ADD_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] ADD %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        addition8(n, 0);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Add Resister with carry
    static inline int ADC_B(Z80* ctx) { return ctx->ADC_R(0b000); }
    static inline int ADC_C(Z80* ctx) { return ctx->ADC_R(0b001); }
    static inline int ADC_D(Z80* ctx) { return ctx->ADC_R(0b010); }
    static inline int ADC_E(Z80* ctx) { return ctx->ADC_R(0b011); }
    static inline int ADC_H(Z80* ctx) { return ctx->ADC_R(0b100); }
    static inline int ADC_L(Z80* ctx) { return ctx->ADC_R(0b101); }
    static inline int ADC_A(Z80* ctx) { return ctx->ADC_R(0b111); }
    static inline int ADC_B_2(Z80* ctx) { return ctx->ADC_R(0b000, 2); }
    static inline int ADC_C_2(Z80* ctx) { return ctx->ADC_R(0b001, 2); }
    static inline int ADC_D_2(Z80* ctx) { return ctx->ADC_R(0b010, 2); }
    static inline int ADC_E_2(Z80* ctx) { return ctx->ADC_R(0b011, 2); }
    static inline int ADC_A_2(Z80* ctx) { return ctx->ADC_R(0b111, 2); }
    inline int ADC_R(unsigned char r, int pc = 1)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, %s <C:%s>", reg.PC, registerDump(0b111), registerDump(r), c ? "ON" : "OFF");
        addition8(*rp, c);
        reg.PC += pc;
        return 0;
    }

    // Add IXH to Acc.
    static inline int ADC_IXH_(Z80* ctx) { return ctx->ADC_IXH(); }
    inline int ADC_IXH()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, IXH<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIXH(), c ? "ON" : "OFF");
        addition8(getIXH(), c);
        reg.PC += 2;
        return 0;
    }

    // Add IXL to Acc.
    static inline int ADC_IXL_(Z80* ctx) { return ctx->ADC_IXL(); }
    inline int ADC_IXL()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, IXL<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIXL(), c ? "ON" : "OFF");
        addition8(getIXL(), c);
        reg.PC += 2;
        return 0;
    }

    // Add IYH to Acc.
    static inline int ADC_IYH_(Z80* ctx) { return ctx->ADC_IYH(); }
    inline int ADC_IYH()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, IYH<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIYH(), c ? "ON" : "OFF");
        addition8(getIYH(), c);
        reg.PC += 2;
        return 0;
    }

    // Add IYL to Acc.
    static inline int ADC_IYL_(Z80* ctx) { return ctx->ADC_IYL(); }
    inline int ADC_IYL()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, IYL<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIYL(), c ? "ON" : "OFF");
        addition8(getIYL(), c);
        reg.PC += 2;
        return 0;
    }

    // Add immediate with carry
    static inline int ADC_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] ADC %s, $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), n, c ? "ON" : "OFF");
        ctx->addition8(n, c);
        ctx->reg.PC += 2;
        return 0;
    }

    // Add memory with carry
    static inline int ADC_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] ADC %s, (%s) = $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n, c ? "ON" : "OFF");
        ctx->addition8(n, c);
        ctx->reg.PC += 1;
        return 0;
    }

    // Add memory with carry
    static inline int ADC_IX_(Z80* ctx) { return ctx->ADC_IX(); }
    inline int ADC_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        addition8(n, c);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Add memory with carry
    static inline int ADC_IY_(Z80* ctx) { return ctx->ADC_IY(); }
    inline int ADC_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] ADC %s, (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        addition8(n, c);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Increment Register
    static inline int INC_B(Z80* ctx) { return ctx->INC_R(0b000); }
    static inline int INC_C(Z80* ctx) { return ctx->INC_R(0b001); }
    static inline int INC_D(Z80* ctx) { return ctx->INC_R(0b010); }
    static inline int INC_E(Z80* ctx) { return ctx->INC_R(0b011); }
    static inline int INC_H(Z80* ctx) { return ctx->INC_R(0b100); }
    static inline int INC_L(Z80* ctx) { return ctx->INC_R(0b101); }
    static inline int INC_A(Z80* ctx) { return ctx->INC_R(0b111); }
    static inline int INC_B_2(Z80* ctx) { return ctx->INC_R(0b000, 2); }
    static inline int INC_C_2(Z80* ctx) { return ctx->INC_R(0b001, 2); }
    static inline int INC_D_2(Z80* ctx) { return ctx->INC_R(0b010, 2); }
    static inline int INC_E_2(Z80* ctx) { return ctx->INC_R(0b011, 2); }
    static inline int INC_H_2(Z80* ctx) { return ctx->INC_R(0b100, 2); }
    static inline int INC_L_2(Z80* ctx) { return ctx->INC_R(0b101, 2); }
    static inline int INC_A_2(Z80* ctx) { return ctx->INC_R(0b111, 2); }
    inline int INC_R(unsigned char r, int pc = 1)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] INC %s", reg.PC, registerDump(r));
        setFlagByIncrement(*rp);
        (*rp)++;
        reg.PC += pc;
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
    static inline int INC_IX_(Z80* ctx) { return ctx->INC_IX(); }
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

    // Increment register high 8 bits of IX
    static inline int INC_IXH_(Z80* ctx) { return ctx->INC_IXH(); }
    inline int INC_IXH()
    {
        unsigned char ixh = getIXH();
        if (isDebug()) log("[%04X] INC IXH<$%02X>", reg.PC, ixh);
        setFlagByIncrement(ixh++);
        setIXH(ixh);
        reg.PC += 2;
        return 0;
    }

    // Increment register low 8 bits of IX
    static inline int INC_IXL_(Z80* ctx) { return ctx->INC_IXL(); }
    inline int INC_IXL()
    {
        unsigned char ixl = getIXL();
        if (isDebug()) log("[%04X] INC IXL<$%02X>", reg.PC, ixl);
        setFlagByIncrement(ixl++);
        setIXL(ixl);
        reg.PC += 2;
        return 0;
    }

    // Increment location (IY+d)
    static inline int INC_IY_(Z80* ctx) { return ctx->INC_IY(); }
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

    // Increment register high 8 bits of IY
    static inline int INC_IYH_(Z80* ctx) { return ctx->INC_IYH(); }
    inline int INC_IYH()
    {
        unsigned char iyh = getIYH();
        if (isDebug()) log("[%04X] INC IYH<$%02X>", reg.PC, iyh);
        setFlagByIncrement(iyh++);
        setIYH(iyh);
        reg.PC += 2;
        return 0;
    }

    // Increment register low 8 bits of IY
    static inline int INC_IYL_(Z80* ctx) { return ctx->INC_IYL(); }
    inline int INC_IYL()
    {
        unsigned char iyl = getIYL();
        if (isDebug()) log("[%04X] INC IYL<$%02X>", reg.PC, iyl);
        setFlagByIncrement(iyl++);
        setIYL(iyl);
        reg.PC += 2;
        return 0;
    }

    // Subtract Register
    static inline int SUB_B(Z80* ctx) { return ctx->SUB_R(0b000); }
    static inline int SUB_C(Z80* ctx) { return ctx->SUB_R(0b001); }
    static inline int SUB_D(Z80* ctx) { return ctx->SUB_R(0b010); }
    static inline int SUB_E(Z80* ctx) { return ctx->SUB_R(0b011); }
    static inline int SUB_H(Z80* ctx) { return ctx->SUB_R(0b100); }
    static inline int SUB_L(Z80* ctx) { return ctx->SUB_R(0b101); }
    static inline int SUB_A(Z80* ctx) { return ctx->SUB_R(0b111); }
    static inline int SUB_B_2(Z80* ctx) { return ctx->SUB_R(0b000, 2); }
    static inline int SUB_C_2(Z80* ctx) { return ctx->SUB_R(0b001, 2); }
    static inline int SUB_D_2(Z80* ctx) { return ctx->SUB_R(0b010, 2); }
    static inline int SUB_E_2(Z80* ctx) { return ctx->SUB_R(0b011, 2); }
    static inline int SUB_A_2(Z80* ctx) { return ctx->SUB_R(0b111, 2); }
    inline int SUB_R(unsigned char r, int pc = 1)
    {
        if (isDebug()) log("[%04X] SUB %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        unsigned char* rp = getRegisterPointer(r);
        subtract8(*rp, 0);
        reg.PC += pc;
        return 0;
    }

    // Subtract IXH to Acc.
    static inline int SUB_IXH_(Z80* ctx) { return ctx->SUB_IXH(); }
    inline int SUB_IXH()
    {
        if (isDebug()) log("[%04X] SUB %s, IXH<$%02X>", reg.PC, registerDump(0b111), getIXH());
        subtract8(getIXH(), 0);
        reg.PC += 2;
        return 0;
    }

    // Subtract IXL to Acc.
    static inline int SUB_IXL_(Z80* ctx) { return ctx->SUB_IXL(); }
    inline int SUB_IXL()
    {
        if (isDebug()) log("[%04X] SUB %s, IXL<$%02X>", reg.PC, registerDump(0b111), getIXL());
        subtract8(getIXL(), 0);
        reg.PC += 2;
        return 0;
    }

    // Subtract IYH to Acc.
    static inline int SUB_IYH_(Z80* ctx) { return ctx->SUB_IYH(); }
    inline int SUB_IYH()
    {
        if (isDebug()) log("[%04X] SUB %s, IYH<$%02X>", reg.PC, registerDump(0b111), getIYH());
        subtract8(getIYH(), 0);
        reg.PC += 2;
        return 0;
    }

    // Subtract IYL to Acc.
    static inline int SUB_IYL_(Z80* ctx) { return ctx->SUB_IYL(); }
    inline int SUB_IYL()
    {
        if (isDebug()) log("[%04X] SUB %s, IYL<$%02X>", reg.PC, registerDump(0b111), getIYL());
        subtract8(getIYL(), 0);
        reg.PC += 2;
        return 0;
    }

    // Subtract immediate
    static inline int SUB_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] SUB %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->subtract8(n, 0);
        ctx->reg.PC += 2;
        return 0;
    }

    // Subtract memory
    static inline int SUB_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] SUB %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->subtract8(n, 0);
        ctx->reg.PC += 1;
        return 0;
    }

    // Subtract memory
    static inline int SUB_IX_(Z80* ctx) { return ctx->SUB_IX(); }
    inline int SUB_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SUB %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        subtract8(n, 0);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Subtract memory
    static inline int SUB_IY_(Z80* ctx) { return ctx->SUB_IY(); }
    inline int SUB_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SUB %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        subtract8(n, 0);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Subtract Resister with carry
    static inline int SBC_B(Z80* ctx) { return ctx->SBC_R(0b000); }
    static inline int SBC_C(Z80* ctx) { return ctx->SBC_R(0b001); }
    static inline int SBC_D(Z80* ctx) { return ctx->SBC_R(0b010); }
    static inline int SBC_E(Z80* ctx) { return ctx->SBC_R(0b011); }
    static inline int SBC_H(Z80* ctx) { return ctx->SBC_R(0b100); }
    static inline int SBC_L(Z80* ctx) { return ctx->SBC_R(0b101); }
    static inline int SBC_A(Z80* ctx) { return ctx->SBC_R(0b111); }
    static inline int SBC_B_2(Z80* ctx) { return ctx->SBC_R(0b000, 2); }
    static inline int SBC_C_2(Z80* ctx) { return ctx->SBC_R(0b001, 2); }
    static inline int SBC_D_2(Z80* ctx) { return ctx->SBC_R(0b010, 2); }
    static inline int SBC_E_2(Z80* ctx) { return ctx->SBC_R(0b011, 2); }
    static inline int SBC_A_2(Z80* ctx) { return ctx->SBC_R(0b111, 2); }
    inline int SBC_R(unsigned char r, int pc = 1)
    {
        unsigned char* rp = getRegisterPointer(r);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, %s <C:%s>", reg.PC, registerDump(0b111), registerDump(r), c ? "ON" : "OFF");
        subtract8(*rp, c);
        reg.PC += pc;
        return 0;
    }

    // Subtract IXH to Acc. with carry
    static inline int SBC_IXH_(Z80* ctx) { return ctx->SBC_IXH(); }
    inline int SBC_IXH()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, IXH<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIXH(), c ? "ON" : "OFF");
        subtract8(getIXH(), c);
        reg.PC += 2;
        return 0;
    }

    // Subtract IXL to Acc. with carry
    static inline int SBC_IXL_(Z80* ctx) { return ctx->SBC_IXL(); }
    inline int SBC_IXL()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, IXL<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIXL(), c ? "ON" : "OFF");
        subtract8(getIXL(), c);
        reg.PC += 2;
        return 0;
    }

    // Subtract IYH to Acc. with carry
    static inline int SBC_IYH_(Z80* ctx) { return ctx->SBC_IYH(); }
    inline int SBC_IYH()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, IYH<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIYH(), c ? "ON" : "OFF");
        subtract8(getIYH(), c);
        reg.PC += 2;
        return 0;
    }

    // Subtract IYL to Acc. with carry
    static inline int SBC_IYL_(Z80* ctx) { return ctx->SBC_IYL(); }
    inline int SBC_IYL()
    {
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, IYL<$%02X> <C:%s>", reg.PC, registerDump(0b111), getIYL(), c ? "ON" : "OFF");
        subtract8(getIYL(), c);
        reg.PC += 2;
        return 0;
    }

    // Subtract immediate with carry
    static inline int SBC_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] SBC %s, $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), n, c ? "ON" : "OFF");
        ctx->subtract8(n, c);
        ctx->reg.PC += 2;
        return 0;
    }

    // Subtract memory with carry
    static inline int SBC_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        unsigned char c = ctx->isFlagC() ? 1 : 0;
        if (ctx->isDebug()) ctx->log("[%04X] SBC %s, (%s) = $%02X <C:%s>", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n, c ? "ON" : "OFF");
        ctx->subtract8(n, c);
        ctx->reg.PC += 1;
        return 0;
    }

    // Subtract memory with carry
    static inline int SBC_IX_(Z80* ctx) { return ctx->SBC_IX(); }
    inline int SBC_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, (IX+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        subtract8(n, c);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Subtract memory with carry
    static inline int SBC_IY_(Z80* ctx) { return ctx->SBC_IY(); }
    inline int SBC_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        unsigned char c = isFlagC() ? 1 : 0;
        if (isDebug()) log("[%04X] SBC %s, (IY+d<$%04X>) = $%02X <C:%s>", reg.PC, registerDump(0b111), addr, n, c ? "ON" : "OFF");
        subtract8(n, c);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Decrement Register
    static inline int DEC_B(Z80* ctx) { return ctx->DEC_R(0b000); }
    static inline int DEC_C(Z80* ctx) { return ctx->DEC_R(0b001); }
    static inline int DEC_D(Z80* ctx) { return ctx->DEC_R(0b010); }
    static inline int DEC_E(Z80* ctx) { return ctx->DEC_R(0b011); }
    static inline int DEC_H(Z80* ctx) { return ctx->DEC_R(0b100); }
    static inline int DEC_L(Z80* ctx) { return ctx->DEC_R(0b101); }
    static inline int DEC_A(Z80* ctx) { return ctx->DEC_R(0b111); }
    static inline int DEC_B_2(Z80* ctx) { return ctx->DEC_R(0b000, 2); }
    static inline int DEC_C_2(Z80* ctx) { return ctx->DEC_R(0b001, 2); }
    static inline int DEC_D_2(Z80* ctx) { return ctx->DEC_R(0b010, 2); }
    static inline int DEC_E_2(Z80* ctx) { return ctx->DEC_R(0b011, 2); }
    static inline int DEC_H_2(Z80* ctx) { return ctx->DEC_R(0b100, 2); }
    static inline int DEC_L_2(Z80* ctx) { return ctx->DEC_R(0b101, 2); }
    static inline int DEC_A_2(Z80* ctx) { return ctx->DEC_R(0b111, 2); }
    inline int DEC_R(unsigned char r, int pc = 1)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] DEC %s", reg.PC, registerDump(r));
        setFlagByDecrement(*rp);
        (*rp)--;
        reg.PC += pc;
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
    static inline int DEC_IX_(Z80* ctx) { return ctx->DEC_IX(); }
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

    // Decrement high 8 bits of IX
    static inline int DEC_IXH_(Z80* ctx) { return ctx->DEC_IXH(); }
    inline int DEC_IXH()
    {
        unsigned char ixh = getIXH();
        if (isDebug()) log("[%04X] DEC IXH<$%02X>", reg.PC, ixh);
        setFlagByDecrement(ixh--);
        setIXH(ixh);
        reg.PC += 2;
        return 0;
    }

    // Decrement low 8 bits of IX
    static inline int DEC_IXL_(Z80* ctx) { return ctx->DEC_IXL(); }
    inline int DEC_IXL()
    {
        unsigned char ixl = getIXL();
        if (isDebug()) log("[%04X] DEC IXL<$%02X>", reg.PC, ixl);
        setFlagByDecrement(ixl--);
        setIXL(ixl);
        reg.PC += 2;
        return 0;
    }

    // Decrement location (IY+d)
    static inline int DEC_IY_(Z80* ctx) { return ctx->DEC_IY(); }
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

    // Decrement high 8 bits of IY
    static inline int DEC_IYH_(Z80* ctx) { return ctx->DEC_IYH(); }
    inline int DEC_IYH()
    {
        unsigned char iyh = getIYH();
        if (isDebug()) log("[%04X] DEC IYH<$%02X>", reg.PC, iyh);
        setFlagByDecrement(iyh--);
        setIYH(iyh);
        reg.PC += 2;
        return 0;
    }

    // Decrement low 8 bits of IY
    static inline int DEC_IYL_(Z80* ctx) { return ctx->DEC_IYL(); }
    inline int DEC_IYL()
    {
        unsigned char iyl = getIYL();
        if (isDebug()) log("[%04X] DEC IYL<$%02X>", reg.PC, iyl);
        setFlagByDecrement(iyl--);
        setIYL(iyl);
        reg.PC += 2;
        return 0;
    }

    inline void setFlagByAdd16(unsigned short before, unsigned short addition)
    {
        int result = before + addition;
        int carrybits = before ^ addition ^ result;
        setFlagN(false);
        setFlagXY((result & 0xFF00) >> 8);
        setFlagC((carrybits & 0x10000) != 0);
        setFlagH((carrybits & 0x1000) != 0);
    }

    inline void setFlagByAdc16(unsigned short before, unsigned short addition)
    {
        int result = before + addition;
        int carrybits = before ^ addition ^ result;
        unsigned short finalResult = (unsigned short)(result);
        // same as ADD
        setFlagN(false);
        setFlagXY((finalResult & 0xFF00) >> 8);
        setFlagC((carrybits & 0x10000) != 0);
        setFlagH((carrybits & 0x1000) != 0);
        // only ADC
        setFlagS(finalResult & 0x8000);
        setFlagZ(0 == finalResult);
        setFlagPV((((carrybits << 1) ^ carrybits) & 0x10000) != 0);
    }

    // Add register pair to H and L
    static inline int ADD_HL_BC(Z80* ctx) { return ctx->ADD_HL_RP(0b00); }
    static inline int ADD_HL_DE(Z80* ctx) { return ctx->ADD_HL_RP(0b01); }
    static inline int ADD_HL_HL(Z80* ctx) { return ctx->ADD_HL_RP(0b10); }
    static inline int ADD_HL_SP(Z80* ctx) { return ctx->ADD_HL_RP(0b11); }
    inline int ADD_HL_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] ADD %s, %s", reg.PC, registerPairDump(0b10), registerPairDump(rp));
        unsigned short hl = getHL();
        unsigned short nn = getRP(rp);
        reg.WZ = nn + 1;
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
        reg.WZ = hl + 1;
        setFlagByAdc16(hl, c + nn);
        setHL(hl + c + nn);
        reg.PC += 2;
        return consumeClock(7);
    }

    // Add register pair to IX
    static inline int ADD_IX_BC(Z80* ctx) { return ctx->ADD_IX_RP(0b00); }
    static inline int ADD_IX_DE(Z80* ctx) { return ctx->ADD_IX_RP(0b01); }
    static inline int ADD_IX_IX(Z80* ctx) { return ctx->ADD_IX_RP(0b10); }
    static inline int ADD_IX_SP(Z80* ctx) { return ctx->ADD_IX_RP(0b11); }
    inline int ADD_IX_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] ADD IX<$%04X>, %s", reg.PC, reg.IX, registerPairDumpIX(rp));
        unsigned short nn = getRPIX(rp);
        setFlagByAdd16(reg.IX, nn);
        reg.IX += nn;
        reg.PC += 2;
        return consumeClock(7);
    }

    // Add register pair to IY
    static inline int ADD_IY_BC(Z80* ctx) { return ctx->ADD_IY_RP(0b00); }
    static inline int ADD_IY_DE(Z80* ctx) { return ctx->ADD_IY_RP(0b01); }
    static inline int ADD_IY_IY(Z80* ctx) { return ctx->ADD_IY_RP(0b10); }
    static inline int ADD_IY_SP(Z80* ctx) { return ctx->ADD_IY_RP(0b11); }
    inline int ADD_IY_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] ADD IY<$%04X>, %s", reg.PC, reg.IY, registerPairDumpIY(rp));
        unsigned short nn = getRPIY(rp);
        setFlagByAdd16(reg.IY, nn);
        reg.IY += nn;
        reg.PC += 2;
        return consumeClock(7);
    }

    // Increment register pair
    static inline int INC_RP_BC(Z80* ctx) { return ctx->INC_RP(0b00); }
    static inline int INC_RP_DE(Z80* ctx) { return ctx->INC_RP(0b01); }
    static inline int INC_RP_HL(Z80* ctx) { return ctx->INC_RP(0b10); }
    static inline int INC_RP_SP(Z80* ctx) { return ctx->INC_RP(0b11); }
    inline int INC_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] INC %s", reg.PC, registerPairDump(rp));
        unsigned short nn = getRP(rp);
        setRP(rp, nn + 1);
        reg.PC++;
        return consumeClock(2);
    }

    // Increment IX
    static inline int INC_IX_reg_(Z80* ctx) { return ctx->INC_IX_reg(); }
    inline int INC_IX_reg()
    {
        if (isDebug()) log("[%04X] INC IX<$%04X>", reg.PC, reg.IX);
        reg.IX++;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Increment IY
    static inline int INC_IY_reg_(Z80* ctx) { return ctx->INC_IY_reg(); }
    inline int INC_IY_reg()
    {
        if (isDebug()) log("[%04X] INC IY<$%04X>", reg.PC, reg.IY);
        reg.IY++;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Decrement register pair
    static inline int DEC_RP_BC(Z80* ctx) { return ctx->DEC_RP(0b00); }
    static inline int DEC_RP_DE(Z80* ctx) { return ctx->DEC_RP(0b01); }
    static inline int DEC_RP_HL(Z80* ctx) { return ctx->DEC_RP(0b10); }
    static inline int DEC_RP_SP(Z80* ctx) { return ctx->DEC_RP(0b11); }
    inline int DEC_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] DEC %s", reg.PC, registerPairDump(rp));
        unsigned short nn = getRP(rp);
        setRP(rp, nn - 1);
        reg.PC++;
        return consumeClock(2);
    }

    // Decrement IX
    static inline int DEC_IX_reg_(Z80* ctx) { return ctx->DEC_IX_reg(); }
    inline int DEC_IX_reg()
    {
        if (isDebug()) log("[%04X] DEC IX<$%04X>", reg.PC, reg.IX);
        reg.IX--;
        reg.PC += 2;
        return consumeClock(2);
    }

    // Decrement IY
    static inline int DEC_IY_reg_(Z80* ctx) { return ctx->DEC_IY_reg(); }
    inline int DEC_IY_reg()
    {
        if (isDebug()) log("[%04X] DEC IY<$%04X>", reg.PC, reg.IY);
        reg.IY--;
        reg.PC += 2;
        return consumeClock(2);
    }

    inline void setFlagBySbc16(unsigned short before, unsigned short subtract)
    {
        int result = before - subtract;
        int carrybits = before ^ subtract ^ result;
        unsigned short finalResult = (unsigned short)result;
        setFlagN(true);
        setFlagXY((finalResult & 0xFF00) >> 8);
        setFlagC((carrybits & 0x10000) != 0);
        setFlagH((carrybits & 0x1000) != 0);
        setFlagS(finalResult & 0x8000);
        setFlagZ(0 == finalResult);
        setFlagPV((((carrybits << 1) ^ carrybits) & 0x10000) != 0);
    }

    // Subtract register pair from HL with carry
    inline int SBC_HL_RP(unsigned char rp)
    {
        if (isDebug()) log("[%04X] SBC %s, %s <C:%s>", reg.PC, registerPairDump(0b10), registerPairDump(rp), isFlagC() ? "ON" : "OFF");
        unsigned short hl = getHL();
        unsigned short nn = getRP(rp);
        unsigned char c = isFlagC() ? 1 : 0;
        reg.WZ = hl + 1;
        setFlagBySbc16(hl, c + nn);
        setHL(hl - c - nn);
        reg.PC += 2;
        return consumeClock(7);
    }

    inline void setFlagByLogical(bool h)
    {
        setFlagS(reg.pair.A & 0x80);
        setFlagZ(reg.pair.A == 0);
        setFlagXY(reg.pair.A);
        setFlagH(h);
        setFlagPV(isEvenNumberBits(reg.pair.A));
        setFlagN(false);
        setFlagC(false);
    }

    inline void and8(unsigned char n, int pc)
    {
        reg.pair.A &= n;
        setFlagByLogical(true);
        reg.PC += pc;
    }

    inline void or8(unsigned char n, int pc)
    {
        reg.pair.A |= n;
        setFlagByLogical(false);
        reg.PC += pc;
    }

    inline void xor8(unsigned char n, int pc)
    {
        reg.pair.A ^= n;
        setFlagByLogical(false);
        reg.PC += pc;
    }

    // AND Register
    static inline int AND_B(Z80* ctx) { return ctx->AND_R(0b000); }
    static inline int AND_C(Z80* ctx) { return ctx->AND_R(0b001); }
    static inline int AND_D(Z80* ctx) { return ctx->AND_R(0b010); }
    static inline int AND_E(Z80* ctx) { return ctx->AND_R(0b011); }
    static inline int AND_H(Z80* ctx) { return ctx->AND_R(0b100); }
    static inline int AND_L(Z80* ctx) { return ctx->AND_R(0b101); }
    static inline int AND_A(Z80* ctx) { return ctx->AND_R(0b111); }
    static inline int AND_B_2(Z80* ctx) { return ctx->AND_R(0b000, 2); }
    static inline int AND_C_2(Z80* ctx) { return ctx->AND_R(0b001, 2); }
    static inline int AND_D_2(Z80* ctx) { return ctx->AND_R(0b010, 2); }
    static inline int AND_E_2(Z80* ctx) { return ctx->AND_R(0b011, 2); }
    static inline int AND_A_2(Z80* ctx) { return ctx->AND_R(0b111, 2); }
    inline int AND_R(unsigned char r, int pc = 1)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] AND %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        and8(*rp, pc);
        return 0;
    }

    // AND with register IXH
    static inline int AND_IXH_(Z80* ctx) { return ctx->AND_IXH(); }
    inline int AND_IXH()
    {
        if (isDebug()) log("[%04X] AND %s, IXH<$%02X>", reg.PC, registerDump(0b111), getIXH());
        and8(getIXH(), 2);
        return 0;
    }

    // AND with register IXL
    static inline int AND_IXL_(Z80* ctx) { return ctx->AND_IXL(); }
    inline int AND_IXL()
    {
        if (isDebug()) log("[%04X] AND %s, IXL<$%02X>", reg.PC, registerDump(0b111), getIXL());
        and8(getIXL(), 2);
        return 0;
    }

    // AND with register IYH
    static inline int AND_IYH_(Z80* ctx) { return ctx->AND_IYH(); }
    inline int AND_IYH()
    {
        if (isDebug()) log("[%04X] AND %s, IYH<$%02X>", reg.PC, registerDump(0b111), getIYH());
        and8(getIYH(), 2);
        return 0;
    }

    // AND with register IYL
    static inline int AND_IYL_(Z80* ctx) { return ctx->AND_IYL(); }
    inline int AND_IYL()
    {
        if (isDebug()) log("[%04X] AND %s, IYL<$%02X>", reg.PC, registerDump(0b111), getIYL());
        and8(getIYL(), 2);
        return 0;
    }

    // AND immediate
    static inline int AND_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] AND %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->and8(n, 2);
        return 0;
    }

    // AND Memory
    static inline int AND_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] AND %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->and8(n, 1);
        return 0;
    }

    // AND Memory
    static inline int AND_IX_(Z80* ctx) { return ctx->AND_IX(); }
    inline int AND_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] AND %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A & n);
        and8(n, 3);
        return consumeClock(3);
    }

    // AND Memory
    static inline int AND_IY_(Z80* ctx) { return ctx->AND_IY(); }
    inline int AND_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] AND %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A & n);
        and8(n, 3);
        return consumeClock(3);
    }

    // OR Register
    static inline int OR_B(Z80* ctx) { return ctx->OR_R(0b000); }
    static inline int OR_C(Z80* ctx) { return ctx->OR_R(0b001); }
    static inline int OR_D(Z80* ctx) { return ctx->OR_R(0b010); }
    static inline int OR_E(Z80* ctx) { return ctx->OR_R(0b011); }
    static inline int OR_H(Z80* ctx) { return ctx->OR_R(0b100); }
    static inline int OR_L(Z80* ctx) { return ctx->OR_R(0b101); }
    static inline int OR_A(Z80* ctx) { return ctx->OR_R(0b111); }
    static inline int OR_B_2(Z80* ctx) { return ctx->OR_R(0b000, 2); }
    static inline int OR_C_2(Z80* ctx) { return ctx->OR_R(0b001, 2); }
    static inline int OR_D_2(Z80* ctx) { return ctx->OR_R(0b010, 2); }
    static inline int OR_E_2(Z80* ctx) { return ctx->OR_R(0b011, 2); }
    static inline int OR_A_2(Z80* ctx) { return ctx->OR_R(0b111, 2); }
    inline int OR_R(unsigned char r, int pc = 1)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] OR %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        or8(*rp, pc);
        return 0;
    }

    // OR with register IXH
    static inline int OR_IXH_(Z80* ctx) { return ctx->OR_IXH(); }
    inline int OR_IXH()
    {
        if (isDebug()) log("[%04X] OR %s, IXH<$%02X>", reg.PC, registerDump(0b111), getIXH());
        or8(getIXH(), 2);
        return 0;
    }

    // OR with register IXL
    static inline int OR_IXL_(Z80* ctx) { return ctx->OR_IXL(); }
    inline int OR_IXL()
    {
        if (isDebug()) log("[%04X] OR %s, IXL<$%02X>", reg.PC, registerDump(0b111), getIXL());
        or8(getIXL(), 2);
        return 0;
    }

    // OR with register IYH
    static inline int OR_IYH_(Z80* ctx) { return ctx->OR_IYH(); }
    inline int OR_IYH()
    {
        if (isDebug()) log("[%04X] OR %s, IYH<$%02X>", reg.PC, registerDump(0b111), getIYH());
        or8(getIYH(), 2);
        return 0;
    }

    // OR with register IYL
    static inline int OR_IYL_(Z80* ctx) { return ctx->OR_IYL(); }
    inline int OR_IYL()
    {
        if (isDebug()) log("[%04X] OR %s, IYL<$%02X>", reg.PC, registerDump(0b111), getIYL());
        or8(getIYL(), 2);
        return 0;
    }

    // OR immediate
    static inline int OR_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] OR %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->or8(n, 2);
        return 0;
    }

    // OR Memory
    static inline int OR_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] OR %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), ctx->reg.pair.A | n);
        ctx->or8(n, 1);
        return 0;
    }

    // OR Memory
    static inline int OR_IX_(Z80* ctx) { return ctx->OR_IX(); }
    inline int OR_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] OR %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A | n);
        or8(n, 3);
        return consumeClock(3);
    }

    // OR Memory
    static inline int OR_IY_(Z80* ctx) { return ctx->OR_IY(); }
    inline int OR_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] OR %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A | n);
        or8(n, 3);
        return consumeClock(3);
    }

    // XOR Reigster
    static inline int XOR_B(Z80* ctx) { return ctx->XOR_R(0b000); }
    static inline int XOR_C(Z80* ctx) { return ctx->XOR_R(0b001); }
    static inline int XOR_D(Z80* ctx) { return ctx->XOR_R(0b010); }
    static inline int XOR_E(Z80* ctx) { return ctx->XOR_R(0b011); }
    static inline int XOR_H(Z80* ctx) { return ctx->XOR_R(0b100); }
    static inline int XOR_L(Z80* ctx) { return ctx->XOR_R(0b101); }
    static inline int XOR_A(Z80* ctx) { return ctx->XOR_R(0b111); }
    static inline int XOR_B_2(Z80* ctx) { return ctx->XOR_R(0b000, 2); }
    static inline int XOR_C_2(Z80* ctx) { return ctx->XOR_R(0b001, 2); }
    static inline int XOR_D_2(Z80* ctx) { return ctx->XOR_R(0b010, 2); }
    static inline int XOR_E_2(Z80* ctx) { return ctx->XOR_R(0b011, 2); }
    static inline int XOR_A_2(Z80* ctx) { return ctx->XOR_R(0b111, 2); }
    inline int XOR_R(unsigned char r, int pc = 1)
    {
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) log("[%04X] XOR %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        xor8(*rp, pc);
        return 0;
    }

    // XOR with register IXH
    static inline int XOR_IXH_(Z80* ctx) { return ctx->XOR_IXH(); }
    inline int XOR_IXH()
    {
        if (isDebug()) log("[%04X] XOR %s, IXH<$%02X>", reg.PC, registerDump(0b111), getIXH());
        xor8(getIXH(), 2);
        return 0;
    }

    // XOR with register IXL
    static inline int XOR_IXL_(Z80* ctx) { return ctx->XOR_IXL(); }
    inline int XOR_IXL()
    {
        if (isDebug()) log("[%04X] XOR %s, IXL<$%02X>", reg.PC, registerDump(0b111), getIXL());
        xor8(getIXL(), 2);
        return 0;
    }

    // XOR with register IYH
    static inline int XOR_IYH_(Z80* ctx) { return ctx->XOR_IYH(); }
    inline int XOR_IYH()
    {
        if (isDebug()) log("[%04X] XOR %s, IYH<$%02X>", reg.PC, registerDump(0b111), getIYH());
        xor8(getIYH(), 2);
        return 0;
    }

    // XOR with register IYL
    static inline int XOR_IYL_(Z80* ctx) { return ctx->XOR_IYL(); }
    inline int XOR_IYL()
    {
        if (isDebug()) log("[%04X] XOR %s, IYL<$%02X>", reg.PC, registerDump(0b111), getIYL());
        xor8(getIYL(), 2);
        return 0;
    }

    // XOR immediate
    static inline int XOR_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] XOR %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->xor8(n, 2);
        return 0;
    }

    // XOR Memory
    static inline int XOR_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] XOR %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), ctx->reg.pair.A ^ n);
        ctx->xor8(n, 1);
        return 0;
    }

    // XOR Memory
    static inline int XOR_IX_(Z80* ctx) { return ctx->XOR_IX(); }
    inline int XOR_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] XOR %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A ^ n);
        xor8(n, 3);
        return consumeClock(3);
    }

    // XOR Memory
    static inline int XOR_IY_(Z80* ctx) { return ctx->XOR_IY(); }
    inline int XOR_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] XOR %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, reg.pair.A ^ n);
        xor8(n, 3);
        return consumeClock(3);
    }

    // Complement acc. (1's Comp.)
    static inline int CPL(Z80* ctx)
    {
        if (ctx->isDebug()) ctx->log("[%04X] CPL %s", ctx->reg.PC, ctx->registerDump(0b111));
        ctx->reg.pair.A = ~ctx->reg.pair.A;
        ctx->setFlagH(true);
        ctx->setFlagN(true);
        ctx->setFlagXY(ctx->reg.pair.A);
        ctx->reg.PC++;
        return 0;
    }

    // Negate Acc. (2's Comp.)
    inline int NEG()
    {
        if (isDebug()) log("[%04X] NEG %s", reg.PC, registerDump(0b111));
        unsigned char a = reg.pair.A;
        reg.pair.A = 0;
        subtract8(a, 0);
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
        ctx->setFlagXY(ctx->reg.pair.A);
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
        ctx->setFlagXY(ctx->reg.pair.A);
        ctx->reg.PC++;
        return 0;
    }

    // Test BIT b of register r
    static inline int BIT_B_0(Z80* ctx) { return ctx->BIT_R(0b000, 0); }
    static inline int BIT_B_1(Z80* ctx) { return ctx->BIT_R(0b000, 1); }
    static inline int BIT_B_2(Z80* ctx) { return ctx->BIT_R(0b000, 2); }
    static inline int BIT_B_3(Z80* ctx) { return ctx->BIT_R(0b000, 3); }
    static inline int BIT_B_4(Z80* ctx) { return ctx->BIT_R(0b000, 4); }
    static inline int BIT_B_5(Z80* ctx) { return ctx->BIT_R(0b000, 5); }
    static inline int BIT_B_6(Z80* ctx) { return ctx->BIT_R(0b000, 6); }
    static inline int BIT_B_7(Z80* ctx) { return ctx->BIT_R(0b000, 7); }
    static inline int BIT_C_0(Z80* ctx) { return ctx->BIT_R(0b001, 0); }
    static inline int BIT_C_1(Z80* ctx) { return ctx->BIT_R(0b001, 1); }
    static inline int BIT_C_2(Z80* ctx) { return ctx->BIT_R(0b001, 2); }
    static inline int BIT_C_3(Z80* ctx) { return ctx->BIT_R(0b001, 3); }
    static inline int BIT_C_4(Z80* ctx) { return ctx->BIT_R(0b001, 4); }
    static inline int BIT_C_5(Z80* ctx) { return ctx->BIT_R(0b001, 5); }
    static inline int BIT_C_6(Z80* ctx) { return ctx->BIT_R(0b001, 6); }
    static inline int BIT_C_7(Z80* ctx) { return ctx->BIT_R(0b001, 7); }
    static inline int BIT_D_0(Z80* ctx) { return ctx->BIT_R(0b010, 0); }
    static inline int BIT_D_1(Z80* ctx) { return ctx->BIT_R(0b010, 1); }
    static inline int BIT_D_2(Z80* ctx) { return ctx->BIT_R(0b010, 2); }
    static inline int BIT_D_3(Z80* ctx) { return ctx->BIT_R(0b010, 3); }
    static inline int BIT_D_4(Z80* ctx) { return ctx->BIT_R(0b010, 4); }
    static inline int BIT_D_5(Z80* ctx) { return ctx->BIT_R(0b010, 5); }
    static inline int BIT_D_6(Z80* ctx) { return ctx->BIT_R(0b010, 6); }
    static inline int BIT_D_7(Z80* ctx) { return ctx->BIT_R(0b010, 7); }
    static inline int BIT_E_0(Z80* ctx) { return ctx->BIT_R(0b011, 0); }
    static inline int BIT_E_1(Z80* ctx) { return ctx->BIT_R(0b011, 1); }
    static inline int BIT_E_2(Z80* ctx) { return ctx->BIT_R(0b011, 2); }
    static inline int BIT_E_3(Z80* ctx) { return ctx->BIT_R(0b011, 3); }
    static inline int BIT_E_4(Z80* ctx) { return ctx->BIT_R(0b011, 4); }
    static inline int BIT_E_5(Z80* ctx) { return ctx->BIT_R(0b011, 5); }
    static inline int BIT_E_6(Z80* ctx) { return ctx->BIT_R(0b011, 6); }
    static inline int BIT_E_7(Z80* ctx) { return ctx->BIT_R(0b011, 7); }
    static inline int BIT_H_0(Z80* ctx) { return ctx->BIT_R(0b100, 0); }
    static inline int BIT_H_1(Z80* ctx) { return ctx->BIT_R(0b100, 1); }
    static inline int BIT_H_2(Z80* ctx) { return ctx->BIT_R(0b100, 2); }
    static inline int BIT_H_3(Z80* ctx) { return ctx->BIT_R(0b100, 3); }
    static inline int BIT_H_4(Z80* ctx) { return ctx->BIT_R(0b100, 4); }
    static inline int BIT_H_5(Z80* ctx) { return ctx->BIT_R(0b100, 5); }
    static inline int BIT_H_6(Z80* ctx) { return ctx->BIT_R(0b100, 6); }
    static inline int BIT_H_7(Z80* ctx) { return ctx->BIT_R(0b100, 7); }
    static inline int BIT_L_0(Z80* ctx) { return ctx->BIT_R(0b101, 0); }
    static inline int BIT_L_1(Z80* ctx) { return ctx->BIT_R(0b101, 1); }
    static inline int BIT_L_2(Z80* ctx) { return ctx->BIT_R(0b101, 2); }
    static inline int BIT_L_3(Z80* ctx) { return ctx->BIT_R(0b101, 3); }
    static inline int BIT_L_4(Z80* ctx) { return ctx->BIT_R(0b101, 4); }
    static inline int BIT_L_5(Z80* ctx) { return ctx->BIT_R(0b101, 5); }
    static inline int BIT_L_6(Z80* ctx) { return ctx->BIT_R(0b101, 6); }
    static inline int BIT_L_7(Z80* ctx) { return ctx->BIT_R(0b101, 7); }
    static inline int BIT_A_0(Z80* ctx) { return ctx->BIT_R(0b111, 0); }
    static inline int BIT_A_1(Z80* ctx) { return ctx->BIT_R(0b111, 1); }
    static inline int BIT_A_2(Z80* ctx) { return ctx->BIT_R(0b111, 2); }
    static inline int BIT_A_3(Z80* ctx) { return ctx->BIT_R(0b111, 3); }
    static inline int BIT_A_4(Z80* ctx) { return ctx->BIT_R(0b111, 4); }
    static inline int BIT_A_5(Z80* ctx) { return ctx->BIT_R(0b111, 5); }
    static inline int BIT_A_6(Z80* ctx) { return ctx->BIT_R(0b111, 6); }
    static inline int BIT_A_7(Z80* ctx) { return ctx->BIT_R(0b111, 7); }
    inline int BIT_R(unsigned char r, unsigned char bit)
    {
        unsigned char* rp = getRegisterPointer(r);
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
        setFlagZ(n ? false : true);
        setFlagPV(isFlagZ());
        setFlagS(!isFlagZ() && 7 == bit);
        setFlagH(true);
        setFlagN(false);
        setFlagXY(*rp);
        reg.PC += 2;
        return 0;
    }

    // Test BIT b of lacation (HL)
    static inline int BIT_HL_0(Z80* ctx) { return ctx->BIT_HL(0); }
    static inline int BIT_HL_1(Z80* ctx) { return ctx->BIT_HL(1); }
    static inline int BIT_HL_2(Z80* ctx) { return ctx->BIT_HL(2); }
    static inline int BIT_HL_3(Z80* ctx) { return ctx->BIT_HL(3); }
    static inline int BIT_HL_4(Z80* ctx) { return ctx->BIT_HL(4); }
    static inline int BIT_HL_5(Z80* ctx) { return ctx->BIT_HL(5); }
    static inline int BIT_HL_6(Z80* ctx) { return ctx->BIT_HL(6); }
    static inline int BIT_HL_7(Z80* ctx) { return ctx->BIT_HL(7); }
    inline int BIT_HL(unsigned char bit)
    {
        unsigned short addr = getHL();
        unsigned char n = readByte(addr);
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
        setFlagS(!isFlagZ() && 7 == bit);
        setFlagH(true);
        setFlagN(false);
        setFlagXY((reg.WZ & 0xFF00) >> 8);
        reg.PC += 2;
        return 0;
    }

    // Test BIT b of lacation (IX+d)
    static inline int BIT_IX_0(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 0); }
    static inline int BIT_IX_1(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 1); }
    static inline int BIT_IX_2(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 2); }
    static inline int BIT_IX_3(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 3); }
    static inline int BIT_IX_4(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 4); }
    static inline int BIT_IX_5(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 5); }
    static inline int BIT_IX_6(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 6); }
    static inline int BIT_IX_7(Z80* ctx, signed char d) { return ctx->BIT_IX(d, 7); }
    inline int BIT_IX(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
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
        setFlagS(!isFlagZ() && 7 == bit);
        setFlagH(true);
        setFlagN(false);
        setFlagXY((reg.WZ & 0xFF00) >> 8);
        reg.PC += 4;
        return 0;
    }

    // Test BIT b of lacation (IY+d)
    static inline int BIT_IY_0(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 0); }
    static inline int BIT_IY_1(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 1); }
    static inline int BIT_IY_2(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 2); }
    static inline int BIT_IY_3(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 3); }
    static inline int BIT_IY_4(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 4); }
    static inline int BIT_IY_5(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 5); }
    static inline int BIT_IY_6(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 6); }
    static inline int BIT_IY_7(Z80* ctx, signed char d) { return ctx->BIT_IY(d, 7); }
    inline int BIT_IY(signed char d, unsigned char bit)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
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
        setFlagS(!isFlagZ() && 7 == bit);
        setFlagH(true);
        setFlagN(false);
        setFlagXY((reg.WZ & 0xFF00) >> 8);
        reg.PC += 4;
        return 0;
    }

    // SET bit b of register r
    static inline int SET_B_0(Z80* ctx) { return ctx->SET_R(0b000, 0); }
    static inline int SET_B_1(Z80* ctx) { return ctx->SET_R(0b000, 1); }
    static inline int SET_B_2(Z80* ctx) { return ctx->SET_R(0b000, 2); }
    static inline int SET_B_3(Z80* ctx) { return ctx->SET_R(0b000, 3); }
    static inline int SET_B_4(Z80* ctx) { return ctx->SET_R(0b000, 4); }
    static inline int SET_B_5(Z80* ctx) { return ctx->SET_R(0b000, 5); }
    static inline int SET_B_6(Z80* ctx) { return ctx->SET_R(0b000, 6); }
    static inline int SET_B_7(Z80* ctx) { return ctx->SET_R(0b000, 7); }
    static inline int SET_C_0(Z80* ctx) { return ctx->SET_R(0b001, 0); }
    static inline int SET_C_1(Z80* ctx) { return ctx->SET_R(0b001, 1); }
    static inline int SET_C_2(Z80* ctx) { return ctx->SET_R(0b001, 2); }
    static inline int SET_C_3(Z80* ctx) { return ctx->SET_R(0b001, 3); }
    static inline int SET_C_4(Z80* ctx) { return ctx->SET_R(0b001, 4); }
    static inline int SET_C_5(Z80* ctx) { return ctx->SET_R(0b001, 5); }
    static inline int SET_C_6(Z80* ctx) { return ctx->SET_R(0b001, 6); }
    static inline int SET_C_7(Z80* ctx) { return ctx->SET_R(0b001, 7); }
    static inline int SET_D_0(Z80* ctx) { return ctx->SET_R(0b010, 0); }
    static inline int SET_D_1(Z80* ctx) { return ctx->SET_R(0b010, 1); }
    static inline int SET_D_2(Z80* ctx) { return ctx->SET_R(0b010, 2); }
    static inline int SET_D_3(Z80* ctx) { return ctx->SET_R(0b010, 3); }
    static inline int SET_D_4(Z80* ctx) { return ctx->SET_R(0b010, 4); }
    static inline int SET_D_5(Z80* ctx) { return ctx->SET_R(0b010, 5); }
    static inline int SET_D_6(Z80* ctx) { return ctx->SET_R(0b010, 6); }
    static inline int SET_D_7(Z80* ctx) { return ctx->SET_R(0b010, 7); }
    static inline int SET_E_0(Z80* ctx) { return ctx->SET_R(0b011, 0); }
    static inline int SET_E_1(Z80* ctx) { return ctx->SET_R(0b011, 1); }
    static inline int SET_E_2(Z80* ctx) { return ctx->SET_R(0b011, 2); }
    static inline int SET_E_3(Z80* ctx) { return ctx->SET_R(0b011, 3); }
    static inline int SET_E_4(Z80* ctx) { return ctx->SET_R(0b011, 4); }
    static inline int SET_E_5(Z80* ctx) { return ctx->SET_R(0b011, 5); }
    static inline int SET_E_6(Z80* ctx) { return ctx->SET_R(0b011, 6); }
    static inline int SET_E_7(Z80* ctx) { return ctx->SET_R(0b011, 7); }
    static inline int SET_H_0(Z80* ctx) { return ctx->SET_R(0b100, 0); }
    static inline int SET_H_1(Z80* ctx) { return ctx->SET_R(0b100, 1); }
    static inline int SET_H_2(Z80* ctx) { return ctx->SET_R(0b100, 2); }
    static inline int SET_H_3(Z80* ctx) { return ctx->SET_R(0b100, 3); }
    static inline int SET_H_4(Z80* ctx) { return ctx->SET_R(0b100, 4); }
    static inline int SET_H_5(Z80* ctx) { return ctx->SET_R(0b100, 5); }
    static inline int SET_H_6(Z80* ctx) { return ctx->SET_R(0b100, 6); }
    static inline int SET_H_7(Z80* ctx) { return ctx->SET_R(0b100, 7); }
    static inline int SET_L_0(Z80* ctx) { return ctx->SET_R(0b101, 0); }
    static inline int SET_L_1(Z80* ctx) { return ctx->SET_R(0b101, 1); }
    static inline int SET_L_2(Z80* ctx) { return ctx->SET_R(0b101, 2); }
    static inline int SET_L_3(Z80* ctx) { return ctx->SET_R(0b101, 3); }
    static inline int SET_L_4(Z80* ctx) { return ctx->SET_R(0b101, 4); }
    static inline int SET_L_5(Z80* ctx) { return ctx->SET_R(0b101, 5); }
    static inline int SET_L_6(Z80* ctx) { return ctx->SET_R(0b101, 6); }
    static inline int SET_L_7(Z80* ctx) { return ctx->SET_R(0b101, 7); }
    static inline int SET_A_0(Z80* ctx) { return ctx->SET_R(0b111, 0); }
    static inline int SET_A_1(Z80* ctx) { return ctx->SET_R(0b111, 1); }
    static inline int SET_A_2(Z80* ctx) { return ctx->SET_R(0b111, 2); }
    static inline int SET_A_3(Z80* ctx) { return ctx->SET_R(0b111, 3); }
    static inline int SET_A_4(Z80* ctx) { return ctx->SET_R(0b111, 4); }
    static inline int SET_A_5(Z80* ctx) { return ctx->SET_R(0b111, 5); }
    static inline int SET_A_6(Z80* ctx) { return ctx->SET_R(0b111, 6); }
    static inline int SET_A_7(Z80* ctx) { return ctx->SET_R(0b111, 7); }
    inline int SET_R(unsigned char r, unsigned char bit)
    {
        unsigned char* rp = getRegisterPointer(r);
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
    static inline int SET_HL_0(Z80* ctx) { return ctx->SET_HL(0); }
    static inline int SET_HL_1(Z80* ctx) { return ctx->SET_HL(1); }
    static inline int SET_HL_2(Z80* ctx) { return ctx->SET_HL(2); }
    static inline int SET_HL_3(Z80* ctx) { return ctx->SET_HL(3); }
    static inline int SET_HL_4(Z80* ctx) { return ctx->SET_HL(4); }
    static inline int SET_HL_5(Z80* ctx) { return ctx->SET_HL(5); }
    static inline int SET_HL_6(Z80* ctx) { return ctx->SET_HL(6); }
    static inline int SET_HL_7(Z80* ctx) { return ctx->SET_HL(7); }
    inline int SET_HL(unsigned char bit)
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
    static inline int SET_IX_0(Z80* ctx, signed char d) { return ctx->SET_IX(d, 0); }
    static inline int SET_IX_1(Z80* ctx, signed char d) { return ctx->SET_IX(d, 1); }
    static inline int SET_IX_2(Z80* ctx, signed char d) { return ctx->SET_IX(d, 2); }
    static inline int SET_IX_3(Z80* ctx, signed char d) { return ctx->SET_IX(d, 3); }
    static inline int SET_IX_4(Z80* ctx, signed char d) { return ctx->SET_IX(d, 4); }
    static inline int SET_IX_5(Z80* ctx, signed char d) { return ctx->SET_IX(d, 5); }
    static inline int SET_IX_6(Z80* ctx, signed char d) { return ctx->SET_IX(d, 6); }
    static inline int SET_IX_7(Z80* ctx, signed char d) { return ctx->SET_IX(d, 7); }
    inline int SET_IX(signed char d, unsigned char bit, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SET (IX+d<$%04X>) = $%02X of bit-%d%s", reg.PC, addr, n, bit, extraLog ? extraLog : "");
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
        if (rp) *rp = n;
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // SET bit b of lacation (IX+d) with load Reg.
    static inline int SET_IX_0_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 0, 0b000); }
    static inline int SET_IX_1_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 1, 0b000); }
    static inline int SET_IX_2_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 2, 0b000); }
    static inline int SET_IX_3_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 3, 0b000); }
    static inline int SET_IX_4_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 4, 0b000); }
    static inline int SET_IX_5_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 5, 0b000); }
    static inline int SET_IX_6_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 6, 0b000); }
    static inline int SET_IX_7_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 7, 0b000); }
    static inline int SET_IX_0_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 0, 0b001); }
    static inline int SET_IX_1_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 1, 0b001); }
    static inline int SET_IX_2_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 2, 0b001); }
    static inline int SET_IX_3_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 3, 0b001); }
    static inline int SET_IX_4_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 4, 0b001); }
    static inline int SET_IX_5_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 5, 0b001); }
    static inline int SET_IX_6_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 6, 0b001); }
    static inline int SET_IX_7_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 7, 0b001); }
    static inline int SET_IX_0_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 0, 0b010); }
    static inline int SET_IX_1_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 1, 0b010); }
    static inline int SET_IX_2_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 2, 0b010); }
    static inline int SET_IX_3_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 3, 0b010); }
    static inline int SET_IX_4_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 4, 0b010); }
    static inline int SET_IX_5_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 5, 0b010); }
    static inline int SET_IX_6_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 6, 0b010); }
    static inline int SET_IX_7_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 7, 0b010); }
    static inline int SET_IX_0_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 0, 0b011); }
    static inline int SET_IX_1_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 1, 0b011); }
    static inline int SET_IX_2_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 2, 0b011); }
    static inline int SET_IX_3_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 3, 0b011); }
    static inline int SET_IX_4_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 4, 0b011); }
    static inline int SET_IX_5_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 5, 0b011); }
    static inline int SET_IX_6_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 6, 0b011); }
    static inline int SET_IX_7_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 7, 0b011); }
    static inline int SET_IX_0_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 0, 0b100); }
    static inline int SET_IX_1_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 1, 0b100); }
    static inline int SET_IX_2_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 2, 0b100); }
    static inline int SET_IX_3_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 3, 0b100); }
    static inline int SET_IX_4_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 4, 0b100); }
    static inline int SET_IX_5_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 5, 0b100); }
    static inline int SET_IX_6_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 6, 0b100); }
    static inline int SET_IX_7_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 7, 0b100); }
    static inline int SET_IX_0_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 0, 0b101); }
    static inline int SET_IX_1_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 1, 0b101); }
    static inline int SET_IX_2_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 2, 0b101); }
    static inline int SET_IX_3_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 3, 0b101); }
    static inline int SET_IX_4_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 4, 0b101); }
    static inline int SET_IX_5_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 5, 0b101); }
    static inline int SET_IX_6_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 6, 0b101); }
    static inline int SET_IX_7_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 7, 0b101); }
    static inline int SET_IX_0_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 0, 0b111); }
    static inline int SET_IX_1_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 1, 0b111); }
    static inline int SET_IX_2_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 2, 0b111); }
    static inline int SET_IX_3_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 3, 0b111); }
    static inline int SET_IX_4_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 4, 0b111); }
    static inline int SET_IX_5_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 5, 0b111); }
    static inline int SET_IX_6_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 6, 0b111); }
    static inline int SET_IX_7_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IX_with_LD(d, 7, 0b111); }
    inline int SET_IX_with_LD(signed char d, unsigned char bit, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SET_IX(d, bit, rp, buf);
    }

    // SET bit b of lacation (IY+d)
    static inline int SET_IY_0(Z80* ctx, signed char d) { return ctx->SET_IY(d, 0); }
    static inline int SET_IY_1(Z80* ctx, signed char d) { return ctx->SET_IY(d, 1); }
    static inline int SET_IY_2(Z80* ctx, signed char d) { return ctx->SET_IY(d, 2); }
    static inline int SET_IY_3(Z80* ctx, signed char d) { return ctx->SET_IY(d, 3); }
    static inline int SET_IY_4(Z80* ctx, signed char d) { return ctx->SET_IY(d, 4); }
    static inline int SET_IY_5(Z80* ctx, signed char d) { return ctx->SET_IY(d, 5); }
    static inline int SET_IY_6(Z80* ctx, signed char d) { return ctx->SET_IY(d, 6); }
    static inline int SET_IY_7(Z80* ctx, signed char d) { return ctx->SET_IY(d, 7); }
    inline int SET_IY(signed char d, unsigned char bit, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] SET (IY+d<$%04X>) = $%02X of bit-%d%s", reg.PC, addr, n, bit, extraLog ? extraLog : "");
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
        if (rp) *rp = n;
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // SET bit b of lacation (IY+d) with load Reg.
    static inline int SET_IY_0_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 0, 0b000); }
    static inline int SET_IY_1_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 1, 0b000); }
    static inline int SET_IY_2_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 2, 0b000); }
    static inline int SET_IY_3_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 3, 0b000); }
    static inline int SET_IY_4_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 4, 0b000); }
    static inline int SET_IY_5_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 5, 0b000); }
    static inline int SET_IY_6_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 6, 0b000); }
    static inline int SET_IY_7_with_LD_B(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 7, 0b000); }
    static inline int SET_IY_0_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 0, 0b001); }
    static inline int SET_IY_1_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 1, 0b001); }
    static inline int SET_IY_2_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 2, 0b001); }
    static inline int SET_IY_3_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 3, 0b001); }
    static inline int SET_IY_4_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 4, 0b001); }
    static inline int SET_IY_5_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 5, 0b001); }
    static inline int SET_IY_6_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 6, 0b001); }
    static inline int SET_IY_7_with_LD_C(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 7, 0b001); }
    static inline int SET_IY_0_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 0, 0b010); }
    static inline int SET_IY_1_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 1, 0b010); }
    static inline int SET_IY_2_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 2, 0b010); }
    static inline int SET_IY_3_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 3, 0b010); }
    static inline int SET_IY_4_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 4, 0b010); }
    static inline int SET_IY_5_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 5, 0b010); }
    static inline int SET_IY_6_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 6, 0b010); }
    static inline int SET_IY_7_with_LD_D(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 7, 0b010); }
    static inline int SET_IY_0_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 0, 0b011); }
    static inline int SET_IY_1_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 1, 0b011); }
    static inline int SET_IY_2_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 2, 0b011); }
    static inline int SET_IY_3_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 3, 0b011); }
    static inline int SET_IY_4_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 4, 0b011); }
    static inline int SET_IY_5_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 5, 0b011); }
    static inline int SET_IY_6_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 6, 0b011); }
    static inline int SET_IY_7_with_LD_E(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 7, 0b011); }
    static inline int SET_IY_0_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 0, 0b100); }
    static inline int SET_IY_1_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 1, 0b100); }
    static inline int SET_IY_2_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 2, 0b100); }
    static inline int SET_IY_3_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 3, 0b100); }
    static inline int SET_IY_4_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 4, 0b100); }
    static inline int SET_IY_5_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 5, 0b100); }
    static inline int SET_IY_6_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 6, 0b100); }
    static inline int SET_IY_7_with_LD_H(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 7, 0b100); }
    static inline int SET_IY_0_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 0, 0b101); }
    static inline int SET_IY_1_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 1, 0b101); }
    static inline int SET_IY_2_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 2, 0b101); }
    static inline int SET_IY_3_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 3, 0b101); }
    static inline int SET_IY_4_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 4, 0b101); }
    static inline int SET_IY_5_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 5, 0b101); }
    static inline int SET_IY_6_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 6, 0b101); }
    static inline int SET_IY_7_with_LD_L(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 7, 0b101); }
    static inline int SET_IY_0_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 0, 0b111); }
    static inline int SET_IY_1_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 1, 0b111); }
    static inline int SET_IY_2_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 2, 0b111); }
    static inline int SET_IY_3_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 3, 0b111); }
    static inline int SET_IY_4_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 4, 0b111); }
    static inline int SET_IY_5_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 5, 0b111); }
    static inline int SET_IY_6_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 6, 0b111); }
    static inline int SET_IY_7_with_LD_A(Z80* ctx, signed char d) { return ctx->SET_IY_with_LD(d, 7, 0b111); }
    inline int SET_IY_with_LD(signed char d, unsigned char bit, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return SET_IY(d, bit, rp, buf);
    }

    // RESET bit b of register r
    static inline int RES_B_0(Z80* ctx) { return ctx->RES_R(0b000, 0); }
    static inline int RES_B_1(Z80* ctx) { return ctx->RES_R(0b000, 1); }
    static inline int RES_B_2(Z80* ctx) { return ctx->RES_R(0b000, 2); }
    static inline int RES_B_3(Z80* ctx) { return ctx->RES_R(0b000, 3); }
    static inline int RES_B_4(Z80* ctx) { return ctx->RES_R(0b000, 4); }
    static inline int RES_B_5(Z80* ctx) { return ctx->RES_R(0b000, 5); }
    static inline int RES_B_6(Z80* ctx) { return ctx->RES_R(0b000, 6); }
    static inline int RES_B_7(Z80* ctx) { return ctx->RES_R(0b000, 7); }
    static inline int RES_C_0(Z80* ctx) { return ctx->RES_R(0b001, 0); }
    static inline int RES_C_1(Z80* ctx) { return ctx->RES_R(0b001, 1); }
    static inline int RES_C_2(Z80* ctx) { return ctx->RES_R(0b001, 2); }
    static inline int RES_C_3(Z80* ctx) { return ctx->RES_R(0b001, 3); }
    static inline int RES_C_4(Z80* ctx) { return ctx->RES_R(0b001, 4); }
    static inline int RES_C_5(Z80* ctx) { return ctx->RES_R(0b001, 5); }
    static inline int RES_C_6(Z80* ctx) { return ctx->RES_R(0b001, 6); }
    static inline int RES_C_7(Z80* ctx) { return ctx->RES_R(0b001, 7); }
    static inline int RES_D_0(Z80* ctx) { return ctx->RES_R(0b010, 0); }
    static inline int RES_D_1(Z80* ctx) { return ctx->RES_R(0b010, 1); }
    static inline int RES_D_2(Z80* ctx) { return ctx->RES_R(0b010, 2); }
    static inline int RES_D_3(Z80* ctx) { return ctx->RES_R(0b010, 3); }
    static inline int RES_D_4(Z80* ctx) { return ctx->RES_R(0b010, 4); }
    static inline int RES_D_5(Z80* ctx) { return ctx->RES_R(0b010, 5); }
    static inline int RES_D_6(Z80* ctx) { return ctx->RES_R(0b010, 6); }
    static inline int RES_D_7(Z80* ctx) { return ctx->RES_R(0b010, 7); }
    static inline int RES_E_0(Z80* ctx) { return ctx->RES_R(0b011, 0); }
    static inline int RES_E_1(Z80* ctx) { return ctx->RES_R(0b011, 1); }
    static inline int RES_E_2(Z80* ctx) { return ctx->RES_R(0b011, 2); }
    static inline int RES_E_3(Z80* ctx) { return ctx->RES_R(0b011, 3); }
    static inline int RES_E_4(Z80* ctx) { return ctx->RES_R(0b011, 4); }
    static inline int RES_E_5(Z80* ctx) { return ctx->RES_R(0b011, 5); }
    static inline int RES_E_6(Z80* ctx) { return ctx->RES_R(0b011, 6); }
    static inline int RES_E_7(Z80* ctx) { return ctx->RES_R(0b011, 7); }
    static inline int RES_H_0(Z80* ctx) { return ctx->RES_R(0b100, 0); }
    static inline int RES_H_1(Z80* ctx) { return ctx->RES_R(0b100, 1); }
    static inline int RES_H_2(Z80* ctx) { return ctx->RES_R(0b100, 2); }
    static inline int RES_H_3(Z80* ctx) { return ctx->RES_R(0b100, 3); }
    static inline int RES_H_4(Z80* ctx) { return ctx->RES_R(0b100, 4); }
    static inline int RES_H_5(Z80* ctx) { return ctx->RES_R(0b100, 5); }
    static inline int RES_H_6(Z80* ctx) { return ctx->RES_R(0b100, 6); }
    static inline int RES_H_7(Z80* ctx) { return ctx->RES_R(0b100, 7); }
    static inline int RES_L_0(Z80* ctx) { return ctx->RES_R(0b101, 0); }
    static inline int RES_L_1(Z80* ctx) { return ctx->RES_R(0b101, 1); }
    static inline int RES_L_2(Z80* ctx) { return ctx->RES_R(0b101, 2); }
    static inline int RES_L_3(Z80* ctx) { return ctx->RES_R(0b101, 3); }
    static inline int RES_L_4(Z80* ctx) { return ctx->RES_R(0b101, 4); }
    static inline int RES_L_5(Z80* ctx) { return ctx->RES_R(0b101, 5); }
    static inline int RES_L_6(Z80* ctx) { return ctx->RES_R(0b101, 6); }
    static inline int RES_L_7(Z80* ctx) { return ctx->RES_R(0b101, 7); }
    static inline int RES_A_0(Z80* ctx) { return ctx->RES_R(0b111, 0); }
    static inline int RES_A_1(Z80* ctx) { return ctx->RES_R(0b111, 1); }
    static inline int RES_A_2(Z80* ctx) { return ctx->RES_R(0b111, 2); }
    static inline int RES_A_3(Z80* ctx) { return ctx->RES_R(0b111, 3); }
    static inline int RES_A_4(Z80* ctx) { return ctx->RES_R(0b111, 4); }
    static inline int RES_A_5(Z80* ctx) { return ctx->RES_R(0b111, 5); }
    static inline int RES_A_6(Z80* ctx) { return ctx->RES_R(0b111, 6); }
    static inline int RES_A_7(Z80* ctx) { return ctx->RES_R(0b111, 7); }
    inline int RES_R(unsigned char r, unsigned char bit)
    {
        unsigned char* rp = getRegisterPointer(r);
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
    static inline int RES_HL_0(Z80* ctx) { return ctx->RES_HL(0); }
    static inline int RES_HL_1(Z80* ctx) { return ctx->RES_HL(1); }
    static inline int RES_HL_2(Z80* ctx) { return ctx->RES_HL(2); }
    static inline int RES_HL_3(Z80* ctx) { return ctx->RES_HL(3); }
    static inline int RES_HL_4(Z80* ctx) { return ctx->RES_HL(4); }
    static inline int RES_HL_5(Z80* ctx) { return ctx->RES_HL(5); }
    static inline int RES_HL_6(Z80* ctx) { return ctx->RES_HL(6); }
    static inline int RES_HL_7(Z80* ctx) { return ctx->RES_HL(7); }
    inline int RES_HL(unsigned char bit)
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
    static inline int RES_IX_0(Z80* ctx, signed char d) { return ctx->RES_IX(d, 0); }
    static inline int RES_IX_1(Z80* ctx, signed char d) { return ctx->RES_IX(d, 1); }
    static inline int RES_IX_2(Z80* ctx, signed char d) { return ctx->RES_IX(d, 2); }
    static inline int RES_IX_3(Z80* ctx, signed char d) { return ctx->RES_IX(d, 3); }
    static inline int RES_IX_4(Z80* ctx, signed char d) { return ctx->RES_IX(d, 4); }
    static inline int RES_IX_5(Z80* ctx, signed char d) { return ctx->RES_IX(d, 5); }
    static inline int RES_IX_6(Z80* ctx, signed char d) { return ctx->RES_IX(d, 6); }
    static inline int RES_IX_7(Z80* ctx, signed char d) { return ctx->RES_IX(d, 7); }
    inline int RES_IX(signed char d, unsigned char bit, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RES (IX+d<$%04X>) = $%02X of bit-%d%s", reg.PC, addr, n, bit, extraLog ? extraLog : "");
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
        if (rp) *rp = n;
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // RESET bit b of lacation (IX+d) with load Reg.
    static inline int RES_IX_0_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 0, 0b000); }
    static inline int RES_IX_1_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 1, 0b000); }
    static inline int RES_IX_2_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 2, 0b000); }
    static inline int RES_IX_3_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 3, 0b000); }
    static inline int RES_IX_4_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 4, 0b000); }
    static inline int RES_IX_5_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 5, 0b000); }
    static inline int RES_IX_6_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 6, 0b000); }
    static inline int RES_IX_7_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 7, 0b000); }
    static inline int RES_IX_0_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 0, 0b001); }
    static inline int RES_IX_1_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 1, 0b001); }
    static inline int RES_IX_2_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 2, 0b001); }
    static inline int RES_IX_3_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 3, 0b001); }
    static inline int RES_IX_4_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 4, 0b001); }
    static inline int RES_IX_5_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 5, 0b001); }
    static inline int RES_IX_6_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 6, 0b001); }
    static inline int RES_IX_7_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 7, 0b001); }
    static inline int RES_IX_0_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 0, 0b010); }
    static inline int RES_IX_1_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 1, 0b010); }
    static inline int RES_IX_2_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 2, 0b010); }
    static inline int RES_IX_3_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 3, 0b010); }
    static inline int RES_IX_4_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 4, 0b010); }
    static inline int RES_IX_5_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 5, 0b010); }
    static inline int RES_IX_6_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 6, 0b010); }
    static inline int RES_IX_7_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 7, 0b010); }
    static inline int RES_IX_0_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 0, 0b011); }
    static inline int RES_IX_1_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 1, 0b011); }
    static inline int RES_IX_2_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 2, 0b011); }
    static inline int RES_IX_3_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 3, 0b011); }
    static inline int RES_IX_4_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 4, 0b011); }
    static inline int RES_IX_5_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 5, 0b011); }
    static inline int RES_IX_6_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 6, 0b011); }
    static inline int RES_IX_7_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 7, 0b011); }
    static inline int RES_IX_0_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 0, 0b100); }
    static inline int RES_IX_1_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 1, 0b100); }
    static inline int RES_IX_2_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 2, 0b100); }
    static inline int RES_IX_3_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 3, 0b100); }
    static inline int RES_IX_4_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 4, 0b100); }
    static inline int RES_IX_5_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 5, 0b100); }
    static inline int RES_IX_6_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 6, 0b100); }
    static inline int RES_IX_7_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 7, 0b100); }
    static inline int RES_IX_0_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 0, 0b101); }
    static inline int RES_IX_1_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 1, 0b101); }
    static inline int RES_IX_2_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 2, 0b101); }
    static inline int RES_IX_3_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 3, 0b101); }
    static inline int RES_IX_4_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 4, 0b101); }
    static inline int RES_IX_5_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 5, 0b101); }
    static inline int RES_IX_6_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 6, 0b101); }
    static inline int RES_IX_7_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 7, 0b101); }
    static inline int RES_IX_0_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 0, 0b111); }
    static inline int RES_IX_1_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 1, 0b111); }
    static inline int RES_IX_2_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 2, 0b111); }
    static inline int RES_IX_3_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 3, 0b111); }
    static inline int RES_IX_4_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 4, 0b111); }
    static inline int RES_IX_5_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 5, 0b111); }
    static inline int RES_IX_6_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 6, 0b111); }
    static inline int RES_IX_7_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IX_with_LD(d, 7, 0b111); }
    inline int RES_IX_with_LD(signed char d, unsigned char bit, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RES_IX(d, bit, rp, buf);
    }

    // RESET bit b of lacation (IY+d) with load Reg.
    static inline int RES_IY_0_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 0, 0b000); }
    static inline int RES_IY_1_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 1, 0b000); }
    static inline int RES_IY_2_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 2, 0b000); }
    static inline int RES_IY_3_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 3, 0b000); }
    static inline int RES_IY_4_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 4, 0b000); }
    static inline int RES_IY_5_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 5, 0b000); }
    static inline int RES_IY_6_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 6, 0b000); }
    static inline int RES_IY_7_with_LD_B(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 7, 0b000); }
    static inline int RES_IY_0_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 0, 0b001); }
    static inline int RES_IY_1_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 1, 0b001); }
    static inline int RES_IY_2_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 2, 0b001); }
    static inline int RES_IY_3_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 3, 0b001); }
    static inline int RES_IY_4_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 4, 0b001); }
    static inline int RES_IY_5_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 5, 0b001); }
    static inline int RES_IY_6_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 6, 0b001); }
    static inline int RES_IY_7_with_LD_C(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 7, 0b001); }
    static inline int RES_IY_0_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 0, 0b010); }
    static inline int RES_IY_1_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 1, 0b010); }
    static inline int RES_IY_2_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 2, 0b010); }
    static inline int RES_IY_3_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 3, 0b010); }
    static inline int RES_IY_4_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 4, 0b010); }
    static inline int RES_IY_5_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 5, 0b010); }
    static inline int RES_IY_6_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 6, 0b010); }
    static inline int RES_IY_7_with_LD_D(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 7, 0b010); }
    static inline int RES_IY_0_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 0, 0b011); }
    static inline int RES_IY_1_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 1, 0b011); }
    static inline int RES_IY_2_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 2, 0b011); }
    static inline int RES_IY_3_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 3, 0b011); }
    static inline int RES_IY_4_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 4, 0b011); }
    static inline int RES_IY_5_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 5, 0b011); }
    static inline int RES_IY_6_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 6, 0b011); }
    static inline int RES_IY_7_with_LD_E(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 7, 0b011); }
    static inline int RES_IY_0_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 0, 0b100); }
    static inline int RES_IY_1_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 1, 0b100); }
    static inline int RES_IY_2_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 2, 0b100); }
    static inline int RES_IY_3_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 3, 0b100); }
    static inline int RES_IY_4_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 4, 0b100); }
    static inline int RES_IY_5_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 5, 0b100); }
    static inline int RES_IY_6_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 6, 0b100); }
    static inline int RES_IY_7_with_LD_H(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 7, 0b100); }
    static inline int RES_IY_0_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 0, 0b101); }
    static inline int RES_IY_1_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 1, 0b101); }
    static inline int RES_IY_2_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 2, 0b101); }
    static inline int RES_IY_3_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 3, 0b101); }
    static inline int RES_IY_4_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 4, 0b101); }
    static inline int RES_IY_5_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 5, 0b101); }
    static inline int RES_IY_6_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 6, 0b101); }
    static inline int RES_IY_7_with_LD_L(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 7, 0b101); }
    static inline int RES_IY_0_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 0, 0b111); }
    static inline int RES_IY_1_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 1, 0b111); }
    static inline int RES_IY_2_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 2, 0b111); }
    static inline int RES_IY_3_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 3, 0b111); }
    static inline int RES_IY_4_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 4, 0b111); }
    static inline int RES_IY_5_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 5, 0b111); }
    static inline int RES_IY_6_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 6, 0b111); }
    static inline int RES_IY_7_with_LD_A(Z80* ctx, signed char d) { return ctx->RES_IY_with_LD(d, 7, 0b111); }
    inline int RES_IY_with_LD(signed char d, unsigned char bit, unsigned char r)
    {
        char buf[80];
        unsigned char* rp = getRegisterPointer(r);
        if (isDebug()) {
            sprintf(buf, " --> %s", registerDump(r));
        } else {
            buf[0] = '\0';
        }
        return RES_IY(d, bit, rp, buf);
    }

    // RESET bit b of lacation (IY+d)
    static inline int RES_IY_0(Z80* ctx, signed char d) { return ctx->RES_IY(d, 0); }
    static inline int RES_IY_1(Z80* ctx, signed char d) { return ctx->RES_IY(d, 1); }
    static inline int RES_IY_2(Z80* ctx, signed char d) { return ctx->RES_IY(d, 2); }
    static inline int RES_IY_3(Z80* ctx, signed char d) { return ctx->RES_IY(d, 3); }
    static inline int RES_IY_4(Z80* ctx, signed char d) { return ctx->RES_IY(d, 4); }
    static inline int RES_IY_5(Z80* ctx, signed char d) { return ctx->RES_IY(d, 5); }
    static inline int RES_IY_6(Z80* ctx, signed char d) { return ctx->RES_IY(d, 6); }
    static inline int RES_IY_7(Z80* ctx, signed char d) { return ctx->RES_IY(d, 7); }
    inline int RES_IY(signed char d, unsigned char bit, unsigned char* rp = NULL, const char* extraLog = NULL)
    {
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] RES (IY+d<$%04X>) = $%02X of bit-%d%s", reg.PC, addr, n, bit, extraLog ? extraLog : "");
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
        if (rp) *rp = n;
        writeByte(addr, n, 3);
        reg.PC += 4;
        return 0;
    }

    // Compare location (HL) and A, increment/decrement HL and decrement BC
    inline int repeatCP(bool isIncHL, bool isRepeat)
    {
        unsigned short hl = getHL();
        unsigned short bc = getBC();
        unsigned char n = readByte(hl);
        if (isDebug()) {
            if (isIncHL) {
                log("[%04X] %s ... %s, %s = $%02X, %s", reg.PC, isRepeat ? "CPIR" : "CPI", registerDump(0b111), registerPairDump(0b10), n, registerPairDump(0b00));
            } else {
                log("[%04X] %s ... %s, %s = $%02X, %s", reg.PC, isRepeat ? "CPDR" : "CPD", registerDump(0b111), registerPairDump(0b10), n, registerPairDump(0b00));
            }
        }
        subtract8(n, 0, false, false);
        int nn = reg.pair.A;
        nn -= n;
        nn -= isFlagH() ? 1 : 0;
        setFlagY(nn & 0b00000010);
        setFlagX(nn & 0b00001000);
        setHL(hl + (isIncHL ? 1 : -1));
        bc--;
        setBC(bc);
        setFlagPV(0 != bc);
        consumeClock(4);
        if (isRepeat && !isFlagZ() && 0 != getBC()) {
            consumeClock(5);
        } else {
            reg.PC += 2;
        }
        reg.WZ += isIncHL ? 1 : -1;
        return 0;
    }
    inline int CPI() { return repeatCP(true, false); }
    inline int CPIR() { return repeatCP(true, true); }
    inline int CPD() { return repeatCP(false, false); }
    inline int CPDR() { return repeatCP(false, true); }

    // Compare Register
    static inline int CP_B(Z80* ctx) { return ctx->CP_R(0b000); }
    static inline int CP_C(Z80* ctx) { return ctx->CP_R(0b001); }
    static inline int CP_D(Z80* ctx) { return ctx->CP_R(0b010); }
    static inline int CP_E(Z80* ctx) { return ctx->CP_R(0b011); }
    static inline int CP_H(Z80* ctx) { return ctx->CP_R(0b100); }
    static inline int CP_L(Z80* ctx) { return ctx->CP_R(0b101); }
    static inline int CP_A(Z80* ctx) { return ctx->CP_R(0b111); }
    static inline int CP_B_2(Z80* ctx) { return ctx->CP_R(0b000, 2); }
    static inline int CP_C_2(Z80* ctx) { return ctx->CP_R(0b001, 2); }
    static inline int CP_D_2(Z80* ctx) { return ctx->CP_R(0b010, 2); }
    static inline int CP_E_2(Z80* ctx) { return ctx->CP_R(0b011, 2); }
    static inline int CP_A_2(Z80* ctx) { return ctx->CP_R(0b111, 2); }
    inline int CP_R(unsigned char r, int pc = 1)
    {
        if (isDebug()) log("[%04X] CP %s, %s", reg.PC, registerDump(0b111), registerDump(r));
        unsigned char* rp = getRegisterPointer(r);
        subtract8(*rp, 0, true, false);
        reg.PC += pc;
        return 0;
    }

    // Compare Register IXH
    static inline int CP_IXH_(Z80* ctx) { return ctx->CP_IXH(); }
    inline int CP_IXH()
    {
        if (isDebug()) log("[%04X] CP %s, IXH<$%02X>", reg.PC, registerDump(0b111), getIXH());
        subtract8(getIXH(), 0, true, false);
        reg.PC += 2;
        return 0;
    }

    // Compare Register IXL
    static inline int CP_IXL_(Z80* ctx) { return ctx->CP_IXL(); }
    inline int CP_IXL()
    {
        if (isDebug()) log("[%04X] CP %s, IXL<$%02X>", reg.PC, registerDump(0b111), getIXL());
        subtract8(getIXL(), 0, true, false);
        reg.PC += 2;
        return 0;
    }

    // Compare Register IYH
    static inline int CP_IYH_(Z80* ctx) { return ctx->CP_IYH(); }
    inline int CP_IYH()
    {
        if (isDebug()) log("[%04X] CP %s, IYH<$%02X>", reg.PC, registerDump(0b111), getIYH());
        subtract8(getIYH(), 0, true, false);
        reg.PC += 2;
        return 0;
    }

    // Compare Register IYL
    static inline int CP_IYL_(Z80* ctx) { return ctx->CP_IYL(); }
    inline int CP_IYL()
    {
        if (isDebug()) log("[%04X] CP %s, IYL<$%02X>", reg.PC, registerDump(0b111), getIYL());
        subtract8(getIYL(), 0, true, false);
        reg.PC += 2;
        return 0;
    }

    // Compare immediate
    static inline int CP_N(Z80* ctx)
    {
        unsigned char n = ctx->readByte(ctx->reg.PC + 1, 3);
        if (ctx->isDebug()) ctx->log("[%04X] CP %s, $%02X", ctx->reg.PC, ctx->registerDump(0b111), n);
        ctx->subtract8(n, 0, true, false);
        ctx->reg.PC += 2;
        return 0;
    }

    // Compare memory
    static inline int CP_HL(Z80* ctx)
    {
        unsigned short addr = ctx->getHL();
        unsigned char n = ctx->readByte(addr, 3);
        if (ctx->isDebug()) ctx->log("[%04X] CP %s, (%s) = $%02X", ctx->reg.PC, ctx->registerDump(0b111), ctx->registerPairDump(0b10), n);
        ctx->subtract8(n, 0, true, false);
        ctx->reg.PC += 1;
        return 0;
    }

    // Compare memory
    static inline int CP_IX_(Z80* ctx) { return ctx->CP_IX(); }
    inline int CP_IX()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IX + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] CP %s, (IX+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        subtract8(n, 0, true, false);
        reg.PC += 3;
        return consumeClock(3);
    }

    // Compare memory
    static inline int CP_IY_(Z80* ctx) { return ctx->CP_IY(); }
    inline int CP_IY()
    {
        signed char d = readByte(reg.PC + 2);
        unsigned short addr = reg.IY + d;
        unsigned char n = readByte(addr);
        if (isDebug()) log("[%04X] CP %s, (IY+d<$%04X>) = $%02X", reg.PC, registerDump(0b111), addr, n);
        subtract8(n, 0, true, false);
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
        ctx->reg.WZ = addr;
        return 0;
    }

    // Conditional Jump
    static inline int JP_C0_NN(Z80* ctx) { return ctx->JP_C_NN(0); }
    static inline int JP_C1_NN(Z80* ctx) { return ctx->JP_C_NN(1); }
    static inline int JP_C2_NN(Z80* ctx) { return ctx->JP_C_NN(2); }
    static inline int JP_C3_NN(Z80* ctx) { return ctx->JP_C_NN(3); }
    static inline int JP_C4_NN(Z80* ctx) { return ctx->JP_C_NN(4); }
    static inline int JP_C5_NN(Z80* ctx) { return ctx->JP_C_NN(5); }
    static inline int JP_C6_NN(Z80* ctx) { return ctx->JP_C_NN(6); }
    static inline int JP_C7_NN(Z80* ctx) { return ctx->JP_C_NN(7); }
    inline int JP_C_NN(unsigned char c)
    {
        unsigned char nL = readByte(reg.PC + 1, 3);
        unsigned char nH = readByte(reg.PC + 2, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] JP %s, $%04X", reg.PC, conditionDump(c), addr);
        bool jump;
        switch (c) {
            case 0: jump = isFlagZ() ? false : true; break;
            case 1: jump = isFlagZ() ? true : false; break;
            case 2: jump = isFlagC() ? false : true; break;
            case 3: jump = isFlagC() ? true : false; break;
            case 4: jump = isFlagPV() ? false : true; break;
            case 5: jump = isFlagPV() ? true : false; break;
            case 6: jump = isFlagS() ? false : true; break;
            case 7: jump = isFlagS() ? true : false; break;
            default: jump = false;
        }
        if (jump) {
            reg.PC = addr;
        } else {
            reg.PC += 3;
        }
        reg.WZ = addr;
        return 0;
    }

    // Jump Relative to PC+e
    static inline int JR_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1);
        if (ctx->isDebug()) ctx->log("[%04X] JR %s", ctx->reg.PC, ctx->relativeDump(e));
        ctx->reg.PC += e;
        ctx->reg.PC += 2;
        return ctx->consumeClock(4);
    }

    // Jump Relative to PC+e, if carry
    static inline int JR_C_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3);
        bool execute = ctx->isFlagC();
        if (ctx->isDebug()) ctx->log("[%04X] JR C, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += 2;
        if (execute) {
            ctx->reg.PC += e;
            ctx->consumeClock(5);
        }
        return 0;
    }

    // Jump Relative to PC+e, if not carry
    static inline int JR_NC_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3);
        bool execute = !ctx->isFlagC();
        if (ctx->isDebug()) ctx->log("[%04X] JR NC, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += 2;
        if (execute) {
            ctx->reg.PC += e;
            ctx->consumeClock(5);
        }
        return 0;
    }

    // Jump Relative to PC+e, if zero
    static inline int JR_Z_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3);
        bool execute = ctx->isFlagZ();
        if (ctx->isDebug()) ctx->log("[%04X] JR Z, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += 2;
        if (execute) {
            ctx->reg.PC += e;
            ctx->consumeClock(5);
        }
        return 0;
    }

    // Jump Relative to PC+e, if zero
    static inline int JR_NZ_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1, 3);
        bool execute = !ctx->isFlagZ();
        if (ctx->isDebug()) ctx->log("[%04X] JR NZ, %s <%s>", ctx->reg.PC, ctx->relativeDump(e), execute ? "YES" : "NO");
        ctx->reg.PC += 2;
        if (execute) {
            ctx->reg.PC += e;
            ctx->consumeClock(5);
        }
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
    static inline int JP_IX_(Z80* ctx) { return ctx->JP_IX(); }
    inline int JP_IX()
    {
        if (isDebug()) log("[%04X] JP IX<$%04X>", reg.PC, reg.IX);
        reg.PC = reg.IX;
        return 0;
    }

    // Jump to IY
    static inline int JP_IY_(Z80* ctx) { return ctx->JP_IY(); }
    inline int JP_IY()
    {
        if (isDebug()) log("[%04X] JP IY<$%04X>", reg.PC, reg.IY);
        reg.PC = reg.IY;
        return 0;
    }

    // 	Decrement B and Jump relative if B=0
    static inline int DJNZ_E(Z80* ctx)
    {
        signed char e = ctx->readByte(ctx->reg.PC + 1);
        if (ctx->isDebug()) ctx->log("[%04X] DJNZ %s (%s)", ctx->reg.PC, ctx->relativeDump(e), ctx->registerDump(0b000));
        ctx->reg.pair.B--;
        ctx->reg.PC += 2;
        if (ctx->reg.pair.B) {
            ctx->reg.PC += e;
            ctx->consumeClock(5);
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
        ctx->reg.WZ = addr;
        ctx->reg.PC = addr;
        ctx->invokeCallHandlers();
        return 0;
    }

    // Return
    static inline int RET(Z80* ctx)
    {
        ctx->invokeReturnHandlers();
        unsigned char nL = ctx->readByte(ctx->reg.SP, 3);
        unsigned char nH = ctx->readByte(ctx->reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (ctx->isDebug()) ctx->log("[%04X] RET to $%04X (%s)", ctx->reg.PC, addr, ctx->registerPairDump(0b11));
        ctx->reg.SP += 2;
        ctx->reg.PC = addr;
        ctx->reg.WZ = addr;
        return 0;
    }

    // Call with condition
    static inline int CALL_C0_NN(Z80* ctx) { return ctx->CALL_C_NN(0); }
    static inline int CALL_C1_NN(Z80* ctx) { return ctx->CALL_C_NN(1); }
    static inline int CALL_C2_NN(Z80* ctx) { return ctx->CALL_C_NN(2); }
    static inline int CALL_C3_NN(Z80* ctx) { return ctx->CALL_C_NN(3); }
    static inline int CALL_C4_NN(Z80* ctx) { return ctx->CALL_C_NN(4); }
    static inline int CALL_C5_NN(Z80* ctx) { return ctx->CALL_C_NN(5); }
    static inline int CALL_C6_NN(Z80* ctx) { return ctx->CALL_C_NN(6); }
    static inline int CALL_C7_NN(Z80* ctx) { return ctx->CALL_C_NN(7); }
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
            invokeCallHandlers();
        }
        reg.WZ = addr;
        return 0;
    }

    // Return with condition
    static inline int RET_C0(Z80* ctx) { return ctx->RET_C(0); }
    static inline int RET_C1(Z80* ctx) { return ctx->RET_C(1); }
    static inline int RET_C2(Z80* ctx) { return ctx->RET_C(2); }
    static inline int RET_C3(Z80* ctx) { return ctx->RET_C(3); }
    static inline int RET_C4(Z80* ctx) { return ctx->RET_C(4); }
    static inline int RET_C5(Z80* ctx) { return ctx->RET_C(5); }
    static inline int RET_C6(Z80* ctx) { return ctx->RET_C(6); }
    static inline int RET_C7(Z80* ctx) { return ctx->RET_C(7); }
    inline int RET_C(unsigned char c)
    {
        bool execute;
        switch (c) {
            case 0: execute = isFlagZ() ? false : true; break;
            case 1: execute = isFlagZ() ? true : false; break;
            case 2: execute = isFlagC() ? false : true; break;
            case 3: execute = isFlagC() ? true : false; break;
            case 4: execute = isFlagPV() ? false : true; break;
            case 5: execute = isFlagPV() ? true : false; break;
            case 6: execute = isFlagS() ? false : true; break;
            case 7: execute = isFlagS() ? true : false; break;
            default: execute = false;
        }
        if (!execute) {
            if (isDebug()) log("[%04X] RET %s <execute:NO>", reg.PC, conditionDump(c));
            reg.PC++;
            return consumeClock(1);
        }
        invokeReturnHandlers();
        unsigned char nL = readByte(reg.SP);
        unsigned char nH = readByte(reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] RET %s to $%04X (%s) <execute:YES>", reg.PC, conditionDump(c), addr, registerPairDump(0b11));
        reg.SP += 2;
        reg.PC = addr;
        reg.WZ = addr;
        return 0;
    }

    // Return from interrupt
    inline int RETI()
    {
        invokeReturnHandlers();
        unsigned char nL = readByte(reg.SP, 3);
        unsigned char nH = readByte(reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] RETI to $%04X (%s)", reg.PC, addr, registerPairDump(0b11));
        reg.SP += 2;
        reg.PC = addr;
        reg.WZ = addr;
        reg.IFF &= ~IFF_IRQ();
        return 0;
    }

    // Return from non maskable interrupt
    inline int RETN()
    {
        invokeReturnHandlers();
        unsigned char nL = readByte(reg.SP, 3);
        unsigned char nH = readByte(reg.SP + 1, 3);
        unsigned short addr = (nH << 8) + nL;
        if (isDebug()) log("[%04X] RETN to $%04X (%s)", reg.PC, addr, registerPairDump(0b11));
        reg.SP += 2;
        reg.PC = addr;
        reg.WZ = addr;
        reg.IFF &= ~IFF_NMI();
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
    static inline int RST00(Z80* ctx) { return ctx->RST(0, true); }
    static inline int RST08(Z80* ctx) { return ctx->RST(1, true); }
    static inline int RST10(Z80* ctx) { return ctx->RST(2, true); }
    static inline int RST18(Z80* ctx) { return ctx->RST(3, true); }
    static inline int RST20(Z80* ctx) { return ctx->RST(4, true); }
    static inline int RST28(Z80* ctx) { return ctx->RST(5, true); }
    static inline int RST30(Z80* ctx) { return ctx->RST(6, true); }
    static inline int RST38(Z80* ctx) { return ctx->RST(7, true); }
    inline int RST(unsigned char t, bool incrementPC)
    {
        unsigned short addr = t * 8;
        if (isDebug()) log("[%04X] RST $%04X (%s)", reg.PC, addr, registerPairDump(0b11));
        if (incrementPC) reg.PC++;
        unsigned char pcH = (reg.PC & 0xFF00) >> 8;
        unsigned char pcL = reg.PC & 0x00FF;
        writeByte(reg.SP - 1, pcH);
        writeByte(reg.SP - 2, pcL, 3);
        reg.SP -= 2;
        reg.WZ = addr;
        reg.PC = addr;
        invokeCallHandlers();
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
        unsigned char i = inPort(reg.pair.C);
        if (isDebug()) log("[%04X] IN %s, (%s) = $%02X", reg.PC, registerDump(r), registerDump(0b001), i);
        *rp = i;
        setFlagS(i & 0x80);
        setFlagZ(i == 0);
        setFlagH(false);
        setFlagPV(isEvenNumberBits(i));
        setFlagN(false);
        setFlagXY(i);
        reg.PC += 2;
        return 0;
    }

    inline void decrementB_forRepeatIO()
    {
        reg.pair.B--;
        reg.pair.F = 0;
        setFlagC(isFlagC());
        setFlagN(true);
        setFlagZ(reg.pair.B == 0);
        setFlagXY(reg.pair.B);
        setFlagS(reg.pair.B & 0x80);
        setFlagH((reg.pair.B & 0x0F) == 0x0F);
        setFlagPV(reg.pair.B == 0x7F);
    }

    // Load location (HL) with input from port (C); or increment/decrement HL and decrement B
    inline int repeatIN(bool isIncHL, bool isRepeat)
    {
        reg.WZ = getBC() + (isIncHL ? 1 : -1);
        unsigned char i = inPort(reg.pair.C);
        unsigned short hl = getHL();
        if (isDebug()) {
            if (isIncHL) {
                log("[%04X] %s ... (%s) <- p(%s) = $%02X [%s]", reg.PC, isRepeat ? "INIR" : "INI", registerPairDump(0b10), registerDump(0b001), i, registerDump(0b000));
            } else {
                log("[%04X] %s ... (%s) <- p(%s) = $%02X [%s]", reg.PC, isRepeat ? "INDR" : "IND", registerPairDump(0b10), registerDump(0b001), i, registerDump(0b000));
            }
        }
        writeByte(hl, i);
        decrementB_forRepeatIO();
        hl += isIncHL ? 1 : -1;
        setHL(hl);
        setFlagZ(reg.pair.B == 0);
        setFlagN(i & 0x80);                                             // NOTE: undocumented
        setFlagC(0xFF < i + ((reg.pair.C + 1) & 0xFF));                 // NOTE: undocumented
        setFlagH(isFlagC());                                            // NOTE: undocumented
        setFlagPV(i + (((reg.pair.C + 1) & 0xFF) & 0x07) ^ reg.pair.B); // NOTE: undocumented
        if (isRepeat && 0 != reg.pair.B) {
            consumeClock(5);
        } else {
            reg.PC += 2;
        }
        return 0;
    }
    inline int INI() { return repeatIN(true, false); }
    inline int INIR() { return repeatIN(true, true); }
    inline int IND() { return repeatIN(false, false); }
    inline int INDR() { return repeatIN(false, true); }

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
        if (isDebug()) log("[%04X] OUT (%s), %s", reg.PC, registerDump(0b001), registerDump(r));
        outPort(reg.pair.C, *rp);
        reg.PC += 2;
        return 0;
    }

    // Load Output port (C) with location (HL), increment/decrement HL and decrement B
    inline int repeatOUT(bool isIncHL, bool isRepeat)
    {
        unsigned short hl = getHL();
        unsigned char o = readByte(hl);
        if (isDebug()) {
            if (isIncHL) {
                log("[%04X] %s ... p(%s) <- (%s) <$%02x> [%s]", reg.PC, isRepeat ? "OUTIR" : "OUTI", registerDump(0b001), registerPairDump(0b10), o, registerDump(0b000));
            } else {
                log("[%04X] %s ... p(%s) <- (%s) <$%02x> [%s]", reg.PC, isRepeat ? "OUTDR" : "OUTD", registerDump(0b001), registerPairDump(0b10), o, registerDump(0b000));
            }
        }
        outPort(reg.pair.C, o);
        decrementB_forRepeatIO();
        reg.WZ = getBC() + (isIncHL ? 1 : -1);
        hl += isIncHL ? 1 : -1;
        setHL(hl);
        setFlagZ(reg.pair.B == 0);
        setFlagN(o & 0x80);                                // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagH(reg.pair.L + o > 0xFF);                   // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagC(isFlagH());                               // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        setFlagPV(((reg.pair.H + o) & 0x07) ^ reg.pair.B); // NOTE: ACTUAL FLAG CONDITION IS UNKNOWN
        if (isRepeat && 0 != reg.pair.B) {
            consumeClock(5);
        } else {
            reg.PC += 2;
        }
        return 0;
    }
    inline int OUTI() { return repeatOUT(true, false); }
    inline int OUTIR() { return repeatOUT(true, true); }
    inline int OUTD() { return repeatOUT(false, false); }
    inline int OUTDR() { return repeatOUT(false, true); }

    // Decimal Adjust Accumulator
    inline int daa()
    {
        int a = reg.pair.A;
        bool c = isFlagC();
        bool ac = reg.pair.A > 0x99;
        int add = (isFlagH() || (a & 0x0F) > 9 ? 0x06 : 0x00) + (c || ac ? 0x60 : 0x00);
        a += isFlagN() ? -add : add;
        a &= 0xFF;
        setFlagS(a & 0x80);
        setFlagXY(a);
        setFlagZ(0 == a);
        setFlagH((a ^ reg.pair.A) & flagH());
        setFlagPV(isEvenNumberBits(a));
        setFlagC(c | ac);
        if (isDebug()) log("[%04X] DAA ... A: $%02X -> $%02X", reg.PC, reg.pair.A, a);
        reg.pair.A = a;
        reg.PC++;
        return 0;
    }
    static inline int DAA(Z80* ctx) { return ctx->daa(); }

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
        setFlagS(reg.pair.A & 0x80);
        setFlagXY(reg.pair.A);
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
        setFlagS(reg.pair.A & 0x80);
        setFlagXY(reg.pair.A);
        setFlagZ(reg.pair.A == 0);
        setFlagH(false);
        setFlagPV(isEvenNumberBits(reg.pair.A));
        setFlagN(false);
        reg.PC += 2;
        return consumeClock(2);
    }

    int (*opSet1[256])(Z80* ctx) = {
        NOP, LD_BC_NN, LD_BC_A, INC_RP_BC, INC_B, DEC_B, LD_B_N, RLCA, EX_AF_AF2, ADD_HL_BC, LD_A_BC, DEC_RP_BC, INC_C, DEC_C, LD_C_N, RRCA,
        DJNZ_E, LD_DE_NN, LD_DE_A, INC_RP_DE, INC_D, DEC_D, LD_D_N, RLA, JR_E, ADD_HL_DE, LD_A_DE, DEC_RP_DE, INC_E, DEC_E, LD_E_N, RRA,
        JR_NZ_E, LD_HL_NN, LD_ADDR_HL, INC_RP_HL, INC_H, DEC_H, LD_H_N, DAA, JR_Z_E, ADD_HL_HL, LD_HL_ADDR, DEC_RP_HL, INC_L, DEC_L, LD_L_N, CPL,
        JR_NC_E, LD_SP_NN, LD_NN_A, INC_RP_SP, INC_HL, DEC_HL, LD_HL_N, SCF, JR_C_E, ADD_HL_SP, LD_A_NN, DEC_RP_SP, INC_A, DEC_A, LD_A_N, CCF,
        LD_B_B, LD_B_C, LD_B_D, LD_B_E, LD_B_H, LD_B_L, LD_B_HL, LD_B_A, LD_C_B, LD_C_C, LD_C_D, LD_C_E, LD_C_H, LD_C_L, LD_C_HL, LD_C_A,
        LD_D_B, LD_D_C, LD_D_D, LD_D_E, LD_D_H, LD_D_L, LD_D_HL, LD_D_A, LD_E_B, LD_E_C, LD_E_D, LD_E_E, LD_E_H, LD_E_L, LD_E_HL, LD_E_A,
        LD_H_B, LD_H_C, LD_H_D, LD_H_E, LD_H_H, LD_H_L, LD_H_HL, LD_H_A, LD_L_B, LD_L_C, LD_L_D, LD_L_E, LD_L_H, LD_L_L, LD_L_HL, LD_L_A,
        LD_HL_B, LD_HL_C, LD_HL_D, LD_HL_E, LD_HL_H, LD_HL_L, HALT, LD_HL_A, LD_A_B, LD_A_C, LD_A_D, LD_A_E, LD_A_H, LD_A_L, LD_A_HL, LD_A_A,
        ADD_B, ADD_C, ADD_D, ADD_E, ADD_H, ADD_L, ADD_HL, ADD_A, ADC_B, ADC_C, ADC_D, ADC_E, ADC_H, ADC_L, ADC_HL, ADC_A,
        SUB_B, SUB_C, SUB_D, SUB_E, SUB_H, SUB_L, SUB_HL, SUB_A, SBC_B, SBC_C, SBC_D, SBC_E, SBC_H, SBC_L, SBC_HL, SBC_A,
        AND_B, AND_C, AND_D, AND_E, AND_H, AND_L, AND_HL, AND_A, XOR_B, XOR_C, XOR_D, XOR_E, XOR_H, XOR_L, XOR_HL, XOR_A,
        OR_B, OR_C, OR_D, OR_E, OR_H, OR_L, OR_HL, OR_A, CP_B, CP_C, CP_D, CP_E, CP_H, CP_L, CP_HL, CP_A,
        RET_C0, POP_BC, JP_C0_NN, JP_NN, CALL_C0_NN, PUSH_BC, ADD_N, RST00, RET_C1, RET, JP_C1_NN, OP_CB, CALL_C1_NN, CALL_NN, ADC_N, RST08,
        RET_C2, POP_DE, JP_C2_NN, OUT_N_A, CALL_C2_NN, PUSH_DE, SUB_N, RST10, RET_C3, EXX, JP_C3_NN, IN_A_N, CALL_C3_NN, OP_IX, SBC_N, RST18,
        RET_C4, POP_HL, JP_C4_NN, EX_SP_HL, CALL_C4_NN, PUSH_HL, AND_N, RST20, RET_C5, JP_HL, JP_C5_NN, EX_DE_HL, CALL_C5_NN, EXTRA, XOR_N, RST28,
        RET_C6, POP_AF, JP_C6_NN, DI, CALL_C6_NN, PUSH_AF, OR_N, RST30, RET_C7, LD_SP_HL, JP_C7_NN, EI, CALL_C7_NN, OP_IY, CP_N, RST38};
    int (*opSetCB[256])(Z80* ctx) = {
        RLC_B, RLC_C, RLC_D, RLC_E, RLC_H, RLC_L, RLC_HL_, RLC_A,
        RRC_B, RRC_C, RRC_D, RRC_E, RRC_H, RRC_L, RRC_HL_, RRC_A,
        RL_B, RL_C, RL_D, RL_E, RL_H, RL_L, RL_HL_, RL_A,
        RR_B, RR_C, RR_D, RR_E, RR_H, RR_L, RR_HL_, RR_A,
        SLA_B, SLA_C, SLA_D, SLA_E, SLA_H, SLA_L, SLA_HL_, SLA_A,
        SRA_B, SRA_C, SRA_D, SRA_E, SRA_H, SRA_L, SRA_HL_, SRA_A,
        SLL_B, SLL_C, SLL_D, SLL_E, SLL_H, SLL_L, SLL_HL_, SLL_A,
        SRL_B, SRL_C, SRL_D, SRL_E, SRL_H, SRL_L, SRL_HL_, SRL_A,
        BIT_B_0, BIT_C_0, BIT_D_0, BIT_E_0, BIT_H_0, BIT_L_0, BIT_HL_0, BIT_A_0,
        BIT_B_1, BIT_C_1, BIT_D_1, BIT_E_1, BIT_H_1, BIT_L_1, BIT_HL_1, BIT_A_1,
        BIT_B_2, BIT_C_2, BIT_D_2, BIT_E_2, BIT_H_2, BIT_L_2, BIT_HL_2, BIT_A_2,
        BIT_B_3, BIT_C_3, BIT_D_3, BIT_E_3, BIT_H_3, BIT_L_3, BIT_HL_3, BIT_A_3,
        BIT_B_4, BIT_C_4, BIT_D_4, BIT_E_4, BIT_H_4, BIT_L_4, BIT_HL_4, BIT_A_4,
        BIT_B_5, BIT_C_5, BIT_D_5, BIT_E_5, BIT_H_5, BIT_L_5, BIT_HL_5, BIT_A_5,
        BIT_B_6, BIT_C_6, BIT_D_6, BIT_E_6, BIT_H_6, BIT_L_6, BIT_HL_6, BIT_A_6,
        BIT_B_7, BIT_C_7, BIT_D_7, BIT_E_7, BIT_H_7, BIT_L_7, BIT_HL_7, BIT_A_7,
        RES_B_0, RES_C_0, RES_D_0, RES_E_0, RES_H_0, RES_L_0, RES_HL_0, RES_A_0,
        RES_B_1, RES_C_1, RES_D_1, RES_E_1, RES_H_1, RES_L_1, RES_HL_1, RES_A_1,
        RES_B_2, RES_C_2, RES_D_2, RES_E_2, RES_H_2, RES_L_2, RES_HL_2, RES_A_2,
        RES_B_3, RES_C_3, RES_D_3, RES_E_3, RES_H_3, RES_L_3, RES_HL_3, RES_A_3,
        RES_B_4, RES_C_4, RES_D_4, RES_E_4, RES_H_4, RES_L_4, RES_HL_4, RES_A_4,
        RES_B_5, RES_C_5, RES_D_5, RES_E_5, RES_H_5, RES_L_5, RES_HL_5, RES_A_5,
        RES_B_6, RES_C_6, RES_D_6, RES_E_6, RES_H_6, RES_L_6, RES_HL_6, RES_A_6,
        RES_B_7, RES_C_7, RES_D_7, RES_E_7, RES_H_7, RES_L_7, RES_HL_7, RES_A_7,
        SET_B_0, SET_C_0, SET_D_0, SET_E_0, SET_H_0, SET_L_0, SET_HL_0, SET_A_0,
        SET_B_1, SET_C_1, SET_D_1, SET_E_1, SET_H_1, SET_L_1, SET_HL_1, SET_A_1,
        SET_B_2, SET_C_2, SET_D_2, SET_E_2, SET_H_2, SET_L_2, SET_HL_2, SET_A_2,
        SET_B_3, SET_C_3, SET_D_3, SET_E_3, SET_H_3, SET_L_3, SET_HL_3, SET_A_3,
        SET_B_4, SET_C_4, SET_D_4, SET_E_4, SET_H_4, SET_L_4, SET_HL_4, SET_A_4,
        SET_B_5, SET_C_5, SET_D_5, SET_E_5, SET_H_5, SET_L_5, SET_HL_5, SET_A_5,
        SET_B_6, SET_C_6, SET_D_6, SET_E_6, SET_H_6, SET_L_6, SET_HL_6, SET_A_6,
        SET_B_7, SET_C_7, SET_D_7, SET_E_7, SET_H_7, SET_L_7, SET_HL_7, SET_A_7};
    int (*opSetIX[256])(Z80* ctx) = {
        NULL, NULL, NULL, NULL, INC_B_2, DEC_B_2, LD_B_N_3, NULL,
        NULL, ADD_IX_BC, NULL, NULL, INC_C_2, DEC_C_2, LD_C_N_3, NULL,
        NULL, NULL, NULL, NULL, INC_D_2, DEC_D_2, LD_D_N_3, NULL,
        NULL, ADD_IX_DE, NULL, NULL, INC_E_2, DEC_E_2, LD_E_N_3, NULL,
        NULL, LD_IX_NN_, LD_ADDR_IX_, INC_IX_reg_, INC_IXH_, DEC_IXH_, LD_IXH_N_, NULL,
        NULL, ADD_IX_IX, LD_IX_ADDR_, DEC_IX_reg_, INC_IXL_, DEC_IXL_, LD_IXL_N_, NULL,
        NULL, NULL, NULL, NULL, INC_IX_, DEC_IX_, LD_IX_N_, NULL,
        NULL, ADD_IX_SP, NULL, NULL, INC_A_2, DEC_A_2, LD_A_N_3, NULL,
        LD_B_B_2, LD_B_C_2, LD_B_D_2, LD_B_E_2, LD_B_IXH, LD_B_IXL, LD_B_IX, LD_B_A_2,
        LD_C_B_2, LD_C_C_2, LD_C_D_2, LD_C_E_2, LD_C_IXH, LD_C_IXL, LD_C_IX, LD_C_A_2,
        LD_D_B_2, LD_D_C_2, LD_D_D_2, LD_D_E_2, LD_D_IXH, LD_D_IXL, LD_D_IX, LD_D_A_2,
        LD_E_B_2, LD_E_C_2, LD_E_D_2, LD_E_E_2, LD_E_IXH, LD_E_IXL, LD_E_IX, LD_E_A_2,
        LD_IXH_B, LD_IXH_C, LD_IXH_D, LD_IXH_E, LD_IXH_IXH_, LD_IXH_IXL_, LD_H_IX, LD_IXH_A,
        LD_IXL_B, LD_IXL_C, LD_IXL_D, LD_IXL_E, LD_IXL_IXH_, LD_IXL_IXL_, LD_L_IX, LD_IXL_A,
        LD_IX_B, LD_IX_C, LD_IX_D, LD_IX_E, LD_IX_H, LD_IX_L, NULL, LD_IX_A,
        LD_A_B_2, LD_A_C_2, LD_A_D_2, LD_A_E_2, LD_A_IXH, LD_A_IXL, LD_A_IX, LD_A_A_2,
        ADD_B_2, ADD_C_2, ADD_D_2, ADD_E_2, ADD_IXH_, ADD_IXL_, ADD_IX_, ADD_A_2,
        ADC_B_2, ADC_C_2, ADC_D_2, ADC_E_2, ADC_IXH_, ADC_IXL_, ADC_IX_, ADC_A_2,
        SUB_B_2, SUB_C_2, SUB_D_2, SUB_E_2, SUB_IXH_, SUB_IXL_, SUB_IX_, SUB_A_2,
        SBC_B_2, SBC_C_2, SBC_D_2, SBC_E_2, SBC_IXH_, SBC_IXL_, SBC_IX_, SBC_A_2,
        AND_B_2, AND_C_2, AND_D_2, AND_E_2, AND_IXH_, AND_IXL_, AND_IX_, AND_A_2,
        XOR_B_2, XOR_C_2, XOR_D_2, XOR_E_2, XOR_IXH_, XOR_IXL_, XOR_IX_, XOR_A_2,
        OR_B_2, OR_C_2, OR_D_2, OR_E_2, OR_IXH_, OR_IXL_, OR_IX_, OR_A_2,
        CP_B_2, CP_C_2, CP_D_2, CP_E_2, CP_IXH_, CP_IXL_, CP_IX_, CP_A_2,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, OP_IX4, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, POP_IX_, NULL, EX_SP_IX_, NULL, PUSH_IX_, NULL, NULL,
        NULL, JP_IX_, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, LD_SP_IX_, NULL, NULL, NULL, NULL, NULL, NULL};
    int (*opSetIY[256])(Z80* ctx) = {
        NULL, NULL, NULL, NULL, INC_B_2, DEC_B_2, LD_B_N_3, NULL,
        NULL, ADD_IY_BC, NULL, NULL, INC_C_2, DEC_C_2, LD_C_N_3, NULL,
        NULL, NULL, NULL, NULL, INC_D_2, DEC_D_2, LD_D_N_3, NULL,
        NULL, ADD_IY_DE, NULL, NULL, INC_E_2, DEC_E_2, LD_E_N_3, NULL,
        NULL, LD_IY_NN_, LD_ADDR_IY_, INC_IY_reg_, INC_IYH_, DEC_IYH_, LD_IYH_N_, NULL,
        NULL, ADD_IY_IY, LD_IY_ADDR_, DEC_IY_reg_, INC_IYL_, DEC_IYL_, LD_IYL_N_, NULL,
        NULL, NULL, NULL, NULL, INC_IY_, DEC_IY_, LD_IY_N_, NULL,
        NULL, ADD_IY_SP, NULL, NULL, INC_A_2, DEC_A_2, LD_A_N_3, NULL,
        LD_B_B_2, LD_B_C_2, LD_B_D_2, LD_B_E_2, LD_B_IYH, LD_B_IYL, LD_B_IY, LD_B_A_2,
        LD_C_B_2, LD_C_C_2, LD_C_D_2, LD_C_E_2, LD_C_IYH, LD_C_IYL, LD_C_IY, LD_C_A_2,
        LD_D_B_2, LD_D_C_2, LD_D_D_2, LD_D_E_2, LD_D_IYH, LD_D_IYL, LD_D_IY, LD_D_A_2,
        LD_E_B_2, LD_E_C_2, LD_E_D_2, LD_E_E_2, LD_E_IYH, LD_E_IYL, LD_E_IY, LD_E_A_2,
        LD_IYH_B, LD_IYH_C, LD_IYH_D, LD_IYH_E, LD_IYH_IYH_, LD_IYH_IYL_, LD_H_IY, LD_IYH_A,
        LD_IYL_B, LD_IYL_C, LD_IYL_D, LD_IYL_E, LD_IYL_IYH_, LD_IYL_IYL_, LD_L_IY, LD_IYL_A,
        LD_IY_B, LD_IY_C, LD_IY_D, LD_IY_E, LD_IY_H, LD_IY_L, NULL, LD_IY_A,
        LD_A_B_2, LD_A_C_2, LD_A_D_2, LD_A_E_2, LD_A_IYH, LD_A_IYL, LD_A_IY, LD_A_A_2,
        ADD_B_2, ADD_C_2, ADD_D_2, ADD_E_2, ADD_IYH_, ADD_IYL_, ADD_IY_, ADD_A_2,
        ADC_B_2, ADC_C_2, ADC_D_2, ADC_E_2, ADC_IYH_, ADC_IYL_, ADC_IY_, ADC_A_2,
        SUB_B_2, SUB_C_2, SUB_D_2, SUB_E_2, SUB_IYH_, SUB_IYL_, SUB_IY_, SUB_A_2,
        SBC_B_2, SBC_C_2, SBC_D_2, SBC_E_2, SBC_IYH_, SBC_IYL_, SBC_IY_, SBC_A_2,
        AND_B_2, AND_C_2, AND_D_2, AND_E_2, AND_IYH_, AND_IYL_, AND_IY_, AND_A_2,
        XOR_B_2, XOR_C_2, XOR_D_2, XOR_E_2, XOR_IYH_, XOR_IYL_, XOR_IY_, XOR_A_2,
        OR_B_2, OR_C_2, OR_D_2, OR_E_2, OR_IYH_, OR_IYL_, OR_IY_, OR_A_2,
        CP_B_2, CP_C_2, CP_D_2, CP_E_2, CP_IYH_, CP_IYL_, CP_IY_, CP_A_2,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, OP_IY4, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, POP_IY_, NULL, EX_SP_IY_, NULL, PUSH_IY_, NULL, NULL,
        NULL, JP_IY_, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, LD_SP_IY_, NULL, NULL, NULL, NULL, NULL, NULL};
    int (*opSetIX4[256])(Z80* ctx, signed char d) = {
        RLC_IX_with_LD_B, RLC_IX_with_LD_C, RLC_IX_with_LD_D, RLC_IX_with_LD_E, RLC_IX_with_LD_H, RLC_IX_with_LD_L, RLC_IX_, RLC_IX_with_LD_A,
        RRC_IX_with_LD_B, RRC_IX_with_LD_C, RRC_IX_with_LD_D, RRC_IX_with_LD_E, RRC_IX_with_LD_H, RRC_IX_with_LD_L, RRC_IX_, RRC_IX_with_LD_A,
        RL_IX_with_LD_B, RL_IX_with_LD_C, RL_IX_with_LD_D, RL_IX_with_LD_E, RL_IX_with_LD_H, RL_IX_with_LD_L, RL_IX_, RL_IX_with_LD_A,
        RR_IX_with_LD_B, RR_IX_with_LD_C, RR_IX_with_LD_D, RR_IX_with_LD_E, RR_IX_with_LD_H, RR_IX_with_LD_L, RR_IX_, RR_IX_with_LD_A,
        SLA_IX_with_LD_B, SLA_IX_with_LD_C, SLA_IX_with_LD_D, SLA_IX_with_LD_E, SLA_IX_with_LD_H, SLA_IX_with_LD_L, SLA_IX_, SLA_IX_with_LD_A,
        SRA_IX_with_LD_B, SRA_IX_with_LD_C, SRA_IX_with_LD_D, SRA_IX_with_LD_E, SRA_IX_with_LD_H, SRA_IX_with_LD_L, SRA_IX_, SRA_IX_with_LD_A,
        SLL_IX_with_LD_B, SLL_IX_with_LD_C, SLL_IX_with_LD_D, SLL_IX_with_LD_E, SLL_IX_with_LD_H, SLL_IX_with_LD_L, SLL_IX_, SLL_IX_with_LD_A,
        SRL_IX_with_LD_B, SRL_IX_with_LD_C, SRL_IX_with_LD_D, SRL_IX_with_LD_E, SRL_IX_with_LD_H, SRL_IX_with_LD_L, SRL_IX_, SRL_IX_with_LD_A,
        BIT_IX_0, BIT_IX_0, BIT_IX_0, BIT_IX_0, BIT_IX_0, BIT_IX_0, BIT_IX_0, BIT_IX_0,
        BIT_IX_1, BIT_IX_1, BIT_IX_1, BIT_IX_1, BIT_IX_1, BIT_IX_1, BIT_IX_1, BIT_IX_1,
        BIT_IX_2, BIT_IX_2, BIT_IX_2, BIT_IX_2, BIT_IX_2, BIT_IX_2, BIT_IX_2, BIT_IX_2,
        BIT_IX_3, BIT_IX_3, BIT_IX_3, BIT_IX_3, BIT_IX_3, BIT_IX_3, BIT_IX_3, BIT_IX_3,
        BIT_IX_4, BIT_IX_4, BIT_IX_4, BIT_IX_4, BIT_IX_4, BIT_IX_4, BIT_IX_4, BIT_IX_4,
        BIT_IX_5, BIT_IX_5, BIT_IX_5, BIT_IX_5, BIT_IX_5, BIT_IX_5, BIT_IX_5, BIT_IX_5,
        BIT_IX_6, BIT_IX_6, BIT_IX_6, BIT_IX_6, BIT_IX_6, BIT_IX_6, BIT_IX_6, BIT_IX_6,
        BIT_IX_7, BIT_IX_7, BIT_IX_7, BIT_IX_7, BIT_IX_7, BIT_IX_7, BIT_IX_7, BIT_IX_7,
        RES_IX_0_with_LD_B, RES_IX_0_with_LD_C, RES_IX_0_with_LD_D, RES_IX_0_with_LD_E, RES_IX_0_with_LD_H, RES_IX_0_with_LD_L, RES_IX_0, RES_IX_0_with_LD_A,
        RES_IX_1_with_LD_B, RES_IX_1_with_LD_C, RES_IX_1_with_LD_D, RES_IX_1_with_LD_E, RES_IX_1_with_LD_H, RES_IX_1_with_LD_L, RES_IX_1, RES_IX_1_with_LD_A,
        RES_IX_2_with_LD_B, RES_IX_2_with_LD_C, RES_IX_2_with_LD_D, RES_IX_2_with_LD_E, RES_IX_2_with_LD_H, RES_IX_2_with_LD_L, RES_IX_2, RES_IX_2_with_LD_A,
        RES_IX_3_with_LD_B, RES_IX_3_with_LD_C, RES_IX_3_with_LD_D, RES_IX_3_with_LD_E, RES_IX_3_with_LD_H, RES_IX_3_with_LD_L, RES_IX_3, RES_IX_3_with_LD_A,
        RES_IX_4_with_LD_B, RES_IX_4_with_LD_C, RES_IX_4_with_LD_D, RES_IX_4_with_LD_E, RES_IX_4_with_LD_H, RES_IX_4_with_LD_L, RES_IX_4, RES_IX_4_with_LD_A,
        RES_IX_5_with_LD_B, RES_IX_5_with_LD_C, RES_IX_5_with_LD_D, RES_IX_5_with_LD_E, RES_IX_5_with_LD_H, RES_IX_5_with_LD_L, RES_IX_5, RES_IX_5_with_LD_A,
        RES_IX_6_with_LD_B, RES_IX_6_with_LD_C, RES_IX_6_with_LD_D, RES_IX_6_with_LD_E, RES_IX_6_with_LD_H, RES_IX_6_with_LD_L, RES_IX_6, RES_IX_6_with_LD_A,
        RES_IX_7_with_LD_B, RES_IX_7_with_LD_C, RES_IX_7_with_LD_D, RES_IX_7_with_LD_E, RES_IX_7_with_LD_H, RES_IX_7_with_LD_L, RES_IX_7, RES_IX_7_with_LD_A,
        SET_IX_0_with_LD_B, SET_IX_0_with_LD_C, SET_IX_0_with_LD_D, SET_IX_0_with_LD_E, SET_IX_0_with_LD_H, SET_IX_0_with_LD_L, SET_IX_0, SET_IX_0_with_LD_A,
        SET_IX_1_with_LD_B, SET_IX_1_with_LD_C, SET_IX_1_with_LD_D, SET_IX_1_with_LD_E, SET_IX_1_with_LD_H, SET_IX_1_with_LD_L, SET_IX_1, SET_IX_1_with_LD_A,
        SET_IX_2_with_LD_B, SET_IX_2_with_LD_C, SET_IX_2_with_LD_D, SET_IX_2_with_LD_E, SET_IX_2_with_LD_H, SET_IX_2_with_LD_L, SET_IX_2, SET_IX_2_with_LD_A,
        SET_IX_3_with_LD_B, SET_IX_3_with_LD_C, SET_IX_3_with_LD_D, SET_IX_3_with_LD_E, SET_IX_3_with_LD_H, SET_IX_3_with_LD_L, SET_IX_3, SET_IX_3_with_LD_A,
        SET_IX_4_with_LD_B, SET_IX_4_with_LD_C, SET_IX_4_with_LD_D, SET_IX_4_with_LD_E, SET_IX_4_with_LD_H, SET_IX_4_with_LD_L, SET_IX_4, SET_IX_4_with_LD_A,
        SET_IX_5_with_LD_B, SET_IX_5_with_LD_C, SET_IX_5_with_LD_D, SET_IX_5_with_LD_E, SET_IX_5_with_LD_H, SET_IX_5_with_LD_L, SET_IX_5, SET_IX_5_with_LD_A,
        SET_IX_6_with_LD_B, SET_IX_6_with_LD_C, SET_IX_6_with_LD_D, SET_IX_6_with_LD_E, SET_IX_6_with_LD_H, SET_IX_6_with_LD_L, SET_IX_6, SET_IX_6_with_LD_A,
        SET_IX_7_with_LD_B, SET_IX_7_with_LD_C, SET_IX_7_with_LD_D, SET_IX_7_with_LD_E, SET_IX_7_with_LD_H, SET_IX_7_with_LD_L, SET_IX_7, SET_IX_7_with_LD_A};
    int (*opSetIY4[256])(Z80* ctx, signed char d) = {
        RLC_IY_with_LD_B, RLC_IY_with_LD_C, RLC_IY_with_LD_D, RLC_IY_with_LD_E, RLC_IY_with_LD_H, RLC_IY_with_LD_L, RLC_IY_, RLC_IY_with_LD_A,
        RRC_IY_with_LD_B, RRC_IY_with_LD_C, RRC_IY_with_LD_D, RRC_IY_with_LD_E, RRC_IY_with_LD_H, RRC_IY_with_LD_L, RRC_IY_, RRC_IY_with_LD_A,
        RL_IY_with_LD_B, RL_IY_with_LD_C, RL_IY_with_LD_D, RL_IY_with_LD_E, RL_IY_with_LD_H, RL_IY_with_LD_L, RL_IY_, RL_IY_with_LD_A,
        RR_IY_with_LD_B, RR_IY_with_LD_C, RR_IY_with_LD_D, RR_IY_with_LD_E, RR_IY_with_LD_H, RR_IY_with_LD_L, RR_IY_, RR_IY_with_LD_A,
        SLA_IY_with_LD_B, SLA_IY_with_LD_C, SLA_IY_with_LD_D, SLA_IY_with_LD_E, SLA_IY_with_LD_H, SLA_IY_with_LD_L, SLA_IY_, SLA_IY_with_LD_A,
        SRA_IY_with_LD_B, SRA_IY_with_LD_C, SRA_IY_with_LD_D, SRA_IY_with_LD_E, SRA_IY_with_LD_H, SRA_IY_with_LD_L, SRA_IY_, SRA_IY_with_LD_A,
        SLL_IY_with_LD_B, SLL_IY_with_LD_C, SLL_IY_with_LD_D, SLL_IY_with_LD_E, SLL_IY_with_LD_H, SLL_IY_with_LD_L, SLL_IY_, SLL_IY_with_LD_A,
        SRL_IY_with_LD_B, SRL_IY_with_LD_C, SRL_IY_with_LD_D, SRL_IY_with_LD_E, SRL_IY_with_LD_H, SRL_IY_with_LD_L, SRL_IY_, SRL_IY_with_LD_A,
        BIT_IY_0, BIT_IY_0, BIT_IY_0, BIT_IY_0, BIT_IY_0, BIT_IY_0, BIT_IY_0, BIT_IY_0,
        BIT_IY_1, BIT_IY_1, BIT_IY_1, BIT_IY_1, BIT_IY_1, BIT_IY_1, BIT_IY_1, BIT_IY_1,
        BIT_IY_2, BIT_IY_2, BIT_IY_2, BIT_IY_2, BIT_IY_2, BIT_IY_2, BIT_IY_2, BIT_IY_2,
        BIT_IY_3, BIT_IY_3, BIT_IY_3, BIT_IY_3, BIT_IY_3, BIT_IY_3, BIT_IY_3, BIT_IY_3,
        BIT_IY_4, BIT_IY_4, BIT_IY_4, BIT_IY_4, BIT_IY_4, BIT_IY_4, BIT_IY_4, BIT_IY_4,
        BIT_IY_5, BIT_IY_5, BIT_IY_5, BIT_IY_5, BIT_IY_5, BIT_IY_5, BIT_IY_5, BIT_IY_5,
        BIT_IY_6, BIT_IY_6, BIT_IY_6, BIT_IY_6, BIT_IY_6, BIT_IY_6, BIT_IY_6, BIT_IY_6,
        BIT_IY_7, BIT_IY_7, BIT_IY_7, BIT_IY_7, BIT_IY_7, BIT_IY_7, BIT_IY_7, BIT_IY_7,
        RES_IY_0_with_LD_B, RES_IY_0_with_LD_C, RES_IY_0_with_LD_D, RES_IY_0_with_LD_E, RES_IY_0_with_LD_H, RES_IY_0_with_LD_L, RES_IY_0, RES_IY_0_with_LD_A,
        RES_IY_1_with_LD_B, RES_IY_1_with_LD_C, RES_IY_1_with_LD_D, RES_IY_1_with_LD_E, RES_IY_1_with_LD_H, RES_IY_1_with_LD_L, RES_IY_1, RES_IY_1_with_LD_A,
        RES_IY_2_with_LD_B, RES_IY_2_with_LD_C, RES_IY_2_with_LD_D, RES_IY_2_with_LD_E, RES_IY_2_with_LD_H, RES_IY_2_with_LD_L, RES_IY_2, RES_IY_2_with_LD_A,
        RES_IY_3_with_LD_B, RES_IY_3_with_LD_C, RES_IY_3_with_LD_D, RES_IY_3_with_LD_E, RES_IY_3_with_LD_H, RES_IY_3_with_LD_L, RES_IY_3, RES_IY_3_with_LD_A,
        RES_IY_4_with_LD_B, RES_IY_4_with_LD_C, RES_IY_4_with_LD_D, RES_IY_4_with_LD_E, RES_IY_4_with_LD_H, RES_IY_4_with_LD_L, RES_IY_4, RES_IY_4_with_LD_A,
        RES_IY_5_with_LD_B, RES_IY_5_with_LD_C, RES_IY_5_with_LD_D, RES_IY_5_with_LD_E, RES_IY_5_with_LD_H, RES_IY_5_with_LD_L, RES_IY_5, RES_IY_5_with_LD_A,
        RES_IY_6_with_LD_B, RES_IY_6_with_LD_C, RES_IY_6_with_LD_D, RES_IY_6_with_LD_E, RES_IY_6_with_LD_H, RES_IY_6_with_LD_L, RES_IY_6, RES_IY_6_with_LD_A,
        RES_IY_7_with_LD_B, RES_IY_7_with_LD_C, RES_IY_7_with_LD_D, RES_IY_7_with_LD_E, RES_IY_7_with_LD_H, RES_IY_7_with_LD_L, RES_IY_7, RES_IY_7_with_LD_A,
        SET_IY_0_with_LD_B, SET_IY_0_with_LD_C, SET_IY_0_with_LD_D, SET_IY_0_with_LD_E, SET_IY_0_with_LD_H, SET_IY_0_with_LD_L, SET_IY_0, SET_IY_0_with_LD_A,
        SET_IY_1_with_LD_B, SET_IY_1_with_LD_C, SET_IY_1_with_LD_D, SET_IY_1_with_LD_E, SET_IY_1_with_LD_H, SET_IY_1_with_LD_L, SET_IY_1, SET_IY_1_with_LD_A,
        SET_IY_2_with_LD_B, SET_IY_2_with_LD_C, SET_IY_2_with_LD_D, SET_IY_2_with_LD_E, SET_IY_2_with_LD_H, SET_IY_2_with_LD_L, SET_IY_2, SET_IY_2_with_LD_A,
        SET_IY_3_with_LD_B, SET_IY_3_with_LD_C, SET_IY_3_with_LD_D, SET_IY_3_with_LD_E, SET_IY_3_with_LD_H, SET_IY_3_with_LD_L, SET_IY_3, SET_IY_3_with_LD_A,
        SET_IY_4_with_LD_B, SET_IY_4_with_LD_C, SET_IY_4_with_LD_D, SET_IY_4_with_LD_E, SET_IY_4_with_LD_H, SET_IY_4_with_LD_L, SET_IY_4, SET_IY_4_with_LD_A,
        SET_IY_5_with_LD_B, SET_IY_5_with_LD_C, SET_IY_5_with_LD_D, SET_IY_5_with_LD_E, SET_IY_5_with_LD_H, SET_IY_5_with_LD_L, SET_IY_5, SET_IY_5_with_LD_A,
        SET_IY_6_with_LD_B, SET_IY_6_with_LD_C, SET_IY_6_with_LD_D, SET_IY_6_with_LD_E, SET_IY_6_with_LD_H, SET_IY_6_with_LD_L, SET_IY_6, SET_IY_6_with_LD_A,
        SET_IY_7_with_LD_B, SET_IY_7_with_LD_C, SET_IY_7_with_LD_D, SET_IY_7_with_LD_E, SET_IY_7_with_LD_H, SET_IY_7_with_LD_L, SET_IY_7, SET_IY_7_with_LD_A};

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
            reg.R = ((reg.R + 1) & 0x7F) | (reg.R & 0x80);
            reg.IFF |= IFF_NMI();
            reg.IFF &= ~IFF1();
            unsigned char pcL = reg.PC & 0x00FF;
            unsigned char pcH = (reg.PC & 0xFF00) >> 8;
            writeByte(reg.SP - 1, pcH);
            writeByte(reg.SP - 2, pcL);
            reg.SP -= 2;
            reg.PC = reg.interruptAddrN;
            consumeClock(11);
            invokeCallHandlers();
        } else if (reg.interrupt & 0b01000000) {
            // execute IRQ
            if (!(reg.IFF & IFF1())) {
                return;
            }
            reg.interrupt &= 0b10111111;
            reg.IFF &= ~IFF_HALT();
            reg.IFF |= IFF_IRQ();
            reg.IFF &= ~(IFF1() | IFF2());
            reg.R = ((reg.R + 1) & 0x7F) | (reg.R & 0x80);
            switch (reg.interrupt & 0b00000011) {
                case 0: // mode 0
                    if (isDebug()) log("EXECUTE INT MODE1 (RST TO $%04X)", reg.interruptVector * 8);
                    if (reg.interruptVector == 0xCD) {
                        consumeClock(7);
                    }
                    RST(reg.interruptVector, false);
                    break;
                case 1: // mode 1 (13Hz)
                    if (isDebug()) log("EXECUTE INT MODE1 (RST TO $0038)");
                    consumeClock(1);
                    RST(7, false);
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
                    pc += ((unsigned short)readByte(addr + 1)) << 8;
                    if (isDebug()) log("EXECUTE INT MODE2: ($%04X) = $%04X", addr, pc);
                    reg.PC = pc;
                    consumeClock(3);
                    invokeCallHandlers();
                    break;
                }
            }
        }
    }

    inline void updateRefreshRegister()
    {
        reg.R = ((reg.R + 1) & 0x7F) | (reg.R & 0x80);
        consumeClock(2);
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
        reg.pair.A = 0xff;
        reg.pair.F = 0xff;
        reg.SP = 0xffff;
        memset(&wtc, 0, sizeof(wtc));
    }

    ~Z80()
    {
        removeAllBreakOperands();
        removeAllBreakPoints();
        removeAllCallHandlers();
        removeAllReturnHandlers();
    }

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

    void addReturnHandler(void (*callback)(void*))
    {
        CB.returnHandlers.push_back(new ReturnHandler(callback));
    }

    void removeReturnHandler(void (*callback)(void*))
    {
        int index = 0;
        for (auto handler : CB.returnHandlers) {
            if (handler->callback == callback) {
                CB.returnHandlers.erase(CB.returnHandlers.begin() + index);
                delete handler;
                return;
            }
            index++;
        }
    }

    void removeAllReturnHandlers()
    {
        for (auto handler : CB.returnHandlers) delete handler;
        CB.returnHandlers.clear();
    }

    void addCallHandler(void (*callback)(void*))
    {
        CB.callHandlers.push_back(new CallHandler(callback));
    }

    void removeCallHandler(void (*callback)(void*))
    {
        int index = 0;
        for (auto handler : CB.callHandlers) {
            if (handler->callback == callback) {
                CB.callHandlers.erase(CB.callHandlers.begin() + index);
                delete handler;
                return;
            }
            index++;
        }
    }

    void removeAllCallHandlers()
    {
        for (auto handler : CB.callHandlers) delete handler;
        CB.callHandlers.clear();
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

    void cancelIRQ()
    {
        reg.interrupt &= 0b10111111;
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
                if (wtc.fretch) consumeClock(wtc.fretch);
                checkBreakPoint();
                reg.execEI = 0;
                int operandNumber = readByte(reg.PC, 2);
                updateRefreshRegister();
                checkBreakOperand(operandNumber);
                if (opSet1[operandNumber](this) < 0) {
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
