---
layout: page
title: Oscillators
permalink: /doc/osc/
---

## Creating a New Oscillator Project

1. Create a copy of a template oscillator project directory (osc/) and rename it to your convenience.
2. Make sure the PLATFORMDIR variable at the top of the *Makefile* is set to point to the *platform/* directory. This path will be used to locate external dependencies and required tools.
3. Set your desired project name in *project.mk*. See the [project.mk](#projectmk) section for syntax details.
4. Add source files to your new project and edit the *project.mk* file accordingly so that they are found during builds.
5. Edit the *manifest.json* file and set the appropriate metadata for your project. See the [manifest.json](#manifestjson) section for syntax details.

## Project Structure

### manifest.json

The manifest file consists essentially of a json dictionary like this one:

```
{
    "header" : 
    {
        "platform" : "prologue",
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

* platform (string) : Name of the target platform. It should be set to *prologue*, *minilogue-xd* or *nutekt-digital*, according to the platform you are building for.
* module (string) : Keyword for the module targetted. It should be set to *osc*, since we are making an oscillator.
* api (string) : API version used. (format: MAJOR.MINOR-PATCH)
* dev_id (int) : Developer ID, currently unused, set to 0
* prg_id (int) : Program ID, currently unused, can be set for reference.
* version (string) : Program version. (format: MAJOR.MINOR-PATCH)
* name (string) : Program name. Up to 12 characters. This will be displayed on the device.
* num_params (int) : Number of parameters in oscillator edit menu. Oscillators can have up to 6 edit parameters.
* params (array) : Edit parameter descriptors. 

Edit parameter descriptors are themselves arrays and should contain 4 values:

0. name (string) : Up to about 10 characters can be displayed in the edit menu of the prologue.
1. minimum value (int) : Value should be in -100,100 range.
2. maximum value (int) : Value should be in -100,100 range and greater than minimum value.
3. type (string) : "%" indicates a percentage type, an empty string indicates a typeless value. 

In the case of typeless values, the minimum and maximum values should be positive. Also the displayed value will be offset by 1 such that a 0-9 range will be shown as 1-10 to the prologue's user. 

### project.mk

This file is included by the main Makefile to simplify customization.

The following variables are available:

* PROJECT : Project name. Will be used in the filename of build products.
* UCSRC : C source files.
* UCXXSRC :  C++ source files.
* UINCDIR : Header search paths.
* UDEFS : Custom gcc define flags.
* ULIB : Linker library flags.
* ULIBDIR : Linker library search paths.

### tests/

The *tests/* directory contains very simple code that illustrates how to write oscillators and effects. They are not meant to do anything useful, nor optimized. You can refer to these as examples of how to use the different interfaces and test your setup.

## Core API

Here's an overview of the core API for oscillators in the logue SDK.

Your main implementation file should include *userosc.h* and implement the following functions.

* `void OSC_INIT(uint32_t platform, uint32_t api)`: Called on instantiation of the oscillator. Use this callback to perform any required initializations. See `inc/userprg.h` for possible values of platform and api.

* `void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames)`: This is where the waveform should be computed. This function is called every time a buffer of *frames* samples is required (1 sample per frame). Samples should be written to the *yn* buffer. Output samples should be formatted in [Q31 fixed point representation](https://en.wikipedia.org/wiki/Q_(number_format)). 

    _Note: Floating-point numbers can be converted to Q31 format using the `f32_to_q31(f)` macro defined in `inc/utils/fixed_math.h`. Also see `inc/userosc.h` for `user_osc_param_t` definition._

    _Note: Buffer lengths up to 64 frames should be supported. However you can perform multiple passes on smaller buffers if you prefer. (Optimize for powers of two: 16, 32, 64)_

* `void OSC_NOTEON(const user_osc_param_t * const params)`: Called whenever a note on event occurs.

* `void OSC_NOTEOFF(const user_osc_param_t * const params)`: Called whenever a note off event occurs.

* `void OSC_PARAM(uint16_t index, uint16_t value)`: Called whenever the user changes a parameter value.

For more details see the [Oscillator Instance API reference](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__osc__inst.html). Also see the [Oscillator Runtime API](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__osc__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__utils.html) and [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/namespacedsp.html) for useful primitives.
