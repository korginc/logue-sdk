## Dockerベースのビルド環境

[English](./README.md)

### 概要

 Dockerベースのビルド環境は, サポートされている各プラットフォーム用のlogue SDKユニットを構築するために必要なすべてのツールをパッケージ化し, 規模の大きいプロジェクトの構築を簡単にする便利なツールを提供します. このDockerベースの環境を利用することで, ホストOSから独立した一貫的な環境で構築することが可能です.

### CLRF errorに遭遇した時の解決法

logue-sdk をクローンした後は、ローカルのGit設定で core.autocrlf を true ではなく input に変更してください。

logue-sdkのフォルダで、

```
git config core.autocrlf input
```

### セットアップ

 1. [Dockerをインストール](https://docs.docker.com/get-docker/) します (Note: Docker Desktopは特に必要なく, [installing Docker Engine](https://docs.docker.com/engine/install/) で十分です).
 2. このリポジトリをクローンし, サブモジュールを初期化/アップデートします.

```
 $ git clone https://github.com/korginc/logue-sdk.git
 $ cd logue-sdk
 $ git submodule update --init
 ```
 
 3. コンテナをビルドします.
 ```
 $ cd docker
 $ ./build_image.sh
 
 [...]
 ```
 あるいは、Docker Hubからこの[コンテナイメージ](https://hub.docker.com/r/xiashj/logue-sdk)をダウンロードします。

 ```
 docker pull xiashj/logue-sdk
 ```
 Docker Desktopから「logue-sdk」を検索して「Pull」を押してもダウンロードできます。


#### Windowsでの注意点

 Windows (10, 11) の場合, [Windows Subsystem for Linux (WSL2)](https://learn.microsoft.com/en-us/windows/wsl/) のインストールが必要です. これはDockerや, powershellやコマンドプロンプトで便利なbashスクリプト（詳細は後述します）を活用するために必要です.

### Interactive Shell

`run_interactive.sh` スクリプトを使用すると, Dockerインスタンスをシステム的に再インスタンス化することなく, 複数の操作を実行できます.

この方法は開発工程で手動でビルドする場合に有効です.

 1. [run_interactive.sh](./run_interactive.sh) を実行します.

```
 $ ./run_interactive.sh
 user@logue-sdk $ 
 ```

 これにより以下のコマンドが利用できるようになります:

```
 user@logue-sdk $ list
 Available logue SDK commands:
 - env : Prepare environment for target platform builds.
 - build : Wrapper script to build logue SDK unit projects.
 - list : List available logue SDK related commands.
 ```
 
### Single Command Execution

`run_cmd.sh` スクリプトは, logue SDKのDockerコンテナ内でコマンドを一つ実行することが可能で, IDEや自動ビルドシステムとビルドを統合する際に便利です.

この方法では, コマンドを実行するたびに新しいコンテナ・インスタンスが生成され, コマンドが終了するとそのインスタンスは破棄されることに注意してください. 手動ビルドを繰り返す場合, これは許容できないシステム負荷を引き起こす可能性があるため, インタラクティブな方法を選択する必要があります. 

#### Examples

```
 $ ./run_cmd.sh list
 Available logue SDK commands:
 - env : Prepare environment for target platform builds.
 - build : Wrapper script to build logue SDK unit projects.
 - list : List available logue SDK related commands.
 
 $ ./run_cmd.sh build -l --drumlogue
 - drumlogue/dummy-delfx
 - drumlogue/dummy-masterfx
 - drumlogue/dummy-revfx
 - drumlogue/dummy-synth
 
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

### `env` コマンド

 `env` コマンドは, ターゲットプラットフォームビルド環境を初期化し, 手動でビルド操作をするために使用します（例: プロジェクトディレクトリで直接 `make` を使用するときなど）. 

#### 使い方

```
 user@logue-sdk $ env -h
 Usage: env [OPTION] [ENV]
 
 Prepare environment for target platform builds.
 
 Options:
  -l, --list    list available environments
  -r, --reset   reset current environment
  -h, --help    display this help
  ```

#### Example

```
 user@logue-sdk:~$ env -l
 Available build environments:
 - nts-3_kaoss
 - minilogue-xd
 - nts-1_mkii
 - prologue
 - nts-1
 - drumlogue

 user@logue-sdk:~$ env drumlogue
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment

 [drumlogue] ~ $ cd drumlogue/dummy-synth
 [drumlogue] ~ $ cd drumlogue/dummy-synth/
 [drumlogue] ~/drumlogue/dummy-synth $ make
 Compiler Options
 arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -std=c11 -W -Wall -Wextra -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.c -o build/obj/xxxx.o
 arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fconcepts -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -fno-threadsafe-statics -std=gnu++14 -W -Wall -Wextra -Wno-ignored-qualifiers -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.cc -o build/obj/xxxx.o
 arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.s -o build/obj/xxxx.o
 arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.s -o build/obj/xxxx.o
 arm-unknown-linux-gnueabihf-g++ xxxx.o -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -shared -Wl,-Map=build/dummy_synth.map,--cref -lm -lc -o build/dummy_synth.elf
 
 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc
 
 Linking build/dummy_synth.drmlgunit
 Stripping build/dummy_synth.drmlgunit
 Creating build/dummy_synth.hex
 Creating build/dummy_synth.bin
 Creating build/dummy_synth.dmp
 
    text	   data	    bss	    dec	    hex	filename
    3267	    316	      8	   3591	    e07	build/dummy_synth.drmlgunit
 Creating build/dummy_synth.list
 
 Done
 
 [...]
 
 [drumlogue] ~ $ env -r
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 user@logue-sdk:~$
 ```

### `build` コマンド

 `build` コマンドはどのようなプラットフォームでもプロジェクトをビルドすることができます. より細かい制御が必要でない限り, この方法でプロジェクトをビルドすることを推奨します. 

#### 使い方

```
 user@logue-sdk:~$ build -h
 Usage: /app/commands/build [OPTIONS] [UNIT_PROJECT_PATH]
 
 Wrapper script to build logue SDK unit projects.
 
 Options:
  -l, --list            list valid unit projects
  -c, --clean           only clean projects
  -a, --all             select all valid unit projects
      --drumlogue       select drumlogue unit projects
      --minilogue-xd    select minilogue-xd unit projects
      --nutekt-digital  select nts-1 unit projects
      --nts-1           alias for --nutekt-digital
      --nts-1_mkii      select nts-1 mkii unit projects
      --nts-3           select nts-3 unit projects
      --prologue        select prologue unit projects
  -f, --force           force clean project before building
      --install-dir=DIR install built units to specified directory
  -h, --help            display this help
 
 Arguments:
 [UNIT_PROJECT_PATH]    specify path (relative to /workspace) to the unit project to build/clean
 
 Notes:
  * UNIT_PROJECT_PATH and platform selection options are mutually exclusive.
  * -a/-all is overriden by platform selection options if specified.
  ```

#### Examples

 * ビルドできるプロジェクトをリストアップ

```
 user@logue-sdk:~$ build -l
 - drumlogue/dummy-delfx
 - drumlogue/dummy-masterfx
 - drumlogue/dummy-revfx
 - drumlogue/dummy-synth
 - minilogue-xd/dummy-delfx
 - minilogue-xd/dummy-modfx
 - minilogue-xd/dummy-osc
 - minilogue-xd/dummy-revfx
 - minilogue-xd/waves
 - nutekt-digital/dummy-delfx
 - nutekt-digital/dummy-modfx
 - nutekt-digital/dummy-osc
 - nutekt-digital/dummy-revfx
 - nutekt-digital/waves
 - prologue/dummy-delfx
 - prologue/dummy-modfx
 - prologue/dummy-osc
 - prologue/dummy-revfx
 - prologue/waves
 - nts-1_mkii/dummy-delfx
 - nts-1_mkii/dummy-modfx
 - nts-1_mkii/dummy-osc
 - nts-1_mkii/dummy-revfx
 - nts-1_mkii/waves
 - nts-3_kaoss/dummy-genericfx
```

 * 特定のプロジェクトのビルド（`drumlogue/dummy-synth` をビルドする例）

```
 user@logue-sdk:~$ build drumlogue/dummy-synth
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/drumlogue/dummy-synth
 Compiler Options
 arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -std=c11 -W -Wall -Wextra -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.c -o build/obj/xxxx.o
 arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fconcepts -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -fno-threadsafe-statics -std=gnu++14 -W -Wall -Wextra -Wno-ignored-qualifiers -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.cc -o build/obj/xxxx.o
 arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/drumlogue/common xxxx.s -o build/obj/xxxx.o
 arm-unknown-linux-gnueabihf-g++ xxxx.o -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -shared -Wl,-Map=build/dummy_synth.map,--cref -lm -lc -o build/dummy_synth.elf

 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc
 
 Linking build/dummy_synth.drmlgunit
 Stripping build/dummy_synth.drmlgunit
 Creating build/dummy_synth.hex
 Creating build/dummy_synth.bin
 Creating build/dummy_synth.dmp
 Creating build/dummy_synth.list
 
    text	   data	    bss	    dec	    hex	filename
    3267	    316	      8	   3591	    e07	build/dummy_synth.drmlgunit
 
 Done

 >> Installing /workspace/drumlogue/dummy-synth
 Deploying to /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 ```
 
 * 特定のプロジェクトのクリーンアップ（`drumlogue/dummy-synth` をクリーンする例）
 
```
 user@logue-sdk:~$ build -c drumlogue/dummy-synth
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/drumlogue/dummy-synth
 
 Cleaning
 rm -fR .dep build /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
  ```
 
 * すべてのnutekt-digital (nts-1) プロジェクトをビルドする
 
```
 user@logue-sdk:~$ build --nts-1
 
 [...]
  ```
  
 * すべてのnutekt-digital (nts-1) プロジェクトをクリーンアップ
 
``` 
 user@logue-sdk:~$ build -c --nts-1
 
 [...]
 
 ```
