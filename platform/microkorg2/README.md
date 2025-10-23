## microKORG2 SDK Source and Template Projects

[日本語](./README_ja.md)

### Overview

This directory contains all source files needed to build custom oscillators and effects for the [microKORG2](https://www.korg.com/jp/products/synthesizers/microkorg2/) synthesizer.

#### Compatibility Notes

Firmware version >= 2.0.0 is required to run user units built with SDK version 2.1.0.

#### Directory Structure:

 * [common/](common/) : Common headers.
 * [ld/](ld/) : Common linker files.
 * [dummy-osc/](dummy-osc/) : Oscillator project template.
 * [dummy-modfx/](dummy-modfx/) : Modulation effect effect project template.
 * [dummy-delfx/](dummy-delfx/) : Delay effect project template.
 * [dummy-revfx/](dummy-revfx/) : Reverb effect project template.
 * [MorphEQ/](MorphEQ/) : A three band EQ in which all three bands can be controlled simultaneously.
 * [Vibrato/](Vibrato/) : A simple vibrato plugin with some silly features.
 * [MultitapDelay/](MultitapDelay/) : A multitap delay that can morph between stereo and ping-pong modes.
 * [breveR/](breveR/) : A reverb based off of Schroeder's landmark paper, with an option to reverse the pre-delay line.
 * [waves/](waves/) : A simple wavetable oscillator.
 * [vox/](vox/) : A vocal formant oscillator.

### Platform Specifications

* *CPU*: ARM Cortex-A7 (i.mx6ulz)
* *Unit Format*: ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamically linked

#### Supported Modules

| Module | Slots | Max Unit Size | Max RAM Load Size | Allocatable External Memory |
|--------|-------|---------------|-------------------|-----------------------------|
| osc    | 32    | ~ 48KB        | 48KB              | 8KB                         |
| modfx  | 32    | ~ 16KB        | 16KB              | 64KB                        |
| delfx  | 32    | ~ 24KB        | 24KB              | 1MB                         |
| revfx  | 32    | ~ 24KB        | 24KB              | 1MB                         |

### Setting up the Development Environment

#### Docker-based Build Environment

 Refer to [Docker-based Build Environment](../../docker).

### Building Units 

#### Using Docker-based Build Environment

 1. Execute [docker/run_interactive.sh](../../docker/run_interactive.sh)

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```

 1.1. (optional) List buildable projects

```
 user@logue-sdk $ build -l --microkorg2
- microkorg2/MorphEQ
- microkorg2/MultitapDelay
- microkorg2/ReverseReverb
- microkorg2/Vibrato
- microkorg2/dummy-delfx
- microkorg2/dummy-modfx
- microkorg2/dummy-osc
- microkorg2/dummy-revfx
- microkorg2/vox
- microkorg2/waves
 ```

 2. Use the build command with the the desired project's path (E.g. `microkorg2/dummy-modfx`)

```
 user@logue-sdk $ build microkorg2/dummy-synth
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Building /workspace/microkorg2/dummy-modfx
Compiler Options
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -std=c11 -W -Wall -Wextra -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.c -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fconcepts -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -fno-threadsafe-statics -std=gnu++14 -W -Wall -Wextra -Wno-ignored-qualifiers -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.cc -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ xxxx.o -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -shared -Wl,-Map=build/dummy.map,--cref -lm -lc -o build/dummy.elf

Compiling header.c
Compiling _unit_base.c
Compiling unit.cc

Linking build/dummy.mk2unit
Stripping build/dummy.mk2unit
Creating build/dummy.hex
Creating build/dummy.bin
Creating build/dummy.dmp
Creating build/dummy.list


Done

   text	   data	    bss	    dec	    hex	filename
   4587	    352	     80	   5019	   139b	build/dummy.mk2unit
>> Installing /workspace/microkorg2/dummy-modfx
Deploying to /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
```

##### `run_cmd.sh` Alternative

 1. Build the desired project by specifying its path to `run_cmd.sh` (E.g. `microkorg2/dummy-modfx`)

```
 $ ./run_cmd.sh build microkorg2/waves
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Building /workspace/microkorg2/dummy-modfx
Compiler Options
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -std=c11 -W -Wall -Wextra -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.c -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -pipe -ffast-math -fsigned-char -fno-stack-protector -fstrict-aliasing -falign-functions=16 -fno-math-errno -fconcepts -fomit-frame-pointer -finline-limit=9999999999 --param max-inline-insns-single=9999999999 -fno-threadsafe-statics -std=gnu++14 -W -Wall -Wextra -Wno-ignored-qualifiers -fPIC -Wa,-alms=build/lst/ -D__arm__ -D__cortex_a7__ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.cc -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-gcc -c -march=armv7-a -mtune=cortex-a7 -marm -fPIC -Wa,-amhls=build/lst/ -MMD -MP -MF .dep/build.d -I. -I/workspace/microkorg2/common -I/workspace/common xxxx.s -o build/obj/xxxx.o
arm-unknown-linux-gnueabihf-g++ xxxx.o -march=armv7-a -mtune=cortex-a7 -marm -Os -flto -mfloat-abi=hard -mfpu=neon-vfpv4 -mvectorize-with-neon-quad -ftree-vectorize -ftree-vectorizer-verbose=4 -funsafe-math-optimizations -shared -Wl,-Map=build/dummy.map,--cref -lm -lc -o build/dummy.elf

Compiling header.c
Compiling _unit_base.c
Compiling unit.cc

Linking build/dummy.mk2unit
Stripping build/dummy.mk2unit
Creating build/dummy.hex
Creating build/dummy.bin
Creating build/dummy.dmp
Creating build/dummy.list


Done

   text	   data	    bss	    dec	    hex	filename
   4587	    352	     80	   5019	   139b	build/dummy.mk2unit
>> Installing /workspace/microkorg2/dummy-modfx
Deploying to /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
```

##### Final Build Product

The final build product is the *.mk2unit* file in the project directory (unless an install location was specified via build scripts).

### Cleaning Units

 Cleaning unit projects will remove temporary and final build products.

 1. Execute [docker/run_interactive.sh](../../docker/run_interactive.sh)

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
 2. Clean the desired project (E.g. `microkorg2/dummy-modfx`) 

```
 user@logue-sdk:~$ build --clean microkorg2/dummy-modfx 
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Cleaning /workspace/microkorg2/dummy-modfx

Cleaning
rm -fR .dep build /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
 ```

##### `run_cmd.sh` Alternative

 1. Clean the desired project (E.g. `microkorg2/dummy-modfx`) 

```
 $ ./run_cmd.sh build --clean microkorg2/dummy-modfx 
>> Initializing microkorg2 development environment.
Note: run 'env -r' to reset the environment
>> Cleaning /workspace/microkorg2/dummy-modfx

Cleaning
rm -fR .dep build /workspace/microkorg2/dummy-modfx//dummy.mk2unit

Done

>> Resetting environment
>> Cleaning up microkorg2 development environment.
```

### Using *unit* Files

*.mk2unit* files can be loaded onto a [microKORG2](https://www.korg.com/jp/products/synthesizers/microkorg2) via USB Mass Storage mode, the [microKORG2 Editor]() (pending release), or the new [loguecli]() (pending release).

Loaded units will appear in slot number order at the end of their respective module's selection list on the device.

*TIP* microKORG2 will remember the unit by its dev/unit ID and name in the current program, so units can be freely moved around slots.

## Creating a New Project

1. Create a copy of a template project directory for the module you are targetting (osc/modfx/delfx/revfx) and rename it to your convenience inside the *platform/microkorg2/* directory.
2. Customize your project build by editing *config.mk*. See the [config.mk](#config-mk) section for details.
3. Adapt the provided *header.c* template to match your project needs. See the [header.c](#header-c) section for details.
4. Adapt the provided *unit.cc* template to integrate your code with the unit API. See the [unit.cc](#unit-cc) section for details.

## Project Structure

### config.mk

The *config.mk* file allows to declare project source files, includes, libraries and override some build parameters without editing the Makefile directly.

By default the following variables are defined or readily available to be set:

 * `PROJECT` : Project name. Will be used in the file name of the final build project. (Note: does not define the actual name of the unit as displayed on the device when loaded)
 * `PROJECT_TYPE` : Determines which type of project is being built. Valid values are `osc`, `modfx`, `delfx`, and `revfx`. Make sure it matches the unit type you are developing.
 * `CSRC` : C source files for the project. This list should at least include the `header.c` file.
 * `CXXSRC` : C++ source files for the project. This list should at least inclde the `unit.cc` file.
 * `UINCDIR` : List of additional include directories.
 * `ULIBDIR` : List of additional library search directories.
 * `ULIBS` : List of additional library flags.
 * `UDEFS` : List of additional compile time defines. (e.g.: `-DENABLE\_MY\_FEATURE`)

### header.c

The *header.c* file is similar to the role played by the *manifest.json* file on older platforms. It provides some metadata information about the unit and the parameter it exposes. It is compiled along with the unit code and placed in a dedicated ELF section called *.unit_header*.

Field descriptions:

 * `.header_size` : Size of the header structure. Should be left as defined in the template provided.
 * `.target` : Defines the target platform and module. The value provided by the template should be kept, but make sure that the module defined matches the actual intended target unit module. (osc: k\_unit\_module\_osc, modfx: k\_unit\_module\_modfx, delfx: k\_unit\_module\_delfx, revfx: k\_unit\_module\_revfx)
 * `.api` : logue SDK API version against which the unit is being built. The default template value ensures the current API value at build time will be used.
 * `.dev_id` : A unique developer identifier as a low endian 32-bit unsigned integer. See [Developer Identifier](#developer-identifier) for details.
 * `.unit_id` : An identifier for the unit itself as a low endian 32-bit unsigned integer. This identifier must only be unique within the scope of a given developer identifier.
 * `.version` : The version for the current unit as a low endian 32-bit unsigned integer, with major in the upper 16 bits, minor and patch number in the two lower bytes, respectively. (e.g.: v1.2.3 -> 0x00010203U)
 * `.name` : Name for the current unit, as displayed on the device when loaded. Nul-terminated array of maximum 8 7-bit ASCII characters. Valid characters are: "`` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._!"#$%&'()*+,/:;<=>?@[]^`~``".
 * `.num_params` : Number of exposed parameters by the unit. This value depends on the target module. Refer to UNIT\_XXX\_MAX\_PARAM\_COUNT in common headers for exact value, where XXX is the target module.
 * `.params` : Array of parameter descriptors. See [Parameter Descriptors](#parameter-descriptors) for details. 
 
### unit.cc
 
 The *unit.cc* file is the main interface with the logue SDK API, it provides entry point implementations for the necessary API functions, and holds globally defined state and/or class instances.

### Developer Identifier

 Developers must choose a unique identifier (32-bit unsigned integer) in order to allow proper identification of units.
 A list of known identifiers is available [here](../../developer_ids.md), it is however not necessarily exhaustive.
 
 *Note* The following developer identifiers are reserved and should not be used: 0x00000000, 0x4B4F5247 (KORG), 0x6B6F7267 (korg), or any upper/lower case combination of the previous two.

### Parameter Descriptors

 Parameter descriptors are defined as part of the [header](#headerc) structure. They allow to name parameters, describe their value range and control how the parameter values are interpreted and displayed.

 ```
 typedef struct unit_param {
  int16_t min;
  int16_t max;
  int16_t center;
  int16_t init;
  uint8_t type;
  uint8_t frac : 4;       // fractional bits / decimals according to frac_mode
  uint8_t frac_mode : 1;  // 0: fixed point, 1: decimal
  uint8_t reserved : 3;
  char name[UNIT_PARAM_NAME_LEN + 1];
 } unit_param_t;
 ```
 
 * `min` and `max` are used to define parameter value boundaries.
 * `center` can be used to make it explicit that the parameter is bipolar, for unipolar parameters simply set it to the `min` value.
 * `init` is the initialization value. Unit parameters are expected to be set to this value after the initialization phase.
 * `type` allows to control how the parameter value is displayed, see [Parameter Types](#parameter-types) below for details.
 * `frac` allows to specify the fractional part of the parameter value. This value will be interpreted as number of fractional bits or decimals depending on the `frac_mode` value.
 * `frac_mode` determines the type of fractional value being described. When set to `0`, values will be assumed to be fixed point with the lower `frac` bits representing the fractional part. When set to `1`, values will be assumed to include a fractional part that is multiplied by 10 times `frac` number of decimals, allowing for base 10 fractional increment/decrements.
 * `reserved` should be set to 0 at all times.
 * `name` allows for a 8 character name. Should be nul-terminated and 7-bit ASCII encoded. Valid characters are: "`` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._!"#$%&'()*+,/:;<=>?@[]^`~``".
 
 *Note* `min`, `max`, `center` and `init` values must take into account the `frac` and `frac_mode` values.

 Even when the number of parameter count is less than the maximum allowed, a descriptor must be provided for each parameter. In order to indicate that a parameter index is not in use, the following parameter descriptor must be used:
 
 ```
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
 ```
 
##### Oscillator Parameters
 
 Up to 13 parameters are accessible via the OSC1/2/3 pages. They are assigned in order from left to right, page 1 ~ 3.

### Oscillator modulation

 Oscillators receive a buffer of 10 modulation sources once per frame, with individual modulation values for up to eight voices per modulation source (depending on the current polyphony). These values can be accessed using the helper functions `GetModDepth` and `GetModData`. To access the modulation data, add `kMk2PlatformExclusiveModData` as a case in the `unit_platform_exclusive` function. To display a name for your custom modulation destination in the virtual patch, add `kMk2PlatformExclusiveModDestName` as a case in `unit_platform_exclusive` and write the name in the buffer provided by `GetModDestNameData`. See [vox](vox/vox.h) or [vox](waves/waves.h) for examples. 
 
##### Effect Parameter Behavior

 By default, the effect parameters match the behavior of the internal fx. This means that when changing fx, the current page 1 parameters will be applied to the target effect, while the page 2 parameters are completely independent. It also means that the three page 1 parameteters are available as modulation destinations in the virtual patch. Both of these behaviors can be controlled by changing the setting of the reserved parameter in the header, as described below.

|                    Enum                      | Value |   Behavior   |
|----------------------------------------------|-------|--------------|
| kMk2FxParamModeBasic                         |     0 | The same as internal fx. Page 1 parameter values are copied from previously selected fx, and parameters are modulatable via the virtual patch. |
| kMk2FxParamModeIgnoreKnobState               |     1 | Page 1 parameter values are _not_ copied from the previously selected fx, and parameters are modulatable via the virtual patch. |
| kMk2FxParamModeIgnoreModulation              |     2 | Page 1 parameter values are copied from the previously selected fx, and parameters are _not_ modulatable via the virtual patch. |
| kMk2FxParamModeIgnoreKnobStateAndModulation  |     3 | Page 1 parameter values are _not_ copied from the previously selected fx, and parameters are _not_ modulatable via the virtual patch. |

##### Modulation Effect Parameters
 
 Up to 8 parameters are accessible via the MOD and EXTRA pages of the Mod fx. The three MOD parameters can be modulated via the virtual patch, and will inherit the parameter values of the previously selected effect (the same behavior as the internal effects) unless otherwise specified by the "reserved" bit in the header. The kMk2FxParamModeXX enums in FxDefines.h can be used to make this configuration more transparent.

##### Delay Effect Parameters
 
 Up to 8 parameters are accessible via the DELAY and EXTRA pages of the Delay fx. The three DELAY parameters can be modulated via the virtual patch, and will inherit the parameter values of the previously selected effect (the same behavior as the internal effects) unless otherwise specified by the "reserved" bit in the header. The kMk2FxParamModeXX enums in FxDefines.h can be used to make this configuration more transparent.

##### Reverb Effect Parameters
 
 Up to 8 parameters are accessible via the REVERB and EXTRA pages of the Reverb fx. The three REVERB parameters can be modulated via the virtual patch, and will inherit the parameter values of the previously selected effect (the same behavior as the internal effects) unless otherwise specified by the "reserved" bit in the header. The kMk2FxParamModeXX enums in FxDefines.h can be used to make this configuration more transparent.


#### Parameter Types

 The following parameter types are available.
 
  * `k_unit_param_type_none` : Describes a typeless value. The value will be displayed as is, while taking into account the fractional part.
  * `k_unit_param_type_percent` : Describe a percent value. 
  * `k_unit_param_type_db` : Describes a decibel value. 
  * `k_unit_param_type_cents` : Describes a pitch cents value. 
  * `k_unit_param_type_semi` : Describes a pitch semitone value. 
  * `k_unit_param_type_oct` : Describes a pitch octave value. 
  * `k_unit_param_type_hertz` : Describes a Hertz value. 
  * `k_unit_param_type_khertz` : Describes a kilo Hertz value. 
  * `k_unit_param_type_bpm` : Describes a beat per minute value.
  * `k_unit_param_type_msec` : Describes a milliseconds value. 
  * `k_unit_param_type_sec` : Describes a seconds value. 
  * `k_unit_param_type_enum` : Describes a numerical enumeration value. If the value minimum is set to 0, the value will be incremented by 1 when displayed. 
  * `k_unit_param_type_strings` : Describes a value with custom string representation. The numerical value will be passed in a call to `unit_get_param_str_value(..)` in order to obtain the string representation. See [Strings](#strings) for details.
  * `k_unit_param_type_drywet` : Describes a dry/wet value. Negative values will be prepended with `D` for dry, positive values with `W` for wet, and zero value replaced with `BALN` to indicate a balanced mix.
  * `k_unit_param_type_pan` : Describes a stereo pan value. Negative values will be prepended with `L` for left, positive values with `R` for right, and zero value replaced with `CNTR` to indicate centered panning.
  * `k_unit_param_type_spread` : Describes a stereo spread value. Negative values will be prepended with `L` for left, positive values with `R` for right, and zero value replaced with `CNTR` to indicate no stereo spread.
  * `k_unit_param_type_onoff`: Describes an on/off toggle value. `0` will be displayed as `off`, and `1` will be displayed as `on`.
  * `k_unit_param_type_midi_note` : Describes a MIDI note value. The numerical note value will be displayed as musical pitches (e.g.: `C0`, `A3`).

## Unit API Overview

Here's an overview of the API for osc and effect units.

### Essential Functions

All units must provide an implementation for the following functions. However, a fallback implementation is provided by default, so only the relevant functions needs to be explicitely provided.

 * `__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc)` : Called after unit is loaded. Should be used to perform basic checks on runtime environment, initialize the unit, and allocate any external memory if needed. `desc` provides a description of the current runtime environment (See [Runtime Descriptor](#runtime-descriptor) for details).
 * `__unit_callback void unit_teardown()` : Called before unit is unloaded. Should be used to perform cleanup and freeing memory if needed.
 * `__unit_callback void unit_reset()` : Called when unit must be reset to a neutral state. Active notes must be deactivated, enveloppe generators reset to their neutral state, oscillator phases reset, delay lines set to be cleared (clearing at once may be to heavy). However, parameter values *should not be* reset to their default values.
 * `__unit_callback void unit_resume()` : Called just before a unit will start processing again after being suspended.
 * `__unit_callback void unit_suspend()` : Called when a unit is being suspended. For instance, when the currently active unit is being swapped for a different unit. Usually followed by a call to `unit_reset()`.
 * `__unit_callback void unit_render(const float * in, float * out, uint32_t frames)` : Audio rendering callback. Input/output buffer geometry information is provided via the `unit_runtime_desc_t` argument of `unit_init(..)`.
 * `__unit_callback int32_t unit_get_param_value(uint8_t index)` : Called to obtain the current value of the parameter designated by the specified index.
 * `__unit_callback const char * unit_get_param_str_value(uint8_t index, int32_t value)` : Called to obtain the string representation of the specified value, for a `k_unit_param_type_strings` typed parameter. The returned value should point to a nul-terminated 7-bit ASCII C string of maximum X characters. It can be safely assumed that the C string pointer will not be cached or reused until `unit_get_param_str_value(..)` is called again, and thus the same memory area can be reused across calls (if convenient).
 * `__unit_callback void unit_set_param_value(uint8_t index, int32_t value)` : Called to set the current value of the parameter designated by the specified index. Note that for the microKORG2 values are stored as 16-bit integers, but to avoid future API changes, they are passed as 32bit integers. For additional safety, make sure to bound check values as per the min/max values declared in the header.
 * `__unit_callback void unit_set_tempo(uint32_t tempo)` : Called when a tempo change occurs. The tempo is formatted in fixed point format, with the BPM integer part in the upper 16 bits, and fractional part in the lower 16 bits (low endian). Care should be taken to keep CPU load as low as possible when handling tempo changes as this handler may be called frequently especially if externally synced.
 
### Optional Functions
 * `__unit_callback void unit_platform_exclusive(uint8_t messageId, void * data, uint32_t dataSize)` : Functions specific to the microKORG2, including virtual patch modulation for oscillator plugins

### Runtime Descriptor 

 A reference to the runtime descriptor is passed to units during the initialization phase. The descriptor provides information on the current device and API, audio rate and buffer geometry, as well as pointers to callable API functions.

 ```
 typedef struct unit_runtime_desc {
  uint16_t target;
  uint32_t api;
  uint32_t samplerate;
  uint16_t frames_per_buffer;
  uint8_t input_channels;
  uint8_t output_channels;
  unit_runtime_hooks_t hooks;
 } unit_runtime_desc_t;
 ```
 
 * `target` describes the current platform and module. It should be set to `k_unit_target_microkorg2` OR'd with one of the following: `k_unit_module_osc`, `k_unit_module_modfx`, `k_unit_module_delfx`, `k_unit_module_revfx`. The convenience macro `UNIT_TARGET_PLATFORM_IS_COMPAT(target)` can be used to check for compatibility of current unit with the runtime environment.
 * `api` describes the API version currently in use. The version is formatted with the major in the upper 16 bits, minor and patch number in the two lower bytes, respectively (e.g.: v1.2.3 -> 0x00010203U). The convenience macro `UNIT_API_IS_COMPAT(api)` can be used to check for compatibility of current unit with the runtime environment API.
 * `samplerate` describes the sample rate used for audio processing. On the microKORG2 this should always be set to 48000. However it should be checked and taken into account by the unit. A unit can reject inconvenient samplerates by returning `k_unit_err_samplerate*` from the `unit_init(..)` callback, which will prevent the unit from being fully loaded.
 * `frames_per_buffer` describes the maximum number of frames per audio processing buffer that should be expected in the `unit_render(..)` callback. In general it should always be equal to the `frames` argument of the callback, however, smaller values should be properly supported nonetheless.
 * `input_channels` describes the number of audio channels (samples) per frame in the input buffer of the `unit_render(..)` callback.
 * `output_channels` describes the number of audio channels (samples) per frame in the output buffer of the `unit_render(..)` callback.
 * `hooks` gives access to additional context and callable APIs presented by the runtime environment. See [Runtime Context](#runtime-context) and [Callable API Functions](#callable-api-functions) below.

#### Runtime Context

The oscillator runtime provides realtime information to the oscillator:

```
  /** Oscillator specific unit runtime context. */
  typedef struct unit_runtime_osc_context 
  {
    float pitch[kMk2MaxVoices]; 
    uint8_t trigger; // bit array
    float * unitModDataPlus; 
    float * unitModDataPlusMinus; 
    uint8_t modDataSize;
    uint16_t bufferOffset;
    uint8_t voiceOffset;
    uint8_t voiceLimit;
    uint16_t outputStride;
  } unit_runtime_osc_context_t;
```

 * `pitch[kMk2MaxVoices]` : An array of eight floats that define the pitch for the corresponding voices. 
 * `trigger` : A bit array in which each bit is set to high when the corresponding voice is triggered
 * `unitModDataPlus` : Optional modulation buffer for the user to write per-voice modulation signals into, normalized to 0 ~ 1.
 * `unitModDataPlusMinus` : Optional modulation buffer for the user to write per-voice modulation signals into, normalized to -1 ~ 1.
 * `modDataSize` : The size of the unitModData buffers.
 * `bufferOffset` : The offset relative to the start of the oscillator output buffer
 * `voiceOffset` : The offset relative to the starting voice for this unit
 * `voiceLimit` : The number of voices this unit is limited to
 * `outputStride` : How many output streams are expected to be interlaced

Other module runtimes do not provide specific realtime information.

#### Other Callable API Functions

 * `sdram_alloc` (`uint8_t *sdram_alloc(size_t size)`) allocates buffers of external memory. The allocation should be done during initialization of the unit. (Recommended: ensure 4 byte alignment)
 * `sdram_free` (`void sdram_free(const uint8_t *mem)`) frees external memory buffer previously allocated via `sdram_alloc`.
 * `sdram_avail` (`size_t sdram_avail()`) returns the currently available amount of external memory.

### Strings

 Strings provided via `unit_get_param_str_value(..)` should be nul terminated C character arrays of 7-bit ASCII characters from the following list: "`` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._!"#$%&'()*+,/:;<=>?@[]^`~``".

#### Oscillator Output
 For efficiency, the microKORG2 runs one oscillator instance per timbre and expects the output to be written into the provided buffer as interlaced data. The number of voices expected, buffer location, and interlacing are all defined by the data provided in the oscillator runtime context. There are helper functions `write_oscillator_output_x1`/`write_oscillator_output_x2`/`write_oscillator_output_x4` and `GetBufferOffset` that handle the necessary calculations and writing. See `waves.h` or `vox.h` for examples. 

 The microKORG2 has a maximum of eight voices, processed with as much parallelization as possible. However, depending on settings like Timbre Mode and Vocoder on/off the maximum number of voices can change, in turn affecting how each voice is interlaced. The chart below describes this behavior in detail.

##### Single Timbre | Vocoder Off | Hard Tune/Harmonizer Off
 8 voices, interlaced in groups of 4
| Timbre | Voices | bufferOffset | voiceOffset | voiceLimit | outputStride |
|--------|--------|--------------|-------------|------------|--------------|
| 1      | 0 ~ 3  | 0            | 0           | 8          | 4            |
| 1      | 5 ~ 7  | 256          | 0           | 8          | 4            |

##### Dual Timbre | Vocoder Off | Hard Tune/Harmonizer Off
 8 voices, interlaced in groups of 4
| Timbre | Voices | bufferOffset | voiceOffset | voiceLimit | outputStride |
|--------|--------|--------------|-------------|------------|--------------|
| 1      | 0 ~ 3  | 0            | 0           | 4          | 4            |
| 2      | 5 ~ 7  | 256          | 0           | 4          | 4            |

##### Single Timbre | Vocoder On / Keyboard Mode | Hard Tune/Harmonizer Off
 4 voices, interlaced in groups of 4
| Timbre | Voices | bufferOffset | voiceOffset | voiceLimit | outputStride |
|--------|--------|--------------|-------------|------------|--------------|
| 1      | 0 ~ 3  | 0            | 0           | 4          | 4            |

##### Dual Timbre | Vocoder On / Keyboard Mode | Hard Tune/Harmonizer Off
 4 voices, interlaced in groups of 2
| Timbre | Voices | bufferOffset | voiceOffset | voiceLimit | outputStride |
|--------|--------|--------------|-------------|------------|--------------|
| 1      | 0 ~ 1  | 0            | 0           | 2          | 4            |
| 2      | 2 ~ 3  | 0.           | 2           | 2          | 4            |

##### Single Timbre | Vocoder On / Scale Mode | Hard Tune/Harmonizer Off
 2 voices, interlaced in groups of 2
| Timbre | Voices | bufferOffset | voiceOffset | voiceLimit | outputStride |
|--------|--------|--------------|-------------|------------|--------------|
| 1      | 0 ~ 1  | 0            | 0           | 2          | 2            |

##### Dual Timbre | Vocoder On / Scale Mode | Hard Tune/Harmonizer Off
 2 voices, interlaced in groups of 1
| Timbre | Voices | bufferOffset | voiceOffset | voiceLimit | outputStride |
|--------|--------|--------------|-------------|------------|--------------|
| 1      | 0 ~ 1  | 0            | 0           | 1          | 2            |
| 2      | 2 ~ 3  | 0            | 1           | 1          | 2            |

##### Any Timbre Setting | Vocoder N/A | Hard Tune/Harmonizer On
 Oscillators are not processed when the Hard Tune or Harmonizer is on.