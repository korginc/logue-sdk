## microKORG2 SDK Source and Template Projects

[English](./README.md)

### 概要

このディレクトリに [microKORG2](https://www.korg.com/jp/products/synthesizers/microkorg2/) で使用できるカスタム・オシレーターやエフェクトのビルドに必要なすべてのファイルが揃っています.

#### 必要なソフトウェアバージョン

SDK version 2.0.0でビルドされたユーザーユニットを実行するために, 製品本体にバージョン2.1.0以降のファームウェアがインストールされている必要があります.

#### このディレクトリの構造:

 * [common/](common/) : 共通のヘッダーファイル.
 * [ld/](ld/) : 共通のリンカーファイル.
 * [dummy-osc/](dummy-osc/) : オシレーターのプロジェクトのテンプレート.
 * [dummy-modfx/](dummy-modfx/) : モジュレーション・エフェクトのプロジェクトのテンプレート.
 * [dummy-delfx/](dummy-delfx/) : ディレイ・エフェクトのプロジェクトのテンプレート.
 * [dummy-revfx/](dummy-revfx/) : リバーブ・エフェクトのプロジェクトのテンプレート.
 * [MorphEQ/](MorphEQ/) : 3つのバンドを同時に制御できる3バンドEQ.
 * [Vibrato/](Vibrato/) : いくつかのおまけ機能をもつシンプルなビブラートプラグイン.
 * [MultitapDelay/](MultitapDelay/) : ステレオ・モードとピンポン・モードを切り替えられるマルチタップ・ディレイ.
 * [breveR/](breveR/) : Schroederの論文に基づいたリバーブ. プリディレイ・ラインをリバースするオプションがあります.
 * [waves/](waves/) : シンプルなウェーブテーブル・オシレーター.
 * [vox/](vox/) : ボーカル・フォルマント・オシレーター.

### 製品の技術仕様

* *CPU*: ARM Cortex-A7 (i.mx6ulz)
* *Unit Format*: ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamically linked

#### 対応モジュール

| モジュール | スロット数 | 最大ユニットサイズ | 最大RAM読み込みサイズ | 利用できる外部メモリのサイズ |
|--------|-------|---------------|-------------------|-----------------------------|
| osc    | 32    | ~ 48KB        | 48KB              | 8KB                         |
| modfx  | 32    | ~ 16KB        | 16KB              | 64KB                        |
| delfx  | 32    | ~ 24KB        | 24KB              | 1MB                         |
| revfx  | 32    | ~ 24KB        | 24KB              | 1MB                         |

### 開発環境の構築

#### Dockerベースのビルド環境

 [Docker-based Build Environment](../../docker/README_ja.md)を参照してください.

#### Dockerベースの開発環境でのビルド

 1. [docker/run_interactive.sh](../../docker/run_interactive.sh) コマンドを実行します.

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```

 1.1. (オプション) ビルド可能なプロジェクトの一覧を表示することができます.

```
 user@logue-sdk $ build -l --microkorg2
- microkorg2/MorphEQ
- microkorg2/MultitapDelay
- microkorg2/ReverseReverb
- microkorg2/Vibrato
- microkorg2/dummy-delfx
- microkorg2/dummy-modfx
- microkorg2/dummy-osc
- microkorg2/dummy-revfx
- microkorg2/vox
- microkorg2/waves
 ```

 2. ビルドしたいプロジェクトのパスを指定しbuildコマンドを実行します (下記は`microkorg2/dummy-modfx`をビルドする例).

```
 user@logue-sdk $ build microkorg2/dummy-synth
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Building /workspace/microkorg2/dummy-modfx
Compiler Options
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -std=c11 -W -Wall -Wextra -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.c -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fconcepts -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -fno-threadsafe-statics -std=gnu++14 -W -Wall -Wextra -Wno-ignored-qualifiers -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.cc -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ xxxx.o -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -shared -Wl,-Map=build/dummy.map,--cref -lm -lc -o build/dummy.elf

Compiling header.c
Compiling _unit_base.c
Compiling unit.cc

Linking build/dummy.mk2unit
Stripping build/dummy.mk2unit
Creating build/dummy.hex
Creating build/dummy.bin
Creating build/dummy.dmp
Creating build/dummy.list


Done

   text	   data	    bss	    dec	    hex	filename
   4587	    352	     80	   5019	   139b	build/dummy.mk2unit
>> Installing /workspace/microkorg2/dummy-modfx
Deploying to /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
 ```

##### run_interactiveの代わりに`run_cmd.sh`を使う方法

 1. `run_cmd.sh`のパスを指定し目的のプロジェクトをビルドします (下記は`microkorg2/dummy-modfx`をビルドする例).

```
 $ ./run_cmd.sh build microkorg2/waves
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Building /workspace/microkorg2/dummy-modfx
Compiler Options
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -std=c11 -W -Wall -Wextra -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.c -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fconcepts -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -fno-threadsafe-statics -std=gnu++14 -W -Wall -Wextra -Wno-ignored-qualifiers -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.cc -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ xxxx.o -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -shared -Wl,-Map=build/dummy.map,--cref -lm -lc -o build/dummy.elf

Compiling header.c
Compiling _unit_base.c
Compiling unit.cc

Linking build/dummy.mk2unit
Stripping build/dummy.mk2unit
Creating build/dummy.hex
Creating build/dummy.bin
Creating build/dummy.dmp
Creating build/dummy.list


Done

   text	   data	    bss	    dec	    hex	filename
   4587	    352	     80	   5019	   139b	build/dummy.mk2unit
>> Installing /workspace/microkorg2/dummy-modfx
Deploying to /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
```

##### ビルドで生成されるファイル

ビルドによって最終的に *.mk2unit* ファイルがプロジェクトのディレクトリ (ビルドスクリプトで場所を指定しなかった場合)に生成されます.

### ユニットのクリーン方法

#### Dockerベースの開発環境の場合

 プロジェクトのクリーンを実行するとビルド成果物と一時ファイルを削除できます.

 1. [docker/run_interactive.sh](../../docker/run_interactive.sh)を実行します.

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
 2. 目的のプロジェクトをクリーンします (`microkorg2/dummy-modfx`の例) .

```
 user@logue-sdk:~$ build --clean microkorg2/dummy-modfx 
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Cleaning /workspace/microkorg2/dummy-modfx

Cleaning
rm -fR .dep build /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
 ```

##### run_interactiveの代わりに`run_cmd.sh`を使う方法

 1. 目的のプロジェクトをクリーンします (`microkorg2/dummy-modfx`の例) .

```
 $ ./run_cmd.sh build --clean microkorg2/dummy-modfx 
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Cleaning /workspace/microkorg2/dummy-modfx

Cleaning
rm -fR .dep build /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
 ```

### *unit* ファイルを実機に書き込む

ビルドした *.mk2unit* ファイルはUSB Mass Storage、microkorg2 librarian(近日公開予定)または新しいloguecli(近日公開予定) を使って [microKORG2](https://www.korg.com/jp/products/synthesizers/microkorg2) にロードできます.

製品にロードされたユニットは, 各モジュール選択時にリストの最後にスロット番号順に表示されます.

*TIP* microkorg2はdev/unit IDを使ってユニットを識別するため, これらを編集してスロットの順序を自由に移動することができます.

## 新規プロジェクトの作成

1. 作りたいモジュール (synth/delfx/revfx/masterfx)のテンプレート・プロジェクトのディレクトリをコピーし, *platform/microkorg2/* において任意の名前を付けます.
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
 * `.name` : ロード時にデバイスに表示されるユニットの名前を指定します. ASCIIのNULL終端配列で最大8文字です. 次の文字を使うことができます: "`` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._!"#$%&'()*+,/:;<=>?@[]^`~``".
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
 * `name`には8文字の名前を付けることができます. 値はNULL終端で7ビットのASCIIフォーマットである必要があり, 次の文字をサポートしています "`` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._!"#$%&'()*+,/:;<=>?@[]^`~``".
 
 *Note* `min`, `max`, `center` ,`init`の値は`frac`と`frac_mode`の値を考慮して設定する必要があります.

 使いたいパラメーター数が利用できる最大パラメーター数より少ない場合でも, 各パラメーターでパラメーター記述子を設定する必要があります. 使用しないパラメーター・インデックスには以下のパラメーター記述子を記入して埋めてください.
 
 ```
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
 ```
 
##### オシレーター・パラメーターのノブへのマッピング
 
   OSC1/2/3ページからは最大13個のパラメータにアクセスできます. パラメータは左から右へ, ページ1～3の順に割り当てられています.

### オシレーター・モジュレーション

オシレーターはフレームごとに10個のモジュレーションソースのバッファを受け取ります。各モジュレーションソースには、最大8ボイス（現在のポリフォニーによって異なります）の個別のモジュレーション値が含まれます。これらの値には、ヘルパー関数「GetModDepth」および「GetModData」を使用してアクセスできます。モジュレーションデータにアクセスするには、「unit_platform_exclusive」関数のcaseとして「kMk2PlatformExclusiveModData」を追加します。仮想パッチでカスタムモジュレーションデスティネーションの名前を表示するには、「unit_platform_exclusive」関数のcaseとして「kMk2PlatformExclusiveModDestName」を追加し、「GetModDestNameData」によって提供されるバッファにその名前を書き込みます。例については、[vox](vox/vox.h) または [vox](waves/waves.h) を参照してください。

##### エフェクター・パラメーター・挙動

根本的にでは、エフェクトパラメータは内部エフェクトの動作と一致します。つまり、エフェクトを変更すると、ページ1の現在のパラメータがターゲットエフェクトに適用されますが、ページ2のパラメータは完全に独立しています。また、ページ1の3つのパラメータは、バーチャルパッチのモジュレーションデスティネーションとして使用できます。これらの動作は、ヘッダー内の予約パラメータの設定を変更することで制御できます。詳細は以下のテーブル通りです。

|                    Enum                      | Value |   Behavior   |
|----------------------------------------------|-------|--------------|
| kMk2FxParamModeBasic                         |     0 | 内部エフェクトと同様。ページ1のパラメータ値は以前に選択したエフェクトからコピーされ、パラメータはバーチャルパッチを介してモジュレーション可能. |
| kMk2FxParamModeIgnoreKnobState               |     1 | ページ 1 のパラメータ値は、以前に選択した fx からコピーされず、パラメータは仮想パッチを介して調整可能です。 |
| kMk2FxParamModeIgnoreModulation              |     2 | ページ 1 のパラメータ値は以前に選択した fx からコピーされ、パラメータは仮想パッチ経由ではモジュレートできません。 |
| kMk2FxParamModeIgnoreKnobStateAndModulation  |     3 | ページ 1 のパラメータ値は、以前に選択した fx からコピーされません。また、パラメータは仮想パッチ経由ではモジュレートできません。 |

##### モジュレーション・エフェクト・パラメーターのノブへのマッピング

Mod fxのMODページとEXTRAページでは最大8つのパラメータにアクセスできます. MODの3つのパラメータはバーチャル・パッチを介してモジュレーション可能で, パラメータ構造体の「reserved」ビットで特に指定しない限り, 以前に選択したエフェクトのパラメータ値を引き継ぎます（内蔵エフェクトと同じ動作）. `FxDefines.h`のkMk2FxParamModeXX列挙型を使用すると, この設定をより分かりやすくすることができます.
 
##### ディレイ・エフェクト・パラメーターのノブへのマッピング

Delay fxのDELAYページとEXTRAページでは最大8つのパラメータにアクセスできます. DELAYの3つのパラメータはバーチャルパッチを介してモジュレーション可能で, パラメータ構造体の「reserved」ビットで特に指定しない限り, 以前に選択したエフェクトのパラメータ値を引き継ぎます（内蔵エフェクトと同じ動作）. `FxDefines.h`のkMk2FxParamModeXX列挙型を使用すると, この設定をより分かりやすくすることができます.

##### リバーブ・エフェクト・パラメーターのノブへのマッピング
 
Reverb fxのREVERBページとEXTRAページでは最大8つのパラメータにアクセスできます. REVERBの3つのパラメータはバーチャルパッチを介してモジュレーション可能で, パラメータ構造体の「reserved」ビットで特に指定しない限り, 以前に選択したエフェクトのパラメータ値を引き継ぎます（内蔵エフェクトと同じ動作）. `FxDefines.h`のkMk2FxParamModeXX列挙型を使用すると、この設定をより分かりやすくすることができます.

#### パラメーターの型

 下記のパラメーター・タイプ（型）を利用し, パラメーターを製品のディスプレイに表示するときのフォーマットをコントロールすることができます.
 
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
 * `__unit_callback void unit_set_param_value(uint8_t index, int32_t value)` : 引数で指定されたインデックスのパラメーターの現在の値を設定するために呼ばれる関数です. microKORG2では値は16ビット整数として保存されますが, 将来的なAPIの互換性を担保するため32ビット整数として値が渡されていることに注意してください. 安全性を高めるため, ヘッダーで宣言したmin/maxの値に従って, 値のチェックと丸め込みを実施してください.
 * `__unit_callback void unit_set_tempo(uint32_t tempo)` : テンポが変更された時に呼び出される関数です. テンポは固定小数点でフォーマットされ, 上位16ビットが整数部分, 下位16ビット（ローエンディアン）が小数部分になります. この関数は外部デバイスとテンポ同期しているときに頻繁に呼び出される可能性があるため, テンポ変更を処理する際にはCPU負荷を可能な限り低く保つように注意してください.
 
### microKORG2で使用可能な関数（オプション）
 * `__unit_callback void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize)` : オシレーター・ユニットのバーチャル・パッチ・モジュレーションを含むmicroKORG2特有の関数です.
 
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
 * `samplerate` : オーディオ処理に適用するサンプルレートを指定します. microKORG2では常に48000であるべきですが, 将来的な互換性のためユニットによってチェックすることを推奨します.  ユニットは`unit_init(..)`関数から `k_unit_err_samplerate*`を返すことで, 不都合なサンプルレートを拒否することができます.
 * `frames_per_buffer` : `unit_render(..)`関数での, オーディオ処理バッファの最大フレーム数を記述します. 一般的には関数の`frames`引数と等しくなければいけませんが, それより小さい値のサポートも検討してください.
 * `input_channels` : `unit_render(..)`関数の入力バッファにある, 1フレーム当たりのオーディオのチャンネル数(サンプル数)を記述します.
 * `output_channels` : `unit_render(..)`関数の出力バッファにある, 1フレーム当たりのオーディオのチャンネル数(サンプル数)を記述します.
 * `hooks` はランタイム環境が提供する追加のコンテキストと呼び出し可能なAPIへのアクセスを提供します. 詳細は[ランタイム・コンテキスト](#ランタイム・コンテキスト)と[呼び出し可能なAPI関数](#呼び出し可能なapi関数)のセクションを参照してください.

#### ランタイム・コンテキスト

オシレーター・ランタイムはオシレーターにリアルタイムの情報を提供します.

```
  /** Oscillator specific unit runtime context. */
  typedef struct unit_runtime_osc_context 
  {
    float pitch[kMk2MaxVoices]; 
    uint8_t trigger; // bit array
    float * unitModDataPlus; 
    float * unitModDataPlusMinus; 
    uint8_t modDataSize;
    uint16_t bufferOffset;
    uint8_t voiceOffset;
    uint8_t voiceLimit;
    uint16_t outputStride;
  } unit_runtime_osc_context_t;
```

 * `pitch[kMk2MaxVoices]` : 対応する音声のピッチを定義する8つのfloatの配列です.
 * `trigger` : 対応する音声がトリガーされたときに各ビットがHi(1)に設定されるビットパターンです.
 * `unitModDataPlus` : ユーザーがボイスごとの変調信号を書き込むためのオプションの変調バッファです. 0～1に正規化されます.
 * `unitModDataPlusMinus` : ユーザーがボイスごとの変調信号を書き込むためのオプションの変調バッファです. -1～1に正規化されます.
 * `modDataSize` : unitModDataバッファのサイズです.
 * `bufferOffset` : オシレーター出力バッファの開始からのオフセットです.
 * `voiceOffset` : このユニットの開始ボイスに対するオフセットです.
 * `voiceLimit` : このユニットが許可する最大ボイス数です.
 * `outputStride` :　インターレースされる出力ストリームの数です.

他のモジュールのランタイムは, オシレーターのような特定のリアルタイム情報を提供しません.

#### 呼び出し可能なAPI関数

 * `sdram_alloc` (`uint8_t *sdram_alloc(size_t size)`) この関数は外部メモリのバッファを確保します. この割り当ては初期化段階で行ってください. バッファのサイズは4バイト・アラインメントを推奨します.
 * `sdram_free` (`void sdram_free(const uint8_t *mem)`) この関数は`sdram_alloc`によって割り当てられた外部メモリのバッファを解放します.
 * `sdram_avail` (`size_t sdram_avail()`) この関数は現在利用可能な外部メモリのサイズを返します.

### 文字列

 `unit_get_param_str_value(..)`関数で扱う文字列は, NULL終端の7ビットASCII文字である必要があり, 次の文字がサポートされています: "`` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._!"#$%&'()*+,/:;<=>?@[]^`~``".
 


