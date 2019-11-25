## NuTekt NTS-1 digital SDK Source and Template Projects 

### Overview

All source files needed to build custom oscillators and effects for the [Nu:Tekt NTS-1 digital kit](https://www.korg.com/products/dj/nts_1) exist under this directory.

#### Compatibility Notes

Firmware version >= 1.02 is required to run user units built with SDK version 1.1-0.

#### Overall Structure:
 * [inc/](inc/) : Common headers.
 * [osc/](osc/) : Custom oscillator project template.
 * [modfx/](modfx/) : Custom modulation effect project template.
 * [delfx/](delfx/) : Custom delay effect project template.
 * [revfx/](revfx/) : Custom reverb effect project template.
 * [demos/](demos/) : Demo projects.

### Setting up the Development Environment

 1. Clone this repository and initialize/update submodules.

```
 $ git clone https://github.com/korginc/logue-sdk.git
 $ cd logue-sdk
 $ git submodule update --init
 ```
 2. Install toolchain: [GNU Arm Embedded Toolchain](../../tools/gcc)
 3. Install other utilties:
    1. [GNU Make](../../tools/make)
    2. [Info-ZIP](../../tools/zip)
    3. [logue-cli](../../tools/logue-cli) (optional)

### Building a Project

 1. move into a project directory.
 
```
$ cd logue-sdk/platform/minilogue-xd/demos/waves/
```
 2. type `make` to build the project.

```
$ make
Compiler Options
../../../../tools/gcc/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -std=c11 -mstructure-size-boundary=8 -W -Wall -Wextra -Wa,-alms=./build/lst/ -DSTM32F446xE -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM4 -D__FPU_PRESENT -I. -I./inc -I./inc/api -I../../inc -I../../inc/dsp -I../../inc/utils -I../../../ext/CMSIS/CMSIS/Include

Compiling _unit.c
Compiling waves.cpp
Linking build/waves.elf
Creating build/waves.hex
Creating build/waves.bin
Creating build/waves.dmp

   text	   data	    bss	    dec	    hex	filename
   2304	      4	    144	   2452	    994	build/waves.elf

Creating build/waves.list
Packaging to ./waves.ntkdigunit

Done
```
 3. As the *Packaging...* line indicates, a *.ntkdigunit* file will be generated. This is the final product.
 
### Manipulating/Using *.ntkdigunit* Files

*.ntkdigunit* files are simple zip files containing the binary payload for the custom oscillator or effect and a metadata file.
They can be loaded onto a [NuTekt NTS-1 digital](https://www.korg.com/products/dj/nts_1) using the [logue-cli utility](../../tools/logue-cli/) or the [Librarian application](https://www.korg.com/products/dj/nts_1/librarian_contents.php).

## Demo Code

### Waves

Waves is a morphing wavetable oscillator that uses the wavetables provided by the custom oscillator API. It is a good example of how to use API functions, declare edit menu parameters and use parameter values of various types. See [demos/waves/](demos/waves/) for code and details.

## Creating a New Project

1. Create a copy of a template project for the module you are targetting (osc/modfx/delfx/revfx) and rename it to your convenience.
2. Make sure the PLATFORMDIR variable at the top of the *Makefile* is set to point to the [platform/](../) directory. This path will be used to locate external dependencies and required tools.
3. Set your desired project name in *project.mk*. See the [project.mk](#project.mk) section for syntax details.
4. Add source files to your new project and edit the *project.mk* file accordingly so that they are found during builds.
5. Edit the *manifest.json* file and set the appropriate metadata for your project. See the [manifest.json](#manifest.json) section for syntax details.

## Project Structure

### manifest.json

The manifest file consists essentially of a json dictionary with the following fields:

* platform (string) : Name of the target platform, should be set to *nutekt-digital*
* module (string) : Keyword for the module targetted, should be one of the following: *osc*, *modfx*, *delfx*, *revfx*
* api (string) : API version used. (format: MAJOR.MINOR-PATCH)
* dev_id (int) : Developer ID, currently unused, set to 0
* prg_id (int) : Program ID, currently unused, can be set for reference.
* version (string) : Program version. (format: MAJOR.MINOR-PATCH)
* name (string) : Program name. (will be displayed on the minilogue xd)
* num_params (int) : Number of parameters in edit menu. Only used for *osc* type projects, should be 0 for custom effects. Oscillators can have up to 6 custom parameters.
* params (array) : Parameter descriptors. Only meaningful for *osc* type projects. Set to an empty array otherwise.

Parameter descriptors are themselves arrays and should contain 4 values:

0. name (string) : Up to about 10 characters can be displayed in the edit menu of the minilogue xd.
1. minimum value (int) : Value should be in -100,100 range.
2. maximum value (int) : Value should be in -100,100 range and greater than minimum value.
3. type (string) : "%" indicates a percentage type, an empty string indicates a typeless value. 

In the case of typeless values, the minimum and maximum values should be positive. Also the displayed value will be offset by 1 such that a 0-9 range will be shown as 1-10 to the minilogue xd user. 

Here is an example of a manifest file:

```
{
    "header" : 
    {
        "platform" : "nutekt-digital",
        "module" : "osc",
        "api" : "1.1-0",
        "dev_id" : 0,
        "prg_id" : 0,
        "version" : "1.0-0",
        "name" : "waves",
        "num_param" : 6,
        "params" : [
            ["Wave A",      0,  45,  ""],
            ["Wave B",      0,  43,  ""],
            ["Sub Wave",    0,  15,  ""],
            ["Sub Mix",     0, 100, "%"],
            ["Ring Mix",    0, 100, "%"],
            ["Bit Crush",   0, 100, "%"]
          ]
    }
}
```

### project.mk

This file is included by the main Makefile to simplify customization.

The following variables are declared:

* PROJECT : Project name. Will be used in the filename of build products.
* UCSRC : C source files.
* UCXXSRC :  C++ source files.
* UINCDIR : Header search paths.
* UDEFS : Custom gcc define flags.
* ULIB : Linker library flags.
* ULIBDIR : Linker library search paths.

### tests/

The *tests/* directory contains very simple code that illustrates how to write oscillators and effects. They are not meant to do anything useful, nor claim to be optimized. You can refer to these as examples of how to use the different interfaces and test your setup.

### Troubleshooting

#### Can't compile, missing CMSIS arm_math.h

The CMSIS submodule is likely not initialized or up to date. Make sure to run `git submodule update --init` to initialize and update all submodules.

## TODO

* clean up doxygen documentation and generate.
* remove hard dependency on Cortex-M4 primitives from CMSIS headers. allow intel native builds.
* emscripten builds.
* Native Intel simulator runtime for better development workflows.

