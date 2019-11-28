## minilogue xd SDK ソースコードとテンプレートプロジェクト

### 概要

下記ディレクトリに [minilogue xd synthesizer](https://www.korg.com/products/synthesizers/minilogue_xd) で使用できる自作オシレーターやエフェクトのビルドに必要なファイルが全て揃っています.

#### 互換性に関して

SDK version 1.1-0 でビルドされた user units (ビルド済みのカスタムコードバイナリ）を実行するためには, version 2.00以降のファームウェアが製品本体にインストールされている必要があります.

#### 全体の構造:
 * [inc/](inc/) : ヘッダファイル
 * [osc/](osc/) : 自作オシレーターのテンプレートプロジェクト
 * [modfx/](modfx/) : 自作モジュレーション・エフェクトのテンプレートプロジェクト
 * [delfx/](delfx/) : 自作ディレイ・エフェクトのテンプレートプロジェクト
 * [revfx/](revfx/) : 自作リバーブ・エフェクトのテンプレートプロジェクト
 * [demos/](demos/) : デモプロジェクト

### 開発環境の設定

 1. このリポジトリをクローンし, 初期化とサブモジュールのアップデートを行います.

```
 $ git clone https://github.com/korginc/logue-sdk.git
 $ cd logue-sdk
 $ git submodule update --init
 ```
 2. 必要なツールチェーンをインストールします: [GNU Arm Embedded Toolchain](../../tools/gcc)
 3. その他のユーティリティをインストールします:
    1. [GNU Make](../../tools/make)
    2. [Info-ZIP](../../tools/zip)
    3. [logue-cli](../../tools/logue-cli) (optional)

### プロジェクトのビルド

 1. プロジェクトのディレクトリに移動します.
 

```
$ cd logue-sdk/platform/minilogue-xd/demos/waves/
```
 2. プロジェクをビルドするために `make` を実行します.
 
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
 3. *Packaging...* という表示の通り,  *.prlgunit* というファイルが生成されます. これがビルド成果物となります.
 
###  *.prlgunit* ファイルの操作と使い方

*.prlgunit* ファイルは自作コンテンツのバイナリデータ本体とメタデータを含む簡潔なパッケージファイルです. このファイルは [logue-cli utility](../../tools/logue-cli/) もしくは [Librarian application](https://www.korg.com/products/synthesizers/minilogue_xd/librarian_contents.php) 経由で [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd) にアップロードすることが出来ます.

## デモコード

### Waves

Waves はlogue-sdkのオシレーターAPIで提供されているウェーブテーブルを使用したモーフィング・ウェーブテーブル・オシレーターです. APIの機能やパラメーターの使い方を学ぶ上で良いリファレンスになるでしょう. ソースコードや詳細は [demos/waves/](demos/waves/) を見て下さい.

## 新しいプロジェクトを作る

1. 作成したいコンテンツ（osc/modfx/delfx/revfx）のテンプレートプロジェクトをコピーし, 好きな名前に変更して下さい.

2. *Makefile* の先頭にある PLATFORMDIR 変数が [platform/](../) ディレクトリを指していることを確認してください. このパスは外部依存ファイルや必要なツールを参照するのに用いられます.

3. *project.mk* に好きなプロジェクト名を入力して下さい. 構文についての詳細は [project.mk](#project.mk) のセクションを参照して下さい.

4. プロジェクトにソースファイルを追加した際は, project.mk* ファイルを編集し, ビルドの際にソースファイルが発見できるようにする必要があります.

5. *manifest.json* ファイルを編集し, 適切なメタデータをプロジェクトに追加します. 構文についての詳細は [manifest.json](#manifest.json) のセクションを参照して下さい.


## プロジェクトの構造
### manifest.json

manifestファイルはjsonフォーマットで下記のフィールドを設定する必要があります.

* platform (string) : プラットフォームの名前. *minilogue-xd* に設定します.
* module (string) : モジュールの名前, *osc*, *modfx*, *delfx*, *revfx* のいずれかに設定して下さい.
* api (string) : 使用している API バージョン. (フォーマット: MAJOR.MINOR-PATCH)
* dev_id (int) : デベロッパーID. 現在は未使用, 0に設定します.
* prg_id (int) : プログラムID. 現在は未使用, リファレンスとして設定しても構いません.
* version (string) : プログラムのバージョン. (フォーマット: MAJOR.MINOR-PATCH)
* name (string) : プログラムの名前. (本体のディスプレイに表示されます.)
* num_params (int) : エディットメニューで操作可能なパラメーターの数. *osc* のプロジェクトの時のみ使用し, エフェクトのプロジェクトでは0に設定します. オシレーターは最大6個のパラメーターを持つことが出来ます. 
* params (array) : パラメーターの詳細. *osc* のプロジェクトにのみ有効です. オシレーター以外の場合はempty array([])にする必要があります.

パラメーターの詳細は下記のフォーマットで, 4個の要素を持つ配列として定義されています.


0. name (string) : 最大10文字がエディットメニューで表示できます
1. minimum value (int) : -100, 100 の間に設定します.
2. maximum value (int) : -100, 100 の間に設定し, なおかつ minimum value より大きな値に設定します.
3. type (string) : "%" を入れると百分率の値として扱われ, "" にすると型無しの値として扱われます.

型無しの値に設定した場合, minimum value と maximum value は正の値に設定する必要があります. また表示される値は1オフセットされます.(0-9は1-10として表示されます.)

manifest file のサンプルは下記の通りです:

```
{
    "header" : 
    {
        "platform" : "minilogue-xd",
        "module" : "osc",
        "api" : "1.1-0",
        "dev_id" : 0,
        "prg_id" : 0,
        "version" : "1.0-0",
        "name" : "waves",
        "num_param" : 6,
        "params" : [
            ["Wave A",      0,  45,  ""],
            ["Wave B",      0,  43,  ""],
            ["Sub Wave",    0,  15,  ""],
            ["Sub Mix",     0, 100, "%"],
            ["Ring Mix",    0, 100, "%"],
            ["Bit Crush",   0, 100, "%"]
          ]
    }
}
```

### project.mk

このファイルはカスタムを簡単にするためにメインのMakefileに含まれています. 

下記の変数が宣言されています:

* PROJECT : プロジェクト名. ビルドされたプロジェクトのファイルネームになります.
* UCSRC : Cのソースファイル.
* UCXXSRC :  C++のソースファイル.
* UINCDIR : ヘッダーの検索パス.
* UDEFS : カスタムgccの定義フラグ.
* ULIB : リンカライブライブラリのフラグ.
* ULIBDIR : リンカライブラリの検索パス.

### tests/

この *tests/* ディレクトリにはオシレーターやエフェクトの書き方の説明したシンプルなテストコードがあります.　有用なコンテンツではありませんが、環境のテスト等を行う際に参照して下さい.

## Web Assembly Builds _(実験中)_

Web Assembly(Emscripten) のビルドと Web Audio Player がalpha/wasm-builds branchにあります.
これはまだ実験的な機能で、正しく動作しない可能性があります.


## トラブルシューティング

### missing CMSIS arm_math.h と表示されコンパイルできない

CMSIS submodule の初期化とアップデートが行われていない可能性があります.
```
git submodule update --init
```
を実行して下さい。
