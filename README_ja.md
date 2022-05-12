# logue-sdk 

このリポジトリには [prologue](https://www.korg.com/jp/products/synthesizers/prologue/), [minilogue xd](https://www.korg.com/jp/products/synthesizers/minilogue_xd/), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/jp/products/dj/nts_1/) , [drumlogue](https://www.korg.com/jp/products/drums/drumlogue/) の4製品で使用できる自作オシレーターやエフェクトのビルドに必要なファイルが全て揃っています.

#### まずは使ってみよう

既に公開されているオシレーターやエフェクトの情報は [Unit Index](https://korginc.github.io/logue-sdk/ja/unit-index/) にあります.
具体的な入手方法については各デベロッパーのウェブサイトにてご確認下さい.

#### 互換性に関して

* prologue: SDKバージョン1.1-0は, v2.0.0以降のファームウェアでサポートされます.
* minilogue xd: SDKバージョン1.1-0は, v2.0.0以降のファームウェアでサポートされます.
* Nu:Tekt NTS-1 digital: SDKバージョン1.1-0は, v1.0.2以降のファームウェアでサポートされます.
* drumlogue: SDKバージョン2.0-0は, 現在利用可能なすべてのバージョンのファームウェアでサポートされています.

prologue, minilogue xd, Nu:Tekt NTS-1 dgital kitの3製品のために作成されたユニットは, SDKのバージョンが一致する限り互換性があります. しかし各製品へのユニットの最適化が推奨されますので、可能であれば各プラットフォームの専用のビルドを優先してください.

drumlogueのユニットはほかの製品とのバイナリ互換性はありません。

#### 全体の構造:
* [platform/prologue/](platform/prologue/) : prologue専用のファイル, テンプレートとデモプロジェクト
* [platform/minilogue-xd/](platform/minilogue-xd/) : minilogue xd専用のファイル, テンプレートとデモプロジェクト
* [platform/nutekt-digital/](platform/nutekt-digital/) : Nu:Tekt NTS-1 digital kit専用のファイル, テンプレートとデモプロジェクト
* [platform/drumlogue/](platform/drumlogue/) : drumlogue専用のファイルとテンプレート
* [platform/ext/](platform/ext/) : 外部依存ファイルとサブモジュール
* [docker/](docker/) : ホストOSに依存せずあらゆるプラットフォーム向けのプロジェクトを構築するためのdocker containerのソース
* [tools/](tools/) : プロジェクトのビルド、またはビルド成果物の操作に必要なツールとドキュメント. dockerを使用する場合は必要ありません
* [devboards/](devboards/) : 限定配布された開発ボードに関する情報やファイル

## 自作コンテンツを共有する

自作のオシレーターやエフェクトをKORGチームに紹介して下さい.
連絡先は *logue-sdk@korg.co.jp* です.

## サポート

KORGはlogue-sdkに関しての技術的なサポートを提供しません.

<!-- ## Troubleshooting -->
