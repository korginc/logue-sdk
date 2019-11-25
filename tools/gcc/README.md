## Installing the GNU Arm Embedded Toolchain

Prebuilt binaries are available [here](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads).

_Required version: 5-2016-q3-update_

### OSX

Run `./get_gcc_osx.sh` to download and unpack gcc automatically.

### Linux

#### Native System or VM

Run `./get_gcc_linux.sh` to download and unpack gcc automatically.

#### Windows Subsystem for Linux

Unfortunately, only 32 bit prebuilt packages are available for Linux and WSL does not support running 32 bit ELF executables.
A possibility is to obtain the source package from the link above and manually build gcc, or install an appropriate package from your distribution and create a symlink named `gcc-arm-none-eabi-5_4-2016q3` pointing to the package's install location.

### Windows

#### MSYS2

If you have MSYS2 installed you can run `./get_gcc_msys.sh` to download and unpack gcc automatically.

_Note 1: MSYS2 installers for Windows can be found at [https://www.msys2.org/](https://www.msys2.org/)_

_Note 2: The install script requires the unzip package. It can be installed by running: `pacman -S unzip`_

#### Manual Procedure

Download version 5-2016-q3-update in *ZIP format* from the link above and unpack it in this directory under the name "gcc-arm-none-eabi-5_4-2016q3".

The directory should have the following structure:

```
gcc-arm-none-eabi-5_4-2016q3/
   |
   +-- arm-none-eabi/
   |
   +-- bin/
   |
   +-- lib/
   |
   +-- share/
```
