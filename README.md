# SUZUKI PLAN - Z80 Emulator (WIP)

The Z80 was an 8-bit CPU developed by Zilog corporation, released in 1976, and widely used in computers and game consoles in the 1980s.
It is not just a relic of the past, but continues to be used in embedded systems that require accuracy in processing execution time, such as real-time systems.

__SUZUKI PLAN - Z80 Emulator__ is an emulator under development based on the following design guidelines to support the development of programs and/or emulators using Z80:

__(FOUR EASY GUIDELINES FOR EASILY)__

1. Make emulator implementation `EASY` & simple (Realized by providing single header: [z80.hpp](z80.hpp))
2. `EASILY` debugging the Z80 programs (Realized by having dynamic disassemble feature)
3. Highly readable and `EASILY` customizable (priority for readability over performance)
4. Provide under the license that `EASY` to adopt in various programs ([MIT](LICENSE.txt))

> Since I do not have deep knowledge about Z80 myself, I'm implementing it with reference to the information on the following site.
> 
> - [8ビット CPU Z80](http://www.yamamo10.jp/yamamoto/comp/Z80/index.php) of [山本研究所](http://www.yamamo10.jp/yamamoto/index.html)
> - [Z80 Code Refference](http://mydocuments.g2.xrea.com/html/p6/z80ref.html) of [Bookworm's Library](http://mydocuments.g2.xrea.com/index.html)

## WIP status

### 1. Implementation

- [x] 3.1 8bit load instructions
- [x] 3.2 16bit load instructions
- [x] 3.3 block load instructions
- [x] 3.4 exchange instructions
- [x] 4. stack instructions
- [x] 5.1 left rotate instructions
- [x] 5.2 right rotate instructions
- [x] 5.3 left shift instructions
- [x] 5.4 right shift instructions
- [ ] __6.1 8bit arithmetic instructions__ <sup>*inprogress*</sup>
- [ ] 6.2 16bit arithmetic instructions
- [ ] 7.1 logical operation instructions
- [ ] 7.2 bit instructions
- [ ] 8.1 search
- [ ] 8.2 compare
- [ ] 9.0 branch
- [ ] 9.1 jump
- [ ] 9.2 sub routin
- [x] 10.1 interrupt
- [ ] 10.2 input
- [ ] 10.3 output
- [ ] 11. binary decimal

> Reference: [http://www.yamamo10.jp/yamamoto/comp/Z80/instructions/index.php](http://www.yamamo10.jp/yamamoto/comp/Z80/instructions/index.php)

### 2. Testing

_Considering good system test method..._

## How to test

### Prerequests

You can test on the any 32bit/64bit platform/OS (UNIX, Linux, macOS...etc) but needs following middlewares:

- Git
- GNU Make
- clang
- clang-format

### Build (UNIX, Linux, macOS)

```
git clone https://github.com/suzukiplan/z80.git
cd z80
make
```

### Result example

```
> 64
2019.07.27 00:21:10 [0000] LD B<$34>, A<$12>
2019.07.27 00:21:10 [0001] LD C<$00>, $56
2019.07.27 00:21:10 [0003] LD D<$00>, (HL<$0001>) = $0E
2019.07.27 00:21:10 [0004] LD E<$00>, (IX<$0000>+$04) = $DD
2019.07.27 00:21:10 [0007] LD H<$00>, (IY<$0001>+$04) = $5E
2019.07.27 00:21:10 [000A] LD (HL<$5E01>), B<$12>
2019.07.27 00:21:10 [000B] NOP
> m 5E00
2019.07.27 00:21:19 [5E00] 00 12 00 00 - 00 00 00 00
> r
2019.07.27 00:21:22 ===== REGISTER DUMP : START =====
2019.07.27 00:21:22 A<$12> B<$12> C<$56> D<$0E> E<$DD> H<$5E> L<$01>
2019.07.27 00:21:22 PC<$000C> SP<$0000> IX<$0000> IY<$0001>
2019.07.27 00:21:22 ===== REGISTER DUMP : END =====
> 
executed 67Hz
```

## Minimum usage

### 1. Include

```c++
#include "z80.hpp"
```

### 2. Implement MMU & access callback

```c++
class MMU
{
  public:
    unsigned char RAM[0x10000]; // 64KB memory (minimum)
    unsigned char IO[0x100]; // 256bytes port

    MMU()
    {
        memset(&RAM, 0, sizeof(RAM));
        memset(&IO, 0, sizeof(IO));
    }
};

// memory read request per 1 byte from CPU
unsigned char readByte(void* arg, unsigned short addr)
{
    // NOTE: implement switching procedure here if your MMU has bank switch feature
    return ((MMU*)arg)->RAM[addr];
}

// memory write request per 1 byte from CPU
void writeByte(void* arg, unsigned short addr, unsigned char value)
{
    // NOTE: implement switching procedure here if your MMU has bank switch feature
    ((MMU*)arg)->RAM[addr] = value;
}

// IN operand request from CPU
unsigned char inPort(void* arg, unsigned char port)
{
    return ((MMU*)arg)->IO[port];
}

// OUT operand request from CPU
void outPort(void* arg, unsigned char port, unsigned char value)
{
    ((MMU*)arg)->IO[port] = value;
}
```

### 3. Make Z80 instance

```c++
    MMU mmu;
    /**
     * readByte: callback of memory read request
     * writeByte: callback of memory write request
     * inPort: callback of input request
     * outPort: callback of output request
     * &mmu: 1st argument of the callbacks
     */
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu);
```

### 4. Execute

```c++
    // when executing about 1234Hz
    int actualExecuteClocks = z80.execute(1234);
```

## Optional features

### Dynamic disassemble (for debug)

You can acquire the debug messages with `setDebugMessage`.
Debug message contains dynamic disassembly results step by step.

```c++
    z80.setDebugMessage([](void* arg, const char* message) -> void {
        time_t t1 = time(NULL);
        struct tm* t2 = localtime(&t1);
        printf("%04d.%02d.%02d %02d:%02d:%02d %s\n",
               t2->tm_year + 1900, t2->tm_mon, t2->tm_mday, t2->tm_hour,
               t2->tm_min, t2->tm_sec, message);
    });
```

### Use break point

If you want to execute processing just before executing an instruction of specific program counter value _(in this ex: $008E)_, you can set a breakpoint as follows:

```c++
    z80.setBreakPoint(0x008E, [](void* arg) -> void {
        printf("Detect break point! (PUSH ENTER TO CONTINUE)");
        char buf[80];
        fgets(buf, sizeof(buf), stdin);
    });
```

### Detect clock consuming

If you want to implement stricter synchronization, you can capture the CPU clock consumption timing as follows:

```c++
    z80.setConsumeClockCallback([](void* arg, int clock) -> void {
        printf("consumed: %dHz\n", clock);
    });
```

### If implement quick save/load

Save the member variable `reg` when quick saving:

```c++
    fwrite(&z80.reg, sizeof(z80.reg), 1, fp);
```

Extract to the member variable `reg` when quick loading:

```c++
    fread(&z80.reg, sizeof(z80.reg), 1, fp);
```

## License

[MIT](LICENSE.txt)
