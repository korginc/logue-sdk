---
layout: page
title: Effects
permalink: /doc/fx/
---

## Creating a New Effects Project

1. Create a copy of a template effects project directory (modfx/, delfx/, or revfx/) and rename it to your convenience.
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
        "module" : "modfx",
        "api" : "1.1-0",
        "dev_id" : 0,
        "prg_id" : 0,
        "version" : "1.0-0",
        "name" : "my modfx",
        "num_param" : 0,
        "params" : []
    }
}
```

* platform (string) : Name of the target platform. It should be set to *prologue*, *minilogue-xd* or *nutekt-digital*, according to the platform you are building for.
* module (string) : Keyword for the module targetted. It should be set to *modfx*, *delfx*, or *revfx*, according to the module you are targetting.
* api (string) : API version used. (format: MAJOR.MINOR-PATCH)
* dev_id (int) : Developer ID, currently unused, set to 0
* prg_id (int) : Program ID, currently unused, can be set for reference.
* version (string) : Program version. (format: MAJOR.MINOR-PATCH)
* name (string) : Program name. Up to 12 characters. This will be displayed on the device.
* num_params (int) : Unused for effects, should be set to 0.
* params (array) : Unused for effects, should be set to empty ([]). 

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

Here's an overview of the core API for effects in the logue SDK.

### Modulation Effects API (modfx)

Your main implementation file should include `usermodfx.h` and implement the following functions.

* `void MODFX_INIT(uint32_t platform, uint32_t api)`: Called on instantiation of the effect. Use this callback to perform any required initializations. See `inc/userprg.h` for possible values of platform and api.

* `void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn,  float *sub_yn, uint32_t frames)`: This is where you should process the input samples. This function is called every time a buffer of *frames* samples is required (1 sample per frame).  *\*\_xn* buffers denote inputs and *\*\_yn* denote output buffers, where you should write your results. 

    _Note: There are *main\_* and *sub\_* versions of the inputs and outputs in order to support the dual timbre feature of the prologue. On the prologue, both main and sub should be processed the same way in parallel. On other non-multitimbral platforms you can ignore *sub\_xn* and *sub\_yn*_

    _Note: Buffer lengths up to 64 frames should be supported. However you can perform multiple passes on smaller buffers if you preffer. (Optimize for powers of two: 16, 32, 64)_

* `void MODFX_PARAM(uint8_t index, uint32_t value)`: Called whenever the user changes a parameter value.

For more details see the [Modulation Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__modfx__inst.html). Also see the [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__utils.html) and [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/namespacedsp.html) for useful primitives.

### Delay Effects API (delfx)

Your main implementation file should include `userdelfx.h` and implement the following functions.

* `void DELFX_INIT(uint32_t platform, uint32_t api)`: Called on instantiation of the effect. Use this callback to perform any required initializations. See `inc/userprg.h` for possible values of platform and api.

* `void DELFX_PROCESS(float *xn, uint32_t frames)`: This is where you should process the input samples. This function is called every time a buffer of *frames* samples is required (1 sample per frame). In this case *xn* is both the input and output buffer. Your results should be written in place mixed with the appropriate amount of dry and wet signal (e.g.: set via the shift-depth parameter).

    _Note: Buffer lengths up to 64 frames should be supported. However you can perform multiple passes on smaller buffers if you preffer. (Optimize for powers of two: 16, 32, 64)_

* `void DELFX_PARAM(uint8_t index, uint32_t value)`: Called whenever the user changes a parameter value.

For more details see the [Delay Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__delfx__inst.html). Also see the [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__utils.html) and [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/namespacedsp.html) for useful primitives.

### Reverb Effects API (revfx)

Your main implementation file should include `userrevfx.h` and implement the following functions.

* `void REVFX_INIT(uint32_t platform, uint32_t api)`: Called on instantiation of the effect. Use this callback to perform any required initializations. See `inc/userprg.h` for possible values of platform and api.

* `void REVFX_PROCESS(float *xn, uint32_t frames)`: This is where you should process the input samples. This function is called every time a buffer of *frames* samples is required (1 sample per frame). In this case *xn* is both the input and output buffer. Your results should be written in place mixed with the appropriate amount of dry and wet signal (e.g.: set via the shift-depth parameter).

    _Note: Buffer lengths up to 64 frames should be supported. However you can perform multiple passes on smaller buffers if you preffer. (Optimize for powers of two: 16, 32, 64)_

* `void REVFX_PARAM(uint8_t index, uint32_t value)`: Called whenever the user changes a parameter value.

For more details see the [Reverb Effect Instance API reference](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__revfx__inst.html). Also see the [Effects Runtime API](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__fx__api.html), [Arithmetic Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/group__utils.html) and [Common DSP Utilities](https://korginc.github.io/logue-sdk/ref/prologue/v1.1-0/html/namespacedsp.html) for useful primitives.
