# SUZUKI PLAN - Z80 Emulator 
[![suzukiplan](https://circleci.com/gh/suzukiplan/z80.svg?style=svg)](https://app.circleci.com/pipelines/github/suzukiplan/z80)

The Z80 is an 8-bit CPU developed by Zilog corporation, released in 1976, and widely used in computers and game consoles in the 1980s.
It is not just a relic of the past, but continues to be used in embedded systems that require accuracy in processing execution time, such as real-time systems.

**SUZUKI PLAN - Z80 Emulator** is an emulator under development based on the following design guidelines to support the development of programs and/or emulators using Z80, 8080:

**(FOUR EASY GUIDELINES FOR EASILY)**

1. Make emulator implementation `EASY` & simple (Realized by providing single header: [z80.hpp](z80.hpp))
2. `EASILY` debugging the Z80 programs (Realized by having [dynamic disassemble feature](#dynamic-disassemble-for-debug))
3. Highly readable and `EASILY` customizable (priority for readability over performance)
4. Provide under the license that `EASY` to adopt in various programs ([MIT](LICENSE.txt))

> Since I do not have deep knowledge about Z80 myself, I'm implementing it with reference to the information on the following web sites:
>
> - [Z80 CPU User Manual - Zilog](https://www.zilog.com/manage_directlink.php?filepath=docs/z80/um0080&extn=.pdf)
> - [8 ビット CPU Z80](http://www.yamamo10.jp/yamamoto/comp/Z80/index.php) of [山本研究所](http://www.yamamo10.jp/yamamoto/index.html)
> - [Z80 Code Refference](http://mydocuments.g2.xrea.com/html/p6/z80ref.html) of [Bookworm's Library](http://mydocuments.g2.xrea.com/index.html)
> - [Zilog Z80 DAA Result Table](http://ver0.sakura.ne.jp/doc/daa.html) of [Version 0](http://ver0.sakura.ne.jp/)

[z80.hpp](z80.hpp) passes all `zexdoc` and `zexall` tests. ([See the detail](test-ex))

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
unsigned char inPort(void* arg, unsigned short port)
{
    return ((MMU*)arg)->IO[port];
}

// OUT operand request from CPU
void outPort(void* arg, unsigned short port, unsigned char value)
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

#### 3-1. Cases when want to use 16bit port 

Note that by default, only the lower 8 bits of the port number can be obtained in the callback argument, and the upper 8 bits must be referenced from register B.

If you want to get it in 16 bits from the beginning, please initialize with `returnPortAs16Bits` (6th argument) to `true` as follows:

```c++
    Z80 z80(readByte, writeByte, inPort, outPort, &mmu, true);
```

#### 3-2. Cases when performance-sensitive

Normally, `std::function` is used for callbacks, but in more performance-sensitive cases, a function pointer can be used.

```c++
    Z80 z80(&mmu);
    z80.setupCallbackFP(readByte, writeByte, inPort, outPort);
```

However, using function pointers causes inconveniences such as the inability to specify a capture in a lambda expression.
In most cases, optimization with the `-O2` option will not cause performance problems, but in case environments where more severe performance is required, `setupCallbackFP` is recommended.

> The following article (in Japanese) provides a performance comparison between function pointers and `std::function`:
>
> https://qiita.com/suzukiplan/items/e459bf47f6c659acc74d
>
> The above article gives an example of the time it took to execute 100 million times call of function-pointer and three patterns of `std::function` calls (`bind`, `direct`, `lambda`)  as following:
>
> |Optimization Option|function pointer|`bind`|`direct`|`lambda`|
> |:-:|-:|-:|-:|-:|
> |none|195,589μs|5,574,856μs|2,570,016μs|2,417,802μs|
> |-O|184,692μs|2,293,151μs|827,113μs|580,442μs|
> |-O2|154,206μs|197,683μs|209,626μs|167,703μs|
> |-Ofast|154,332μs|250,490μs|255,401μs|164,773μs|

### 4. Execute

```c++
    // when executing about 1234Hz
    int actualExecutedClocks = z80.execute(1234);
```

#### 4-1. Actual executed clocks

- The `execute` method repeats the execution of an instruction until the total number of clocks from the time of the call is greater than or equal to the value specified in the "clocks" argument.
- If a value less than or equal to 0 is specified, no instruction is executed at all.
- If you want single operand execution, you can specify 1.

#### 4-2. Interruption of execution

Execution of the `requestBreak` method can abort the `execute` at an arbitrary time.

> A typical 8-bit game console emulator implementation that I envision:
>
> - Implement synchronization with Video Display Processor (VDP) and other devices (sound modules, etc.) in `consumeClock` callback.
> - Call `requestBreak` when V-SYNC signal is received from VDP.
> - Call `execute` with a large value such as `INT_MAX`.

#### 4-3. Example

Code: [test/test-execute.cpp](test/test-execute.cpp)

```c++
#include "z80.hpp"

int main()
{
    unsigned char rom[256] = {
        0x01, 0x34, 0x12, // LD BC, $1234
        0x3E, 0x01,       // LD A, $01
        0xED, 0x79,       // OUT (C), A
        0xED, 0x78,       // IN A, (C)
        0xc3, 0x09, 0x00, // JMP $0009
    };
    Z80 z80([=](void* arg, unsigned short addr) { return rom[addr & 0xFF]; },
            [](void* arg, unsigned short addr, unsigned char value) {},
            [](void* arg, unsigned short port) { return 0x00; },
            [](void* arg, unsigned short port, unsigned char value) {
                // request break the execute function after output port operand has executed.
                ((Z80*)arg)->requestBreak();
            }, &z80);
    z80.setDebugMessage([](void* arg, const char* msg) { puts(msg); });
    z80.setConsumeClockCallback([](void* arg, int clocks) { printf("consume %dHz\n", clocks); });
    puts("===== execute(0) =====");
    printf("actualExecuteClocks = %dHz\n", z80.execute(0));
    puts("===== execute(1) =====");
    printf("actualExecuteClocks = %dHz\n", z80.execute(1));
    puts("===== execute(0x7FFFFFFF) =====");
    printf("actualExecuteClocks = %dHz\n", z80.execute(0x7FFFFFFF));
    return 0;
}
```

Result is following:

```
===== execute(0) =====
actualExecuteClocks = 0Hz ... 0 is specified, no instruction is executed at all
===== execute(1) =====
consume 2Hz
consume 2Hz
consume 3Hz
consume 3Hz
[0000] LD BC<$0000>, $1234
actualExecuteClocks = 10Hz ... specify 1 to single step execution
===== execute(0x7FFFFFFF) =====
consume 2Hz
consume 2Hz
consume 3Hz
[0003] LD A<$FF>, $01
consume 2Hz
consume 2Hz
consume 4Hz
[0005] OUT (C<$34>), A<$01>
consume 4Hz
actualExecuteClocks = 19Hz ... 2147483647Hz+ is not executed but interrupted after completion of OUT due to requestBreak during OUT execution
```

### 5. Generate interrupt

#### IRQ; Interrupt Request

```c++
    z80.generateIRQ(vector);
```

#### NMI; Non Maskable Interrupt

```c++
    z80.generateNMI(address);
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

- call `resetDebugMessage` if you want to remove the detector.
- call `setDebugMessageFP` if you want to use the function pointer.

### Use break point

If you want to execute processing just before executing an instruction of specific program counter value _(in this ex: \$008E)_, you can set a breakpoint as follows:

```c++
    z80.addBreakPoint(0x008E, [](void* arg) -> void {
        printf("Detect break point! (PUSH ENTER TO CONTINUE)");
        char buf[80];
        fgets(buf, sizeof(buf), stdin);
    });
```

- `addBreakPoint` can set multiple breakpoints for the same address.
- call `removeBreakPoint` or `removeAllBreakPoints` if you want to remove the break point(s).
- call `addBreakPointFP` if you want to use the function pointer.

### Use break operand

If you want to execute processing just before executing an instruction of specific operand number, you can set a breakpoint as follows:

```c++
    // break when NOP ... $00
    z80.addBreakOperand(0x00, [](void* arg, unsigned char* opcode, int opcodeLength) -> void {
        printf("Detect break operand! (PUSH ENTER TO CONTINUE)");
        char buf[80];
        fgets(buf, sizeof(buf), stdin);
    });

    // break when RLC B ... $CB $00
    z80.addBreakOperand(0xCB, 0x00, [](void* arg, unsigned char* opcode, int opcodeLength) -> void {
        printf("Detect break operand! (PUSH ENTER TO CONTINUE)");
        char buf[80];
        fgets(buf, sizeof(buf), stdin);
    });

    // break when RLC (IX+d) ... $DD $CB, $06
    z80.addBreakOperand(0xDD, 0xCB, 0x06, [](void* arg, unsigned char* opcode, int opcodeLength) -> void {
        printf("Detect break operand! (PUSH ENTER TO CONTINUE)");
        char buf[80];
        fgets(buf, sizeof(buf), stdin);
    });
```

- the opcode and length at break are stored in `opcode` and `opcodeLength` when the callback is made.
- `addBreakOperand` can set multiple breakpoints for the same operand.
- call `removeBreakOperand` or `removeAllBreakOperands` if you want to remove the break operand(s).
- call `addBreakOperandFP` if you want to use the function pointer.

### Detect clock consuming

If you want to implement stricter synchronization, you can capture the CPU clock consumption timing as follows:

```c++
    z80.setConsumeClockCallback([](void* arg, int clock) -> void {
        printf("consumed: %dHz\n", clock);
    });
```

- call `resetConsumeClockCallback` if you want to remove the detector.
- call `setConsumeClockCallbackFP` if you want to use the function pointer.

> With this callback, the CPU cycle (clock) can be synchronized in units of 3 to 4 Hz, and while the execution of a single Z80 instruction requires approximately 10 to 20 Hz of CPU cycle (time), the SUZUKI PLAN - Z80 Emulator can synchronize the CPU cycle (time) for fetch, execution, write back, etc. However, the SUZUKI PLAN - Z80 Emulator can synchronize fetches, executions, writes, backs, etc. in smaller units. This makes it easy to implement severe timing emulation.

### If implement quick save/load

Save the member variable `reg` when quick saving:

```c++
    fwrite(&z80.reg, sizeof(z80.reg), 1, fp);
```

Extract to the member variable `reg` when quick loading:

```c++
    fread(&z80.reg, sizeof(z80.reg), 1, fp);
```

### Handling of CALL instructions

The occurrence of the branches by the CALL instructions can be captured by the CallHandler.
CallHandler will be called back immediately **after** a branch by a CALL instruction occurs.

> CallHandle will called back after the return address is stacked in the RAM.

```c++
    z80.addCallHandler([](void* arg) -> void {
        printf("Executed a CALL instruction:\n");
        printf("- Branched to: $%04X\n", ((Z80*)arg)->reg.PC);
        unsigned short sp = ((Z80*)arg)->reg.SP;
        unsigned short returnAddr = ((Z80*)arg)->readByte(sp + 1);
        returnAddr <<= 8;
        returnAddr |= ((Z80*)arg)->readByte(sp);
        printf("- Return to: $%04X\n", returnAddr);
    });
```

- `addCallHandler` can set multiple CallHandlers.
- call `removeAllCallHandlers` if you want to remove the CallHandler(s).
- call `addCallHandlerFP` if you want to use the function pointer.
- CallHandler also catches branches caused by interrupts.
- In the case of a condition-specified branch instruction, only the case where the branch is executed is callbacked.

### Handling of RET instructions

The occurrence of the branches by the RET instructions can be captured by the ReturnHandler.
ReturnHandler will be called back immediately **before** a branch by a RET instruction occurs.

> ReturnHandle will called back while the return address is stacked in the RAM.

```c++
    z80.addReturnHandler([](void* arg) -> void {
        printf("Detected a RET instruction:\n");
        printf("- Branch from: $%04X\n", ((Z80*)arg)->reg.PC);
        unsigned short sp = ((Z80*)arg)->reg.SP;
        unsigned short returnAddr = ((Z80*)arg)->readByte(sp + 1);
        returnAddr <<= 8;
        returnAddr |= ((Z80*)arg)->readByte(sp);
        printf("- Return to: $%04X\n", returnAddr);
    });
```

- `addReturnHandler` can set multiple ReturnHandlers.
- call `removeAllReturnHandlers` if you want to remove the ReturnHandler(s).
- call `addReturnHandlerFP` if you want to use the function pointer.
- In the case of a condition-specified branch instruction, only the case where the branch is executed is callbacked.

## Advanced Compile Flags

There is a compile flag that disables certain features in order to adapt to environments with poor performance environments, i.e: Arduino or ESP32:

|Compile Flag|Feature|
|:-|:-|
|`-DZ80_DISABLE_DEBUG`|disable `setDebugMessage` method|
|`-DZ80_DISABLE_BREAKPOINT`|disable `addBreakPoint` and `addBreakOperand` methods|
|`-DZ80_DISABLE_NESTCHECK`|disable `addCallHandler` and `addReturnHandler` methods|
|`-DZ80_CALLBACK_WITHOUT_CHECK`|Omit the check process when calling `consumeClock` callback (NOTE: Crashes if `setConsumeClock` is not done)|
|`-DZ80_CALLBACK_PER_INSTRUCTION`|Calls `consumeClock` callback on an instruction-by-instruction basis (NOTE: two or more instructions when interrupting)|
|`-DZ80_UNSUPPORT_16BIT_PORT`|Reduces extra branches by always assuming the port number to be 8 bits|

## License

[MIT](LICENSE.txt)
