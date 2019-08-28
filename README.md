# SUZUKI PLAN - Z80 Emulator (WIP)

The Z80 was an 8-bit CPU developed by Zylog corporation, announced in 1976, and widely used in computers and game consoles in the 1980s.
It is not just a relic of the past, but continues to be used in embedded systems that require accuracy in processing execution time, such as real-time systems.

"SUZUKI PLAN - Z80 Emulator" is an emulator under development based on the following design guidelines to support the development of programs and/or emulators using Z80.

__(THREE EASY GUIDELINES)__

- Make emulator implementation `EASY` & simple (Realized by providing single header: [z80.hpp](z80.hpp))
-Provide under the license that `EASY` to adopt in various programs ([MIT](LICENSE.txt))
-Highly readable and `EASILY` customizable (priority for readability over performance)

> Since I do not have deep knowledge about Z80 myself, I'm implementing it with reference to the information on the following site.
> 
> - [8ビット CPU Z80](http://www.yamamo10.jp/yamamoto/comp/Z80/index.php) of [山本研究所](http://www.yamamo10.jp/yamamoto/index.html)

## WIP status

- [ ] 3.1 8bit load instructions <sup>*inprogress*</sup>
- [ ] 3.2 16bit load instructions
- [ ] 3.3 block load instructions
- [ ] 3.4 exchange instructions
- [ ] 4. stack instructions
- [ ] 5.1 left rotate instructions
- [ ] 5.2 right rotate instructions
- [ ] 5.3 left shift instructions
- [ ] 5.4 right shift instructions
- [ ] 6.1 8bit arithmetic (ADD) instructions
- [ ] 6.1 8bit arithmetic (DEC) instructions
- [ ] 6.2 16bit arithmetic (ADD) instructions
- [ ] 6.2 16bit arithmetic (DEC) instructions
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

## How to test

### Command

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

## How to use

See the [test.cpp](test.cpp) implementation.

## License

[MIT](LICENSE.txt)
