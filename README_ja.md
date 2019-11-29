# logue-sdk 

このリポジトリには [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) の3製品で使用できる自作オシレーターやエフェクトのビルドに必要なファイルが全て揃っています.

#### まずは使ってみよう

既に公開されているオシレーターやエフェクトの情報は[Unit Index](https://korginc.github.io/logue-sdk/ja/unit-index/) にあります.
具体的な入手方法については各デベロッパーのウェブサイトにてご確認下さい.

#### 互換性に関して

SDK version 1.1-0 でビルドされた user units (ビルド済みのカスタムコードバイナリ）を実行するためには, 下記バージョンのファームウェアが製品本体にインストールされている必要があります.

* prologue: v2.00以降
* minilogue xd: v2.00以降
* Nu:Tekt NTS-1 digital: v1.02以降

#### 全体の構造:
* [platform/prologue/](platform/prologue/) : prologue専用のファイル, テンプレートとデモプロジェクト.
* [platform/minilogue-xd/](platform/minilogue-xd/) : minilogue xd専用のファイル, テンプレートとデモプロジェクト.
* [platform/nutekt-digital/](platform/nutekt-digital/) : Nu:Tekt NTS-1 digital kit専用のファイル, テンプレートとデモプロジェクト.
* [platform/ext/](platform/ext/) : 外部依存ファイルとサブモジュール.
* [tools/](tools/) : プロジェクトのビルド、またはビルド成果物の操作に必要なツールとドキュメント.
* [devboards/](devboards/) : 限定配布された開発ボードに関する情報やファイル.

## 自作コンテンツを共有する

自作のオシレーターやエフェクトをKORGチームに紹介して下さい.
連絡先は *logue-sdk@korg.co.jp* です.

## サポート

KORGはlogue-sdkに関しての技術的なサポートを提供しません.

<!-- ## Troubleshooting -->
