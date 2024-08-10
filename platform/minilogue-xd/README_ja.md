## minilogue xd SDK ソースコードとテンプレートプロジェクト

### 概要

下記ディレクトリに [minilogue xd synthesizer](https://www.korg.com/jp/products/synthesizers/minilogue_xd) で使用できる自作オシレーターやエフェクトのビルドに必要なファイルが全て揃っています.

#### 互換性に関して

SDK version 1.1-0 でビルドされた user units (ビルド済みのカスタムコードバイナリ）を実行するためには, version 2.00以降のファームウェアが製品本体にインストールされている必要があります.

#### 全体の構造:
 * [inc/](inc/) : ヘッダファイル
 * [dummy-osc/](dummy-osc/) : 自作オシレーターのテンプレートプロジェクト
 * [dummy-modfx/](dummy-modfx/) : 自作モジュレーション・エフェクトのテンプレートプロジェクト
 * [dummy-delfx/](dummy-delfx/) : 自作ディレイ・エフェクトのテンプレートプロジェクト
 * [dummy-revfx/](dummy-revfx/) : 自作リバーブ・エフェクトのテンプレートプロジェクト
 * [waves/](waves/) : デモオシレータープロジェクト

### 開発環境の設定

#### 従来の方法（Dockerを使用しない方法）

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


### Docker開発環境

[Docker-based Build Environment](../../docker) をご参照ください.

### デモプロジェクトのビルド （Waves）

Waves はlogue-sdkのオシレーターAPIで提供されているウェーブテーブルを使用したモーフィング・ウェーブテーブル・オシレーターです. APIの機能やパラメーターの使い方を学ぶ上で良いリファレンスになるでしょう. ソースコードや詳細は [waves/](waves/) を見て下さい.

#### 従来のビルド手法（Dockerを使用しない方法）

 1. プロジェクトのディレクトリに移動します.
 
```
$ cd logue-sdk/platform/minilogue-xd/waves/
```

 2. プロジェクをビルドするために `make` を実行します.
 
```
$ make
Compiler Options
<path>/logue-sdk/tools/gcc/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -std=c11 -mstructure-size-boundary=8 -W -Wall -Wextra -Wa,-alms=<path>/logue-sdk/platform/minilogue-xd/waves/build/lst/ -DSTM32F401xC -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM4 -D__FPU_PRESENT -I. -I<path>/logue-sdk/platform/minilogue-xd/waves/inc -I<path>/logue-sdk/platform/minilogue-xd/waves/inc/api -I<path>/logue-sdk/platform/minilogue-xd/inc -I<path>/logue-sdk/platform/minilogue-xd/inc/dsp -I<path>/logue-sdk/platform/minilogue-xd/inc/utils -I<path>/logue-sdk/platform/minilogue-xd/../ext/CMSIS/CMSIS/Include

Compiling _unit.c
Compiling waves.cpp
Linking <path>/logue-sdk/platform/minilogue-xd/waves/build/waves.elf
Creating <path>/logue-sdk/platform/minilogue-xd/waves/build/waves.hex
Creating <path>/logue-sdk/platform/minilogue-xd/waves/build/waves.bin
Creating <path>/logue-sdk/platform/minilogue-xd/waves/build/waves.dmp

   text	   data	    bss	    dec	    hex	filename
   2040	      4	    144	   2188	    88c	<path>/logue-sdk/platform/minilogue-xd/waves/build/waves.elf

Creating <path>/logue-sdk/platform/minilogue-xd/waves/build/waves.list
Done
```
 3. 最終パッケージファイル (*.mnlgxdunit*)を作成するために`make install`を実行します.
 
 ```
 $ make install
Packaging to <path>/logue-sdk/platform/minilogue-xd/waves/build/waves.mnlgxdunit
Deploying to <path>/logue-sdk/platform/minilogue-xd/waves/waves.mnlgxdunit
Done
 ```
 4. *.mnlgxdunit* ファイルが生成されます. これがビルド成果物となります.
 
#### Docker Containerを使用したビルド

 1. [docker/run_interactive.sh](../../docker/run_interactive.sh) を実行します.

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
 1.1. (オプション) ビルド可能なプロジェクトを表示したい場合は下記コマンドを実行します.

```
user@logue-sdk:~$ build -l --minilogue-xd
- minilogue-xd/dummy-delfx
- minilogue-xd/dummy-modfx
- minilogue-xd/dummy-osc
- minilogue-xd/dummy-revfx
- minilogue-xd/waves
 ```

 2. ビルドしたいプロジェクトのパスを指定し, ビルドコマンドを実行します (下記は `minilogue-xd/waves` をビルドする例です).

```
 user@logue-sdk:~$ build minilogue-xd/waves
 >> Initializing minilogue-xd development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/minilogue-xd/waves
 Compiler Options
 /usr/bin/arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -std=c11 -mstructure-size-boundary=8 -W -Wall -Wextra -Wa,-alms=/workspace/minilogue-xd/waves/build/lst/ -DSTM32F401xC -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM4 -D__FPU_PRESENT -I. -I/workspace/minilogue-xd/waves/inc -I/workspace/minilogue-xd/waves/inc/api -I/workspace/minilogue-xd/inc -I/workspace/minilogue-xd/inc/dsp -I/workspace/minilogue-xd/inc/utils -I/workspace/ext/CMSIS/CMSIS/Include
 
 Compiling _unit.c
 Compiling waves.cpp
 cc1: warning: option '-mstructure-size-boundary' is deprecated
 Linking /workspace/minilogue-xd/waves/build/waves.elf
 Creating /workspace/minilogue-xd/waves/build/waves.hex
 Creating /workspace/minilogue-xd/waves/build/waves.bin
 Creating /workspace/minilogue-xd/waves/build/waves.dmp
 Creating /workspace/minilogue-xd/waves/build/waves.list
 
    text	   data	    bss	    dec	    hex	filename
    2032	      4	    144	   2180	    884	/workspace/minilogue-xd/waves/build/waves.elf
 
 Done
 
 >> Installing /workspace/minilogue-xd/waves
 Packaging to /workspace/minilogue-xd/waves/build/waves.mnlgxdunit
 Deploying to /workspace/minilogue-xd/waves/waves.mnlgxdunit
 Done
 
 >> Resetting environment
 >> Cleaning up minilogue-xd development environment.

 ```

###  「unit」ファイルの操作と使い方

*.mnlgxdunit* ファイルは自作コンテンツのバイナリデータ本体とメタデータを含む簡潔なパッケージファイルです. このファイルは [logue-cli utility](../../tools/logue-cli/) もしくは [Librarian application](https://www.korg.com/jp/products/synthesizers/minilogue_xd/librarian_contents.php) 経由で [minilogue xd](https://www.korg.com/jp/products/synthesizers/minilogue_xd) にアップロードすることが出来ます.

## 新しいプロジェクトを作る

1. 作成したいコンテンツ（osc/modfx/delfx/revfx）のテンプレートプロジェクトをコピーし, 好きな名前に変更して下さい.

2. *Makefile* の先頭にある PLATFORMDIR 変数が [platform/](../) ディレクトリを指していることを確認してください. このパスは外部依存ファイルや必要なツールを参照するのに用いられます.

3. *project.mk* に好きなプロジェクト名を入力して下さい. 構文についての詳細は [project.mk](#projectmk) のセクションを参照して下さい.

4. プロジェクトにソースファイルを追加した際は, project.mk* ファイルを編集し, ビルドの際にソースファイルが発見できるようにする必要があります.

5. *manifest.json* ファイルを編集し, 適切なメタデータをプロジェクトに追加します. 構文についての詳細は [manifest.json](#manifestjson) のセクションを参照して下さい.

## プロジェクトの構造

### manifest.json

manifestファイルはjsonフォーマットで下記の例のように書きます.

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

## Core API

logue SDK での「unit」作成にあたり, 中心となるAPIの概要を説明します.

### オシレーター

主要な機能のソースファイルには *userosc.h* をインクルードし, 下記の関数を含む必要があります.

* `void OSC_INIT(uint32_t platform, uint32_t api)`: オシレーターのインスタンス化の際に呼ばれます. このコールバック関数を使用して必要な初期化処理を行います.

* `void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)`: ここで実際の波形が計算されます. この関数は *frames* サンプルで必ず呼ばれます(フレーム毎に1サンプル).
サンプルは *yn* バッファーに書き込む必要があります. 出力されるサンプルは[Q31 固定小数点フォーマット](https://en.wikipedia.org/wiki/Q_(number_format)) にする必要があります.

    _Note: `inc/utils/fixed_math.h` で定義されている `f32_to_q31(f)` マクロを使うことで, 浮動小数点数をQ31フォーマットに変換できます.  `user_osc_param_t` の定義については `inc/userosc.h`_ も参照して下さい.

    _Note: 最大64フレームまでのバッファサイズをサポートする必要があります. 必要な場合はより小さいバッファーで複数の処理を実行することができます. （2のべき乗で最適化してください : 16, 32, 64）_

* `void OSC_NOTEON(const user_osc_param_t * const params)`: ノート・オン時に呼ばれます.

* `void OSC_NOTEOFF(const user_osc_param_t * const params)`: ノート・オフ時に呼ばれます.

* `void OSC_PARAM(uint16_t index, uint16_t value)`: パラメーター変更時に呼ばれます.

より詳細な情報については [Oscillator Instance API reference](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__osc__inst.html) を参照してください. また, [Oscillator Runtime API](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__osc__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__utils.html) ,  [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/namespacedsp.html) も役に立ちます.

### Modulation Effects API (modfx)

主要な機能のソースファイルには *usermodfx.h* をインクルードし, 下記の関数を含む必要があります.

* `void MODFX_INIT(uint32_t platform, uint32_t api)`: エフェクトのインスタンス化の際に呼ばれます. このコールバック関数を使用して必要な初期化処理を行います. APIについての詳細は `inc/userprg.h` を参照して下さい.

* `void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn,  float *sub_yn, uint32_t frames)`: ここでは入力サンプルを処理します. この関数は *frames* サンプルで必ず呼ばれます(フレーム毎に1サンプル). *\*\_xn* は入力バッファ,  *\*\_yn*  は出力バッファとなり, ここで処理結果を書き込みます。

    _Note: prologueの2ティンバーをサポートするために、main\_とsub\_の入力と出力のバージョンがあります. prologueではmainとsubの両方に同じ方法で並列に処理する必要があります. マルチティンバーでないプラットフォームでは *sub\_xn* , *sub\_yn* は無視できます._

    _Note: 最大64フレームまでのバッファサイズをサポートする必要があります. 必要な場合はより小さいバッファーで複数の処理を実行することができます. （2のべき乗で最適化してください : 16, 32, 64）_

* `void MODFX_PARAM(uint8_t index, uint32_t value)`: パラメーター変更時に呼ばれます.

より詳細な情報については [Modulation Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__modfx__inst.html) 参照してください. また, [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__utils.html),   [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/namespacedsp.html) も役に立ちます.

### Delay Effects API (delfx)

主要な機能のソースファイルには *userdelfx.h* をインクルードし, 下記の関数を含む必要があります.

* `void DELFX_INIT(uint32_t platform, uint32_t api)`: エフェクトのインスタンス化の際に呼ばれます. このコールバック関数を使用して必要な初期化処理を行います.

* `void DELFX_PROCESS(float *xn, uint32_t frames)`: ここでは入力サンプルを処理します. この関数は *frames* サンプルで必ず呼ばれます(フレーム毎に1サンプル). この場合, *xn* は入力と出力バッファーの両方になります. 結果はDryとWetを適切な割合で合わせて書き込む必要があります. （例: shift-depthパラメーターで設定する）

    _Note: 最大64フレームまでのバッファサイズをサポートする必要があります. 必要な場合はより小さいバッファーで複数の処理を実行することができます. （2のべき乗で最適化してください : 16, 32, 64）_
    
* `void DELFX_PARAM(uint8_t index, uint32_t value)`: パラメーター変更時に呼ばれます.

より詳細な情報については [Delay Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__delfx__inst.html) を参照してください. また [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__utils.html) , [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/namespacedsp.html) も役に立ちます.

### Reverb Effects API (revfx)

主要な機能のソースファイルには *userrevfx.h* をインクルードし, 下記の関数を含む必要があります.

* `void REVFX_INIT(uint32_t platform, uint32_t api)`: エフェクトのインスタンス化の際に呼ばれます. このコールバック関数を使用して必要な初期化処理を行います. 

* `void REVFX_PROCESS(float *xn, uint32_t frames)`: ここでは入力サンプルを処理します.  この関数は *frames* サンプルで必ず呼ばれます(フレーム毎に1サンプル). この場合, *xn* は入力と出力バッファーの両方になります. 結果はDryとWetを適切な割合で合わせて書き込む必要があります. （例: shift-depthパラメーターで設定する）

    _Note: 最大64フレームまでのバッファサイズをサポートする必要があります. 必要な場合はより小さいバッファーで複数の処理を実行することができます. （2のべき乗で最適化してください : 16, 32, 64）_
    
* `void REVFX_PARAM(uint8_t index, uint32_t value)`: パラメーター変更時に呼ばれます.

より詳細な情報については [Reverb Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__revfx__inst.html) を参照してください. また. [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__utils.html) , [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/namespacedsp.html) も役に立ちます.

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
