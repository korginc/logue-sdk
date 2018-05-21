# prologue-sdk 

This repository contains all the files and tools needed to build custom oscillators and effects for the [prologue synthesizer](http://korg.com/prologue).

#### Overall Structure:
* [platform/prologue/](platform/prologue/) : Prologue specific files, templates and demo projects.
* [platform/ext/](platform/ext/) : External dependencies and submodules.
* [tools/](tools/) : Installation location and documentation for tools required to build projects and manipulate built products.
* [devboards/](devboards/) : Information and files related to limited edition development boards.

## Quick Start

### Building Projects

**Note: Make sure git submodules are initialized and updated.**

**Note: Make sure the necessary toolchain is properly installed. Refer to [tools/](tools/) for installation instructions.**

 1. move into a project directory.
 2. type `make` to build the project.

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
 3. As the *Packaging...* line indicates, a *.prlgunit* file will be generated.
 
### Manipulating/Using *.prlgunit* Files

*.prlgunit* files are simple zip files containing the binary payload for the custom oscillator or effect and a metadata file.
They can be loaded onto a [prologue](http://korg.com/prologue) (or [development board](devboards/)) using the [logue-cli utility](tools/logue-cli/) or the [Librarian application (to be released)](http://korg.com/prologue).

**Note: The prologue firmware currently does not support the custom oscillator and effects, the feature will be provided in an update planned for June 2018. Currently the feature is only available on the development board**

## Demo Code

### Waves

Waves is a morphing wavetable oscillator that uses the wavetables provided by the custom oscillator API. It is a good example of how to use API functions, declare edit menu parameters and use parameter values of various types. See [platform/prologue/demos/waves/](platform/prologue/demos/waves/) for code and details.

## Sharing your Oscillators/Effects with us

To show us your work please reach out to *logue-sdk@korg.co.jp*.

## Support

The SDK is provided as-is, no technical support will be provided by KORG.

<!-- ## Troubleshooting -->





