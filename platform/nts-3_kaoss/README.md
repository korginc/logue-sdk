## Nu:Tekt NTS-3 kaoss pad kit SDK Source and Template Projects

[日本語](./README_ja.md)

### Overview

This directory contains all source files needed to build custom effects for the [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/products/dj/nts_3) synthesizer.

#### Compatibility Notes

Firmware version >= 1.0.0 is required to run user units built with SDK version 2.0.0.

#### Directory Structure:

 * [common/](common/) : Common headers.
 * [ld/](ld/) : Common linker files.
 * [dummy-genericfx/](dummy-genericfx/) : Project template for generic effects.

### Platform Specifications

* *CPU*: ARM Cortex-M7 (STM32H725)
* *Unit Format*: ELF 32-bit LSB shared object, ARM, EABI5 v1 (SYSV), dynamically linked
* *Toolchain*: gcc-arm-none-eabi-10.3-2021.10

#### Supported Modules

| Module    | Slots | Max Unit Size | Max RAM Load Size | Allocatable External Memory |
|-----------|-------|---------------|-------------------|-----------------------------|
| genericfx | 50    | ~ 32KB        | 32KB              | 3MB (per runtime)           |

##### Generic Effects

Generic effects on the NTS-3 kaoss pad kit do not have specific purpose designation like on other logue SDK platforms (e.g.: modfx, delfx, revfx, etc), they can implement any type of effect, and/or even be sound generators.
The NTS-3 kaoss pad kit provides four identical effect runtimes in which any generic effect unit can be loaded.
The same effect unit can be loaded multiple times using different runtimes.

### Setting up the Development Environment

#### Docker-based Build Environment

 Refer to [Docker-based Build Environment](../../docker).

#### Manual Method

 1. Clone this repository and initialize/update submodules.

```
 $ git clone https://github.com/korginc/logue-sdk.git
 $ cd logue-sdk
 $ git submodule update --init
 ```
 2. Install toolchain: [GNU Arm Embedded Toolchain](../../tools/gcc)
 3. Install other utilties:
    1. [GNU Make](../../tools/make)

### Building Units

#### Using Docker-based Build Environment

 1. Execute [docker/run_interactive.sh](../../docker/run_interactive.sh)

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```

 1.1. (optional) List buildable projects

```
 user@logue-sdk $ build -l --nts-3
 - nts-3_kaoss/dummy-genericfx
 ```

 2. Use the build command with the the desired project's path (E.g. `nts-3_kaoss/dummy-genericfx`)

```
 user@logue-sdk $ build nts-3_kaoss/dummy-genericfx
 >> Initializing NTS-3 kaoss development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/nts-3_kaoss/dummy-genericfx
 Compiler Options
 /usr/bin/arm-none-eabi-gcc -c -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -fPIC -std=c11 -fno-exceptions -W -Wall -Wextra -Wa,-alms=/workspace/nts-3_kaoss/dummy-genericfx//build/lst/ -DSTM32H725xE -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7 -D__FPU_PRESENT -I. -I/workspace/nts-3_kaoss/common -I/workspace/ext/CMSIS/CMSIS/Include
 
 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc
 Linking /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.elf
 /usr/bin/arm-none-eabi-gcc /workspace/nts-3_kaoss/dummy-genericfx//build/obj/header.o /workspace/nts-3_kaoss/dummy-genericfx//build/obj/_unit_base.o /workspace/nts-3_kaoss/dummy-genericfx//build/obj/unit.o -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -nostartfiles -Wl,-z,max-page-size=128,-Map=/workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.map,--cref,--no-warn-mismatch,--library-path=/workspace/nts-3_kaoss/dummy-genericfx//../ld,--script=/workspace/nts-3_kaoss/dummy-genericfx//../ld/unit.ld -shared --entry=0 -specs=nano.specs -specs=nosys.specs -lc -lm -o /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.elf
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.hex
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.dmp
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.bin
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.list
 
    text	   data	    bss	    dec	    hex	filename
    2595	    168	     60	   2823	    b07	/workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.elf
 
 Done

 >> Installing /workspace/nts-3_kaoss/dummy-genericfx
 Making /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.nts3unit
 Deploying to /workspace/nts-3_kaoss/dummy-genericfx//dummy_genericfx.nts3unit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-3 kaoss development environment.
 ```

##### `run_cmd.sh` Alternative

 1. Build the desired project by specifying its path to `run_cmd.sh` (E.g. `nts-3_kaoss/dummy-genericfx`)

```
 $ ./run_cmd.sh build nts-3_kaoss/dummy-genericfx
 >> Initializing NTS-3 kaoss development environment.
 Note: run 'env -r' to reset the environment
 >> Building /workspace/nts-3_kaoss/dummy-genericfx
 Compiler Options
 /usr/bin/arm-none-eabi-gcc -c -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -fPIC -std=c11 -fno-exceptions -W -Wall -Wextra -Wa,-alms=/workspace/nts-3_kaoss/dummy-genericfx//build/lst/ -DSTM32H725xE -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7 -D__FPU_PRESENT -I. -I/workspace/nts-3_kaoss/common -I/workspace/ext/CMSIS/CMSIS/Include
 
 Compiling header.c
 Compiling _unit_base.c
 Compiling unit.cc
 Linking /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.elf
 /usr/bin/arm-none-eabi-gcc /workspace/nts-3_kaoss/dummy-genericfx//build/obj/header.o /workspace/nts-3_kaoss/dummy-genericfx//build/obj/_unit_base.o /workspace/nts-3_kaoss/dummy-genericfx//build/obj/unit.o -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -nostartfiles -Wl,-z,max-page-size=128,-Map=/workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.map,--cref,--no-warn-mismatch,--library-path=/workspace/nts-3_kaoss/dummy-genericfx//../ld,--script=/workspace/nts-3_kaoss/dummy-genericfx//../ld/unit.ld -shared --entry=0 -specs=nano.specs -specs=nosys.specs -lc -lm -o /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.elf
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.hex
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.dmp
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.bin
 Creating /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.list
 
    text	   data	    bss	    dec	    hex	filename
    2595	    168	     60	   2823	    b07	/workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.elf
 
 Done

 >> Installing /workspace/nts-3_kaoss/dummy-genericfx
 Making /workspace/nts-3_kaoss/dummy-genericfx//build/dummy_genericfx.nts3unit
 Deploying to /workspace/nts-3_kaoss/dummy-genericfx//dummy_genericfx.nts3unit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-3 kaoss development environment.
 ```

##### Final Build Product

The final build product is the *.nts3unit* file in the project directory (unless an install location was specified via build scripts).

#### Using Legacy Method

 1. Move into the project directory.
 
```
$ cd logue-sdk/platform/nts-3_kaoss/dummy-genericfx/
```
 2. Run `make` to build the project.

```
$ make
Compiler Options
../../../tools/gcc/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc -c -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -fPIC -std=c11 -fno-exceptions -W -Wall -Wextra -Wa,-alms=build/lst/ -DSTM32H725xE -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7 -D__FPU_PRESENT -I. -I../common -I../../ext/CMSIS/CMSIS/Include

Compiling header.c
Compiling _unit_base.c
Compiling unit.cc
Linking build/dummy_genericfx.elf
../../../tools/gcc/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc build/obj/header.o build/obj/_unit_base.o build/obj/unit.o -mcpu=cortex-m7 -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT -g -Os -mlittle-endian -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new -nostartfiles -Wl,-z,max-page-size=128,-Map=build/dummy_genericfx.map,--cref,--no-warn-mismatch,--library-path=../ld,--script=../ld/unit.ld -shared --entry=0 -specs=nano.specs -specs=nosys.specs -lc -lm -o build/dummy_genericfx.elf
Creating build/dummy_genericfx.hex
Creating build/dummy_genericfx.bin
Creating build/dummy_genericfx.dmp

   text	   data	    bss	    dec	    hex	filename
   2595	    168	     60	   2823	    b07	build/dummy_genericfx.elf

Creating build/dummy_genericfx.list
Done
```
 3. Run `make install` to finalize and deploy unit file.
```
$ make install
Making build/dummy-genericfx.nts3unit
Deploying to ./dummy-genericfx.nts3unit
Done
```
 4. As the *Deploying...* line indicates, a *.nts3unit* file will be generated. This is the final product.
 
 *TIP*: The install directory can be defined by setting the `INSTALLDIR` environment variable.

### Cleaning Units

#### Using Docker-based Build Environment

 Cleaning unit projects will remove temporary and final build products.

 1. Execute [docker/run_interactive.sh](../../docker/run_interactive.sh)

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
 2. Clean the desired project (E.g. `nts-3_kaoss/dummy-genericfx`) 

```
 user@logue-sdk:~$ build --clean nts-3_kaoss/dummy-genericfx 
 >> Initializing NTS-3 kaoss development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/nts-3_kaoss/dummy-genericfx
 Cleaning
 rm -fR /workspace/nts-3_kaoss/dummy-genericfx//.dep /workspace/nts-3_kaoss/dummy-genericfx//build /workspace/nts-3_kaoss/dummy-genericfx//dummy_genericfx.nts3unit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-3 kaoss development environment.
 ```

##### `run_cmd.sh` Alternative

 1. Clean the desired project (E.g. `nts-3_kaoss/dummy-genericfx`) 

```
 $ ./run_cmd.sh build --clean nts-3_kaoss/dummy-genericfx 
 >> Initializing NTS-3 kaoss development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/nts-3_kaoss/dummy-genericfx
 Cleaning
 rm -fR /workspace/nts-3_kaoss/dummy-genericfx//.dep /workspace/nts-3_kaoss/dummy-genericfx//build /workspace/nts-3_kaoss/dummy-genericfx//dummy_genericfx.nts3unit
 Done
 
 >> Resetting environment
 >> Cleaning up NTS-3 kaoss development environment.
 ```

#### Using Legacy Method

 1. Move into the project directory.
 
```
$ cd logue-sdk/platform/nts-3_kaoss/dummy-genericfx/
```
 2. Run `make clean` to clean the project.

```
$ make clean
Cleaning
rm -fR .dep build dummy-genericfx.nts3unit
Done
```

### Using *unit* Files

*.nts3unit* files can be loaded onto a [Nu:Tekt NTS-3 kaoss pad kit](https://www.korg.com/products/dj/nts_3) via either the [KORG Kontrol Editor]() (pending release), or the new [loguecli]() (pending release).

Loaded units will appear in slot number order at the end of the effects selection list.

*TIP* NTS-3 kaoss pad kit will remember the unit by its dev/unit ID and name in the current program, so units can be freely moved around slots.

## Creating a New Project

1. Create a copy of a template project directory (`dummy-genericfx/`) and rename it to your convenience inside the *platform/nts-3_kaoss/* directory.
2. Customize your project build by editing *config.mk*. See the [config.mk](#config-mk) section for details.
3. Adapt the provided *header.c* template to match your project needs. See the [header.c](#header-c) section for details.
4. Adapt the provided *unit.cc* template to integrate your code with the unit API. See the [unit.cc](#unit-cc) section for details.

## Project Structure

### config.mk

The *config.mk* file allows to declare project source files, includes, libraries and override some build parameters without editing the Makefile directly.

By default the following variables are defined or readily available to be set:

 * `PROJECT` : Project name. Will be used in the file name of the final build project. (Note: does not define the actual name of the unit as displayed on the device when loaded)
 * `PROJECT_TYPE` : Determines which type of project is being built. It should be set to `genericfx`.
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
 * `.target` : Defines the target platform and module. The value provided by the template should be kept, but make sure that the module defined matches the actual intended target unit module. (k\_unit\_module\_genericfx)
 * `.api` : logue SDK API version against which the unit is being built. The default template value ensures the current API value at build time will be used.
 * `.dev_id` : A unique developer identifier as a low endian 32-bit unsigned integer. See [Developer Identifier](#developer-identifier) for details.
 * `.unit_id` : An identifier for the unit itself as a low endian 32-bit unsigned integer. This identifier must only be unique within the scope of a given developer identifier.
 * `.version` : The version for the current unit as a low endian 32-bit unsigned integer, with major in the upper 16 bits, minor and patch number in the two lower bytes, respectively. (e.g.: v1.2.3 -> 0x00010203U)
 * `.name` : Name for the current unit, as displayed on the device when loaded. Nul-terminated array of maximum 19 7-bit ASCII characters. Valid characters are: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_`".
 * `.num_params` : Number of exposed parameters by the unit. Should be set to 8. (See UNIT\_MAX\_PARAM\_COUNT in `common/runtime.h`).
 * `.params` : Array of parameter descriptors. See [Parameter Descriptors](#parameter-descriptors) for details. 
 
#### Default Parameter Mappings

The unit header for NTS-3 kaoss pad kit generic effects also include additional fields (`.default_mappings`) to specify the default mapping of each of the effects' parameters to the device's UI components.

A parameter mapping is defined as follows:

```
  typedef struct genericfx_param_mapping {
    uint8_t assign;
    uint8_t curve:7;
    uint8_t curve_polarity:1;
    int16_t min;
    int16_t max;
    int16_t value;
  } genericfx_param_mapping_t;
```

 * `.assign` : Defines the UI component to which the parameter should be mapped. Valid values are: k\_genericfx\_param\_assign\_none, k\_genericfx\_param\_assign\_x, k\_genericfx\_param\_assign\_y, and k\_genericfx\_param\_assign\_depth.
 * `.curve` : Defines a curve to be applied to the UI component's value before being applied to the parameter. Refer to [Curves](#curves) for details.
 * `.curve_polarity` : Sets whether to use a unipolar or bipolar type curve for the parameter. Refer to [Curves](#curves) for details.
 * `.min` : Defines the parameter value that corresponds to the minimum point of the controlling UI component.
 * `.max` : Defines the parameter value that corresponds to the maximum point of the controlling UI component.
 * `.value` : Defines the default value to which the parameter should be set before it is actively controlled by an assigned UI component.
 
`.min`, `.max`, and `.value` are expressed in the parameter's value context, and thus must be within the minimum/maximum range of the parameter values defined in the corresponding parameter descriptor. (See [Parameter Descriptors](#parameter-descriptors))

*TIP*: Inverting `.min` and `.max` allows to define an inverted UI mapping, thus controlling the parameter in the opposite direction implied by its parameter descriptor.
 
##### Curves
 
Valid curve values are:

 * `k_genericfx_curve_linear` : Parameter value progresses linearly between `.min` and `.max`.
 * `k_genericfx_curve_exp` : Parameter value follows an exponential-like progression between `.min` and `.max`.
 * `k_genericfx_curve_log` : Parameter value follows a logarithmic-like progression between `.min` and `.max`.
 * `k_genericfx_curve_toggle` : Parameter value is `.min` below the mid-point of the controlling UI component, and `.max` above.
 * `k_genericfx_curve_minclip` : Parameter value is `.min` below the mid-point of the controlling UI component, and grows linearly to `.max` above.
 * `k_genericfx_curve_maxclip` : Parameter value grows linearly from `.min` to `.max` at the mid-point of the controlling UI component, and stays at `.max` above.
 
When `k_genericfx_curve_bipolar` is set, the curves above are aligned to and reflected/inverted on each side of the mid-point of the controlling UI component. (E.g.: logarithmic curves, now looks like the letter `S`)

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
 * `name` allows for a 21 character name. Should be nul-terminated and 7-bit ASCII encoded. Valid characters are: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_`".
 
 *Note* `min`, `max`, `center` and `init` values must take into account the `frac` and `frac_mode` values.

 Even when the number of parameter count is less than the maximum allowed, a descriptor must be provided for each parameter. In order to indicate that a parameter index is not in use, the following parameter descriptor must be used:
 
 ```
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
 ```
 
#### Parameter Types

 The following parameter types are available.
 
 *Note:* Due to the limitations of the NTS-3 kaoss pad kit 7-segment display, most parameter types do not alter the value displayed, yet the most adequate type for a parameter should be used to allow its use in the future.

  * `k_unit_param_type_none` : Describes a typeless value. The value will be displayed as is, while taking into account the fractional part.
  * `k_unit_param_type_percent` : Describe a percent value. 
  * `k_unit_param_type_db` : Describes a decibel value. 
  * `k_unit_param_type_cents` : Describes a pitch cents value. 
  * `k_unit_param_type_semi` : Describes a pitch semitone value. 
  * `k_unit_param_type_oct` : Describes a pitch semitone value. 
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

Here's an overview of the API for generic effect units.

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
 * `__unit_callback void unit_set_param_value(uint8_t index, int32_t value)` : Called to set the current value of the parameter designated by the specified index. Note that for the NTS-3 kaoss pad kit values are stored as 16-bit integers, but to avoid future API changes, they are passed as 32bit integers. For additional safety, make sure to bound check values as per the min/max values declared in the header.
 * `__unit_callback void unit_set_tempo(uint32_t tempo)` : Called when a tempo change occurs. The tempo is formatted in fixed point format, with the BPM integer part in the upper 16 bits, and fractional part in the lower 16 bits (low endian). Care should be taken to keep CPU load as low as possible when handling tempo changes as this handler may be called frequently especially if externally synced.
 * `__unit_callback void unit_tempo_4ppqn_tick_func(uint32_t counter)` : After initialization, the callback may be called at any time to notify the unit of a clock event (4PPQN interval, ie: 16th notes with regards to tempo).
 
### NTS-3 Specific Touch Event Handler
 
The NTS-3 kaoss pad kit requires the following additional API:
 
 * `__unit_callback void unit_touch_event(uint8_t id, uint8_t phase, uint32_t x, uint32_t y)` : After initialization, the callback may be called at any time to notify the unit of a touch event.

This API is called to notify the unit of a touch event, the function parameters are as follows:

 * `id` : The id of the touch to which the event is related. Always 0 for single touch platforms. (NTS-3 kaoss pad kit is single touch)
 * `phase` : Current phase of the touch. See below for a description of touch phases.
 * `x` : X-axis coordinates of touch. (Value within X-axis resolution bounds, see [Runtime Context](#runtime-context))
 * `y` : Y-axis coordinates of touch. (Value within Y-axis resolution bounds, see [Runtime Context](#runtime-context))
 
##### Touch Phases

 * `k_unit_touch_phase_began` : A new touch was detected.
 * `k_unit_touch_phase_moved` : Motion to new coordinates.
 * `k_unit_touch_phase_ended` : End of touch lifecycle.
 * `k_unit_touch_phase_stationary` : Used to force-refresh current coordinates.
 * `k_unit_touch_phase_cancelled` : Touch lifecyle forcibly ended. e.g.: by a UI mode change.

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
 
 * `target` describes the current platform and module. It should be set to: `k_unit_target_nts3_kaoss_genericfx`. The convenience macro `UNIT_TARGET_PLATFORM_IS_COMPAT(target)` can be used to check for compatibility of current unit with the runtime environment.
 * `api` describes the API version currently in use. The version is formatted with the major in the upper 16 bits, minor and patch number in the two lower bytes, respectively (e.g.: v1.2.3 -> 0x00010203U). The convenience macro `UNIT_API_IS_COMPAT(api)` can be used to check for compatibility of current unit with the runtime environment API.
 * `samplerate` describes the sample rate used for audio processing. On NTS-3 kaoss pad kit this should always be set to 48000. However it should be checked and taken into account by the unit. A unit can reject inconvenient samplerates by returning `k_unit_err_samplerate*` from the `unit_init(..)` callback, which will prevent the unit from being fully loaded.
 * `frames_per_buffer` describes the maximum number of frames per audio processing buffer that should be expected in the `unit_render(..)` callback. In general it should always be equal to the `frames` argument of the callback, however, smaller values should be properly supported nonetheless.
 * `input_channels` describes the number of audio channels (samples) per frame in the input buffer of the `unit_render(..)` callback.
 * `output_channels` describes the number of audio channels (samples) per frame in the output buffer of the `unit_render(..)` callback.
 * `hooks` gives access to additional context and callable APIs presented by the runtime environment. See [Runtime Context](#runtime-context) and [Callable API Functions](#callable-api-functions) below.

#### Runtime Context

The NTS-3 kaoss pad kit genericfx runtimes also provide additional contextual information to the unit:

 * `.touch_area_width` : Width of the XY pad area. X-axis touch positions will be bound by this value. (Typically 1024, implying X-axis range of 0-1023)
 * `.touch_area_height` : Height of the XY pad area. Y-axis touch positions will be bound by this value. (Typically 1024, implying X-axis range of 0-1023)

#### Callable API Functions

 * `sdram_alloc` (`uint8_t *sdram_alloc(size_t size)`) allocates buffers of external memory. The allocation should be done during initialization of the unit. (Recommended: ensure 4 byte alignment)
 * `sdram_free` (`void sdram_free(const uint8_t *mem)`) frees external memory buffer previously allocated via `sdram_alloc`.
 * `sdram_avail` (`size_t sdram_avail()`) returns the currently available amount of external memory.

 Additionally, the genericfx runtime provides the following NTS-3 kaoss pad kit specific API:
 
 * `get_raw_input` (`const float *get_raw_input()`) : Get a reference to a raw audio input buffer, unaffected by the current on/off state of the effect.
 
 This API is provided for effects that require pre-buffering of input audio before being enabled by a touch event. (E.g.: loopers, granular effects)
 
 *Note*: The raw input buffer reference should not be reused accross `unit_render` calls.

### Strings

 Strings provided via `unit_get_param_str_value(..)` should be nul terminated C character arrays of 7-bit ASCII characters from the following list: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_`".
 
