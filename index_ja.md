---
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: home_ja
permalink: /ja/
---

*logue SDK* とは KORG [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) 等のシンセサイザーで使用可能な自作オシレーターやエフェクトを作成可能な C/C++ のソフト開発キットとAPIです。

## Quick Start

### 開発環境の設定

 * [リポジトリ](https://github.com/korginc/logue-sdk)をクローンし, 初期化とサブモジュールのアップデートを行います.

```
 $ git clone https://github.com/korginc/logue-sdk.git
 $ cd logue-sdk
 $ git submodule update --init
 ```
 * 必要なツールチェーンをインストールします: [GNU Arm Embedded Toolchain](https://github.com/korginc/logue-sdk/tree/master/tools/gcc)
 * その他のユーティリティをインストールします:
    * [GNU Make](https://github.com/korginc/logue-sdk/tree/master/tools/make)
    * [Info-ZIP](https://github.com/korginc/logue-sdk/tree/master/tools/zip)
    * [logue-cli](https://github.com/korginc/logue-sdk/tree/master/tools/logue-cli) (optional)

### デモプロジェクトのビルド （Waves）

Waves はlogue-sdkのオシレーターAPIで提供されているウェーブテーブルを使用したモーフィング・ウェーブテーブル・オシレーターです. APIの機能やパラメーターの使い方を学ぶ上で良いリファレンスになるでしょう. 

 * プロジェクトのディレクトリに移動します.
 
```
$ cd logue-sdk/platform/nutekt-digital/demos/waves/
```
_Note: パスの中にある "prologue" を対象のプラットフォーム名に置き換えてください._

 * プロジェクをビルドするために `make` を実行します.
 
```
$ make
Compiler Options
../../../../tools/gcc/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -std=c11 -mstructure-size-boundary=8 -W -Wall -Wextra -Wa,-alms=./build/lst/ -DSTM32F401xC -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM4 -D__FPU_PRESENT -I. -I./inc -I./inc/api -I../../inc -I../../inc/dsp -I../../inc/utils -I../../../ext/CMSIS/CMSIS/Include
    
Compiling _unit.c
Compiling waves.cpp
Linking build/waves.elf
Creating build/waves.hex
Creating build/waves.bin
Creating build/waves.dmp

   text	   data	    bss	    dec	    hex	filename
   2304	      4	    144	   2452	    994	build/waves.elf

Creating build/waves.list
Packaging to ./waves.prlgunit
 
Done
```
    
 * *Packaging...* という表示の通り,  *.prlgunit* というファイルが生成されます. これがビルド成果物となります.

    _Note: 拡張子は minilogue xd の場合 *.mnlgxdunit*, Nu:Tekt NTS-1 digital の場合 *.ntkdigunit* となります._

### *「unit」* ファイルの操作と使い方

*.prlgunit*, *.mnlgxdunit*, and  *.ntkdigunit* ファイルは自作コンテンツのバイナリデータ本体とメタデータを含む簡潔なパッケージファイルです. 
このファイルは [logue-cli utility](https://github.com/korginc/logue-sdk/tools/logue-cli/) もしくは 製品の Librarian application ([KORG ウエブサイト](https://korg.com)の製品ページに参考) 経由でアップロードすることが出来ます.
