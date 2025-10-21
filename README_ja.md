# logue-sdk 

[English](./README.md)

このリポジトリには [prologue](https://www.korg.com/jp/products/synthesizers/prologue/), [minilogue xd](https://www.korg.com/jp/products/synthesizers/minilogue_xd/), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/jp/products/dj/nts_1/), [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/jp/products/synthesizers/nts_1_mk2), [microKORG2](https://www.korg.com/products/synthesizers/microkorg2) synthesizers, the [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/jp/products/dj/nts_3), and [drumlogue](https://www.korg.com/jp/products/drums/drumlogue/) の6製品で使用できる自作オシレーターやエフェクトのビルドに必要なファイルが全て揃っています.

## まずは使ってみよう

既に公開されているオシレーターやエフェクトの情報は [Unit Index](https://korginc.github.io/logue-sdk/ja/unit-index/) にあります.
具体的な入手方法については各デベロッパーのウェブサイトにてご確認下さい.
[logue-SDK-filter](https://logue-sdk.vercel.app/) という、より検索しやすいユニットインデックスページもあります。

## プラットフォームと互換性に関して

| 製品                           | SDK    | ファームウエア | プロセッサー  | ユニットフォーマット                                        |
|--------------------------------|--------|----------------|---------------|-------------------------------------------------------------|
| prologue                       | v1.1.0 | >= v2.00       | ARM Cortex-M4 | Custom 32-bit LSB executable, ARM, EABI5 v1 (SYSV), static  |
| minilogue-xd                   | v1.1.0 | >= v2.00       | ARM Cortex-M4 | Custom 32-bit LSB executable, ARM, EABI5 v1 (SYSV), static  |
| Nu:Tekt NTS-1 digital kit      | v1.1.0 | >= v1.02       | ARM Cortex-M4 | Custom 32-bit LSB executable, ARM, EABI5 v1 (SYSV), static  |
| drumlogue                      | v2.0.0 | >= v1.0.0      | ARM Cortex-A7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |
| Nu:Tekt NTS-1 digital kit mkII | v2.0.0 | >= v1.0.0      | ARM Cortex-M7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |
| Nu:Tekt NTS-3 kaoss pad kit    | v2.0.0 | >= v1.0.0      | ARM Cortex-M7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |
| microKORG2                     | v2.1.0 | >= v2.0.0      | ARM Cortex-A7 | ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamic |

#### バイナリ互換性について

prologue, minilogue xd, Nu:Tekt NTS-1 dgital kitの3製品のために作成されたユニットは, SDKのバージョンが一致する限りバイナリレベルで互換性があります. しかし各製品へのユニットの最適化が推奨されますので、可能であれば各プラットフォームの専用のビルドを優先してください.

#### リポジトリー構造:
* [platform/prologue/](platform/prologue/) : *prologue*専用のファイル, テンプレートとデモプロジェクト
* [platform/minilogue-xd/](platform/minilogue-xd/) : *minilogue xd*専用のファイル, テンプレートとデモプロジェクト
* [platform/nutekt-digital/](platform/nutekt-digital/) : *Nu:Tekt NTS-1 digital kit*専用のファイル, テンプレートとデモプロジェクト
* [platform/drumlogue/](platform/drumlogue/) : *drumlogue*専用のファイルとテンプレート
* [platform/nts-1_mkii/](platform/nts-1_mkii/) : *Nu:Tekt NTS-1 digital kit mkII*専用のファイル, テンプレートとデモプロジェクト
* [platform/nts-3_kaoss/](platform/nts-3_kaoss/) : *Nu:Tekt NTS-3 kaoss pad kit*専用のファイル, テンプレートとデモプロジェクト
* [platform/microkorg2/](platform/microkorg2/) : *microKORG2*専用のファイル, テンプレートとデモプロジェクト
* [platform/ext/](platform/ext/) : 外部依存ファイルとサブモジュール
* [docker/](docker/) : ホストOSに依存せずあらゆるプラットフォーム向けのプロジェクトを構築するためのdocker containerのソース
* [tools/](tools/) : プロジェクトのビルド、またはビルド成果物の操作に必要なツールとドキュメント. dockerを使用する場合は必要ありません
* [devboards/](devboards/) : 限定配布された開発ボードに関する情報やファイル

## 自作コンテンツを共有する

自作のオシレーターやエフェクトをKORGチームに紹介して下さい.
連絡先は *logue-sdk@korg.co.jp* です.

## サポート

KORGはlogue-sdkに関しての技術的なサポートを提供しません.



