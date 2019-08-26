# SUZUKI PLAN - Z80 Emulator (WIP)

Z80は、嶋正利ら元インテル社のメンバーがザイログ社にスピンアウトして開発した8bit CPUで、1976年に発表され、1980年代のコンピュータやゲーム機に広く使われていました。それは、単なる過去の遺物ではなく、現代でもリアルタイムシステムなど、処理実行時間の正確さが求められる組み込みシステムで利用され続けています。

SUZUKI PLAN - Z80 Emulatorは、Z80を用いたプログラムやエミュレータの開発を支援する目的で、以下の設計指針に基づき開発中のエミュレータです。

- 簡単にエミュレータ実装できること（1ヘッダー形式で提供することで実現）
- 自由度の高いライセンスであること（MITライセンスを採用）
- 可読性が高くカスタマイズし易い実装であること

なお、私自身Z80について深い知見がある訳ではないので、以下サイトの情報を参考に実装中です。

- [山本研究所](http://www.yamamo10.jp/yamamoto/index.html) の [8ビット CPU Z80](http://www.yamamo10.jp/yamamoto/comp/Z80/index.php)記載情報

## How to test

```
git clone https://github.com/suzukiplan/z80.git
cd z80
make
```

> 現状NOPしか動かしていません^^;

## How to use

[test.cpp](test.cpp)の実装を参照。

## License

[MIT](LICENSE.txt)
