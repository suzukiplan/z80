#ifndef INCLUDE_Z80_HPP
#define INCLUDE_Z80_HPP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    };

  private: // Internal variables
    std::function<unsigned char(unsigned short addr)> read;
    std::function<void(unsigned short addr, unsigned char value)> write;
    std::function<unsigned char(unsigned char port)> in;
    std::function<void(unsigned char port, unsigned char value)> out;
    struct Register reg;

  public: // Constructor and Destructor
    Z80(std::function<unsigned char(unsigned short addr)> read,
        std::function<void(unsigned short addr, unsigned char value)> write,
        std::function<unsigned char(unsigned char port)> in,
        std::function<void(unsigned char port, unsigned char value)> out)
    {
        this->read = read;
        this->write = write;
        this->in = in;
        this->out = out;
        ::memset(&reg, 0, sizeof(reg));
    }

    ~Z80() {}

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

    inline int nop()
    {
        reg.PC++;
        return 4;
    }

  public: // API functions
    void execute(int clock)
    {
        while (0 < clock) {
            unsigned char op = read(reg.PC);
            switch (op) {
                case 0b00000000: clock -= nop(); break;
                default: puts("TODO: unimplemented"); exit(-1);
            }
        }
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