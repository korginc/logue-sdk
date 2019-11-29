---
# To modify the layout, see https://jekyllrb.com/docs/themes/#overriding-theme-defaults

layout: home
---

The *logue SDK* is a C/C++ software development kit and API that allows to create custom oscillators and effects for the KORG [prologue](https://www.korg.com/products/synthesizers/prologue), [minilogue xd](https://www.korg.com/products/synthesizers/minilogue_xd) and [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) synthesizers.

## Quick Start

### Installing the logue SDK

 * Clone the [git repository](https://github.com/korginc/logue-sdk) and initialize/update submodules.

```
$ git clone https://github.com/korginc/logue-sdk.git
$ cd logue-sdk
$ git submodule update --init
```
 * Install toolchain: [GNU Arm Embedded Toolchain](https://github.com/korginc/logue-sdk/tools/gcc)
 * Install other utilties:
    * [GNU Make](https://github.com/korginc/logue-sdk/tools/make)
    * [Info-ZIP](https://github.com/korginc/logue-sdk/tools/zip)
    * [logue-cli](https://github.com/korginc/logue-sdk/tools/logue-cli) (optional)

### Building the Demo Oscillator (Waves)

Waves is a morphing wavetable oscillator that uses the wavetables provided by the custom oscillator API. It is a good example of how to use API functions, declare edit menu parameters and use parameter values of various types.

 * move into the project directory.

```
$ cd logue-sdk/platform/prologue/demos/waves/
```

  _Note: replace "prologue" in the path with the target platform of your choice_

 * type `make` to build the project.
 
 ```
$ make
Compiler Options
../../../../tools/gcc/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -std=c11 -mstructure-size-boundary=8 -W -Wall -Wextra -Wa,-alms=./build/lst/ -DSTM32F401xC -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM4 -D__FPU_PRESENT -I. -I./inc -I./inc/api -I../../inc -I../../inc/dsp -I../../inc/utils -I../../../ext/CMSIS/CMSIS/Include
    
Compiling _unit.c
Compiling waves.cpp
Linking build/waves.elf
Creating build/waves.hex
Creating build/waves.bin
Creating build/waves.dmp

   text	   data	    bss	    dec	    hex	filename
   2304	      4	    144	   2452	    994	build/waves.elf

Creating build/waves.list
Packaging to ./waves.prlgunit
 
Done
```
    
 * As the *Packaging...* line indicates, a *.prlgunit* file will be generated. This is the final product.

_Note: for minilogue xd the extension will be *.mnlgxdunit*, for NuTekt NTS-1 digital it will be *.ntkdigunit*_

### Using *unit* Files

*.prlgunit*, *.mnlgxdunit*, and  *.ntkdigunit* files are simple zip files containing the binary payload for the custom oscillator or effect and a metadata file describing it.
They can be loaded onto a device matching the target platform using the [logue-cli utility](https://github.com/korginc/logue-sdk/tools/logue-cli/) or the Librarian application for that device (see product page on [KORG's official website](https://korg.com)).
