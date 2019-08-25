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
        bool isHalt;
    };

  private: // Internal variables
    std::function<unsigned char(unsigned short addr)> read;
    std::function<void(unsigned short addr, unsigned char value)> write;
    std::function<unsigned char(unsigned char port)> in;
    std::function<void(unsigned char port, unsigned char value)> out;
    struct Register reg;
    bool isDebug;

  public: // utility functions
    inline void log(const char* format, ...)
    {
        if (!isDebug) return;
        char buf[1024];
        va_list args;
        va_start(args, format);
        vsprintf(buf, format, args);
        va_end(args);
        time_t t1 = time(NULL);
        struct tm* t2 = localtime(&t1);
        fprintf(stderr, "%04d.%02d.%02d %02d:%02d:%02d %s\n", t2->tm_year + 1900, t2->tm_mon, t2->tm_mday, t2->tm_hour, t2->tm_min, t2->tm_sec, buf);
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

    static inline int nop(Z80* ctx)
    {
        ctx->log("[%04X] NOP", ctx->reg.PC);
        ctx->reg.PC++;
        return 4;
    }

    static inline int halt(Z80* ctx)
    {
        ctx->log("[%04X] HALT", ctx->reg.PC);
        ctx->reg.isHalt = true;
        ctx->reg.PC++;
        return 4;
    }

    int (*opSet1[256])(Z80* ctx);

  public: // API functions
    Z80(std::function<unsigned char(unsigned short addr)> read,
        std::function<void(unsigned short addr,
                           unsigned char value)> write,
        std::function<unsigned char(unsigned char port)> in,
        std::function<void(unsigned char port, unsigned char value)> out,
        bool isDebug = false)
    {
        this->read = read;
        this->write = write;
        this->in = in;
        this->out = out;
        this->isDebug = isDebug;
        ::memset(&reg, 0, sizeof(reg));
        ::memset(&opSet1, 0, sizeof(opSet1));
        opSet1[0b00000000] = nop;
        opSet1[0b01110110] = halt;
    }

    ~Z80() {}

    bool execute(int clock)
    {
        while (0 < clock && !reg.isHalt) {
            int operandNumber = read(reg.PC);
            int (*op)(Z80*) = opSet1[operandNumber];
            if (!op) {
                log("detected an unimplement operand: $%02X", operandNumber);
                return false;
            }
            clock -= op(this);
        }
        return true;
    }

    void executeTick4MHz()
    {
        execute(4194304 / 60);
    }

    void executeTick8MHz()
    {
        execute(8388608 / 60);
    }
};

#endif // INCLUDE_Z80_HPP