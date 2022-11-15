## drumlogue SDK Source and Template Projects

[日本語](./README_ja.md)

### Overview

All source files needed to build custom synths and effects for the [drumlogue](https://www.korg.com/products/drums/drumlogue) exist under this directory.

#### Compatibility Notes

Firmware version >= 1.00 is required to run user units built with SDK version 2.0-0.

#### Overall Structure:
 * [common/](common/) : Common headers.
 * [dummy-synth/](dummy-synth/) : User synth project template.
 * [dummy-delfx/](dummy-delfx/) : User delay effect project template.
 * [dummy-revfx/](dummy-revfx/) : User reverb effect project template.
 * [dummy-masterfx/](dummy-masterfx/) : User master effect project template.

### Setting up the Development Environment (Docker Only)

 Refer to [Docker-based Build Environment](../../docker).

### Building Units

 1. Execute [docker/run_interactive.sh](../../docker/run_interactive.sh)

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```

 1.1. (optional) List buildable projects

```
 user@logue-sdk $ build -l --drumlogue
 - drumlogue/dummy-delfx
 - drumlogue/dummy-masterfx
 - drumlogue/dummy-revfx
 - drumlogue/dummy-synth
 ```

 2. Use the build command with the the desired project's path (E.g. `drumlogue/dummy-synth`)

```
 user@logue-sdk $ build drumlogue/dummy-synth
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

#### `run_cmd.sh` Alternative

 1. Build the desired project by specifying its path to `run_cmd.sh` (E.g. `drumlogue/dummy-synth`)

```
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

#### Final Build Product

The final build product is the *.drmlgunit* file in the project directory (unless an install location was specified via build scripts).

### Cleaning Units

 Cleaning unit projects will remove temporary and final build products.

 1. Execute [docker/run_interactive.sh](../../docker/run_interactive.sh)

```
 $ docker/run_interactive.sh
 user@logue-sdk $ 
 ```
 
 2. Clean the desired project (E.g. `drumlogue/dummy-synth`) 

```
 user@logue-sdk:~$ build --clean drumlogue/dummy-synth 
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/drumlogue/dummy-synth
 
 Cleaning
 rm -fR .dep build /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 ```

#### `run_cmd.sh` Alternative

 1. Clean the desired project (E.g. `drumlogue/dummy-synth`) 

```
 $ ./run_cmd.sh build --clean drumlogue/dummy-synth 
 >> Initializing drumlogue development environment.
 Note: run 'env -r' to reset the environment
 >> Cleaning /workspace/drumlogue/dummy-synth
 
 Cleaning
 rm -fR .dep build /workspace/drumlogue/dummy-synth//dummy_synth.drmlgunit
 
 Done
 
 >> Resetting environment
 >> Cleaning up drumlogue development environment.
 ```

### Using *unit* Files

*.drmlgunit* files can be loaded onto a [drumlogue](https://www.korg.com/products/drums/drumlogue) by powering it up in USB mass storage device mode, and placing the files into the appropriate directory under *Units/*. User synth, delay, reverb and master effect units should be placed in the *Units/Synths/*, *Units/DelayFXs*, *Units/ReverbFXs/* and *Units/MasterFXs*, respectively. The units will be loaded in alphabetical order upon restarting the device.

*TIP* Loading order of units can be forced by prefixing unit file names with a number (e.g.: *01_my_unit.drmlgunit*).

## Creating a New Project

1. Create a copy of a template project directory for the module you are targetting (synth/delfx/revfx/masterfx) and rename it to your convenience inside the *platform/drumlogue/* directory.
2. Customize your project build by editing *config.mk*. See the [config.mk](#config-mk) section for details.
3. Adapt the provided *header.c* template to match your project needs. See the [header.c](#header-c) section for details.
4. Adapt the provided *unit.cc* template to integrate your code with the unit API. See the [unit.cc](#unit-cc) section for details.

## Project Structure

### config.mk

The *config.mk* file allows to declare project source files, includes, libraries and override some build parameters without editing the Makefile directly.

By default the following variables are defined or readily available to be set:

 * `PROJECT` : Project name. Will be used in the file name of the final build project. (Note: does not define the actual name of the unit as displayed on the device when loaded)
 * `PROJECT_TYPE` : Determines which type of project is being built. Valid values are `synth`, `delfx`, `revfx`, and `masterfx`. Make sure it matches the unit type you are developing.
 * `CSRC` : C source files for the project. This list should at least include the `header.c` file.
 * `CXXSRC` : C++ source files for the project. This list should at least inclde the `unit.cc` file.
 * `UINCDIR` : List of additional include directories.
 * `ULIBDIR` : List of additional library search directories.
 * `ULIBS` : List of additional library flags.
 * `UDEFS` : List of additional compile time defines. (e.g.: `-DENABLE\_MY\_FEATURE`)

### header.c

The *header.c* file is similar in structure to the *manifest.json* file of other platforms. It provides some metadata information about the unit and the parameter it exposes. It is compiled along with the unit code and placed in a dedicated ELF section called *.unit_header*, so it can be extracted by using the appropriate objdump or readelf tool.

Field descriptions:

 * `.header_size` : Size of the header structure. Should be left as defined in the template provided.
 * `.target` : Defines the target platform and module. The value provided by the template should be kept, but make sure that the module defined matches the actual intended target unit module. (synth: k\_unit\_module\_synth, delfx: k\_unit\_module\_delfx, revfx: k\_unit\_module\_revfx, masterfx: k\_unit\_module\_masterfx)
 * `.api` : logue SDK API version against which the unit is being built. The default template value ensures the current API value at build time will be used.
 * `.dev_id` : A unique developer identifier as a low endian 32-bit unsigned integer. See [Developer Identifier](#developer-identifier) for details.
 * `.unit_id` : An identifier for the unit itself as a low endian 32-bit unsigned integer. This identifier must only be unique within the scope of a given developer identifier.
 * `.version` : The version for the current unit as a low endian 32-bit unsigned integer, with major in the upper 16 bits, minor and patch number in the two lower bytes, respectively. (e.g.: v1.2.3 -> 0x00010203U)
 * `.name` : Name for the current unit, as displayed on the device when loaded. Nul-terminated array of maximum 13 7-bit ASCII characters. Valid characters are: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?#$%&'()*+,-.:;<=>@`".
 * `.num_presets` : Number of exposed presets by the unit. See [Presets](#presets) for details.
 * `.num_params` : Number of exposed parameters by the unit. Up to 24 parameters can be exposed.
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
 * `name` allows for a 12 character name. Should be nul-terminated and 7-bit ASCII encoded. Valid characters are: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?#$%&'()*+,-.:;<=>@`".
 
 *Note* `min`, `max`, `center` and `init` values must be take into account the `frac` and `frac_mode` values.

 Even when the number of parameter count is less than the maximum allowed, a descriptor must be provided for each 24 parameters. In order to indicate that a parameter index is not in use, the following parameter descriptor must be used:
 
 ```
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
 ```
 
 On the drumlogue, the 24 parameters are layed out over pages of 4 parameters, following the order of parameter indexes. It is possible to leave some parameter indexes unassigned in order to display certain parameters on a later page. To do so, simply introduce an unassigned parameter where the index should be skipped.
 
 ```
 // Page 1
 {0, (100 << 1), 0, (25 << 1), k_unit_param_type_percent, 1, 0, 0, {"PARAM1"}},
 {-5, 5, 0, 0, k_unit_param_type_none, 0, 0, 0, {"PARAM2"}},
 {-100, 100, 0, 0, k_unit_param_type_pan, 0, 0, 0, {"PARAM3"}},
 // blank parameter, leaves that parameter slot blank in UI, and align next parameter to next page
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},

 // Page 2
 {0, 9, 0, 0, k_unit_param_type_strings, 0, 0, 0, {"PARAM4"}},
 {0, 9, 0, 0, k_unit_param_type_bitmaps, 0, 0, 0, {"PARAM5"}},
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
 {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}, 
 
 [...]
 ```

#### Parameter Types

 The following parameter types are available.

  * `k_unit_param_type_none` : Describes a typeless value. The value will be displayed as is, while taking into account the fractional part.
  * `k_unit_param_type_percent` : Describe a percent value. A `%` character will be automatically appended to the displayed value.
  * `k_unit_param_type_db` : Describes a decibel value. `dB`will be automatically appended to the displayed value.
  * `k_unit_param_type_cents` : Describes a pitch cents value. `C` will be automatically appended to the displayed value. Positive values will be prepended with a `+` to indicate it is an offset.
  * `k_unit_param_type_semi` : Describes a pitch semitone value. Positive values will be prepended with a `+` to indicate it is an offset.
  * `k_unit_param_type_oct` : Describes a pitch semitone value. Positive values will be prepended with a `+` to indicate it is an offset.
  * `k_unit_param_type_hertz` : Describes a Hertz value. `Hz` will be automatically appended to the displayed value.
  * `k_unit_param_type_khertz` : Describes a kilo Hertz value. `kHz` will be automatically appended to the displayed value.
  * `k_unit_param_type_bpm` : Describes a beat per minute value.
  * `k_unit_param_type_msec` : Describes a milliseconds value. `ms` will be automatically appended to the displayed value. 
  * `k_unit_param_type_sec` : Describes a seconds value. `s` will be automatically appended to the displayed value. 
  * `k_unit_param_type_enum` : Describes a numerical enumeration value. If the value minimum is set to 0, the value will be incremented by 1 when displayed. 
  * `k_unit_param_type_strings` : Describes a value with custom string representation. The numerical value will be passed in a call to `unit_get_param_str_value(..)` in order to obtain the string representation. See [Strings](#strings) for details.
  * `k_unit_param_type_bitmaps` : Describes a value with custom bitmap representation. The numerical value will be passed in a call to `unit_get_param_bmp_value(..)` in order to obtain the bitmap representation. See [Bitmaps](#bitmaps) for details.
  * `k_unit_param_type_drywet` : Describes a dry/wet value. Negative values will be prepended with `D` for dry, positive values with `W` for wet, and zero value replaced with `BAL` to indicate a balanced mix.
  * `k_unit_param_type_pan` : Describes a stereo pan value. `%` will automatically be appended to the displayed value. Negative values will be prepended with `L` for left, positive values with `R` for right, and zero value replaced with `C` to indicate centered panning.
  * `k_unit_param_type_spread` : Describes a stereo spread value. Negative values will be prepended with `<`, positive values with `>`.
  * `k_unit_param_type_onoff`: Describes an on/off toggle value. `0` will be displayed as `off`, and `1` will be displayed as `on`.
  * `k_unit_param_type_midi_note` : Describes a MIDI note value. The numerical note value will be displayed as musical pitches (e.g.: `C0`, `A3`).

## Unit API Overview

Here's an overview of the API for synth and effect units.

### Essential Functions

All units must provide an implementation for the following functions. However, a fallback implementation is provided by default, so only the relevant functions needs to be explicitely provided.

 * `__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc)` : Called after unit is loaded. Should be used to perform basic checks on runtime environment, initialize the unit, and perform any dynamic memory allocations if needed. `desc` provides a description of the current runtime environment (See [Runtime Descriptor](#runtime-descriptor) for details).
 * `__unit_callback void unit_teardown()` : Called before unit is unloaded. Should be used to perform cleanup and freeing memory if needed.
 * `__unit_callback void unit_reset()` : Called when unit must be reset to a neutral state. Active notes must be deactivated, enveloppe generators reset to their neutral state, oscillator phases reset, delay lines set to be cleared (clearing at once may be to heavy). However, parameter values *should not be* reset to their default values.
 * `__unit_callback void unit_resume()` : Called just before a unit will start processing again after being suspended.
 * `__unit_callback void unit_suspend()` : Called when a unit is being suspended. For instance, when the currently active unit is being swapped for a different unit. Usually followed by a call to `unit_reset()`.
 * `__unit_callback void unit_render(const float * in, float * out, uint32_t frames)` : Audio rendering callback. Synth units should ignore the `in` argument. Input/output buffer geometry information is provided via the `unit_runtime_desc_t` argument of `unit_init(..)`.
 * `__unit_callback uint8_t unit_get_preset_index()` : Should return the preset index currently used by the unit.
 * `__unit_callback const char * unit_get_preset_name(uint8_t index)` : Called to obtain the name of given preset index. The returned value should point to a nul-terminated 7-bit ASCII C string of maximum X characters. The displayable characters are the same as for the unit name.
 * `__unit_callback void unit_load_preset(uint8_t index)` : Called to load the preset designated by the specified index. See [Presets](#presets) for details.
 * `__unit_callback int32_t unit_get_param_value(uint8_t index)` : Called to obtain the current value of the parameter designated by the specified index.
 * `__unit_callback const char * unit_get_param_str_value(uint8_t index, int32_t value)` : Called to obtain the string representation of the specified value, for a `k_unit_param_type_strings` typed parameter. The returned value should point to a nul-terminated 7-bit ASCII C string of maximum X characters. It can be safely assumed that the C string pointer will not be cached or reused after `unit_get_param_str_value(..)` is called again, and thus the same memory area can be reused across calls (if convenient).
 * `__unit_callback const uint8_t * unit_get_param_bmp_value(uint8_t index, int32_t value)` : Called to obtain the bitmap representation of the specified value, for a `k_unit_param_type_bitmaps` typed parameter. It can be safely assumed that the pointer will not be cached or reused after `unit_get_param_bmp_value(..)` is called again, and thus the same memory area can be reused across calls (if convenient). For details concerning bitmap data format see [Bitmaps](#bitmaps).
 * `__unit_callback void unit_set_param_value(uint8_t index, int32_t value)` : Called to set the current value of the parameter designated by the specified index. Note that for the drumlogue values are stored as 16-bit integers, but to avoid future API changes, they are passed as 32bit integers. For additional safety, make sure to bound check values as per the min/max values declared in the header.
 * `__unit_callback void unit_set_tempo(uint32_t tempo)` : Called when a tempo change occurs. The tempo is formatted in fixed point format, with the BPM integer part in the upper 16 bits, and fractional part in the lower 16 bits (low endian). Care should be taken to keep CPU load as low as possible when handling tempo changes as this handler may be called frequently especially if externally synced.
 
### Synth Unit Specific Functions
 
 * `__unit_callback void unit_note_on(uint8_t note, uint8_t velocity)` : Called upon MIDI note on events, and upon internal sequencer gate on events if an explicit `unit_gate_on(..)` handler is not provided, in which case note will be set to 0xFFU. `velocity` is a 7-bit value.
 * `__unit_callback void unit_note_off(uint8_t note)` : Called upon MIDI note off events, and upon internal sequencer gate off events if an explicit `unit_gate_off(..)` handler is not provided, in which case note will be set to 0xFFU.
 * `__unit_callback void unit_gate_on(uint8_t velocity)` (optional) : If provided, will be called upon internal sequencer gate on events. `velocity` is a 7-bit value.
 * `__unit_callback void unit_gate_off(void)` (optional) : If provided, will be called upon internal sequencer gate off events.
 * `__unit_callback void unit_all_note_off(void)` : When called all active notes should be deactivated and enveloppe generators reset.
 * `__unit_callback void unit_pitch_bend(uint16_t bend)` : Called upon MIDI pitch bend event. `bend` is a 14-bit value with neutral center at 0x2000U. Sensitivity can be defined according to the unit's needs.
 * `__unit_callback void unit_channel_pressure(uint8_t pressure)` : Called upon MIDI channel pressure events. `pressure` is a 7-bit value.
 * `__unit_callback void unit_aftertouch(uint8_t note, uint8_t aftertouch)` : Called upon MIDI aftertouch events. `afterotuch` is a 7-bit value.
 
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
  unit_runtime_get_num_sample_banks_ptr get_num_sample_banks;
  unit_runtime_get_num_samples_for_bank_ptr get_num_samples_for_bank;
  unit_runtime_get_sample_ptr get_sample;
 } unit_runtime_desc_t;
 ```
 
 * `target` describes the current platform and module. It should be set to one of: `k_unit_target_drumlogue_delfx`, `k_unit_target_drumlogue_revfx`, `k_unit_target_drumlogue_synth`, `k_unit_target_drumlogue_masterfx`. The convenience macro `UNIT_TARGET_PLATFORM_IS_COMPAT(target)` can be used to check for compatibility of current unit with the runtime environment.
 * `api` describes the API version currently in use. The version is formatted with the major in the upper 16 bits, minor and patch number in the two lower bytes, respectively (e.g.: v1.2.3 -> 0x00010203U). The convenience macro `UNIT_API_IS_COMPAT(api)` can be used to check for compatibility of current unit with the runtime environment API.
 * `samplerate` describes the sample rate used for audio processing. On drumlogue this should always be set to 48000. However it should be checked and taken into account by the unit. A unit can reject inconvenient samplerates by returning `k_unit_err_samplerate*` from the `unit_init(..)` callback, which will prevent the unit from being fully loaded.
 * `frames_per_buffer` describes the maximum number of frames per audio processing buffer that should be expected in the `unit_render(..)` callback. In general it should always be equal to the `frames` argument of the callback, however, smaller values should be properly supported nonetheless.
 * `input_channels` describes the number of audio channels (samples) per frame in the input buffer of the `unit_render(..)` callback.
 * `output_channels` describes the number of audio channels (samples) per frame in the output buffer of the `unit_render(..)` callback.

#### Callable API Functions

 * `get_num_sample_banks` (`uint8_t get_num_sample_banks()`) allows to obtain the number of sample banks available on the device.
 * `get_num_samples_for_bank` (`uint8_t get_num_samples_for_bank(uint8_t bank_idx)`) allows to obtain the number of samples for a given bank index.
 * `get_sample` (`const sample_wrapper_t * get_sample(uint8_t bank_idx, uint8_t sample_idx)`) allows to obtain a reference to a sample wrapper for the given bank and sample indexes.

#### Sample Wrappers

 ```
 typedef struct sample_wrapper {
  uint8_t bank;
  uint8_t index;
  uint8_t channels;
  uint8_t _padding;
  char name[UNIT_SAMPLE_WRAPPER_MAX_NAME_LEN + 1];
  size_t frames;
  const float * sample_ptr;
 } sample_wrapper_t;
 ```

 * `bank` is the bank index in which this sample resides.
 * `index` is the sample index within the bank for this sample.
 * `channels` describes the number of audio channels for this sample (e.g.: `1`: mono, `2` : stereo). Note that audio data is interlaced into frames in the same way as the `unit_render(..)` callback buffers.
 * `_padding` should be ignored.
 * `name` is the sample's name, bounded to 31 characters (+ NUL termination).
 * `frames` is the number of audio frames in the sample data.
 * `sample_ptr` is a pointer to the sample data itself. Data length in floats can be obtained by multiplying `frames` with `channels`.

### Presets

 Units can expose presets by setting `.num_presets` to a non-zero value in the [header structure](#header-c), and implementing the `unit_get_preset_index(..)`, `unit_get_preset_name(..)` and `unit_load_preset(..)` callbacks, for the corresponding preset indexes.
 When presets are exposed, a preset selection UI will be displayed.
 
 Loading a preset should be considered as setting a base configuration on top of which exposed parameters can be further modified. Modifying the exposed parameters should not cause the current preset index to change. However, loading a preset can cause exposed parameters to change value as a side effect.

### Strings

 Strings provided via `unit_get_param_str_value(..)` should be nul terminated C character arrays of 7-bit ASCII characters from the following list: "` ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?#$%&'()*+,-.:;<=>@`".
 
 Parameter value strings can be effectively up to 32 characters long, however, the displayable area is narrow and strings that exceed the area will be truncated with `...`.

### Bitmaps

 *Note* Bitmap API is only supported on drumlogue firmware v1.1.0 and up.
 
 Bitmaps are displayed as 16x16 black and white pixels, with origin in the upper left corner. The bitmap data provided via `unit_get_param_bmp_value(..)` should be formatted in 16x16 1bpp packed bitmap format, that is as a 32 byte array with each pair of bytes representing a single row of 16 pixels (0 for black, 1 for white), starting from the top row. Each byte is processed from the least significant bit to the most, displaying corresponding pixels from left to right.

 A tutorial on how to create such bitmap data using [Gimp](https://www.gimp.org/) can be found [here](https://zilogic.com/creating-glcd-bitmaps-using-gimp/).
 
 Alternatively, a tutorial using MS Paint is also available [here](https://exploreembedded.com/wiki/Displaying_Images_and_Icons_on_GLCD).

 Here a few examples:
 
 *White square*
 ```
 const uint8_t bmp[32] = {
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU,
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU, 
   0xFFU, 0xFFU
 };
 ```
 
 *Alternating horizontal lines*
 ```
 const uint8_t bmp[32] = {
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U,
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U, 
   0xFFU, 0xFFU, 
   0x00U, 0x00U
 };
 ```

 *Alternating vertical lines*
 ```
 const uint8_t bmp[32] = {
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU,
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU, 
   0xAAU, 0xAAU
 };
 ```

 *Box outline with diagonal between upper left and lower right corner*
  ```
 const uint8_t bmp[32] = {
   0xFFU, 0xFFU, 
   0x03U, 0x80U, 
   0x05U, 0x80U, 
   0x09U, 0x80U, 
   0x11U, 0x80U, 
   0x21U, 0x80U,
   0x41U, 0x80U, 
   0x81U, 0x80U, 
   0x01U, 0x81U, 
   0x01U, 0x82U, 
   0x01U, 0x84U, 
   0x01U, 0x88U,
   0x01U, 0x90U, 
   0x01U, 0xA0U, 
   0x01U, 0xC0U, 
   0xFFU, 0xFFU
 };
 ```


