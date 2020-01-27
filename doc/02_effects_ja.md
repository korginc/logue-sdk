---
layout: page_ja
title: Effects
title_long: エフェクト
permalink: /ja/doc/fx/
language: ja
---

## 新しいエフェクトプロジェクトを作る

1. 作成したいエフェクト（modfx/delfx/revfx）のテンプレートプロジェクトをコピーし, 好きな名前に変更して下さい.

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
        "module" : "modfx",
        "api" : "1.1-0",
        "dev_id" : 0,
        "prg_id" : 0,
        "version" : "1.0-0",
        "name" : "my modfx",
        "num_param" : 0,
        "params" : []
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
* num_params (int) : エフェクトのプロジェクトでは未使用の為、0に設定します.
* params (array) : エフェクトのプロジェクトでは未使用の為、empty array([])にする必要があります.

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

logue SDK でのエフェクト作成にあたり, 中心となるAPIの概要を説明します.

### Modulation Effects API (modfx)

主要な機能のソースファイルには *usermodfx.h* をインクルードし, 下記の関数を含む必要があります.

* `void MODFX_INIT(uint32_t platform, uint32_t api)`: エフェクトのインスタンス化の際に呼ばれます. このコールバック関数を使用して必要な初期化処理を行います. APIについての詳細は `inc/userprg.h` を参照して下さい.

* `void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn,  float *sub_yn, uint32_t frames)`: ここでは入力サンプルを処理します. この関数は *frames* サンプルで必ず呼ばれます(フレーム毎に1サンプル). *\*\_xn* は入力バッファ,  *\*\_yn*  は出力バッファとなり, ここで処理結果を書き込みます。

    _Note: prologueの2ティンバーをサポートするために、main\_とsub\_の入力と出力のバージョンがあります. prologueではmainとsubの両方に同じ方法で並列に処理する必要があります. マルチティンバーでないプラットフォームでは *sub\_xn* , *sub\_yn* は無視できます._

    _Note: 最大64フレームまでのバッファサイズをサポートする必要があります. 必要な場合はより小さいバッファーで複数の処理を実行することができます. （2のべき乗で最適化してください : 16, 32, 64）_

* `void MODFX_PARAM(uint8_t index, int32_t value)`: パラメーター変更時に呼ばれます.

より詳細な情報については [Modulation Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__modfx__inst.html) 参照してください. また, [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__utils.html),   [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/namespacedsp.html) も役に立ちます.

### Delay Effects API (delfx)

主要な機能のソースファイルには *userdelfx.h* をインクルードし, 下記の関数を含む必要があります.

* `void DELFX_INIT(uint32_t platform, uint32_t api)`: エフェクトのインスタンス化の際に呼ばれます. このコールバック関数を使用して必要な初期化処理を行います.

* `void DELFX_PROCESS(float *xn, uint32_t frames)`: ここでは入力サンプルを処理します. この関数は *frames* サンプルで必ず呼ばれます(フレーム毎に1サンプル). この場合, *xn* は入力と出力バッファーの両方になります. 結果はDryとWetを適切な割合で合わせて書き込む必要があります. （例: shift-depthパラメーターで設定する）

    _Note: 最大64フレームまでのバッファサイズをサポートする必要があります. 必要な場合はより小さいバッファーで複数の処理を実行することができます. （2のべき乗で最適化してください : 16, 32, 64）_
    
* `void DELFX_PARAM(uint8_t index, int32_t value)`: パラメーター変更時に呼ばれます.

より詳細な情報については [Delay Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__delfx__inst.html) を参照してください. また [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__utils.html) , [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/namespacedsp.html) も役に立ちます.

### Reverb Effects API (revfx)

主要な機能のソースファイルには *userrevfx.h* をインクルードし, 下記の関数を含む必要があります.

* `void REVFX_INIT(uint32_t platform, uint32_t api)`: エフェクトのインスタンス化の際に呼ばれます. このコールバック関数を使用して必要な初期化処理を行います. 

* `void REVFX_PROCESS(float *xn, uint32_t frames)`: ここでは入力サンプルを処理します.  この関数は *frames* サンプルで必ず呼ばれます(フレーム毎に1サンプル). この場合, *xn* は入力と出力バッファーの両方になります. 結果はDryとWetを適切な割合で合わせて書き込む必要があります. （例: shift-depthパラメーターで設定する）

    _Note: 最大64フレームまでのバッファサイズをサポートする必要があります. 必要な場合はより小さいバッファーで複数の処理を実行することができます. （2のべき乗で最適化してください : 16, 32, 64）_
    
* `void REVFX_PARAM(uint8_t index, int32_t value)`: パラメーター変更時に呼ばれます.

より詳細な情報については [Reverb Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__revfx__inst.html) を参照してください. また. [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/group__utils.html) , [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/minilogue-xd/v1.1-0/html/namespacedsp.html) も役に立ちます.
