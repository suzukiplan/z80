# SUZUKI PLAN - Z80 Emulator (WIP)

Z80は、嶋正利ら元インテル社のメンバーがザイログ社にスピンアウトして開発した8bit CPUで、1976年に発表され、1980年代のコンピュータやゲーム機に広く使われていました。それは、単なる過去の遺物ではなく、現代でもリアルタイムシステムなど、処理実行時間の正確さが求められる組み込みシステムで利用され続けています。

SUZUKI PLAN - Z80 Emulatorは、Z80を用いたプログラムやエミュレータの開発を支援する目的で、以下の設計指針に基づき開発中のエミュレータです。

- 簡単にエミュレータ実装できること（1ヘッダー形式で提供することで実現）
- 自由度の高いライセンスであること（MITライセンスを採用）
- 可読性が高くカスタマイズし易い実装であること（パフォーマンスより可読性を優先）

なお、私自身Z80について深い知見がある訳ではないので、以下サイトの情報を参考に実装中です。

- [山本研究所](http://www.yamamo10.jp/yamamoto/index.html) の [8ビット CPU Z80](http://www.yamamo10.jp/yamamoto/comp/Z80/index.php)記載情報

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

[test.cpp](test.cpp)の実装を参照。

## License

[MIT](LICENSE.txt)
