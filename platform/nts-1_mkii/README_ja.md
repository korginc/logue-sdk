## Nu:Tekt NTS-1 digital kit mkII SDK Source and Template Projects

[English](./README.md)

### 概要

このディレクトリに [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/jp/products/synthesizers/nts_1_mk2/) で使用できるカスタム・オシレーターやエフェクトのビルドに必要なすべてのファイルが揃っています.

#### 必要なソフトウェアバージョン

SDK version 2.0.0でビルドされたユーザーユニットを実行するために, 製品本体にバージョン1.0.0以降のファームウェアがインストールされている必要があります.

#### このディレクトリの構造:

 * [common/](common/) : 共通のヘッダーファイル.
 * [ld/](ld/) : 共通のリンカーファイル.
 * [dummy-osc/](dummy-osc/) : オシレーターのプロジェクトのテンプレート.
 * [dummy-modfx/](dummy-modfx/) : モジュレーション・エフェクトのプロジェクトのテンプレート.
 * [dummy-delfx/](dummy-delfx/) : ディレイ・エフェクトのプロジェクトのテンプレート.
 * [dummy-revfx/](dummy-revfx/) : リバーブ・エフェクトのプロジェクトのテンプレート.

### 製品の技術仕様

* *CPU*: ARM Cortex-M7 (STM32H725)
* *ユニットのファイル形式*: ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamically linked
* *ツールチェイン*: gcc-arm-none-eabi-10.3-2021.10

#### 対応モジュール

| モジュール | スロット数 | 最大ユニットサイズ | 最大RAM読み込みサイズ | 利用できる外部メモリのサイズ |
|--------|-------|---------------|-------------------|-----------------------------|
| osc    | 16    | ~ 48KB        | 48KB              | -                           |
| modfx  | 16    | ~ 16KB        | 16KB              | 256KB                       |
| delfx  | 8     | ~ 24KB        | 24KB              | 3MB                         |
| revfx  | 8     | ~ 24KB        | 24KB              | 3MB                         |

### 開発環境の環境構築

#### Dockerベースのビルド環境

 [Docker-based Build Environment](../../docker)を参照してください.

#### Dockerを使わないビルド環境（prologue, minilogue XD, 初代NTS-1の環境と同じ手法）

 1. Gitでこのリポジトリをクローンし, サブモジュールの初期化/アップデートを実行します.

```
 $ git clone https://github.com/korginc/logue-sdk.git
 $ cd logue-sdk
 $ git submodule update --init
 ```
 2. ツールチェインをインストールします: [GNU Arm Embedded Toolchain](../../tools/gcc)
 3. ビルドに必要なそのほかのユーティリティをインストールします:
    1. [GNU Make](../../tools/make)

### ユニットのビルド方法

#### Dockerベースの開発環境でのビルド

 1. [docker/run_interactive.sh](../../docker/run_interactive.sh) コマンドを実行します.

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```

 1.1. (オプション) ビルド可能なプロジェクトの一覧を表示します.

```
 user@logue-sdk $ build -l --nts-1_mkii
 - nts-1_mkii/dummy-delfx
 - nts-1_mkii/dummy-modfx
 - nts-1_mkii/dummy-osc
 - nts-1_mkii/dummy-revfx
 - nts-1_mkii/waves
 ```

 2. ビルドしたいプロジェクトのパスを指定しbuildコマンドを実行します (下記は`nts-1_mkii/waves`をビルドする例).

```
 user@logue-sdk $ build nts-1_mkii/waves
 >> Initializing NTS-1 mkii development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/nts-1_mkii/waves
 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc
 Linking /workspace/nts-1_mkii/waves//build/waves.elf
 /usr/bin/arm-none-eabi-gcc /workspace/nts-1_mkii/waves//build/obj/header.o /workspace/nts-1_mkii/waves//build/obj/_unit_base.o /workspace/nts-1_mkii/waves//build/obj/unit.o -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -nostartfiles -Wl,-z,max-page-size=128,-Map=/workspace/nts-1_mkii/waves//build/waves.map,--cref,--no-warn-mismatch,--library-path=/workspace/nts-1_mkii/waves//../ld,--script=/workspace/nts-1_mkii/waves//../ld/unit.ld -shared --entry=0 -specs=nano.specs -specs=nosys.specs -lc -lm -o /workspace/nts-1_mkii/waves//build/waves.elf
 Creating /workspace/nts-1_mkii/waves//build/waves.hex
 Creating /workspace/nts-1_mkii/waves//build/waves.bin
 Creating /workspace/nts-1_mkii/waves//build/waves.dmp
 Creating /workspace/nts-1_mkii/waves//build/waves.list
 
    text	   data	    bss	    dec	    hex	filename
    5279	    236	    180	   5695	   163f	/workspace/nts-1_mkii/waves//build/waves.elf
 
 Done
 
 >> Installing /workspace/nts-1_mkii/waves
 Making /workspace/nts-1_mkii/waves//build/waves.nts1mkiiunit
 Deploying to /workspace/nts-1_mkii/waves//waves.nts1mkiiunit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-1 development environment.
 ```

##### run_interactiveの代わりに`run_cmd.sh`を使う方法

 1. `run_cmd.sh`のパスを指定し目的のプロジェクトをビルドします (下記は`nts-1_mkii/waves`をビルドする例).

```
 $ ./run_cmd.sh build nts-1_mkii/waves
 >> Initializing NTS-1 mkii development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/nts-1_mkii/waves
 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc
 Linking /workspace/nts-1_mkii/waves//build/waves.elf
 /usr/bin/arm-none-eabi-gcc /workspace/nts-1_mkii/waves//build/obj/header.o /workspace/nts-1_mkii/waves//build/obj/_unit_base.o /workspace/nts-1_mkii/waves//build/obj/unit.o -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -nostartfiles -Wl,-z,max-page-size=128,-Map=/workspace/nts-1_mkii/waves//build/waves.map,--cref,--no-warn-mismatch,--library-path=/workspace/nts-1_mkii/waves//../ld,--script=/workspace/nts-1_mkii/waves//../ld/unit.ld -shared --entry=0 -specs=nano.specs -specs=nosys.specs -lc -lm -o /workspace/nts-1_mkii/waves//build/waves.elf
 Creating /workspace/nts-1_mkii/waves//build/waves.hex
 Creating /workspace/nts-1_mkii/waves//build/waves.bin
 Creating /workspace/nts-1_mkii/waves//build/waves.dmp
 Creating /workspace/nts-1_mkii/waves//build/waves.list
 
    text	   data	    bss	    dec	    hex	filename
    5279	    236	    180	   5695	   163f	/workspace/nts-1_mkii/waves//build/waves.elf
 
 Done
 
 >> Installing /workspace/nts-1_mkii/waves
 Making /workspace/nts-1_mkii/waves//build/waves.nts1mkiiunit
 Deploying to /workspace/nts-1_mkii/waves//waves.nts1mkiiunit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-1 development environment.
 ```

##### ビルドで生成されるファイル

ビルドによって最終的に *.nts1mkiiunit* ファイルがプロジェクトのディレクトリ (ビルドスクリプトで場所を指定しなかった場合)に生成されます.

#### Dockerを使わない（prologue, minilogue XD, 初代NTS-1の環境と同じ）開発環境でのビルド方法

 1. プロジェクトのディレクトリに移動します.
 
```
$ cd logue-sdk/platform/nts-1_mkii/waves/
```
 2. `make`コマンドを実行しプロジェクトをビルドします.

```
$ make
Compiler Options
../../../tools/gcc/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc -c -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -fPIC -std=c11 -fno-exceptions -W -Wall -Wextra -Wa,-alms=build/lst/ -DSTM32H725xE -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7 -D__FPU_PRESENT -I. -I../common -I../../ext/CMSIS/CMSIS/Include

Compiling header.c
Compiling _unit_base.c
Compiling unit.cc
Linking build/waves.elf
../../../tools/gcc/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc build/obj/header.o /home/zncb/Korg/logue-sdk-private/platform/nts-1_mkii/waves//build/obj/_unit_base.o build/obj/unit.o -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -nostartfiles -Wl,-z,max-page-size=128,-Map=build/waves.map,--cref,--no-warn-mismatch,--library-path=../ld,--script=../ld/unit.ld -shared --entry=0 -specs=nano.specs -specs=nosys.specs -lc -lm -o build/waves.elf
Creating build/waves.hex
Creating build/waves.bin
Creating build/waves.dmp

   text	   data	    bss	    dec	    hex	filename
   5259	    236	    180	   5675	   162b build/waves.elf

Creating build/waves.list
Done
```
 3. `make install`を実行し, 最終的なユニットファイルにします.
```
$ make install
Making build/waves.nts1mkiiunit
Deploying to ./waves.nts1mkiiunit
Done
```
 4. *Deploying...* で示された場所に, *.nts1mkiiunit* ファイルが生成されます. このファイルを製品に書き込んで使用します.
 
 *TIP*: 環境変数`INSTALLDIR`を編集することで, インストールディレクトリを変更することができます.

### ユニットのクリーン方法

#### Dockerベースの開発環境の場合

 プロジェクトのクリーンを実行するとビルド成果物と一時ファイルを削除できます.

 1. [docker/run_interactive.sh](../../docker/run_interactive.sh)を実行します.

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
 2. 目的のプロジェクトをクリーンします (`nts-1_mkii/waves`の例) .

```
 user@logue-sdk:~$ build --clean nts-1_mkii/waves 
 >> Initializing NTS-1 mkii development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/nts-1_mkii/waves
 Cleaning
 rm -fR /workspace/nts-1_mkii/waves//.dep /workspace/nts-1_mkii/waves//build /workspace/nts-1_mkii/waves//waves.nts1mkiiunit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-1 development environment.
 ```

##### run_interactiveの代わりに`run_cmd.sh`を使う方法

 1. 目的のプロジェクトをクリーンします (`nts-1_mkii/waves`の例) .

```
 $ ./run_cmd.sh build --clean nts-1_mkii/waves 
 >> Initializing NTS-1 mkii development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/nts-1_mkii/waves
 Cleaning
 rm -fR /workspace/nts-1_mkii/waves//.dep /workspace/nts-1_mkii/waves//build /workspace/nts-1_mkii/waves//waves.nts1mkiiunit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-1 development environment.
 ```

#### Dockerを使わない（prologue, minilogue XD, 初代NTS-1の環境と同じ）開発環境でのクリーン

 1. プロジェクトのディレクトリに移動します.
 
```
$ cd logue-sdk/platform/nts-1_mkii/waves/
```
 2. `make clean`を実行しプロジェクトをクリーンします.

```
$ make clean
Cleaning
rm -fR .dep build waves.nts1mkiiunit
Done
```

### *unit* ファイルを実機に書き込む

ビルドした *.nts1mkiiunit* ファイルはKORG Kontrol Editor(準備中)または新しいloguecli(準備中) を使って [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/jp/products/synthesizers/nts_1_mk2/) にロードできます.

製品にロードされたユニットは, 各モジュール選択時にリストの最後にスロット番号順に表示されます.

*TIP* NTS-1 digital kit mkIIはdev/unit IDを使ってユニットを識別するため, これらを編集してスロットの順序を自由に移動することができます.

## 新規プロジェクトの作成

1. 作りたいモジュール (synth/delfx/revfx/masterfx)のテンプレート・プロジェクトのディレクトリをコピーし, *platform/nts-1_mkii/* において任意の名前を付けます.
2. プロジェクトのビルドをカスタマイズしたい場合は *config.mk* を編集します. 詳細は [config.mk](#configmkファイル) セクションを参照してください.
3. *header.c* テンプレートをプロジェクトに合わせて変更してください. 詳細は[header.c](#headercファイル) セクションを参照してください.
4. *unit.cc* テンプレートで, 作成したコードとユニットAPIを統合します. 詳細は [unit.cc](#unitccファイル) セクションを参照してください.

## プロジェクトの構造

### config.mkファイル

*config.mk* ではMakefileを直接書き換えることなく, プロジェクトのソースファイルやinclude, ライブラリの宣言, 特定のビルドパラメーターのオーバライドをおこなうことができます.

デフォルトでは以下の変数が定義, もしくはすぐに使用できるよう設定されています.

 * `PROJECT` : プロジェクトの名前です. 最終的なビルド成果物のファイル名として使用されます. (Note: 製品にロードしたときに画面に表示される名前ではありません)
 * `PROJECT_TYPE` : ビルドするプロジェクトの種類を決定します. `osc`, `modfx`, `delfx`, `revfx`の4種類のうち, 開発するユニットと同じ種類のものを設定してください.
 * `CSRC` : プロジェクトで使用するC言語のソースファイルのリストです. 少なくとも`header.c`ファイルが含まれている必要があります.
 * `CXXSRC` : プロジェクトで使用するC++のソースファイルのリストです. 少なくとも `unit.cc` ファイルが含まれている必要があります.
 * `UINCDIR` : 追加のインクルードのリストです.
 * `ULIBDIR` : 追加のライブラリのリストです.
 * `ULIBS` : 追加のライブラリフラグのリストです.
 * `UDEFS` : コンパイル時の追加定義のリストです. (記述例: `-DENABLE\_MY\_FEATURE`)

### header.cファイル

*header.c* はprologue, minilogue XD, 初代のNTS-1の *manifest.json* と似た役割を担っており, ユニットとその公開パラメーターに関するいくつかのメタデータ情報を提供します. このファイルはユニットのコードと一緒にコンパイルされ, *.unit_header* という専用のELFセクションに置かれます.

フィールド:

 * `.header_size` : ヘッダー・ストラクチャーのサイズです. テンプレートの定義のままにしてください.
 * `.target` : ターゲット・プラットフォームとモジュールを定義します. テンプレートの定義のままで使用しますが, 定義されているモジュールが実際に作りたいモジュールのタイプと一致していることを確認してください. (osc: k\_unit\_module\_osc, modfx: k\_unit\_module\_modfx, delfx: k\_unit\_module\_delfx, revfx: k\_unit\_module\_revfx)
 * `.api` : ユニットがビルドされる際の Logue SDK APIのバージョンです. テンプレートのデフォルト値では, 現行のAPIが使用されます.
 * `.dev_id` : 開発者を識別するための固有のIDです. ローエンディアンの32ビット符号なし整数で表現されます. 詳細は[開発者ID](#開発者id)のセクションをご参照ください.
 * `.unit_id` : ユニットを識別するための固有のIDです. ローエンディアンの32ビット符号なし整数で表現されます. 同一の`dev_id`の範囲内では, ユニットごとに異なる固有の`unit_id`を設定してください.
 * `.version` : ユニットのバージョンをローエンディアンの32ビット符号なし整数で記述することができます. 上位16ビットにメジャー・バージョン番号を, 下位２バイトにそれぞれマイナー・バージョン番号とパッチ番号を設定できます (例: v1.2.3 -> 0x00010203U).
 * `.name` : ロード時にデバイスに表示されるユニットの名前を指定します. ASCIIのNULL終端配列で最大18文字です. 次の文字を使うことができます: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._`".
 * `.num_params` : ユニットの公開パラメーターの数です. この値はモジュールごとに異なり, common headerの UNIT\_XXX\_MAX\_PARAM\_COUNT に正確な値があります(XXXがターゲットとなるモジュールを示します).
 * `.params` : パラメーター記述子の配列です. 詳細は[パラメーター記述子](#パラメーター記述子)のセクションをご参照ください.
 
### unit.ccファイル
 
 *unit.cc* はlogue SDK APIとのメインのインターフェイスです. API関数へのエントリー・ポイントを提供し, グローバルに定義されたステートメントやクラス・インスタンスを保持します.

### 開発者ID

 開発者はユニットの適切な識別のために, 既に使用されているIDと被らないように, ユニークな開発者ID(32ビット符号なし整数)を設定する必要があります.
 既に設定されているIDのリストは[こちら](../../developer_ids.md)にあります.
 
 *Note* 次の開発者IDは予約済みであり, 使用できません: 0x00000000, 0x4B4F5247 (KORG)と0x6B6F7267 (korg)とこれらの大文字/小文字の組み合わせ

### パラメーター記述子

 パラメーター記述子は [header](#headercファイル) 構造体の一部として定義されます. パラメーターの名前, パラメーターの値の範囲, パラメーターの値の解釈や表示方法の制御といった項目を設定できます.

 ```
 typedef struct unit_param {
  int16_t min;
  int16_t max;
  int16_t center;
  int16_t init;
  uint8_t type;
  uint8_t frac : 4;       // fractional bits / decimals according to frac_mode
  uint8_t frac_mode : 1;  // 0: 固定小数点, 1: 10進数
  uint8_t reserved : 3;
  char name[UNIT_PARAM_NAME_LEN + 1];
 } unit_param_t;
 ```
 
 * `min`と`max` はパラメーターの最小値/最大値を決定します.
 * `center`はパラメーターがバイポーラー（両極性, プラス値とマイナス値, ステレオ・パンのLとRなど）であることを示すために使うことができます. ユニポーラーの場合はシンプルに`min`の値に設定してください.
 * `init`はパラメーターの初期値です. ユニット・パラメーターは初期化段階のあとに`init`の値に設定されることが期待されます.
 * `type`を設定することで, パラメーターを製品のディスプレイに表示するときのフォーマットをコントロールすることができます. 詳細は[パラメーターの型](#パラメーターの型)のセクションを参照してください.
 * `frac`でパラメーターの小数部分を設定できます. この値は`frac_mode`の値に応じて小数ビット数または小数として解釈されます. 
 * `frac_mode`で記述される小数値のタイプを決定します. `0`に設定すると値は固定小数点となり, `frac`で指定されたビットが小数部を表します. `1`に設定すると値は`frac`の10倍を乗じたの小数部分を含むとみなされ,基数10の小数の増減が可能になります.
 * `reserved` は常に0に設定してください.
 * `name`には21文字の名前を付けることができます. 値はNULL終端で7ビットのASCIIフォーマットである必要があり, 次の文字をサポートしています "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._`".
 
 *Note* `min`, `max`, `center` ,`init`の値は`frac`と`frac_mode`の値を考慮して設定する必要があります.

 使いたいパラメーター数が利用できる最大パラメーター数より少ない場合でも, 各パラメーターでパラメーター記述子を設定する必要があります. 使用しないパラメーター・インデックスには以下のパラメーター記述子を記入して埋めてください.
 
 ```
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
 ```
 
##### オシレーター・パラメーターのノブへのマッピング
 
 最初の二つのパラメーターが, NTS-1 digital kit mkIIのAノブとBノブに自動的にマッピングされます.
 残りのパラメーターは, OSCを押しながらTYPEノブを回すとアクセスできるオシレーターのパラメーター・エディット・モードから操作できます.
 
##### モジュレーション・エフェクト・パラメーターのノブへのマッピング

 最初の二つのパラメーターが, NTS-1 digital kit mkIIのAノブとBノブに自動的にマッピングされます.
 残りのパラメーターは, MODを押しながらTYPEノブを回すとアクセスできるモジュレーション・エフェクトのパラメーター・エディット・モードから操作できます. 
 
##### ディレイ・エフェクト・パラメーターのノブへのマッピング

 最初の二つのパラメーターが, NTS-1 digital kit mkIIのAノブとBノブに自動的にマッピングされます.
 三つめのパラメーターはDELボタンを押しながらBノブを回す動作にマッピングされ, 通常はドライ・ウェット・バランスをコントロールします.
 残りのパラメーターは, DELを押しながらTYPEノブを回すとアクセスできるディレイ・エフェクトのパラメーター・エディット・モードから操作できます. 

##### リバーブ・エフェクト・パラメーターのノブへのマッピング
 
 最初の二つのパラメーターが, NTS-1 digital kit mkIIのAノブとBノブに自動的にマッピングされます.
 三つめのパラメーターはREVボタンを押しながらBノブを回す動作にマッピングされ, 通常はドライ・ウェット・バランスをコントロールします.
 残りのパラメーターは, REVを押しながらTYPEノブを回すとアクセスできるリバーブ・エフェクトのパラメーター・エディット・モードから操作できます. 

#### パラメーターの型

 下記のパラメーター・タイプ（型）を利用し, パラメーターを製品のディスプレイに表示するときのフォーマットをコントロールすることができます.
 
 *Note:* DNTS-1 digital kit mkIIの7セグメント・ディスプレイの制限により多くのパラメーターでは表示形式に大きな変化はありませんが, 将来的に利用できるようにパラメーターに対し適切なタイプを選択することを推奨します.

  * `k_unit_param_type_none` : 型を持たない値の記述に用います. 値がそのまま表示されます（端数が考慮されます）.
  * `k_unit_param_type_percent` : パーセント値の記述に用います.
  * `k_unit_param_type_db` : デシベル値の記述に用います. 
  * `k_unit_param_type_cents` : ピッチのセント値の記述に用います. 
  * `k_unit_param_type_semi` : ピッチのセミトーンの記述に用います. 
  * `k_unit_param_type_oct` : ピッチのオクターブ値の記述に用います.
  * `k_unit_param_type_hertz` : ヘルツ値の記述に用います.
  * `k_unit_param_type_khertz` : キロヘルツ値の記述に用います.
  * `k_unit_param_type_bpm` : BPM(Beat Per Minute)の記述に用います.
  * `k_unit_param_type_msec` : ミリ単位の値の記述に用います.
  * `k_unit_param_type_sec` : 秒単位の値の記述に用います.
  * `k_unit_param_type_enum` : 数値列挙値を記述に用います. パラメーター記述子で`min`を0に設定している場合, 表示の際に1増加した値が表示されます.
  * `k_unit_param_type_strings` : ユーザー・カスタムの文字列表現による値の記述に用います. `unit_get_param_str_value(..)`を呼び出して数値を渡すことで, 文字列を取得します. 詳細は[文字列](#文字列) セクションを参照してください.
  * `k_unit_param_type_drywet` : ドライ・ウェットの値の記述に用います. マイナスの値の前にはドライを示す`D`が, プラスの値の前にはウェットを示す`W`の文字が追加され, 値が0になった場合はバランス・ミックスを表す`BALN`の文字に置き換わります.
  * `k_unit_param_type_pan` : ステレオ・パンの値の記述に用います. マイナスの値の前には左を示す`L`, プラスの値には右を示す`R`の文字が追加され, 値が0になった場合はパンニングが中央（センター）であることを示す`CNTR`の文字に置き換わります.
  * `k_unit_param_type_spread` : ステレオの広がりの値の記述に用います. マイナスの値の前には左を示す`L`, プラスの値には右を示す`R`の文字が追加され, 値が0になった場合はステレオの広がりがないことを表す`CNTR`の文字に置き換わります.
  * `k_unit_param_type_onoff`: オン・オフのトグルの記述に用います. `0`が`off`に, `1`が`on`として文字で表示されるようになります.
  * `k_unit_param_type_midi_note` : MIDIノートの値の記述に用います. ノート値を示す数値が, 音名として表示されます(例: `C0`, `A3`).

## ユニットAPIの概要

ここではオシレーターとエフェクトのユニットの概要を説明します.

### Essential Functions

全てのユニットで, 以下の関数の実装が必要です. ただし, 各関数でデフォルトでフォール・バック実装が存在するため, 目的に合った関数のみの実装で十分です.

 * `__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc)` : ユニットがロードされた後に自動的に呼ばれる関数です. ランタイム環境の基本的なチェック, ユニットの初期化, 外部メモリの確保(必要な場合)を実行します. `desc`は現在のランタイム環境の記述子を提供します (詳細は[ランタイム記述子](#ランタイム記述子)のセクションを参照してください).
 * `__unit_callback void unit_teardown()` : ユニットがアンロードされる前に呼ばれます. クリーン・アップやメモリの解放が必要な場合に使用してください.
 * `__unit_callback void unit_reset()` : ユニットをニュートラルな状態にリセットしなければならないケースで呼ばれる関数です. 必要に応じて, アクティブなノートの非アクティブ化, エンヴェロープ・ジェネレーターをニュートラル状態にリセット, オシレーターの位相のリセット, ディレイ・ラインのクリア (一度に全てクリアするのは重い可能性があります) などの処理を実装してください. ただしパラメーターのデフォルト値へのリセットはここでは実施しないでください.
 * `__unit_callback void unit_resume()` : 処理が中断されていたユニットが処理を再開する直前に呼び出される関数です.
 * `__unit_callback void unit_suspend()` : ユニットを切り替えたときなど, ユニットの処理が中断されるときに呼ばれる関数です.通常, そのあとに`unit_reset()`が呼ばれます.
 * `__unit_callback void unit_render(const float * in, float * out, uint32_t frames)` : オーディオ・レンダリングのコールバック関数です. インプット/アウトプット・バッファーの情報は`unit_init(..)`関数の引数`unit_runtime_desc_t`で提供されます.
 * `__unit_callback int32_t unit_get_param_value(uint8_t index)` : 引数で指定されたインデックスのパラメーターの現在の値を取得するために呼び出されます.
 * `__unit_callback const char * unit_get_param_str_value(uint8_t index, int32_t value)` : `k_unit_param_type_strings`型のパラメーターのカスタム文字列を取得するために呼ばれる関数です. 戻り値はNULL終端の7ビットASCII文字列を指すポインターにしてください. このポインターは `unit_get_param_str_value(..)`が再び呼び出されるまでキャッシュ/再利用されることはありません.
 * `__unit_callback void unit_set_param_value(uint8_t index, int32_t value)` : 引数で指定されたインデックスのパラメーターの現在の値を設定するために呼ばれる関数です. NTS-1 digital kit mkIIでは値は16ビット整数として保存されますが, 将来的なAPIの互換性を担保するため32ビット整数として値が渡されていることに注意してください. 安全性を高めるため, ヘッダーで宣言したmin/maxの値に従って, 値のチェックと丸め込みを実施してください.
 * `__unit_callback void unit_set_tempo(uint32_t tempo)` : テンポが変更された時に呼び出される関数です. テンポは固定小数点でフォーマットされ, 上位16ビットが整数部分, 下位16ビット（ローエンディアン）が小数部分になります. この関数は外部デバイスとテンポ同期しているときに頻繁に呼び出される可能性があるため, テンポ変更を処理する際にはCPU負荷を可能な限り低く保つように注意してください.
 * `__unit_callback void unit_tempo_4ppqn_tick_func(uint32_t counter)` : 初期化後, 4PPQNインターバル（16分音符）のクロック・イベントをユニットに通知するためにいつでも呼び出すことができる関数です.
 
### オシレーター・ユニット専用の関数
 
 * `__unit_callback void unit_note_on(uint8_t note, uint8_t velocity)` : MIDIノートオンイベントで呼ばれます. また明示的に`unit_gate_on(..)`関数を無効にしている場合は内部シーケンサーのゲートオンイベントで呼び出されます（その場合ノートは0xFFUに設定されます）.
 * `__unit_callback void unit_note_off(uint8_t note)` : MIDIノートオフイベントで呼ばれます. また明示的に`unit_gate_off(..)`ハンドラを無効にしている場合は内部シーケンサーのゲートオフイベントで呼び出されます（その場合ノートは0xFFUに設定されます）.
 * `__unit_callback void unit_all_note_off(void)` : すべてのアクティブなノートを非アクティブにし, エンベロープ・ジェネレーターをリセットする関数です.
 * `__unit_callback void unit_pitch_bend(uint16_t bend)` : MIDIピッチベンドイベントで呼び出される関数です. `bend`は0x2000Uを中心とする14ビットの値です. 感度はユニットが必要とする数値に定義できます.
 * `__unit_callback void unit_channel_pressure(uint8_t pressure)` : MIDIチャンネルのプレッシャー・イベントで呼ばれる関数です. `pressure`は7ビットです.
 * `__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch)` : MIDIアフター・タッチイベントで呼ばれる関数です. `afterotuch`は7ビットです.
 
### ランタイム記述子

 ランタイム記述子への参照は初期化段階でユニットに渡されます. この記述子は, 現在のデバイス, API, オーディオ・レート, バッファに関する情報と, 呼び出し可能なAPI巻子へのポインタを提供します. 

 ```
 typedef struct unit_runtime_desc {
  uint16_t target;
  uint32_t api;
  uint32_t samplerate;
  uint16_t frames_per_buffer;
  uint8_t input_channels;
  uint8_t output_channels;
  unit_runtime_hooks_t hooks;
 } unit_runtime_desc_t;
 ```
 
 * `target` : 現在のプラットフォームとモジュールを記述します. `k_unit_target_nts1_mkii_osc`, `k_unit_target_nts1_mkii_modfx`, `k_unit_target_nts1_mkii_delfx`, `k_unit_target_nts1_mkii_revfx`のいずれかに設定してください. マクロ`UNIT_TARGET_PLATFORM_IS_COMPAT(target)`を使うと現在のユニットと実行環境の互換性をチェックできます.
 * `api` : 現在使用されているAPIのバージョンを記述します. バージョンは上位16ビットにメジャー番号, 下位2バイトにそれぞれマイナー番号とパッチ番号が格納されます (例: v1.2.3 -> 0x00010203U). マクロ`UNIT_API_IS_COMPAT(api)`を使うと現在のユニットとランタイム環境APIの互換性をチェックできます.
 * `samplerate` : オーディオ処理に適用するサンプルレートを指定します. NTS-1 digital kit mkIIでは常に48000であるべきですが, 将来的な互換性のためユニットによってチェックすることを推奨します.  ユニットは`unit_init(..)`関数から `k_unit_err_samplerate*`を返すことで, 不都合なサンプルレートを拒否することができます.
 * `frames_per_buffer` : `unit_render(..)`関数での, オーディオ処理バッファの最大フレーム数を記述します. 一般的には関数の`frames`引数と等しくなければいけませんが, それより小さい値のサポートも検討してください.
 * `input_channels` : `unit_render(..)`関数の入力バッファにある, 1フレーム当たりのオーディオのチャンネル数(サンプル数)を記述します.
 * `output_channels` : `unit_render(..)`関数の出力バッファにある, 1フレーム当たりのオーディオのチャンネル数(サンプル数)を記述します.
 * `hooks` はランタイム環境が提供する追加のコンテキストと呼び出し可能なAPIへのアクセスを提供します. 詳細は[ランタイム・コンテキスト](#ランタイム・コンテキスト)と[呼び出し可能なAPI関数](#呼び出し可能なapi関数)のセクションを参照してください.

#### ランタイム・コンテキスト

オシレーター・ランタイムはオシレーターにリアルタイムの情報を提供します.

```
  /** Oscillator specific unit runtime context. */
  typedef struct unit_runtime_osc_context {
    int32_t  shape_lfo;
    uint16_t pitch;	// 0x0000 ~ 0x9000?
    uint16_t cutoff;	// 0x0000 ~ 0x1fff
    uint16_t resonance;	// 0x0000 ~ 0x1fff	
    uint8_t  amp_eg_phase;
    uint8_t  amp_eg_state:3;
    uint8_t  padding0:5;
    uint8_t  padding1[4];
  } unit_runtime_osc_context_t;
```

他のモジュールのランタイムは, オシレーターのような特定のリアルタイム情報を提供しません.

#### 呼び出し可能なAPI関数

 * `sdram_alloc` (`uint8_t *sdram_alloc(size_t size)`) この関数は外部メモリのバッファを確保します. この割り当ては初期化段階で行ってください. バッファのサイズは4バイト・アラインメントを推奨します.
 * `sdram_free` (`void sdram_free(const uint8_t *mem)`) この関数は`sdram_alloc`によって割り当てられた外部メモリのバッファを解放します.
 * `sdram_avail` (`size_t sdram_avail()`) この関数は現在利用可能な外部メモリのサイズを返します.

### 文字列

 `unit_get_param_str_value(..)`関数で扱う文字列は, NULL終端の7ビットASCII文字である必要があり, 次の文字がサポートされています: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._`".
 
