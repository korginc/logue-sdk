---
layout: page_ja
title: Oscillators
title_long: オシレーター
permalink: /ja/doc/osc/
language: ja
---

## 新しいオシレータープロジェクトを作る

1. オシレーター（osc/）のテンプレートプロジェクトをコピーし, 好きな名前に変更して下さい.

2. *Makefile* の先頭にある PLATFORMDIR 変数が *platform/* ディレクトリを指していることを確認してください. このパスは外部依存ファイルや必要なツールを参照するのに用いられます.

3. *project.mk* に好きなプロジェクト名を入力して下さい. 構文についての詳細は [project.mk](#projectmk) のセクションを参照して下さい.

4. プロジェクトにソースファイルを追加した際は, *project.mk* ファイルを編集し, ビルドの際にソースファイルが発見できるようにする必要があります.

5. *manifest.json* ファイルを編集し, 適切なメタデータをプロジェクトに追加します. 構文についての詳細は [manifest.json](#manifestjson) のセクションを参照して下さい.

## プロジェクトの構造

### manifest.json

manifestファイルはjsonフォーマットで下記の例のように書きます。

```
{
    "header" : 
    {
        "platform" : "prologue",
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

* platform (string) : プラットフォームの名前. プラットフォームに合わせて, *prologue*, *minilogue-xd* か *nutekt-digital* に設定します.
* module (string) : モジュールの名前, *osc*, *modfx*, *delfx*, *revfx* のいずれかに設定して下さい.
* api (string) : 使用している API バージョン. (フォーマット: MAJOR.MINOR-PATCH)
* dev_id (int) : デベロッパーID. 現在は未使用, 0に設定します.
* prg_id (int) : プログラムID. 現在は未使用, リファレンスとして設定しても構いません.
* version (string) : プログラムのバージョン. (フォーマット: MAJOR.MINOR-PATCH)
* name (string) : プログラムの名前. (本体のディスプレイに表示されます.)
* num_params (int) : エディットメニューで操作可能なパラメーターの数. 最大6個のパラメーターを持つことが出来ます.
* params (array) : パラメーターの詳細.

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

logue SDK でのオシレーター作成にあたり, 中心となるAPIの概要を説明します.

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
