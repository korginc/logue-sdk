## drumlogue SDK ソースコードとテンプレートプロジェクト

### 概要

下記ディレクトリに [drumlogue](https://www.korg.com/jp/products/drums/drumlogue/) で使用できるシンセやエフェクトのビルドに必要なファイルが全て揃っています.

#### 互換性に関して

SDK version 2.0-0 でビルドされた user units (ビルド済みのカスタムコードバイナリ）を実行するためには, version 1.00以降のファームウェアが製品本体にインストールされている必要があります.

#### 全体の構造:
 * [common/](common/) : 共通のヘッダファイル.
 * [dummy-synth/](dummy-synth/) : 自作シンセのテンプレートプロジェクト.
 * [dummy-delfx/](dummy-delfx/) : 自作ディレイ・エフェクトのテンプレートプロジェクト.
 * [dummy-revfx/](dummy-revfx/) : 自作リバーブ・エフェクトのテンプレートプロジェクト.
 * [dummy-masterfx/](dummy-masterfx/) : 自作マスター・エフェクトのテンプレートプロジェクト.

### 開発環境の設定 (Dockerのみ)

 [Docker-based Build Environment](../../docker) をご参照ください.

### ユニットのビルド

 1. [docker/run_interactive.sh](../../docker/run_interactive.sh) を実行します.

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
1.1. (オプション) ビルド可能なプロジェクトを表示したい場合は下記コマンドを実行します.

```
 user@logue-sdk $ build -l --drumlogue
 - drumlogue/dummy-delfx
 - drumlogue/dummy-masterfx
 - drumlogue/dummy-revfx
 - drumlogue/dummy-synth
 ```

 2. ビルドしたいプロジェクトのパスを指定し, ビルドコマンドを実行します (下記は `drumlogue/dummy-synth` をビルドする例です).

```
 user@logue-sdk $ build drumlogue/dummy-synth
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/drumlogue/dummy-synth
 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc

 Linking build/dummy_synth.drmlgunit
 Stripping build/dummy_synth.drmlgunit
 Creating build/dummy_synth.hex
 Creating build/dummy_synth.bin
 Creating build/dummy_synth.dmp
 Creating build/dummy_synth.list
 
 
 Done
 
    text	   data	    bss	    dec	    hex	filename
    3267	    316	      8	   3591	    e07	build/dummy_synth.drmlgunit
 >> Installing /workspace/drumlogue/dummy-synth
 Deploying to /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 ```

#### `run_cmd.sh` での代替

 1. ビルドしたいプロジェクトのパスを指定し, ビルドコマンドを実行します (下記は `drumlogue/dummy-synth` をビルドする例です).

```
 $ ./run_cmd.sh build drumlogue/dummy-synth
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/drumlogue/dummy-synth
 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc

 Linking build/dummy_synth.drmlgunit
 Stripping build/dummy_synth.drmlgunit
 Creating build/dummy_synth.hex
 Creating build/dummy_synth.bin
 Creating build/dummy_synth.dmp
 Creating build/dummy_synth.list
 
 
 Done
 
    text	   data	    bss	    dec	    hex	filename
    3267	    316	      8	   3591	    e07	build/dummy_synth.drmlgunit
 >> Installing /workspace/drumlogue/dummy-synth
 Deploying to /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 ```

#### ビルドの成果物

最終的な成果物として *.drmlgunit* がプロジェクトディレクトリ (ビルドスクリプトでインストール場所が指定されている場合を除きます) に生成されます.

### ユニットのクリーニング

ユニットのクリーニングを実行すると, ビルドで生成されたファイル群を取り除くことができます.

 1. [docker/run_interactive.sh](../../docker/run_interactive.sh) を実行します.

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
 2. プロジェクトを指定し, クリーンします (下記は `drumlogue/dummy-synth` をクリーンする例です) .

```
 user@logue-sdk:~$ build --clean drumlogue/dummy-synth 
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/drumlogue/dummy-synth
 
 Cleaning
 rm -fR .dep build /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 ```

#### `run_cmd.sh` での代替

 1. プロジェクトを指定し, クリーンします (下記は `drumlogue/dummy-synth` をビルドする例です) .

```
 $ ./run_cmd.sh build --clean drumlogue/dummy-synth 
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/drumlogue/dummy-synth
 
 Cleaning
 rm -fR .dep build /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 ```

### 「Unit」ファイルの操作と使い方

*.drmlgunit* ファイルは[drumlogue](https://www.korg.com/products/drums/drumlogue) をUSBマスストレージデバイス・モードで起動し, *Units/* の下の適切なディレクトにファイルを置くことで[drumlogue](https://www.korg.com/products/drums/drumlogue) に読み込ませることができます. ユーザーシンセ, ディレイ, リバーブ, マスター・エフェクトはそれぞれ *Units/Synths/*, *Units/DelayFXs*, *Units/ReverbFXs/*, *Units/MasterFXs* に配置する必要があります.
ユニットはデバイスを再起動するとアルファベット順に読み込まれます.

ユニットのファイル名の先頭に番号をつけることで, ユニットのロード順を強制することができます
(例: *01_my_unit.drmlgunit*).

## 新しいプロジェクトを作る

1. 作成したいコンテンツ (synth/delfx/revfx/masterfx) のテンプレートプロジェクトをコピーし, *platform/drumlogue/* ディレクトリ内で好きな名前に変更してください.
2. プロジェクトのビルドをカスタマイズする場合は *config.mk* を編集します. 詳細な情報は下記 [config.mk](#configmk-ファイル) のセクションにあります.
3. *header.c* テンプレートをプロジェクトに合わせて変更してください. 詳細な情報は下記 [header.c](#headerc-ファイル) のセクションにあります.
4. *unit.cc* テンプレートを用いて, 作成したコードとユニットAPIを統合します. 詳細な情報は下記 [unit.cc](#unitcc-ファイル) のセクションにあります.

## プロジェクトの構造

### config.mk ファイル

*config.mk* ファイルは *Makefile* を直接編集することなく, プロジェクトのソースファイルやInclude, ライブラリの宣言, 特定のビルドパラメータのオーバーライドを可能にします.

デフォルトでは以下の変数が定義 (もしくは設定可能) されています.

 * `PROJECT` : プロジェクト名. 最終的なビルドプロジェクトのファイル名として使用されます. (Note: ロード時にdrumlogueに表示されるユニット名ではありません)
 * `PROJECT_TYPE` : ビルドするプロジェクトの種類を決定します. `synth`, `delfx`, `revfx`, `masterfx` の4種類の中から設定可能です. 開発するユニットの種類と一致させてください.
 * `CSRC` : プロジェクトで使用するCのソースファイル. このリストには `header.c` ファイルが含まれている必要があります.
 * `CXXSRC` : プロジェクトで使用するC++のソースファイル. このリストには `unit.cc` ファイルが含まれている必要があります.
 * `UINCDIR` : 追加のインクルードのリスト.
 * `ULIBDIR` : 追加のライブラリのリスト.
 * `ULIBS` : 追加のライブラリフラグのリスト.
 * `UDEFS` : コンパイル時の追加定義のリスト. (例: `-DENABLE\_MY\_FEATURE`)

### header.c ファイル

*header.c* ファイルは他の機種における *manifest.json* ファイルに似た構造をしており, ユニットとそのパラメータに関するいくつかのメタデータを提供します. このファイルはユニットのコードと一緒にコンパイルされ, *.unit_header* という専用のELFセクションに置かれるため, objdumpもしくはreadelfツールを使って抽出することができます.

要素の説明:

 * `.header_size` : ヘッダ構造体のサイズ. 各テンプレートで定義されている通りにしておく必要があります.
 * `.target` : ターゲットプラットフォームとモジュールを定義します. 各テンプレートの値を保持しますが, 定義されているモジュールが意図するターゲットユニットモジュールと一致しているか確認してください (synth: k\_unit\_module\_synth, delfx: k\_unit\_module\_delfx, revfx: k\_unit\_module\_revfx, masterfx: k\_unit\_module\_masterfx).
 * `.api` : ビルド時のlogue SDKのAPIバージョン. デフォルトの値はビルドにそのバージョンが使用されることを保証します.
 * `.dev_id` : 開発者ID（ローエンディアン, 32bit符号なし整数）. 詳細は下記 [開発者ID（dev_id）](#開発者id) のセクションをご参照ください.
 * `.unit_id` : ユニットID（ローエンディアン, 32bit符号なし整数）. 同一の開発者IDの範囲内では, unit_idは固有の値である必要があります.
 * `.version` : ユニットのバージョン情報（ローエンディアン, 32bit符号なし整数）. 上位16ビットにメジャー, 下位2バイトにマイナーとパッチ番号を指定します (例: v1.2.3 -> 0x00010203U).
 * `.name` : ユニットの名前. ロード時にデバイス上に表示される名前です. 最大13文字の7ビットASCIIからなるヌル終端文字列です. 次の文字が使用できます: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?#$%&'()*+,-.:;<=>@`".
 * `.num_presets` : ユニットが公開するプリセットの数. 詳細は下記 [プリセット](#プリセット) のセクションをご参照ください.
 * `.num_params` : ユニットが公開するパラメータの数. 最大24のパラメータを公開できます.
 * `.params` : パラメータ記述子の配列. 詳細は [パラメータ記述子](#パラメータ記述子) のセクションをご参照ください.
 
### unit.cc ファイル
 
*unit.cc* ファイルはlogue SDK APIのメインとなるインターフェイスで, 必要なAPI関数のエントリポイント実装を提供し、グローバルに定義された状態, and/orクラスインスタンスを持ちます.

### 開発者ID

 開発者はユニットを適切に識別できるようにするために、固有のID（32bit符号なし整数）を選択する必要があります.
 既にあるIDのリストが[こちら](../../developer_ids.md) にあります（完全なものではありません）.
 
 *Note* 次の開発者IDは予約されているため使用しないでください: 0x00000000, 0x4B4F5247 (KORG), 0x6B6F7267 (korg), 前者ふたつの大文字/小文字の組み合わせ.

### パラメータ記述子

パラメータ記述子は [header](#headerc-ファイル) 構造体の一部として定義されます. パラメータに名前を付け, その値のレンジを記述し, パラメータの値の解釈と実機での表示を制御できます.

 ```
 typedef struct unit_param {
  int16_t min;
  int16_t max;
  int16_t center;
  int16_t init;
  uint8_t type;
  uint8_t frac : 4;       // fractional bits / decimals according to frac_mode
  uint8_t frac_mode : 1;  // 0: fixed point, 1: decimal
  uint8_t reserved : 3;
  char name[UNIT_PARAM_NAME_LEN + 1];
 } unit_param_t;
 ```
 
 * `min` と `max` はパラメータ値の範囲を定義します.
 * `center` を使用すると, パラメータがバイポーラ（双極性）であることを明示的に設定できます. ユニポーラのパラメータでは `min` を設定するだけです.
 * `init` はパラメータの初期値です. ユニットパラメータは初期化のあと, この値に設定されることが期待されます.
 * `type` はパラメータの値の表示方法を設定できます, 詳細は [Parameter Types](#parameter-types) をご参照ください.
 * `frac` はパラメータ値の小数部分を指定します. この値は `frac_mode` の値に応じて, 少数部のビット数または10進数として解釈されます.
 * `frac_mode` は記述される小数のタイプを設定します. `0` に設定した場合は固有小数点とみなされ, `frac` ビットが小数部分であることを表します. `1` に設定した場合は, 値が小数部分を含むと仮定し, `frac` を10回かけたものになり, 1/10の増減ができるようになります.
 * `reserved` は常に0に設定する必要があります.
 * `name` には12文字の名前を指定できます（7ビットASCIIのヌル終端文字である必要があります）. 次の文字を使用することができます: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?#$%&'()*+,-.:;<=>@`".
 
 *Note* `min`, `max`, `center`, `init` の値は `frac` と `frac_mode` の値を考慮し適切に設定する必要があります.

 使用するパラメータ数が最大許容数より少ない場合でも, 24個の各パラメータに記述子を用意する必要があります. パラメータインデックスが使用中でないことを示すために以下のパラメータ記述子を指定してください.
 
 ```
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
 ```
 
 drumlogueでは24個のパラメータが, パラメータインデックスの順番に従って4個ずつページにレイアウトされています. あるパラメータを後ろのページで表示するためにいくつかのパラメータインデックスを未アサインにすることができます. そのためには, インデックスをスキップする未アサインのパラメータを導入します.
 
 ```
 // Page 1
 {0, (100 << 1), 0, (25 << 1), k_unit_param_type_percent, 1, 0, 0, {"PARAM1"}},
 {-5, 5, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PARAM2"}},
 {-100, 100, 0, 0, k_unit_param_type_pan, 0, 0, 0, {"PARAM3"}},
 // blank parameter, leaves that parameter slot blank in UI, and align next parameter to next page
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

 // Page 2
 {0, 9, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"PARAM4"}},
 {0, 9, 0, 0, k_unit_param_type_bitmaps, 0, 0, 0, {"PARAM5"}},
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}, 
 
 [...]
 ```

#### Parameter Types

次のパラメータタイプが利用可能です.

  * `k_unit_param_type_none` : 型のない値. 端数部分を考慮しつつそのまま値を表示します.
  * `k_unit_param_type_percent` : パーセント値. 表示される値に `%` の文字が添えられます.
  * `k_unit_param_type_db` : デシベル値. 表示される値に `dB` の文字が添えられます.
  * `k_unit_param_type_cents` : ピッチセント値. 表示される値に `C` の文字が添えられます. 正の値にはオフセットであることを示す `+` の文字が付きます.
  * `k_unit_param_type_semi` : ピッチセミトーン値. 正の値の先頭にはオフセットであることを示す `+` の文字が添えられます.
  * `k_unit_param_type_oct` : ピッチセミトーン値. 正の値の先頭にはオフセットであることを示す `+` の文字が添えられます.
  * `k_unit_param_type_hertz` : ヘルツ値. 表示される値に `Hz` の文字が添えられます.
  * `k_unit_param_type_khertz` : キロヘルツ値. 表示される値に `kHz` の文字が添えられます.
  * `k_unit_param_type_bpm` : BPM（Beat per minute）値.
  * `k_unit_param_type_msec` : ミリ秒の値. 表示される値に `ms` の文字が添えられます.
  * `k_unit_param_type_sec` : 秒数. 表示される値に `s` の文字が添えられます.
  * `k_unit_param_type_enum` : 数値列挙値. 値の最小値が0に設定されている場合, 表示時に1ずつ増加します.
  * `k_unit_param_type_strings` : カスタム文字列による値. 文字列を取得するために `unit_get_param_str_value(..)` を呼び出すとその数値が渡されます. 詳細は [文字列](#文字列) のセクションをご参照ください.
  * `k_unit_param_type_bitmaps` : カスタムビットマップ表現による値. ビットマップ表現を取得するために `unit_get_param_bmp_value(..)` を呼び出すとその値が渡されます. 詳細は [ビットマップ](#ビットマップ) のセクションをご参照ください.
  * `k_unit_param_type_drywet` : ドライ/ウェットの値. 負の値にはドライを表す `D`, 正の値にはウェットを表す `W`, 0の値にはバランスのとれたミックスを表す `BAL` の文字が添えられます.
  * `k_unit_param_type_pan` : ステレオパン値. `%` の文字が自動的に添えられます. 負の値には左を表す `L`, 正の値には右を表す `R` の文字が添えられ, 0の値は `C` の文字に置き換えられます.
  * `k_unit_param_type_spread` : ステレオの広がりを表す値. 負の値には `<`, 正の値には `>` の文字が添えられます.
  * `k_unit_param_type_onoff`: オン/オフのトグル値. `0`　が `off` として, `1` が `on` として表示されます.
  * `k_unit_param_type_midi_note` : MIDIノートの値. ノート値がピッチとして表示されます (表示例: `C0`, `A3`).

## ユニットAPIの概要

ここではシンセとエフェクトのAPIの概要を説明します.

### Essential Functions

すべてのユニットは下記の関数について実装しなければなりません. ただしデフォルトでフォールバック実装があるため, 該当するファンクションのみ明示的に実装する必要があります.

 * `__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc)` : ユニットが読み込まれた後に呼び出されます. ランタイム環境の基本的なチェック, ユニットの初期化, 動的なメモリ割り当て（必要であれば）のために使用する必要があります. `desc` は現在のランタイム環境の説明を提供します (詳細は [ランタイム記述子](#ランタイム記述子) のセクションをご参照ください).
 * `__unit_callback void unit_teardown()` : ユニットがアンロードされる前に呼び出されます. メモリの解放とクリーンアップを実行する際に必要です.
 * `__unit_callback void unit_reset()` : ユニットをリセットする必要があるときに呼ばれます. アクティブなノートを非アクティブにし, エンベロープジェネレーターをニュートラルな状態へ, オシレーターフェイズをリセットし, ディレイラインをクリアしなければなりません（すべて同時にクリアすると負荷が重いかもしれません）. ただしパラメータ値はデフォルト値にリセットしてはいけません. 
 * `__unit_callback void unit_resume()` : ユニットが一時停止した後, 再び処理を開始する直前に呼ばれます.
 * `__unit_callback void unit_suspend()` : ユニットがサスペンドされているときに呼ばれます. 例として現在アクティブなユニットが別のユニットに交換される場合などがあり, 通常そのあとに `unit_reset()` が呼ばれます.
 * `__unit_callback void unit_render(const float * in, float * out, uint32_t frames)` : オーディオレンダリングのコールバックです. シンセユニットは引数 `in` を無視する必要があります. インプット/アウトプットのバッファーのジオメトリ情報は `unit_init(..)` の引数 `unit_runtime_desc_t` で提供されます.
 * `__unit_callback uint8_t unit_get_preset_index()` : ユニットが現在使用しているプリセットインデックスを返す必要があります.
 * `__unit_callback const char * unit_get_preset_name(uint8_t index)` : 与えられたプリセットインデックスの名前を取得するために呼び出されます. 戻り値は最大X文字の7ビットASCII ヌル終端文字列である必要があり, 表示可能な文字はユニット名と同じです.
 * `__unit_callback void unit_load_preset(uint8_t index)` : 指定されたインデックスのプリセットをロードするために呼び出されます. 詳細は [プリセット](#プリセット) のセクションをご参照ください.
 * `__unit_callback int32_t unit_get_param_value(uint8_t index)` : 指定されたインデックスのパラメータの現在値を取得するために呼び出されます.
 * `__unit_callback const char * unit_get_param_str_value(uint8_t index, int32_t value)` : これは`k_unit_param_type_strings` 型のパラメータに対して, 指定した値の文字列表現を取得するために呼ばれます.  戻り値は最大X文字の7ビットASCII ヌル終端文字列である必要があります. C言語の文字列ポインタは `unit_get_param_str_value(..)` が再び呼ばれた後もキャッシュや再利用されることはないと考えられます. よって同じメモリ領域を複数の呼び出しにわたり再利用することもできます.
 * `__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t index, int32_t value)` : これは `k_unit_param_type_bitmaps` 型のパラメータに対して, 指定した値のビットマップ表現を取得するために呼び出されます. このポインタは `unit_get_param_bmp_value(..)` が再び呼ばれた後もキャッシュや再利用されることはないと考えられます. よって同じメモリ領域の複数の呼び出しにわたって再利用することもできます. ビットマップデータのフォーマットに関する詳細は [ビットマップ](#ビットマップ) のセクションをご参照ください.
 * `__unit_callback void unit_set_param_value(uint8_t index, int32_t value)` : 指定されたインデックスのパラメータの現在値を設定するために呼ばれます. drumlogueでは値は16ビット整数として格納されますが, 将来のAPI変更を避けるため32ビット整数として渡されることに注意してください. より安全に運用するために, ヘッダで宣言されたmin/maxの値に従ってチェック値を制限してください.
 * `__unit_callback void unit_set_tempo(uint32_t tempo)` : テンポが変更された時に呼ばれます. テンポは固定小数点で, 上位16ビットがBPMの整数部分, 下位16ビットが小数部分（ローエンディアン）です. このハンドラは特に外部機器と同期している場合に頻繁に呼ばれる可能性があるため, テンポ変更を処理するときはCPU負荷をできるだけ下げないよう注意してください. 
 
### シンセユニットの固有のFunctions
 
 * `__unit_callback void unit_note_on(uint8_t note, uint8_t velocity)` : MIDIノートオンイベントで呼ばれます. また明示的に `unit_gate_on(..)` ハンドラが提供されていない場合は内部シーケンサーのゲートオンイベントで呼び出されます（その場合ノートは0xFFUに設定されます）.
 * `__unit_callback void unit_note_off(uint8_t note)` : MIDIノートオフイベントで呼ばれます. また明示的に `unit_gate_off(..)` ハンドラが提供されていない場合は内部シーケンサーのゲートオフイベントで呼び出されます（その場合ノートは0xFFUに設定されます）.
 * `__unit_callback void unit_gate_on(uint8_t velocity)` (任意) : 使用する場合, 内部シーケンサのゲートオンイベントで呼び出されます.
 * `__unit_callback void unit_gate_off(void)` (任意) : 使用する場合,　内部シーケンサのゲートオフイベントで呼び出されます.
 * `__unit_callback void unit_all_note_off(void)` : 呼ばれると, すべてのアクティブなノートを非アクティブにし, エンベロープジェネレータをリセットします.
 * `__unit_callback void unit_pitch_bend(uint16_t bend)` : MIDIピッチベンドイベントで呼ばれます.
 * `__unit_callback void unit_channel_pressure(uint8_t pressure)` : MIDIチャンネルプレッシャーイベントで呼ばれます.
 * `__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch)` : MIDIアフタータッチイベントで呼ばれます.
 
### ランタイム記述子

 ランタイムディスクリプタへの参照は, 初期化時にユニットに渡されます. この記述子は, 現在のデバイスとAPI, オーディオレート, バッファジオメトリに関する情報, 呼び出し可能なAPI関数へのポインタを提供します.

 ```
 typedef struct unit_runtime_desc {
  uint16_t target;
  uint32_t api;
  uint32_t samplerate;
  uint16_t frames_per_buffer;
  uint8_t input_channels;
  uint8_t output_channels;
  unit_runtime_get_num_sample_banks_ptr get_num_sample_banks;
  unit_runtime_get_num_samples_for_bank_ptr get_num_samples_for_bank;
  unit_runtime_get_sample_ptr get_sample;
 } unit_runtime_desc_t;
 ```
 
 * `target` : 現在のプラットフォームとモジュール. 次のいずれかに設定してください: `k_unit_target_drumlogue_delfx`, `k_unit_target_drumlogue_revfx`, `k_unit_target_drumlogue_synth`, `k_unit_target_drumlogue_masterfx`. 現在のユニットとランタイム環境の互換性をチェックしたい場合は `UNIT_TARGET_PLATFORM_IS_COMPAT(target)` マクロが便利です.
 * `api` : 使用しているAPIのバージョン. 上位16ビットにメジャー, 下位2バイトにマイナーとパッチ番号が入ります. (例: v1.2.3 -> 0x00010203U). 現在のユニットとランタイム環境APIの互換性をチェックしたい場合は `UNIT_API_IS_COMPAT(api)` マクロが便利です.
 * `samplerate` : オーディオ処理に使用されるサンプルレート. drumlogueでは常に48000に設定されるべきですがこれはユニットによってチェックされ, 考慮される必要があります. ユニットは `unit_init(..)` コールバックから `k_unit_err_samplerate*` を返すことで, 予期せぬサンプルレートを拒否することができます.
 * `frames_per_buffer` : `unit_render(..)` コールバックで期待される, オーディオバッファごとの最大フレーム数を記述します. 一般的にはコールバックの引数 `frames` と同じであるべきですが, それよりも小さい値でも適切にサポートされるべきです. 
 * `input_channels` : `unit_render(..)` コールバックの入力バッファにあるフレームごとのオーディオチャンネル数（サンプル数）です.
 * `output_channels` : `unit_render(...)` コールバックの出力バッファにあるフレームごとのオーディオチャンネル数（サンプル数）です.

#### コールバックAPI Functions

 * `get_num_sample_banks` (`uint8_t get_num_sample_banks()`) : デバイス上で利用できるサンプルバンクの数を取得できます.
 * `get_num_samples_for_bank` (`uint8_t get_num_samples_for_bank(uint8_t bank_idx)`) : 指定されたバンクインデックスに対するサンプル数を取得できます.
 * `get_sample` (`const sample_wrapper_t * get_sample(uint8_t bank_idx, uint8_t sample_idx)`) : 与えられたバンクとサンプルインデックスに対するサンプルラッパーへの参照を取得できます.

#### サンプルラッパー

 ```
 typedef struct sample_wrapper {
  uint8_t bank;
  uint8_t index;
  uint8_t channels;
  uint8_t _padding;
  char name[UNIT_SAMPLE_WRAPPER_MAX_NAME_LEN + 1];
  size_t frames;
  const float * sample_ptr;
 } sample_wrapper_t;
 ```

 * `bank` : このサンプルが存在するバンクのインデックス.
 * `index` : このサンプルのバンク内のサンプルインデックス.
 * `channels` このサンプルのオーディオチャンネル数 (例: `1`: モノラル, `2` : ステレオ). オーディオデータは `unit_render(..)` のコールバックバッファと同様にフレームにインターレースされることに注意してください.
 * `_padding` : 無視してください.
 * `name` : サンプルの名前. 31文字以内です(+ NUL termination).
 * `frames` : サンプルデータに含まれるオーディオフレームの数.
 * `sample_ptr` : サンプルデータ自身へのポインタ. データの長さは `frames` と `channels` を掛け合わせた浮動小数点で表されます.

### プリセット

 ユニットは[ヘッダ構造体](#headerc-ファイル) で `.num_presets` をゼロ以外の値に設定し, 対応するプリセットインデックスに対して `unit_get_preset_index(..)`, `unit_get_preset_name(..)`, `unit_load_preset(..)` コールバックを実行すればプリセット情報を公開することができます. プリセットが公開されている場合, プリセット選択UIが表示されます.
 
 プリセットの読み込みは, 公開されたパラメータを, さらに変更可能な構成として設定することとみなされるべきです. 公開されたパラメータを変更しても, 現在のプリセットインデックスは変更されないはずです. しかし, プリセットの読み込みは, 副次的に公開したパラメータの値を変更させる可能性があります.
 
### 文字列

 `unit_get_param_str_value(..)` で与えられる文字列は, 以下のリストにある7ビットASCIIからなるヌル終端文字列でなければなりません: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?#$%&'()*+,-.:;<=>@`".
 
 パラメータ値の文字列は実質32文字まで設定可能ですが, 表示可能なエリアに限りがあるため, 超過した文字列は `...` として切り捨てられます.

### ビットマップ

 *Note* ビットマップAPIはdrumlogue firmware v1.1.0以降でサポートされています.
 
 ビットマップは16x16の白黒ピクセルで表示され, 左上が原点になります. `unit_get_param_bmp_value(..)` によって与えられるビットマップデータは, 16x16 1bpp package bitmap formatでフォーマットされるべきです. つまり32バイトの配列で, 各バイトの組は最上段から順に16ピクセル（`0 ` : 黒, `1` : 白）一行分を表しています. 書くバイトは最下位ビットから最上位ビットへと処理され, 対応するピクセルが左から右へと表示されます. 

  [Gimp](https://www.gimp.org/) を使ってこのような [ビットマップデータを作成するチュートリアル](https://zilogic.com/creating-glcd-bitmaps-using-gimp/) があります.
 
 また, [ペイント(MS)を使ったチュートリアル](https://exploreembedded.com/wiki/Displaying_Images_and_Icons_on_GLCD) もあります.

 下記はビットマップの例です:
 
 *White square*
 ```
 const uint8_t bmp[32] = {
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU,
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU
 };
 ```
 
 *Alternating horizontal lines*
 ```
 const uint8_t bmp[32] = {
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U,
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U
 };
 ```

 *Alternating vertical lines*
 ```
 const uint8_t bmp[32] = {
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU,
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU
 };
 ```

 *Box outline with diagonal between upper left and lower right corner*
  ```
 const uint8_t bmp[32] = {
   0xFFU, 0xFFU, 
   0x03U, 0x80U, 
   0x05U, 0x80U, 
   0x09U, 0x80U, 
   0x11U, 0x80U, 
   0x21U, 0x80U,
   0x41U, 0x80U, 
   0x81U, 0x80U, 
   0x01U, 0x81U, 
   0x01U, 0x82U, 
   0x01U, 0x84U, 
   0x01U, 0x88U,
   0x01U, 0x90U, 
   0x01U, 0xA0U, 
   0x01U, 0xC0U, 
   0xFFU, 0xFFU
 };
 ```
