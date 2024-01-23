---
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: home_ja
permalink: /ja/
---

*logue SDK* とは KORG [prologue](https://www.korg.com/jp/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/jp/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/jp/products/dj/nts_1), [Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/jp/products/dj/nts_1_mk2), [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/jp/products/dj/nts_3), [drumlogue](https://www.korg.com/jp/products/drums/drumlogue/) のカスタムオシレーターやシンセ, エフェクトを作成可能なソフト開発キットとAPIです.

SDKで作成された単一のカスタムコンテンツは *"ユニット"* と呼ばれます. 各プラットフォームは製品の設計と信号経路に応じた特定のユニットに対応しており, 他のユニットには対応しないことがあります.

## prologueとminilogue-xd, NTS-1 digital kit (mkI and mkII)

これらの機種では, オシレーター, モジュレーションエフェクト, ディレイエフェクト, リバーブエフェクトの4種類のカスタムユニットを作成することができます.

### カスタムオシレーター

カスタムオシレーターは, バッファ処理コールバックを介して安定したオーディオ信号を提供することが期待される自己完結型のサウンドジェネレーターです.
これらは各機種のボイス構造の一部で動作するため, アーティキュレーションとフィルター処理はすでにおこなわれており, オシレーターは指定されたピッチ情報とその他の利用可能なパラメータに従って波形を提供するだけでよいのです.

### モジュレーションエフェクト

モジュレーションエフェクトは、ボイスのアーティキュレーションとフィルタリングの後, ディレイとリバーブエフェクトの前に処理されるインサートエフェクトです.
[prologue](https://www.korg.com/jp/products/synthesizers/prologue) ではデュアルティンバー機能をサポートするために, APIは二つのバッファーを提供し, 同様の処理をする必要であることに注意してください. [minilogue xd](https://www.korg.com/jp/products/synthesizers/minilogue_xd) と [Nu:Tekt NTS-1 digital kit](https://www.korg.com/jp/products/dj/nts_1) では二つ目のバッファーは安全に省略することができます.

[Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/jp/products/dj/nts_1_mk2)ではAPIが他のエフェクトと統一されて, 2つ目のバッファーは存在しません.

### ディレイエフェクトとリバーブエフェクト

ディレイエフェクトとリバーブエフェクトは, どちらもモジュレーションエフェクトの後に処理されるセンドエフェクトです.  [prologue](https://www.korg.com/jp/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/jp/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/jp/products/dj/nts_1) ではカスタムディレイとリバーブエフェクトが同じランタイムにロードされるため, ディレイとリバーブの両方が有効であってもカスタムエフェクトは片方だけしか使用することができません. ただし内部エフェクトとカスタムディレイ, リバーブエフェクトは自由に組み合わせることができます.

[Nu:Tekt NTS-1 digital kit mkII](https://www.korg.com/jp/products/dj/nts_1_mk2)では, ディレーとリバーブそれぞれの個別ランタイムがあるため, 制限なく利用できます.

### バイナリ互換性について

[prologue](https://www.korg.com/jp/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/jp/products/synthesizers/minilogue_xd), [Nu:Tekt NTS-1 digital kit](https://www.korg.com/jp/products/dj/nts_1)の機種に限ってバイナリ互換性があります. しかし機種間でスペックに違いがあるため, カスタムユニットは各機種ごとに個別に最適化しビルドする必要があります.

### 各プラットフォームの詳細

より詳しい情報は下記のREADMEファイルに記載しています.

 * [prologue platform](https://github.com/korginc/logue-sdk/tree/master/platform/prologue/README_ja.md)
 * [minilogue-xd platform](https://github.com/korginc/logue-sdk/tree/master/platform/minilogue-xd/README_ja.md)
 * [NTS-1 digital kit platform](https://github.com/korginc/logue-sdk/tree/master/platform/nutekt-digital/README_ja.md)
 * [NTS-1 digital kit mkII platform](https://github.com/korginc/logue-sdk/tree/master/platform/nts-1_mkii/README.md) (日本語版準備中)

## NTS-3 kaoss pad kit

[Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/jp/products/dj/nts_3)では４つのエフェクトランタイムがあって, その環境用に作成されたエフェクトは*ジェネリックエフェクト*と呼ばれます.

### 詳細な情報

 より詳細な情報は [NTS-3 kaoss pad kit platform README](https://github.com/korginc/logue-sdk/tree/master/platform/nts-3_kaoss/README.md) ファイルをご参照ください.  (日本語版準備中)

## drumlogue

[drumlogue](https://www.korg.com/jp/products/drums/drumlogue) では, シンセ, ディレイエフェクト, リバーブエフェクト, マスターエフェクトの4種類のカスタムユニットを作成することができます.

[drumlogue](https://www.korg.com/jp/products/drums/drumlogue) のカスタムユニットAPIは, 上記のほかのターゲットプラットフォームとの互換性はありません. しかしコアAPI構造には類似点があるため, ユニットは比較的容易に移植できるはずです.

### カスタムシンセ

カスタムシンセは, ボイスのアーティキュレーションとフィルタリング（該当する場合）を担当する自己完結型のサウンドジェネレーターです. これらは [drumlogue](https://www.korg.com/jp/products/drums/drumlogue) のマルチエンジンセクションで個々のシンセパートとして処理されます.

### ディレイエフェクト

ディレイエフェクトは専用のランタイム環境で実行され, 専用のセンドバスを処理します. 出力はマスターエフェクトの前または後にミックスバックすることができ, リバーブエフェクトへのルーティングも可能です.

### リバーブエフェクト

リバーブエフェクトは専用のランタイム環境で実行され, 専用のセンドバスを処理します. 出力はマスターエフェクトの前または後にミックスバックすることができます.

### マスターエフェクト

マスターエフェクトはインラインエフェクトで, パートごとにバイパスすることができます. またAPIではオプションのサイドチェインセンドバスを使用することも可能です.

### 詳細な情報

 より詳細な情報は [drumlogue platform README](https://github.com/korginc/logue-sdk/tree/master/platform/drumlogue/README_ja.md) ファイルをご参照ください.
