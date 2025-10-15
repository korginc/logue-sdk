## Docker-based Build Environment

[日本語](./README_ja.md)

### Overview

The Docker-based build environment packages together all the tools required to build logue SDK units for any of the supported platforms, provides some convenience tools to simplify bulk project building and allows to build in consistent environment independent from the host operating system.

### how to fix CLRF errors on Windows

After you have cloned logue-sdk, make sure to change local git config core.autocrlf to input instead of true. 

Inside logue-sdk directory, 
```
git config core.autocrlf input
```

### Setup

 1. [Install Docker](https://docs.docker.com/get-docker/) (Note: Docker Desktop is not specifically required, [installing Docker Engine](https://docs.docker.com/engine/install/) is sufficient)
 2. Clone this repository and initialize/update submodules.

```
 $ git clone https://github.com/korginc/logue-sdk.git
 $ cd logue-sdk
 $ git submodule update --init
 ```
 
 3. Build the container.
 ```
 $ cd docker
 $ ./build_image.sh
 
 [...]
 ```

 Alternatively, grab this [container image](https://hub.docker.com/r/xiashj/logue-sdk) from Docker Hub.

 ```
 docker pull xiashj/logue-sdk
 ```
 
 or simply search "logue-sdk" inside Docker Desktop and press "Pull".


#### Windows Notes

 On Windows (10/11) the [Windows Subsystem for Linux (WSL2)](https://learn.microsoft.com/en-us/windows/wsl/) must be installed, including a Linux distribution.
 This is a prerequisite for Docker, and to be able to make use of convenience bash scripts (detailed below) in powershell and command prompt.
 When cloning the repository, make sure that the Git setting `core.autocrlf` is set to `input`, otherwise none of the setup scripts or build scripts will run.
 When running the convenience bash scripts you must run `wsl` or `bash` in powershell and command prompt to get to a bash prompt, otherwise they will be launched in Git Bash, 
 which doesn't have the TTY support required for `run_cmd.sh` and `run_interactive.sh`.

### Interactive Shell

The `run_interactive.sh` script allows to perform multiple operations without systematically re-instantiating the Docker instance.

This method is mainly useful when performing manual builds during the development process. 

 1. Execute [run_interactive.sh](./run_interactive.sh)

```
 $ ./run_interactive.sh
 user@logue-sdk $ 
 ```

 The following commands are then available:

```
 user@logue-sdk $ list
 Available logue SDK commands:
 - env : Prepare environment for target platform builds.
 - build : Wrapper script to build logue SDK unit projects.
 - list : List available logue SDK related commands.
 ```
 
### Single Command Execution

The `run_cmd.sh` script allows to run a single command within a logue SDK Docker container.

This method can be useful when integrating builds with an IDE or an automated build system.

Note that when using this method each command execution implies spinning up new container instance that then gets destroyed once the command terminates. When performing repeated manual builds this can cause unnaceptable overhead, and thus the interactive method should then be preferred.

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

### `env` Command

The `env` command is used to initialize the build environment for a given target platform, and then perform manual build operations (E.g.: using `make` directly in a project directory).

#### Usage

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

### `build` Command

The `build` command can be used to build projects for any platform. Unless more fine grained control is required, this should be the prefered way to build projects.

#### Usage

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

 * Listing buildeable projects

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

 * Building a specific project

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
 
 * Cleaning a specific project
 
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
 
 * Build all nutekt-digital (nts-1) projects
 
```
 user@logue-sdk:~$ build --nts-1
 
 [...]
  ```
  
 * Clean all nutekt-digital (nts-1) projects
 
``` 
 user@logue-sdk:~$ build -c --nts-1
 
 [...]
 
 ```


